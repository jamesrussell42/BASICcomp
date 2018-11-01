#!/bin/bash

# Creates libopencm3 based environment for Blue Pill STM32F103C8 board
# Based on https://gist.github.com/Ariki/de7c39b458f8f3d5868716185529923a

EXAMPLES_MASTER=\
"https://raw.githubusercontent.com/libopencm3/libopencm3-examples/master"

git clone https://github.com/libopencm3/libopencm3

cd libopencm3
make
cd ..

wget "${EXAMPLES_MASTER}/examples/rules.mk" -O libopencm3.rules.mk
  
wget "${EXAMPLES_MASTER}/examples/stm32/f1/Makefile.include" \
  -O libopencm3.target.mk

sed -i 's|include ../../../../rules.mk|include ../libopencm3.rules.mk|g' \
  libopencm3.target.mk
