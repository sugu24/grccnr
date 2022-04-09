#include "grccnr.h"

char *arg_register[6] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

// 関数内のローカル変数のサイズを計算
int locals_var_size(LVar *loc) {
    size_t bytes = 0;
    for (; loc ;) {
        if (loc->type->ptrs > 0) bytes += 8;
        else if (loc->type->ty == INT) bytes += 8;
        loc = loc->next;
    }
    return bytes;
}

// 変数が示すアドレスをスタックにpushする
void gen_lval(Node *node) {
    if (node->kind != ND_LVAR)
        error("代入の左辺値が変数ではありません");
    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->offset);
    printf("  push rax\n");
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
    printf("  mov [rax], %s\n", arg_register[argc]);
    return argc + 1;
} 

// ジェネレータ
void gen_stmt(Node *node) {
    int i;
	switch (node->kind) {
        case ND_CALL_FUNC:
            for (i = 0; i < 6 && node->arg[i]; i++)
                gen_stmt(node->arg[i]);
            for (i = i - 1; i >= 0; i--)
                printf("  pop %s\n", arg_register[i]);
            // call前にrspが16の倍数である必要がある
            // and rsp, 0xfffff0 で16の倍数にしても
            // スタックに空洞ができると考えるがだいたい上で実装されている
            // そこが分からない
            printf("  mov rax, rsp\n");
            printf("  and rsp, 0xfffffffffffffff8\n");
            printf("  push rax\n");
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
        case ND_LVAR:
            gen_lval(node);
            printf("  pop rax\n");
            printf("  mov rax, [rax]\n");
            printf("  push rax\n");
            return;
        case ND_ADDR:
            gen_lval(node->lhs);
            return;
        case ND_DEREF:
            gen_stmt(node->lhs);
            for (int i = 0; i <= node->ptrs; i++) {
                printf("  pop rax\n");
                printf("  mov rax, [rax]\n");
                printf("  push rax\n");
            }
            return;
        case ND_ASSIGN:
            if (node->lhs->kind == ND_DEREF) {
                node->lhs->ptrs--;
                gen_stmt(node->lhs);
            } else
                gen_lval(node->lhs);
            gen_stmt(node->rhs);

            printf("  pop rdi\n");
            printf("  pop rax\n");
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