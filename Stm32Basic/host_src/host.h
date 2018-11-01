#ifndef _HOST_H_
#define _HOST_H_
#include <stdint.h>
#include <stdbool.h>

#ifdef PCBASIC_TARGET
#ifdef WIN32
#include <Windows.h>
#elif _POSIX_C_SOURCE >= 199309L
#include <time.h>       /* For nanosleep */
#else
#include <unistd.h>     /* For usleep */
#endif
#endif

#ifndef WIN32
#include <unistd.h>     /* _getch */
#include <termios.h>    /* _getch */
#endif

#define pgm_read_byte(x)                        (*(x))
#define pgm_read_word(x)                        (*(x))
#define pgm_read_float(x)                       (*(x))
#define pgm_read_byte_near(x) (*(x))

#ifndef PCBASIC_TARGET
#ifdef I2C_LCD1602_LCD_20x4_DISPLAY_IN_USE
#define SCREEN_WIDTH                            20
#define SCREEN_HEIGHT                           4
#ifdef FLASHING_CURSOR_IN_USE
#define CURSOR_CHR                              255
#else
#define CURSOR_CHR                              '_'
#endif
#endif
#else
#define SCREEN_WIDTH                            20
#define SCREEN_HEIGHT                           8
#define CURSOR_CHR                              95
#ifdef WIN32
#define CHAR_CR                                 13
#define CHAR_DELETE                             8
#else
#define CHAR_CR                                 10  /* TODO */
#define CHAR_DELETE                             127
#endif /* WIN32 */
#define CHAR_ESC                                27
#endif

#ifdef KEYPAD_8x5_IN_USE
#define KEY_ENTER                               13
#define KEY_SHIFT                               1
#define KEY_DELETE                              127
#define ROWS                                    8
#define COLS                                    16
#define TIMER2_PERIOD                           180     /* Microseconds */
#define KEYPRESS_TIMEOUT                        1000
#endif /* KEYPAD_8x5_IN_USE */

#ifdef SD_CARD_IN_USE
#define SD_CHIP_SELECT                          PA4
#endif

#ifdef BUZZER_IN_USE
#define BUZZER_PIN    0		/* TODO */
#else
/* Buzzer pin, 0 = disabled/not present */
#define BUZZER_PIN    0
#endif

#define MAGIC_AUTORUN_NUMBER                    0xFC
#define TIMER1_PRELOAD                          34286

#ifdef KEYPAD_8x5_IN_USE
enum E_keys_state
{
    E_set_row_1st = 0,
    E_read_col_1st,
    E_set_row_2nd,
    E_read_col_2nd
};
#endif

#ifdef PCBASIC_TARGET
void SetCursorToPos(int x, int y);
void ShowConsoleCursor(bool showFlag);
void ShowPrompt();
#endif

void host_init(int buzzerPin);
void host_sleep(long ms);
void host_digitalWrite(int pin,int state);
int host_digitalRead(int pin);
int host_analogRead(int pin);
void host_pinMode(int pin, int mode);
void host_click(void);
void host_startupTone(void);
void host_cls(void);
void scroll_buffer(void);
void host_showBuffer(void);
void host_moveCursor(int x, int y);
void host_output_string(char *str);
void host_outputProgMemString(const char *str);
void host_output_char(char c);
void host_output_float(float f);
char *host_float_to_str(float f, char *buf);
int host_output_int(long val);
void host_new_line(void);
char *host_readLine(void);
char host_getKey(void);
uint8_t host_esc_pressed(void);
void host_outputFreeMem(unsigned int val);
void host_save_program(uint8_t autoexec);
void host_loadProgram(void);

#ifdef KEYPAD_8x5_IN_USE
void handler_timer_int(void);
void state_machine_init(void);
void state_machine_set_row_1st(void);
void state_machine_read_column_1st(void);
void state_machine_set_row_2nd(void);
void state_machine_read_column_2nd(void);
char get_key(void);
#endif /* KEYPAD_8x5_IN_USE */

#ifdef PCBASIC_TARGET
#ifndef WIN32
char lingetch(void);
#endif /* WIN32 */
#endif /* PCBASIC_TARGET */

#ifdef SERIAL_TRACES_ON
void host_trace_str(const char *string, uint16_t str_size);
void host_trace_int(uint32_t int_num);
#endif

#ifdef SD_CARD_IN_USE
bool host_saveSdCard(char *fileName);
bool host_loadSdCard(char *fileName);
#endif
#endif /* _HOST_H_ */
