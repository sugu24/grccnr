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
    switch (arg->type->ty) {
        case CHAR:
            printf("  mov [rax], %s\n", arg_register[argc][3]);
            break;
        case 100:
            printf("  mov [rax], %s\n", arg_register[argc][2]);
            break;
        case INT:
            printf("  mov [rax], %s\n", arg_register[argc][1]);
            break;
        case LONG_LONG_INT:
            printf("  mov [rax], %s\n", arg_register[argc][0]);
        case STRUCT:
            error("関数の引数に構造体は未定義です");
        default:
            printf("  mov [rax], %s\n", arg_register[argc][0]);
            break;
    }
    return argc + 1;
}

// raxにデータを転送する
void mov_rax_data(Node *node) {
    int size = get_size(AST_type(0, node));
    if (size == 1)
        printf("  movsx rax, BYTE PTR [rax]\n");
    else if (size == 4)
        printf("  mov eax, [rax]\n");
    else 
        printf("  mov rax, [rax]\n");

    return;
}

// グローバル変数
// 代入式の左辺はアドレスをpushする
void gen_addr(Node *node) {
    switch (node->kind) {
        case ND_MEMBAR_ACCESS:
            gen_stmt(node->lhs);
            return;
        case ND_INDEX:
            gen_stmt(node->lhs);
            return;
        case ND_DEREF:
            gen_stmt(node->lhs);
            return;
        case ND_LVAR:
            if (node->lvar->glb_var)
                printf("  lea rax, .%s[rip]\n", node->lvar->name);
            else {
                printf("  mov rax, rbp\n");
                printf("  sub rax, %d\n", node->offset);
            }
            printf("  push rax\n");
            return;
        default:
            error("左辺の型を処理できません");
    }
    
}

