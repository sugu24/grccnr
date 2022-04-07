#include "grccnr.h"

Func *code[1024];
LVar *locals;

int control = 0;

Node *new_node(NodeKind kind) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = kind;
	return node;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
	Node *node = new_node(kind);
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *new_num(int val) {
	Node *node = new_node(ND_NUM);
	node->val = val;
	return node;
}

// 変数を名前で検索する　見つからないならNULLを返す
LVar *find_lvar(Token *tok) {
    for (LVar *var = locals; var; var = var->next) {
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
            return var;
    }
    return NULL;
}

// ND_BLOCKを返す　先頭のstmtはNULL
Node *create_block_node() {
    Node *head_node = new_node(ND_BLOCK);
    Node *cur_node = head_node;
    Node *next_node;
    Node *next_cand_stmt;
    cur_node->next_stmt = NULL;
    while (!consume("}")) {
        next_cand_stmt = stmt();
        if (!next_cand_stmt) continue; // 変数宣言
        next_node = calloc(1, sizeof(Node));
        next_node->stmt = next_cand_stmt;
        cur_node->next_stmt = next_node;
        cur_node = cur_node->next_stmt;
        cur_node->next_stmt = NULL;
    }
    return head_node;
}

// ---------- if or else if or else ---------- //
// 初めのif節 BNFに沿ってelse if or else 入り(連結リスト)のNodeを返す 
Node *create_if_node(int con, int chain) {
    Node *node;
    if (chain) {
        node = new_node(ND_ELSE_IF);
        node->control = con;
        node->offset = chain;
    } else {
        node = new_node(ND_IF);
        con = node->control = control++;
    }

    if (!consume("("))
        error_at(token->str, "'('ではないトークンです");
    node->lhs = expr(); // 条件式
    if (!consume(")"))
        error_at(token->str, "')'ではないトークンです");
    node->stmt = stmt();

    node->next_if_else = NULL; // 明示的にNULLを代入
    if (consume_kind(TK_ELSE))
        node->next_if_else = create_else_node(con, chain+1);
    return node;
}

// TokenがelseのときにBNFに沿ってelse if or else のNodeを返す
Node *create_else_node(int con, int chain) {
    Node *node;
    if (consume_kind(TK_IF)) 
        node = create_if_node(con, chain);
    else {
        node = new_node(ND_ELSE);
        node->stmt = stmt();
    }
    return node;
}

// stmtがwhileの場合にBNFに沿ってNodeを生成して、Nodeを返す
Node *create_while_node() {
    Node *node = new_node(ND_WHILE);
    node->control = control++;
    if (!consume("("))
        error_at(token->str, "'('でないトークンです");
    node->lhs = expr(); // 条件式
    if (!consume(")"))
        error_at(token->str, "')'でないトークンです");
    node->stmt = stmt();
    return node;
}

// stmtがforの場合にBNFに沿ってNodeを生成して、Nodeを返す
Node *create_for_node() {
    Node *node = new_node(ND_FOR);
    node->control = control++;
    if (!consume("("))
        error_at(token->str, "'('でないトークンです");
        
    if (!consume(";")) { // for (now;;)
        node->lhs = expr();
        if (!consume(";"))
            error_at(token->str, "';'でないトークンです");
    }

    if (!consume(";")) { // for (;now;)
        node->mhs = expr();
        if (!consume(";"))
            error_at(token->str, "';'でないトークンです");
    }

    if (!consume(")")) { // for (;;now)
        node->rhs = expr();
        if (!consume(")"))
            error_at(token->str, "')'でないトークンです");
    }
    node->stmt = stmt();
    return node;
}

// // stmtがreturnの場合にBNFに沿ってNodeを生成して、Nodeを返す
Node *create_return_node() {
    Node *node = new_node(ND_RETURN);
    node->lhs = expr();
    return node;
}

// 変数の型を返す
VarType *new_var_type(Token *tok) {
    VarType *var_type = calloc(1, sizeof(VarType));
    int ptrs = 0;
    if (tok->len == 3 && strncmp(tok->str, "int", tok->len) == 0)
        var_type->ty = INT;
    else
        error_at(tok->str, "未定義の型です");
    
    while (consume("*")) ptrs++;
    var_type->ptrs = ptrs;
    return var_type;
}

// 変数の宣言 失敗なら途中でexitされる
Token *declare_var(Token *tok_var_type) {
    Token *tok_var_name;
    LVar *lvar = calloc(1, sizeof(LVar));
    lvar->next = locals;
    lvar->type = new_var_type(tok_var_type);
    tok_var_name = consume_kind(TK_IDENT);
    
    if (!tok_var_name) 
        error_at(token->str, "宣言する変数名がありません");
    if (find_lvar(tok_var_name)) 
        error_at(tok_var_name->str, "既に宣言された変数名です");
    
    lvar->name = tok_var_name->str;
    lvar->len = tok_var_name->len;
    if (locals)
        lvar->offset = locals->offset + 8;
    else
        lvar->offset = 8;
    locals = lvar;
    return tok_var_name;
}

// 関数名を(までヒープ領域にコピーしてそれを指す
char *str_copy(Token *tok) {
    char *temp = (char*)malloc(sizeof(char) * (tok->len + 1));
    strncpy(temp, tok->str, tok->len);
    temp[tok->len] = '\0';
    return temp;
}

// ---------- parser ---------- //
// program = stmt*
void program() {
    int i = 0;
    while (!at_eof()) {
        code[i++] = glbstmt();
    }    
    code[i] = NULL;
}

