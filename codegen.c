#include "grccnr.h"

char *arg_register[6][4] = {
    {"rdi","edi","di","dil"}, 
    {"rsi","esi","si","sil"}, 
    {"rdx","edx","dx","dl"}, 
    {"rcx","ecx","cx","cl"}, 
    {"r8", "r8d","r8w","r8b"}, 
    {"r9", "r9d","r9w","r9b"}
};

int min(int a, int b) {
    if (a <= b) return a;
    else return b;
}

// 関数内のローカル変数のサイズを計算
int locals_var_size(LVar *loc) {
    if (loc)
        return loc->offset;
    else
        return 0;
}

// gen_stmt(node->stmt)後にスタックにraxがpushされる
// gen_stmt(node->stmt)後にmainに戻らない場合に使用
void gen_pop(Node *node) {
    gen_stmt(node);
    printf("  pop rax\n");
    return;
}

// 関数の引数をpushする
int gen_arg_push(LVar *arg) {
    if (!arg) return 0;
    int argc = gen_arg_push(arg->next);
    // rbpのオフセットに引数をmov
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", arg->offset);

    // arg->typeに合わせてレジスタを選択
    switch (get_size(arg->type)) {
        case 1:
            printf("  mov [rax], %s\n", arg_register[argc][3]);
            break;
        case 2:
            printf("  mov [rax], %s\n", arg_register[argc][2]);
            break;
        case 4:
            printf("  mov [rax], %s\n", arg_register[argc][1]);
            break;
        case 8:
            printf("  mov [rax], %s\n", arg_register[argc][0]);
            break;
    }
    return argc + 1;
}

// raxにデータを転送する
void mov_rax_data(Node *node) {
    int size = get_size(AST_type(node));
    if (size == 1)
        printf("  movsx rax, BYTE PTR [rax]\n");
    else if (size == 4)
        printf("  mov eax, [rax]\n");
    else
        printf("  mov rax, [rax]\n");
            
}

// グローバル変数
// 代入式の左辺はアドレスをpushする
void gen_addr(Node *node) {
    switch (node->kind) {
        case ND_DEREF:
            gen_stmt(node->lhs);
            break;
        case ND_LVAR:
            if (node->lvar->glb_var)
                printf("  lea rax, %s[rip]\n", node->lvar->name);
            else {
                printf("  mov rax, rbp\n");
                printf("  sub rax, %d\n", node->offset);
            }
            printf("  push rax\n");
            break;
        default:
            error("左辺の型を処理できません");
    }
    
}