// ジェネレータ
void gen_stmt(Node *node) {
    int i;
    int size;
    Node *case_node;
    // printf("%d\n", node->kind);
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
                printf("  je .Lendif%d\n", node->control);
                gen_stmt(node->stmt);
            } else if (node->next_if_else->kind == ND_ELSE_IF) { //if節の次がelse ifの場合
                printf("  je .Lelseif%d_1\n", node->control);
                gen_stmt(node->stmt);
                printf("  jmp .Lendif%d\n", node->control);
                gen_stmt(node->next_if_else);
            } else if (node->next_if_else->kind == ND_ELSE) { // if節の次がelse節の場合
                printf("  je .Lelseif%d\n", node->control);
                gen_stmt(node->stmt);
                printf("  jmp .Lendif%d\n", node->control);
                gen_stmt(node->next_if_else);
            }

            printf(".Lendif%d:\n", node->control);
            return;
        case ND_ELSE_IF:
            printf(".Lelseif%d_%d:\n", node->control, node->offset);
            gen_stmt(node->lhs);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");

            if (!node->next_if_else) { // else if節の次はない場合
                printf("  push 0\n");
                printf("  je .Lendif%d\n", node->control);
                gen_stmt(node->stmt);
            } else if (node->next_if_else->kind == ND_ELSE_IF) { // else if節の次がelse if節の場合
                printf("  je .Lelseif%d_%d\n", node->control, node->offset+1);
                gen_stmt(node->stmt);
                printf("  jmp .Lendif%d\n", node->control);
                gen_stmt(node->next_if_else);
            } else if (node->next_if_else->kind == ND_ELSE) { // else if節の次がelse節の場合
                printf("  je .Lelseif%d\n", node->control);
                gen_stmt(node->stmt);
                printf("  jmp .Lendif%d\n", node->control);
                gen_stmt(node->next_if_else);
            }
            return;
        case ND_ELSE:
            printf(".Lelseif%d:\n", node->control);
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
            printf(".Lcmp%d:\n", node->control);
            gen_stmt(node->mhs);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je  .Lend%d\n", node->control);
            gen_pop(node->stmt);
            printf(".Lbegin%d:\n", node->control);
            gen_pop(node->rhs);
            printf("  jmp .Lcmp%d\n", node->control);
            printf(".Lend%d:\n", node->control);
            printf("  push 0\n"); // mainに戻るとpopされるから適当にpushしておく
            return;
        case ND_SWITCH:
            gen_stmt(node->lhs);
            printf("  pop rdi\n");
            case_node = node;
            while (case_node = case_node->next_if_else) {
                switch (case_node->kind) {
                    case ND_CASE:
                        printf("  mov rax, %d\n", case_node->rhs->val);
                        printf("  cmp rax, rdi\n");
                        printf("  sete al\n");
                        printf("  movzb rax, al\n");
                        printf("  je .Lcase%d_%d\n", node->control, case_node->offset);
                        break;
                    case ND_DEFAULT:
                        printf("  jmp .Ldefault%d\n", node->control);
                        break;
                    default:
                        error("switch文でcase, default以外を検出しました");
                }
            }
            printf("  jmp .Lend%d\n", node->control);
            gen_pop(node->stmt);
            printf(".Lend%d:\n", node->control);
            printf("  push 0\n");
            return;
        case ND_CASE:
            printf(".Lcase%d_%d:\n", node->lhs->control, node->offset);
            printf("  push 0\n");
            return;
        case ND_DEFAULT:
            printf(".Ldefault%d:\n", node->lhs->control);
            printf("  push 0\n");
            return;
        case ND_CONTINUE:
            printf("  jmp .Lbegin%d\n", node->control);
            return;
        case ND_BREAK:
            printf("  jmp .Lend%d\n", node->control);
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
        case ND_CHAR:
            printf("  mov rax, %d\n", (int)node->val);
            printf("  push rax\n");
            return;
        case ND_STR_PTR:
            printf("  lea rax, .STR%d[rip]\n", node->lvar->offset);
            printf("  push rax\n");
            return;
        case ND_LVAR_ADD:
            gen_stmt(node->lhs);
            printf("  pop rax\n");
            printf("  push [rax]\n");
            printf("  add [rax], QWORD PTR 1\n");
            return;
        case ND_LVAR_SUB:
            gen_stmt(node->lhs);
            printf("  pop rax\n");
            printf("  push [rax]\n");
            printf("  sub [rax], QWORD PTR 1\n");
            return;
        case ND_NOT:
            gen_stmt(node->lhs);
            printf("  cmp rax, 0\n");
            printf("  sete al\n");
            printf("  movzb rax, al\n");
            printf("  push rax\n");
            return;
        case ND_LVAR: // 変数
            if (node->lvar->glb_var) 
                printf("  lea rax, .%s[rip]\n", node->lvar->name);
            else {
                printf("  mov rax, rbp\n");
                printf("  sub rax, %d\n", node->offset);
            }
            
            // アクセス中または配列、構造体は単体だとアドレス
            if (!node->access && node->lvar->type->ty != ARRAY && node->lvar->type->ty != STRUCT) {
                mov_rax_data(node);
            }

            printf("  push rax\n");
            return;
        case ND_NULL:
            printf("  push 0\n");
            return;
        case ND_ADDR:
            gen_addr(node->lhs);
            return;
        case ND_DEREF:
            gen_stmt(node->lhs);
            printf("  pop rax\n");
            if (node->access) printf("  mov rax, [rax]\n");
            else mov_rax_data(node);
            printf("  push rax\n");
            return;
        case ND_INDEX:
            gen_stmt(node->lhs);
            if (!node->access &&
                AST_type(0, node->lhs)->ptr_to->ty != ARRAY) {
                // ポインタに添え字がついていて添え字がlastか配列の途中出ない場合
                printf("  pop rax\n");
                mov_rax_data(node);
                printf("  push rax\n");
            }
            return;
        case ND_MEMBAR_ACCESS:
            gen_stmt(node->lhs);
            if (!node->access) {
                printf("  pop rax\n");
                mov_rax_data(node);
                printf("  push rax\n");
            }
            return;
        case ND_MEMBAR:
            printf("  push %d\n", node->offset);
            return;
        case ND_ASSIGN:
            gen_addr(node->lhs);
            gen_stmt(node->rhs);

            size = get_size(AST_type(0, node->lhs));
            
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
        case ND_MOD:
            printf("  cqo\n");
            printf("  idiv rdi\n");
            printf("  mov rax, rdx\n");
            break;
        case ND_LOGICAL_ADD:
            printf("  cmp rax, 0\n");
            printf("  setne al\n");
            printf("  movzb rax, al\n");
            printf("  cmp rdi, 0\n");
            printf("  setne dil\n");
            printf("  movzb rdi, dil\n");
            printf("  or rax, rdi\n");
            break;
        case ND_LOGICAL_AND:
            printf("  cmp rax, 0\n");
            printf("  setne al\n");
            printf("  movzb rax, al\n");
            printf("  cmp rdi, 0\n");
            printf("  setne dil\n");
            printf("  movzb rdi, dil\n");
            printf("  and rax, rdi\n");
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
    printf(".globl %s\n", func->func_type_name->name);
    printf("%s:\n", func->func_type_name->name);

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

