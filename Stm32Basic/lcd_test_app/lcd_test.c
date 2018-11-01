/*
 *  Based on libopencm3 library
 *  https://github.com/libopencm3/libopencm3
 */
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include "../hal_src/hal.h"
#include "../lcd_src/i2c_lcd.h"

const char ver[] = "\r\nLCD tester, v.0.13\r\n";
const char teststr[] = " Stm32Basic";

int main(void)
{
    int i;
    clock_setup();
    init_KBD();
    usart_setup();
    i2c_setup();

    lcd_init(
        LCD_SERIAL_ADDRESS,
        LCD_SCREEN_WIDTH,
        LCD_SCREEN_HEIGHT);

    lcd_begin(LCD_SCREEN_HEIGHT, 0);

    usart_send_string(USART1, ver, sizeof(ver));

    /* Test of LCD backlight */
    usart_send_string(USART1, "=== Test of LCD backlight\r\n", STRING_NULL);

    for(i = 0; i < 6; i++)
    {
        delay_us100(ONE_SECOND);                   /* 1 sec */
        if (i % 2)
        {
            lcd_no_backlight();
            usart_send_string(USART1, "Backlight OFF\r\n", STRING_NULL);
        }
        else
        {
            lcd_backlight();
            usart_send_string(USART1, "Backlight ON\r\n", STRING_NULL);
        }
    }

    delay_us100(ONE_SECOND);
    lcd_backlight();
    
    /* Test of LCD cursor */
    usart_send_string(USART1, "=== Test of LCD cursor\r\n", STRING_NULL);
    
    lcd_set_cursor(0, 0);
    lcd_write_str("@0,0");
    lcd_write_str(teststr);
    lcd_set_cursor(1, 1);
    lcd_write_str("@1,1");
    lcd_write_str(teststr);   
    lcd_set_cursor(2, 2);
    lcd_write_str("@2,2");
    lcd_write_str(teststr);     
    lcd_set_cursor(3, 3);
    lcd_write_str("@3,3");
    lcd_write_str(teststr);     
    delay_us100(ONE_SECOND * 3);
    usart_send_string(USART1, "=== Test of LCD clean\r\n", STRING_NULL);
    lcd_clear();
    delay_us100(ONE_SECOND * 3);
    
    usart_send_string(USART1, "Backlight OFF\r\n", STRING_NULL);
    lcd_no_backlight();
    
    usart_send_string(USART1, "Ok\r\n", STRING_NULL);
    return 0;
}

