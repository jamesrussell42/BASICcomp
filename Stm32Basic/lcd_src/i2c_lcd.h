#ifndef LCD_H
#define LCD_H

#include <stdint.h>

/* Commands */
#define LCD_CLEARDISPLAY                    0x01
#define LCD_RETURNHOME                      0x02
#define LCD_ENTRYMODESET                    0x04
#define LCD_DISPLAYCONTROL                  0x08
#define LCD_CURSORSHIFT                     0x10
#define LCD_FUNCTIONSET                     0x20
#define LCD_SETCGRAMADDR                    0x40
#define LCD_SETDDRAMADDR                    0x80

/* Flags for display entry mode */
#define LCD_ENTRYRIGHT                      0x00
#define LCD_ENTRYLEFT                       0x02
#define LCD_ENTRYSHIFTINCREMENT             0x01
#define LCD_ENTRYSHIFTDECREMENT             0x00

/* Flags for display on/off control */
#define LCD_DISPLAYON                       0x04
#define LCD_DISPLAYOFF                      0x00
#define LCD_CURSORON                        0x02
#define LCD_CURSOROFF                       0x00
#define LCD_BLINKON                         0x01
#define LCD_BLINKOFF                        0x00

/* Flags for display/cursor shift */
#define LCD_DISPLAYMOVE                     0x08
#define LCD_CURSORMOVE                      0x00
#define LCD_MOVERIGHT                       0x04
#define LCD_MOVELEFT                        0x00

/* Flags for function set */
#define LCD_8BITMODE                        0x10
#define LCD_4BITMODE                        0x00
#define LCD_2LINE                           0x08
#define LCD_1LINE                           0x00
#define LCD_5x10DOTS                        0x04
#define LCD_5x8DOTS                         0x00

/* Flags for backlight control */
#define LCD_BACKLIGHT                       0x08
#define LCD_NOBACKLIGHT                     0x00

#define En                                  0x04 /* Enable bit */
#define Rw                                  0x02 /* Read/Write bit */
#define Rs                                  0x01 /* Register select bit */

void lcd_init(uint8_t lcd_addr,uint8_t lcd_cols,uint8_t lcd_rows);
void lcd_begin(uint8_t rows, uint8_t charsize);
void lcd_command(uint8_t cmd);
void lcd_display(void);
void lcd_clear(void);
void lcd_home(void);
void lcd_no_backlight(void);
void lcd_backlight(void);
void lcd_set_cursor(uint8_t col, uint8_t row);
void lcd_write_str(const char *message);
void lcd_write_byte(uint8_t byte);
#endif  /* LCD_H */
