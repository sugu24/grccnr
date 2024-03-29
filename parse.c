#include "grccnr.h"

typedef struct Loop Loop;
struct Loop {
    Node *node;
    Loop *bfr;
};

Func *code[1024];
LVar *locals;
LVar *global_var;
LVar *strs;
Func *now_func;
Typedef *typedefs = NULL;
Struct *structs = NULL;
Enum *enums = NULL;
Prototype *prototype = NULL;

int control = 0;
int str_num = 0;
Loop *Continue = NULL;
Loop *Break = NULL;

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

Node *new_num(long long int val) {
	Node *node = new_node(ND_NUM);
	node->val = val;
	return node;
}

Node *new_char(char *c) {
    Node *node = new_node(ND_CHAR);
    node->val = to_ascii(c);
    return node;
}

Node *new_null() {
    Node *node = new_node(ND_NULL);
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
LVar *find_lvar(int local, Token *tok) {
    // ローカル変数
    for (LVar *var = locals; var && local; var = var->next) {
        if (tok->len == var->len && 
            !memcmp(tok->str, var->name, var->len))
            return var;
    }

    // グローバル変数
    LVar *bfr = NULL;
    for (LVar *var = global_var; var && local > 1; var = var->next) {
        if (tok->len == var->len && 
            !memcmp(tok->str, var->name, var->len)) {
            if (local == 2 && var->type->extern_) {
                if (!bfr && var->next) global_var = var->next;
                else if (!bfr && !var->next) global_var = NULL;
                else if (var->next) bfr->next = var->next;
                else if (!var->next) bfr->next = NULL;
                continue;
            }
            return var;
        }
        bfr = var;
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
Struct *get_struct(char *str, int len) {
    for (Struct *struct_p = structs; struct_p; struct_p = struct_p->next) {
        if (len == struct_p->len && !memcmp(str, struct_p->name, len)) {
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

// Continueに追加
void add_continue(Node *node) {
    Loop *loop = calloc(1, sizeof(Loop));
    loop->node = node;
    loop->bfr = Continue;
    Continue = loop;
}

// Breakに追加
void add_break(Node *node) {
    Loop *loop = calloc(1, sizeof(Loop));
    loop->node = node;
    loop->bfr = Break;
    Break = loop;
}

// Continueから削除
void remove_continue() {
    Loop *loop = Continue;
    Continue = Continue->bfr;
    free(loop);
}

// Breakから削除
void remove_break() {
    Loop *loop = Break;
    Break = Break->bfr;
    free(loop);
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
    
    add_continue(node);
    add_break(node);
    node->stmt = stmt();
    remove_continue();
    remove_break();

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
    
    add_continue(node);
    add_break(node);
    node->stmt = stmt();
    remove_continue();
    remove_break();
    
    return node;
}

// stmtがswitchの場合にBNFにしたがってNodeを生成して、Nodeを返す
Node *create_switch_node() {
    Node *node = new_node(ND_SWITCH);
    expect("(");
    node->lhs = expr();
    expect(")");
    add_break(node);
    node->control = control++;
    expect("{");
    node->stmt = create_block_node();
    remove_break();
    return node;
}

// caseのnodeを返す
Node * create_case_node() {
    if (!Break || Break->node->kind != ND_SWITCH)
        error_at(token->str, "case が switch 文内に来ていません");
    Node *node = new_node(ND_CASE);
    Node *case_node = Break->node;
    node->lhs = Break->node;
    node->rhs = primary();
    expect(":");

    // switchのcaseのlastに入れる
    int i;
    for (i = 0; case_node->next_if_else; i++) case_node = case_node->next_if_else;
    node->offset = i;

    case_node->next_if_else = node;

    if (node->rhs->kind != ND_NUM && node->rhs->kind != ND_CHAR) 
        error_at(token->str, "caseで指定する値は定数である必要があります");

    return node;
}

// defaultのnodeを返す
Node *create_default_node() {
    if (!Break || Break->node->kind != ND_SWITCH)
        error_at(token->str, "default が switch 文内に来ていません");
    Node *node = new_node(ND_DEFAULT);
    node->lhs = Break->node;
    Node *case_node = Break->node;
    expect(":");

    // switchのcaseのlastに入れる
    int i;
    for (i = 0; case_node->next_if_else; i++) case_node = case_node->next_if_else;
    node->offset = i;

    case_node->next_if_else = node;

    return node;
}

// // stmtがreturnの場合にBNFに沿ってNodeを生成して、Nodeを返す
Node *create_return_node() {
    Node *node = new_node(ND_RETURN);
    if (!token_str(";")) node->lhs = expr();
    expect(";");
    return node;
}

Node *create_continue_node() {
    if (!Continue) error_at(token->str, "繰り返し構文内のみで continue は使用可能です");
    Node *node = new_node(ND_CONTINUE);
    node->control = Continue->node->control;
    expect(";");
    return node;
}

Node *create_break_node() {
    if (!Break) error_at(token->str, "繰り返し構文内 または switch 文内のみで break は使用可能です");
    Node *node = new_node(ND_BREAK);
    node->control = Break->node->control;
    expect(";");
    return node;
}

Node *create_asm_node() {
    Token *tok;
    Node *node = new_node(ND_ASM);
    expect("(");
    if (tok = consume_kind(TK_STR))
        node->asm_str = str_copy(tok);
    else
        error_at(token->str, "asm(string)である必要があります");
    expect(")");
    expect(";");
    return node;
}

// ----------------------- 変数宣言系 ----------------------- //

// structのtypeを生成する
Struct *new_struct() {
    Struct *struct_p;
    LVar *membar, *top_membar, *cur_membar;
    Token *tok;

    // タグ名があるか
    tok = consume_kind(TK_IDENT);
    if (tok && (struct_p = get_struct(tok->str, tok->len)))
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
    top_membar = cur_membar = calloc(1, sizeof(LVar));
    while (membar = declare_var(0)) {
        cur_membar->next = membar;
        cur_membar = membar;
        expect(";");
    }
    struct_p->membar = top_membar->next;

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
    int minus;

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
            if (consume("-"))
                minus = -1;
            else
                minus = 1;

            if (tok = consume_kind(TK_NUM))
                val = tok->val * minus;
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

// ヒープ領域にコピーしてそれを指す
char *str_copy(Token *tok) {
    char *temp = (char*)malloc(sizeof(char) * (tok->len + 1));
    strncpy(temp, tok->str, tok->len);
    temp[tok->len] = '\0';
    return temp;
}

// 原始的な型かどうか
Type get_var_type() {
    if (consume_kind(TK_VOID))
        return VOID;
    else if (consume_kind(TK_INT))
        return INT;
    else if (consume_kind(TK_CHAR))
        return CHAR;
    else if (consume_kind(TK_LONG)) {
        if (consume_kind(TK_INT))
            return INT;
        else if (consume_kind(TK_LONG))
            if (consume_kind(TK_INT))
                return LONG_LONG_INT;
            else
                error_at(token->str, "long long の次は int である必要があります");
        else
            error_at(token->str, "long の後は long か int である必要があります");
    } else 
        return 0;
}

// 変数の型を返す
VarType *new_var_type(int type) {
    VarType *var_type, *temp_type, *top_var_type;
    Type ty;
    int typedef_type = 0;
    var_type = calloc(1, sizeof(VarType));
    
    // 型取得    
    while (true) {
        if (consume_kind(TK_TYPEDEF)) {
            typedef_type = 1;
        } else if (consume_kind(TK_EXTERN)) {
            if (var_type->extern_)
                error_at(token->str, "externを重複して宣言しています");
            if (type < 2)
                error_at(token->str, "まだグローバル領域のみextern宣言の処理が出来ません");
            var_type->extern_ = 1;
        } else if (ty = get_var_type()) {
            if (var_type->ty)
                error_at(token->str, "型は一意に定めなければいけません");
            if (type != 2 && ty == VOID)
                error_at(token->str, "void型は変数の型として指定できません");
            var_type->ty = ty;
        } else if (temp_type = get_typedefed_type()) {
            if (var_type->ty)
                error_at(token->str, "型は一意に定めなければいけません");
            /*if (temp_type->ty == STRUCT)
                var_type->struct_p = temp_type->struct_p;
            else if (temp_type->enum_p)
                var_type->enum_p = temp_type->enum_p;*/
            temp_type->extern_ = var_type->extern_;
            free(var_type);
            var_type = temp_type;
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
    // printf("end\n");

    top_var_type = var_type;

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
        top_var_type->extern_ = var_type->extern_;
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
    Token *tok;
    Node *node;
    VarType *array_top, *array_bfr, *array_now;
    LVar *lvar = calloc(1, sizeof(LVar));

    // 変数の型
    lvar->type = new_var_type(type);
    if (!lvar->type)
        return NULL;

    // 変数名
    tok = consume_kind(TK_IDENT);
    if (!tok) {
        if (lvar->type->ty == STRUCT || lvar->type->enum_p) {
            // structかつvar_nameがない
            // structの定義だけだった
            expect(";");
            return NULL;
        }
        error_at(token->str, "宣言する変数名がありません");
    }

    if (find_lvar(type, tok))
        error_at(tok->str, "既に宣言された変数名です");

    lvar->name = str_copy(tok);
    lvar->len = tok->len;

    // 関数ならreturn グローバル変数ならlvar->glb_var=1
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
            } 
            else if (tok = consume_kind(TK_IDENT)) {
                if (node = find_enum_membar(tok)) {
                    array_now->array_size = node->val;
                    if (array_now->array_size <= 0) {
                        printf("array size must be plus.\n");
                        exit(1);
                    }
                } else
                    error_at(token->str, "unexpect : there is identification after [");
            }
            else if (array_bfr != array_top)
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

Func *glbstmt() {
    Func *func; // 関数の場合に仕様
    LVar *var_or_func, *arg_var;
    
    if (consume("#")) {
        // #include処理
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
                error_at(token->str, "1つ前の関数名が重複しています.");
            
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
        // voidかたはNG
        VarType *temp = var_or_func->type;
        while (temp) {
            if (temp->ty == VOID) 
                error_at(token->str, "void型は変数の型として指定できません");
            temp = temp->ptr_to;
        }

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
        return lvar->initial;
    } else if (typed != typedefs)
        return NULL;

    if (consume("{")) { // block文
        return create_block_node();
    } else if (consume_kind(TK_IF)) {
        return create_if_node(-1,0);
    } else if (consume_kind(TK_WHILE)) {
        return create_while_node();
    } else if (consume_kind(TK_FOR)) { 
        return create_for_node();
    } else if (consume_kind(TK_SWITCH)) {
        return create_switch_node();
    } else if (consume_kind(TK_CASE)) {
        return create_case_node();
    } else if (consume_kind(TK_DEFAULT)) {
        return create_default_node();
    } else if (consume_kind(TK_RETURN)) {
        return create_return_node();
    } else if (consume_kind(TK_CONTINUE)) {
        return create_continue_node();
    } else if (consume_kind(TK_BREAK)) {
        return create_break_node();
    } else if (consume_kind(TK_ASM)) {
        return create_asm_node();
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
    Node *node = Ladd();
    
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

int and_add = 0;
// Ladd = Land ("||" Land)*
Node *Ladd() {
    Node *node = Land();

    for (;;) {
        if (consume("||"))
            node = new_binary(ND_LOGICAL_ADD, node, Ladd());
        else
            return node;
        node->control = and_add++;
    }
}

// Land = equality ("&&" equality)*
Node *Land() {
    Node *node = bit_calc();

    for (;;) {
        if (consume("&&"))
            node = new_binary(ND_LOGICAL_AND, node, Land());
        else
            return node;
        node->control = and_add++;
    }
}

Node *bit_calc() {
    Node *node = equality();

    for (;;) {
        if (consume("|"))
            node = new_binary(ND_OR, node, bit_calc());
        else if (consume("^"))
            node = new_binary(ND_XOR, node, bit_calc());
        else if (consume("&"))
            node = new_binary(ND_AND, node, bit_calc());
        else
            return node;
    }
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
	Node *node = shift();

	for (;;) {
		if (consume("<"))
			node = new_binary(ND_LT, node, shift());
		else if (consume("<="))
			node = new_binary(ND_LE, node, shift());
		else if (consume(">"))
			node = new_binary(ND_LT, shift(), node);
		else if (consume(">="))
			node = new_binary(ND_LE, shift(), node);
		else
			return node;
	}
}

Node *shift() {
    Node *node = add();

    for (;;) {
        if (consume("<<"))
            node = new_binary(ND_LEFT_SHIFT, node, add());
        else if (consume(">>"))
            node = new_binary(ND_RIGHT_SHIFT, node, add());
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

Node *sizeof_val() {
    VarType *var_type;
    Token *tok = token;

    // sizeof(int,struct ...) or sizeof(変数)に対応
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

Node *unary() {
	if (consume("+"))
		return unary();
	else if (consume("-"))
		return new_binary(ND_SUB, new_num(0), unary());
    else if (consume("!"))
        return new_binary(ND_NOT, unary(), NULL);
    else if (consume("~"))
        return new_binary(ND_BIT_NOT, unary(), NULL);
    else if (consume("*"))
        return new_binary(ND_DEREF, unary(), NULL);
    else if (consume("&"))
        return new_binary(ND_ADDR, unary(), NULL);
    else if (consume_kind(TK_SIZEOF)) 
        return sizeof_val();
    return primary();
}

Node *primary() {
    LVar *lvar;
	Token *tok;
    Node *node, *enum_mem_node;
    VarType *cast;

    // cast処理
    // 次のトークンが"("なら、"(" expr ")"
    while (consume("(")) {
        if (cast = new_var_type(0)) {
            expect(")");
            node = primary();
            node->cast = cast;
            return node;
        } else {
            node = assign();
            expect(")");
            return attach(node);
        }
    }
    
    // 変数か関数
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
            
            // 変数,enumの順番で変数確認
            if (node->lvar = find_lvar(3, tok)) {}            
            else if (enum_mem_node = find_enum_membar(tok))
                return enum_mem_node;
            else 
                error_at(tok->str, "宣言されていない変数です");
                
            // offset設定
            if (!node->lvar->glb_var)
                node->offset = node->lvar->offset;
            
        }
        node = attach(node);
        // ++ -- の処理
        node = add_add_minus_minus(node);
        return node;
    }
    
    // 文字リテラル
    else if (tok = consume_kind(TK_STR)) {
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

    else if (tok = consume_kind(TK_ONE_CHAR))
        // ''で囲まれたascii
        return new_char(tok->str);
    else if (tok = consume_kind(TK_NULL))
        return new_null();
    else if (tok = consume_kind(TK_NUM))
        return new_num(tok->val);
    
    error_at(token->str, "処理できません");
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
            node->access = 2;
            mem_node = new_node(ND_MEMBAR);
            mem_node->lvar = lvar;
            node = new_binary(ND_DEREF, node, NULL);
            // CALL_FUNCならアドレスが返されるからDEREFしないようにする
            if (node->lhs->kind == ND_CALL_FUNC) node->access = 2;
            else node->access = 1;
            node = new_binary(ND_MEMBAR_ACCESS, new_binary(ND_ADD, node, mem_node), NULL);
            type = lvar->type;
            if (type->ty == PTR && type->ptr_to->ty == STRUCT)
                type->ptr_to->struct_p = get_struct(type->ptr_to->struct_p->name, type->ptr_to->struct_p->len);
        }
        else
            break;
        // 1つ前のlhsはデータが欲しいわけではない
        
    }
    return node;
}