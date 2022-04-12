#include "grccnr.h"

// 関数の引数の個数が一致していたら1
int arg_check(Func *func, Node *node) {
    int argc1 = 0, argc2 = 0;
    LVar *arg = func->arg;
    for (; node->arg[argc1]; argc1++) {}
    for (; arg; arg = arg->next) argc2++;
    
    if (argc1 == argc2) return 1;
    else return 0;
}

// 一致する関数の型を返す
VarType *func_type(Node *node) {
    for (int i = 0; code[i]; i++) {
        if (strcmp(code[i]->func_name, node->func_name) == 0 &&
            arg_check(code[i], node))
            return code[i]->type;
    }

    if (strcmp(now_func->func_name, node->func_name) == 0 && 
        arg_check(now_func, node)) 
        return now_func->type;
    
    // printfはlink.cのstdio.hを使う
    if (strcmp("printf", node->func_name) == 0) {
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
    switch (type->ty) {
        case INT: 
            return INT_SIZE;
        case CHAR: 
            return CHAR_SIZE;
        case PTR:
            return PTR_SIZE;
        case ARRAY:
            return type->array_size * get_size(type->ptr_to);
        default:
            error_at(token->str, "型が処理できません");
    }
}

// 戻り値 Type:(int,ptr) ptrs:ptr?
VarType *AST_type(Node *node) {
    VarType *lhs_var_type, *rhs_var_type;
    VarType *var_type; // 整数や比較演算の場合に使う
    int lsize, rsize;
    switch (node->kind) {
        case ND_ADD:
            lhs_var_type = AST_type(node->lhs);
            rhs_var_type = AST_type(node->rhs);
            //printf("#lhs_ptr=%d, lhs_arsz=%d, rhs_ptr=%d, rhs_arsz=%d\n",
            //    lhs_var_type->ptrs, lhs_var_type->array_size,
            //    rhs_var_type->ptrs, rhs_var_type->array_size);
            break;
        case ND_SUB:
            lhs_var_type = AST_type(node->lhs);
            rhs_var_type = AST_type(node->rhs);
            break;
        case ND_MUL:
            lhs_var_type = AST_type(node->lhs);
            rhs_var_type = AST_type(node->rhs);
            break;
        case ND_DIV:
            lhs_var_type = AST_type(node->lhs);
            rhs_var_type = AST_type(node->rhs);
            break;
        case ND_NUM:
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            /*
            if (node->val < (1 << 8))
                var_type->ty = CHAR;
            else if ((long int)node->val < ((long int)1 << 32))
                var_type->ty = INT;
            else
                error_at(token->str, "数字が4バイトより大きいため処理できません");
            */
            return var_type;
        case ND_STR:
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = PTR;
            var_type->ptr_to = calloc(1, sizeof(VarType));
            var_type->ptr_to->ty = CHAR;
            return var_type;
        case ND_ADDR:
            AST_type(node->lhs);
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = PTR;
            return var_type;
        case ND_DEREF:
            var_type = AST_type(node->lhs);
            if (var_type->ptr_to)
                var_type = var_type->ptr_to;
            else
                error_at(token->str, "式内のポインタの型が一致しません");
            return var_type;
        case ND_EQ:
            AST_type(node->lhs);
            AST_type(node->rhs);
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            return var_type;
        case ND_NE:
            AST_type(node->lhs);
            AST_type(node->rhs);
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            return var_type;
        case ND_LT:
            AST_type(node->lhs);
            AST_type(node->rhs);
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            return var_type;
        case ND_LE:
            AST_type(node->lhs);
            AST_type(node->rhs);
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            return var_type;
        case ND_ASSIGN:
            lhs_var_type = AST_type(node->lhs);
            rhs_var_type = AST_type(node->rhs);
            //printf("#%d %d\n", lhs_var_type->size, rhs_var_type->size);
            lsize = get_size(lhs_var_type);
            rsize = get_size(rhs_var_type);
            if (lsize > 8 || rsize > 8)
                error_at(token->str, "右辺と左辺の型が一致しません");
            return rhs_var_type;
        case ND_LVAR:
            //printf("# type->ptrs=%d, type->array_size=%d\n", var_type->ptrs, var_type->array_size);
            return node->lvar->type;
        case ND_RETURN:
            lhs_var_type = AST_type(node->lhs);
            if (returnable(now_func->type, lhs_var_type))
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
            error_at(token->str, "block節は評価出来ません");
        case ND_CALL_FUNC:
            return func_type(node);
        default:
            error_at(token->str, "予期しないNodeKindです");
        
    }

    //printf("kind=%d lhs_kind=%d rhs_kind=%d lhs_ty=%d rhs_ty=%d lhs_ptr=%d rhs_ptr=%d\n", 
    //        node->kind, node->lhs->kind, node->rhs->kind, lhs_var_type->ty, rhs_var_type->ty,
    //        lhs_var_type->ptrs, rhs_var_type->ptrs);
    
    // 演算の場合

    // それぞれ指している値の意味が異なる場合
    // ptrs--してからsizeを取得 例) int*なら+4
    if (lhs_var_type->ptr_to && !rhs_var_type->ptr_to) {
        node->rhs = new_binary(ND_MUL, node->rhs, new_num(get_size(lhs_var_type->ptr_to)));
        return lhs_var_type;
    }
    else if (!lhs_var_type->ptr_to && rhs_var_type->ptr_to) {
        node->lhs = new_binary(ND_MUL, node->lhs, new_num(get_size(rhs_var_type->ptr_to)));
        return rhs_var_type;
    }

    // ポインタとポインタ(アドレスとアドレス)の演算
    // lhsにrhsを足すイメージでlhs_var_typeを返す
    return lhs_var_type;
}