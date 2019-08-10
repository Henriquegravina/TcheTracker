# TcheTracker

A APRS tracker based on Arduino hardware, and the Tcheduino shield or MicroModem

Tcheduino is a shield developed by PY3NZ and PU3MSR to advance APRS in Brazil with very low cost.


To compile this code you will need: 
 - LibAPRS[https://github.com/markqvist/LibAPRS]
 - TinyGPSPlus[https://github.com/mikalhart/TinyGPSPlus]

**By default this code use the only one serial port on ATmega328 to comunicate with GPS and to configure the device.**

1 - to put he device in config mode you will need to put a jumper in Digital IO 11 to VCC;

2 - to enter in trasnmit mode you ne to put a jumper from Digital IO 11 to GND;

**to use it without a gps module, you have to set SPEED1, SPEED2 and SPEED3 to 0;**

3 - You need to use a gps with GPS sentences with: $GPRMC. Only Glonass senteces will not work.

4 - This traker is base on LibAPRS: https://unsigned.io/projects/libaprs/ / https://github.com/markqvist/LibAPRS in this link you could get more info about hardware.

5 - PTT pin is Digital IO 9 of Arduino.





