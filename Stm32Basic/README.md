### Retro computer with BASIC interpreter (GCC/libopencm3 toolchain version)
_Based on https://github.com/robinhedwards/ArduinoBASIC Arduino-BASIC_

* MCU: STM32F103C8T6 (72 MHz, 20 KB RAM, 64 KB Flash)
* 10 x 4 keyboard 
* 20x4 LCD Alphanumeric Display with I2C interface

## Setting environment (examples are for Ubuntu host)
* Get ARM Toolchain:

`sudo apt-get install gcc-arm-none-eabi binutils-arm-none-eabi gdb-arm-none-eabi`

* Get OpenOCD:

`apt-get install openocd`

* Clone this Stm32Basic repository, get libopencm3 library and build keyboard test:

`cd Stm32Basic`

`./get_libopencm3.sh`

* Add -lm in to user libraries (libopencm3.rules.mk)

`LDLIB += .... -lm`

* Build keyboard test

`cd kbd_test_app`

`make V=1`

### UNDER CONSTRUCTION
_Wiring_

_Practical stuff_

openocd -f ./interface/stlink-v2.cfg -f ./target/stm32f1x.cfg



