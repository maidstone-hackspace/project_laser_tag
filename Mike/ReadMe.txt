Arduino Based LaserTag System

I'm using a Teensy 3.2 as the extra processing power will come in handy when I later add a colour TFT display and a sound processor.

So far I have:

Trigger button on Pin 2
Reload Button on Pin 6
RGB LED on Pins, 21, 22, 23
IR LED (20 degree) on Pin 5
Piezo Buzzer on Pin 4
16x2 LCD DIsplay on pins 11,12,10,9,8,7
Muzzle Flash LED's on Pin 3
Up menu button on Pin 14
Down menu button on Pin 13
Enter menu button on pin 15
IR Receiver on Pn 16
HIT LED on Pin 17

The data packet is 32 bits:

0..6     7-bit  : Damage (using a 128 element look-up table for damage) 
7..12     6-bit  : Player ID (64 Players per team)
13..15    3-bit  : Team ID (8 teams - Red, Blue, Green, Yellow, Cyan, magenta, White, Solo)
16..31   16-bit : CRC-16

At some point in the future the code will need to be re-written as a Finite State Machine to allow multi-tasking. 

Required libraries:

<Button.h>      https://github.com/JChristensen/Button
<IRRemote.h>    https://github.com/z3t0/Arduino-IRremote
<FastCRC.h>     https://github.com/FrankBoesing/FastCRC




