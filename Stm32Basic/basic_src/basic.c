/* ---------------------------------------------------------------------------
 * Basic Interpreter
 * Robin Edwards 2014
 * ---------------------------------------------------------------------------
 * This BASIC is modelled on Sinclair BASIC for the ZX81 and ZX Spectrum. It
 * should be capable of running most of the examples in the manual for both
 * machines, with the exception of anything machine specific (i.e. graphics,
 * sound & system variables).
 *
 * Notes
 *  - All numbers (except line numbers) are floats internally
 *  - Multiple commands are allowed per line, seperated by :
 *  - LET is optional e.g. LET a = 6: b = 7
 *  - MOD provides the modulo operator which was missing from Sinclair BASIC.
 *     Both numbers are first rounded to ints e.g. 5 mod 2 = 1
 *  - CONT can be used to continue from a STOP. It does not continue from any
 *     other error condition.
 *  - Arrays can be any dimension. There is no single char limit to names.
 *  - Like Sinclair BASIC, DIM a(10) and LET a = 5 refer to different 'a's.
 *     One is a simple variable, the other is an array. There is no ambiguity
 *     since the one being referred to is always clear from the context.
 *  - String arrays differ from Sinclair BASIC. DIM a$(5,5) makes an array
 *     of 25 strings, which can be any length. e.g. LET a$(1,1)="long string"
 *  - functions like LEN, require brackets e.g. LEN(a$)
 *  - String manipulation functions are LEFT$, MID$, RIGHT$
 *  - RND is a nonary operator not a function i.e. RND not RND()
 *  - PRINT AT x,y ... is replaced by POSITION x,y : PRINT ...
 *  - LIST takes an optional start and end e.g. LIST 1,100 or LIST 50
 *  - INKEY$ reads the last key pressed from the keyboard, or an empty string
 *     if no key pressed. The (single key) buffer is emptied after the call.
 *     e.g. a$ = INKEY$
 *  - LOAD/SAVE load and save the current program to the EEPROM (1k limit).
 *     SAVE+ will set the auto-run flag, which loads the program automatically
 *     on boot. With a filename e.g. SAVE "test" saves to an external EEPROM.
 *  - DIR/DELETE "filename" - list and remove files from external EEPROM.
 *  - PINMODE <pin>, <mode> - sets the pin mode (0=input, 1=output, 2=pullup)
 *  - PIN <pin>, <state> - sets the pin high (non zero) or low (zero)
 *  - PINREAD(pin) returns pin value, ANALOGRD(pin) for analog pins
 * ---------------------------------------------------------------------------
 */

/* TODO
   ABS, SIN, COS, EXP etc
   DATA, READ, RESTORE */

/* Modified by VS for Stm32Basic project (2018) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include "basic.h"
#include "../host_src/host.h"

int32_t sysPROGEND;
int32_t sysSTACKSTART, sysSTACKEND;
int32_t sysVARSTART, sysVAREND;
int32_t sysGOSUBSTART, sysGOSUBEND;

const char string_0[] = "OK";
const char string_1[] = "Bad number";
const char string_2[] = "Line too long";
const char string_3[] = "Unexpected input";
const char string_4[] = "Unterminated string";
const char string_5[] = "Missing bracket";
const char string_6[] = "Error in expr";
const char string_7[] = "Numeric expr expected";
const char string_8[] = "String expr expected";
const char string_9[] = "Line number too big";
const char string_10[] = "Out of memory";
const char string_11[] = "Div by zero";
const char string_12[] = "Variable not found";
const char string_13[] = "Bad command";
const char string_14[] = "Bad line number";
const char string_15[] = "Break pressed";
const char string_16[] = "NEXT without FOR";
const char string_17[] = "STOP statement";
const char string_18[] = "Missing THEN in IF";
const char string_19[] = "RETURN without GOSUB";
const char string_20[] = "Wrong array dims";
const char string_21[] = "Bad array index";
const char string_22[] = "Bad string index";
const char string_23[] = "Error in VAL input";
const char string_24[] = "Bad parameter";

const char* errorTable[] =
{
    string_0, string_1, string_2, string_3,
    string_4, string_5, string_6, string_7,
    string_8, string_9, string_10, string_11,
    string_12, string_13, string_14, string_15,
    string_16, string_17, string_18, string_19,
    string_20, string_21, string_22, string_23,
    string_24
};

/* Token flags
   bits 1+2 number of arguments */
#define TKN_ARGS_NUM_MASK                           0x03

/* Bit 3 return type (set if string) */
#define TKN_RET_TYPE_STR                            0x04

/* Bits 4-6 argument type (set if string) */
#define TKN_ARG1_TYPE_STR                           0x08
#define TKN_ARG2_TYPE_STR                           0x10
#define TKN_ARG3_TYPE_STR                           0x20
#define TKN_ARG_MASK                                0x38
#define TKN_ARG_SHIFT                               3

/* Bits 7,8 formatting */
#define TKN_FMT_POST                                0x40
#define TKN_FMT_PRE                                 0x80

const TokenTableEntry tokenTable[] =
{
    {0, 0}, {0, 0}, {0, 0}, {0, 0},
    {0, 0}, {0, 0}, {0, 0}, {0, 0},
    {"(", 0}, {")",0}, {"+",0}, {"-",0},
    {"*",0}, {"/",0}, {"=",0}, {">",0},
    {"<",0}, {"<>",0}, {">=",0}, {"<=",0},
    {":",TKN_FMT_POST}, {";",0}, {",",0}, {"AND",TKN_FMT_PRE|TKN_FMT_POST},
    {"OR",TKN_FMT_PRE|TKN_FMT_POST}, {"NOT",TKN_FMT_POST}, {"PRINT",TKN_FMT_POST},
    {"LET",TKN_FMT_POST}, {"LIST",TKN_FMT_POST}, {"RUN",TKN_FMT_POST}, {"GOTO",TKN_FMT_POST},
    {"REM",TKN_FMT_POST}, {"STOP",TKN_FMT_POST}, {"INPUT",TKN_FMT_POST},  {"CONT",TKN_FMT_POST},
    {"IF",TKN_FMT_POST}, {"THEN",TKN_FMT_PRE|TKN_FMT_POST}, {"LEN",1|TKN_ARG1_TYPE_STR},
    {"VAL",1|TKN_ARG1_TYPE_STR}, {"RND",0}, {"INT",1}, {"STR$", 1|TKN_RET_TYPE_STR},
    {"FOR",TKN_FMT_POST}, {"TO",TKN_FMT_PRE|TKN_FMT_POST}, {"STEP",TKN_FMT_PRE|TKN_FMT_POST},
    {"NEXT", TKN_FMT_POST}, {"MOD",TKN_FMT_PRE|TKN_FMT_POST}, {"NEW",TKN_FMT_POST},{"GOSUB",TKN_FMT_POST},
    {"RETURN",TKN_FMT_POST}, {"DIM", TKN_FMT_POST}, {"LEFT$",2|TKN_ARG1_TYPE_STR|TKN_RET_TYPE_STR},
    {"RIGHT$",2|TKN_ARG1_TYPE_STR|TKN_RET_TYPE_STR}, {"MID$",3|TKN_ARG1_TYPE_STR|TKN_RET_TYPE_STR},
    {"CLS",TKN_FMT_POST}, {"PAUSE",TKN_FMT_POST}, {"POSITION", TKN_FMT_POST},  {"PIN",TKN_FMT_POST},
    {"PINMODE", TKN_FMT_POST}, {"INKEY$", 0}, {"SAVE", TKN_FMT_POST}, {"LOAD", TKN_FMT_POST},
    {"PINREAD",1}, {"ANALOGRD",1}, {"DIR", TKN_FMT_POST}, {"DELETE", TKN_FMT_POST}
};


/* **************************************************************************
 * PROGRAM FUNCTIONS
 * **************************************************************************/
void print_tokens(uint8_t *p)
{
    int32_t modeREM = 0;

    while (*p != TOKEN_EOL)
    {
        if (*p == TOKEN_IDENT)
        {
            p++;
            while (*p < 0x80)
            {
                host_output_char(*p++);
            }

            host_output_char((*p++) - 0x80);
        }
        else if (*p == TOKEN_NUMBER)
        {
            p++;
            host_output_float(*(float*)p);
            p += sizeof(float);
        }
        else if (*p == TOKEN_INTEGER)
        {
            p++;
            host_output_int(*(long*)p);
            p += sizeof(long);
        }
        else if (*p == TOKEN_STRING)
        {
            p++;
            if (modeREM)
            {
                host_output_string((char*)p);
                p += 1 + strlen((char*)p);
            }
            else
            {
                host_output_char('\"');
                while (*p)
                {
                    if (*p == '\"')
                    {
                        host_output_char('\"');
                    }

                    host_output_char(*p++);
                }

                host_output_char('\"');
                p++;
            }
        }
        else
        {
            uint8_t fmt = *(&tokenTable[*p].format);

            if (fmt & TKN_FMT_PRE)
            {
                host_output_char(' ');
            }

            host_output_string((char *)*(&tokenTable[*p].token));

            if (fmt & TKN_FMT_POST)
            {
                host_output_char(' ');
            }

            if (*p == TOKEN_REM)
            {
                modeREM = 1;
            }

            p++;
        }
    }
}

void list_prog(uint16_t first, uint16_t last)
{
    uint8_t *p = &mem[0];

    while (p < &mem[sysPROGEND])
    {
        uint16_t lineNum = *(uint16_t*)(p + 2);
        if ((!first || lineNum >= first) && (!last || lineNum <= last))
        {
            host_output_int(lineNum);
            host_output_char(' ');
            print_tokens(p + 4);
            host_new_line();
        }

        p += *(uint16_t *)p;
    }
}

uint8_t *find_prog_line(uint16_t targetLineNumber)
{
    uint8_t *p = &mem[0];

    while (p < &mem[sysPROGEND])
    {
        uint16_t lineNum = *(uint16_t*)(p + 2);
        if (lineNum >= targetLineNumber)
        {
            break;
        }

        p += *(uint16_t *)p;
    }

    return p;
}