Func *glbstmt() {
    Func *func;
    Token *tok_var_type = consume_kind(TK_VAR_TYPE);
    Token *tok_var_name;
    if (tok_var_type) { // 関数の場合
        func = calloc(1, sizeof(func));
        func->type = new_var_type(tok_var_type);
        // 関数名
        tok_var_name = consume_kind(TK_IDENT);
        if (!tok_var_name)
            error_at(token->str, "関数名である必要があります");
        func->func_name = str_copy(tok_var_name);
        // ローカル変数の初期化
        locals = NULL;
        expect("(");
        // 引数
        for (int i = 0; ; i++) {
            if (i == 6)
                error_at(token->str, "引数が7つ以上に対応していません");
            
            tok_var_type = consume_kind(TK_VAR_TYPE);
            if (tok_var_type) declare_var(tok_var_type); 
            if (consume(",")) continue;
            else if (consume(")")) break;
            else error_at(token->str, "','か')'である必要があります");
        }
        func->arg = locals;
        func->stmt = stmt();
        func->locals = locals;
    } else 
        error_at(token->str, "関数の型がありません"); 
    
    return func;
}

/* 
stmt = expr ";"
     | "if" "(" expr ")" stmt ("else" stmt)?
     | "while" "(" expr ")" stmt
     | "for" "(" expr? ";" expr? ";" expr? ")" stmt
     | "return" expr ";"
*/
Node *stmt() {
    Node *node;
    
    if (consume("{")) { // block文
        node = create_block_node();
        return node;
    } else if (consume_kind(TK_IF)) {
        node = create_if_node(-1,0);
        return node;
    } else if (consume_kind(TK_WHILE)) {
        node = create_while_node();
        return node;
    } else if (consume_kind(TK_FOR)) { 
        node = create_for_node();
        return node;
    } else if (consume_kind(TK_RETURN)) {
        node = create_return_node();
    } else {
        node = expr();
    }
    
    if (!consume(";"))
        error_at(token->str, "';'ではないトークンです");
    return node;
}

// expr = assign
Node *expr() {
    Token *tok_var_type = consume_kind(TK_VAR_TYPE);
    if (tok_var_type) // 変数宣言
        token = declare_var(tok_var_type); // int a = 1;のように宣言後に代入を考慮
    
	return assign();
}

// assign = equality ("=" assign)?
Node *assign() {
    Node *node = equality();
    if (consume("="))
        node = new_binary(ND_ASSIGN, node, assign());
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality() {
	Node *node = relational();

	for (;;) {
		if (consume("=="))
			node = new_binary(ND_EQ, node, relational());
		else if (consume("!="))
			node = new_binary(ND_NE, node, relational());
		else
			return node;
	}
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
	Node *node = add();

	for (;;) {
		if (consume("<"))
			node = new_binary(ND_LT, node, add());
		else if (consume("<="))
			node = new_binary(ND_LE, node, add());
		else if (consume(">"))
			node = new_binary(ND_LT, add(), node);
		else if (consume(">="))
			node = new_binary(ND_LE, add(), node);
		else
			return node;
	}
}

// add = mul("+" mul | "-" mul)*
Node *add() {
	Node *node = mul();

	for (;;) {
		if (consume("+"))
			node = new_binary(ND_ADD, node, mul());
		else if (consume("-"))
			node = new_binary(ND_SUB, node, mul());
		else
			return node;
	}
}

Node *mul() {
	Node *node = unary();

	for (;;) {
		if (consume("*"))
			node = new_binary(ND_MUL, node, unary());
		else if (consume("/"))
			node = new_binary(ND_DIV, node, unary());
		else
			return node;
	}
}

Node *unary() {
	if (consume("+"))
		return unary();
	if (consume("-"))
		return new_binary(ND_SUB, new_num(0), unary());
    if (consume("*")) { // *を数え、変数の型に違反いていないか確認
        int ptrs = 1;
        Node* node;
        Token *tok;
        while (consume("*")) ptrs++;
        token = tok = consume_kind(TK_IDENT);
        LVar *lvar = find_lvar(tok);
        if (ptrs > lvar->type->ptrs) // *の数が変数の型に違反していないか
            error_at(tok->str, "変数の型と異なります");
        
        node = unary();
        node->ptrs = ptrs;
        node->offset = lvar->offset;
        return new_binary(ND_DEREF, node, NULL);
    } 
    if (consume("&"))
        return new_binary(ND_ADDR, unary(), NULL);
	return primary();
}

Node *primary() {
	// 次のトークンが"("なら、"(" expr ")"
	if (consume("(")) {
		Node *node = expr();
		expect(")");
		return node;
	}
    
	// それ以外なら数値(関数)か変数
    Token *tok = consume_kind(TK_IDENT);
    if (tok) {
        Node *node = calloc(1, sizeof(Node));
        if (consume("(")) { // 関数の場合
            node->kind = ND_CALL_FUNC;
            node->func_name = str_copy(tok);
            if (!consume(")")){
                int i;
                for (i = 0; ; i++) {
                    if (i == 6)
                        error_at(token->str, "引数の個数は7個以上に対応していません");
                    node->arg[i] = expr();

                    if (consume(",")) {} // 次の引数がある
                    else if (consume(")")) break; // 引数終了
                    else error_at(token->str, "引数が正しくありません");
                }
            }
        } else { // 変数の場合
            node->kind = ND_LVAR;
            LVar *lvar = find_lvar(tok);
            if (lvar) {// 既存の変数
                node->offset = lvar->offset;
            }else // 未定義の変数
                error_at(tok->str, "宣言されていない変数です");
        }
        return node;
    }

    // 変数ではないなら数値
	return new_num(expect_number());
}