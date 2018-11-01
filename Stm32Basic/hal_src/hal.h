#ifndef _HAL_H_
#define _HAL_H_

#ifndef PCBASIC_TARGET
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/systick.h>
#endif

/********************** Keyboard **********************/
#ifdef KEYPAD_8x5_IN_USE
/* Port A */
#define KB1_5                                   GPIO8
#define KB1_4                                   GPIO3
#define KB1_3                                   GPIO2
#define KB1_2                                   GPIO1
#define KB1_1                                   GPIO0

/* Port B */
#define KB2_6                                   GPIO0
#define KB2_4                                   GPIO1
#define KB2_2                                   GPIO10
#define KB2_1                                   GPIO11
#define KB2_3                                   GPIO13
#define KB2_5                                   GPIO14
#define KB2_7                                   GPIO15
#define KB2_8                                   GPIO8

#define ROWS                                    8
#define COLS                                    16
#endif

/********************** UART **********************/
#define UART_SPEED                              38400

/********************** I2C **********************/
/* Port B */
#define LCD_I2C_SCL                             GPIO6
#define LCD_I2C_SDA                             GPIO7

/********************** LCD **********************/
#ifdef I2C_LCD1602_LCD_20x4_DISPLAY_IN_USE
#define LCD_SCREEN_WIDTH                        20
#define LCD_SCREEN_HEIGHT                       4
#define LCD_SERIAL_ADDRESS                      0x27
#endif

/********************** Misc. **********************/
#define STRING_NULL                             400
#define ONE_SECOND                              10000

/* Port C */
#define LED_GPIO                                GPIO13

void clock_setup(void);
void systick_setup(void);
void delay_us100(uint32_t us100);
void usart_setup(void);
void usart_send_string(uint32_t usart, const char *string, uint16_t str_size);
void uart_write_number(uint32_t usart, uint32_t num);
void i2c_setup(void);
void i2c_write_byte(uint8_t address, uint8_t data);
void init_KBD(void);
void reset_KBD2(void);
void set_KBD2(int pin);
uint16_t read_KBD1(void);
#endif