void delete_prog_line(uint8_t *p)
{
    uint16_t lineLen = *(uint16_t*)p;
    sysPROGEND -= lineLen;
    memmove(p, p + lineLen, &mem[sysPROGEND] - p);
}

int32_t do_prog_line(uint16_t lineNum, uint8_t* tokenPtr, int32_t tokensLength)
{
    /* Find line of the at or immediately after the number */
    uint8_t *p = find_prog_line(lineNum);
    uint16_t foundLine = 0;
    int32_t bytesNeeded;

    if (p < &mem[sysPROGEND])
    {
        foundLine = *(uint16_t*)(p + 2);
    }

    /* If there's a line matching this one - delete it */
    if (foundLine == lineNum)
    {
        delete_prog_line(p);
    }

    /* Now check to see if this is an empty line, if so don't insert it */
    if (*tokenPtr == TOKEN_EOL)
    {
        return 1;
    }

    /* We now need to insert the new line at p */
    bytesNeeded = 4 + tokensLength;         /* length, linenum + tokens */
    if (sysPROGEND + bytesNeeded > sysVARSTART)
    {
        return 0;
    }

    /* Make room if this isn't the last line */
    if (foundLine)
    {
        memmove(p + bytesNeeded, p, &mem[sysPROGEND] - p);
    }

    *(uint16_t *)p = bytesNeeded;
    p += 2;
    *(uint16_t *)p = lineNum;
    p += 2;
    memcpy(p, tokenPtr, tokensLength);
    sysPROGEND += bytesNeeded;
    return 1;
}

/* **************************************************************************
 * CALCULATOR STACK FUNCTIONS
 * **************************************************************************/

/* Calculator stack starts at the start of memory after the program
   and grows towards the end
   contains either floats or null-terminated strings with the length on the end */

int32_t stack_push_num(float val)
{
    if (sysSTACKEND + (int32_t)sizeof(float) > sysVARSTART)
    {
        return 0;           /* Out of memory */
    }

    uint8_t *p = &mem[sysSTACKEND];
    *(float *)p = val;
    sysSTACKEND += sizeof(float);
    return 1;
}

float stack_pop_num(void)
{
    sysSTACKEND -= sizeof(float);
    uint8_t *p = &mem[sysSTACKEND];
    return *(float *)p;
}

int32_t stack_push_str(char *str)
{
    int32_t len = 1 + strlen(str);

    if (sysSTACKEND + len + 2 > sysVARSTART)
    {
        return 0;           /* Out of memory */
    }

    uint8_t *p = &mem[sysSTACKEND];
    strcpy((char*)p, str);
    p += len;
    *(uint16_t *)p = len;
    sysSTACKEND += len + 2;
    return 1;
}

char *stack_get_str(void)
{
    /* Returns string without popping it */
    uint8_t *p = &mem[sysSTACKEND];
    int32_t len = *(uint16_t *)(p - 2);
    return (char *)(p - len - 2);
}

char *stack_pop_str(void)
{
    uint8_t *p = &mem[sysSTACKEND];
    int32_t len = *(uint16_t *)(p - 2);
    sysSTACKEND -= (len + 2);
    return (char *)&mem[sysSTACKEND];
}

void stack_add_2_strs(void)
{
    /* Equivalent to popping 2 strings, concatenating them and pushing the result */
    uint8_t *p = &mem[sysSTACKEND];
    int32_t str2len = *(uint16_t *)(p - 2);
    sysSTACKEND -= (str2len + 2);
    char *str2 = (char*)&mem[sysSTACKEND];
    p = &mem[sysSTACKEND];
    int32_t str1len = *(uint16_t *)(p - 2);
    sysSTACKEND -= (str1len + 2);
    char *str1 = (char*)&mem[sysSTACKEND];
    p = &mem[sysSTACKEND];

    /* Shift the second string up (overwriting the null terminator of the first string) */
    memmove(str1 + str1len - 1, str2, str2len);

    /* Write the length and update stackend */
    int32_t newLen = str1len + str2len - 1;
    p += newLen;
    *(uint16_t *)p = newLen;
    sysSTACKEND += newLen + 2;
}

/* Mode 0 = LEFT$, 1 = RIGHT$ */
void stack_left_or_right_str(int32_t len, int32_t mode)
{
    /* Equivalent to popping the current string, doing the operation then pushing it again */
    uint8_t *p = &mem[sysSTACKEND];
    int32_t strlen = *(uint16_t *)(p - 2);
    len++;                      /* Include trailing null */
    if (len > strlen)
    {
        len = strlen;
    }

    if (len == strlen)
    {
        return;     /* Nothing to do */
    }

    sysSTACKEND -= (strlen + 2);
    p = &mem[sysSTACKEND];
    if (mode == 0)
    {
        /* Truncate the string on the stack */
        *(p + len - 1) = 0;
    }
    else
    {
        /* Copy the rightmost characters */
        memmove(p, p + strlen - len, len);
    }

    /* Write the length and update stackend */
    p += len;
    *(uint16_t *)p = len;
    sysSTACKEND += len + 2;
}

void stack_mid_str(int32_t start, int32_t len)
{
    /* Equivalent to popping the current string, doing the operation then pushing it again */
    uint8_t *p = &mem[sysSTACKEND];
    int32_t strlen = *(uint16_t *)(p - 2);

    len++;      /* Include trailing null */
    if (start > strlen)
    {
        start = strlen;
    }

    start--;    /* Basic strings start at 1 */
    if (start + len > strlen)
    {
        len = strlen - start;
    }

    if (len == strlen)
    {
        return;  /* Nothing to do */
    }

    sysSTACKEND -= (strlen + 2);
    p = &mem[sysSTACKEND];

    /* Copy the characters */
    memmove(p, p + start, len - 1);
    *(p+len-1) = 0;

    /* Write the length and update stackend */
    p += len;
    *(uint16_t *)p = len;
    sysSTACKEND += len + 2;
}

/* **************************************************************************
 * VARIABLE TABLE FUNCTIONS
 * **************************************************************************/

/* Variable table starts at the end of memory and grows towards the start
   Simple variable
   table +--------+-------+-----------------+-----------------+ . . .
    <--- | len    | type  | name            | value           |
   grows | 2bytes | 1byte | null terminated | float/string    |
         +--------+-------+-----------------+-----------------+ . . .

   Array
   +--------+-------+-----------------+----------+-------+ . . .+-------+-------------+. .
   | len    | type  | name            | num dims | dim1  |      | dimN  | elem(1,..1) |
   | 2bytes | 1byte | null terminated | 2bytes   | 2bytes|      | 2bytes| float       |
   +--------+-------+-----------------+----------+-------+ . . .+-------+-------------+. .
*/

/* Variable type byte */
#define VAR_TYPE_NUM                            0x1
#define VAR_TYPE_FORNEXT                        0x2
#define VAR_TYPE_NUM_ARRAY                      0x4
#define VAR_TYPE_STRING                         0x8
#define VAR_TYPE_STR_ARRAY                      0x10

uint8_t *find_variable(char *searchName, int32_t searchMask)
{
    uint8_t *p = &mem[sysVARSTART];
    while (p < &mem[sysVAREND])
    {
        int32_t type = *(p + 2);
        if (type & searchMask)
        {
            uint8_t *name = p + 3;
            if (strcasecmp((char*)name, searchName) == 0)
            {
                return p;
            }
        }

        p += *(uint16_t *)p;
    }

    return NULL;
}

void delete_variable_at(uint8_t *pos)
{
    int32_t len = *(uint16_t *)pos;

    if (pos == &mem[sysVARSTART])
    {
        sysVARSTART += len;
        return;
    }

    memmove(&mem[sysVARSTART] + len, &mem[sysVARSTART], pos - &mem[sysVARSTART]);
    sysVARSTART += len;
}

/* TODO - consistently return errors rather than 1 or 0? */

int32_t store_num_variable(char *name, float val)
{
    uint8_t *p;

    /* These can be modified in place */
    int32_t nameLen = strlen(name);
    p = find_variable(name, VAR_TYPE_NUM | VAR_TYPE_FORNEXT);

    if (p != NULL)
    {   /* Replace the old value
           (could either be VAR_TYPE_NUM or VAR_TYPE_FORNEXT) */

        p += 3;         /* len + type */
        p += nameLen + 1;
        *(float *)p = val;
    }
    else
    {   /* Allocate a new variable */
        int32_t bytesNeeded = 3;        /* len + flags */
        bytesNeeded += nameLen + 1;     /* name */
        bytesNeeded += sizeof(float);   /* val */

        if (sysVARSTART - bytesNeeded < sysSTACKEND)
        {
            return 0;       /* Out of memory */
        }

        sysVARSTART -= bytesNeeded;

        p = &mem[sysVARSTART];
        *(uint16_t *)p = bytesNeeded;
        p += 2;
        *p++ = VAR_TYPE_NUM;
        strcpy((char*)p, name);
        p += nameLen + 1;
        *(float *)p = val;
    }

    return 1;
}

int32_t store_for_next_variable(
    char *name,
    float start,
    float step,
    float end,
    uint16_t lineNum,
    uint16_t stmtNum)
{
    int32_t nameLen = strlen(name);
    int32_t bytesNeeded = 3;                /* len + flags */
    bytesNeeded += nameLen + 1;             /* name */
    bytesNeeded += 3 * sizeof(float);       /* vals */
    bytesNeeded += 2 * sizeof(uint16_t);

    /* Unlike simple numeric variables, these are reallocated if they already exist
       since the existing value might be a simple variable or a for/next variable */
    uint8_t *p = find_variable(name, VAR_TYPE_NUM | VAR_TYPE_FORNEXT);

    if (p != NULL)
    {
        /* Check there will actually be room for the new value */
        uint16_t oldVarLen = *(uint16_t*)p;
        if (sysVARSTART - (bytesNeeded - oldVarLen) < sysSTACKEND)
        {
            return 0;       /* Not enough memory */
        }

        delete_variable_at(p);
    }

    if (sysVARSTART - bytesNeeded < sysSTACKEND)
    {
        return 0;           /* Out of memory */
    }

    sysVARSTART -= bytesNeeded;

    p = &mem[sysVARSTART];
    *(uint16_t *)p = bytesNeeded;
    p += 2;
    *p++ = VAR_TYPE_FORNEXT;
    strcpy((char*)p, name);
    p += nameLen + 1;
    *(float *)p = start;
    p += sizeof(float);
    *(float *)p = step;
    p += sizeof(float);
    *(float *)p = end;
    p += sizeof(float);
    *(uint16_t *)p = lineNum;
    p += sizeof(uint16_t);
    *(uint16_t *)p = stmtNum;

    return 1;
}

