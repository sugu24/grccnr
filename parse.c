#include "grccnr.h"

Func *code[1024];
LVar *locals;
LVar *global_var;
LVar *strs;
Func *now_func;
Typedef *typedefs;
Struct *structs;
Enum *enums;
Prototype *prototype = NULL;

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

Typedef *new_typedef(VarType *type, char *name, int len) {
    Typedef *type_def = calloc(1, sizeof(Typedef));
    type_def->type = type;
    type_def->name = name;
    type_def->len = len;
    return type_def;
}

// 変数を名前で検索する　見つからないならNULLを返す
LVar *find_lvar(Token *tok) {
    // ローカル変数
    for (LVar *var = locals; var; var = var->next) {
        if (tok->len == var->len && 
            !memcmp(tok->str, var->name, var->len))
            return var;
    }

    // グローバル変数
    for (LVar *var = global_var; var; var = var->next) {
        if (tok->len == var->len && 
            !memcmp(tok->str, var->name, var->len)) {
            return var;
        }
    }

    return NULL;
}

// enumのメンバ変数を返す
Node *find_enum_membar(Token *tok) {
    for (Enum *enum_p = enums; enum_p; enum_p = enum_p->next) {
        for (EnumMem *membar = enum_p->membar; membar; membar = membar->next) {
            if (tok->len == membar->len &&
                !memcmp(tok->str, membar->name, membar->len))
                return membar->num_node;
        }
    }

    return NULL;
}

// typedef宣言された型名と一致する物があれば返す
VarType *get_typedefed_type() {
    Token *tok = consume_kind(TK_IDENT);
    if (!tok)
        return NULL;
    
    for (Typedef *type_def = typedefs; type_def; type_def = type_def->next) {
        if (tok->len == type_def->len &&
            !memcmp(tok->str, type_def->name, tok->len))
            return type_def->type;
    }

    // tokの意味はtypedefでないから戻してNULLを返す
    token = tok;
    return NULL;
}

// タグ名が定義されたstructならばStructを返す
Struct *get_struct(Token *tok) {
    for (Struct *struct_p = structs; structs; struct_p = struct_p->next) {
        if (tok->len == struct_p->len &&
            !memcmp(tok->str, struct_p->name, tok->len)) {
            return struct_p;
        }
    }
    return NULL;
}

// 引数でもらったStructからtokと同じメンバ変数のlvarを返す
LVar *get_membar(Struct *struct_p, Token *tok) {
    for (LVar *membar = struct_p->membar; membar; membar = membar->next) {
        if (tok->len == membar->len &&
            !memcmp(tok->str, membar->name, tok->len))
            return membar;
    }
    return NULL;
}

// タグ名が定義されたenumならばEnumを返す
Enum *get_enum(Token *tok) {
    for (Enum *enum_p = enums; enum_p; enum_p = enum_p->next) {
        if (tok->len == enum_p->len &&
            !memcmp(tok->str, enum_p->name, tok->len)) {
            return enum_p;
        }
    }
    return NULL;
}

// 関数宣言でprototypeに追加
void add_prototype(Func *func) {
    Prototype *proto = calloc(1, sizeof(Prototype));
    proto->func = func;
    proto->next = prototype;
    prototype = proto;
}

// プロトタイプと同じ関数名なのに引数が異なる場合1を返す
void prototype_arg(Func *func, Func *proto_func) {
    LVar *proto_arg, *func_arg;
    proto_arg = proto_func->arg;
    func_arg = func->arg;
    while (1) {
        // argの比較
        if (!proto_arg && !func_arg) break;
        else if (!(proto_arg && func_arg)) 
            error("関数%sの関数宣言との引数の個数が異なります", func->func_type_name->name);

        if (!same_type(proto_arg->type, func_arg->type))
            error("関数 %s の引数 %s の型が関数宣言時の変数 %s の型と異なります", func->func_type_name->name, func_arg->name, func_arg->name);
        
        proto_arg = proto_arg->next;
        func_arg = func_arg->next;
    }
}

