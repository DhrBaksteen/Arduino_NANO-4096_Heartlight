# Arduino NANO-4096 Heartlight
This repository contains the code and other required files that allow you to play the classic PC game [Heartlight](http://www.mobygames.com/game/dos/heartlight) by Janusz Pelc on an Arduino Nano. For more information visit [the Hackaday.io project page](https://hackaday.io/project/9132-nano-4096).

![](https://raw.githubusercontent.com/DhrBaksteen/Arduino_NANO-4096_Heartlight/master/nano-4096.jpg)

## Requirements:
To build the NANO-4096 you need the following components that can be found cheap on eBay.
* Arduino Nano
* SD card breakout board
* LM15SGFNZ07 LCD screen
* Piezo speaker
* 8 Push buttons
* 5 5.6k resistors
* 1uF electrolytic capacitor

## How to make it work?
1. Follow the instructions on my [https://hackaday.io/project/9132-nano-4096](https://hackaday.io/project/9132-nano-4096) project page to build the NANO-4096.
2. Download the required libraries and place them in the arduino `libraries` folder:
  1. [LM15SGFNZ07 LCD](https://github.com/DhrBaksteen/Arduino-SPI-LM15SGFNZ07-LCD-Library) 
  2. [SdFat](https://github.com/greiman/SdFat)
3. Place files in the `sd` folder on your SD card
4. Compile the `heartlight.ino` code and upload to your Arduino
