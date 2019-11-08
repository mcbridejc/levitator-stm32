/*
 * Copyright (c) 2014, Georgi Grinshpun
 * Copyright (c) 2014, Sascha Schade
 * Copyright (c) 2015-2017, 2019 Niklas Hauser
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

#include <board.hpp>
#include <modm/debug/logger.hpp>
#include <modm/processing/protothread.hpp>

#include <cmath>

#include "bargraph.hpp"

using namespace modm::platform;
using namespace modm::literals;

using namespace modm::platform;

// Instantiate logger objects
Board::LoggerDevice loggerDevice;
modm::log::Logger modm::log::debug(loggerDevice);
modm::log::Logger modm::log::info(loggerDevice);
modm::log::Logger modm::log::warning(loggerDevice);
modm::log::Logger modm::log::error(loggerDevice);

static bool buttonFlag = false;

MODM_ISR(EXTI15_10) {
	Board::Button::acknowledgeExternalInterruptFlag();
	buttonFlag = true;
}

template <typename I2cMaster>
class TunerDisplay : public BarGraph<I2cMaster>
{
public:
	TunerDisplay(float low_limit, float high_limit) : BarGraph<I2cMaster>(), limL(low_limit), limH(high_limit) {}

	void setValue(float value) {
		if(value > limH) {
			value = limH;
		} else if(value < limL) {
			value = limL;
		}
		
		float led_pos = 23.0 * (value - limL) / (limH - limL);
		float remainder = fmod(led_pos, 1);
		float delta = fabs(led_pos - 11.5);
		BarGraphColor color;
		if(delta < 1) {
			color = BarGraphColor::GREEN;
		} else if(delta < 6) {
			color = BarGraphColor::YELLOW;
		} else {
			color = BarGraphColor::RED;
		}

		this->clear();
		if(remainder < 0.33) {
			this->setBar((uint8_t)led_pos, color);
		} else if(remainder < 0.66) {
			this->setBar((uint8_t)led_pos, color);
			this->setBar((uint8_t)led_pos + 1, color);
		} else {
			this->setBar((uint8_t)led_pos + 1, color);
		}
	}
private:
	float limL;
	float limH;
};


#define FREQ_START 1000
#define FREQ_MIN 800
#define FREQ_MAX 1200


TunerDisplay<Board::DisplayI2c> Display(FREQ_MIN, FREQ_MAX);

class MainThread : public modm::pt::Protothread 
{
public:
	bool
    run()
    {	
		PT_BEGIN();
		PT_CALL(Display.initialize());

		while(true) {
			freq = Board::EncoderTimer::getValue();
			if(buttonFlag) {
				buttonFlag = false;
				freq = FREQ_START;
				Board::EncoderTimer::setValue(freq);
			}
			else if(freq < FREQ_MIN) {
				freq = FREQ_MIN;
				Board::EncoderTimer::setValue(freq);
			} else if(freq > FREQ_MAX) {
				freq = FREQ_MAX;
				Board::EncoderTimer::setValue(freq);
			}
			Board::PwmTimer::setOverflow(freq);
			//Board::PwmTimer::applyAndReset();
			Display.setValue(freq);
			// Display.clear();
			// Display.setBar(position, BarGraphColor::YELLOW);
			//position = (position+1)%24;
			PT_CALL(Display.write());

			modm::delayMilliseconds(100);
		}
		PT_END();
    }

private: 
	uint8_t position;
	uint16_t freq;
};


// ----------------------------------------------------------------------------
int
main()
{
	Board::initialize();

	
	MainThread mainThread;
	
	// auto result = LEDS.ping();
	// if(result.getResult()) {
	// 	modm::log::warning << "Ping succeeded\n";
	// } else {
	// 	modm::log::warning << "Ping succeeded\n";
	// }

	Board::EncoderTimer::enable();
	Board::EncoderTimer::setValue(FREQ_START);
	Board::EncoderTimer::setMode(Board::EncoderTimer::Mode::UpCounter, Board::EncoderTimer::SlaveMode::Encoder1, AdvancedControlTimer::SlaveModeTrigger::Internal0);
	Board::EncoderTimer::configureInputChannel(1, Board::EncoderTimer::InputCaptureMapping::InputOwn, Board::EncoderTimer::InputCapturePrescaler::Div1, Board::EncoderTimer::InputCapturePolarity::Rising, 0);
	Board::EncoderTimer::configureInputChannel(2, Board::EncoderTimer::InputCaptureMapping::InputOwn, Board::EncoderTimer::InputCapturePrescaler::Div1, Board::EncoderTimer::InputCapturePolarity::Rising, 0);
	Board::EncoderTimer::start();

	Board::PwmTimer::enable();
	Board::PwmTimer::setMode(Board::PwmTimer::Mode::UpCounter);
	Board::PwmTimer::setPrescaler(1);
	Board::PwmTimer::setOverflow(FREQ_START);
	Board::PwmTimer::configureOutputChannel(
		 1,
		 Board::PwmTimer::OutputCompareMode::Toggle,
		 1
	);
	Board::PwmTimer::start();

	// Total hack here. I think the modm timer driver doesn't properly support TIM15/16 on the L4 series, which had some but not all
	// of the advanced timer features (e.g. a BDTR register). I want to look into updating modm drivers, but here's a stopgap. 
	TIM16->BDTR |= (1<<15); // Set MOE bit
	TIM16->CCER = 0x5; // Enable Ch1 and Ch1n

	while(true) {
		mainThread.run();
	}
	return 0;
}
