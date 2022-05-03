#include "grccnr.h"

// 関数の引数の個数が一致していたら1
int arg_check(Func *func, Node *node) {
    int argc = 0;
    while (node->arg[argc]) argc++;
    
    LVar *arg = func->arg;

    while (1) {
        // argの比較
        if (!node->arg[argc-1] && !arg) return 1;
        else if (!(node->arg[argc-1] && arg)) return 0;

        // same_type(AST_type(呼び出し元, 関数引数定義))
        if (!same_type(AST_type(0, node->arg[argc-1]), arg->type)) return 0;
        
        argc--;
        while (arg->next && arg->offset == arg->next->offset) arg = arg->next;
        arg = arg->next;
    }
}

int same_type(VarType *v1, VarType *v2) {
    while (1) {
        //printf("v1 %d v2 %d\n", v1->ty, v2->ty);
        if ((v1->ty == ARRAY && v2->ty == PTR) || (v1->ty == PTR && v2->ty == ARRAY)) {}
        else if (v1->ty != v2->ty) return 0;
        else if (v1->ty == STRUCT && get_size(v1) != get_size(v2)) return 0;

        if (v1->ptr_to && v2->ptr_to) {
            v1 = v1->ptr_to;
            v2 = v2->ptr_to;
        } else if (!v1->ptr_to && !v2->ptr_to)
            return 1;
        else 
            return 0;
    
    }
    
}

