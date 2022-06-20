#include "grccnr.h"

bool token_str(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
		return false;
    else 
        return true;
}

bool token_kind(TokenKind kind) {
    if (token->kind == kind) return true;
    else return false;
}

// 次のトークンが期待している記号のときにはトークンを1つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op) {
	if (!token_str(op))
        return false;
	token = token->next;
	return true;
}

// 次のトークンが指定されたkindならTokenを返す
Token *consume_kind(TokenKind kind) {
    Token *tok = token;
    if (!token_kind(kind))
        return NULL;
    token = token->next;
    return tok;
}

// 次のトークンが期待している記号のときには。トークンを1つ読み進める
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
	if (!token_str(op))
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

// 文字列からasciiに
char to_ascii(char *c) {
    if (*c == '\\') {
        switch (*(c+1)) {
            case 'a': return '\a';
            case 'n': return '\n';
            case 't': return '\t';
            case 'r': return '\r';
            case 'f': return '\f';
            case '\'': return '\'';
            case '\"': return '\"';
            case '0': return '\0';
            case '\\': return '\\';
            case '?': return '\?';
            default: error_at(token->str, "%c%cが処理できません", *c, *(c+1));
        }
    } else return *c;
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
    int str = 0;

	while (*p) {
		// 空白文字をスキップ
		if (isspace(*p)) {
			p++;
			continue;
		}

        // 行コメントをスキップ
        if (strncmp(p, "//", 2) == 0) {
            p += 2;
            while (*p != '\n')
                p++;
            continue;
        }

        // ブロックコメントをスキップ
        if (strncmp(p, "/*", 2) == 0) {
            char *q = strstr(p + 2, "*/");
            if (!q)
                error_at(p, "コメントが閉じられていません");
            p = q + 2;
            continue;
        }

		if (startswith(p, "||") || startswith(p, "&&")  || 
            startswith(p, "==") || startswith(p, "!=")  || 
            startswith(p, "<=") || startswith(p, ">=")  ||
            startswith(p, "->") || startswith(p, "++")  ||
            startswith(p, "--") || startswith(p, "+=")  ||
            startswith(p, "-=") || startswith(p, "*=")  ||
            startswith(p, "/=") || startswith(p, "%=")){
			cur = new_token(TK_RESERVED, cur, p, 2);
			p += 2;
			continue;
		}

		if (strchr("+-*/%()<>;={}&,[].#:!", *p)) {
			cur = new_token(TK_RESERVED, cur, p++, 1);
			continue;
		}

		if (isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p, 0);
			char *q = p;
			cur->val = strtoll(p, &p, 10);
			cur->len = p - q;
			continue;
		}

        // ダブル久オートで記される文字列
        if (strchr("\"", *p)) {
            p++;
            char *q = p;
            
            while (!strchr("\"", *q)) {
                if (strchr("\\", *q)) q++;
                q++;
                if (!*p) 
                    error("ダブルクオートで閉じられていません");
            }
            cur = new_token(TK_STR, cur, p, q-p);
            p = q+1;
            continue;
        }

        // シングルクオートで記される文字
        if (strchr("\'", *p)) {
            p++;
            if (strchr("\\", *p)) {
                cur = new_token(TK_ONE_CHAR, cur, p, 2);
                p++;
            } else
                cur = new_token(TK_ONE_CHAR, cur, p, 1);
            p++;
            if (!strchr("\'", *p))
                error_at(p, "シングルクオートで囲む文字は1文字である必要があります");
            p++;
            continue;
        }

        // トークンが数字以外なら文字列の長さを取得して予約語か変数か判定
        if (len = token_len(p)) {
            if (len == 6 && strncmp(p, "return", len) == 0)
                cur = new_token(TK_RETURN, cur, p, len);
            else if (len == 2 && strncmp(p, "if", len) == 0)
                cur = new_token(TK_IF, cur, p, len);
            else if (len == 4 && strncmp(p, "else", len) == 0)
                cur = new_token(TK_ELSE, cur, p, len);
            else if (len == 5 && strncmp(p, "while", len) == 0)
                cur = new_token(TK_WHILE, cur, p, len);
            else if (len == 3 && strncmp(p, "for", len) == 0)
                cur = new_token(TK_FOR, cur, p, len);
            else if (len == 3 && strncmp(p, "int", len) == 0)
                cur = new_token(TK_INT, cur, p, len);
            else if (len == 4 && strncmp(p, "long", len) == 0)
                cur = new_token(TK_LONG, cur, p, len);
            else if (len == 4 && strncmp(p, "char", len) == 0)
                cur = new_token(TK_CHAR, cur, p, len);
            else if (len == 4 && strncmp(p, "void", len) == 0)
                cur = new_token(TK_VOID, cur, p, len);
            else if (len == 6 && strncmp(p, "sizeof", len) == 0)
                cur = new_token(TK_SIZEOF, cur, p, len);
            else if (len == 7 && strncmp(p, "typedef", len) == 0)
                cur = new_token(TK_TYPEDEF, cur, p, len);
            else if (len == 6 && strncmp(p, "struct", len) == 0)
                cur = new_token(TK_STRUCT, cur, p, len);
            else if (len == 4 && strncmp(p, "enum", len) == 0)
                cur = new_token(TK_ENUM, cur, p, len);
            else if (len == 7 && strncmp(p, "include", len) == 0)
                cur = new_token(TK_INCLUDE, cur, p, len);
            else if (len == 8 && strncmp(p, "continue", len) == 0)
                cur = new_token(TK_CONTINUE, cur, p, len);
            else if (len == 5 && strncmp(p, "break", len) == 0)
                cur = new_token(TK_BREAK, cur, p, len);
            else if (len == 6 && strncmp(p, "switch", len) == 0)
                cur = new_token(TK_SWITCH, cur, p, len);
            else if (len == 4 && strncmp(p, "case", len) == 0)
                cur = new_token(TK_CASE, cur, p, len);
            else if (len == 7 && strncmp(p, "default", len) == 0)
                cur = new_token(TK_DEFAULT, cur, p, len);
            else if (len == 4 && strncmp(p, "NULL", len) == 0)
                cur = new_token(TK_NULL, cur, p, len);
            else if (len == 6 && strncmp(p, "extern", len) == 0)
                cur = new_token(TK_EXTERN, cur, p, len);
            else if (len == 3 && strncmp(p, "asm", len) == 0)
                cur = new_token(TK_ASM, cur, p, len);
            else
                cur = new_token(TK_IDENT, cur, p, len);
            p += len;
            continue;
        }
        printf("%d\n", *p);
		error_at(p, "invalid token");
	}
	new_token(TK_EOF, cur, p, 0);
	return head.next;
}