#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct TokenAsm TokenAsm;
typedef enum AsmKind AsmKind;

enum AsmKind {
    POP = 1,
    PUSH,
    MOV,
    SUB,
    LEA,
    OTHER,
    SKIP,
};

struct TokenAsm {
    TokenAsm *next;
    AsmKind kind;
    char *str;
    int len;
    int space;
};

extern TokenAsm **asm_token;

int opecode_len(char *p);
int operand1_len(char *p);
int operand2_len(char *p);
TokenAsm *new_asm_token(AsmKind kind, char *str, int len, int space);