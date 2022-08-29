#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grccnr.h"

VarType *cfunc_type(Node *node) {
    VarType *p = calloc(1, sizeof(VarType));
    // printfはlink.cのstdio.hを使う
    if (strcmp("printf", node->func_name) == 0)
        p->ty= INT;
    

    // argc, argv + 1つの変数でscanfを実行するとsegment fault
    else if (strcmp("scanf", node->func_name) == 0)
        p->ty = INT;
        
    // callocはlink.cのstdlib.hを使う
    else if (strcmp("calloc", node->func_name) == 0) {
        p->ty = PTR;
        p->ptr_to = calloc(1, sizeof(VarType));
        p->ptr_to->ty = VOID;
    }
    
    else if (strcmp("malloc", node->func_name) == 0) {
        p->ty = PTR;
        p->ptr_to = calloc(1, sizeof(VarType));
        p->ptr_to->ty = VOID;
    }

    else if (strcmp("free", node->func_name) == 0)
        p->ty = VOID;

    // strncmpはlink.cのstring.hを使う
    else if (strcmp("strncmp", node->func_name) == 0)
        p->ty = INT;
    
    else if (strcmp("strncpy", node->func_name) == 0) {
        p->ty = PTR;
        p->ptr_to = calloc(1, sizeof(VarType));
        p->ptr_to->ty = CHAR;
    }
    
    else if (strcmp("strchr", node->func_name) == 0) {
        p->ty = PTR;
        p->ptr_to = calloc(1, sizeof(VarType));
        p->ptr_to->ty = CHAR;
    }

    else if (strcmp("memcmp", node->func_name) == 0) {
        p->ty = PTR;
        p->ptr_to = calloc(1, sizeof(VarType));
        p->ptr_to->ty = VOID;
    }

    else if (strcmp("memcpy", node->func_name) == 0) {
        p->ty = PTR;
        p->ptr_to = calloc(1, sizeof(VarType));
        p->ptr_to->ty = VOID;
    }
    
    else if (strcmp("strlen", node->func_name) == 0)
        p->ty = INT;
    
    else if (strcmp("isdigit", node->func_name) == 0)
        p->ty = INT;
    
    else if (strcmp("strtol", node->func_name) == 0)
        p->ty = INT;

    else if (strcmp("strtoll", node->func_name) == 0)
        p->ty = LONG_LONG_INT;

    // fopenはlink.cのstring.hを使う
    else if (strcmp("fopen", node->func_name) == 0) {
        p->ty = PTR;
        p->ptr_to = calloc(1, sizeof(VarType));
    }

    // fcloseはlink.cのstring.hを使う
    else if (strcmp("fclose", node->func_name) == 0)
        p->ty = INT;
        
    // fseekはlink.cのstring.hを使う
    else if (strcmp("fseek", node->func_name) == 0) 
        p->ty = INT;
        
    // ftellはlink.cのstring.hを使う
    else if (strcmp("ftell", node->func_name) == 0)
        p->ty = INT;
    
    // freadはlink.cのstring.hを使う
    else if (strcmp("fread", node->func_name) == 0)
        p->ty = INT;
        
    else if (strcmp("isspace", node->func_name) == 0)
        p->ty = INT;
    
    else if (strcmp("exit", node->func_name) == 0)
        p->ty = VOID;

    if (p->ty) 
        return p;
    else
        return NULL;
}
