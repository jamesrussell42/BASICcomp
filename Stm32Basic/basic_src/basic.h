#ifndef _BASIC_H
#define _BASIC_H

/* Modified by VS for Stm32Basic project */

#include <stdint.h>
#include <stdbool.h>

#define TOKEN_EOL		                0
#define TOKEN_IDENT		                1   /* Special case - identifier follows */
#define TOKEN_INTEGER	                2	/* special case - integer follows (line numbers only) */
#define TOKEN_NUMBER	          3	    // special case - number follows
#define TOKEN_STRING	          4	    // special case - string follows

#define TOKEN_LBRACKET	        8
#define TOKEN_RBRACKET	        9
#define TOKEN_PLUS	    	      10
#define TOKEN_MINUS		          11
#define TOKEN_MULT		          12
#define TOKEN_DIV		            13
#define TOKEN_EQUALS	          14
#define TOKEN_GT		            15
#define TOKEN_LT		            16
#define TOKEN_NOT_EQ	          17
#define TOKEN_GT_EQ		          18
#define TOKEN_LT_EQ		          19
#define TOKEN_CMD_SEP	          20
#define TOKEN_SEMICOLON	        21
#define TOKEN_COMMA		          22
#define TOKEN_AND		            23	  // FIRST_IDENT_TOKEN
#define TOKEN_OR		            24
#define TOKEN_NOT		            25
#define TOKEN_PRINT		          26
#define TOKEN_LET		            27
#define TOKEN_LIST		          28
#define TOKEN_RUN		            29
#define TOKEN_GOTO		          30
#define TOKEN_REM		            31
#define TOKEN_STOP		          32
#define TOKEN_INPUT	            33
#define TOKEN_CONT		          34
#define TOKEN_IF		            35
#define TOKEN_THEN		          36
#define TOKEN_LEN		            37
#define TOKEN_VAL		            38
#define TOKEN_RND		            39
#define TOKEN_INT		            40
#define TOKEN_STR		            41
#define TOKEN_FOR		            42
#define TOKEN_TO		            43
#define TOKEN_STEP		          44
#define TOKEN_NEXT		          45
#define TOKEN_MOD		            46
#define TOKEN_NEW		            47
#define TOKEN_GOSUB		          48
#define TOKEN_RETURN	          49
#define TOKEN_DIM		            50
#define TOKEN_LEFT		          51
#define TOKEN_RIGHT	            52
#define TOKEN_MID		            53
#define TOKEN_CLS               54
#define TOKEN_PAUSE             55
#define TOKEN_POSITION          56
#define TOKEN_PIN               57
#define TOKEN_PINMODE           58
#define TOKEN_INKEY             59
#define TOKEN_SAVE              60
#define TOKEN_LOAD              61
#define TOKEN_PINREAD           62
#define TOKEN_ANALOGRD          63
#define TOKEN_DIR               64
#define TOKEN_DELETE            65

#define FIRST_IDENT_TOKEN       23
#define LAST_IDENT_TOKEN        65

#define FIRST_NON_ALPHA_TOKEN   8
#define LAST_NON_ALPHA_TOKEN    22

#define ERROR_NONE				      0

// parse errors
#define ERROR_LEXER_BAD_NUM			          1
#define ERROR_LEXER_TOO_LONG			        2
#define ERROR_LEXER_UNEXPECTED_INPUT	    3
#define ERROR_LEXER_UNTERMINATED_STRING   4
#define ERROR_EXPR_MISSING_BRACKET		    5
#define ERROR_UNEXPECTED_TOKEN			      6
#define ERROR_EXPR_EXPECTED_NUM			      7
#define ERROR_EXPR_EXPECTED_STR			      8
#define ERROR_LINE_NUM_TOO_BIG			      9
// runtime errors
#define ERROR_OUT_OF_MEMORY			          10
#define ERROR_EXPR_DIV_ZERO			          11
#define ERROR_VARIABLE_NOT_FOUND		      12
#define ERROR_UNEXPECTED_CMD			        13
#define ERROR_BAD_LINE_NUM			          14
#define ERROR_BREAK_PRESSED			          15
#define ERROR_NEXT_WITHOUT_FOR			      16
#define ERROR_STOP_STATEMENT			        17
#define ERROR_MISSING_THEN			          18
#define ERROR_RETURN_WITHOUT_GOSUB		    19
#define ERROR_WRONG_ARRAY_DIMENSIONS	    20
#define ERROR_ARRAY_SUBSCRIPT_OUT_RANGE	  21
#define ERROR_STR_SUBSCRIPT_OUT_RANGE	    22
#define ERROR_IN_VAL_INPUT			          23
#define ERROR_BAD_PARAMETER               24

