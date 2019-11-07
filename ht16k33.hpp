
#pragma once

#include <modm/debug/logger.hpp>
#include "modm/architecture/interface/i2c_transaction.hpp"
#include "modm/architecture/interface/i2c_device.hpp"

#define HT16K33_BUFFER_SIZE 16

#define HT16K33_NESTEDDEPTH 15

template <typename I2cMaster>
class ht16k33 : public modm::I2cDevice< I2cMaster, HT16K33_NESTEDDEPTH, modm::I2cWriteTransaction > {
public:
    ht16k33(uint8_t address = 0x70) : modm::I2cDevice< I2cMaster, HT16K33_NESTEDDEPTH, modm::I2cWriteTransaction >(address) {
        buffer[0] = 0;
        for(uint8_t i=1; i<HT16K33_BUFFER_SIZE + 1; i++) {
            buffer[i] = 0xff;
        }
    }

    modm::ResumableResult<void> initialize();

    /** Synchronously write a command byte */
    modm::ResumableResult<bool> writeByte(uint8_t value);

    void clear() {
        for(int i=1; i<HT16K33_BUFFER_SIZE + 1; i++) {
            buffer[i] = 0;
        }
    }
    void setLed(uint8_t n) {
        uint8_t byte = n / 8;
        uint8_t bit = n % 8;
        if(byte > HT16K33_BUFFER_SIZE) {
            return;
        }
        buffer[byte+1] |= 1<<bit;
    }
    void clearLed(uint8_t n) {
        uint8_t byte = n / 8;
        uint8_t bit = n % 8;
        if(byte > HT16K33_BUFFER_SIZE) {
            return;
        }
        buffer[byte+1] &= ~(1<<bit);
    }

    modm::ResumableResult<bool>
    write() {
        RF_BEGIN();
        this->transaction.configureWrite(buffer, sizeof(buffer));
        RF_END_RETURN_CALL(this->runTransaction());
    }

private: 
    uint8_t buffer[HT16K33_BUFFER_SIZE + 1];
}; 

template<typename I2cMaster>
modm::ResumableResult<void>
ht16k33<I2cMaster>::initialize() {
    RF_BEGIN();
    RF_CALL(writeByte(0x21)); // Turn on oscillator
    RF_CALL(writeByte(0x81));  // Turn on display with no blink
    RF_END();
}

template<typename I2cMaster>
modm::ResumableResult<bool>
ht16k33<I2cMaster>::writeByte(uint8_t value) {
    RF_BEGIN();
    this->transaction.configureWrite(&value, 1);
    RF_CALL(this->runTransaction());
    RF_END_RETURN(true);
}