int32_t store_str_variable(char *name, char *val)
{
    int32_t nameLen = strlen(name);
    int32_t valLen = strlen(val);
    int32_t bytesNeeded = 3;            /* len + type */
    bytesNeeded += nameLen + 1;         /* name */
    bytesNeeded += valLen + 1;          /* val */

    /* Strings and arrays are re-allocated if they already exist */
    uint8_t *p = find_variable(name, VAR_TYPE_STRING);
    if (p != NULL)
    {
        /* Check there will actually be room for the new value */
        uint16_t oldVarLen = *(uint16_t*)p;
        if (sysVARSTART - (bytesNeeded - oldVarLen) < sysSTACKEND)
        {
            return 0;           /* Not enough memory */
        }

        delete_variable_at(p);
    }

    if (sysVARSTART - bytesNeeded < sysSTACKEND)
    {
        return 0;               /* Out of memory */
    }

    sysVARSTART -= bytesNeeded;

    p = &mem[sysVARSTART];
    *(uint16_t *)p = bytesNeeded;
    p += 2;
    *p++ = VAR_TYPE_STRING;
    strcpy((char*)p, name);
    p += nameLen + 1;
    strcpy((char*)p, val);

    return 1;
}

int32_t create_array(char *name, int32_t isString)
{
    /* Dimensions and number of dimensions on the calculator stack */
    int32_t nameLen = strlen(name);
    int32_t bytesNeeded = 3;            /* len + flags */
    bytesNeeded += nameLen + 1;         /* name */
    bytesNeeded += 2;                   /* num dims */
    int32_t numElements = 1;
    int32_t i = 0;
    int32_t numDims = (int32_t)stack_pop_num();
    int32_t dim;

    /* Keep the current stack position, since we'll need to pop these values again */
    int32_t oldSTACKEND = sysSTACKEND;

    for (i = 0; i < numDims; i++)
    {
        dim = (int32_t)stack_pop_num();
        numElements *= dim;
    }

    bytesNeeded += 2 * numDims + (isString ? 1 : sizeof(float)) * numElements;

    /* Strings and arrays are re-allocated if they already exist */
    uint8_t *p = find_variable(name, (isString ? VAR_TYPE_STR_ARRAY : VAR_TYPE_NUM_ARRAY));

    if (p != NULL)
    {
        /* Check there will actually be room for the new value */
        uint16_t oldVarLen = *(uint16_t*)p;

        if (sysVARSTART - (bytesNeeded - oldVarLen) < sysSTACKEND)
        {
            return 0;           /* Not enough memory */
        }

        delete_variable_at(p);
    }

    if (sysVARSTART - bytesNeeded < sysSTACKEND)
    {
        return 0;               /* Out of memory */
    }

    sysVARSTART -= bytesNeeded;

    p = &mem[sysVARSTART];
    *(uint16_t *)p = bytesNeeded;
    p += 2;
    *p++ = (isString ? VAR_TYPE_STR_ARRAY : VAR_TYPE_NUM_ARRAY);
    strcpy((char*)p, name);
    p += nameLen + 1;
    *(uint16_t *)p = numDims;
    p += 2;
    sysSTACKEND = oldSTACKEND;

    for (i = 0; i < numDims; i++)
    {
        dim = (int32_t)stack_pop_num();
        *(uint16_t *)p = dim;
        p += 2;
    }

    memset(p, 0, numElements * (isString ? 1 : sizeof(float)));
    return 1;
}

int32_t get_array_elem_offset(uint8_t **p, int32_t *pOffset)
{
    /* Check for correct dimensionality */
    int32_t numArrayDims = *(uint16_t*)*p;
    *p += 2;
    int32_t numDimsGiven = (int32_t)stack_pop_num();

    if (numArrayDims != numDimsGiven)
    {
        return ERROR_WRONG_ARRAY_DIMENSIONS;
    }

    /* Now lookup the element */
    int32_t offset = 0;
    int32_t base = 1;
    for (int32_t i = 0; i < numArrayDims; i++)
    {
        int32_t index = (int32_t)stack_pop_num();
        int32_t arrayDim = *(uint16_t*)*p;
        *p += 2;

        if (index < 1 || index > arrayDim)
        {
            return ERROR_ARRAY_SUBSCRIPT_OUT_RANGE;
        }

        offset += base * (index - 1);
        base *= arrayDim;
    }

    *pOffset = offset;
    return 0;
}

int32_t set_num_array_elem(char *name, float val)
{
    /* Each index and number of dimensions on the calculator stack */
    uint8_t *p = find_variable(name, VAR_TYPE_NUM_ARRAY);

    if (p == NULL)
    {
        return ERROR_VARIABLE_NOT_FOUND;
    }

    p += 3 + strlen(name) + 1;

    int32_t offset;
    int32_t ret = get_array_elem_offset(&p, &offset);
    if (ret)
    {
        return ret;
    }

    p += sizeof(float)*offset;
    *(float *)p = val;
    return ERROR_NONE;
}

int32_t set_str_array_elem(char *name)
{
    /* String is top of the stack
       each index and number of dimensions on the calculator stack */

    /* Keep the current stack position, since we can't overwrite the value string */
    int32_t oldSTACKEND = sysSTACKEND;

    /* How long is the new value? */
    char *newValPtr = stack_pop_str();
    int32_t newValLen = strlen(newValPtr);

    uint8_t *p = find_variable(name, VAR_TYPE_STR_ARRAY);
    uint8_t *p1 = p;    /* So we can correct the length when done */
    if (p == NULL)
    {
        return ERROR_VARIABLE_NOT_FOUND;
    }

    p += 3 + strlen(name) + 1;

    int32_t offset;
    int32_t ret = get_array_elem_offset(&p, &offset);
    if (ret)
    {
        return ret;
    }

    /* Find the correct element by skipping over null terminators */
    int32_t i = 0;
    while (i < offset)
    {
        if (*p == 0) i++;
        p++;
    }

    int32_t oldValLen = strlen((char*)p);
    int32_t bytesNeeded = newValLen - oldValLen;

    /* Check if we've got enough room for the new value */
    if (sysVARSTART - bytesNeeded < oldSTACKEND)
    {
        return 0;       /* Out of memory */
    }

    /* Correct the length of the variable */
    *(uint16_t*)p1 += bytesNeeded;
    memmove(&mem[sysVARSTART - bytesNeeded], &mem[sysVARSTART], p - &mem[sysVARSTART]);

    /* Copy in the new value */
    strcpy((char*)(p - bytesNeeded), newValPtr);
    sysVARSTART -= bytesNeeded;
    return ERROR_NONE;
}

float lookup_num_array_elem(char *name, int32_t *error)
{
    // each index and number of dimensions on the calculator stack
    uint8_t *p = find_variable(name, VAR_TYPE_NUM_ARRAY);
    if (p == NULL)
    {
        *error = ERROR_VARIABLE_NOT_FOUND;
        return 0.0f;
    }

    p += 3 + strlen(name) + 1;

    int32_t offset;
    int32_t ret = get_array_elem_offset(&p, &offset);
    if (ret)
    {
        *error = ret;
        return 0.0f;
    }

    p += sizeof(float)*offset;
    return *(float *)p;
}

char *lookup_str_array_elem(char *name, int32_t *error)
{
    /* Each index and number of dimensions on the calculator stack */
    uint8_t *p = find_variable(name, VAR_TYPE_STR_ARRAY);
    if (p == NULL)
    {
        *error = ERROR_VARIABLE_NOT_FOUND;
        return NULL;
    }

    p += 3 + strlen(name) + 1;

    int32_t offset;
    int32_t ret = get_array_elem_offset(&p, &offset);
    if (ret)
    {
        *error = ret;
        return NULL;
    }

    /* Find the correct element by skipping over null terminators */
    int32_t i = 0;
    while (i < offset)
    {
        if (*p == 0) i++;
        p++;
    }

    return (char *)p;
}

float lookup_num_variable(char *name)
{
    uint8_t *p = find_variable(name, VAR_TYPE_NUM|VAR_TYPE_FORNEXT);
    if (p == NULL)
    {
        return FLT_MAX;
    }

    p += 3 + strlen(name) + 1;
    return *(float *)p;
}

char *lookup_str_variable(char *name)
{
    uint8_t *p = find_variable(name, VAR_TYPE_STRING);
    if (p == NULL)
    {
        return NULL;
    }

    p += 3 + strlen(name) + 1;
    return (char *)p;
}

ForNextData lookup_for_next_variable(char *name)
{
    ForNextData ret;
    uint8_t *p = find_variable(name, VAR_TYPE_NUM | VAR_TYPE_FORNEXT);
    if (p == NULL)
    {
        ret.val = FLT_MAX;
    }
    else if (*(p+2) != VAR_TYPE_FORNEXT)
    {
        ret.step = FLT_MAX;
    }
    else
    {
        p += 3 + strlen(name) + 1;
        ret.val = *(float *)p;
        p += sizeof(float);
        ret.step = *(float *)p;
        p += sizeof(float);
        ret.end = *(float *)p;
        p += sizeof(float);
        ret.lineNumber = *(uint16_t *)p;
        p += sizeof(uint16_t);
        ret.stmtNumber = *(uint16_t *)p;
    }

    return ret;
}

/* **************************************************************************
 * GOSUB STACK
 * **************************************************************************/
