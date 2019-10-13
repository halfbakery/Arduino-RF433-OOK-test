# Arduino-RF433-OOK-test

Experimental simple OOK decoder for the (rtl_433)[https://github.com/merbanan/rtl_433] project. I never ment this to be an officional or recommended version, just a testbed for new ideas. If it works, better integration with the rtl_433 project will be possible.


## How to use

Compile and download the sketch and run:

$ socat OPEN:/dev/ttyACM0,b115200,echo=0,echoe=0,echok=0,echoctl=0,echoke=0,onlcr=0,raw - | rtl_433 -r ook:-
