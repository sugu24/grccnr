#include "grccnr.h"

// 関数の引数の個数が一致していたら1
int arg_check(Func *func, Node *node) {
    int argc = 0;
    while (node->arg[argc]) argc++;
    LVar *arg = func->arg;

    while (1) {
        // argの比較
        if (!(argc && node->arg[argc-1]) && !arg) return 1;
        else if (!(node->arg[argc-1] && arg)) return 0;

        // same_type(AST_type(呼び出し元, 関数引数定義))
        if (!same_type(AST_type(0, node->arg[argc-1]), arg->type) && node->arg[argc-1]->kind != ND_SUB) 
            return 0;
        
        argc--;
        while (arg->next && arg->offset == arg->next->offset) arg = arg->next;
        arg = arg->next;
    }
}

int same_type(VarType *v1, VarType *v2) {
    while (1) {
        if ((v1->ty == ARRAY && v2->ty == PTR) || (v1->ty == PTR && v2->ty == ARRAY)) {}
        else if (v1->ty <= 4 && v2->ty <= 4) {}
        else if (v1->ty == VOID) {}
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
        
    VarType *cfunc = cfunc_type(node);
    if (cfunc) return cfunc;

    error_at(token->str, "一致する関数がありません");
}

// 戻り値が関数の戻り値と一致しているか
int returnable(VarType *func_type, VarType *return_type) {
    while (func_type->ty == return_type->ty) {
        if (func_type->ty == VOID)
            error_at(token->str, "関数の型が void であるが戻り値が指定されています");
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
        case LONG_LONG_INT:
            res = LONG_LONG_INT_SIZE;
            break;
        case CHAR: 
            res = CHAR_SIZE;
            break;
        case VOID:
            res = VOID_SIZE;
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

int get_cal_size(VarType *type) {
    switch (type->ty) {
        case CHAR:
            return CHAR_SIZE;
        case INT:
            return INT_SIZE;
        case LONG_LONG_INT:
            return LONG_LONG_INT_SIZE;
        case VOID:
            return VOID_SIZE;
        case PTR:
        case ARRAY:
            return PTR_SIZE;
        case STRUCT:
            return get_cal_size(type->struct_p->membar->type);
        default:
            error_at(token->str, "unexpected type");
    }
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
    VarType *lhs_var_type = NULL, *rhs_var_type = NULL;
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
            if (get_cal_size(lhs_var_type) >= get_cal_size(rhs_var_type))
                var_type = lhs_var_type;
            else
                var_type = rhs_var_type;
            break;
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
            if (get_cal_size(lhs_var_type) >= get_cal_size(rhs_var_type))
                var_type = lhs_var_type;
            else
                var_type = rhs_var_type;
            break;
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
            if (get_cal_size(lhs_var_type) >= get_cal_size(rhs_var_type))
                var_type = lhs_var_type;
            else
                var_type = rhs_var_type;
            break;
        case ND_LOGICAL_ADD:
            AST_type(ch, node->lhs);
            AST_type(ch, node->rhs);
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            break;
        case ND_LOGICAL_AND:
            AST_type(ch, node->lhs);
            AST_type(ch, node->rhs);
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            break;
        case ND_NUM:
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = LONG_LONG_INT;
            break;
        case ND_CHAR:
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = CHAR;
            break;
        case ND_STR:
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = ARRAY;
            var_type->array_size = node->lvar->len;
            var_type->ptr_to = calloc(1, sizeof(VarType));
            var_type->ptr_to->ty = CHAR;
            break;
        case ND_STR_PTR:
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = PTR;
            var_type->ptr_to = calloc(1, sizeof(VarType));
            var_type->ptr_to->ty = CHAR;
            break;
        case ND_NULL:
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = PTR;
            var_type->ptr_to = calloc(1, sizeof(VarType));
            var_type->ptr_to->ty = VOID;
            break;
        case ND_ADDR:
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = PTR;
            var_type->ptr_to = AST_type(ch, node->lhs);
            break;
        case ND_DEREF:
            var_type = AST_type(ch, node->lhs);
            if (var_type->ty == ARRAY || var_type->ty == PTR)
                var_type = var_type->ptr_to;
            else
                error_at(token->str, "式内のポインタの型が一致しません");
            break;
        case ND_INDEX:
            var_type = AST_type(ch, node->lhs);
            if (var_type->ty == STRUCT || var_type->ty == ARRAY || var_type->ty == PTR) {
                var_type = var_type->ptr_to;
            } else
                error_at(token->str, "式内のポインタの型が一致しません");
            break;
        case ND_MEMBAR_ACCESS:
            //printf("%d %d\n", node->lhs->kind, node->lhs->lhs->kind);
            var_type = AST_type(ch, node->lhs);
            //printf("%d\n", var_type->ty);
            break;
        case ND_EQ:
            lhs_var_type = AST_type(ch, node->lhs);
            rhs_var_type = AST_type(ch, node->rhs);

            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            break;
        case ND_NE:
            lhs_var_type = AST_type(ch, node->lhs);
            rhs_var_type = AST_type(ch, node->rhs);

            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            break;
        case ND_LT:
            lhs_var_type = AST_type(ch, node->lhs);
            rhs_var_type = AST_type(ch, node->rhs);
            
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            break;
        case ND_LE:
            lhs_var_type = AST_type(ch, node->lhs);
            rhs_var_type = AST_type(ch, node->rhs);
            
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            break;
        case ND_NOT:
            AST_type(ch, node->lhs);
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            break;
        case ND_ASSIGN:
            lhs_var_type = AST_type(ch, node->lhs);
            rhs_var_type = AST_type(ch, node->rhs);
            if (rhs_var_type->ty == VOID)
                error_at(token->str, "void 値が無視されていません");
                
            lsize = get_size(lhs_var_type);
            if (node->rhs->kind != ND_STR) rsize = get_cal_size(rhs_var_type);
            else rsize = get_size(rhs_var_type);
            
            if ((node->rhs->kind == ND_STR && lsize <= rsize) || (node->rhs->kind != ND_STR && (rsize > 8 || lsize > 8)))
                error_at(token->str, "assign size is unexpected");
            
            if (lsize > rsize && lsize == 8)
                node->rhs->cltq = 1;
            
            var_type = lhs_var_type;
            return var_type;
        case ND_LVAR_ADD:
            var_type = AST_type(ch, node->lhs);
            break;
        case ND_LVAR_SUB:
            var_type = AST_type(ch, node->lhs);
            break;
        case ND_LVAR:
            var_type = node->lvar->type;
            break;
        case ND_MEMBAR:
            var_type = node->lvar->type;
            break;
        case ND_RETURN:
            if (node->lhs) lhs_var_type = AST_type(ch, node->lhs);
            else { 
                lhs_var_type = calloc(1, sizeof(VarType));
                lhs_var_type->ty = VOID;
            }

            if (returnable(now_func->func_type_name->type, lhs_var_type))
                return lhs_var_type;
            else if (returnable(now_func->func_type_name->type, node->cast))
                return node->cast;
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
        case ND_CONTINUE:
            error_at(token->str, "continueは評価できません");
        case ND_BREAK:
            error_at(token->str, "breakは評価できません");
        case ND_SWITCH:
            error_at(token->str, "switchは評価できません");
        case ND_CASE:
            error_at(token->str, "caseは評価できません");
        case ND_DEFAULT:
            error_at(token->str, "defaultは評価できません");
        case ND_BLOCK:
            return AST_type(ch, node->next_stmt);
        case ND_CALL_FUNC:
            var_type = func_type(node);
            break;
        default:
            error_at(token->str, "予期しないNodeKindです");
    }

    // cltqを行うべきか
    if (lhs_var_type && rhs_var_type && !node->lhs->access) {
        lsize = get_cal_size(lhs_var_type);
        rsize = get_cal_size(rhs_var_type);
        
        if (lsize < rsize && rsize == 8)
            node->lhs->cltq = 1;
        else if (lsize > rsize && lsize == 8)
            node->rhs->cltq = 1;
    }

    if (node->cast && get_cal_size(node->cast) == 8 && get_cal_size(var_type) < 8)
        node->cltq = 1;

    // castされているか
    if (node->kind != ND_ADD && node->kind != ND_SUB)
        if (node->cast)
            return node->cast;
        else
            return var_type;
            
    // 演算の場合
    // それぞれ指している値の意味が異なる場合
    // ptrs--してからsizeを取得 例) int*なら+4

    Type lt = lhs_var_type->ty, rt = rhs_var_type->ty;
    
    if (lt == STRUCT || (lt == PTR && lhs_var_type->ptr_to->ty == STRUCT)) {
        if (ch) node->rhs->offset = get_membar_offset(node->rhs->lvar);
        if (node->cast)
            return node->cast;
        return rhs_var_type;
    }

    if (lt == rt) {
        if (node->cast)
            return node->cast;
        return lhs_var_type;
    } 
    
    if ((1 <= lt && lt <= 4) && (1 <= rt && rt <= 4)) {
        if (node->cast)
            return node->cast;
        return lhs_var_type;
    }

    if ((lt == PTR || lt == ARRAY) && (rt == CHAR || rt == INT || rt == LONG_LONG_INT))  {
        if (ch) node->rhs = new_binary(ND_MUL, node->rhs, new_num(get_size(lhs_var_type->ptr_to)));
        if (node->cast)
            return node->cast;
        return lhs_var_type;
    }
    
    if ((lt == CHAR || lt == INT || lt == LONG_LONG_INT) && (rt == PTR && rt == ARRAY)) {
        if (ch) node->lhs = new_binary(ND_MUL, node->lhs, new_num(get_size(rhs_var_type->ptr_to)));
        if (node->cast)
            return node->cast;
        return rhs_var_type;
    }

    error_at(token->str, "左辺の型 %d と 右辺の型 %d との演算は出来ません", lt, rt);
}