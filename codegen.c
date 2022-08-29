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
    fprintf(output_file, "  pop rax\n");
    return;
}

// 関数の引数をpushする
int gen_arg_push(LVar *arg) {
    if (!arg) return 0;
    int argc = gen_arg_push(arg->next);
    // rbpのオフセットに引数をmov
    fprintf(output_file, "  mov rax, rbp\n");
    fprintf(output_file, "  sub rax, %d\n", arg->offset);

    // arg->typeに合わせてレジスタを選択
    switch (arg->type->ty) {
        case CHAR:
            fprintf(output_file, "  mov [rax], %s\n", arg_register[argc][3]);
            break;
        case 100:
            fprintf(output_file, "  mov [rax], %s\n", arg_register[argc][2]);
            break;
        case INT:
            fprintf(output_file, "  mov [rax], %s\n", arg_register[argc][1]);
            break;
        case LONG_LONG_INT:
            fprintf(output_file, "  mov [rax], %s\n", arg_register[argc][0]);
            break;
        case STRUCT:
            error("関数の引数に構造体は未定義です");
        default:
            fprintf(output_file, "  mov [rax], %s\n", arg_register[argc][0]);
            break;
    }
    return argc + 1;
}

// raxにデータを転送する
void mov_rax_data(Node *node) {
    int size = get_size(AST_type(0, node));
    if (size == 1)
        fprintf(output_file, "  movsx eax, BYTE PTR [rax]\n");
    else if (size == 4)
        fprintf(output_file, "  mov eax, [rax]\n");
    else 
        fprintf(output_file, "  mov rax, [rax]\n");

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
                fprintf(output_file, "  lea rax, .%s[rip]\n", node->lvar->name);
            else {
                fprintf(output_file, "  mov rax, rbp\n");
                fprintf(output_file, "  sub rax, %d\n", node->offset);
            }
            fprintf(output_file, "  push rax\n");
            return;
        default:
            error("左辺の型を処理できません %d", node->kind);
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
                fprintf(output_file, "  pop %s\n", arg_register[i][0]);

            // call前にrspが16の倍数である必要がある
            // and rsp, 0xfffff0 で16の倍数にしても
            // スタックに空洞ができると考えるがだいたい上で実装されている
            // そこが分からない
            fprintf(output_file, "  mov rax, rsp\n");
            fprintf(output_file, "  and rsp, 0xfffffffffffffff8\n");
            fprintf(output_file, "  push rax\n");
            fprintf(output_file, "  mov al, 0\n");
            fprintf(output_file, "  call %s\n", node->func_name);
            fprintf(output_file, "  pop rsp\n");
            fprintf(output_file, "  push rax\n");
            return;
        case ND_BLOCK:
            while (node->next_stmt) {
                node = node->next_stmt;

                if (node->stmt->kind == ND_ASM)
                    gen_stmt(node->stmt);
                else
                    gen_pop(node->stmt);
            }
            fprintf(output_file, "  push 0\n");
            return;
        case ND_IF:
            gen_stmt(node->lhs);
            fprintf(output_file, "  pop rax\n");
            fprintf(output_file, "  cmp rax, 0\n");

            if (!node->next_if_else) { // if節単体の場合
                //printf("  push 0\n");
                fprintf(output_file, "  je .Lendif%d\n", node->control);
                if (node->stmt) gen_pop(node->stmt);
            } else if (node->next_if_else->kind == ND_ELSE_IF) { //if節の次がelse ifの場合
                fprintf(output_file, "  je .Lelseif%d_1\n", node->control);
                if (node->stmt) gen_pop(node->stmt);
                fprintf(output_file, "  jmp .Lendif%d\n", node->control);
                gen_stmt(node->next_if_else);
            } else if (node->next_if_else->kind == ND_ELSE) { // if節の次がelse節の場合
                fprintf(output_file, "  je .Lelseif%d\n", node->control);
                if (node->stmt) gen_pop(node->stmt);
                fprintf(output_file, "  jmp .Lendif%d\n", node->control);
                gen_stmt(node->next_if_else);
            }

            fprintf(output_file, ".Lendif%d:\n", node->control);
            fprintf(output_file, "  push 0\n");
            return;
        case ND_ELSE_IF:
            fprintf(output_file, ".Lelseif%d_%d:\n", node->control, node->offset);
            gen_stmt(node->lhs);
            fprintf(output_file, "  pop rax\n");
            fprintf(output_file, "  cmp rax, 0\n");

            if (!node->next_if_else) { // else if節の次はない場合
                //fprintf(output_file, "  push 0\n");
                fprintf(output_file, "  je .Lendif%d\n", node->control);
                if (node->stmt) gen_pop(node->stmt);
            } else if (node->next_if_else->kind == ND_ELSE_IF) { // else if節の次がelse if節の場合
                fprintf(output_file, "  je .Lelseif%d_%d\n", node->control, node->offset+1);
                if (node->stmt) gen_pop(node->stmt);
                fprintf(output_file, "  jmp .Lendif%d\n", node->control);
                gen_stmt(node->next_if_else);
            } else if (node->next_if_else->kind == ND_ELSE) { // else if節の次がelse節の場合
                fprintf(output_file, "  je .Lelseif%d\n", node->control);
                if (node->stmt) gen_pop(node->stmt);
                fprintf(output_file, "  jmp .Lendif%d\n", node->control);
                gen_stmt(node->next_if_else);
            }
            return;
        case ND_ELSE:
            fprintf(output_file, ".Lelseif%d:\n", node->control);
            if (node->stmt) gen_pop(node->stmt);
            return;
        case ND_WHILE:
            fprintf(output_file, ".Lbegin%d:\n", node->control);
            gen_stmt(node->lhs);
            fprintf(output_file, "  pop rax\n");
            fprintf(output_file, "  cmp rax, 0\n");
            fprintf(output_file, "  je  .Lend%d\n", node->control);
            if (node->stmt) gen_pop(node->stmt);
            fprintf(output_file, "  jmp .Lbegin%d\n", node->control);
            fprintf(output_file, ".Lend%d:\n", node->control);
            fprintf(output_file, "  push 0\n"); // mainに戻るとpopされるから適当にpushしておく
            return;
        case ND_FOR:
            // 初期値設定
            if (node->lhs) gen_pop(node->lhs);
            
            fprintf(output_file, ".Lcmp%d:\n", node->control);
            
            // ループ条件(省略なら1)
            if (node->mhs) 
                gen_stmt(node->mhs);
            else
                fprintf(output_file, "  push 1\n");
            fprintf(output_file, "  pop rax\n");
            fprintf(output_file, "  cmp rax, 0\n");
            fprintf(output_file, "  je  .Lend%d\n", node->control);

            // body部分
            if (node->stmt) gen_pop(node->stmt);

            // ループの先頭に戻る時に行う処理
            fprintf(output_file, ".Lbegin%d:\n", node->control);
            if (node->rhs) gen_pop(node->rhs);
            fprintf(output_file, "  jmp .Lcmp%d\n", node->control);

            fprintf(output_file, ".Lend%d:\n", node->control);
            fprintf(output_file, "  push 0\n"); // mainに戻るとpopされるから適当にpushしておく
            return;
        case ND_SWITCH:
            gen_stmt(node->lhs);
            fprintf(output_file, "  pop rdi\n");
            case_node = node;
            while (case_node = case_node->next_if_else) {
                switch (case_node->kind) {
                    case ND_CASE:
                        fprintf(output_file, "  mov rax, %lld\n", case_node->rhs->val);
                        fprintf(output_file, "  cmp rax, rdi\n");
                        fprintf(output_file, "  sete al\n");
                        fprintf(output_file, "  movzb rax, al\n");
                        fprintf(output_file, "  je .Lcase%d_%d\n", node->control, case_node->offset);
                        break;
                    case ND_DEFAULT:
                        fprintf(output_file, "  jmp .Ldefault%d\n", node->control);
                        break;
                    default:
                        error("switch文でcase, default以外を検出しました");
                }
            }
            fprintf(output_file, "  jmp .Lend%d\n", node->control);
            gen_pop(node->stmt);
            fprintf(output_file, ".Lend%d:\n", node->control);
            fprintf(output_file, "  push 0\n");
            return;
        case ND_CASE:
            fprintf(output_file, ".Lcase%d_%d:\n", node->lhs->control, node->offset);
            fprintf(output_file, "  push 0\n");
            return;
        case ND_DEFAULT:
            fprintf(output_file, ".Ldefault%d:\n", node->lhs->control);
            fprintf(output_file, "  push 0\n");
            return;
        case ND_CONTINUE:
            fprintf(output_file, "  jmp .Lbegin%d\n", node->control);
            return;
        case ND_BREAK:
            fprintf(output_file, "  jmp .Lend%d\n", node->control);
            return;
        case ND_ASM:
            fprintf(output_file, "  %s\n", node->asm_str);
            return;
        case ND_RETURN:
            if (node->lhs) {
                gen_stmt(node->lhs);
                if (node->lhs->control)
                    fprintf(output_file, "  cltq\n");
                fprintf(output_file, "  pop rax\n");
            }
            fprintf(output_file, "  leave\n");
            fprintf(output_file, "  ret\n");
            return;
        case ND_NUM:
            fprintf(output_file, "  movq rax, %lld\n", node->val);
            break;
        case ND_CHAR:
            fprintf(output_file, "  mov rax, %d\n", (int)node->val);
            break;
        case ND_STR_PTR:
            fprintf(output_file, "  lea rax, .STR%d[rip]\n", node->lvar->offset);
            fprintf(output_file, "  push rax\n");
            return;
        case ND_LVAR_ADD:
            gen_stmt(node->lhs);
            fprintf(output_file, "  pop rdi\n");
            fprintf(output_file, "  mov rax, rdi\n");
            mov_rax_data(node->lhs);
            fprintf(output_file, "  add [rdi], QWORD PTR 1\n");
            break;
        case ND_LVAR_SUB:
            gen_stmt(node->lhs);
            fprintf(output_file, "  pop rdi\n");
            fprintf(output_file, "  mov rax, rdi\n");
            mov_rax_data(node->lhs);
            fprintf(output_file, "  sub [rdi], QWORD PTR 1\n");
            break;
        case ND_NOT:
            gen_stmt(node->lhs);
            fprintf(output_file, "  pop rax\n");
            fprintf(output_file, "  cmp rax, 0\n");
            fprintf(output_file, "  sete al\n");
            fprintf(output_file, "  movzb rax, al\n");
            break;
        case ND_LOGICAL_ADD:
            gen_stmt(node->lhs);
            fprintf(output_file, "  pop rax\n");
            fprintf(output_file, "  cmp rax, 0\n");
            fprintf(output_file, "  jne .ADD_AND%d\n", node->control);
            gen_stmt(node->rhs);
            fprintf(output_file, "  pop rax\n");
            fprintf(output_file, ".ADD_AND%d:\n", node->control);
            break;
        case ND_LOGICAL_AND:
            gen_stmt(node->lhs);
            fprintf(output_file, "  pop rax\n");
            fprintf(output_file, "  cmp rax, 0\n");
            fprintf(output_file, "  je .ADD_AND%d\n", node->control);
            gen_stmt(node->rhs);
            fprintf(output_file, "  pop rax\n");
            fprintf(output_file, ".ADD_AND%d:\n", node->control);
            break;
        case ND_LVAR: // 変数
            if (node->lvar->glb_var) 
                fprintf(output_file, "  lea rax, .%s[rip]\n", node->lvar->name);
            else {
                fprintf(output_file, "  mov rax, rbp\n");
                fprintf(output_file, "  sub rax, %d\n", node->offset);
            }
            
            // アクセス中または配列は単体だとアドレス
            if (!node->access && node->lvar->type->ty != ARRAY)
                mov_rax_data(node);

            break;
        case ND_NULL:
            fprintf(output_file, "  push 0\n");
            return;
        case ND_ADDR:
            gen_addr(node->lhs);
            return;
        case ND_DEREF:
            gen_stmt(node->lhs);
            fprintf(output_file, "  pop rax\n");
            if (node->access == 2) {}
            else if (node->access) fprintf(output_file, "  mov rax, [rax]\n");
            else mov_rax_data(node);
            break;
        case ND_INDEX:
            gen_stmt(node->lhs);
            if (!node->access &&
                AST_type(0, node->lhs)->ptr_to->ty != ARRAY) {
                // ポインタに添え字がついていて添え字がlastか配列の途中出ない場合
                fprintf(output_file, "  pop rax\n");
                mov_rax_data(node);
                break;
            } else
                return;
        case ND_MEMBAR_ACCESS:
            gen_stmt(node->lhs);
            if (!node->access) {
                fprintf(output_file, "  pop rax\n");
                mov_rax_data(node);
                break;
            }
            else 
                return;
        case ND_MEMBAR:
            fprintf(output_file, "  push %d\n", node->offset);
            return;
        case ND_ASSIGN:
            gen_addr(node->lhs);
            gen_stmt(node->rhs);

            size = get_size(AST_type(0, node->lhs));
            
            fprintf(output_file, "  pop rax\n");
            fprintf(output_file, "  pop rdi\n");

            if (node->cltq)
                fprintf(output_file, "  cltq\n");
                
            // charなら1バイトのみ書き込まれる
            // int ポインタなどは4,8バイト
            if (size == 1)
                fprintf(output_file, "  mov [rdi], al\n");
            else if (size == 4)
                fprintf(output_file, "  mov [rdi], eax\n");
            else 
                fprintf(output_file, "  mov [rdi], rax\n");

            fprintf(output_file, "  push rax\n");
            return;
    }

	switch (node->kind) {
		case ND_ADD:
            gen_stmt(node->lhs);
            gen_stmt(node->rhs);

            fprintf(output_file, "  pop rdi\n");
            fprintf(output_file, "  pop rax\n");
			fprintf(output_file, "  add rax, rdi\n");
			break;
		case ND_SUB:
            gen_stmt(node->lhs);
            gen_stmt(node->rhs);

            fprintf(output_file, "  pop rdi\n");
            fprintf(output_file, "  pop rax\n");
			fprintf(output_file, "  sub rax, rdi\n");
			break;
		case ND_MUL:
            gen_stmt(node->lhs);
            gen_stmt(node->rhs);

            fprintf(output_file, "  pop rdi\n");
            fprintf(output_file, "  pop rax\n");
			fprintf(output_file, "  imul rax, rdi\n");
			break;
		case ND_DIV:
            gen_stmt(node->lhs);
            gen_stmt(node->rhs);

            fprintf(output_file, "  pop rdi\n");
            fprintf(output_file, "  pop rax\n");
			fprintf(output_file, "  cqo\n");
			fprintf(output_file, "  idiv rdi\n");
			break;
        case ND_MOD:            
            gen_stmt(node->lhs);
            gen_stmt(node->rhs);
            
            fprintf(output_file, "  pop rdi\n");
            fprintf(output_file, "  pop rax\n");
            fprintf(output_file, "  cqo\n");
            fprintf(output_file, "  idiv rdi\n");
            fprintf(output_file, "  mov rax, rdx\n");
            break;
        case ND_LEFT_SHIFT:
            gen_stmt(node->lhs);
            gen_stmt(node->rhs);

            fprintf(output_file, "  pop rcx\n");
            fprintf(output_file, "  pop rax\n");
            fprintf(output_file, "  shl rax, cl\n");
            break;
        case ND_RIGHT_SHIFT:
            gen_stmt(node->lhs);
            gen_stmt(node->rhs);

            fprintf(output_file, "  pop rcx\n");
            fprintf(output_file, "  pop rax\n");
            fprintf(output_file, "  shr rax, cl\n");
            break;
        case ND_AND:
            gen_stmt(node->lhs);
            gen_stmt(node->rhs);

            fprintf(output_file, "  pop rdi\n");
            fprintf(output_file, "  pop rax\n");
            fprintf(output_file, "  and rax, rdi\n");
            break;
        case ND_OR:
            gen_stmt(node->lhs);
            gen_stmt(node->rhs);

            fprintf(output_file, "  pop rdi\n");
            fprintf(output_file, "  pop rax\n");
            fprintf(output_file, "  or rax, rdi\n");
            break;
        case ND_XOR:
            gen_stmt(node->lhs);
            gen_stmt(node->rhs);

            fprintf(output_file, "  pop rdi\n");
            fprintf(output_file, "  pop rax\n");
            fprintf(output_file, "  xor rax, rdi\n");
            break;
        case ND_BIT_NOT:
            gen_stmt(node->lhs);

            fprintf(output_file, "  pop rax\n");
            fprintf(output_file, "  not rax\n");
            break;
		case ND_EQ:
            gen_stmt(node->lhs);
            gen_stmt(node->rhs);
            
            fprintf(output_file, "  pop rdi\n");
            fprintf(output_file, "  pop rax\n");
			fprintf(output_file, "  cmp rax, rdi\n");
			fprintf(output_file, "  sete al\n");
			fprintf(output_file, "  movzb rax, al\n");
			break;
		case ND_NE:
            gen_stmt(node->lhs);
            gen_stmt(node->rhs);

            fprintf(output_file, "  pop rdi\n");
            fprintf(output_file, "  pop rax\n");
			fprintf(output_file, "  cmp rax, rdi\n");
			fprintf(output_file, "  setne al\n");
			fprintf(output_file, "  movzb rax, al\n");
			break;
		case ND_LT:
            gen_stmt(node->lhs);
            gen_stmt(node->rhs);

            fprintf(output_file, "  pop rdi\n");
            fprintf(output_file, "  pop rax\n");
			fprintf(output_file, "  cmp rax, rdi\n");
			fprintf(output_file, "  setl al\n");
			fprintf(output_file, "  movzb rax, al\n");
			break;
		case ND_LE:
            gen_stmt(node->lhs);
            gen_stmt(node->rhs);

            fprintf(output_file, "  pop rdi\n");
            fprintf(output_file, "  pop rax\n");
			fprintf(output_file, "  cmp rax, rdi\n");
			fprintf(output_file, "  setle al\n");
			fprintf(output_file, "  movzb rax, al\n");
			break;
	}

    if (node->cast && get_size(node->cast) == 1)
        fprintf(output_file, "  movsx rax, al\n");

    if (node->cltq)
        fprintf(output_file, "  cltq\n");

    fprintf(output_file, "  push rax\n");
	return;
}

