#include "grccnr.h"

// 次のトークンが期待している記号のときにはトークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op) {
	if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len)) {
		return false;
    }
	token = token->next;
	return true;
}

// 次のトークンが指定されたkindならTokenを返す
Token *consume_kind(TokenKind kind) {
    Token *tok = token;
    if (tok->kind != kind)
        return NULL;
    token = token->next;
    return tok;
}

// 次のトークンが期待している記号のときには。トークンを1つ読み進める
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
	if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
		error_at(token->str, "expected '%s'", op);
	token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
	if (token->kind != TK_NUM)
		error_at(token->str, "expected a number");
	int val = token->val;
	token = token->next;
	return val;
}

// アルファベットまたはアンダーバーならtrue
int is_alpha(char c) {
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           (c == '_');
}

// 数字ならtrue
int is_num(char c) {
    return ('0' <= c && c <= '9');
}

// トークンの長さを返す
int token_len(char *p) {
    int len = 0;
    while ((len == 0 && is_alpha(*p)) || 
           (len > 0 && (is_alpha(*p) || is_num(*p)))) {
        p++;
        len++;
    }
    return len;
}

bool at_eof() {
	return token->kind == TK_EOF;
}

// 新しいトークンを生成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
	Token *tok = calloc(1, sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	tok->len = len;
	cur->next = tok;
	return tok;
}

bool startswith(char *p, char *q) {
	return memcmp(p, q, strlen(q)) == 0;
}

// 入力文字列pをトークナイズしてしてそれを返す
Token *tokenize() {
	char *p = user_input;
	Token head;
	head.next = NULL;
	Token *cur = &head;
    int len;

	while (*p) {
		// 空白文字をスキップ
		if (isspace(*p)) {
			p++;
			continue;
		}

		if (startswith(p, "==") || startswith(p, "!=") ||
			 startswith(p, "<=") || startswith(p, ">=")){
			cur = new_token(TK_RESERVED, cur, p, 2);
			p += 2;
			continue;
		}

		if (strchr("+-*/()<>;={}&,[]", *p)) {
			cur = new_token(TK_RESERVED, cur, p++, 1);
			continue;
		}

		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p, 0);
			char *q = p;
			cur->val = strtol(p, &p, 10);
			cur->len = p - q;
			continue;
		}

        // トークンが数字以外なら文字列の長さを取得して予約語か変数か判定
        if (len = token_len(p)) {
            if (len == 6 && strncmp(p, "return", len) == 0)
                cur = new_token(TK_RETURN, cur, p, 6);
            else if (len == 2 && strncmp(p, "if", len) == 0)
                cur = new_token(TK_IF, cur, p, 2);
            else if (len == 4 && strncmp(p, "else", len) == 0)
                cur = new_token(TK_ELSE, cur, p, 4);
            else if (len == 5 && strncmp(p, "while", len) == 0)
                cur = new_token(TK_WHILE, cur, p, 5);
            else if (len == 3 && strncmp(p, "for", len) == 0)
                cur = new_token(TK_FOR, cur, p, 3);
            else if (len == 3 && strncmp(p, "int", len) == 0)
                cur = new_token(TK_VAR_TYPE, cur, p, 3);
            else if (len == 6 && strncmp(p, "sizeof", len) == 0)
                cur = new_token(TK_SIZEOF, cur, p, 6);
            else
                cur = new_token(TK_IDENT, cur, p, len);
            p += len;
            continue;
        }

		error_at(p, "invalid token");
	}
	new_token(TK_EOF, cur, p, 0);
	return head.next;
}