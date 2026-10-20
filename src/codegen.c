#include "src/codegen.h"
#include "src/parser.h"
#include "src/symbols.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

void codegen_gen_return(FILE* const file) {
  printf("# codegen_gen_return\n");
  fputs("movq -16(%rbp), %rsp\nmovq -8(%rbp), %rbp\nret\n", file);
}

void codegen_gen_local_variable_from_offset(FILE* const file, int offset) {
  assert(offset >=0);
  fprintf(file, "-%d(%%rbp)", offset + CODEGEN_FUNCTION_RESERVED_STACK_SPACE);
}

void codegen_gen_global_variable_from_offset(FILE* const file, int offset) {
  assert(offset >=0);
    fprintf(file, CODEGEN_GLOBAL_VARIABLE_ORIGIN_LABEL "+%d(%%rip)", offset);
}

enum datatype_e codegen_gen_variable(FILE* const file, const char* const name_ptr, const size_t name_len, const struct SymbolTable st) {
  printf("# codegen_gen_variable\n");
  struct Variable* var = variable_table_get_var(&st.global_variables, name_ptr, name_len);
  if (var != NULL) {
      codegen_gen_global_variable_from_offset(file, var->offset);
  } else { 
    assert(st.local_variables != NULL); // ERROR Variable not found in global scope, currently in global scope
    var = variable_table_get_var(st.local_variables, name_ptr, name_len);
    if (var == NULL) {
      return DT_VOID;
    } 
    codegen_gen_local_variable_from_offset(file, var->offset);
  } 
  return var->type;
}

// Assumes either function decl or function call
void codegen_gen_function_identifier(FILE* const file, ASTNode* const function) {
  assert(function->node_type == ASTN_FUNCTION_CALL || function->node_type == ASTN_FUNCTION_DECL);
  fputs(CODEGEN_FUNCTION_PREFIX, file);
  if (function->node_type == ASTN_FUNCTION_CALL) {
    for (size_t i = 0; i<function->u.function_call.name.len; ++i) {
      if (function->u.function_call.name.ptr[i] == '-') {
        fputc('_', file);
      } else {
        fputc(function->u.function_call.name.ptr[i], file);
      }
    }
  } else {
    for (size_t i = 0; i<function->u.function_decl.name.len; ++i) {
      if (function->u.function_decl.name.ptr[i] == '-') {
        fputc('_', file);
      } else {
        fputc(function->u.function_decl.name.ptr[i], file);
      }
    }
  }
}

int codegen_gen_throw(FILE* const file, ASTNode* const throw, struct ASTNodeStack* const node_stack, const struct SymbolTable st) {
  printf("# codegen_gen_throw\n");
  assert(throw != NULL);
  if (throw->u.throw.value_idx != -1) {
    fputs("xorq %rax, %rax\nmovl ", file);
    ASTNode* const value = astnode_stack_get_node(node_stack, throw->u.throw.value_idx);
    assert(value != NULL);
    switch (value->node_type) {
      case ASTN_INT:
        fprintf(file, "$%d", value->u.integer);
        break;
      case ASTN_UINT:
        fprintf(file, "$%u", value->u.uinteger);
        break;
      case ASTN_VAR:
        assert(value != NULL);
        assert(value->u.var.name.ptr != NULL);
        enum datatype_e type = codegen_gen_variable(file, value->u.var.name.ptr, value->u.var.name.len, st);
        if (type == DT_VOID) { return 1; } 
        break;
      default:
        return 2;
    }
  }
  fputs(", %eax\n", file);
  codegen_gen_return(file);
  return 0;
}


int codegen_gen_yap(FILE* const file, ASTNode* const yap, struct ASTNodeStack* const node_stack, const struct SymbolTable st) {
  printf("# codegen_gen_yap\n");
  assert(yap!= NULL);
  ASTNode* const value = astnode_stack_get_node(node_stack, yap->u.yap.value_idx);
  if (value == NULL) {
    return 1;
  } 
  fputs("movl ", file);
  switch (value->node_type) {
    case ASTN_INT:
     fprintf(file, "$%d", value->u.integer);
     break;
    case ASTN_UINT:
     fprintf(file, "$%u", value->u.uinteger);
     break;
   case ASTN_VAR:
     ;
     // Should be fine, internaly the int passed to putchar is converted to a unsigned char
     // If variables are packed then maybe unaligned access might be a problem
     enum datatype_e res = codegen_gen_variable(file, value->u.var.name.ptr, value->u.var.name.len, st);
     if (res == DT_VOID) { return 2; }
    default:
     return 3;
  }
  fputs(", %edi\ncall putchar\n", file);
  return 0;
}

