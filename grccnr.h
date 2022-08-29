#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define INT_SIZE 4
#define LONG_LONG_INT_SIZE 8
#define PTR_SIZE 8
#define CHAR_SIZE 1
#define VOID_SIZE 1

typedef struct Token Token;
typedef struct Node Node;
typedef struct Func Func;

typedef struct LVar LVar;
typedef struct VarType VarType;

typedef struct Typedef Typedef;
typedef struct Struct Struct;
typedef struct Enum Enum;
typedef struct EnumMem  EnumMem;
typedef struct Prototype Prototype;

// ---------- global var ---------- //
extern char *user_input;    // 入力プログラム
extern Token *token;        // 現在着目しているトークン
extern Func *code[1024];
extern LVar *locals;        // 変数名の連結リスト
extern LVar *global_var;    // グローバル変数
extern LVar *strs;          // 文字リテラル
extern Typedef *typedefs;   // typedefの連結リスト
extern Struct *structs;     // structの連結リスト
extern Enum *enums;         // enumの連結リスト
extern Prototype *prototype;// funcのプロトタイプ
extern char *arg_register[6][4]; // 引数を記憶するレジスタ

extern int control;
extern Func *now_func; // 自己呼び出し用

extern FILE *output_file;

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
    TK_INT,      // int
    TK_CHAR,     // char
    TK_SIZEOF,   // sizeof
    TK_STR,      // 文字列
    TK_TYPEDEF,  // typedef
    TK_STRUCT,   // struct
    TK_ENUM,     // enum
    TK_INCLUDE,  // include
    TK_ONE_CHAR, // 1文字
    TK_CONTINUE, // continue
    TK_BREAK,    // break
    TK_LONG,     // long
    TK_SWITCH,   // switch
    TK_CASE,     // case
    TK_DEFAULT,  // default
    TK_VOID,     // void
    TK_NULL,     // NULL
    TK_EXTERN,   // extern
    TK_ASM,      // asm
	TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

// トークン型
struct Token {
	TokenKind kind; // トークンの型
	Token *next;    // 次の入力トークン
	long long int val;        // kindがTK_NUMの場合、その数値
	char *str;      // トークン文字列
	int len;        // トークンの長さ
};

// 抽象構文木のノードの種類
typedef enum {
	ND_ADD,    // 0 +
	ND_SUB,    // 1 -
	ND_MUL,    // 2 *
	ND_DIV,    // 3 /
    ND_MOD,    // 4 %
	ND_NUM,    // 5 整数
    ND_ADDR,   // 6 &var
    ND_DEREF,  // 7 *var
	ND_EQ,     // 8 ==
	ND_NE,     // 9 !=
	ND_LT,     // 10 <
	ND_LE,     // 11 <=
    ND_ASSIGN, // 12= 
    ND_LVAR,   // 13 ローカル変数
    ND_RETURN, // 14 return
    ND_IF,     // 15 if
    ND_ELSE_IF,// 16 else if
    ND_ELSE,   // 17 else
    ND_WHILE,  // 18 while
    ND_FOR,    // 19 for
    ND_BLOCK,  // 20 block文
    ND_CALL_FUNC, // 21 関数呼び出し
    ND_STR_PTR, // 22 文字列のポインタ
    ND_STR,     // 23 文字リテラル
    ND_CHAR,    // 24 1文字
    ND_INDEX,   // 25 []
    ND_MEMBAR,  // 26 メンバ変数
    ND_MEMBAR_ACCESS, // 27 struct.membar
    ND_LVAR_ADD, // 28 ++
    ND_LVAR_SUB, // 29 --
    ND_LOGICAL_ADD, // 30 ||
    ND_LOGICAL_AND, // 31 &&
    ND_CONTINUE,    // 32 continue
    ND_BREAK,       // 33 break
    ND_SWITCH,      // 34 switch
    ND_CASE,        // 35 case
    ND_DEFAULT,     // 36 default
    ND_NOT,         // 37 not
    ND_NULL,        // 38 NULL
    ND_ASM,         // 39 asm
    ND_LEFT_SHIFT,  // 40 <<
    ND_RIGHT_SHIFT, // 41 >>
    ND_AND,         // 42 &
    ND_OR,          // 43 |
    ND_XOR,         // 44 ^
    ND_BIT_NOT,     // 45 ~
} NodeKind;