// 関数のジェネレータ
void gen_func(Func *func) {
    fprintf(output_file, ".globl %s\n", func->func_type_name->name);
    fprintf(output_file, "%s:\n", func->func_type_name->name);

    // プロローグ
    fprintf(output_file, "  push rbp\n");
    fprintf(output_file, "  mov rbp, rsp\n");
    fprintf(output_file, "  sub rsp, %d\n", locals_var_size(func->locals));

    // 引数をpush
    gen_arg_push(func->arg); 

    gen_stmt(func->stmt);
    fprintf(output_file, "  pop rax\n");

    // エピローグ
    fprintf(output_file, "  leave\n");
    fprintf(output_file, "  ret\n");
}

// ----------------- グローバル変数 --------------------- //

// グローバル変数の初期化
void gen_initialize_global_var(Node *node) {
    switch (node->kind) {
        case ND_ADD:
            gen_initialize_global_var(node->lhs);
            fprintf(output_file, " + ");
            gen_initialize_global_var(node->rhs);
            return;
        case ND_SUB:
            gen_initialize_global_var(node->lhs);
            fprintf(output_file, " - ");
            gen_initialize_global_var(node->rhs);
            return;
        case ND_MUL:
            gen_initialize_global_var(node->lhs);
            fprintf(output_file, " * ");
            gen_initialize_global_var(node->rhs);
            return;
        case ND_DIV:
            gen_initialize_global_var(node->lhs);
            fprintf(output_file, " / ");
            gen_initialize_global_var(node->rhs);
            return;
        case ND_MOD:
            gen_initialize_global_var(node->lhs);
            fprintf(output_file, " %% ");
            gen_initialize_global_var(node->rhs);
            return;
        case ND_NUM:
            fprintf(output_file, "%lld", node->val);
            return;
        case ND_ADDR:
            fprintf(output_file, ".%s", node->lhs->lvar->name);
            return;
        case ND_NULL:
            fprintf(output_file, "0");
            return ;
        default:
            error("グローバル変数の初期化は配列.文字リテラル.和.差.積.商.数字.アドレスである必要があります");
    }
}

