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

int codegen_gen_variable(FILE* const file, const char* const name_ptr, const size_t name_len, const struct SymbolTable st) {
  printf("# codegen_gen_variable\n");
  int offset = variable_table_get_offset(&st.global_variables, name_ptr, name_len);
  if (offset >= 0) {
    fprintf(file, "GLOBAL_VARIABLES+%d(%%rip)", offset);
  } else {
    // TODO: Add local variables
    return 10;
  } 
  return 0;
}

int codegen_gen_throw(FILE* const file, ASTNode* const throw, struct ASTNodeStack* const node_stack, const struct SymbolTable st) {
  printf("# codegen_gen_throw\n");
  assert(throw != NULL);
  if (throw->u.throw.value_idx != -1) {
    fputs("movl ", file);
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
        int res = codegen_gen_variable(file, value->u.var.name.ptr, value->u.var.name.len, st);
        if (res != 0) { return res; } 
        break;
      default:
        return 1;
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
  } else {
    switch (value->node_type) {
      case ASTN_INT:
      case ASTN_UINT:
       fprintf(file, "movl $%d", value->u.integer);
       break;
     case ASTN_VAR:
       ;
       // Check global variables first
       struct Variable* var = variable_table_get_var(&st.global_variables, value->u.var.name.ptr, value->u.var.name.len);
       if (var != NULL) {
         fputs("mov", file);
         switch (var->type) {
           case DT_UBYTE:
           case DT_SBYTE:
             fputs("zb", file);
             // Intentionally left out break
          case DT_UQUAD:
          case DT_SQUAD:
             fputc('l', file);
             break;
          default:
            return 2;
         }
         fprintf(file, " GLOBAL_VARIABLES+%d(%%rip)", var->offset);
       } else {
          // TODO: Add local variables
         return 1000;
       } 

       break;
      default:
       return 3;
    }
    fputs(", %edi\ncall putchar\n", file);
  }
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
    case ASTN_MUL:
      fputs("mull %r10d, %r11d\n", file);
      break;
    default:
      return 2002;
  }
  fputs("push %r11\n", file);
  return 0;
}

int codegen_gen_expression(FILE* const file, ASTNode* const expr, struct ASTNodeStack* const node_stack, const struct SymbolTable st) {
  printf("# codegen_gen_expression\n");
  assert(expr != NULL);
  switch (expr->node_type) {
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
      ;
      int res = codegen_gen_binop(file, expr, node_stack, st);
      return res;
    default:
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
  assert(node->u.if_else.var_idx != -1);
  ASTNode* const var = astnode_stack_get_node(node_stack, node->u.if_else.var_idx);
  assert(var != NULL);
  assert(var->u.var.name.ptr != NULL);

  fputs("movl ", file);
  int res = codegen_gen_variable(file, var->u.var.name.ptr, var->u.var.name.len, st);
  if (res != 0) { return res; }
  fputs(", %r10d\ntestl %r10d, %r10d\n", file);
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

int codegen_gen_statement(FILE* const file, ASTNode* const node, struct ASTNodeStack* const node_stack, const struct SymbolTable st) {
  printf("# codegen_gen_statement\n");
  assert(node != NULL);
  switch (node->node_type) {
    case ASTN_THROW:
      return codegen_gen_throw(file, node, node_stack, st);
    case ASTN_YAP:
      return codegen_gen_yap(file, node, node_stack, st);
    case ASTN_ASSIGNMENT:
      return codegen_gen_assignment(file, node, node_stack, st);
    case ASTN_IF_ELSE:
      return codegen_gen_if_else(file, node, node_stack, st);
    default:
      printf("# UNEXPECTED STATEMENT %d\n", node->node_type);
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

void codegen_print_function(FILE* const file, ASTNode* const function) {
  fputs("FUNCTION_", file);
  for (size_t i = 0; i<function->u.function.name.len; ++i) {
    if (function->u.function.name.ptr[i] == '-') {
      fputc('_', file);
    } else {
      fputc(function->u.function.name.ptr[i], file);
    }
  }
}


int codegen_gen_function(FILE* const file, ASTNode* const function, struct ASTNodeStack* const node_stack, const struct SymbolTable st) {
  printf("# codegen_gen_function\n");
  assert(function != NULL);
  fputs( ".text\n.global ", file);
  codegen_print_function(file, function);
  fputc('\n', file);
  codegen_print_function(file, function);
  fputs(":\nmovq %rbp, -8(%rsp)\nmovq %rsp, -16(%rsp)\nmovq %rsp, %rbp\n", file);
  fprintf(file, "subq $%d, %%rsp\n", variable_table_get_max_offset(&function->u.function.local_variables));

  ASTNode* const first_statement = astnode_stack_get_node(node_stack, function->u.function.first_statement_idx);
  assert(first_statement != NULL);
  int res = codegen_gen_block(file, first_statement, node_stack, st);

  if (res != 0) { return res; }
  if (function->u.function.return_type == DT_VOID) {
    codegen_gen_return(file);
  }
  return 0;
}

int codegen_gen_top_level_declaration(FILE* const file, ASTNode* const top_decl, struct ASTNodeStack* const node_stack, const struct SymbolTable st) {
  printf("# codegen_gen_top_level_declaration\n");
  if (top_decl == NULL) {
    return 0;
  }
  printf("TOP DECL\n");
  switch (top_decl->node_type) {
      case ASTN_FUNCTION:
        ;
        int res = codegen_gen_function(file, top_decl, node_stack, st);
        return res;
      case ASTN_VAR_DECL:
        printf("# var decl (nothing happends)\n");
        return 0;
      default:
        printf("# other %d\n", top_decl->node_type);
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
  fputs(".data\nGLOBAL_VARIABLES:\n", file);
  int total_size = variable_table_get_total_size(&st.global_variables);
  fprintf(file, ".fill %d, 1, 0\n", total_size);
  int res = codegen_gen_top(file, top, node_stack, st);
  if (res != 0) { return res; }
  return 0;
}