// 変数の型
typedef enum { 
    VOID = 1,      // 1
    INT,           // 2
    LONG_LONG_INT, // 3
    CHAR,          // 4
    PTR,           // 5
    ARRAY,         // 6
    STRUCT,        // 7
    ENUM,          // 8
} Type;

// 抽象構文木のノード型
struct Node {
	NodeKind kind; // ノードの型
	Node *lhs;     // 左辺
	Node *rhs;     // 右辺
    Node *mhs;     // forの(;;)で使う
    Node *stmt;    // if,for,whileのstmt
    Node *next_if_else; // else if or elseのノード
    Node *next_stmt;  // block内のstmtを表す
    long long int val;       // kindがND_NUMの場合のみ使う
    int offset;    // kindがND_LVAR, ND_ELSE_IF,ND_DEREFの場合に使う
    int control;   // kindが制御構文の場合のみ使う(ラベル番号)
    char *func_name;  // kindがND_CALL_FUNCの場合のみ使う
    int access;    // 配列でmov rax, [rax]しない場合に使う
    Node *arg[7];     // kindがND_CALL_FUNCの場合のみ使う
    LVar *lvar;       // 変数の場合
    Struct *struct_p; // STRUCTの場合に使用
    Enum *enm_p;      // なんとなく付けた
    char *asm_str;
    int len;
    VarType *cast;
    int cltq;
};

// 変数の種類
struct VarType {
    Type ty;
    VarType *ptr_to;
    size_t array_size;
    Struct *struct_p;  // ty=STRUCTの場合使用
    Enum *enum_p;
    int extern_;
};

// 変数の型
struct LVar {
    LVar *next; // 次は変数かNULL
    VarType *type; // 変数の型
    Node *initial; // グローバル変数の初期値
    char *name; // 変数の名前
    char *str;  // 文字リテラル
    int len;    // len(name)
    int offset; // オフセット
    int glb_var;  // グローバル変数
};

// 関数の型
struct Func {
    LVar *func_type_name;
    LVar *arg;
    LVar *locals;
    Node *stmt;
};

// typedefの型
struct Typedef {
    Typedef *next;     // 次のTypedef
    VarType *type;     // INT, CHARなど
    char *name; // 宣言する名前
    int len;           // len(define_name)
};

// structの型
struct Struct {
    Struct *next;  // 次のstruct
    char *name;    // タグ名
    int len;       // len(タグ名)
    LVar *membar;  // メンバ変数(ND_NUM)
}; 

// enumの型
struct Enum {
    Enum *next;    // 次のenumメンバ
    char *name;
    int len;
    EnumMem *membar;  // enum内のメンバ
};

// enumのメンバの型
struct EnumMem {
    EnumMem *next;
    Node *num_node;
    char *name;
    int len;
};

// prototypeの型
struct Prototype {
    Prototype *next;
    Func *func;
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
char to_ascii(char *c);

int token_len(char *p); // トークンの長さを返す
bool startswitch(char *p, char *q); // pとqが一致するか
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize();

// ---------- parser ---------- //
Node *new_node(NodeKind kind);
Node *new_binary(NodeKind kind, Node *lhs, Node *rhs);
Node *new_num(long long int val);
Node *new_char(char *c);
LVar *find_lvar(int local, Token *tok);
char *str_copy(Token *tok);
LVar *declare_var(int type);
Func *same_prototype(Func *func);
Func *same_function(Func *func);

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
Node *Ladd();
Node *Land();
Node *bit_calc();
Node *equality();
Node *relational();
Node *shift();
Node *add();
Node *mul();
Node *unary();
Node *primary();
Node *add_add_minus_minus(Node *node);
Node *attach(Node *node);
// ---------- generator ---------- //
void gen_func(Func *func);
void gen_addr(Node *node);
void gen_stmt(Node *node);
void gen_global_var();

// ---------- 構文木の型を調査 ---------- //
VarType *AST_type(int ch, Node *node);
int get_size(VarType *type);
VarType *get_type(VarType *type);
int get_offset(Node *node);
VarType *func_type(Node *node);
int same_type(VarType *v1, VarType *v2);

// ---------- 初期化 ---------- //
Node *initialize(int type, VarType *var_type, Node *lvar_node);

// ---------- main ---------- //
char *read_file(char *path);

// ---------- cfunc ---------- //
VarType *cfunc_type(Node *node);

// ---------- optimize ---------- //
void optimizeAsm(char *output);