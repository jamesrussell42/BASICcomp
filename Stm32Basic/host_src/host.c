/*
 *  Based on Robin Edwards Arduino-BASIC
 *  https://github.com/robinhedwards/ArduinoBASIC
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "host.h"
#include "../hal_src/hal.h"
#include "../basic_src/basic.h"

#ifndef PCBASIC_TARGET
#ifdef I2C_LCD1602_LCD_20x4_DISPLAY_IN_USE
#include "../lcd_src/i2c_lcd.h"
#endif
#endif

#ifdef SD_CARD_IN_USE
/* #include <SPI.h>  TODO
#include <SD.h> */
#endif

/* Global variables */
char screenBuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
char lineDirty[SCREEN_HEIGHT];
int curX = 0, curY = 0;
volatile char flash = 1, redraw = 0;
char inputMode = 0;
char inkeyChar = 0;
#ifdef BUZZER_IN_USE
char buzPin = 0
#endif

#ifdef PCBASIC_TARGET
volatile uint8_t cur_x = 1;
#endif

#ifdef SD_CARD_IN_USE
/* bool sd_card_ok = false; TODO */
#endif

#ifdef KEYPAD_8x5_IN_USE
volatile uint8_t value_ROW = 1;
volatile uint8_t COLs_array[ROWS];
volatile uint8_t key_check_state_machine = E_set_row_1st;
volatile bool key_pressed = false;
volatile bool key_not_printed = true;
volatile uint16_t keypress_timeout_counter = 0;

// TODO
const char key_map[ROWS][COLS] =
{//COL 0  1   2   3   4   5   6   7   8   9  10  11  12  13  14  15           ROW
    {'1','2','_','3','5','6','_','4','_','_','1','_','.','.','.','5'},        // 0
    {'Q','W','_','E','_','_','_','R','_','_','1','_','.','.','.','T'},        // 1
    {'0','9','_','8','_','_','_','7','_','_','1','_','.','.','.','6'},        // 2
    {'A','S','_','D','_','_','_','F','_','_','1','_','.','.','.','G'},        // 3
    {'P','O','_','I','_','_','_','U','_','_','1','_','.','.','.','Y'},        // 4
    {KEY_SHIFT,'Z','_','X','_','_','_','C','_','_','1','_','.','.','.','V'},  // 5
    {KEY_ENTER,'L','_','K','_','_','_','J','_','_','1','_','.','.','.','H'},  // 6
    {' ','.','_','M','_','_','_','N','_','_','1','_','.','.','.','B'},        // 7
};

const char key_map_shift[ROWS][COLS] =
{//COL 0  1   2   3   4   5   6   7   8   9  10  11  12  13  14  15           ROW
    {'a','s','d','f','g','f','g','n','m','g','h','j','k','l','g','b'},        // 0
    {'x','z','q','w','e','f','n','m','n','b','v','d','a','s','d','f'},        // 1
    {KEY_DELETE,'s','f','g','h','j','k','x','c','v','b','n','f','d','s','a'}, // 2
    {'v','b','q','w','e','r','t','y','d','s','c','v','b','g','f','d'},        // 3
    {'"',')','d','(','f','g','n','?','k','j','h','g','f','s','a','s'},        // 4
    {'q','w','e','r','t','y','u','i','o','p','z','x','c','v','b','w'},        // 5
    {'a','=','d','s','a','r','t','y','u','i','o','b','f','d','s','e'},        // 6
    {'q',',','e','>','t','y','u','<','f','g','h','x','w','e','r','*'},        // 7
};
#endif

const char bytesFreeStr[] = "bytes free";

#ifndef PCBASIC_TARGET
#ifdef KEYPAD_8x5_IN_USE
void handler_timer_int(void)
{
    switch(key_check_state_machine)
    {
        case E_set_row_1st:
            state_machine_set_row_1st();
            break;

        case E_read_col_1st:
            state_machine_read_column_1st();
            break;

        case E_set_row_2nd:
            state_machine_set_row_2nd();
            break;

        case E_read_col_2nd:
            state_machine_read_column_2nd();
            break;

        default:
            break;
    }
}
#endif
#endif

