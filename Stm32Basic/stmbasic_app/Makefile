BINARY = stm32basic

OBJS = ../hal_src/hal.o
OBJS += ../lcd_src/i2c_lcd.o
OBJS += ../host_src/host.o
OBJS += ../basic_src/basic.o

DEFS += -DKEYPAD_8x5_IN_USE
DEFS += -DI2C_LCD1602_LCD_20x4_DISPLAY_IN_USE

OPENCM3_DIR=../libopencm3
LDSCRIPT = $(OPENCM3_DIR)/lib/stm32/f1/stm32f103x8.ld

include ../libopencm3.target.mk