/* gosub stack (if used) is after the variables */
int32_t gosub_stack_push(int32_t lineNum,int32_t stmtNumber)
{
    int32_t bytesNeeded = 2 * sizeof(uint16_t);
    if (sysVARSTART - bytesNeeded < sysSTACKEND)
    {
        return 0;           /* out of memory */
    }

    /* Shift the variable table */
    memmove(&mem[sysVARSTART] - bytesNeeded, &mem[sysVARSTART], sysVAREND - sysVARSTART);
    sysVARSTART -= bytesNeeded;
    sysVAREND -= bytesNeeded;

    /* Push the return address */
    sysGOSUBSTART = sysVAREND;
    uint16_t *p = (uint16_t*)&mem[sysGOSUBSTART];
    *p++ = (uint16_t)lineNum;
    *p = (uint16_t)stmtNumber;
    return 1;
}

int32_t gosub_stack_pop(int32_t *lineNum, int32_t *stmtNumber)
{
    if (sysGOSUBSTART == sysGOSUBEND)
    {
        return 0;
    }

    uint16_t *p = (uint16_t*)&mem[sysGOSUBSTART];
    *lineNum = (int32_t)*p++;
    *stmtNumber = (int32_t)*p;
    int32_t bytesFreed = 2 * sizeof(uint16_t);

    /* Shift the variable table */
    memmove(&mem[sysVARSTART] + bytesFreed, &mem[sysVARSTART], sysVAREND - sysVARSTART);
    sysVARSTART += bytesFreed;
    sysVAREND += bytesFreed;
    sysGOSUBSTART = sysVAREND;
    return 1;
}

/* **************************************************************************
 * LEXER
 * **************************************************************************/

static uint8_t *tokenIn, *tokenOut;
static int32_t tokenOutLeft;

/* next_token returns -1 for end of input, 0 for success, +ve number = error code */
int32_t next_token(void)
{
    int32_t i;

    /* Skip any whitespace */
    while (isspace(*tokenIn))
    {
        tokenIn++;
    }

    /* Check for end of line */
    if (*tokenIn == 0)
    {
        *tokenOut++ = TOKEN_EOL;
        tokenOutLeft--;
        return -1;
    }

    /* Number: [0-9.]+ */
    /* TODO - handle 1e4 etc */
    if (isdigit(*tokenIn) || *tokenIn == '.')      /* Number: [0-9.]+ */
    {
        int32_t gotDecimal = 0;
        char numStr[MAX_NUMBER_LEN+1];
        int32_t numLen = 0;

        do
        {
            if (numLen == MAX_NUMBER_LEN)
            {
                return ERROR_LEXER_BAD_NUM;
            }

            if (*tokenIn == '.')
            {
                if (gotDecimal)
                {
                    return ERROR_LEXER_BAD_NUM;
                }
                else
                {
                    gotDecimal = 1;
                }
            }

            numStr[numLen++] = *tokenIn++;
        }
        while (isdigit(*tokenIn) || *tokenIn == '.');

        numStr[numLen] = 0;
        if (tokenOutLeft <= 5)
        {
            return ERROR_LEXER_TOO_LONG;
        }

        tokenOutLeft -= 5;
        if (!gotDecimal)
        {
            long val = strtol(numStr, 0, 10);
            if (val == LONG_MAX || val == LONG_MIN)
            {
                gotDecimal = 1; /* TODO True; */
            }
            else
            {
                *tokenOut++ = TOKEN_INTEGER;
                *(long*)tokenOut = (long)val;
                tokenOut += sizeof(long);
            }
        }

        if (gotDecimal)
        {
            *tokenOut++ = TOKEN_NUMBER;
            *(float*)tokenOut = (float)strtod(numStr, 0);
            tokenOut += sizeof(float);
        }

        return 0;
    }

    /* Identifier: [a-zA-Z][a-zA-Z0-9]*[$] */
    if (isalpha(*tokenIn))
    {
        char identStr[MAX_IDENT_LEN + 1];
        int32_t identLen = 0;
        identStr[identLen++] = *tokenIn++;  /* Copy first char */

        while (isalnum(*tokenIn) || *tokenIn == '$')
        {
            if (identLen < MAX_IDENT_LEN)
            {
                identStr[identLen++] = *tokenIn;
            }

            tokenIn++;
        }

        identStr[identLen] = 0;

        /* Check to see if this is a keyword */
        for (i = FIRST_IDENT_TOKEN; i <= LAST_IDENT_TOKEN; i++)
        {
            if (strcasecmp(identStr, (char *)pgm_read_word(&tokenTable[i].token)) == 0)
            {
                if (tokenOutLeft <= 1)
                {
                    return ERROR_LEXER_TOO_LONG;
                }

                tokenOutLeft--;
                *tokenOut++ = i;

                /* Special case for REM */
                if (i == TOKEN_REM)
                {
                    *tokenOut++ = TOKEN_STRING;

                    /* Skip whitespace */
                    while (isspace(*tokenIn))
                    {
                        tokenIn++;
                    }

                    /* Copy the comment */
                    while (*tokenIn)
                    {
                        *tokenOut++ = *tokenIn++;
                    }

                    *tokenOut++ = 0;
                }
                return 0;
            }
        }

        /* No matching keyword - this must be an identifier
           $ is only allowed at the end */
        char *dollarPos = strchr(identStr, '$');
        if (dollarPos && dollarPos != &identStr[0] + identLen - 1)
        {
            return ERROR_LEXER_UNEXPECTED_INPUT;
        }

        if (tokenOutLeft <= 1 + identLen)
        {
            return ERROR_LEXER_TOO_LONG;
        }

        tokenOutLeft -= 1 + identLen;
        *tokenOut++ = TOKEN_IDENT;
        strcpy((char*)tokenOut, identStr);
        tokenOut[identLen - 1] |= 0x80;
        tokenOut += identLen;
        return 0;
    }

    /* String */
    if (*tokenIn=='\"')
    {
        *tokenOut++ = TOKEN_STRING;
        tokenOutLeft--;
        if (tokenOutLeft <= 1)
        {
            return ERROR_LEXER_TOO_LONG;
        }

        tokenIn++;
        while (*tokenIn)
        {
            if (*tokenIn == '\"' && *(tokenIn+1) != '\"')
            {
                break;
            }
            else if (*tokenIn == '\"')
            {
                tokenIn++;
            }

            *tokenOut++ = *tokenIn++;
            tokenOutLeft--;
            if (tokenOutLeft <= 1)
            {
                return ERROR_LEXER_TOO_LONG;
            }
        }

        if (!*tokenIn)
        {
            return ERROR_LEXER_UNTERMINATED_STRING;
        }

        tokenIn++;
        *tokenOut++ = 0;
        tokenOutLeft--;
        return 0;
    }

    /* Handle non-alpha tokens e.g. = */
    for (i = LAST_NON_ALPHA_TOKEN; i >= FIRST_NON_ALPHA_TOKEN; i--)
    {
        /* Do this "backwards" so we match >= correctly, not as > then = */
        int32_t len = strlen((char *)pgm_read_word(&tokenTable[i].token));
        if (strncmp((char *)pgm_read_word(&tokenTable[i].token), (char*)tokenIn, len) == 0)
        {
            if (tokenOutLeft <= 1)
            {
                return ERROR_LEXER_TOO_LONG;
            }

            *tokenOut++ = i;
            tokenOutLeft--;
            tokenIn += len;
            return 0;
        }
    }

    return ERROR_LEXER_UNEXPECTED_INPUT;
}

int32_t tokenize(uint8_t *input, uint8_t *output, int32_t outputSize)
{
    tokenIn = input;
    tokenOut = output;
    tokenOutLeft = outputSize;
    int32_t ret;
    while (1)
    {
        ret = next_token();
        if (ret)
        {
            break;
        }
    }

    return (ret > 0) ? ret : 0;
}

/* **************************************************************************
 * PARSER / INTERPRETER
 * **************************************************************************/

static char executeMode;            /* 0 = syntax check only, 1 = execute */
uint16_t lineNumber, stmtNumber;

/* stmt number is 0 for the first statement, then increases after each command seperator (:)
   Note that IF a=1 THEN PRINT "x": print "y" is considered to be only 2 statements */
static uint16_t jumpLineNumber, jumpStmtNumber;
static uint16_t stopLineNumber, stopStmtNumber;
static char breakCurrentLine;

static uint8_t *tokenBuffer, *prevToken;
static int32_t curToken;
static char identVal[MAX_IDENT_LEN+1];
static char isStrIdent;
static float numVal;
static char *strVal;
/* static long numIntVal; TODO */

int32_t get_next_token(void)
{
    prevToken = tokenBuffer;
    curToken = *tokenBuffer++;
    if (curToken == TOKEN_IDENT)
    {
        int32_t i = 0;
        while (*tokenBuffer < 0x80)
        {
            identVal[i++] = *tokenBuffer++;
        }

        identVal[i] = (*tokenBuffer++) - 0x80;
        isStrIdent = (identVal[i++] == '$');
        identVal[i++] = '\0';
    }
    else if (curToken == TOKEN_NUMBER)
    {
        numVal = *(float*)tokenBuffer;
        tokenBuffer += sizeof(float);
    }
    else if (curToken == TOKEN_INTEGER)
    {
        /* These are really just for line numbers */
        numVal = (float)(*(long*)tokenBuffer);
        tokenBuffer += sizeof(long);
    }
    else if (curToken == TOKEN_STRING)
    {
        strVal = (char*)tokenBuffer;
        tokenBuffer += 1 + strlen(strVal);
    }

    return curToken;
}

/* Value (int) returned from parseXXXXX */
#define ERROR_MASK                              0x0FFF
#define TYPE_MASK                               0xF000
#define TYPE_NUMBER                             0x0000
#define TYPE_STRING                             0x1000

#define IS_TYPE_NUM(x) ((x & TYPE_MASK) == TYPE_NUMBER)
#define IS_TYPE_STR(x) ((x & TYPE_MASK) == TYPE_STRING)

/* Forward declarations */
int32_t parse_expression(void);
int32_t parse_primary(void);
int32_t expect_number(void);

