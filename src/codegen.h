#ifndef CODEGEN_H
#define CODEGEN_H
#include "src/parser.h"
#include "src/symbols.h"

#define CODEGEN_FUNCTION_PREFIX "EINAPL_FUNCTION_"
#define CODEGEN_GLOBAL_VARIABLE_ORIGIN_LABEL "EINAPL_GLOBAL_VARIABLES"

#define CODE_HEADER_EXTERNS ".extern putchar\n.extern getchar\n.extern printf\n"

#define CODEGEN_YAP67_SIGNED_LABEL "EINAPL_BUILTIN_YAP67I"
#define CODEGEN_YAP67_UNSIGNED_LABEL "EINAPL_BUILTIN_YAP67U"

// call (NOT JUMP!!) this function with the integer in esi
// xorq %rax, %rax is there because the calling convention uses rax to specify the number of vector registers passed
#define CODE_HEADER_YAP67 \
".data\n\
EINAPL_BUILTIN_YAP67I_STR:\n\
.ascii \"%d\\0\"\n\
EINAPL_BUILTIN_YAP67U_STR:\n\
.ascii \"%u\\0\"\n\
\n\
.text\n"\
CODEGEN_YAP67_SIGNED_LABEL ":\n\
movq $EINAPL_BUILTIN_YAP67I_STR, %rdi\n\
xorq %rax, %rax\n\
jmp printf\n"\
CODEGEN_YAP67_UNSIGNED_LABEL ":\n\
movq $EINAPL_BUILTIN_YAP67U_STR, %rdi\n\
xorq %rax, %rax\n\
jmp printf\n"

#define CODEGEN_LOCKIN67_LABEL "EINAPL_BUILTIN_LOCKIN67"

/*
Derived from this code compiled with GCC -Os:
int32_t read_i32(void)  {
  int_least8_t neg = 1;
  uint_least32_t acum = 0;
  char c = getchar();
  if(c == '-') {
      neg = -1;
      c= getchar();
  }
  while((c >= '0') && (c <= '9')) {
    acum = acum * 10;
    acum = acum + (c - 48);
    c = getchar();
  }
  return acum * (int_least32_t)neg;
}
*/
#define CODE_HEADER_LOCKIN67 \
".text\n"\
CODEGEN_LOCKIN67_LABEL ":\n\
pushq %rbp\n\
pushq %rbx\n\
movb $1, %bl\n\
pushq %rcx\n\
call getchar\n\
cmpb $45, %al\n\
jne " CODEGEN_LOCKIN67_LABEL "_NOT_NEG\n\
call getchar\n\
movb $-1, %bl\n\
" CODEGEN_LOCKIN67_LABEL "_NOT_NEG:\n\
xorl %ebp, %ebp\n\
" CODEGEN_LOCKIN67_LABEL "_LOOP_START:\n\
leal -48(%rax), %edx\n\
cmpb $9, %dl\n\
ja " CODEGEN_LOCKIN67_LABEL "_LOOP_END\n\
imull $10, %ebp, %ebp\n\
movsbl %al, %eax\n\
leal -48(%rbp,%rax), %ebp\n\
call getchar\n\
jmp " CODEGEN_LOCKIN67_LABEL "_LOOP_START\n\
" CODEGEN_LOCKIN67_LABEL "_LOOP_END:\n\
movsbl %bl, %eax\n\
popq %rdx\n\
popq %rbx\n\
imull %ebp, %eax\n\
popq %rbp\n\
ret\n\n"

#define CODE_HEADER_RUNTIME "\
.data\n\
EINAPL_RUNTIME_ERROR_MESSAGE:\n\
.ascii \"ERROR\\n\"\n\
\n\
.text\n\
.global main\n\
main:\n\
call EINAPL_FUNCTION_entry_point\n\
test %eax, %eax\n\
jz EINAPL_RUNTIME_SUCCESS_LABEL\n\
movq $1, %rax\n\
movq $1, %rdi\n\
movq $EINAPL_RUNTIME_ERROR_MESSAGE, %rsi\n\
movq $6, %rdx\n\
syscall\n\
EINAPL_RUNTIME_SUCCESS_LABEL:\n\
ret\n\n"

#define CODE_HEADER CODE_HEADER_EXTERNS CODE_HEADER_LOCKIN67 CODE_HEADER_YAP67 CODE_HEADER_RUNTIME "// END OF CODE HEADER\n\n"




int codegen_gen_top(FILE* const file, ASTNode* const top, struct ASTNodeStack* const node_stack, const struct SymbolTable st);

int codegen_generate(FILE* const file, ASTNode* const top, struct ASTNodeStack* const node_stack, const struct SymbolTable st);

#endif