// 一致する関数の型を返す
VarType *func_type(Node *node) {
    // 定義済みの関数
    for (int i = 0; code[i]; i++) {
        if (strcmp(code[i]->func_type_name->name, node->func_name) == 0) {
            if (arg_check(code[i], node))
                return code[i]->func_type_name->type;
            else
                error_at(token->str, "引数が異なります");
        }
    }
    
    // prototypeで宣言された関数
    for (Prototype *proto = prototype; proto; proto = proto->next) {
        if (strcmp(proto->func->func_type_name->name, node->func_name) == 0) {
            if (arg_check(proto->func, node)) 
                return proto->func->func_type_name->type;
            else
                error_at(token->str, "引数の型が異なります");
        }
    }
    
    // 定義途中の関数
    if (strcmp(now_func->func_type_name->name, node->func_name) == 0 && 
        arg_check(now_func, node)) 
        return now_func->func_type_name->type;
        
    // printfはlink.cのstdio.hを使う
    if (strcmp("printf", node->func_name) == 0) {
        VarType *p = calloc(1, sizeof(VarType));
        p->ty= INT;
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
    error_at(token->str, "一致する関数がありません");
}

// 戻り値が関数の戻り値と一致しているか
int returnable(VarType *func_type, VarType *return_type) {
    while (func_type->ty == return_type->ty) {
        if (!func_type->ptr_to && !return_type->ptr_to)
            return 1;
        else if (!func_type->ptr_to || !return_type->ptr_to)
            return 0;
        func_type = func_type->ptr_to;
        return_type = return_type->ptr_to;
    }
    return 0;
}

// 変数のサイズを返す
// get_size呼ぶときは*を処理した後
int get_size(VarType *type) {
    int res = 0;
    switch (type->ty) {
        case INT: 
            res = INT_SIZE;
            break;
        case CHAR: 
            res = CHAR_SIZE;
            break;
        case PTR:
            res = PTR_SIZE;
            break;
        case ARRAY:
            res = type->array_size * get_size(type->ptr_to);
            break;
        case STRUCT:
            for (LVar *var = type->struct_p->membar; var; var = var->next)
                res += get_size(var->type);
            break;
        default:
            error_at(token->str, "型が処理できません");
    }
    return res;
}

// VarTypeが配列抜きで何の型を格納しているか返す
VarType *get_type(VarType *type) {
    while (type->ty == ARRAY) type = type->ptr_to;
    return type;
}

// 初期化時のグローバル変数の配列の場合にoffsetを返す
int get_offset(Node *node) {
    if (node->kind != ND_ASSIGN)
        error("get_offsetはkindがND_ASSIGNである必要があります");

    int offset = 0;
    node = node->lhs;
    while (node->kind != ND_LVAR) {
        offset += node->lhs->rhs->lhs->val * node->lhs->rhs->rhs->val;
        node = node->lhs->lhs;
    }
    
    return offset;
}

// 構造体でのメンバ変数のオフセット
int get_membar_offset(LVar *membar) {
    int offset = 0;
    for (membar = membar->next; membar; membar = membar->next) {
        offset += get_size(membar->type);
    }
    return offset;
}

// 戻り値 Type:(int,ptr) ptrs:ptr?
VarType *AST_type(int ch, Node *node) {
    // printf("kind %d\n", node->kind);
    VarType *lhs_var_type, *rhs_var_type;
    VarType *var_type; // 整数や比較演算の場合に使う
    int lsize, rsize;
    switch (node->kind) {
        case ND_ADD:
            lhs_var_type = AST_type(ch, node->lhs);
            rhs_var_type = AST_type(ch, node->rhs);
            break;
        case ND_SUB:
            lhs_var_type = AST_type(ch, node->lhs);
            rhs_var_type = AST_type(ch, node->rhs);
            break;
        case ND_MUL:
            lhs_var_type = AST_type(ch, node->lhs);
            rhs_var_type = AST_type(ch, node->rhs);
            if (lhs_var_type->ty == ARRAY  ||
                lhs_var_type->ty == PTR    ||
                lhs_var_type->ty == STRUCT)
                error_at(token->str, "積の左の項がポインタか配列か構造体で演算できません");
            if (rhs_var_type->ty == ARRAY  ||
                rhs_var_type->ty == PTR    ||
                rhs_var_type->ty == STRUCT)
                error_at(token->str, "積の左の項がポインタか配列か構造体で演算できません");
            return lhs_var_type;
        case ND_DIV:
            lhs_var_type = AST_type(ch, node->lhs);
            rhs_var_type = AST_type(ch, node->rhs);
            if (lhs_var_type->ty == ARRAY  ||
                lhs_var_type->ty == PTR    ||
                lhs_var_type->ty == STRUCT)
                error_at(token->str, "積の左の項がポインタか配列か構造体で演算できません");
            if (rhs_var_type->ty == ARRAY  ||
                rhs_var_type->ty == PTR    ||
                rhs_var_type->ty == STRUCT)
                error_at(token->str, "積の左の項がポインタか配列か構造体で演算できません");
            return lhs_var_type;
        case ND_MOD:
            lhs_var_type = AST_type(ch, node->lhs);
            rhs_var_type = AST_type(ch, node->rhs);
            if (lhs_var_type->ty == ARRAY  ||
                lhs_var_type->ty == PTR    ||
                lhs_var_type->ty == STRUCT)
                error_at(token->str, "積の左の項がポインタか配列か構造体で演算できません");
            if (rhs_var_type->ty == ARRAY  ||
                rhs_var_type->ty == PTR    ||
                rhs_var_type->ty == STRUCT)
                error_at(token->str, "積の左の項がポインタか配列か構造体で演算できません");
            return lhs_var_type;
        case ND_LOGICAL_ADD:
            AST_type(ch, node->lhs);
            AST_type(ch, node->rhs);
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            return var_type;
        case ND_LOGICAL_AND:
            AST_type(ch, node->lhs);
            AST_type(ch, node->rhs);
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            return var_type;
        case ND_NUM:
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            return var_type;
        case ND_CHAR:
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = CHAR;
            return var_type;
        case ND_STR:
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = ARRAY;
            var_type->array_size = node->lvar->len;
            var_type->ptr_to = calloc(1, sizeof(VarType));
            var_type->ptr_to->ty = CHAR;
            return var_type;
        case ND_STR_PTR:
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = PTR;
            var_type->ptr_to = calloc(1, sizeof(VarType));
            var_type->ptr_to->ty = CHAR;
            return var_type;
        case ND_ADDR:
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = PTR;
            var_type->ptr_to = AST_type(ch, node->lhs);
            return var_type;
        case ND_DEREF:
            var_type = AST_type(ch, node->lhs);
            if (var_type->ty == ARRAY || var_type->ty == PTR)
                var_type = var_type->ptr_to;
            else
                error_at(token->str, "式内のポインタの型が一致しません");
            return var_type;
        case ND_INDEX:
            var_type = AST_type(ch, node->lhs);
            if (var_type->ty == STRUCT || var_type->ty == ARRAY || var_type->ty == PTR) {
                var_type = var_type->ptr_to;
            } else
                error_at(token->str, "式内のポインタの型が一致しません");
            return var_type;
        case ND_MEMBAR_ACCESS:
            var_type = AST_type(ch, node->lhs);
            return var_type;
        case ND_EQ:
            AST_type(ch, node->lhs);
            AST_type(ch, node->rhs);
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            return var_type;
        case ND_NE:
            AST_type(ch, node->lhs);
            AST_type(ch, node->rhs);
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            return var_type;
        case ND_LT:
            AST_type(ch, node->lhs);
            AST_type(ch, node->rhs);
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            return var_type;
        case ND_LE:
            AST_type(ch, node->lhs);
            AST_type(ch, node->rhs);
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            return var_type;
        case ND_ASSIGN:
            lhs_var_type = AST_type(ch, node->lhs);
            rhs_var_type = AST_type(ch, node->rhs);
            //printf("#%d %d\n", lhs_var_type->size, rhs_var_type->size);
            lsize = get_size(lhs_var_type);
            rsize = get_size(get_type(rhs_var_type));
            if (rsize >= 8 && lsize < rsize)
                error_at(token->str, "右辺より左辺の方がサイズが小さいです");
            return lhs_var_type;
        case ND_LVAR_ADD:
            return AST_type(ch, node->lhs);
        case ND_LVAR_SUB:
            return AST_type(ch, node->lhs);
        case ND_LVAR:
            return node->lvar->type;
        case ND_MEMBAR:
            return node->lvar->type;
        case ND_RETURN:
            lhs_var_type = AST_type(ch, node->lhs);
            if (returnable(now_func->func_type_name->type, lhs_var_type))
                return lhs_var_type;
            else
                error_at(token->str, "戻り値の型が異なります");
        case ND_IF:
            error_at(token->str, "if節は評価出来ません");
        case ND_ELSE_IF:
            error_at(token->str, "else if節は評価出来ません");
        case ND_ELSE:
            error_at(token->str, "else節は評価できません");
        case ND_WHILE:
            error_at(token->str, "while節は評価出来ません");
        case ND_FOR:
            error_at(token->str, "for節は評価出来ません");
        case ND_BLOCK:
            return AST_type(ch, node->next_stmt);
        case ND_CALL_FUNC:
            lhs_var_type = func_type(node);
            return lhs_var_type;
        default:
            error_at(token->str, "予期しないNodeKindです");
        
    }

    // 演算の場合
    // それぞれ指している値の意味が異なる場合
    // ptrs--してからsizeを取得 例) int*なら+4
    
    if (!ch) return lhs_var_type;

    Type lt = lhs_var_type->ty, rt = rhs_var_type->ty;
    if (lt == rt) {
        return lhs_var_type;
    } 
    
    if ((lt == INT || lt == CHAR) && (rt == INT || rt == CHAR))
        return lhs_var_type;
    
    if ((lt == PTR || lt == ARRAY) && (rt == CHAR || rt == INT))  {
        node->rhs = new_binary(ND_MUL, node->rhs, new_num(get_size(lhs_var_type->ptr_to)));
        return lhs_var_type;
    }
    
    if ((lt == CHAR || lt == INT) && (rt == PTR && rt == ARRAY)) {
        node->lhs = new_binary(ND_MUL, node->lhs, new_num(get_size(rhs_var_type->ptr_to)));
        return rhs_var_type;
    }
    
    if (lt == STRUCT) {
        node->rhs->offset = get_membar_offset(node->rhs->lvar);
        return rhs_var_type;
    }

    error_at(token->str, "左辺の型 %d と 右辺の型 %d との演算は出来ません", lt, rt);
}