// ----------------- グローバル変数 --------------------- //

// グローバル変数の初期化
void gen_initialize_global_var(Node *node) {
    switch (node->kind) {
        case ND_ADD:
            gen_initialize_global_var(node->lhs);
            printf(" + ");
            gen_initialize_global_var(node->rhs);
            return;
        case ND_SUB:
            gen_initialize_global_var(node->lhs);
            printf(" - ");
            gen_initialize_global_var(node->rhs);
            return;
        case ND_MUL:
            gen_initialize_global_var(node->lhs);
            printf(" * ");
            gen_initialize_global_var(node->rhs);
            return;
        case ND_DIV:
            gen_initialize_global_var(node->lhs);
            printf(" / ");
            gen_initialize_global_var(node->rhs);
            return;
        case ND_MOD:
            gen_initialize_global_var(node->lhs);
            printf(" % ");
            gen_initialize_global_var(node->rhs);
            return;
        case ND_NUM:
            printf("%d", node->val);
            return;
        case ND_ADDR:
            printf(".%s", node->lhs->lvar->name);
            return;
        default:
            error("グローバル変数の初期化は配列.文字リテラル.和.差.積.商.数字.アドレスである必要があります");
    }
}

// 1つのグローバル変数の初期化
void gen_initialize_data(LVar *glb_var) {
    if (!glb_var->initial) {
        printf("  .zero %d\n", get_size(glb_var->type));
    }

    // 配列などのオフセット
    int offset = 0;
    int zero;
    
    for (Node *glb_ini = glb_var->initial; glb_ini; glb_ini = glb_ini->next_stmt) {
        if (glb_ini->kind == ND_BLOCK) continue;
        
        // ゼロ埋め 配列のみ使用
        if (zero = get_offset(glb_ini->stmt) - offset) {
            printf("  .zero %d\n", zero);
            offset += zero;
        }

        if (glb_ini->stmt->rhs->kind == ND_STR) {
            printf("  .string \"%s\"\n", glb_ini->stmt->rhs->lvar->str);
            offset += glb_ini->stmt->rhs->lvar->len + 1;
        } else {
            // var(lhs) = assign(rhs)
            switch (get_size(AST_type(0, glb_ini->stmt->rhs))) {
                case 1:
                    printf("  .byte ");
                    offset += 1;
                    break;
                case 4:
                    printf("  .long ");
                    offset += 4;
                    break;
                case 8:
                    printf("  .quad ");
                    offset += 8;
                    break;
                default:
                    error("グローバル変数の初期化時のサイズが処理できません");
            }
            gen_initialize_global_var(glb_ini->stmt->rhs);
            printf("\n");
        }
    }
    
    // ゼロ埋め
    if (glb_var->type->ty == ARRAY && (zero = get_size(glb_var->type) - offset))
        printf("  .zero %d\n", zero);
}

// グローバル変数
void gen_global_var() {
    LVar *glb_var;
    printf("\n.data\n");

    for (glb_var = global_var; glb_var; glb_var = glb_var->next) {
        printf(".%s:\n", glb_var->name);
        gen_initialize_data(glb_var);
    }

    printf("\n");

    for (glb_var = strs; glb_var; glb_var = glb_var->next) {
        printf(".STR%d:\n", glb_var->offset);
        printf("  .string \"%s\"\n", glb_var->str);
    }

    printf("\n.text\n\n");
}