#ifdef PCBASIC_TARGET
void ShowConsoleCursor(bool showFlag)
{
#ifdef WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_CURSOR_INFO     cursorInfo;

    GetConsoleCursorInfo(out, &cursorInfo);
    cursorInfo.bVisible = showFlag;
    SetConsoleCursorInfo(out, &cursorInfo);
#else
    printf("\e[?25l");
    fflush(stdout);
#endif /* WIN32 */
}

void SetCursorToPos(int x, int y)
{
#ifdef WIN32
    COORD newPosition = {x, y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), newPosition);
#else
    printf("\033[%d;%dH", y + 1, x + 1);
    fflush(stdout);
#endif /* WIN32 */
}

void ShowPrompt()
{
    uint8_t k;

    SetCursorToPos(0, SCREEN_HEIGHT);

    for(k = 0; k < SCREEN_WIDTH; k++)
    {
        printf("-");
    }

    SetCursorToPos(0, SCREEN_HEIGHT + 1);
    printf(">");

    SetCursorToPos(1, SCREEN_HEIGHT + 1);
    for(k = 0; k < SCREEN_WIDTH << 1; k++)
    {
        printf(" ");
    }

#ifndef WIN32
    fflush(stdout);
#endif
}
#endif /* PCBASIC_TARGET */

void host_init(int buzzerPin)
{
    buzzerPin = buzzerPin; /* TODO */

#ifndef PCBASIC_TARGET
#ifdef BUZZER_IN_USE
    /* buzPin = buzzerPin; TODO */
#endif

    clock_setup();
    host_sleep(500);
    init_KBD();

#ifdef SERIAL_TRACES_ON
    usart_setup();
    host_sleep(500);
#endif

    i2c_setup();

#ifdef I2C_LCD1602_LCD_20x4_DISPLAY_IN_USE
    lcd_init(
        LCD_SERIAL_ADDRESS,
        SCREEN_WIDTH,
        SCREEN_HEIGHT);

    lcd_begin(SCREEN_HEIGHT, 0);
    lcd_clear();
    lcd_set_cursor(0, 0);
#endif

#ifdef KEYPAD_8x5_IN_USE
    init_KBD();
    reset_KBD2();
#endif

#ifdef BUZZER_IN_USE
    if (buzPin)
        /* pinMode(buzPin, OUTPUT); TODO */
#endif

#ifdef SD_CARD_IN_USE
    SPI.setClockDivider(SPI_CLOCK_DIV64);
    delay(200);
    if(!SD.begin(SD_CHIP_SELECT))
    {
#ifdef SERIAL_TRACES_ON
        /* Serial1.println("SD card error"); */
#endif
    }
#endif
#endif

#ifdef PCBASIC_TARGET
    ShowConsoleCursor(false);
    SetCursorToPos(0, 0);
#endif
}

void host_sleep(long ms)
{
#ifdef PCBASIC_TARGET
#ifdef WIN32
    Sleep(ms);
#elif _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
#else
    usleep(ms * 1000);
#endif
#else
    delay_us100(ms * 10);
#endif
}

void host_digitalWrite(int pin, int state)
{
    /* TODO */
    pin = pin;
    state = state;
}

int host_digitalRead(int pin)
{
    pin = pin;
    return 0;       /* TODO*/
}

int host_analogRead(int pin)
{
    pin = pin;
    return 0;       /* TODO */
}

void host_pinMode(int pin,int mode)
{
    /* TODO */
    pin = pin;
    mode = mode;
}

#ifdef BUZZER_IN_USE
void host_click()
{
/* TODO
    if (!buzPin) return;

    digitalWrite(buzPin, HIGH);
    delay_us100(1);
    digitalWrite(buzPin, LOW);
*/
}

void host_startupTone()
{
    if (!buzPin) return;

    for (int i=1; i<=2; i++)
    {
        for (int j=0; j<50*i; j++)
        {
            digitalWrite(buzPin, HIGH);
            delay(3-i);
            digitalWrite(buzPin, LOW);
            delay(3-i);
        }

        delay(100);
    }
}
#endif

