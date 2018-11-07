#include "config.h"
#include "basic.h"
#include "host.h"
#include <EEPROM.h>
#ifdef ANSI_VT220_TERMINAL_OUTPUT
#include <BasicTerm.h>
#endif

#ifdef I2C_LCD1602_LCD_16x2_DISPLAY
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(LCD_SERIAL_ADDRESS, SCREEN_WIDTH, SCREEN_HEIGHT);
#endif

// BASIC
unsigned char mem[MEMORY_SIZE];
#define TOKEN_BUF_SIZE    64
unsigned char tokenBuf[TOKEN_BUF_SIZE];

const char welcomeStr[] PROGMEM = "Arduino BASIC";
const char verStr[] PROGMEM = "Ver.0.52";
char autorun = 0;

#ifdef ANSI_VT220_TERMINAL_OUTPUT
BasicTerm term(&Serial);
#endif

void setup() 
{
#ifdef ANSI_VT220_TERMINAL_OUTPUT
    Serial.begin(9600);
    while (!Serial);
#endif

#ifdef I2C_LCD1602_LCD_16x2_DISPLAY
    lcd.init();
#endif

    reset();
    host_init(BUZZER_PIN);
    host_cls();
    host_outputProgMemString(welcomeStr);
       
    // Show memory size
    host_outputFreeMem(sysVARSTART - sysPROGEND);
    host_showBuffer();
    delay(1500);

    if (EEPROM.read(0) == MAGIC_AUTORUN_NUMBER)
        autorun = 1;
#ifdef BUZZER_IN_USE
    else
        host_startupTone();
#endif
}


void loop() 
{
    int ret = ERROR_NONE;

    if (!autorun)
    {
        // Get a line from the user
        char *input = host_readLine();
        
        // Special editor commands
        if (input[0] == '?' && input[1] == 0) 
        {
            host_outputFreeMem(sysVARSTART - sysPROGEND);
            host_showBuffer();
            return;
        }
        
        // Otherwise tokenize
        ret = tokenize((unsigned char*)input, tokenBuf, TOKEN_BUF_SIZE);
    }
    else 
    {
        host_loadProgram();
        tokenBuf[0] = TOKEN_RUN;
        tokenBuf[1] = 0;
        autorun = 0;
    }
    
    // Execute the token buffer
    if (ret == ERROR_NONE) 
    {
        host_newLine();
        ret = processInput(tokenBuf);
    }
    
    if (ret != ERROR_NONE) 
    {
        host_newLine();
        if (lineNumber !=0) 
        {
            host_outputInt(lineNumber);
            host_outputChar('-');
        }

        host_outputProgMemString((char *)pgm_read_word(&(errorTable[ret])));
    }
}