/* Parse a number */
int32_t parse_number_expr(void)
{
    if (executeMode && !stack_push_num(numVal))
    {
        return ERROR_OUT_OF_MEMORY;
    }

    get_next_token();       /* Consume the number */
    return TYPE_NUMBER;
}

/* Parse (x1,....xn) e.g. DIM a(10,20) */
int32_t parse_subscript_expr(void)
{
    /* Stacks x1, .. xn followed by n */
    int32_t numDims = 0;
    if (curToken != TOKEN_LBRACKET)
    {
        return ERROR_EXPR_MISSING_BRACKET;
    }

    get_next_token();
    while(1)
    {
        numDims++;
        int32_t val = expect_number();
        if (val)
        {
            return val;     /* Error */
        }

        if (curToken == TOKEN_RBRACKET)
        {
            break;
        }
        else if (curToken == TOKEN_COMMA)
        {
            get_next_token();
        }
        else
        {
            return ERROR_EXPR_MISSING_BRACKET;
        }
    }
    get_next_token();       /* eat ) */
    if (executeMode && !stack_push_num(numDims))
    {
        return ERROR_OUT_OF_MEMORY;
    }

    return 0;
}

/* Parse a function call e.g. LEN(a$) */
int32_t parse_fn_call_expr(void)
{
    int32_t op = curToken;
    int32_t fnSpec = pgm_read_byte_near(&tokenTable[curToken].format);
    get_next_token();

    /* Get the required arguments and types from the token table */
    if (curToken != TOKEN_LBRACKET)
    {
        return ERROR_EXPR_MISSING_BRACKET;
    }

    get_next_token();

    int32_t reqdArgs = fnSpec & TKN_ARGS_NUM_MASK;
    int32_t argTypes = (fnSpec & TKN_ARG_MASK) >> TKN_ARG_SHIFT;
    int32_t ret = (fnSpec & TKN_RET_TYPE_STR) ? TYPE_STRING : TYPE_NUMBER;
    for (int32_t i = 0; i < reqdArgs; i++)
    {
        int32_t val = parse_expression();
        if (val & ERROR_MASK)
        {
            return val;
        }

        /* Check we've got the right type */
        if (!(argTypes & 1) && !IS_TYPE_NUM(val))
        {
            return ERROR_EXPR_EXPECTED_NUM;
        }

        if ((argTypes & 1) && !IS_TYPE_STR(val))
        {
            return ERROR_EXPR_EXPECTED_STR;
        }

        argTypes >>= 1;

        /* If this isn't the last argument, eat the , */
        if (i + 1 < reqdArgs)
        {
            if (curToken != TOKEN_COMMA)
            {
                return ERROR_UNEXPECTED_TOKEN;
            }

            get_next_token();
        }
    }

    /* Now all the arguments will be on the stack (last first) */
    if (executeMode)
    {
        int32_t tmp;
        switch (op)
        {
            case TOKEN_INT:
                stack_push_num((float)floor(stack_pop_num()));
                break;
            case TOKEN_STR:
                {
                    char buf[16];
                    if (!stack_push_str(host_float_to_str(stack_pop_num(), buf)))
                    {
                        return ERROR_OUT_OF_MEMORY;
                    }
                }
                break;
            case TOKEN_LEN:
                {
                    tmp = strlen(stack_pop_str());
                    if (!stack_push_num(tmp))
                    {
                        return ERROR_OUT_OF_MEMORY;
                    }
                }
                break;
        case TOKEN_VAL:
            {
                /* Tokenise str onto the stack */
                int32_t oldStackEnd = sysSTACKEND;
                uint8_t *oldTokenBuffer = prevToken;
                int32_t val = tokenize((unsigned char*)stack_get_str(), &mem[sysSTACKEND], sysVARSTART - sysSTACKEND);

                if (val)
                {
                    if (val == ERROR_LEXER_TOO_LONG)
                    {
                        return ERROR_OUT_OF_MEMORY;
                    }
                    else
                    {
                        return ERROR_IN_VAL_INPUT;
                    }
                }

                /* Set tokenBuffer to point to the new set of tokens on the stack */
                tokenBuffer = &mem[sysSTACKEND];

                /* Move stack end to the end of the new tokens */
                sysSTACKEND = tokenOut - &mem[0];
                get_next_token();

                /* .. then parse_expression */
                val = parse_expression();
                if (val & ERROR_MASK)
                {
                    if (val == ERROR_OUT_OF_MEMORY)
                    {
                        return val;
                    }
                    else
                    {
                        return ERROR_IN_VAL_INPUT;
                    }
                }

                if (!IS_TYPE_NUM(val))
                {
                    return ERROR_EXPR_EXPECTED_NUM;
                }

                /* Read the result from the stack */
                float f = stack_pop_num();

                /* Pop the tokens from the stack */
                sysSTACKEND = oldStackEnd;

                /* and pop the original string */
                stack_pop_str();

                /* Finally, push the result and set the token buffer back */
                stack_push_num(f);
                tokenBuffer = oldTokenBuffer;
                get_next_token();
            }
            break;
        case TOKEN_LEFT:
            {
                tmp = (int32_t)stack_pop_num();
                if (tmp < 0)
                {
                    return ERROR_STR_SUBSCRIPT_OUT_RANGE;
                }

                stack_left_or_right_str(tmp, 0);
            }
            break;
        case TOKEN_RIGHT:
            {
                tmp = (int32_t)stack_pop_num();
                if (tmp < 0)
                {
                    return ERROR_STR_SUBSCRIPT_OUT_RANGE;
                }

                stack_left_or_right_str(tmp, 1);
            }
            break;
        case TOKEN_MID:
            {
                tmp = (int32_t)stack_pop_num();
                int32_t start = stack_pop_num();
                if (tmp < 0 || start < 1)
                {
                    return ERROR_STR_SUBSCRIPT_OUT_RANGE;
                }

                stack_mid_str(start, tmp);
            }
            break;
        case TOKEN_PINREAD:
            {
                tmp = (int32_t)stack_pop_num();
                if (!stack_push_num(host_digitalRead(tmp)))
                {
                    return ERROR_OUT_OF_MEMORY;
                }
            }
            break;
        case TOKEN_ANALOGRD:
            {
                tmp = (int32_t)stack_pop_num();
                if (!stack_push_num(host_analogRead(tmp)))
                {
                    return ERROR_OUT_OF_MEMORY;
                }
            }
            break;
        default:
            return ERROR_UNEXPECTED_TOKEN;
        }
    }

    if (curToken != TOKEN_RBRACKET)
    {
        return ERROR_EXPR_MISSING_BRACKET;
    }

    get_next_token();         /* Eat ) */
    return ret;
}

/* Parse an identifer e.g. a$ or a(5,3) */
int32_t parse_identifier_expr(void)
{
    char ident[MAX_IDENT_LEN + 1];
    if (executeMode)
    {
        strcpy(ident, identVal);
    }

    int32_t isStringIdentifier = isStrIdent;
    get_next_token();           /* Eat ident */

    if (curToken == TOKEN_LBRACKET)
    {
        /* Array access */
        int32_t val = parse_subscript_expr();
        if (val)
        {
            return val;
        }

        if (executeMode)
        {
            if (isStringIdentifier)
            {
                int32_t error = 0;
                char *str = lookup_str_array_elem(ident, &error);
                if (error)
                {
                    return error;
                }
                else if (!stack_push_str(str))
                {
                    return ERROR_OUT_OF_MEMORY;
                }
            }
            else
            {
                int32_t error = 0;
                float f = lookup_num_array_elem(ident, &error);
                if (error)
                {
                    return error;
                }
                else if (!stack_push_num(f))
                {
                    return ERROR_OUT_OF_MEMORY;
                }
            }
        }
    }
    else
    {
        /* Simple variable */
        if (executeMode)
        {
            if (isStringIdentifier)
            {
                char *str = lookup_str_variable(ident);
                if (!str)
                {
                    return ERROR_VARIABLE_NOT_FOUND;
                }
                else if (!stack_push_str(str))
                {
                    return ERROR_OUT_OF_MEMORY;
                }
            }
            else
            {
                float f = lookup_num_variable(ident);
                if (f == FLT_MAX)
                {
                    return ERROR_VARIABLE_NOT_FOUND;
                }
                else if (!stack_push_num(f))
                {
                    return ERROR_OUT_OF_MEMORY;
                }
            }
        }
    }

    return isStringIdentifier ? TYPE_STRING : TYPE_NUMBER;
}

/* Parse a string e.g. "hello" */
int32_t parse_string_expr(void)
{
    if (executeMode && !stack_push_str(strVal))
    {
        return ERROR_OUT_OF_MEMORY;
    }

    get_next_token();           /* Consume the string */
    return TYPE_STRING;
}

/* Parse a bracketed expressed e.g. (5+3) */
int32_t parse_paren_expr(void)
{
    get_next_token();           /* Eat ( */
    int32_t val = parse_expression();
    if (val & ERROR_MASK)
    {
        return val;
    }

    if (curToken != TOKEN_RBRACKET)
    {
        return ERROR_EXPR_MISSING_BRACKET;
    }

    get_next_token();           /* Eat ) */
    return val;
}

int32_t parse_RND(void)
{
    get_next_token();
    if (executeMode && !stack_push_num((float)rand() / (float)RAND_MAX))
    {
        return ERROR_OUT_OF_MEMORY;
    }

    return TYPE_NUMBER;
}

int32_t parse_INKEY(void)
{
    get_next_token();
    if (executeMode)
    {
        char str[2];
        str[0] = host_getKey();
        str[1] = 0;
        if (!stack_push_str(str))
        {
            return ERROR_OUT_OF_MEMORY;
        }
    }

    return TYPE_STRING;
}

int32_t parse_unary_num_exp(void)
{
    int32_t ret = ERROR_UNEXPECTED_TOKEN;
    int32_t op = curToken;
    get_next_token();
    int32_t val = parse_primary();

    if (val & ERROR_MASK)
    {
        return val;
    }

    if (!IS_TYPE_NUM(val))
    {
        return ERROR_EXPR_EXPECTED_NUM;
    }

    switch (op)
    {
        case TOKEN_MINUS:
            {
                if (executeMode)
                {
                    stack_push_num(stack_pop_num() * -1.0f);
                    ret = TYPE_NUMBER;
                }
            }
            break;

        case TOKEN_NOT:
            {
                if (executeMode)
                {
                    stack_push_num(stack_pop_num() ? 0.0f : 1.0f);
                    ret = TYPE_NUMBER;
                }
            }
            break;

        default:
            break;
    }

    return ret;
}