void host_cls()
{
    memset(screenBuffer, 32, SCREEN_WIDTH * SCREEN_HEIGHT);
    memset(lineDirty, 1, SCREEN_HEIGHT);
    curX = 0;
    curY = 0;

#ifdef PCBASIC_TARGET
    ShowPrompt();
#endif
}

void host_moveCursor(int x, int y)
{
    if (x < 0)
        x = 0;

    if (x >= SCREEN_WIDTH)
        x = SCREEN_WIDTH - 1;

    if (y < 0)
        y = 0;

    if (y >= SCREEN_HEIGHT)
        y = SCREEN_HEIGHT - 1;

    curX = x;
    curY = y;
}

void host_showBuffer()
{
    for (int y = 0; y < SCREEN_HEIGHT; y++)
    {
        if (lineDirty[y] || (inputMode && y == curY))
        {
#ifndef PCBASIC_TARGET
#ifdef I2C_LCD1602_LCD_20x4_DISPLAY_IN_USE
            lcd_set_cursor(0, y);
#endif
#else
            SetCursorToPos(0, y);
#endif
            for (int x = 0; x < SCREEN_WIDTH; x++)
            {
                char c = screenBuffer[y * SCREEN_WIDTH + x];
                if (c < 32)
                {
                    c = ' ';
                }

                if (x == curX && y == curY && inputMode && flash)
                {
                    c = CURSOR_CHR;
                }

#ifndef PCBASIC_TARGET
#ifdef I2C_LCD1602_LCD_20x4_DISPLAY_IN_USE
                lcd_write_byte(c);
#endif
#else
                printf("%c", c);
#endif
            }

            lineDirty[y] = 0;
        }
    }
}

void scroll_buffer(void)
{
    memcpy(screenBuffer, screenBuffer + SCREEN_WIDTH, SCREEN_WIDTH * (SCREEN_HEIGHT - 1));
    memset(screenBuffer + SCREEN_WIDTH * (SCREEN_HEIGHT - 1), 32, SCREEN_WIDTH);
    memset(lineDirty, 1, SCREEN_HEIGHT);
    curY--;
}

void host_output_string(char *str)
{
    int pos = curY * SCREEN_WIDTH + curX;
    while (*str)
    {
        lineDirty[pos / SCREEN_WIDTH] = 1;
        screenBuffer[pos++] = *str++;
        if (pos >= SCREEN_WIDTH * SCREEN_HEIGHT)
        {
            scroll_buffer();
            pos -= SCREEN_WIDTH;
        }
    }

    curX = pos % SCREEN_WIDTH;
    curY = pos / SCREEN_WIDTH;
}

void host_outputProgMemString(const char *p)
{
    while (1)
    {
        unsigned char c = *(p++);
        if (c == 0)
            break;

        host_output_char(c);
    }
}

void host_output_char(char c)
{
    int pos = curY * SCREEN_WIDTH + curX;
    lineDirty[pos / SCREEN_WIDTH] = 1;

    screenBuffer[pos++] = c;
    if (pos >= SCREEN_WIDTH * SCREEN_HEIGHT)
    {
        scroll_buffer();
        pos -= SCREEN_WIDTH;
    }

    curX = pos % SCREEN_WIDTH;
    curY = pos / SCREEN_WIDTH;
}

int host_output_int(long num)
{
    /* Returns len */
    long i = num, xx = 1;
    int c = 0;
    do {
        c++;
        xx *= 10;
        i /= 10;
    }
    while (i);

    for (int z = 0; z < c; z++)
    {
        xx /= 10;
        char digit = ((num/xx) % 10) + '0';
        host_output_char(digit);
    }

    return c;
}

