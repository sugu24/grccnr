#include "grccnr.h"

Node *code[100];
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

// stmtがifの場合にBNFに沿ってNodeを生成して、Nodeを返す
Node *create_if_node() {
    Node *node = new_node(ND_IF);
    node->control = control++;
    if (!consume("("))
        error_at(token->str, "'('ではないトークンです");
    node->lhs = expr(); // 条件式
    if (!consume(")"))
        error_at(token->str, "')'ではないトークンです");
    node->stmt1 = stmt();
    if (consume_kind(TK_ELSE)) {
        node->is_else = new_node(ND_ELSE);
        node->stmt2 = stmt();
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
    node->stmt1 = stmt();
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
    node->stmt1 = stmt();
    return node;
}

// // stmtがreturnの場合にBNFに沿ってNodeを生成して、Nodeを返す
Node *create_return_node() {
    Node *node = new_node(ND_RETURN);
    node->lhs = expr();
    return node;
}

// ---------- parser ---------- //
// program = stmt*
void program() {
    int i = 0;
    while (!at_eof()) {
        code[i++] = stmt();
    }    
    code[i] = NULL;
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

    if (consume_kind(TK_IF)) {
        node = create_if_node();
        return node;
    } else if (consume_kind(TK_WHILE)) {
        node = create_while_node();
        return node;
    } else if (consume_kind(TK_FOR)) { 
        node = create_for_node();
        return node;
    } 
    
    if (consume_kind(TK_RETURN)) {
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
	return primary();
}

Node *primary() {
	// 次のトークンが"("なら、"(" expr ")"
	if (consume("(")) {
		Node *node = expr();
		expect(")");
		return node;
	}
    
	// それ以外なら数値か変数
    Token *tok = consume_kind(TK_IDENT);
    if (tok) {
        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_LVAR;
        
        LVar *lvar = find_lvar(tok);
        if (lvar) {
            node->offset = lvar->offset;
        } else {
            lvar = calloc(1, sizeof(LVar));
            lvar->next = locals;
            lvar->name = tok->str;
            lvar->len = tok->len;
            if (locals)
                lvar->offset = locals->offset + 8;
            else 
                lvar->offset = 8;
            node->offset = lvar->offset;
            locals = lvar;
        }
        return node;
    }

    // 変数ではないなら数値
	return new_num(expect_number());
}