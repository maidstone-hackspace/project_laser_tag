Arduino Based LaserTag System

I'm using a Teensy 3.2 as the extra processing power will come in handy when I later add a colour TFT display and a sound processor.

So far I have:

Trigger button on Pin 2
IR LED (20 degree) on Pin 5
Piezo Buzzer on Pin 4
16x2 LCD DIsplay on pins 11,12,10,9,8,7
Muzzle Flash LED's on Pin 3
Up menu button on Pin 14
Down menu button on Pin 13
Enter menu button on pin 15

The data packet is 32 bits:

0..3     4-bit  : Damage (using a look-up table for 1-100 damage)
4..8     5-bit  : Player ID (32 Players per team)
9..10    2-bit  : Team ID (4 teams - Red, Blue, Yellow, Green)
11..15   5-bit  : Unused. For future expansion. 
16..31   16-bit : CRC-16