int codegen_gen_yap67(FILE* const file, ASTNode* const yap, struct ASTNodeStack* const node_stack, const struct SymbolTable st) {
  printf("# codegen_gen_yap\n");
  assert(yap!= NULL);
  ASTNode* const value = astnode_stack_get_node(node_stack, yap->u.yap.value_idx);
  if (value == NULL) {
    return 1;
  } 
  enum datatype_e type;
  fputs("movl ", file);
  switch (value->node_type) {
    case ASTN_INT:
     fprintf(file, "$%d", value->u.integer);
     type = DT_SQUAD;
     return 0;
    case ASTN_UINT:
     fprintf(file, "$%u", value->u.uinteger);
     type = DT_UQUAD;
     return 0;
   case ASTN_VAR:
     ;
     type = codegen_gen_variable(file, value->u.var.name.ptr, value->u.var.name.len, st);
     if (type == DT_VOID) { return 2; }
     break;
    default:
     return 3;
  }
  fputs(", %esi\npush %rbp\ncall " , file);
  switch (type) {
    case DT_UBYTE:
    case DT_UQUAD:
      fputs(CODEGEN_YAP67_UNSIGNED_LABEL, file);
      break;
    case DT_SBYTE:
    case DT_SQUAD:
      fputs(CODEGEN_YAP67_SIGNED_LABEL, file);
      break;
    default:
      return 4;
  }
  fputs("\npop %rbp\n", file);
  return 0;
}

int codegen_gen_lockin(FILE* const file, ASTNode* const lockin, struct ASTNodeStack* const node_stack, const struct SymbolTable st) {
  printf("# codegen_gen_lockin\n");
  assert(lockin!= NULL);
  ASTNode* const var_node = astnode_stack_get_node(node_stack, lockin->u.lockin.var_idx);
  if (var_node == NULL) {
    return 1;
  } else {
    if (var_node->node_type != ASTN_VAR) {
      return 2;
    }
    fputs("call getchar\n", file);
    fputs("mov %eax, ", file);
    enum datatype_e type = codegen_gen_variable(file, var_node->u.var.name.ptr, var_node->u.var.name.len, st);
    if (type == DT_VOID) {
      return 3;
    }
    fputc('\n', file);
  }
  return 0;
}

int codegen_gen_lockin67(FILE* const file, ASTNode* const lockin, struct ASTNodeStack* const node_stack, const struct SymbolTable st) {
  printf("# codegen_gen_lockin67\n");
  assert(lockin!= NULL);
  ASTNode* const var_node = astnode_stack_get_node(node_stack, lockin->u.lockin.var_idx);
  if (var_node == NULL) {
    return 1;
  } else {
    if (var_node->node_type != ASTN_VAR) {
      return 2;
    }
    fputs("call "CODEGEN_LOCKIN67_LABEL"\n", file);
    fputs("mov %eax, ", file);
    enum datatype_e type = codegen_gen_variable(file, var_node->u.var.name.ptr, var_node->u.var.name.len, st);
    if (type == DT_VOID) {
      return 3;
    }
    fputc('\n', file);
  }
  return 0;
}

int codegen_gen_function_call(FILE* const file, ASTNode* const function, struct ASTNodeStack* const node_stack, struct SymbolTable st) {
  (void)(node_stack);
  (void)(st);
  printf("# codegen_gen_function_call\n");
  assert(function->node_type == ASTN_FUNCTION_CALL);
  // Ensure 16 byte alignment
  fputs("movq %rsp, -24(%rbp)\nandq $-16, %rsp\ncall ", file);
  codegen_gen_function_identifier(file, function);
  fputs("\nmovq -24(%rbp), %rsp\n", file);
  return 0;
}