#define MAX_IDENT_LEN	                    8
#define MAX_NUMBER_LEN	                  10
#define TOKEN_BUF_SIZE                    128
#define MEMORY_SIZE	                      1024*8

extern uint8_t mem[];
extern int32_t sysPROGEND;
extern int32_t sysSTACKSTART;
extern int32_t sysSTACKEND;
extern int32_t sysVARSTART;
extern int32_t sysVAREND;
extern int32_t sysGOSUBSTART;
extern int32_t sysGOSUBEND;

extern uint16_t lineNumber;	        /* 0 = input buffer */

typedef struct {
    float val;
    float step;
    float end;
    uint16_t lineNumber;
    uint16_t stmtNumber;
}ForNextData;

typedef struct {
    char *token;
    uint8_t format;
}TokenTableEntry;

extern const char* errorTable[];

void reset_basic(void);
int32_t tokenize(uint8_t *input, uint8_t *output, int32_t outputSize);
int32_t process_input(uint8_t *tokenBuf);
void print_tokens(uint8_t *p);
void list_prog(uint16_t first, uint16_t last);
uint8_t *find_prog_line(uint16_t targetLineNumber);
void delete_prog_line(uint8_t *p);
int32_t do_prog_line(uint16_t lineNum, uint8_t* tokenPtr, int32_t tokensLength);
int32_t stack_push_num(float val);
float stack_pop_num(void);
int32_t stack_push_str(char *str);
int32_t create_array(char *name, int32_t isString);
int32_t get_array_elem_offset(uint8_t **p, int32_t *pOffset);
int32_t set_num_array_elem(char *name, float val);
int32_t set_str_array_elem(char *name);
float lookup_num_array_elem(char *name, int32_t *error);
char *lookup_str_array_elem(char *name, int32_t *error);
float lookup_num_variable(char *name);
int32_t store_str_variable(char *name, char *val);
char *lookup_str_variable(char *name);
ForNextData lookup_for_next_variable(char *name);
uint8_t *find_variable(char *searchName, int32_t searchMask);
void stack_mid_str(int32_t start, int32_t len);
void delete_variable_at(uint8_t *pos);
int32_t store_num_variable(char *name, float val);
char *stack_get_str(void);
char *stack_pop_str(void);
void stack_add_2_strs(void);
void stack_left_or_right_str(int32_t len, int32_t mode);
int32_t gosub_stack_push(int32_t lineNum,int32_t stmtNumber);
int32_t gosub_stack_pop(int32_t *lineNum, int32_t *stmtNumber);
int32_t next_token(void);
int32_t get_next_token(void);
int32_t parse_number_expr(void);
int32_t parse_subscript_expr(void);
int32_t parse_fn_call_expr(void);
int32_t parse_identifier_expr(void);
int32_t parse_string_expr(void);
int32_t parse_paren_expr(void);
int32_t parse_RND(void);
int32_t parse_INKEY(void);
int32_t parse_unary_num_exp(void);
int32_t get_to_k_precedence(void);
int32_t parse_bin_op_RHS(int32_t ExprPrec, int32_t lhsVal);
int32_t parse_RUN(void);
int32_t parse_GOTO(void);
int32_t parse_PAUSE(void);
int32_t parse_LIST(void);
int32_t parse_PRINT(void);
int32_t parse_two_int_cmd(void);
int32_t parse_assignment(bool inputStmt);
int32_t parse_IF(void);
int32_t parse_FOR(void);
int32_t parse_NEXT(void);
int32_t parse_GOSUB(void);
int32_t parse_load_save_cmd(void);
int32_t parse_simple_cmd(void);
int32_t parse_DIM(void);
int32_t parse_stmts(void);

int32_t store_for_next_variable(
    char *name, 
    float start,
    float step,
    float end,
    uint16_t lineNum,
    uint16_t stmtNum);
#endif

