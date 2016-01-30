STC\-1000 ESP8266 WiFi 
======================

The purpose of this project is to combine the cheap WiFi capable ESP8266 with the STC-1000 (specifically running the STC-1000+ communication firmware). The ESP8266 replaces the Arduino in this setup and opens up for a lot of new possibilities.

This project is still in its infancy and is not yet ready for public use.

The ESP8266 needs to run a custom firmware. This is implemented using the awesome [Sming framework](https://github.com/SmingHub/Sming). To be able to build (and modify) this project, a working Sming development environment needs to be setup first.

Hardware
========

* STC\-1000 (A400_P hardware version)
* ESP8266 module (ESP-12E recommended, ESP-12 and ESP-07 will work)
* 3 zener diodes 3.3v
* 3 resistors 330 ohm
* 20k resistor
* (a schottky diode)
* an USB serial adapter that can do 3.3v and preferrably has RTS and DTR signals broken out
* a 3.3v power source that can provide 300mA minimum

A list of the connections:

|STC\-1000|ESP8266|
|--------|-------|
|nMCLR | GPIO14 |
|VDD | VCC |
|GND | GND |
|ICSPDAT | GPIO12 |
|ICSPCLK/COM | GPIO13 |

|Serial|ESP8266|
|------|-------|
|VCC | 3.3V |
|DTR | GPIO0 |
|RXD | TXD |
|TXD | RXD |
|RTS | CH_PD |
|GND | GND |

Note that the ESP is not 5v tolerant, so levelshifting (using zeners) is required.
Heres the schematic of my setup, sorry for the crappy image.

![Schematic](http://i67.tinypic.com/2v2famu.jpg)