char *host_float_to_str(float f, char *buf)
{
    /* Floats have approx 7 sig figs */
    float a = (double)fabs(f);

    if (f == 0.0f)
    {
        buf[0] = '0';
        buf[1] = 0;
    }
    else if (a < 0.0001 || a > 1000000)
    {
#if 1 /* TODO */
        sprintf(buf, "%f", f);
#else
        /* This will output -1.123456E99 = 13 characters max including trailing nul */
        dtostre(f, buf, 6, 0);
#endif
    }
    else
    {
        int decPos = 7 - (int)(floor(log10(a))+1.0f);

#if 1 /* TODO */
        sprintf(buf, "%f", f);
#else
        dtostrf(f, 1, decPos, buf);
#endif

        if (decPos)
        {
            /* Remove trailing 0s */
            char *p = buf;
            while (*p) p++;
            p--;
            while (*p == '0')
            {
                *p-- = 0;
            }

            if (*p == '.')
                *p = 0;
        }
    }

    return buf;
}

void host_output_float(float f)
{
    char buf[16];
    host_output_string(host_float_to_str(f, buf));
}

void host_new_line()
{
    curX = 0;
    curY++;

    if (curY == SCREEN_HEIGHT)
        scroll_buffer();

    memset(screenBuffer + SCREEN_WIDTH * (curY), 32, SCREEN_WIDTH);
    lineDirty[curY] = 1;
}

char *host_readLine()
{
    inputMode = 1;

    if (curX == 0)
        memset(screenBuffer + SCREEN_WIDTH * (curY), 32, SCREEN_WIDTH);
    else
        host_new_line();

    int startPos = curY * SCREEN_WIDTH + curX;
    int pos = startPos;
    bool done = false;

    while (!done)
    {
#ifdef KEYPAD_8x5_IN_USE
        if(key_not_printed && key_pressed)
#endif
        {
#ifdef BUZZER_IN_USE
            host_click();
#endif
            /* Read the next key */
            lineDirty[pos / SCREEN_WIDTH] = 1;

#ifndef PCBASIC_TARGET
#ifdef KEYPAD_8x5_IN_USE
            char c = get_key();

            if(c != 0)
            {
                key_not_printed = false;
                keypress_timeout_counter = 0;

                if (c >= 32 && c <= 126)
                    screenBuffer[pos++] = c;
                else if (c == KEY_DELETE && pos > startPos)
                    screenBuffer[--pos] = 0;
                else if (c == KEY_ENTER)
                    done = true;

                redraw = 1;
            }
#endif /* KEYPAD_8x5_IN_USE */
#else
#ifdef WIN32
            char c = (char)getch();

            if (c >= 32 && c <= 126)
            {
                SetCursorToPos(cur_x++, SCREEN_HEIGHT + 1);
                printf("%c", c);
                screenBuffer[pos++] = c;
            }
            else if (c == CHAR_DELETE && pos > startPos)
            {
                SetCursorToPos(--cur_x, SCREEN_HEIGHT + 1);
                printf("  ");
                SetCursorToPos(cur_x, SCREEN_HEIGHT + 1);
                screenBuffer[--pos] = 0;
            }
            else if (c == CHAR_CR)
            {
                ShowPrompt();
                cur_x = 1;
                done = true;
            }
#else
            char c = lingetch();

            if (c >= 32 && c <= 126)
            {
                SetCursorToPos(cur_x++, SCREEN_HEIGHT + 1);
                printf("%c", c);
                screenBuffer[pos++] = c;
            }
            else if (c == CHAR_DELETE && pos > startPos)
            {
                SetCursorToPos(--cur_x, SCREEN_HEIGHT + 1);
                printf("  ");
                SetCursorToPos(cur_x, SCREEN_HEIGHT + 1);
                screenBuffer[--pos] = 0;
            }
            else if (c == CHAR_CR)
            {
                ShowPrompt();
                cur_x = 1;
                done = true;
            }

            fflush(stdout);
#endif /* WIN32 */
#endif /* PCBASIC_TARGET */

            curX = pos % SCREEN_WIDTH;
            curY = pos / SCREEN_WIDTH;

            /* Scroll if needed to */
            if (curY == SCREEN_HEIGHT)
            {
                if (startPos >= SCREEN_WIDTH)
                {
                    startPos -= SCREEN_WIDTH;
                    pos -= SCREEN_WIDTH;
                    scroll_buffer();
                }
                else
                {
                    screenBuffer[--pos] = 0;
                    curX = pos % SCREEN_WIDTH;
                    curY = pos / SCREEN_WIDTH;
                }
            }
        }

        if (redraw)
            host_showBuffer();
    }

    screenBuffer[pos] = 0;
    inputMode = 0;

    /* Remove the cursor */
    lineDirty[curY] = 1;
    host_showBuffer();
    return &screenBuffer[startPos];
}

