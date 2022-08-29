#include "optimize.h"

enum { BUFF_SIZE = 128 };

TokenAsm **asm_token;


int getLine(char *input_file) {
    char buf[BUFF_SIZE];
    int line = 0;
    FILE *fp;

    fp = fopen(input_file, "r");
    if (fp == NULL) {
        printf("cannot open output_file : <%s>\n", input_file);
        exit(1);
    }

    while (fgets(buf, BUFF_SIZE, fp) != NULL)
        line++;
    
    fclose(fp);

    return line;
}

int readFile(char *input_file, char **asms) {
    FILE *fp = fopen(input_file, "r");
    if (fp == NULL) {
        printf("cannot open output_file : <%s>\n", input_file);
        exit(1);
    }

    int i;
    for (i = 0; fgets(asms[i], BUFF_SIZE, fp) != NULL; i++) {}

    fclose(fp);
    return i;
}


void tokenize_asm(char **asms, int line) {
    asm_token = calloc(line, sizeof(TokenAsm*));
    
    for (int i = 0; i < line; i++) {
        int j = 0;
        while (asms[i][j] && isspace(asms[i][j])) j++;
        if (!asms[i][j]) {
            asm_token[i] = new_asm_token(SKIP, 0, 0, 0);
            continue;
        }

        int token_size = opecode_len(&asms[i][j]);
        if (!strncmp(&asms[i][j], "push", token_size)) {
            asm_token[i] = new_asm_token(PUSH, &asms[i][j], token_size, j);
            j += token_size;
            while (isspace(asms[i][j])) j++;
            token_size = operand2_len(&asms[i][j]);
            asm_token[i]->next = new_asm_token(OTHER, &asms[i][j], token_size, j);
        } else if (!strncmp(&asms[i][j], "pop", token_size)) {
            asm_token[i] = new_asm_token(POP, &asms[i][j], token_size, j);
            j += token_size;
            while (isspace(asms[i][j])) j++;
            token_size = operand2_len(&asms[i][j]);
            asm_token[i]->next = new_asm_token(OTHER, &asms[i][j], token_size, j);
        } else if (!strncmp(&asms[i][j], "mov", token_size)) {
            asm_token[i] = new_asm_token(MOV, &asms[i][j], token_size, -j);
            j += token_size;
            while (isspace(asms[i][j])) j++;
            token_size = operand1_len(&asms[i][j]);
            asm_token[i]->next = new_asm_token(OTHER, &asms[i][j], token_size, j);
            j += token_size;
            if (asms[i][j] != ',') { printf("error asm : mov a, b\n"); exit(1); }
            j++;
            while (isspace(asms[i][j])) j++;
            token_size = operand2_len(&asms[i][j]);
            asm_token[i]->next->next = new_asm_token(OTHER, &asms[i][j], token_size, -j);
        } else if (!strncmp(&asms[i][j], "sub", token_size)) {
            asm_token[i] = new_asm_token(SUB, &asms[i][j], token_size, -j);
            j += token_size;
            while (isspace(asms[i][j])) j++;
            token_size = operand1_len(&asms[i][j]);
            asm_token[i]->next = new_asm_token(OTHER, &asms[i][j], token_size, j);
            j += token_size;
            if (asms[i][j] != ',') { printf("error asm : sub a, b\n"); exit(1); }
            j++;
            while (isspace(asms[i][j])) j++;
            token_size = operand2_len(&asms[i][j]);
            asm_token[i]->next->next = new_asm_token(OTHER, &asms[i][j], token_size, j);
        } else
            asm_token[i] = new_asm_token(OTHER, &asms[i][j], BUFF_SIZE, j);
    }
}

