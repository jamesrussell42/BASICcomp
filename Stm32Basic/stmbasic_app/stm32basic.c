#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "../hal_src/hal.h"
#include "../lcd_src/i2c_lcd.h"
#include "../host_src/host.h"
#include "../basic_src/basic.h"

const char welcomeStr[] = "Stm32Basic v.0.61";

/* Global variables */
uint8_t mem[MEMORY_SIZE];
uint8_t tokenBuf[TOKEN_BUF_SIZE];

int main(void)
{
    uint8_t in_loop = 1;

    reset_basic();
    host_init(BUZZER_PIN);
    host_cls();
    
#ifdef SERIAL_TRACES_ON
    host_trace_str(welcomeStr, sizeof(welcomeStr));
#endif

    host_outputProgMemString(welcomeStr);
    
    /* Show memory size */
    host_outputFreeMem(sysVARSTART - sysPROGEND);
    host_showBuffer();

    while(in_loop)
    {
        int ret = ERROR_NONE;

        /* Get a line from the user */
        char *input = host_readLine();

        /* Special editor commands */
        if (input[0] == '?' && input[1] == 0)
        {
            host_outputFreeMem(sysVARSTART - sysPROGEND);
            host_showBuffer();
            return 0;
        }

        /* Otherwise tokenize */
        ret = tokenize((unsigned char*)input, tokenBuf, TOKEN_BUF_SIZE);

        /* Execute the token buffer */
        if (ret == ERROR_NONE)
        {
            host_new_line();
            ret = process_input(tokenBuf);
        }

        if (ret != ERROR_NONE)
        {
            host_new_line();
            if (lineNumber !=0)
            {
                host_output_int(lineNumber);
                host_output_char('-');
            }

            host_outputProgMemString((char *)pgm_read_word(&(errorTable[ret])));
        }
    }

    return 0;
}
