# Dev Notes

I still need to figure out how to get lbuild/modm to correctly generate openocd.cfg. Until then, you need to add: 

`source [find board/st_nucleo_l4.cfg]`

to modm/openocd.cfg. 