char host_getKey()
{
    char c = inkeyChar;
    inkeyChar = 0;

    if (c >= 32 && c <= 126)
        return c;
    else
        return 0;
}

uint8_t host_esc_pressed()
{
    return false; /* TODO */
}

void host_outputFreeMem(unsigned int val)
{
    host_new_line();
    host_output_int(val);
    host_output_char(' ');
    host_outputProgMemString(bytesFreeStr);

#ifndef PCBASIC_TARGET
#ifdef SERIAL_TRACES_ON
    host_trace_int(val);
    host_trace_str(bytesFreeStr, sizeof(bytesFreeStr));
#endif
#endif
}

void host_save_program(uint8_t autoexec)
{
    autoexec = autoexec;
#if 0 /* TODO */
    EEPROM.write(0, autoexec ? MAGIC_AUTORUN_NUMBER : 0x00);
    EEPROM.write(1, sysPROGEND & 0xFF);
    EEPROM.write(2, (sysPROGEND >> 8) & 0xFF);

    for (int i = 0; i < sysPROGEND; i++)
        EEPROM.write(3 + i, mem[i]);
#endif
}

void host_loadProgram()
{
#if 0 /* TODO */
    sysPROGEND = EEPROM.read(1) | (EEPROM.read(2) << 8);

    for (int i=0; i<sysPROGEND; i++)
        mem[i] = EEPROM.read(i+3);
#endif
}

#ifdef PCBASIC_TARGET
#ifndef WIN32
char lingetch(void)
{
    /*#include <unistd.h>   //_getch*/
    /*#include <termios.h>  //_getch*/
    char buf = 0;
    struct termios old = {0};
    fflush(stdout);
    if(tcgetattr(0, &old) < 0)
    {
        perror("tcsetattr()");
    }

    old.c_lflag &=~ ICANON;
    old.c_lflag &=~ ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;

    if(tcsetattr(0, TCSANOW, &old) < 0)
    {
        perror("tcsetattr ICANON");
    }

    if(read(0, &buf, 1) < 0)
    {
        perror("read()");
    }

    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if(tcsetattr(0, TCSADRAIN, &old) <0 )
    {
        perror ("tcsetattr ~ICANON");
    }

    /* printf("%c\n",buf); */
    return buf;
}
#endif /* WIN32 */
#endif /* PCBASIC_TARGET */

#ifndef PCBASIC_TARGET
#ifdef KEYPAD_8x5_IN_USE
void state_machine_init()
{
    value_ROW = 1;
    key_check_state_machine = E_set_row_1st;
}

void state_machine_set_row_1st()
{
    set_KBD2(value_ROW);
    key_check_state_machine = E_read_col_1st;
}

void state_machine_read_column_1st()
{
    COLs_array[value_ROW - 1] = read_KBD1();
    value_ROW++;

    if (value_ROW > ROWS)
    {
        value_ROW = 1;
        key_check_state_machine = E_set_row_2nd;
    }
    else
    {
        key_check_state_machine = E_set_row_1st;
    }
}

void state_machine_set_row_2nd()
{
    set_KBD2(value_ROW);
    key_check_state_machine = E_read_col_2nd;
}

void state_machine_read_column_2nd()
{
    uint8_t i, col_value;

    col_value = read_KBD1();

    if(col_value != COLs_array[value_ROW - 1])          // Contact bounce -> start again
    {
        state_machine_init();
    }
    else
    {
        value_ROW++;
        if (value_ROW > ROWS)                      // Done (cycle) -> analyse keyboard array
        {
            for(i = 0; i < ROWS; i++)
            {
                if(key_pressed == false)
                {
                    if(COLs_array[i] != 0)          // A key was pressed
                    {
                        key_pressed = true;
                        break;
                    }
                }
            }

            state_machine_init();
        }
        else
        {
            key_check_state_machine = E_set_row_2nd;
        }
    }
}

