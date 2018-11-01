/* Based on the work by DFRobot */

#include <stdio.h>
#include <string.h>
#include "i2c_lcd.h"
#include "../hal_src/hal.h"

typedef struct
{
    uint8_t _addr;
    uint8_t _displayfunction;
    uint8_t _displaycontrol;
    uint8_t _displaymode;
    uint8_t _numlines;
    uint8_t _cols;
    uint8_t _rows;
    uint8_t _backlightval;
} LCDHANDLE;

static void write_4_bits(uint8_t value);
static void pulse_enable(uint8_t data);
static void expander_write(uint8_t data);
static void send(uint8_t value, uint8_t mode);
static inline void lcd_data_write(uint8_t data);

/* Global variables */
static LCDHANDLE lcdhdl;

/*
 When the display powers up, it is configured as follows:
 1. Display clear
 2. Function set:
    DL = 1; 8-bit interface data
    N = 0; 1-line display
    F = 0; 5x8 dot character font
 3. Display on/off control:
    D = 0; Display off
    C = 0; Cursor off
    B = 0; Blinking off
 4. Entry mode set:
    I/D = 1; Increment by 1
    S = 0; No shift
*/

void lcd_init(uint8_t lcd_addr,uint8_t lcd_cols,uint8_t lcd_rows)
{
    memset(&lcdhdl, 0, sizeof(lcdhdl));
    lcdhdl._addr = lcd_addr;
    lcdhdl._cols = lcd_cols;
    lcdhdl._rows = lcd_rows;
    lcdhdl._backlightval = LCD_BACKLIGHT; //LCD_NOBACKLIGHT;
}

void lcd_begin(uint8_t rows, uint8_t charsize)
{
    if (rows > 1)
    {
        lcdhdl._displayfunction |= LCD_2LINE;
    }

    lcdhdl._numlines = rows;

    /* For some 1-line displays you can select a 10 pixel high font */
    if ((charsize != 0) && (rows == 1))
    {
        lcdhdl._displayfunction |= LCD_5x10DOTS;
    }
    else
    {
        lcdhdl._displayfunction |= LCD_5x8DOTS;
    }

    /* SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
       According to datasheet, we need at least 40ms after power rises above 2.7V
       before sending commands */
    delay_us100(500);                           /* 50 ms */

    /* Now we pull both RS and R/W low to begin commands */
    expander_write(lcdhdl._backlightval);       /* Reset expanderand turn backlight off (Bit 8 = 1) */
    delay_us100(10000);                         /* 1 sec */

    /* Put the LCD into 4 bit mode
       this is according to the hitachi HD44780 datasheet figure 24, pg 46.
       We start in 8bit mode, try to set 4 bit mode */

    write_4_bits(0x03 << 4);
    delay_us100(50);                            /* Wait min 4.1ms */

    /* Second try */
    write_4_bits(0x03 << 4);
    delay_us100(50);                            /* Wait min 4.1ms */

    /* Third go! */
    write_4_bits(0x03 << 4);
    delay_us100(2);

    /* Finally, set to 4-bit interface */
    write_4_bits(0x02 << 4);

    /* Set # lines, font size, etc. */
    lcd_command(LCD_FUNCTIONSET | lcdhdl._displayfunction);

    /* Turn the display on with no cursor or blinking default */
    lcdhdl._displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    lcd_display();

    /* Clear it off */
    lcd_clear();

    /* Initialize to default text direction (for roman languages) */
    lcdhdl._displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

    /* Set the entry mode */
    lcd_command(LCD_ENTRYMODESET | lcdhdl._displaymode);

    lcd_home();
}

inline void lcd_command(uint8_t value)
{
    send(value, 0);
}

void lcd_display(void)
{
    lcdhdl._displaycontrol |= LCD_DISPLAYON;
    lcd_command(LCD_DISPLAYCONTROL | lcdhdl._displaycontrol);
}

void lcd_clear(void)
{
    lcd_command(LCD_CLEARDISPLAY);  /* Clear display, set cursor position to zero */
    delay_us100(20);                /* This command takes a long time! */
}

void lcd_home(void)
{
    lcd_command(LCD_RETURNHOME);    /* Set cursor position to zero */
    delay_us100(20);                /* This command takes a long time! */
}

void lcd_set_cursor(uint8_t col, uint8_t row)
{
    int32_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};

    if (row > lcdhdl._numlines)
    {
        row = lcdhdl._numlines - 1;  /* We count rows starting w/0 */
    }

    lcd_command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

void lcd_write_str(const char *message)
{
    uint8_t *message_p = (uint8_t *)message;

    while (*message_p)
        lcd_data_write((uint8_t)(*message_p++));
}

void lcd_write_byte(uint8_t byte)
{
    lcd_data_write(byte);
}

/* Turn the (optional) backlight off/on */
void lcd_no_backlight(void)
{
    lcdhdl._backlightval = LCD_NOBACKLIGHT;
    expander_write(0);
}

void lcd_backlight(void)
{
    lcdhdl._backlightval = LCD_BACKLIGHT;
    expander_write(0);
}

/* Private functions */
static inline void lcd_data_write(uint8_t data)
{
    send(data, Rs);
}

static void write_4_bits(uint8_t value)
{
    expander_write(value);
    pulse_enable(value);
}

static void pulse_enable(uint8_t data)
{
    expander_write(data | En);      /* En high */
    delay_us100(1);                 /* Enable pulse must be >450ns */
    expander_write(data & ~En);     /* En low */
    delay_us100(1);                 /* Commands need > 37us to settle */
}

static void expander_write(uint8_t data)
{
    i2c_write_byte(lcdhdl._addr, data | lcdhdl._backlightval);
}

/* Write either command or data */
static void send(uint8_t value, uint8_t mode)
{
    uint8_t highnib = value & 0xf0;
    uint8_t lownib = (value << 4) & 0xf0;
    write_4_bits((highnib) | mode);
    write_4_bits((lownib) | mode);
}
