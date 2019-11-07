#include "ht16k33.hpp"


enum BarGraphColor { RED, GREEN, YELLOW };

/** Thin wrapper on ht16k33 to map LED outputs on the chip to bargraph position/color */
template <typename I2cMaster>
class BarGraph : public ht16k33<I2cMaster>
{
public:
    void setBar(uint8_t n, BarGraphColor c) {
        uint8_t led_pos;
        if (n < 12)
            led_pos = (n / 4) * 16 ;
        else 
            led_pos = ((n - 12) / 4) * 16;

        led_pos += n % 4;
        if (n >= 12)
            led_pos += 4;
        if(c == RED) {           
            this->setLed(led_pos);
        } else if(c == GREEN) {
            this->setLed(led_pos + 8);
        } else {
            this->setLed(led_pos);
            this->setLed(led_pos + 8);
        }
    }
};