// ジェネレータ
void gen_stmt(Node *node) {
    int i;
    int size;
	switch (node->kind) {
        case ND_CALL_FUNC:
            for (i = 0; i < 6 && node->arg[i]; i++)
                gen_stmt(node->arg[i]);
            for (i = i-1; i >= 0; i--)
                printf("  pop %s\n", arg_register[i][0]);

            // call前にrspが16の倍数である必要がある
            // and rsp, 0xfffff0 で16の倍数にしても
            // スタックに空洞ができると考えるがだいたい上で実装されている
            // そこが分からない
            printf("  mov rax, rsp\n");
            printf("  and rsp, 0xfffffffffffffff8\n");
            printf("  push rax\n");
            printf("  mov al, 0\n");
            printf("  call %s\n", node->func_name);
            printf("  pop rsp\n");
            printf("  push rax\n");
            return;
        case ND_BLOCK:
            while (node->next_stmt) {
                node = node->next_stmt;
                gen_pop(node->stmt);
            }
            printf("  push 0\n");
            return;
        case ND_IF:
            gen_stmt(node->lhs);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");

            if (!node->next_if_else) { // if節単体の場合
                printf("  push 0\n");
                printf("  je .Lend%d\n", node->control);
                gen_stmt(node->stmt);
            } else if (node->next_if_else->kind == ND_ELSE_IF) { //if節の次がelse ifの場合
                printf("  je .Lelseif%d_1\n", node->control);
                gen_stmt(node->stmt);
                printf("  jmp .Lend%d\n", node->control);
                gen_stmt(node->next_if_else);
            } else if (node->next_if_else->kind == ND_ELSE) { // if節の次がelse節の場合
                printf("  je .Lelse%d\n", node->control);
                gen_stmt(node->stmt);
                printf("  jmp .Lend%d\n", node->control);
                gen_stmt(node->next_if_else);
            }

            printf(".Lend%d:\n", node->control);
            return;
        case ND_ELSE_IF:
            printf(".Lelseif%d_%d:\n", node->control, node->offset);
            gen_stmt(node->lhs);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");

            if (!node->next_if_else) { // else if節の次はない場合
                printf("  push 0\n");
                printf("  je .Lend%d\n", node->control);
                gen_stmt(node->stmt);
            } else if (node->next_if_else->kind == ND_ELSE_IF) { // else if節の次がelse if節の場合
                printf("  je .Lelseif%d_%d\n", node->control, node->offset+1);
                gen_stmt(node->stmt);
                printf("  jmp .Lend%d\n", node->control);
                gen_stmt(node->next_if_else);
            } else if (node->next_if_else->kind == ND_ELSE) { // else if節の次がelse節の場合
                printf("  je .Lelse%d\n", node->control);
                gen_stmt(node->stmt);
                printf("  jmp .Lend%d\n", node->control);
                gen_stmt(node->next_if_else);
            }
            return;
        case ND_ELSE:
            printf(".Lelse%d:\n", node->control);
            gen_stmt(node->stmt);
            return;
        case ND_WHILE:
            printf(".Lbegin%d:\n", node->control);
            gen_stmt(node->lhs);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je  .Lend%d\n", node->control);
            gen_pop(node->stmt);
            printf("  jmp .Lbegin%d\n", node->control);
            printf(".Lend%d:\n", node->control);
            printf("  push 0\n"); // mainに戻るとpopされるから適当にpushしておく
            return;
        case ND_FOR:
            gen_pop(node->lhs);
            printf(".Lbegin%d:\n", node->control);
            gen_stmt(node->mhs);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je  .Lend%d\n", node->control);
            gen_pop(node->stmt);
            gen_pop(node->rhs);
            printf("  jmp .Lbegin%d\n", node->control);
            printf(".Lend%d:\n", node->control);
            printf("  push 0\n"); // mainに戻るとpopされるから適当にpushしておく
            return;
        case ND_RETURN:
            gen_stmt(node->lhs);
            printf("  pop rax\n");
            printf("  mov rsp, rbp\n");
            printf("  pop rbp\n");
            printf("  ret\n");
            return;
        case ND_NUM:
            printf("  push %d\n", node->val);
            return;
        case ND_STR:
            printf("  lea rax, .STR%d[rip]\n", node->lvar->offset);
            printf("  push rax\n");
            return;
        case ND_LVAR: // 変数
            if (node->lvar->glb_var) 
                printf("  lea rax, %s[rip]\n", node->lvar->name);
            else {
                printf("  mov rax, rbp\n");
                printf("  sub rax, %d\n", node->offset);
            }
            
            // 配列でないならデータをraxへ
            if (!(node->lvar->type->ty == ARRAY))
                mov_rax_data(node);
            printf("  push rax\n");
            return;
        case ND_ADDR:
            gen_addr(node->lhs);
            return;
        case ND_DEREF:
            gen_stmt(node->lhs);
            if (node->array_accessing == 0) {
                printf("  pop rax\n");
                mov_rax_data(node);
                printf("  push rax\n");
            }
            return;
        case ND_ASSIGN:
            gen_addr(node->lhs);
            gen_stmt(node->rhs);

            size = min(get_size(AST_type(node->lhs)), get_size(AST_type(node->rhs)));

            printf("  pop rdi\n");
            printf("  pop rax\n");

            // charなら1バイトのみ書き込まれる
            if (size == 1)
                printf("  mov [rax], dil\n");
            // int ポインタなどは4,8バイト
            else if (size == 4)
                printf("  mov [rax], edi\n");
            else 
                printf("  mov [rax], rdi\n");
            printf("  push rdi\n");
            return;
    }

	gen_stmt(node->lhs);
	gen_stmt(node->rhs);

	printf("  pop rdi\n");
	printf("  pop rax\n");

	switch (node->kind) {
		case ND_ADD:
			printf("  add rax, rdi\n");
			break;
		case ND_SUB:
			printf("  sub rax, rdi\n");
			break;
		case ND_MUL:
			printf("  imul rax, rdi\n");
			break;
		case ND_DIV:
			printf("  cqo\n");
			printf("  idiv rdi\n");
			break;
		case ND_EQ:
			printf("  cmp rax, rdi\n");
			printf("  sete al\n");
			printf("  movzb rax, al\n");
			break;
		case ND_NE:
			printf("  cmp rax, rdi\n");
			printf("  setne al\n");
			printf("  movzb rax, al\n");
			break;
		case ND_LT:
			printf("  cmp rax, rdi\n");
			printf("  setl al\n");
			printf("  movzb rax, al\n");
			break;
		case ND_LE:
			printf("  cmp rax, rdi\n");
			printf("  setle al\n");
			printf("  movzb rax, al\n");
			break;
	}

	printf("  push rax\n");
	return;
}

// 関数のジェネレータ
void gen_func(Func *func) {
    printf("%s:\n", func->func_name);

    // プロローグ
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", locals_var_size(func->locals));

    // 引数をpush
    gen_arg_push(func->arg); 

    gen_stmt(func->stmt);
    printf("  pop rax\n");

    // エピローグ
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
}

// グローバル変数
void gen_global_var() {
    LVar *glb_var;
    printf(".data\n");
    for (glb_var = global_var; glb_var; glb_var = glb_var->next) {
        printf("%s:\n", glb_var->name);
        printf("  .zero %d\n", get_size(glb_var->type));
    }

    for (glb_var = strs; glb_var; glb_var = glb_var->next) {
        printf(".STR%d:\n", glb_var->offset);
        printf("  .string \"%s\"\n", glb_var->str);
    }

    printf("\n\n.text\n");
    printf(".globl main\n");
}