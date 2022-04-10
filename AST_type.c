#include "grccnr.h"

int arg_check(Func *func, Node *node) {
    int argc1 = 0, argc2 = 0;
    LVar *arg = func->arg;
    for (; node->arg[argc1]; argc1++) {}
    for (; arg; arg = arg->next) argc2++;
    
    if (argc1 == argc2) return 1;
    else return 0;
}

VarType *func_type(Node *node) {
    for (int i = 0; code[i]; i++) {
        if (strcmp(code[i]->func_name, node->func_name) == 0 &&
            arg_check(code[i], node))
            return code[i]->type;
    }

    if (strcmp(now_func->func_name, node->func_name) == 0 && 
        arg_check(now_func, node)) 
        return now_func->type;
    
    error_at(token->str, "一致する関数がありません");
}

int unassignable(VarType *lhs, VarType *rhs) {
    // printf("lhs->ty=%d, lhs->ptrs=%d, rhs->ty=%d, rhs->ptrs=%d\n", lhs->ty, lhs->ptrs, rhs->ty, rhs->ptrs);
    if ((lhs->ty == CHAR && lhs->ptrs == 0 && rhs->ty == INT) ||
        (lhs->ty == INT && rhs->ty == CHAR && rhs->ptrs == 0) ||
        (lhs->ty == rhs->ty && lhs->ptrs == rhs->ptrs))
        return 0;
    else 
        return 1;
}

// 戻り値 Type:(int,ptr) ptrs:ptr?
VarType *AST_type(Node *node) {
    VarType *lhs_var_type, *rhs_var_type;
    VarType *var_type; // 整数や比較演算の場合に使う
    switch (node->kind) {
        case ND_ADD:
            lhs_var_type = AST_type(node->lhs);
            rhs_var_type = AST_type(node->rhs);
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
            var_type->ptrs = 0;
            return var_type;
        case ND_ADDR:
            return AST_type(node->lhs);
        case ND_DEREF:
            lhs_var_type = AST_type(node->lhs);
            lhs_var_type->ptrs--;
            if (lhs_var_type->ptrs < 0)
                error_at(token->str, "式内のポインタの型が一致しません");
            return lhs_var_type;
        case ND_EQ:
            lhs_var_type = AST_type(node->lhs);
            rhs_var_type = AST_type(node->rhs);
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            var_type->ptrs = 0;
            return var_type;
        case ND_NE:
            lhs_var_type = AST_type(node->lhs);
            rhs_var_type = AST_type(node->rhs);
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            var_type->ptrs = 0;
            return var_type;
        case ND_LT:
            lhs_var_type = AST_type(node->lhs);
            rhs_var_type = AST_type(node->rhs);
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            var_type->ptrs = 0;
            return var_type;
        case ND_LE:
            lhs_var_type = AST_type(node->lhs);
            rhs_var_type = AST_type(node->rhs);
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = INT;
            var_type->ptrs = 0;
            return var_type;
        case ND_ASSIGN:
            lhs_var_type = AST_type(node->lhs);
            rhs_var_type = AST_type(node->rhs);
            if (unassignable(lhs_var_type, rhs_var_type))
                error_at(token->str, "右辺と左辺の型が一致しません");
            return rhs_var_type;
        case ND_LVAR:
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = node->lvar->type->ty;
            var_type->ptrs = node->lvar->type->ptrs;
            var_type->array = node->lvar->type->array_size;
            return var_type;
        case ND_ARRAY:
            var_type = calloc(1, sizeof(VarType));
            var_type->ty = node->lvar->type->ty;
            var_type->ptrs = node->lvar->type->ptrs;
            return var_type;
        case ND_RETURN:
            lhs_var_type = AST_type(node->lhs);
            if (now_func->type->ty == lhs_var_type->ty &&
                now_func->type->ty == lhs_var_type->ptrs)
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
    // char型a[index]の場合
    if (lhs_var_type->ty == CHAR && rhs_var_type->ty == INT) {
        return lhs_var_type;
    }
    // ptrsが1,0のの場合
    else if (lhs_var_type->ptrs == 1 && rhs_var_type->ptrs == 0) {
        node->rhs = new_binary(ND_MUL, node->rhs, new_num(8));
        return lhs_var_type;
    }
    else if (lhs_var_type->ptrs == 0 && rhs_var_type->ptrs == 1) {
        node->lhs = new_binary(ND_MUL, node->lhs, new_num(8));
        return rhs_var_type;
    }
    
    // ptrsが2と0の場合
    else if (lhs_var_type->ptrs >= 2 && rhs_var_type->ptrs == 0) {
        node->rhs = new_binary(ND_MUL, node->rhs, new_num(8));
        return lhs_var_type;
    }
    else if (lhs_var_type->ptrs == 0 && rhs_var_type->ptrs >= 2) {
        node->lhs = new_binary(ND_MUL, node->lhs, new_num(8));
        return rhs_var_type;
    }

    // ポインタとポインタ(アドレスとアドレス)の演算
    // lhsにrhsを足すイメージでlhs_var_typeを返す
    return lhs_var_type;
}