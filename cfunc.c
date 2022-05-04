#include "grccnr.h"

VarType *cfunc_type(Node *node) {
    // printfはlink.cのstdio.hを使う
    if (strcmp("printf", node->func_name) == 0) {
        VarType *p = calloc(1, sizeof(VarType));
        p->ty= INT;
        return p;
    }
    if (strcmp("scanf", node->func_name) == 0) {
        VarType *p = calloc(1, sizeof(VarType));
        p->ty = INT;
        return p;
    }
    // callocはlink.cのstdlib.hを使う
    if (strcmp("calloc", node->func_name) == 0) {
        VarType *p = calloc(1, sizeof(VarType));
        p->ty= PTR;
        p->ptr_to = calloc(1, sizeof(VarType));
        return p;
    }
    // strncmpはlink.cのstring.hを使う
    if (strcmp("strncmp", node->func_name) == 0) {
        VarType *p = calloc(1, sizeof(VarType));
        p->ty= INT;
        return p;
    }
    // fopenはlink.cのstring.hを使う
    if (strcmp("fopen", node->func_name) == 0) {
        VarType *p = calloc(1, sizeof(VarType));
        p->ty= PTR;
        p->ptr_to = calloc(1, sizeof(VarType));
        return p;
    }
    // fcloseはlink.cのstring.hを使う
    if (strcmp("fclose", node->func_name) == 0) {
        VarType *p = calloc(1, sizeof(VarType));
        p->ty= INT;
        return p;
    }
    // fseekはlink.cのstring.hを使う
    if (strcmp("fseek", node->func_name) == 0) {
        VarType *p = calloc(1, sizeof(VarType));
        p->ty= INT;
        return p;
    }
    // ftellはlink.cのstring.hを使う
    if (strcmp("ftell", node->func_name) == 0) {
        VarType *p = calloc(1, sizeof(VarType));
        p->ty= INT;
        return p;
    }
    // freadはlink.cのstring.hを使う
    if (strcmp("fread", node->func_name) == 0) {
        VarType *p = calloc(1, sizeof(VarType));
        p->ty= INT;
        return p;
    }
    return NULL;
}