int codegen_gen_expression(FILE* const file, ASTNode* const expr, struct ASTNodeStack* const node_stack, const struct SymbolTable st);

int codegen_gen_binop(FILE* const file, ASTNode* const binop, struct ASTNodeStack* const node_stack, const struct SymbolTable st) {
  printf("# codegen_gen_binop\n");
  assert(binop->u.binop.left_idx != -1);
  assert(binop->u.binop.right_idx != -1);
  ASTNode* const left = astnode_stack_get_node(node_stack, binop->u.binop.left_idx);
  assert(left != NULL);
  ASTNode* const right = astnode_stack_get_node(node_stack, binop->u.binop.right_idx);
  assert(right != NULL);
  codegen_gen_expression(file, left, node_stack, st);
  codegen_gen_expression(file, right, node_stack, st);
  fputs("pop %r10\npop %r11\n", file);
  switch (binop->node_type) {
    case ASTN_ADD:
      fputs("addl %r10d, %r11d\n", file);
      break;
    case ASTN_SUB:
      fputs("subl %r10d, %r11d\n", file);
      break;
    // signed and unsigned multiplication behaves the same on a subset of the bits given there is no funny business on the upper bits
    case ASTN_MUL:
      fputs("imull %r10, %r11\n", file);
      break;
    case ASTN_IDIV:
      // Does not preserve eax and edx
      fputs("movl %r10d, %eax\ncdq\n\nidivl %r11d\nmovl %eax, %r11d\n", file);
      break;
    case ASTN_UDIV:
      // Does not preserve eax and edx
      fputs("movl %r10d, %eax\nxorl %edx, %edx\ndivl %r11d\nmovl %eax, %r11d\n", file);
      break;
    case ASTN_MOD:
      // Does not preserve eax and edx
      fputs("movl %r10d, %eax\nxorl %edx, %edx\ndivl %r11d\nmovl %edx, %r11d\n", file);
      break;
    case ASTN_LOGOR:
      fputs("orl %r11d, %r10d\nmovl $0, %r11d\nsetne %r11b\n", file);
      break;
    case ASTN_LOGAND:
      fputs("testl %r10d, %r10d\nmovl $0, %r10d\nsetne %r10b\ntestl %r11d, %r11d\nmovl $0, %r11d\nsetne %r11b\nandb %r11b, %r10b\n", file);
      break;
    case ASTN_BITAND:
      fputs("andl %r10d, %r11d\n", file);
      break;
    case ASTN_BITOR:
      fputs("orl %r10d, %r11d\n", file);
      break;
    case ASTN_BITXOR:
      fputs("xorl %r10d, %r11d\n", file);
      break;
    case ASTN_EQUALITY:
      // mov does not affect the flags
      fputs("cmp %r10d, %r11d\nmovl $0, %r11d\nsete %r11b\n", file);
      break;
    default:
      fprintf(stderr, "Codegen: Unexpected node type (%d) for binop\n", binop->node_type);
      return 2002;
  }
  fputs("push %r11\n", file);
  return 0;
}

int codegen_gen_uniop(FILE* const file, ASTNode* const uniop, struct ASTNodeStack* const node_stack, const struct SymbolTable st) {
  printf("# codegen_gen_uniop\n");
  assert(uniop->u.uniop.operand_idx != -1);
  ASTNode* const operand = astnode_stack_get_node(node_stack, uniop->u.uniop.operand_idx);
  assert(operand != NULL);
  codegen_gen_expression(file, operand, node_stack, st);
  fputs("pop %r11\n", file);
  switch (uniop->node_type) {
    case ASTN_LOGNOT:
      fputs("notl %r11d\nmovl $0, %r11d\nsetne %r11b\n", file);
      break;
    case ASTN_BITNOT:
      fputs("notl %r11d\n", file);
      break;
    default:
      fprintf(stderr, "Codegen: Unexpected node type (%d) for uninop\n", uniop->node_type);
      return 2003;
  }
  fputs("push %r11\n", file);
  return 0;
}


