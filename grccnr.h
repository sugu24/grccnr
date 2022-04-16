#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define INT_SIZE 4
#define PTR_SIZE 8
#define CHAR_SIZE 1

typedef struct Token Token;
typedef struct Node Node;
typedef struct Func Func;

typedef struct LVar LVar;
typedef struct VarType VarType;

// ---------- global var ---------- //
extern char *user_input; // 入力プログラム
extern Token *token; // 現在着目しているトークン
extern Func *code[1024];
extern LVar *locals; // 変数名の連結リスト
extern LVar *global_var; // グローバル変数
extern LVar *strs;
extern LVar *func_locals[1024]; // 関数内のローカル変数
extern char *arg_register[6][4]; // 引数を記憶するレジスタ

extern int control;
extern Func *now_func; // 自己呼び出し用

// トークンの種類
typedef enum {
	TK_RESERVED, // 記号
    TK_IDENT,    // 識別子
	TK_NUM,      // 整数トークン
    TK_RETURN,   // return
    TK_IF,       // if
    TK_ELSE,     // else
    TK_WHILE,    // while
    TK_FOR,      // for
    TK_VAR_TYPE, // int
    TK_SIZEOF,   // sizeof
    TK_STR,      // 文字列
	TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

// トークン型
struct Token {
	TokenKind kind; // トークンの型
	Token *next;    // 次の入力トークン
	int val;        // kindがTK_NUMの場合、その数値
	char *str;      // トークン文字列
	int len;        // トークンの長さ
};

// 抽象構文木のノードの種類
typedef enum {
	ND_ADD,    // 0 +
	ND_SUB,    // 1 -
	ND_MUL,    // 2 *
	ND_DIV,    // 3 /
	ND_NUM,    // 4 整数
    ND_ADDR,   // 5 &var
    ND_DEREF,  // 6 *var
	ND_EQ,     // 7 ==
	ND_NE,     // 8 !=
	ND_LT,     // 9 <
	ND_LE,     // 10 <=
    ND_ASSIGN, // 11= 
    ND_LVAR,   // 12 ローカル変数
    ND_RETURN, // 13 return
    ND_IF,     // 14 if
    ND_ELSE_IF,// 15 else if
    ND_ELSE,   // 16 else
    ND_WHILE,  // 17 while
    ND_FOR,    // 18 for
    ND_BLOCK,  // 19 block文
    ND_CALL_FUNC, // 20 関数呼び出し
    ND_STR_PTR, // 21 文字列のポインタ
    ND_STR,     // 22 文字リテラル
} NodeKind;

// 変数の型
typedef enum { INT = 1, CHAR, PTR, ARRAY } Type;

// 抽象構文木のノード型
struct Node {
	NodeKind kind; // ノードの型
	Node *lhs;     // 左辺
	Node *rhs;     // 右辺
    Node *mhs;     // forの(;;)で使う
    Node *stmt;   // if,for,whileのstmt
    Node *next_if_else; // else if or elseのノード
    Node *next_stmt; // block内のstmtを表す
    int val;       // kindがND_NUMの場合のみ使う
    int offset;    // kindがND_LVAR, ND_ELSE_IF,ND_DEREFの場合に使う
    int control;   // kindが制御構文の場合のみ使う(ラベル番号)
    char *func_name; // kindがND_CALL_FUNCの場合のみ使う
    //char *str;     // 文字リテラル
    int array_accessing;   // 配列でmov rax, [rax]しない場合に使う
    Node *arg[7];  // kindがND_CALL_FUNCの場合のみ使う
    LVar *lvar;     // 変数の場合
};

// 変数の種類
struct VarType {
    Type ty;
    VarType *ptr_to;
    size_t array_size;
};

// 変数の型
struct LVar {
    LVar *next; // 次は変数かNULL
    VarType *type; // 変数の型
    Node *initial; // グローバル変数の初期値
    char *name; // 変数の名前
    char *str;  // 文字リテラル
    int len;    // len(name)
    int offset; // RBPからのオフセット
    int glb_var;  // グローバル変数
};

// 関数の型
struct Func {
    LVar *func_type_name;
    LVar *arg;
    LVar *locals;
    Node *stmt;
};

// ---------- error ---------- //
void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);

// ---------- tokenizer ---------- //
bool token_str(char *op);
bool token_kind(TokenKind kind);
bool consume(char *op);
Token *consume_kind(TokenKind kind);
void expect(char *op);
int expect_number();
bool at_eof();

int token_len(char *p); // トークンの長さを返す
bool startswitch(char *p, char *q); // pとqが一致するか
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize();

// ---------- parser ---------- //
Node *new_node(NodeKind kind);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
Node *new_num(int val);
LVar *find_lvar(Token *tok);

Node *create_if_node(int con, int chain);
Node *create_else_node(int con, int chain);
Node *create_while_node();
Node *create_for_node();
Node *create_return_node();

void program();
Func *glbstmt();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();

// ---------- generator ---------- //
void gen_func(Func *func);
void gen_addr(Node *node);
void gen_stmt(Node *node);
void gen_global_var();

// ---------- 構文木の型を調査 ---------- //
VarType *AST_type(Node *node);
int get_size(VarType *type);