#include "grccnr.h"

// ローカル変数の文字列初期化
Node *init_local_string(Token *tok, VarType *var_type, Node *lvar_node) {
    Node *head_node = calloc(1, sizeof(Node));
    Node *cur_node = head_node;
    Node *next_node, *next_cand_stmt;
    cur_node->next_stmt = NULL;
    int i;
    
    lvar_node->access = 1;
    AST_type(1, lvar_node);

    for (i = 0; tok->str[i]; i++) {
        //next_cand_stmt = calloc(1, sizeof(Node));
        next_cand_stmt = new_binary(ND_ASSIGN,
            new_binary(ND_INDEX, 
                new_binary(ND_ADD, lvar_node, new_binary(ND_MUL, new_num(i), new_num(1))), 
                NULL),
            new_char(tok->str[i]));
        
        next_node = calloc(1, sizeof(Node));
        next_node->stmt = next_cand_stmt;
        cur_node->next_stmt = next_node;
        cur_node = cur_node->next_stmt;
        cur_node->next_stmt = NULL;
    }
    
    if (!var_type->array_size && var_type->ptr_to && var_type->ptr_to->ty == CHAR)
        var_type->array_size = i+1;
    
    return head_node->next_stmt;
}

Node *init_global_string(Token *tok, VarType *var_type, Node *lvar_node) {
    Node *node_str = new_node(ND_STR);
    LVar *lstr = calloc(1, sizeof(LVar));
    lstr->str = tok->str;
    lstr->len = tok->len;
    node_str->lvar = lstr;

    if (!var_type->array_size && var_type->ptr_to && var_type->ptr_to->ty == CHAR)
        var_type->array_size = tok->len + 1;
    
    Node *node = new_binary(ND_ASSIGN, lvar_node, node_str);
    AST_type(1, node);
    return node;
}

Node *init_value(Node *lvar_node) {
    Node *node = new_binary(ND_ASSIGN, lvar_node, assign());
    AST_type(1, node);
    return node;
}

Node *init_data(int type, VarType *var_type, Node *lvar_node) {
    Node *cur_node;
    if (var_type->ty == ARRAY && get_type(var_type)->ty == CHAR) {
        // stringを直接格納
        Token *tok = consume_kind(TK_STR);
        if (!tok) error_at(token->str, "文字列である必要があります");
        tok->str = str_copy(tok);

        if (type == 2) // グローバル変数
            return init_global_string(tok, var_type, lvar_node);
        else // ローカル変数
            return init_local_string(tok, var_type, lvar_node);
    } else {
        return init_value(lvar_node);
    }
}

// Node *array_initialize(VarType *type)
// { : array_initialize(type->ptr_to, 0)
// } : return node
// 数字 : node->next_stmt = initialize
Node *array_initialize(int type, VarType *var_type, Node *lvar_node) {
    Node *head_node = new_node(ND_BLOCK);
    Node *cur_node = head_node;
    Node *next_node, *next_cand_stmt;
    cur_node->next_stmt = NULL;
    int comma_stack[100] = { 0 };
    int i = 0;
    while (i > 0 || !token_str("}")) {
        if (consume("}"))
            comma_stack[i--] = 0;
        else if (consume("{"))
            i++;
        else if (consume(","))
            comma_stack[i]++;
        else {
            next_cand_stmt = lvar_node;
            next_cand_stmt->access = 1;
            for (int j = 0; j < i; j++) {
                next_cand_stmt = new_binary(ND_INDEX, 
                    new_binary(ND_ADD, next_cand_stmt, new_num(comma_stack[j])),
                    NULL);
                next_cand_stmt->access = 1;
            }
            
            next_cand_stmt = init_data(type, var_type, 
                new_binary(ND_INDEX, new_binary(ND_ADD, next_cand_stmt, new_num(comma_stack[i])), NULL));

            next_node = calloc(1, sizeof(Node));
            if (next_cand_stmt->stmt)
                next_node = next_cand_stmt;
            else
                next_node->stmt = next_cand_stmt;
            cur_node->next_stmt = next_node;

            // next_stmtをたどってnext_stmt==NULLのNodeをcur_nodeに設定する
            while (next_node->next_stmt) 
                next_node = next_node->next_stmt;

            cur_node = next_node;
        }
    }

    if (!var_type->array_size)
        var_type->array_size = comma_stack[0] + 1;

    if (get_offset(cur_node->stmt) + get_size(get_type(var_type)) > get_size(var_type))
        error_at(token->str, "配列のサイズを超えた範囲に初期化されています");
    return head_node;
}

//  宣言したlvarのlvar->initialに初期値を設定して返す
// lvar->initial = new_binary(ND_ADDIGN, lnode, 初期値)
// lnodeにrbpからのoffsetは未設定
// lnodes:代入先
Node *initialize(int type, VarType *var_type, Node *lvar_node) {
    Node *node;
    if (consume("{")) {
        // 配列の初期化
        node = array_initialize(type, var_type, lvar_node);
        expect("}");
    } else {
        node = new_node(ND_BLOCK);
        Node *chain_node;
        node->next_stmt = calloc(1, sizeof(Node));
        
        chain_node = init_data(type, var_type, lvar_node);
        
        if (chain_node->stmt) {// init_local_string
            node->next_stmt = chain_node;
        } else {
            node->next_stmt->stmt = chain_node;
        }
    }
    return node;
}