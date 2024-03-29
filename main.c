#include "grccnr.h"

// 宣言されているグローバル変数を定義
char *user_input;
Token *token;
char *filename;
char *output;
int optimize;
FILE *output_file;

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

    // locが含まれている行の開始地点と終了地点を取得
    char *line = loc;
    while (user_input < line && line[-1] != '\n')
        line--;

    char *end = loc;
    while (*end != '\n')
        end++;
    
    // 見つかった行が全体の何行目なのかを調べる
    int line_num = 1;
    for (char *p = user_input; p < line; p++) {
        if (*p == '\n')
            line_num++;
    }
    
    // 見つかった行をファイル名と行番号と一緒に表示
    int indent = fprintf(stderr, "%s:%d:", filename, line_num);
    fprintf(stderr, "%.*s\n", (int)(end - line), line);

    // エラーカ所を ^ で指示してエラーメッセージを表示
    int pos = loc - line + indent;
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
    exit(1);
}

// output, optimize, input取得
void setOption(int argc, char *argv[]) {
    optimize = 0;
    output = "temp.s";
    filename = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp("-o", argv[i]))
            if (argc > ++i) {
                for (int j = 0; argv[i][j] && argv[i][j+1]; j++) {
                    if (!strncmp(".s", &argv[i][j], 2))
                        output = argv[i];
                }
                if (!output) {
                    printf("expected : -o [output file]\n");
                    exit(1);
                }
            } else {
                printf("expected : -o [output file]\n");
                exit(1);
            }
        else if (!strcmp("-O", argv[i]))
            optimize = 1;
        else {
            for (int j = 0; argv[i][j] && argv[i][j+1] && argv[i][j+2]; j++)
                if (!strncmp(".gc", &argv[i][j], 3))
                    filename = argv[i];
        }
    }

    if (!filename) {
        printf("input file is not specify\n");
        exit(1);
    }
}

// 指定されたファイルの内容を返す
char *read_file(char *path) {
    // ファイルを開く
    FILE *fp = fopen(path, "r");
    if (!fp)
        error("cannot open <%s> : %s", path, strerror(errno));
    
    // ファイルの長さを調べる
    if (fseek(fp, 0, SEEK_END) == -1)
        error("%s : fseek : %s", path, strerror(errno));
    size_t size = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) == -1)
        error("%s : fseek : %s", path, strerror(errno));
    
    // ファイル内容を読み込む
    char *buf = calloc(1, size+2);
    fread(buf, size, 1, fp);

    // ファイルが必ず \n\0 で終わるようにする
    if (size == 0 || buf[size - 1] != '\n')
        buf[size++] = '\n';
    buf[size] = '\0';
    fclose(fp);
    return buf;
}

int main(int argc, char *argv[]){
    if (argc < 2){
		fprintf(stderr, "引数の個数が正しくありません\n");
		return 1;
	}

    setOption(argc, argv);
    
	// トークナイズしてパースする
    user_input = read_file(filename);
    
    token = tokenize();
    
	program();
    
    output_file = fopen(output, "w");
    if (output_file == NULL) {
        printf("cannot open output_file : <%s>\n", output);
        exit(1);
    }

	// アセンブリの前半部分を出力
	fprintf(output_file, ".intel_syntax noprefix\n");

    // グローバル変数
    gen_global_var();

    // 先頭の式から順にコード生成
    for (int i = 0; code[i]; i++) {
	    // 抽象構文木を下りながらコード生成
	    gen_func(code[i]);
    }
    fclose(output_file);

    if (optimize)
        // asm読み込み
        optimizeAsm(output);
    
	return 0;
}
