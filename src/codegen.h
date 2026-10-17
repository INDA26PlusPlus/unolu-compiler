#ifndef CODEGEN_H
#define CODEGEN_H
#include "src/parser.h"
#include "src/symbols.h"


#define CODE_HEADER ".extern putchar\n\n.data\nRUNTIME_ERROR_MESSAGE:\n.ascii \"ERROR\\n\"\n\n.text\n.global main\nmain:\ncall FUNCTION_entry_point\n\
test %eax, %eax\njz RUNTIME_SUCCESS_LABEL\nmovq $1, %rax\nmovq $1, %rdi\nmovq $RUNTIME_ERROR_MESSAGE, %rsi\nmovq $6, %rdx\nsyscall\nRUNTIME_SUCCESS_LABEL:\nret\n\n"





int codegen_gen_top(FILE* const file, ASTNode* const top, struct ASTNodeStack* const node_stack, const struct SymbolTable st);

int codegen_generate(FILE* const file, ASTNode* const top, struct ASTNodeStack* const node_stack, const struct SymbolTable st);

#endif