int codegen_gen_expression(FILE* const file, ASTNode* const expr, struct ASTNodeStack* const node_stack, const struct SymbolTable st) {
  printf("# codegen_gen_expression\n");
  assert(expr != NULL);
  int res;
  switch (expr->node_type) {
    case ASTN_FUNCTION_CALL:
      codegen_gen_function_call(file, expr, node_stack, st);
      fputs("push %rax\n", file);
      return 0;
    case ASTN_INT:
      fprintf(file, "push $%d\n", expr->u.integer);
      return 0;
    case ASTN_UINT:
      fprintf(file, "push $%u\n", expr->u.uinteger);
      return 0;
    case ASTN_VAR:
      ;
      fputs("push ", file);
      assert(expr->u.var.name.ptr != NULL);
      codegen_gen_variable(file, expr->u.var.name.ptr, expr->u.var.name.len, st);
      fputc('\n', file);
      return 0;
    case ASTN_ADD:
    case ASTN_SUB:
    case ASTN_MUL:
    case ASTN_IDIV:
    case ASTN_UDIV:
    case ASTN_MOD:
    case ASTN_LOGAND:
    case ASTN_LOGOR:
    case ASTN_BITAND:
    case ASTN_BITOR:
    case ASTN_BITXOR:
    case ASTN_EQUALITY:
      res = codegen_gen_binop(file, expr, node_stack, st);
      return res;
    case ASTN_LOGNOT:
    case ASTN_BITNOT:
      res = codegen_gen_uniop(file, expr, node_stack, st);
      return res;
    default:
      fprintf(stderr, "Codegen: Unexpected node type (%d) for expression\n", expr->node_type);
      return 2002;
  }
  return 0;
}

int codegen_gen_assignment(FILE* const file, ASTNode* const assign, struct ASTNodeStack* const node_stack, const struct SymbolTable st) {
  printf("# codegen_gen_assignment\n");
  assert(assign!= NULL);
  if (assign->node_type != ASTN_ASSIGNMENT) {
    return 1;
  }
  ASTNode* const expr = astnode_stack_get_node(node_stack, assign->u.assign.expr_idx);
  assert(expr != NULL);
  int res = codegen_gen_expression(file, expr, node_stack, st);
  if (res != 0) {
    return res;
  }

  fputs("pop %r10\n", file);
  fputs("movl %r10d, ", file);

  ASTNode* const var = astnode_stack_get_node(node_stack, assign->u.assign.var_idx);
  assert(var != NULL);
  res = codegen_gen_variable(file, var->u.var.name.ptr, var->u.var.name.len, st);
  fputc('\n', file);
  return 0;
}

int codegen_gen_block(FILE* const file, ASTNode* const block, struct ASTNodeStack* const node_stack, const struct SymbolTable st);

int codegen_gen_if_else(FILE* const file, ASTNode* const node, struct ASTNodeStack* const node_stack, struct SymbolTable st) {
  printf("# codegen_gen_if_else\n");
  assert(node->node_type == ASTN_IF_ELSE);
  assert(node->u.if_else.statements_if_idx != -1);
  assert(node->u.if_else.expr_idx != -1);
  ASTNode* const expr = astnode_stack_get_node(node_stack, node->u.if_else.expr_idx);
  int res = codegen_gen_expression(file, expr, node_stack, st);
  if (res != 0) { return res; }
  fputs("pop %r10\ntestl %r10d, %r10d\n", file);
  int if_id = (*st.if_id_counter)++;
  fprintf(file, "jz IF%d_ELSE\n", if_id);

  ASTNode* const statements_if = astnode_stack_get_node(node_stack, node->u.if_else.statements_if_idx);
  assert(statements_if != NULL);

  res = codegen_gen_block(file, statements_if, node_stack, st);
  if (res != 0) { return res; }
  if (node->u.if_else.has_else) {
    fprintf(file, "jmp IF%d_END\n", if_id);
  }
  fprintf(file, "IF%d_ELSE:\n", if_id);
  if (node->u.if_else.has_else) {
    ASTNode* const statements_else = astnode_stack_get_node(node_stack, node->u.if_else.statements_else_idx);
    assert(statements_else != NULL);
    res = codegen_gen_block(file, statements_else, node_stack, st);
    if (res != 0) { return res; }
  }
  fprintf(file, "IF%d_END:\n", if_id);
  return 0;
}