void parser_asm(int line) {
    int kind11, kind12, kind13;
    int kind21, kind22, kind23;
    char *str11, *str12, *str13;
    char *str21, *str22, *str23;
    int len11, len12, len13;
    int len21, len22, len23;
    for (int i = 0; i < line-1; i++) {
        if (asm_token[i]->kind == SKIP) continue;
        
        kind11 = asm_token[i]->kind;
        len11 = asm_token[i]->len;
        str11 = asm_token[i]->str;
        if (asm_token[i]->next) {
            kind12 = asm_token[i]->next->kind;
            len12 = asm_token[i]->next->len;
            str12 = asm_token[i]->next->str;
        }
        if (asm_token[i]->next && asm_token[i]->next->next) {
            kind13 = asm_token[i]->next->next->kind;
            len13 = asm_token[i]->next->next->len;
            str13 = asm_token[i]->next->next->str;
        }
        
        kind21 = asm_token[i+1]->kind;
        len21 = asm_token[i+1]->len;
        str21 = asm_token[i+1]->str;
        if (asm_token[i+1]->next) {
            kind22 = asm_token[i+1]->next->kind;
            len22 = asm_token[i+1]->next->len;
            str22 = asm_token[i+1]->next->str;
        }
        if (asm_token[i+1]->next && asm_token[i+1]->next->next) {
            kind23 = asm_token[i+1]->next->next->kind;
            len23 = asm_token[i+1]->next->next->len;
            str23 = asm_token[i+1]->next->next->str;
        }

        if (kind11 == PUSH && kind21 == POP) {
            if (len12 == len22 && !strncmp(str12, str22, len12)) {
                asm_token[i]->kind = SKIP;
                asm_token[i+1]->kind = SKIP;
            } else {
                asm_token[i]->kind = MOV;
                asm_token[i]->str = "mov";
                asm_token[i]->len = 3;
                asm_token[i]->space *= -1;
                asm_token[i]->next->next = new_asm_token(kind12, str12, len12, -64);
                asm_token[i]->next->kind = kind22;
                asm_token[i]->next->str = str22;
                asm_token[i]->next->len = len22;
                asm_token[i+1]->kind = SKIP;
            }
        } else if (kind11 == MOV && kind21 == SUB) {
            if (!strncmp(str12, "rax", len12) && !strncmp(str22, "rax", len22) && !strncmp(str13, "rbp", len13)) {
                asm_token[i]->kind = LEA;
                asm_token[i]->str = "lea";
                asm_token[i]->len = 3;

                asm_token[i]->next->next->kind = OTHER;
                sprintf(asm_token[i]->next->next->str, "[rbp - %.*s]\n", len23, str23);
                asm_token[i]->next->next->len = 8 + len23;
                asm_token[i+1]->kind = SKIP;
            }
        }
    }
}

void write_file(char *input_file, int line) {
    FILE *fp = fopen(input_file, "w");
    if (fp == NULL) {
        printf("cannot open output_file : <%s>\n", input_file);
        exit(1);
    }

    for (int i = 0; i < line; i++) {
        if (asm_token[i]->kind == SKIP) continue;

        TokenAsm *token = asm_token[i];
        if (token->kind == OTHER) {
            if (token->space == 0)
                fprintf(fp, "%s", token->str);
            else
                fprintf(fp, "  %s", token->str);
        } else {
            fprintf(fp, "  %.*s", token->len, token->str);
            if (token = token->next) {
                fprintf(fp, " %.*s", token->len, token->str);
                if (token = token->next) {
                    fprintf(fp, ", %s", token->str);
                }
            }
            if (asm_token[i]->space > 0) fprintf(fp, "\n");
        }
    }

    fclose(fp);
}

// push rax, pop rax
// push rax, pop rdi -> mov rdi, rax
// push 0, pop rax -> mov rax, 0
void optimizeAsm(char *input_file) {
    int line = getLine(input_file);
    char **asms = calloc(line, sizeof(char*));
    for (int i = 0; i < line; i++) asms[i] = calloc(1, sizeof(char[BUFF_SIZE]));
    
    // ---------- read file ---------- //
    line = readFile(input_file, asms);
    
    // ---------- tokenize ---------- //
    tokenize_asm(asms, line);

    // ---------- parser ---------- //
    parser_asm(line);

    // ---------- write file ---------- //
    write_file(input_file, line);
}

// トークンの長さを返す
int opecode_len(char *p) {
    int len = 0;
    while (!isspace(*p)) {
        p++;
        len++;
    }
    return len;
}

// トークンの長さを返す
int operand1_len(char *p) {
    int len = 0;
    while (*p != ',') {
        p++;
        len++;
    }
    return len;
}

// トークンの長さを返す
int operand2_len(char *p) {
    int len = 0;
    
    while (*p != '\n') {
        p++;
        len++;
    }
    return len;
}

TokenAsm *new_asm_token(AsmKind kind, char *str, int len, int space) {
    TokenAsm *token = calloc(1, sizeof(TokenAsm));
    token->kind = kind;
    token->str = str;
    token->len = len;
    token->space = space;
    return token;
}