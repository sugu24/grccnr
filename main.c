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
	program();
    
	// アセンブリの前半部分を出力
	printf(".intel_syntax noprefix\n");
	printf(".globl main\n");
	printf("main:\n");

    // プロローグ
    // 変数26個分の領域を確保する
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, 208\n");

    // 先頭の式から順にコード生成
    for (int i = 0; code[i]; i++) {
	    // 抽象構文木を下りながらコード生成
	    gen(code[i]);

        // 式の評価結果はスタックトップに残っている
        printf("  pop rax\n");
    }

	// エピローグ
    // 最後の式の結果がＲＡＸに残っているのでそれが戻り値になる
    printf("  mov rsp, rbp\n");
	printf("  pop rbp\n");
	printf("  ret\n");
	return 0;
}
