#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../host_src/host.h"
#include "../basic_src/basic.h"

/* Global variables */
uint8_t mem[MEMORY_SIZE];
uint8_t tokenBuf[TOKEN_BUF_SIZE];

#ifdef WIN32
const char welcomeStr[] = "PCBASIC v0.62 WIN";
#else
const char welcomeStr[] = "PCBASIC v0.62 LIN";
#endif

int main(void)
{
    uint8_t in_loop = 1;

    reset_basic();
    host_init(BUZZER_PIN);
    host_cls();

    host_outputProgMemString(welcomeStr);

    /* Show memory size */
    host_outputFreeMem(sysVARSTART - sysPROGEND);
    host_showBuffer();

    while(in_loop)
    {
        int ret = ERROR_NONE;

        /* Get a line from the user */
        char *input = host_readLine();

        if(strcmp(input, "qnow") == 0)
        {
            printf("\r\nBye!\r\n");

#ifndef WIN32
            fflush(stdout);
#endif
            return 0;
        }

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
