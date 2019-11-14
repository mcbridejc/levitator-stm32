#include "modm/platform/adc/adc_1.hpp"
#include "modm/processing/resumable.hpp"

using PwmTimer = Timer16;

class PhaseMeasurement {
public:
    static void initialize() {
        Adc1::initialize();
        Adc1::connect<GpioInputA3::In8, GpioInputA4::In9>();

        setDifferentialMode(8, true);
        setWatchdog1(8, 0, 2047);
        setWatchdog2(8, 1900 >> 4, 4095>>4);
        
        Adc1::setChannel(Adc1::Channel::Channel8);
        Adc1::setFreeRunningMode(true);
        Adc1::startConversion();

        Adc1::enableInterrupt(Adc1::Interrupt::AnalogWatchdog1);
        Adc1::enableInterrupt(Adc1::Interrupt::AnalogWatchdog2);
        Adc1::enableInterruptVector(1);

        GpioA5::setOutput();
    }

    static bool isLocked()
    {
        uint32_t delta = modm::Clock::now().getTime() - lastEdgeTime;
        if(delta < 500) {
            return true;
        } else {
            return false;
        }
    }

    static uint16_t getPhase() 
    {
        constexpr int32_t offset = (2.1e-6) * 80e6 + 470;
        constexpr uint16_t counterOvf = 2000;
        return (lastPwmTime - offset + 1000) % counterOvf;
    }

    static void setDifferentialMode(uint8_t channel, bool differential) {
        if(channel <= 15) {
            if(differential) {
                ADC1->DIFSEL |= (1<<(channel));
            } else {
                ADC1->DIFSEL &= ~(1<<(channel));
            }
        }
    }

    static void setWatchdog1(uint8_t channel, uint16_t low_guard, uint16_t high_guard) {
        uint32_t tmp;

        tmp = ADC1->CFGR & ~(ADC_CFGR_AWD1CH);
        tmp |= (channel << ADC_CFGR_AWD1CH_Pos);
        tmp |= ADC_CFGR_AWD1EN;
        ADC1->CFGR = tmp;
        ADC1->TR1 = (high_guard << 16) | low_guard;
    }

    static void setWatchdog2(uint8_t channel, uint16_t low_guard, uint16_t high_guard) {
        ADC1->AWD2CR = (1<<channel);
        ADC1->TR2 = (high_guard << 16) | low_guard;
    }

    static void stopSampling() {
        ADC1->CR |= ADC_CR_ADSTP;
        while(ADC1->CR & ADC_CR_ADSTP) { }
    }

    static inline void WatchdogISR() {
        uint32_t flags = ADC1->ISR;
        if(activeFlag && (flags & ADC_ISR_AWD1) != 0)  {
            lastPwmTime = PwmTimer::getValue();
            GpioA5::set(true);
            watchDogCounter++;
            lastEdgeTime = modm::Clock::now().getTime();
            stopSampling();
            Adc1::disableInterrupt(Adc1::Interrupt::AnalogWatchdog1);
            Adc1::enableInterrupt(Adc1::Interrupt::AnalogWatchdog2);
            Adc1::startConversion();
            GpioA5::set(false);
            activeFlag = false;
        } else if(!activeFlag && (flags & ADC_ISR_AWD2)) {
            GpioA5::set(true);
            stopSampling();
            Adc1::disableInterrupt(Adc1::Interrupt::AnalogWatchdog2);
            Adc1::enableInterrupt(Adc1::Interrupt::AnalogWatchdog1);
            Adc1::startConversion();
            GpioA5::set(false);
            activeFlag = true;
        }
        Adc1::acknowledgeInterruptFlag(Adc1::InterruptFlag::AnalogWatchdog1 | Adc1::InterruptFlag::AnalogWatchdog2);   
    }

    static inline bool activeFlag = true;
    static inline uint32_t watchDogCounter = 0;
    static inline uint32_t lastPwmTime = 0;
    static inline uint32_t lastEdgeTime = 0;
    static inline uint32_t lastAdcValue = 0;
};

MODM_ISR(ADC1) {
    PhaseMeasurement::WatchdogISR();
}
