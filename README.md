# levitator-stm32

Software for my ultrasonic levitator controller. It drives a 40khz squarewave, reads an ADC input, and displays the phase relationship between the generated and input waveforms on an I2C bargraph LED. 

## Dev Notes

I still need to figure out how to get lbuild/modm to correctly generate openocd.cfg. Until then, you need to add: 

`source [find board/st_nucleo_l4.cfg]`

to modm/openocd.cfg. 
