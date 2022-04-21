#include "grccnr.h"

Func *code[1024];
LVar *locals;
LVar *global_var;
LVar *strs;
Func *now_func;

int control = 0;
int str_num = 0;

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

Node *new_char(char c) {
    Node *node = new_node(ND_CHAR);
    node->val = (int)c;
    return node;
}

// 変数を名前で検索する　見つからないならNULLを返す
LVar *find_lvar(Token *tok) {
    // ローカル変数
    for (LVar *var = locals; var; var = var->next) {
        if (var->len == tok->len && 
            !memcmp(tok->str, var->name, var->len))
            return var;
    }

    // グローバル変数
    for (LVar *var = global_var; var; var = var->next) {
        if (var->len == tok->len && 
            !memcmp(tok->str, var->name, var->len)) {
            return var;
        }
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

// 関数名を(までヒープ領域にコピーしてそれを指す
char *str_copy(Token *tok) {
    char *temp = (char*)malloc(sizeof(char) * (tok->len + 1));
    strncpy(temp, tok->str, tok->len);
    temp[tok->len] = '\0';
    return temp;
}

// 変数の型を返す
VarType *new_var_type() {
    VarType *var_type, *next_var_type, *top_var_type;
    Type type;
    top_var_type = var_type = calloc(1, sizeof(VarType));
    Token *tok = consume_kind(TK_VAR_TYPE);
    if (!tok) return NULL;

    if (tok->kind == TK_VAR_TYPE && strncmp(tok->str, "int", tok->len) == 0)
        type = INT;
    else if (tok->kind == TK_VAR_TYPE && strncmp(tok->str, "char", tok->len) == 0)
        type = CHAR;
    else
        error_at(tok->str, "未定義の型です");
    
    while (consume("*")) {
        next_var_type = calloc(1, sizeof(VarType));
        var_type->ty = PTR;
        var_type->ptr_to = next_var_type;
        var_type = next_var_type;
    }
    var_type->ty = type;
    return top_var_type;
}

// 変数の宣言 失敗なら途中でexitされる
// type: 0->arg 1->locals 2->global
LVar *declare_var(int type) {
    Token *tok_var_name;
    LVar *lvar = calloc(1, sizeof(LVar));
    VarType *array_top, *array_bfr, *array_now;

    // 変数の型
    lvar->type = new_var_type();
    if (!lvar)
        return NULL;
    
    // 変数名
    tok_var_name = consume_kind(TK_IDENT);
    
    if (!tok_var_name) 
        error_at(token->str, "宣言する変数名がありません");
    if (find_lvar(tok_var_name)) 
        error_at(tok_var_name->str, "既に宣言された変数名です");
    
    lvar->name = str_copy(tok_var_name);
    lvar->len = tok_var_name->len;
    
    // 関数ならreturn グローバル変数ならlvar->glb_var=1
    //printf("%d %c\n", type, *token->str);
    if (type == 2) {
        if (token_str("("))
            return lvar;
        else
            lvar->glb_var = 1;
    }

    // []?
    if (token_str("[")) {
        array_top = array_bfr = calloc(1, sizeof(VarType));
        while (consume("[")) {
            array_now = calloc(1, sizeof(VarType));
            array_now->ty = ARRAY;
            array_bfr->ptr_to = array_now;
            if (token_kind(TK_NUM)) {
                array_now->array_size = expect_number();
                if (array_now->array_size <= 0)
                    error_at(token->str, "配列のサイズは正の整数である必要があります");
            } else if (array_bfr != array_top)
                error_at(token->str, "配列の要素数省略は最高次元の場合のみ可です");
            array_bfr = array_now;
            expect("]");
        }
        array_bfr->ptr_to = lvar->type;
        lvar->type = array_top->ptr_to;
    }

    // 初期化ありローカル変数かグローバル変数
    Node *lvar_node = new_node(ND_LVAR);
    lvar_node->lvar = lvar;
    
    if (type > 0 && consume("="))
        lvar->initial = initialize(type, lvar->type, lvar_node);
    
    int offset = get_size(lvar->type);

    // RBPからのオフセット
    // type==2はグローバル変数
    if (type == 2)
        lvar->offset = offset;
    else if (locals)
        lvar->offset = locals->offset + offset;
    else
        lvar->offset = offset;

    lvar_node->lvar = lvar;
    lvar_node->offset = lvar->offset;

    // 変数宣言終了
    if (type == 0 || token_str(";"))
        return lvar;

    error_at(token->str, "文の末尾は ; である必要があります");
}

// ---------- parser ---------- //
// program = stmt*
void program() {
    int i = 0;
    while (!at_eof()) {
        code[i] = glbstmt();
        if (code[i]) i++;
    }    
    code[i] = NULL;
}

Func *glbstmt() {
    Func *func; // 関数の場合に仕様
    LVar *var_or_func, *arg_var;
    var_or_func = declare_var(2);
    // 関数
    if (consume("(")) {
        if (var_or_func->initial)
            error_at(token->str, "変数の初期化後に ( は未定義です");
        func = calloc(1, sizeof(Func));
        now_func = func;

        func->func_type_name = var_or_func;
        // ローカル変数の初期化
        locals = NULL;

        // 引数
        for (int i = 0; ; i++) {
            if (i == 6)
                error_at(token->str, "引数が7つ以上に対応していません");
            
            if (token_kind(TK_VAR_TYPE)) { 
                arg_var = declare_var(0);
                arg_var->next = locals;
                locals = arg_var;
            }
            if (consume(",")) continue;
            else if (consume(")")) break;
            else error_at(token->str, "','か')'である必要があります");
        }
        func->arg = locals; // 引数
        func->stmt = stmt(); // 処理
        func->locals = locals; // ローカル変数
        return func;
    }
    // グローバル変数
    else {
        // 連結リスト構築
        var_or_func->next = global_var;
        global_var = var_or_func;
        expect(";");

        return NULL;
    }
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
    LVar *lvar;

    // 変数宣言
    while (token_kind(TK_VAR_TYPE)) {
        lvar = declare_var(1);
        lvar->next = locals;
        locals = lvar;
        expect(";");
        if (lvar->initial) 
            return lvar->initial;
        else 
            return NULL;
    }

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
    Node *node = assign();
    AST_type(1, node);
	return node;
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
	else if (consume("-"))
		return new_binary(ND_SUB, new_num(0), unary());
    else if (consume("*"))
        return new_binary(ND_DEREF, unary(), NULL);
    else if (consume("&"))
        return new_binary(ND_ADDR, unary(), NULL);
    else if (consume_kind(TK_SIZEOF)) {
        Node *node = unary();
        if (node->kind == ND_STR_PTR)
            node->kind = ND_STR;
        return new_num(get_size(AST_type(1, node)));
    }
    return primary();
}

Node *primary() {
	// 次のトークンが"("なら、"(" expr ")"
	if (consume("(")) {
		Node *node = assign();
		expect(")");
		return node;
	}
    
	// それ以外なら数値(関数)か変数
    Token *tok = consume_kind(TK_IDENT);
    //printf("%s\n", token->str);
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

                    node->arg[i] = assign();
                    AST_type(1, node->arg[i]);

                    if (consume(",")) {} // 次の引数がある
                    else if (consume(")")) break; // 引数終了
                    else error_at(token->str, "引数が正しくありません");
                }
            }
        } else { // 変数の場合
            node->kind = ND_LVAR;
            LVar *lvar = find_lvar(tok);
            if (!lvar)
                error_at(tok->str, "宣言されていない変数です");
            
            // 既存の変数
            node->lvar = lvar;
            if (!lvar->glb_var)
                node->offset = lvar->offset;

            // 配列の場合
            if (node->lvar->type->ty == ARRAY ||
                node->lvar->type->ty == PTR) {
                // 添え字の場合
                VarType *type = lvar->type;
                while (consume("[")) {
                    node = new_binary(ND_DEREF, new_binary(ND_ADD, node, assign()), NULL);
                    if ((type->ty == ARRAY || type->ty == PTR) && 
                        (type->ptr_to->ty == ARRAY || type->ptr_to->ty == PTR)) 
                        node->array_accessing = 1;
                    
                    if (type->ptr_to)
                        type = type->ptr_to;
                    else
                        error_at(token->str, "型違反です");
                    expect("]");
                }
            }    
        }
        return node;
    }
    
    // 文字リテラル
    tok = consume_kind(TK_STR);
    if (tok) {
        // strsに追加してND_STRに設定
        LVar *str = calloc(1, sizeof(LVar));
        Node *node = new_node(ND_STR_PTR);
        node->lvar = str;
        str->offset = str_num++;
        str->str = str_copy(tok);
        str->len = tok->len;
        str->next = strs;
        strs = str;
        return node;
    }

    // 変数ではないなら数値
	return new_num(expect_number());
}