/*====== Primary ====== */
int32_t parse_primary(void)
{
    int32_t ret = ERROR_UNEXPECTED_TOKEN;

    switch (curToken)
    {
        case TOKEN_IDENT:
            ret = parse_identifier_expr();
            break;

        case TOKEN_NUMBER:
        case TOKEN_INTEGER:
            ret = parse_number_expr();
            break;

        case TOKEN_STRING:
            ret = parse_string_expr();
            break;

        case TOKEN_LBRACKET:
            ret = parse_paren_expr();
            break;

        /* "Psuedo-identifiers" */
        case TOKEN_RND:
            ret = parse_RND();
            break;

        case TOKEN_INKEY:
            ret = parse_INKEY();
            break;

        /* Unary ops */
        case TOKEN_MINUS:
        case TOKEN_NOT:
            ret = parse_unary_num_exp();
            break;

        /* Functions */
        case TOKEN_INT:
        case TOKEN_STR:
        case TOKEN_LEN:
        case TOKEN_VAL:
        case TOKEN_LEFT:
        case TOKEN_RIGHT:
        case TOKEN_MID:
        case TOKEN_PINREAD:
        case TOKEN_ANALOGRD:
            ret = parse_fn_call_expr();
            break;

        default:
            break;
    }

    return ret;
}

int32_t get_to_k_precedence(void)
{
    if (curToken == TOKEN_AND || curToken == TOKEN_OR)
    {
        return 5;
    }

    if (curToken == TOKEN_EQUALS || curToken == TOKEN_NOT_EQ)
    {
        return 10;
    }

    if (curToken == TOKEN_LT || curToken == TOKEN_GT || curToken == TOKEN_LT_EQ || curToken == TOKEN_GT_EQ)
    {
        return 20;
    }

    if (curToken == TOKEN_MINUS || curToken == TOKEN_PLUS)
    {
        return 30;
    }
    else if (curToken == TOKEN_MULT || curToken == TOKEN_DIV || curToken == TOKEN_MOD)
    {
        return 40;
    }
    else
    {
        return -1;
    }
}

/* Operator-Precedence Parsing */
int32_t parse_bin_op_RHS(int32_t ExprPrec, int32_t lhsVal)
{
    /* If this is a binop, find its precedence */
    while (1)
    {
        int32_t TokPrec = get_to_k_precedence();
        int32_t BinOp, rhsVal, NextPrec;

        /* If this is a binop that binds at least as tightly as the current binop,
           consume it, otherwise we are done */
        if (TokPrec < ExprPrec)
        {
            return lhsVal;
        }

        /* Okay, we know this is a binop */
        BinOp = curToken;
        get_next_token();           /* Eat binop */

        /* Parse the primary expression after the binary operator */
        rhsVal = parse_primary();
        if (rhsVal & ERROR_MASK)
        {
            return rhsVal;
        }

        /* If BinOp binds less tightly with RHS than the operator after RHS, let
           the pending operator take RHS as its LHS */
        NextPrec = get_to_k_precedence();
        if (TokPrec < NextPrec)
        {
            rhsVal = parse_bin_op_RHS(TokPrec + 1, rhsVal);
            if (rhsVal & ERROR_MASK)
            {
                return rhsVal;
            }
        }

        if (IS_TYPE_NUM(lhsVal) && IS_TYPE_NUM(rhsVal))
        {
            /* Number operations */
            float r = 0;
            float l = 0;

            if (executeMode)
            {
                r = stack_pop_num();
                l = stack_pop_num();
            }

            if (BinOp == TOKEN_PLUS)
            {
                if (executeMode)
                {
                    stack_push_num(l + r);
                }
            }
            else if (BinOp == TOKEN_MINUS)
            {
                if (executeMode)
                {
                    stack_push_num(l - r);
                }
            }
            else if (BinOp == TOKEN_MULT)
            {
                if (executeMode)
                {
                    stack_push_num(l * r);
                }
            }
            else if (BinOp == TOKEN_DIV)
            {
                if (executeMode)
                {
                    if (r)
                    {
                        stack_push_num(l / r);
                    }
                    else
                    {
                        return ERROR_EXPR_DIV_ZERO;
                    }
                }
            }
            else if (BinOp == TOKEN_MOD)
            {
                if (executeMode)
                {
                    if ((int32_t)r)
                    {
                        stack_push_num((float)((int32_t)l % (int32_t)r));
                    }
                    else
                    {
                        return ERROR_EXPR_DIV_ZERO;
                    }
                }
            }
            else if (BinOp == TOKEN_LT)
            {
                if (executeMode)
                {
                    stack_push_num(l < r ? 1.0f : 0.0f);
                }
            }
            else if (BinOp == TOKEN_GT)
            {
                if (executeMode)
                {
                    stack_push_num(l > r ? 1.0f : 0.0f);
                }
            }
            else if (BinOp == TOKEN_EQUALS)
            {
                if (executeMode)
                {
                    stack_push_num(l == r ? 1.0f : 0.0f);
                }
            }
            else if (BinOp == TOKEN_NOT_EQ)
            {
                if (executeMode)
                {
                    stack_push_num(l != r ? 1.0f : 0.0f);
                }
            }
            else if (BinOp == TOKEN_LT_EQ)
            {
                if (executeMode)
                {
                    stack_push_num(l <= r ? 1.0f : 0.0f);
                }
            }
            else if (BinOp == TOKEN_GT_EQ)
            {
                if (executeMode)
                {
                    stack_push_num(l >= r ? 1.0f : 0.0f);
                }
            }
            else if (BinOp == TOKEN_AND)
            {
                if (executeMode)
                {
                    stack_push_num(r != 0.0f ? l : 0.0f);
                }
            }
            else if (BinOp == TOKEN_OR)
            {
                if (executeMode)
                {
                    stack_push_num(r != 0.0f ? 1 : l);
                }
            }
            else
            {
                return ERROR_UNEXPECTED_TOKEN;
            }
        }
        else if (IS_TYPE_STR(lhsVal) && IS_TYPE_STR(rhsVal))
        {
            /* String operations */
            if (BinOp == TOKEN_PLUS)
            {
                if (executeMode)
                {
                    stack_add_2_strs();
                }
            }
            else if (BinOp >= TOKEN_EQUALS && BinOp <=TOKEN_LT_EQ)
            {
                if (executeMode)
                {
                    char *r = stack_pop_str();
                    char *l = stack_pop_str();
                    int32_t ret = strcmp(l,r);

                    if (BinOp == TOKEN_EQUALS && ret == 0)
                    {
                        stack_push_num(1.0f);
                    }
                    else if (BinOp == TOKEN_NOT_EQ && ret != 0)
                    {
                        stack_push_num(1.0f);
                    }
                    else if (BinOp == TOKEN_GT && ret > 0)
                    {
                        stack_push_num(1.0f);
                    }
                    else if (BinOp == TOKEN_LT && ret < 0)
                    {
                        stack_push_num(1.0f);
                    }
                    else if (BinOp == TOKEN_GT_EQ && ret >= 0)
                    {
                        stack_push_num(1.0f);
                    }
                    else if (BinOp == TOKEN_LT_EQ && ret <= 0)
                    {
                        stack_push_num(1.0f);
                    }
                    else
                    {
                        stack_push_num(0.0f);
                    }
                }

                lhsVal = TYPE_NUMBER;
            }
            else
            {
                return ERROR_UNEXPECTED_TOKEN;
            }
        }
        else
        {
            return ERROR_UNEXPECTED_TOKEN;
        }
    }
}

int32_t parse_expression(void)
{
    int32_t val = parse_primary();

    if (val & ERROR_MASK)
    {
        return val;
    }

    return parse_bin_op_RHS(0, val);
}

int32_t expect_number(void)
{
    int32_t val = parse_expression();

    if (val & ERROR_MASK)
    {
        return val;
    }

    if (!IS_TYPE_NUM(val))
    {
        return ERROR_EXPR_EXPECTED_NUM;
    }

    return 0;
}

int32_t parse_RUN(void)
{
    uint16_t startLine;

    get_next_token();
    startLine = 1;

    if (curToken != TOKEN_EOL)
    {
        int32_t val = expect_number();
        if (val)
        {
            return val;             /* Error */
        }

        if (executeMode)
        {
            startLine = (uint16_t)stack_pop_num();
            if (startLine <= 0)
            {
                return ERROR_BAD_LINE_NUM;
            }
        }
    }

    if (executeMode)
    {
        /* Clear variables */
        sysVARSTART = sysVAREND = sysGOSUBSTART = sysGOSUBEND = MEMORY_SIZE;
        jumpLineNumber = startLine;
        stopLineNumber = stopStmtNumber = 0;
    }

    return 0;
}

int32_t parse_GOTO(void)
{
    get_next_token();
    int32_t val = expect_number();

    if (val)
    {
        return val;         /* Error */
    }

    if (executeMode)
    {
        uint16_t startLine = (uint16_t)stack_pop_num();

        if (startLine <= 0)
        {
            return ERROR_BAD_LINE_NUM;
        }

        jumpLineNumber = startLine;
    }

    return 0;
}

int32_t parse_PAUSE(void)
{
    get_next_token();
    int32_t val = expect_number();

    if (val)
    {
        return val;         /* Error */
    }

    if (executeMode)
    {
        long ms = (long)stack_pop_num();

        if (ms < 0)
        {
            return ERROR_BAD_PARAMETER;
        }

        host_sleep(ms);
    }

    return 0;
}