char get_key()
{
    bool shift_pressed = false;
    bool other_keys_pressed = false;
    bool key_ready = false;
    int i, col, row;
    char key = 0;

    for(i = 0; i < ROWS; i++)
    {
        if (COLs_array[i] != 0)
        {
            if ((i == 5) && (COLs_array[i] == 1))
            {
                shift_pressed = true;
            }
            // Ugly workaround. TODO
            else if ((i == 5) && (COLs_array[i] == 3))
            {
                key = ':';
                key_ready = true;
                break;
            }
            else if ((i == 5) && (COLs_array[i] == 5))
            {
                key = ';';
                key_ready = true;
                break;
            }
            else if ((i == 5) && (COLs_array[i] == 9))
            {
                key = '?';
                key_ready = true;
                break;
            }
            else if ((i == 5) && (COLs_array[i] == 17))
            {
                key = '/';
                key_ready = true;
                break;
            }
            else
            {
                other_keys_pressed = true;
                row = i;
                col = COLs_array[i] - 1;
            }
        }
    }

    if(!key_ready)
    {
        if(other_keys_pressed)
        {
            if(shift_pressed)
            {
                key = key_map_shift[row][col];
            }
            else
            {
                 key = key_map[row][col];
            }
        }
    }

    return key;
}
#endif
#endif

#ifndef PCBASIC_TARGET
#ifdef SERIAL_TRACES_ON
void host_trace_str(const char *string, uint16_t str_size)
{
    usart_send_string(USART1, "\r\n", STRING_NULL);
    usart_send_string(USART1, string, str_size);
}

void host_trace_int(uint32_t int_num)
{
    usart_send_string(USART1, "\r\n", STRING_NULL);
    uart_write_number(USART1, int_num);
}
#endif
#endif // PCBASIC_TARGET

#ifdef SD_CARD_IN_USE
bool host_saveSdCard(char *fileName)
{
    File dataFile;
    bool ret = true;
    unsigned int fileNameLen = strlen(fileName);
    char buf[16];

    if(fileNameLen > 8)
    {
#ifdef SERIAL_TRACES_ON
        Serial1.print("File name is too long!");
#endif
        return false;
    }

    sprintf(buf, "%s.BAS", fileName);

#ifdef SERIAL_TRACES_ON
    Serial1.print("Save FileName:");
    Serial1.print(buf);
    Serial1.print(",FileNameLen:");
    Serial1.print(fileNameLen);
    Serial1.print(",progLen:");
    Serial1.println(sysPROGEND);
#endif

    dataFile = SD.open(buf, FILE_WRITE);
    if(dataFile)
    {
        dataFile.write(mem, sysPROGEND);
        dataFile.close();
#ifdef SERIAL_TRACES_ON
        Serial1.print("File saved");
#endif
    }
    else
    {
#ifdef SERIAL_TRACES_ON
        Serial1.print("SD file error");
#endif
        ret = false;
    }

    return ret;
}

bool host_loadSdCard(char *fileName)
{
    File dataFile;
    bool ret = true;
    int fileSize = 0;
    char buf[16];

    sprintf(buf, "%s.BAS", fileName);

#ifdef SERIAL_TRACES_ON
    Serial1.print("Load FileName:");
    Serial1.println(buf);
#endif

    dataFile = SD.open(buf, FILE_READ);
    if(dataFile)
    {
        fileSize = dataFile.size();

#ifdef SERIAL_TRACES_ON
        Serial1.print("File size:");
        Serial1.println(fileSize);
#endif

        sysPROGEND = fileSize;
        dataFile.read(mem, fileSize);
        dataFile.close();
    }
    else
    {
#ifdef SERIAL_TRACES_ON
        Serial1.print("SD file error");
#endif
        ret = false;
    }

    return ret;
}
#endif