int codegen_gen_loop(FILE* const file, ASTNode* const node, struct ASTNodeStack* const node_stack, struct SymbolTable st) {
  printf("# codegen_gen_loop\n");
  assert(node->node_type == ASTN_LOOP);
  assert(node->u.loop.first_statement_idx != -1);
  assert(node->u.loop.loop_id != -1);
  fprintf(file, "LOOP%d_START:\n", node->u.loop.loop_id);
  ASTNode* const block = astnode_stack_get_node(node_stack, node->u.loop.first_statement_idx);
  assert(block != NULL);
  int res = codegen_gen_block(file, block, node_stack, st);
  if (res) { return res; }
  fprintf(file, "jmp LOOP%d_START\nLOOP%d_END:\n", node->u.loop.loop_id, node->u.loop.loop_id);
  return 0;
}

int codegen_gen_break(FILE* const file, ASTNode* const node, struct ASTNodeStack* const node_stack, struct SymbolTable st) {
  (void)(node_stack);
  (void)(st);
  printf("# codegen_gen_break\n");
  assert(node->node_type == ASTN_BREAK);
  assert(node->u.break_stmnt.loop_id != -1);
  fprintf(file, "jmp LOOP%d_END\n", node->u.break_stmnt.loop_id);
  return 0;
}


int codegen_gen_statement(FILE* const file, ASTNode* const node, struct ASTNodeStack* const node_stack, const struct SymbolTable st) {
  printf("# codegen_gen_statement\n");
  assert(node != NULL);
  switch (node->node_type) {
    case ASTN_FUNCTION_CALL:
      return codegen_gen_function_call(file, node, node_stack, st);
    case ASTN_THROW:
      return codegen_gen_throw(file, node, node_stack, st);
    case ASTN_YAP:
      return codegen_gen_yap(file, node, node_stack, st);
    case ASTN_YAP67:
      return codegen_gen_yap67(file, node, node_stack, st);
    case ASTN_LOCKIN:
      return codegen_gen_lockin(file, node, node_stack, st);
    case ASTN_LOCKIN67:
      return codegen_gen_lockin67(file, node, node_stack, st);
    case ASTN_ASSIGNMENT:
      return codegen_gen_assignment(file, node, node_stack, st);
    case ASTN_VAR_DECL:
      printf("# local var decl (nothing)\n");
      return 0;
    case ASTN_LOOP:
      return codegen_gen_loop(file, node, node_stack, st);
    case ASTN_BREAK:
      return codegen_gen_break(file, node, node_stack, st);
    case ASTN_IF_ELSE:
      return codegen_gen_if_else(file, node, node_stack, st);
    default:
      fprintf(stderr, "Codegen: Unexpected node type (%d) for statement\n", node->node_type);
      return 1;
  }
}

int codegen_gen_block(FILE* const file, ASTNode* const block, struct ASTNodeStack* const node_stack, const struct SymbolTable st) {
  printf("# codegen_gen_block\n");
  if (block == NULL) {
    return 0;
  }

  ASTNode* const current = astnode_stack_get_node(node_stack, block->u.statement.current_idx);
  assert(current!= NULL);
  int res = codegen_gen_statement(file, current, node_stack, st);
  if (res != 0) { return res; }
  if (block->u.statement.next_idx != -1) {
    ASTNode* const next = astnode_stack_get_node(node_stack, block->u.statement.next_idx);
    assert(next!= NULL);
    res = codegen_gen_block(file, next, node_stack, st);
    if (res != 0) { return res; }
  }
  return 0;
}