int32_t parse_LIST(void)
{
    get_next_token();
    uint16_t first = 0;
    uint16_t last = 0;

    if (curToken != TOKEN_EOL && curToken != TOKEN_CMD_SEP)
    {
        int32_t val = expect_number();

        if (val)
        {
            return val;             /* Error */
        }

        if (executeMode)
        {
            first = (uint16_t)stack_pop_num();
        }
    }

    if (curToken == TOKEN_COMMA)
    {
        get_next_token();
        int32_t val = expect_number();

        if (val)
        {
            return val;             /* Error */
        }

        if (executeMode)
        {
            last = (uint16_t)stack_pop_num();
        }
    }

    if (executeMode)
    {
        list_prog(first, last);
        host_showBuffer();
    }

    return 0;
}

int32_t parse_PRINT(void)
{
    get_next_token();

    /* zero + expressions seperated by semicolons */
    int32_t newLine = 1;

    while (curToken != TOKEN_EOL && curToken != TOKEN_CMD_SEP)
    {
        int32_t val = parse_expression();

        if (val & ERROR_MASK)
        {
            return val;
        }

        if (executeMode)
        {
            if (IS_TYPE_NUM(val))
            {
                host_output_float(stack_pop_num());
            }
            else
            {
                host_output_string(stack_pop_str());
            }

            newLine = 1;
        }

        if (curToken == TOKEN_SEMICOLON)
        {
            newLine = 0;
            get_next_token();
        }
    }

    if (executeMode)
    {
        if (newLine)
        {
            host_new_line();
        }

        host_showBuffer();
    }

    return 0;
}

/* Parse a stmt that takes two int parameters
   e.g. POSITION 3,2 */
int32_t parse_two_int_cmd(void)
{
    int32_t val;
    int32_t op = curToken;
    get_next_token();

    val = expect_number();
    if (val)
    {
        return val;         /* Error */
    }

    if (curToken != TOKEN_COMMA)
    {
        return ERROR_UNEXPECTED_TOKEN;
    }

    get_next_token();
    val = expect_number();

    if (val)
    {
        return val;         /* Error */
    }

    if (executeMode)
    {
        int32_t second = (int32_t)stack_pop_num();
        int32_t first = (int32_t)stack_pop_num();

        switch(op)
        {
            case TOKEN_POSITION:
                host_moveCursor(first, second);
                break;

            case TOKEN_PIN:
                host_digitalWrite(first, second);
                break;

            case TOKEN_PINMODE:
                host_pinMode(first, second);
                break;

            default:
                break;
        }
    }

    return 0;
}

/* This handles both LET a$="hello" and INPUT a$ type assignments */
int32_t parse_assignment(bool inputStmt)
{
    char ident[MAX_IDENT_LEN + 1];
    int32_t val, isStringIdentifier, isArray = 0;

    if (curToken != TOKEN_IDENT)
    {
        return ERROR_UNEXPECTED_TOKEN;
    }

    if (executeMode)
    {
        strcpy(ident, identVal);
    }

    isStringIdentifier = isStrIdent;
    isArray = 0;
    get_next_token();               /* Eat ident */

    if (curToken == TOKEN_LBRACKET)
    {
        /* Array element being set */
        val = parse_subscript_expr();
        if (val)
        {
            return val;
        }

        isArray = 1;
    }

    if (inputStmt)
    {
        /* From INPUT statement */
        if (executeMode)
        {
            char *inputStr = host_readLine();
            if (isStringIdentifier)
            {
                if (!stack_push_str(inputStr))
                {
                    return ERROR_OUT_OF_MEMORY;
                }
            }
            else
            {
                float f = (float)strtod(inputStr, 0);
                if (!stack_push_num(f))
                {
                    return ERROR_OUT_OF_MEMORY;
                }
            }

            host_new_line();
            host_showBuffer();
        }

        val = isStringIdentifier ? TYPE_STRING : TYPE_NUMBER;
    }
    else
    {
        /* From LET statement */
        if (curToken != TOKEN_EQUALS)
        {
            return ERROR_UNEXPECTED_TOKEN;
        }

        get_next_token();               /* Eat = */
        val = parse_expression();
        if (val & ERROR_MASK)
        {
            return val;
        }
    }

    /* Type checking and actual assignment */
    if (!isStringIdentifier)
    {
        /* Numeric variable */
        if (!IS_TYPE_NUM(val))
        {
            return ERROR_EXPR_EXPECTED_NUM;
        }

        if (executeMode)
        {
            if (isArray)
            {
                val = set_num_array_elem(ident, stack_pop_num());
                if (val)
                {
                    return val;
                }
            }
            else
            {
                if (!store_num_variable(ident, stack_pop_num()))
                {
                    return ERROR_OUT_OF_MEMORY;
                }
            }
        }
    }
    else
    {
        /* String variable */
        if (!IS_TYPE_STR(val))
        {
            return ERROR_EXPR_EXPECTED_STR;
        }

        if (executeMode)
        {
            if (isArray)
            {
                /* Annoyingly, we've got the string at the top of the stack
                   (from parse_expression) and the array index stuff (from
                   parse_subscript_expr) underneath */
                val = set_str_array_elem(ident);
                if (val)
                {
                    return val;
                }
            }
            else
            {
                if (!store_str_variable(ident, stack_get_str()))
                {
                    return ERROR_OUT_OF_MEMORY;
                }

                stack_pop_str();
            }
        }
    }

    return 0;
}

int32_t parse_IF(void)
{
    int32_t val;
    get_next_token();           /* Eat if */
    val = expect_number();

    if (val)
    {
        return val;             /* Error */
    }

    if (curToken != TOKEN_THEN)
    {
        return ERROR_MISSING_THEN;
    }

    get_next_token();
    if (executeMode && stack_pop_num() == 0.0f)
    {
        /* Condition not met */
        breakCurrentLine = 1;
        return 0;
    }
    else
    {
        return 0;
    }
}

int32_t parse_FOR(void)
{
    int32_t val;
    char ident[MAX_IDENT_LEN + 1];
    float start = 0;
    float end = 0;
    float step = 1.0f;

    get_next_token();           /* Eat for */

    if (curToken != TOKEN_IDENT || isStrIdent)
    {
        return ERROR_UNEXPECTED_TOKEN;
    }

    if (executeMode)
    {
        strcpy(ident, identVal);
    }

    get_next_token();           /* Eat ident */

    if (curToken != TOKEN_EQUALS)
    {
        return ERROR_UNEXPECTED_TOKEN;
    }

    get_next_token();           /* Eat = */

    /* Parse START */
    val = expect_number();
    if (val)
    {
        return val;             /* Error */
    }

    if (executeMode)
    {
        start = stack_pop_num();
    }

    /* Parse TO */
    if (curToken != TOKEN_TO)
    {
        return ERROR_UNEXPECTED_TOKEN;
    }

    get_next_token();       /* Eat TO */

    /* Parse END */
    val = expect_number();

    if (val)
    {
        return val;         /* Error */
    }

    if (executeMode)
    {
        end = stack_pop_num();
    }

    /* Parse optional STEP */
    if (curToken == TOKEN_STEP)
    {
        get_next_token();       /* Eat STEP */
        val = expect_number();
        if (val)
        {
            return val;         /* Error */
        }

        if (executeMode)
        {
            step = stack_pop_num();
        }
    }

    if (executeMode)
    {
        if (!store_for_next_variable(ident, start, step, end, lineNumber, stmtNumber))
        {
            return ERROR_OUT_OF_MEMORY;
        }
    }

    return 0;
}

int32_t parse_NEXT(void)
{
    get_next_token();           /* Eat next */
    if (curToken != TOKEN_IDENT || isStrIdent)
    {
        return ERROR_UNEXPECTED_TOKEN;
    }

    if (executeMode)
    {
        ForNextData data = lookup_for_next_variable(identVal);
        if (data.val == FLT_MAX)
        {
            return ERROR_VARIABLE_NOT_FOUND;
        }
        else if (data.step == FLT_MAX)
        {
            return ERROR_NEXT_WITHOUT_FOR;
        }

        /* Update and store the count variable */
        data.val += data.step;
        store_num_variable(identVal, data.val);

        /* Loop? */
        if ((data.step >= 0 && data.val <= data.end) || (data.step < 0 && data.val >= data.end))
        {
            jumpLineNumber = data.lineNumber;
            jumpStmtNumber = data.stmtNumber + 1;
        }
    }

    get_next_token();       /* Eat ident */
    return 0;
}

int32_t parse_GOSUB(void)
{
    int32_t val;
    get_next_token();       /* Eat gosub */
    val = expect_number();

    if (val)
    {
        return val;         /* Error */
    }

    if (executeMode)
    {
        uint16_t startLine = (uint16_t)stack_pop_num();
        if (startLine <= 0)
        {
            return ERROR_BAD_LINE_NUM;
        }

        jumpLineNumber = startLine;

        if (!gosub_stack_push(lineNumber,stmtNumber))
        {
            return ERROR_OUT_OF_MEMORY;
        }
    }

    return 0;
}

/* LOAD or LOAD "x"
   SAVE, SAVE+ or SAVE "x"
   DELETE "x" */
int32_t parse_load_save_cmd(void)
{
    int32_t op = curToken;
    uint8_t autoexec = 0;
    char gotFileName = 0;
    get_next_token();

    if (op == TOKEN_SAVE && curToken == TOKEN_PLUS)
    {
        get_next_token();
        autoexec = 1;
    }
    else if (curToken != TOKEN_EOL && curToken != TOKEN_CMD_SEP)
    {
        int32_t val = parse_expression();
        if (val & ERROR_MASK)
        {
            return val;
        }

        if (!IS_TYPE_STR(val))
        {
            return ERROR_EXPR_EXPECTED_STR;
        }

        gotFileName = 1;
    }

    if (executeMode)
    {
        if (gotFileName)
        {
#if 0 /* #if EXTERNAL_EEPROM */
            char fileName[MAX_IDENT_LEN + 1];

            if (strlen(stack_get_str()) > MAX_IDENT_LEN)
            {
                return ERROR_BAD_PARAMETER;
            }

            strcpy(fileName, stack_pop_str());

            if (op == TOKEN_SAVE)
            {
                if (!host_saveSdCard(fileName))
                {
                    return ERROR_OUT_OF_MEMORY;
                }

 /*               if (!host_saveExtEEPROM(fileName))
                     return ERROR_OUT_OF_MEMORY; */
            }
            else if (op == TOKEN_LOAD)
            {
                reset();
                if (!host_loadSdCard(fileName))
                {
                    return ERROR_BAD_PARAMETER;
                }

 /*               if (!host_loadExtEEPROM(fileName))
                      return ERROR_BAD_PARAMETER; */
            }
            else if (op == TOKEN_DELETE)
            {
 /*               if (!host_removeExtEEPROM(fileName))
                      return ERROR_BAD_PARAMETER; */
            }
#endif
        }
        else
        {
            if (op == TOKEN_SAVE)
            {
                host_save_program(autoexec);
            }
            else if (op == TOKEN_LOAD)
            {
                reset_basic();
                host_loadProgram();
            }
            else
            {
                return ERROR_UNEXPECTED_CMD;
            }
        }
    }

    return 0;
}