// 1つのグローバル変数の初期化
void gen_initialize_data(LVar *glb_var) {
    if (!glb_var->initial) {
        fprintf(output_file, "  .zero %d\n", get_size(glb_var->type));
        return;
    }

    // 配列などのオフセット
    int offset = 0;
    int zero;
    
    for (Node *glb_ini = glb_var->initial; glb_ini; glb_ini = glb_ini->next_stmt) {
        if (glb_ini->kind == ND_BLOCK) continue;
        
        // ゼロ埋め 配列のみ使用
        if (zero = get_offset(glb_ini->stmt) - offset) {
            fprintf(output_file, "  .zero %d\n", zero);
            offset += zero;
        }

        if (glb_ini->stmt->rhs->kind == ND_STR) {
            fprintf(output_file, "  .string \"%s\"\n", glb_ini->stmt->rhs->lvar->str);
            offset += glb_ini->stmt->rhs->lvar->len + 1;
        } else {
            // var(lhs) = assign(rhs)
            //switch (get_size(AST_type(0, glb_ini->stmt->rhs))) {
            switch (get_size(get_type(AST_type(0, glb_ini->stmt->lhs)))) {
                case 1:
                    fprintf(output_file, "  .byte ");
                    offset += 1;
                    break;
                case 4:
                    fprintf(output_file, "  .long ");
                    offset += 4;
                    break;
                case 8:
                    fprintf(output_file, "  .quad ");
                    offset += 8;
                    break;
                default:
                    error("グローバル変数の初期化時のサイズが処理できません");
            }
            gen_initialize_global_var(glb_ini->stmt->rhs);
            fprintf(output_file, "\n");
        }
    }
    
    // ゼロ埋め
    if (glb_var->type->ty == ARRAY && (zero = get_size(glb_var->type) - offset))
        fprintf(output_file, "  .zero %d\n", zero);
}

// グローバル変数
void gen_global_var() {
    LVar *glb_var;
    fprintf(output_file, "\n.data\n");

    for (glb_var = global_var; glb_var; glb_var = glb_var->next) {
        if (glb_var->type->extern_) 
            fprintf(output_file, ".extern %s:\n", glb_var->name);
        else {
            fprintf(output_file, ".globl .%s\n", glb_var->name);
            fprintf(output_file, ".%s:\n", glb_var->name);
        }
        gen_initialize_data(glb_var);
    }

    fprintf(output_file, "\n");

    for (glb_var = strs; glb_var; glb_var = glb_var->next) {
        fprintf(output_file, ".STR%d:\n", glb_var->offset);
        fprintf(output_file, "  .string \"%s\"\n", glb_var->str);
    }

    fprintf(output_file, "\n.text\n\n");
}