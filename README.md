# BNO08X Hatire Head Tracker

This project assumes I2C for communicating with the *BNO08X*.  
SPI is untested because frankly, I can't be bothered breaking what is working.  
*It might be faster though?*

My setup uses an Arduino Pro Micro and a transistor-based level shifter.  
I've tried a TXS0108E but it either arrived dead or wasn't fit for my purpose.  
You ***should*** level shift unless your board has 3.3V logic.

![Full I2C schematics](/res/schematics-i2c.png)

Performance-wise, this project gets a ~375 Hz update rate; ~360 without `FAST_MATH`.  
The *BNO08X* is capable of 1000 Hz as set up here, so grab the fastest AVR MCU you can.  
Unsure if the deciding factor is FLOPS, IOPS or raw clock speed,
but anything generally more powerful than an *ATmega32U4* is gonna be good.

Tested with OpenTrack in numerous games.  
There is [a bug in OpenTrack](https://github.com/opentrack/opentrack/discussions/2100) causing jitter.

[BNO08X Datasheet (CEVA)](https://www.ceva-ip.com/wp-content/uploads/BNO080_085-Datasheet.pdf)
