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

BarGraph<Board::DisplayI2c> LEDS;

class MainThread : public modm::pt::Protothread 
{
public:
	bool
    run()
    {	
		PT_BEGIN();
		PT_CALL(LEDS.initialize());
		while(true) {
			modm::delayMilliseconds(100);
			modm::log::warning << "Loop\n";
			Board::PwmTimer::setOverflow(Board::EncoderTimer::getValue());
			Board::PwmTimer::applyAndReset();
			LEDS.clear();
			LEDS.setBar(position, BarGraphColor::YELLOW);
			position = (position+1)%24;
			PT_CALL(LEDS.write());
		}
		PT_END();
    }

private: 
	uint8_t position;
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
	Board::EncoderTimer::setValue(1000);
	Board::EncoderTimer::setMode(Board::EncoderTimer::Mode::UpCounter, Board::EncoderTimer::SlaveMode::Encoder1, AdvancedControlTimer::SlaveModeTrigger::Internal0);
	Board::EncoderTimer::configureInputChannel(1, Board::EncoderTimer::InputCaptureMapping::InputOwn, Board::EncoderTimer::InputCapturePrescaler::Div1, Board::EncoderTimer::InputCapturePolarity::Rising, 0);
	Board::EncoderTimer::configureInputChannel(2, Board::EncoderTimer::InputCaptureMapping::InputOwn, Board::EncoderTimer::InputCapturePrescaler::Div1, Board::EncoderTimer::InputCapturePolarity::Rising, 0);
	Board::EncoderTimer::start();

	Board::PwmTimer::enable();
	Board::PwmTimer::setMode(Board::PwmTimer::Mode::UpCounter);
	Board::PwmTimer::setPrescaler(1);
	Board::PwmTimer::setOverflow(2000);
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