int32_t parse_simple_cmd(void)
{
    int32_t ret = 0;
    int32_t op = curToken;
    get_next_token();           /* Eat op */

    if (executeMode)
    {
        switch (op)
        {
            case TOKEN_NEW:
                {
                    reset_basic();
                    breakCurrentLine = 1;
                }
                break;

            case TOKEN_STOP:
                {
                    stopLineNumber = lineNumber;
                    stopStmtNumber = stmtNumber;
                    ret = ERROR_STOP_STATEMENT;
                }
                break;

            case TOKEN_CONT:
                if (stopLineNumber)
                {
                    jumpLineNumber = stopLineNumber;
                    jumpStmtNumber = stopStmtNumber + 1;
                }
                break;

            case TOKEN_RETURN:
                {
                    int32_t returnLineNumber, returnStmtNumber;
                    if (!gosub_stack_pop(&returnLineNumber, &returnStmtNumber))
                    {
                        ret = ERROR_RETURN_WITHOUT_GOSUB;
                    }
                    else
                    {
                        jumpLineNumber = returnLineNumber;
                        jumpStmtNumber = returnStmtNumber + 1;
                    }
                }
                break;

            case TOKEN_CLS:
                {
                    host_cls();
                    host_showBuffer();
                }
                break;

            case TOKEN_DIR:
#ifdef EXTERNAL_EEPROM
                host_directoryExtEEPROM();
#endif
                break;

            default:
                break;
        }
    }

    return ret;
}

int32_t parse_DIM(void)
{
    int32_t isStringIdentifier, val;
    char ident[MAX_IDENT_LEN + 1];
    get_next_token();                   /* Eat DIM */

    if (curToken != TOKEN_IDENT)
    {
        return ERROR_UNEXPECTED_TOKEN;
    }

    if (executeMode)
    {
        strcpy(ident, identVal);
    }

    isStringIdentifier = isStrIdent;
    get_next_token();                   /* Eat ident */

    val = parse_subscript_expr();
    if (val)
    {
        return val;
    }

    if (executeMode && !create_array(ident, isStringIdentifier ? 1 : 0))
    {
        return ERROR_OUT_OF_MEMORY;
    }

    return 0;
}

static int32_t targetStmtNumber;

int32_t parse_stmts(void)
{
    int32_t ret = 0;
    int32_t needCmdSep = 1;

    breakCurrentLine = 0;
    jumpLineNumber = 0;
    jumpStmtNumber = 0;

    while (ret == 0)
    {
        if (curToken == TOKEN_EOL)
        {
            break;
        }

        if (executeMode)
        {
            sysSTACKEND = sysSTACKSTART = sysPROGEND;       /* Clear calculator stack */
        }

        switch (curToken)
        {
            case TOKEN_PRINT:
                ret = parse_PRINT();
                break;

            case TOKEN_LET:
                get_next_token();
                ret = parse_assignment(false);
                break;

            case TOKEN_IDENT:
                ret = parse_assignment(false);
                break;

            case TOKEN_INPUT:
                get_next_token();
                ret = parse_assignment(true);
                break;

            case TOKEN_LIST:
                ret = parse_LIST();
                break;

            case TOKEN_RUN:
                ret = parse_RUN();
                break;

            case TOKEN_GOTO:
                ret = parse_GOTO();
                break;

            case TOKEN_REM:
                get_next_token();
                get_next_token();
                break;

            case TOKEN_IF:
                ret = parse_IF();
                needCmdSep = 0;
                break;

            case TOKEN_FOR:
                ret = parse_FOR();
                break;

            case TOKEN_NEXT:
                ret = parse_NEXT();
                break;

            case TOKEN_GOSUB:
                ret = parse_GOSUB();
                break;

            case TOKEN_DIM:
                ret = parse_DIM();
                break;

            case TOKEN_PAUSE:
                ret = parse_PAUSE();
                break;

            case TOKEN_LOAD:
            case TOKEN_SAVE:
            case TOKEN_DELETE:
                ret = parse_load_save_cmd();
                break;

            case TOKEN_POSITION:
            case TOKEN_PIN:
            case TOKEN_PINMODE:
                ret = parse_two_int_cmd();
                break;

            case TOKEN_NEW:
            case TOKEN_STOP:
            case TOKEN_CONT:
            case TOKEN_RETURN:
            case TOKEN_CLS:
            case TOKEN_DIR:
                ret = parse_simple_cmd();
                break;

            default:
                ret = ERROR_UNEXPECTED_CMD;
        }

        /* If error, or the execution line has been changed, exit here */
        if (ret || breakCurrentLine || jumpLineNumber || jumpStmtNumber)
        {
            break;
        }

        /* It should either be the end of the line now, and (generally) a command seperator
           before the next command */
        if (curToken != TOKEN_EOL)
        {
            if (needCmdSep)
            {
                if (curToken != TOKEN_CMD_SEP)
                {
                    ret = ERROR_UNEXPECTED_CMD;
                }
                else
                {
                    get_next_token();

                    /* Don't allow a trailing : */
                    if (curToken == TOKEN_EOL)
                    {
                        ret = ERROR_UNEXPECTED_CMD;
                    }
                }
            }
        }

        /* Increase the statement number */
        stmtNumber++;

        /* If we're just scanning to find a target statement, check */
        if (stmtNumber == targetStmtNumber)
        {
            break;
        }
    }

    return ret;
}

int32_t process_input(uint8_t *tokenBuf)
{
    /* First token can be TOKEN_INTEGER for line number - stored as a float in numVal
       store as WORD line number (max 65535) */
    tokenBuffer = tokenBuf;
    get_next_token();

    /* Check for a line number at the start */
    uint16_t gotLineNumber = 0;
    uint8_t *lineStartPtr = 0;

    if (curToken == TOKEN_INTEGER)
    {
        long val = (long)numVal;
        if (val <=65535)
        {
            gotLineNumber = (uint16_t)val;
            lineStartPtr = tokenBuffer;
            get_next_token();
        }
        else
        {
            return ERROR_LINE_NUM_TOO_BIG;
        }
    }

    executeMode = 0;
    targetStmtNumber = 0;
    int32_t ret = parse_stmts();        /* Syntax check */
    if (ret != ERROR_NONE)
    {
        return ret;
    }

    if (gotLineNumber)
    {
        if (!do_prog_line(gotLineNumber, lineStartPtr, tokenBuffer - lineStartPtr))
        {
            ret = ERROR_OUT_OF_MEMORY;
        }
    }
    else
    {
        /* We start off executing from the input buffer */
        tokenBuffer = tokenBuf;
        executeMode = 1;
        lineNumber = 0;         /* Buffer */
        uint8_t *p = NULL;

        while (1)
        {
            get_next_token();
            stmtNumber = 0;

            /* Skip any statements? (e.g. for/next) */
            if (targetStmtNumber)
            {
                executeMode = 0;
                parse_stmts();
                executeMode = 1;
                targetStmtNumber = 0;
            }

            /* Now execute */
            ret = parse_stmts();
            if (ret != ERROR_NONE)
            {
                break;
            }

            /* Are we processing the input buffer? */
            if (!lineNumber && !jumpLineNumber && !jumpStmtNumber)
            {
                break;          /* If no control flow jumps, then just exit */
            }

            /* Are we RETURNing to the input buffer? */
            if (lineNumber && !jumpLineNumber && jumpStmtNumber)
            {
                lineNumber = 0;
            }

            if (!lineNumber && !jumpLineNumber && jumpStmtNumber)
            {
                /* We're executing the buffer, and need to jump stmt (e.g. for/next) */
                tokenBuffer = tokenBuf;
            }
            else
            {
                /* We're executing the program */
                if (jumpLineNumber || jumpStmtNumber)
                {
                    /* Line/statement number was changed e.g. goto */
                    p = find_prog_line(jumpLineNumber);
                }
                else
                {
                    /* Line number didn't change, so just move one to the next one */
                    p += *(uint16_t *)p;
                }

                /* End of program? */
                if (p == &mem[sysPROGEND])
                {
                    break;              /* End of program */
                }

                lineNumber = *(uint16_t*)(p + 2);
                tokenBuffer = p + 4;

                /* If the target for a jump is missing (e.g. line deleted) and we're on the next line
                   reset the stmt number to 0 */
                if (jumpLineNumber && jumpStmtNumber && lineNumber > jumpLineNumber)
                {
                    jumpStmtNumber = 0;
                }
            }

            if (jumpStmtNumber)
            {
                targetStmtNumber = jumpStmtNumber;
            }

            if (host_esc_pressed())
            {
                ret = ERROR_BREAK_PRESSED;
                break;
            }
        }
    }

    return ret;
}

void reset_basic(void)
{
    /* Program at the start of memory */
    sysPROGEND = 0;

    /* Stack is at the end of the program area */
    sysSTACKSTART = sysSTACKEND = sysPROGEND;

    /* variables/gosub stack at the end of memory */
    sysVARSTART = sysVAREND = sysGOSUBSTART = sysGOSUBEND = MEMORY_SIZE;
    memset(&mem[0], 0, MEMORY_SIZE);

    stopLineNumber = 0;
    stopStmtNumber = 0;
    lineNumber = 0;
}


