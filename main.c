#include "grccnr.h"

// 宣言されているグローバル変数を定義
char *user_input;
Token *token;

// エラーを警告するための関数
// printfと同じ引数
void error(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

// エラー箇所を報告する
void error_at(char *loc, char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	int pos = loc - user_input;
	fprintf(stderr, "%s\n", user_input);
	fprintf(stderr, "%*s", pos, " ");
	fprintf(stderr, "^ ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

int main(int argc, char** argv){
    if (argc != 2){
		fprintf(stderr, "引数の個数が正しくありません\n");
		return 1;
	}
	
	// トークナイズしてパースする
	user_input = argv[1];
	token = tokenize();
	Node *node = expr();

	// アセンブリの前半部分を出力
	printf(".intel_syntax noprefix\n");
	printf(".globl main\n");
	printf("main:\n");

	// 抽象構文木を下りながらコード生成
	gen(node);

	// スタックトップに式全体の値が残っているはずなので
	// それをRAXにロードして関数からの戻り値とする
	printf("  pop rax\n");
	printf("  ret\n");
	return 0;
}