// 同じPrototypeがあれば1を返す その他は0を返す
Func *same_prototype(Func *func) {
    // prototypeに宣言済みの
    for (Prototype *proto = prototype; proto; proto = proto->next) {
        if (proto->func->func_type_name->len == func->func_type_name->len &&
            strncmp(proto->func->func_type_name->name, func->func_type_name->name, func->func_type_name->len) == 0) {
            if (same_type(proto->func->func_type_name->type, func->func_type_name->type))
                return proto->func;
            else
                error("関数 %s の戻り値の型が関数宣言時の戻り値の型と異なります", func->func_type_name->name);
        }
    }

    return 0;
}

// 同じ関数があれば1を返す その他は0を返す
Func *same_function(Func *func) {
    // 定義済みの関数
    for (int i = 0; code[i]; i++) {
        if (code[i]->func_type_name->len == func->func_type_name->len &&
            strcmp(code[i]->func_type_name->name, func->func_type_name->name) == 0) {
            if (same_type(code[i]->func_type_name->type, func->func_type_name->type))
                return code[i];
            else
                error_at(token->str, "戻り値の型が異なります");
        }
    }

    Func *proto_func;
    if (proto_func = same_prototype(func)) {
        prototype_arg(func, proto_func);
    }
    return 0;
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
        node->control = con;
        node->offset = chain;
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

// ----------------------- 変数宣言系 ----------------------- //

// structのtypeを生成する
Struct *new_struct() {
    Struct *struct_p;
    LVar *membar, *last_membar;
    Token *tok;

    // タグ名があるか
    tok = consume_kind(TK_IDENT);
    if (tok && (struct_p = get_struct(tok)))
        // 既に定義されたタグ名ならそれを返す
        return struct_p;
    
    // 新しいstructの生成
    struct_p = calloc(1, sizeof(Struct));
    if (tok) {
        struct_p->name = tok->str;
        struct_p->len = tok->len;
    }

    // structのメンバ変数ブロック開始
    if (!consume("{"))
        return struct_p;

    // declare_varの引数が0なのは
    // 初期化、typedef、structなしだから
    last_membar = NULL;
    while (membar = declare_var(0)) {
        membar->next = last_membar;
        last_membar = membar;
        expect(";");
    }
    struct_p->membar = last_membar;

    // structのメンバ変数ブロック終了
    expect("}");

    // 構造体定義されていたら登録
    if (tok) {
        // タグ名があるstructだから連結リストに追加
        struct_p->next = structs;
        structs = struct_p;

        // typedefで同じタグ名のものがあれたVarTypeを入れ替える
        for (Typedef *typedef_ = typedefs; typedef_; typedef_ = typedef_->next)
            if (typedef_->type->ty == STRUCT && 
                typedef_->type->struct_p->len == struct_p->len &&
                !strncmp(typedef_->type->struct_p->name, struct_p->name, struct_p->len))
                typedef_->type->struct_p = struct_p;
    }

    return struct_p;
}

Enum *new_enum() {
    Enum *enum_p;
    Token *tok;
    EnumMem *membar, *last_membar;
    int val; // メンバの数字

    // タグ名があるか
    tok = consume_kind(TK_IDENT);
    if (tok && (enum_p = get_enum(tok)))
        // 既に定義されたタグ名ならそれを返す
        return enum_p;
    
    // 新しいenumの生成
    enum_p = calloc(1, sizeof(Enum));
    if (tok) {
        enum_p->name = tok->str;
        enum_p->len = tok->len;
    }

    // enumのメンバ変数ブロック開始
    // または typedef enum name nameの形ならreturn 1
    if (!consume("{"))
        return enum_p;

    // enumのカッコ内の値をenumに追加
    last_membar = NULL;
    val = 0;
    while (tok = consume_kind(TK_IDENT)) {
        membar = calloc(1, sizeof(EnumMem));
        membar->name = tok->str;
        membar->len = tok->len;
        if (consume("=")) {
            if (tok = consume_kind(TK_NUM))
                val = tok->val;
            else
                error_at(token->str, "数字である必要があります");
        }
        membar->num_node = calloc(1, sizeof(Node));
        membar->num_node->kind = ND_NUM;
        membar->num_node->val = val;
        membar->next = last_membar;
        last_membar = membar;
        expect(",");
        val++;
    }
    enum_p->membar = last_membar;

    // structのメンバ変数ブロック終了
    expect("}");

    // enum登録
    enum_p->next = enums;
    enums = enum_p;
    
    // typedefで同じタグ名のものがあれたVarTypeを入れ替える
    for (Typedef *typedef_ = typedefs; typedef_; typedef_ = typedef_->next)
        if (typedef_->type->ty == INT && 
            typedef_->type->enum_p->len == enum_p->len &&
            !strncmp(typedef_->type->enum_p->name, enum_p->name, enum_p->len))
            typedef_->type->enum_p = enum_p;
    
    return enum_p;
}

// 関数名を(までヒープ領域にコピーしてそれを指す
char *str_copy(Token *tok) {
    char *temp = (char*)malloc(sizeof(char) * (tok->len + 1));
    strncpy(temp, tok->str, tok->len);
    temp[tok->len] = '\0';
    return temp;
}

// include 
void include() {
    Token *tok;
    char *temp, *filename;
    if (tok = consume_kind(TK_STR)) {
        filename = str_copy(tok);
        temp = user_input;
        tok = token;
	    user_input = read_file(filename);
        token = tokenize();
	    program();
        user_input = temp;
        token = tok;
    }
    else {
        error_at(token->str, "ファイル名である必要があります");
    }
}

// 原始的な型かどうか
Type get_var_type() {
    if (consume_kind(TK_INT))
        return INT;
    else if (consume_kind(TK_CHAR))
        return CHAR;
    else 
        return 0;
}

// 変数の型を返す
VarType *new_var_type(int type) {
    VarType *var_type, *temp_type, *top_var_type;
    Type ty;
    int typedef_type = 0;
    top_var_type = var_type = calloc(1, sizeof(VarType));
    
    // 型取得
    while (true) {
        if (consume_kind(TK_TYPEDEF)) {
            typedef_type = 1;
        } else if (ty = get_var_type()) {
            if (var_type->ty)
                error_at(token->str, "型は一意に定めなければいけません");
            var_type->ty = ty;
        } else if (temp_type = get_typedefed_type()) {
            if (var_type->ty)
                error_at(token->str, "型は一意に定めなければいけません");
            if (temp_type->ty == STRUCT)
                var_type->struct_p = temp_type->struct_p;
            else if (temp_type->enum_p)
                var_type->enum_p = temp_type->enum_p;
            var_type->ty = temp_type->ty;
        } else if (consume_kind(TK_STRUCT)) {
            if (var_type->ty)
                error_at(token->str, "型は一意に定めなければいけません");
            var_type->ty = STRUCT;
            var_type->struct_p = new_struct();
        } else if (consume_kind(TK_ENUM)) {
            if (var_type->ty)
                error_at(token->str, "型は一意に定めなければいけません"); 
            var_type->ty = INT;
            var_type->enum_p = new_enum();
        } else
            break;
    }

    // type=0 (arg) ならtypedef_typeはないはず
    if (typedef_type && type == 0)
        error_at(token->str, "引数ではtypedef宣言出来ません");

    // 変数宣言なし
    if (!var_type->ty)
        return NULL;

    // ポインタ取得
    while (consume("*")) {
        top_var_type = calloc(1, sizeof(VarType));
        top_var_type->ty = PTR;
        top_var_type->ptr_to = var_type;
        var_type = top_var_type;
    }
    
    // typedef宣言ならtypdefefsにlvarとtypedef名を加える
    if (typedef_type) {
        Token *name;
        if (!(name = consume_kind(TK_IDENT)))
            error_at(token->str, "typedef宣言での宣言後の名前を指定してください");

        Typedef *type_def = new_typedef(top_var_type, name->str, name->len);
        type_def->next = typedefs;
        typedefs = type_def;
        expect(";");
        return NULL;
    } 
    else {
        return top_var_type;
    }
}

// 変数の宣言 失敗なら途中でexitされる
// type: 0->arg 1->locals 2->global
LVar *declare_var(int type) {
    Token *tok_var_name;
    LVar *lvar = calloc(1, sizeof(LVar));
    VarType *array_top, *array_bfr, *array_now;

    // 変数の型
    lvar->type = new_var_type(type);
    if (!lvar->type)
        return NULL;

    // 変数名
    tok_var_name = consume_kind(TK_IDENT);

    if (!tok_var_name) {
        if (lvar->type->ty == STRUCT || lvar->type->enum_p) {
            // structかつvar_nameがない
            // structの定義だけだった
            expect(";");
            return NULL;
        }
        error_at(token->str, "宣言する変数名がありません");
    }
    if (find_lvar(tok_var_name)) 
        error_at(tok_var_name->str, "既に宣言された変数名です");
    
    lvar->name = str_copy(tok_var_name);
    lvar->len = tok_var_name->len;

    // 関数ならreturn グローバル変数ならlvar->glb_var=1
    // printf("%d %c\n", type, *token->str);
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
    if (type == 0 || consume(";"))
        return lvar;

    error_at(token->str, "文の末尾は ; である必要があります");
}

// ++ -- の処理
Node *add_add_minus_minus(Node *node) {
    if (consume("++")) {
        node->access = 1;
        node = new_binary(ND_LVAR_ADD, node, NULL);
    } else if (consume("--")) {
        node->access = 1;
        node = new_binary(ND_LVAR_SUB, node, NULL);
    } 
    return node;
}

// ---------- parser ---------- //
// program = stmt*
void program() {
    int i = 0;
    Token *bfr;
    while (!at_eof()) {
        bfr = token;
        code[i] = glbstmt();
        if (code[i]) i++;
        else if (bfr == token)
            error_at(token->str, "%sが処理できません", str_copy(token));
    }
    code[i] = NULL;
}

Func *glbstmt() {
    Func *func; // 関数の場合に仕様
    LVar *var_or_func, *arg_var;

    if (consume("#")) {
        if (consume_kind(TK_INCLUDE))
            include();
        else
            error_at(token->str, "処理できません");
        return NULL;
    }

    // declare_varからNULLが返ってくる条件は
    // typedef宣言
    var_or_func = declare_var(2);

    // typedef宣言の場合Funcは返さない
    if (!var_or_func)
        return NULL;
    
    // 関数
    else if (consume("(")) {
        if (var_or_func->initial)
            error_at(token->str, "変数の初期化後に ( は未定義です");

        func = calloc(1, sizeof(Func));
        func->func_type_name = var_or_func;

        // ローカル変数の初期化
        locals = NULL;

        // 引数
        for (int i = 0; ; i++) {
            if (i == 6)
                error_at(token->str, "引数が7つ以上に対応していません");
            
            if (arg_var = declare_var(0)) {
                if (arg_var->type->ty == STRUCT) error_at(token->str, "関数の引数に構造体はまだ処理できません");
                
                VarType *temp_type = arg_var->type;
                while (temp_type->ty == ARRAY) {
                    // 配列の場合はポインタに変換(配列のアドレスをもらうため) 
                    temp_type->ty = PTR;
                    temp_type->array_size = 0;
                    temp_type = temp_type->ptr_to;
                }

                arg_var->next = locals;
                locals = arg_var;
            }
            if (consume(",")) continue;
            else if (consume(")")) break;
            else error_at(token->str, "','か')'である必要があります");
        }
        func->arg = locals; // 引数    
        now_func = func;
        func->stmt = stmt(); // 処理
        func->locals = locals; // ローカル変数

        if (!func->stmt) {
            // 関数宣言
            if (same_prototype(func))// 同じ名前の関数があればエラー
                error_at(token->str, "関数名が重複しています");
            add_prototype(func);
            return NULL;
        } else {
            // 関数定義
            if (same_function(func))// 同じ名前の関数があればエラー
                error_at(token->str, "関数名が重複しています");
            return func;
        }       
    }
    // グローバル変数
    else {
        // 連結リスト構築
        var_or_func->next = global_var;
        global_var = var_or_func;
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
    Node *node = NULL;
    LVar *lvar;

    // 変数宣言ならdeclare_varから返ってくる
    Typedef *typed = typedefs;
    if (lvar = declare_var(1)) {
        lvar->next = locals;
        locals = lvar;
        if (lvar->initial)
            return lvar->initial;
        else 
            return NULL;
    } else if (typed != typedefs)
        return NULL;

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
    } else if (!token_str(";")) {
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
    else if (consume("+="))
        node = new_binary(ND_ASSIGN, node, new_binary(ND_ADD, node, assign()));
    else if (consume("-="))
        node = new_binary(ND_ASSIGN, node, new_binary(ND_SUB, node, assign()));
    else if (consume("*="))
        node = new_binary(ND_ASSIGN, node, new_binary(ND_MUL, node, assign()));
    else if (consume("/="))
        node = new_binary(ND_ASSIGN, node, new_binary(ND_DIV, node, assign()));
    else if (consume("%="))
        node = new_binary(ND_ASSIGN, node, new_binary(ND_MOD, node, assign()));
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
		if (consume("+")) {
            node = new_binary(ND_ADD, node, mul());
        } else if (consume("-"))
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
        else if (consume("%"))
            node = new_binary(ND_MOD, node, unary());
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
        VarType *var_type;
        Token *tok = token;

        // sizeof(int or struct ...) or sizeof(変数)に対応
        if (consume("(")) {
            if (var_type = new_var_type(0)) {
                expect(")");
                return new_num(get_size(var_type));
            }
        }
        token = tok;
        Node *node = unary();
        if (node->kind == ND_STR_PTR)
            node->kind = ND_STR;
        return new_num(get_size(AST_type(1, node)));
    }
    return primary();
}

Node *primary() {
    LVar *lvar;
	Token *tok;
    Node *node, *enum_mem_node;
    VarType *var_type;

    // 次のトークンが"("なら、"(" expr ")"
    while (consume("(")) {
        node = assign();
        expect(")");
        return attach(node);
    }
    
    if (tok = consume_kind(TK_IDENT)) {
        // 変数か関数
        if (consume("(")) { // 関数の場合
            node = new_node(ND_CALL_FUNC);
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
            node = new_node(ND_LVAR);
            
            // 変数,enumの順番
            if (node->lvar = find_lvar(tok)) {}            
            else if (enum_mem_node = find_enum_membar(tok))
                return enum_mem_node;
            else 
                error_at(tok->str, "宣言されていない変数です");
                
            // 既存の変数
            if (!node->lvar->glb_var)
                node->offset = node->lvar->offset;
            
            // ++ -- の処理
            node = add_add_minus_minus(node);

            node = attach(node);
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

Node *attach(Node *node) {
    Node *mem_node;
    LVar *lvar;
    Token *tok;
    VarType *type = AST_type(0, node);

    // (assign), ->membar, .membar, [assign] の繰り返し
	while (true) {
        if (consume("[")) {
            if (type->ty == ARRAY)
                node->access = 1;
            if (type->ty != PTR && type->ty != ARRAY)
                error_at(token->str, "ポインタか配列に添え字を付けてください");
            node = new_binary(ND_INDEX, new_binary(ND_ADD, node, assign()), NULL);
            expect("]");
            type = type->ptr_to;
        }
        else if (consume(".")) {
            if (type->ty != STRUCT)
                error_at(token->str, "左の候がstructでははありません");
            if (!(tok = consume_kind(TK_IDENT)))
                error_at(token->str, "メンバ変数である必要があります");
            if (!(lvar = get_membar(type->struct_p, tok)))
                error_at(token->str, "メンバ変数である必要があります");
            node->access = 1;
            mem_node = new_node(ND_MEMBAR);
            mem_node->lvar = lvar;
            node = new_binary(ND_MEMBAR_ACCESS, new_binary(ND_ADD, node, mem_node), NULL);
            type = lvar->type;
        }
        else if (consume("->")) {
            if (type->ty != PTR || type->ptr_to->ty != STRUCT)
                error_at(token->str, "左の候がstructのポインタではありません");
            if (!(tok = consume_kind(TK_IDENT)))
                error_at(token->str, "メンバ変数である必要があります");
            if (!(lvar = get_membar(type->ptr_to->struct_p, tok)))
                error_at(token->str, "メンバ変数である必要があります");
            node->access = 1;
            mem_node = new_node(ND_MEMBAR);
            mem_node->lvar = lvar;
            node = new_binary(ND_DEREF, node, NULL);
            node = new_binary(ND_MEMBAR_ACCESS, new_binary(ND_ADD, node, mem_node), NULL);
            type = lvar->type;
        }
        else
            break;
        // 1つ前のlhsはデータが欲しいわけではない
        
    }
    return node;
}