int codegen_gen_function_decl(FILE* const file, ASTNode* const function, struct ASTNodeStack* const node_stack, struct SymbolTable st) {
  printf("# codegen_gen_function_decl\n");
  assert(function != NULL);
  fputs( ".text\n", file);
  codegen_gen_function_identifier(file, function);
  fputs(":\nmovq %rbp, -8(%rsp)\nmovq %rsp, -16(%rsp)\nmovq %rsp, %rbp\n", file);
  fprintf(file, "subq $%d, %%rsp\n", CODEGEN_FUNCTION_RESERVED_STACK_SPACE+variable_table_get_total_size(&function->u.function_decl.local_variables));

  ASTNode* const first_statement = astnode_stack_get_node(node_stack, function->u.function_decl.first_statement_idx);
  assert(first_statement != NULL);
  st.local_variables = &function->u.function_decl.local_variables;
  int res = codegen_gen_block(file, first_statement, node_stack, st);
  st.local_variables = NULL;

  if (res != 0) { return res; }
  if (function->u.function_decl.return_type == DT_VOID) {
    codegen_gen_return(file);
  }
  fputc('\n', file);
  return 0;
}

int codegen_gen_globalization_decl(FILE* const file, ASTNode* const globalization_decl, struct ASTNodeStack* const node_stack, struct SymbolTable st) {
  (void)node_stack;
  (void)st;
  assert(globalization_decl != NULL);
  if (globalization_decl->node_type != ASTN_GLOBL_DECL) { return 1; }
  assert (globalization_decl->u.globl_decl.function_name.ptr != NULL);
  assert (globalization_decl->u.globl_decl.function_name.len != 0);
  assert (globalization_decl->u.globl_decl.global_name.ptr != NULL);
  assert (globalization_decl->u.globl_decl.global_name.len != 0);
  fprintf(file, ".text\n.global %.*s\n", (int)globalization_decl->u.globl_decl.global_name.len, globalization_decl->u.globl_decl.global_name.ptr);
  fprintf(file, "%.*s:\n", (int)globalization_decl->u.globl_decl.global_name.len, globalization_decl->u.globl_decl.global_name.ptr);
  fprintf(file, "jmp "CODEGEN_FUNCTION_PREFIX"%.*s\n\n", (int)globalization_decl->u.globl_decl.function_name.len, globalization_decl->u.globl_decl.function_name.ptr);
  return 0;
}

int codegen_gen_top_level_declaration(FILE* const file, ASTNode* const top_decl, struct ASTNodeStack* const node_stack, const struct SymbolTable st) {
  printf("# codegen_gen_top_level_declaration\n");
  if (top_decl == NULL) {
    return 0;
  }
  int res;
  switch (top_decl->node_type) {
      case ASTN_FUNCTION_DECL:
        ;
        res = codegen_gen_function_decl(file, top_decl, node_stack, st);
        return res;
      case ASTN_VAR_DECL:
        printf("# global var decl (nothing)\n");
        return 0;
      case ASTN_GLOBL_DECL:
        res = codegen_gen_globalization_decl(file, top_decl, node_stack, st);
        return res;
      default:
        fprintf(stderr, "Codegen: Unexpected node type (%d) for top level declaration\n", top_decl->node_type);
        return 1;
    }
}

int codegen_gen_top(FILE* const file, ASTNode* const top, struct ASTNodeStack* const node_stack, const struct SymbolTable st) {
  printf("# codegen_gen_top\n");
  if (top == NULL) {
    return 0;
  }

  ASTNode* const current = astnode_stack_get_node(node_stack, top->u.top_decl.current_idx);
  assert(current != NULL);
  int res = codegen_gen_top_level_declaration(file, current, node_stack, st);
  if (res != 0) { return res; }
  if (top->u.top_decl.next_idx != -1) {
    ASTNode* const next = astnode_stack_get_node(node_stack, top->u.top_decl.next_idx);
    assert(next != NULL);
    res = codegen_gen_top(file, next, node_stack, st);
    if (res != 0) { return res; }
  }
  return 0;
}

int codegen_generate(FILE* const file, ASTNode* const top, struct ASTNodeStack* const node_stack, struct SymbolTable st) {
  if (top == NULL) {
    return 0;
  }
  int if_id_counter = 0;
  st.if_id_counter = &if_id_counter;
  fputs(CODE_HEADER, file);
  fputs(".data\n"CODEGEN_GLOBAL_VARIABLE_ORIGIN_LABEL":\n", file);
  int total_size = variable_table_get_total_size(&st.global_variables);
  fprintf(file, ".fill %d, 1, 0\n\n", total_size);
  int res = codegen_gen_top(file, top, node_stack, st);
  if (res != 0) { return res; }
  return 0;
}

