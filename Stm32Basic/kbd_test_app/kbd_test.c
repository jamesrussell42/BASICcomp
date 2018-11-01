/*
 *  Based on libopencm3 library
 *  https://github.com/libopencm3/libopencm3
 */
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include "../hal_src/hal.h"

const char ver[] = "\r\nKBD tester, v.0.27\r\n";
const char key_map[ROWS][COLS] =
{//COL 0  1   2   3   4   5   6   7   8   9  10  11  12  13  14  15    ROW
    {'5','4','_','3','5','6','_','2','_','_','1','_','.','.','.','1'}, // 0
    {'T','R','_','E','_','_','_','W','_','_','1','_','.','.','.','Q'}, // 1
    {'6','7','_','8','_','_','_','9','_','_','1','_','.','.','.','0'}, // 2
    {'G','F','_','D','_','_','_','S','_','_','1','_','.','.','.','A'}, // 3
    {'Y','U','_','I','_','_','_','O','_','_','1','_','.','.','.','P'}, // 4
    {'V','C','_','X','_','_','_','Z','_','_','1','_','.','.','.','s'}, // 5
    {'H','J','_','K','_','_','_','L','_','_','1','_','.','.','.','e'}, // 6
    {'B','N','_','M','_','_','_','.','_','_','1','_','.','.','.','p'}, // 7
};

const char row_str[] = "== ROW:";
const char col_str[] = ", COL:";
const char key_str[] = ", key:";

/* Global variables */
volatile int value_ROW = 1;

int main(void)
{
    clock_setup();
    init_KBD();
    usart_setup();

    usart_send_string(USART1, ver, sizeof(ver));

    while(1)
    {
        int i;
        int value_COL = 0;
        char key_value = 0;
        int row, col;

        set_KBD2(value_ROW);

        for (i = 0; i < 5000; i++)              /* Wait a bit. */
            __asm__("NOP");

        value_COL = read_KBD1();


        if(value_COL != 0)
        {
            row = value_ROW - 1;
            col = value_COL - 1;
            key_value = key_map[row][col];

            usart_send_string(USART1, row_str, sizeof(row_str));
            uart_write_number(USART1, row);
            usart_send_string(USART1, col_str, sizeof(col_str));
            uart_write_number(USART1, col);
            usart_send_string(USART1, key_str, sizeof(key_str));
            usart_send_blocking(USART1, key_value);
            usart_send_blocking(USART1, '\r');
            usart_send_blocking(USART1, '\n');
        }

        value_ROW++;
        if(value_ROW > ROWS)
            value_ROW = 1;

        for (i = 0; i < 210000; i++)         /* Wait a bit. */
            __asm__("NOP");
    }

    return 0;
}

