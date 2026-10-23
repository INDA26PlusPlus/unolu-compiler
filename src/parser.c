#include "parser.h"
#include "src/lexer.h"
#include "src/symbols.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// In the future you probably want to remove the freeing of the stack upon parse failure and just exit.

int astnode_stack_create(struct ASTNodeStack* const stack, const size_t initial_size) {
  if (stack == NULL) { return 1; }
  if (initial_size == 0) { return 2; }
  void* const alloc = malloc(initial_size*sizeof(ASTNode));
  if (alloc == NULL) { return 3; }
  stack->size = initial_size;
  stack->occupied = 0;
  stack->nodes = alloc;
  return 0;
}

int astnode_stack_destroy(struct ASTNodeStack* const stack) {
  if (stack->nodes == NULL) { return 1; }
  for (size_t i = 0; i<stack->occupied; ++i) {
    switch (stack->nodes[i].node_type) {
      case ASTN_FUNCTION_DECL:
        variable_table_destroy(stack->nodes[i].u.function_decl.local_variables);
        break;
      default:
        break;
    }
  }
  free(stack->nodes);
  stack->nodes = NULL;
  stack->size = 0;
  stack->occupied = 0;
  return 0;
}


// May try to realloc
int astnode_stack_pusha(struct ASTNodeStack* const stack, const ASTNode node) {
  assert(stack->occupied <= stack->size); // Debug check, ensure the stack is never overflowed
  if (stack->nodes == NULL) { return 1; }

  if (stack->occupied == stack->size) {
    const size_t new_size = ASTNODE_STACK_REALLOC_POLICY(stack->size);
    //ASTNode* const new_alloc = realloc(stack->nodes, new_size);
    ASTNode* const new_alloc = (ASTNode*)malloc(new_size*sizeof(ASTNode));
    if (new_alloc == NULL) { return 2; }

    // TODO: Make Realloc
    memcpy(new_alloc, stack->nodes, stack->size*sizeof(ASTNode));
    free(stack->nodes);

    stack->nodes = new_alloc;
    stack->size = new_size;
  }
  stack->nodes[stack->occupied++] = node;
  return 0;
}

// Pop from stack to node
int astnode_stack_pop(struct ASTNodeStack* const stack, ASTNode* const node) {
  if (stack->nodes == NULL) { return 1; }
  if (stack->occupied == 0) { return 2; }
  *node = stack->nodes[--stack->occupied];
  return 0;
}

ASTNode* astnode_stack_get_node(struct ASTNodeStack* const stack, const int index) {
  assert(index >= 0);
  assert(index < stack->occupied);
  if (index >= stack->occupied || index < 0) {
    return NULL;
  }
  return &stack->nodes[index];
}

// -1 if empty
int astnode_stack_top_idx(struct ASTNodeStack* const stack) {
  return stack->occupied-1;
}

void astnode_stack_removen(struct ASTNodeStack* const stack, const size_t n) {
  assert(stack->nodes != NULL);
  assert(((int64_t)stack->occupied)-((int64_t)n) >= 0);
  stack->occupied -= n;
}


struct Parser parser_create(struct TokenStack* const token_stack, struct ASTNodeStack* const node_stack) {
  assert(token_stack != NULL);
  assert(node_stack != NULL);
  return (struct Parser){
    .tok_stack = token_stack,
    .stack = node_stack,
    .next_pos = 0,
    .global_variable_table = variable_table_create(),
    .local_variable_table = NULL,
    .current_loop_id = -1,
    .loop_counter = 0,
  };

}

void parser_destroy(struct Parser* const parser) {
  parser->loop_counter = 0;
  parser->next_pos = 0;
  parser->current_loop_id = 0;
  variable_table_destroy(parser->global_variable_table);
  parser->local_variable_table = NULL;
}

Token parser_next(struct Parser* const parser) {
  assert(parser != NULL);
  assert(parser->tok_stack != NULL);
  assert(parser->tok_stack->tokens != NULL);
  assert(parser->next_pos != parser->tok_stack->occupied);
  return parser->tok_stack->tokens[parser->next_pos++];
}

Token parser_peek(struct Parser* const parser) {
  assert(parser != NULL);
  assert(parser->tok_stack != NULL);
  assert(parser->tok_stack->tokens != NULL);
  assert(parser->next_pos != parser->tok_stack->occupied);
  return parser->tok_stack->tokens[parser->next_pos];
}

bool parser_is_end(struct Parser* const parser) {
  assert(parser != NULL);
  return parser->next_pos == parser->tok_stack->occupied;
}




int parse_integer(struct Parser* const parser) {
  assert(parser != NULL);
  Token t = parser_next(parser);
  ASTNode node;
  if (t.tok_type == TOKT_UINT) {
    node = (ASTNode){.node_type = ASTN_UINT, .u.uinteger = t.data.uinteger};
  } else if (t.tok_type == TOKT_INT) {
    node = (ASTNode){.node_type = ASTN_INT, .u.integer = t.data.integer};
  } else {
    fprintf(stderr, "Expected integer @%d:%d\n", t.line, t.col);
    return -1;
  }
  int res = astnode_stack_pusha(parser->stack, node);
  if (res != 0) {
    return -2;
  }
  return astnode_stack_top_idx(parser->stack);
}

int parse_variable(struct Parser* const parser) {
  assert(parser != NULL);
  Token t = parser_next(parser);
  ASTNode node;
  if (t.tok_type == TOKT_VAR) {
    node = (ASTNode){.node_type = ASTN_VAR, .u = {.var.name = {.ptr = t.data.string.ptr, .len = t.data.string.length}}};
  } else {
    fprintf(stderr, "Expected variable identifier @%d:%d\n", t.line, t.col);
    return -1;
  }
  int res = astnode_stack_pusha(parser->stack, node);
  if (res != 0) {
    return -1;
  }
  return astnode_stack_top_idx(parser->stack);
}

int parse_throw(struct Parser* const parser) {
  assert(parser != NULL);
  Token t = parser_next(parser);
  if (t.tok_type != TOKT_THROW) {
    fprintf(stderr, "Expected keyword \"Throw\" @%d:%d\n", t.line, t.col);
    return -1;
  }
  int value_node_idx = -1;
  t = parser_peek(parser);
  switch (t.tok_type) {
    case TOKT_UINT:
    case TOKT_INT:
      value_node_idx = parse_integer(parser);
      break;
    case TOKT_VAR:
      value_node_idx = parse_variable(parser);
      break;
    default:
      fprintf(stderr, "Expected variable identifier or integer after Throw @%d:%d\n", t.line, t.col);
      return -1;
  }
  if (value_node_idx == -1 ) { return -1; }
  ASTNode node = {.node_type = ASTN_THROW, .u.throw.value_idx = value_node_idx };
  int res = astnode_stack_pusha(parser->stack, node);
  if (res) { return -1; } 
  return astnode_stack_top_idx(parser->stack);
}

int parse_yap(struct Parser* const parser) {
  assert(parser != NULL);
  Token t = parser_next(parser);
  enum tok_type_e tokt = t.tok_type;
  if (tokt != TOKT_YAP && tokt != TOKT_YAP67) {
    fprintf(stderr, "Expected keyword \"Yap\" or \"Yap67\" @%d:%d\n", t.line, t.col);
    return -1;
  }
  int value_node_idx = -1;
  switch (parser_peek(parser).tok_type) {
    case TOKT_UINT:
    case TOKT_INT:
      value_node_idx = parse_integer(parser);
      break;
    case TOKT_VAR:
      value_node_idx = parse_variable(parser);
      break;
    default:
      fprintf(stderr, "Expected variable identifier or integer after Yap/Yap67 @%d:%d\n", t.line, t.col);
      return -1;
  }
  if (value_node_idx == -1) { return -1; }
  ASTNode node = {.node_type = tokt == TOKT_YAP ? ASTN_YAP : ASTN_YAP67, .u.yap.value_idx = value_node_idx };
  int res = astnode_stack_pusha(parser->stack, node);
  if (res) { return -1; } 
  return astnode_stack_top_idx(parser->stack);
}

int parse_lockin(struct Parser* const parser) {
  assert(parser != NULL);
  Token t = parser_next(parser);
  enum tok_type_e tokt = t.tok_type;
  if (tokt != TOKT_LOCKIN && tokt != TOKT_LOCKIN67) {
    fprintf(stderr, "Expected keyword \"LockIn\" or \"LockIn67\" @%d:%d\n", t.line, t.col);
    return -1;
  }
  t = parser_peek(parser);
  if (t.tok_type != TOKT_VAR) {
    fprintf(stderr, "Expected variable identifier after LockIn/LockIn67 @%d:%d\n", t.line, t.col);
    return -1;
  }
  int var_idx = parse_variable(parser);
  if (var_idx == -1) { return -1; }
  ASTNode node = {.node_type = tokt == TOKT_LOCKIN ? ASTN_LOCKIN : ASTN_LOCKIN67, .u.lockin.var_idx = var_idx };
  int res = astnode_stack_pusha(parser->stack, node);
  if (res) { return -1; } 
  return astnode_stack_top_idx(parser->stack);
}


int parse_variable_declaration(struct Parser* const parser, const bool is_local) {
  assert(parser != NULL);
  Token t = parser_next(parser);
  enum datatype_e dt;
  switch (t.tok_type) {
    case TOKT_UBYTE:
      dt = DT_UBYTE;
      break;
    case TOKT_SBYTE:
      dt = DT_SBYTE;
      break;
    case TOKT_UQUAD:
      dt = DT_UQUAD;
      break;
    case TOKT_SQUAD:
      dt = DT_SQUAD;
      break;
    default:
      fprintf(stderr, "Expected type @%d:%d\n", t.line, t.col);
      return -1;
  }
  t = parser_next(parser);
  if (t.tok_type != TOKT_VAR) {
    fprintf(stderr, "Expected variable identifier after type @%d:%d\n", t.line, t.col);
    return -1;
  }

  if (!is_local && parser_next(parser).tok_type != TOKT_SEMICOLON) {
    fprintf(stderr, "Expected semicolon @%d:%d\n", t.line, t.col);
    return -1;
  }
  variable_table_insert(is_local ? parser->local_variable_table : &parser->global_variable_table, t.data.string.ptr, t.data.string.length, dt);
  int res = astnode_stack_pusha(parser->stack, (ASTNode){.node_type = ASTN_VAR_DECL, .u.var_decl = {.name = { .ptr = t.data.string.ptr, .len = t.data.string.length }, .type = dt }});
  assert(res == 0);
  if (res) { return -1; } 
  return astnode_stack_top_idx(parser->stack);
}

int parse_single_argument(struct Parser* const parser, size_t* const stack_usage, bool count_stack) {
  assert(parser != NULL);
  int node_idx = -1;
  Token t = parser_peek(parser);
  switch (t.tok_type) {
    // Pushing to the stack in 64 bit mode decrements the stack by 8
    case TOKT_VAR:
      node_idx = parse_variable(parser);
      if (count_stack) {
        *stack_usage += 8;
      }
      break;
    case TOKT_UINT:
    case TOKT_INT:
      node_idx = parse_integer(parser);
      if (count_stack) {
        *stack_usage += 8;
      }
      break;
    default:
      fprintf(stderr, "Variable or integer as argument @%d:%d\n", t.line, t.col);
      return -1;
  }
  assert(node_idx != -1);
  return node_idx;
}

int parse_arguments(struct Parser* parser, size_t* const stack_usage, int arg_depth) {
  assert(parser != NULL);
  if (parser_peek(parser).tok_type == TOKT_RPAREN) {
    return -1;
  } else if (parser_is_end(parser)) {
    return -1; // ERROR, Parser should not be end here
  } else  {
    const int current_idx = parse_single_argument(parser, stack_usage, arg_depth >= 6); 
    assert(current_idx != -1);
    if (current_idx == -1) {
      return -1; // ERROR, could not parse argument
    }
    const ASTNode argument = {
      .node_type = ASTN_ARGUMENT,
      .u.argument = {
        .current_idx = current_idx,
        .next_idx = parse_arguments(parser, stack_usage, arg_depth+1)
      },
    };
    int res = astnode_stack_pusha(parser->stack, argument);
    assert(res == 0);
    const int argument_idx = astnode_stack_top_idx(parser->stack);
    assert(argument_idx != -1);
    return argument_idx;
  }
}

int parse_function_call(struct Parser* const parser) {
  assert(parser != NULL);
  Token t = parser_next(parser);
  if (t.tok_type != TOKT_FUNCTION) {
    fprintf(stderr, "Expected function identifier @%d:%d\n", t.line, t.col);
    return -1;
  }
  ASTNode function_call = {.node_type = ASTN_FUNCTION_CALL, .u.function_call = { .name = { .ptr = t.data.string.ptr, .len = t.data.string.length }, .has_arguments = false, .first_argument_idx = -1, .stack_usage = 0 } };
  if (parser_next(parser).tok_type != TOKT_LPAREN) {
    fprintf(stderr, "Expected left parentheses after function identifier @%d:%d\n", t.line, t.col);
    return -1;
  }
  if (parser_peek(parser).tok_type != TOKT_RPAREN) {
    size_t stack_usage = 0;
    int first_argument_idx = parse_arguments(parser, &stack_usage, 0);
    assert(first_argument_idx != -1);
    function_call.u.function_call.has_arguments = true;
    function_call.u.function_call.first_argument_idx = first_argument_idx;
    function_call.u.function_call.stack_usage = stack_usage;
  }
  if (parser_next(parser).tok_type != TOKT_RPAREN) {
    fprintf(stderr, "Expected right parentheses @%d:%d\n", t.line, t.col);
    return -1;
  }
  int res = astnode_stack_pusha(parser->stack, function_call);
  if (res) { return -1; } 
  return astnode_stack_top_idx(parser->stack);
}

int parse_comparison(struct Parser* const parser);

int parse_primary(struct Parser* const parser) {
  Token t = parser_peek(parser);
  switch (t.tok_type) {
    case TOKT_FUNCTION:
      return parse_function_call(parser);
    case TOKT_VAR:
      return parse_variable(parser);
    case TOKT_UINT:
    case TOKT_INT:
      return parse_integer(parser);
    case TOKT_LPAREN:
      parser_next(parser);
      ;
      const int node = parse_comparison(parser);
      if (node == -1) { return -1; }
      if (parser_peek(parser).tok_type != TOKT_RPAREN) { return -1; }
      parser_next(parser);
      return node;
    default:
      fprintf(stderr, "Expected primary: variable, integer, function call or parentheses @%d:%d\n", t.line, t.col);
      return -1;
  }
}

int parse_unary(struct Parser* const parser) {
  int res;
  ast_node_e node_type;
  switch (parser_peek(parser).tok_type) {
    case TOKT_LOGNOT:
      node_type = ASTN_LOGNOT;
      break;
    case TOKT_BITNOT:
      node_type = ASTN_BITNOT;
      break;
    default:
      return parse_primary(parser);
  }
  parser_next(parser);
  int const operand_idx = parse_unary(parser);
  if (operand_idx == -1) { return -1; }
  res = astnode_stack_pusha(parser->stack, (ASTNode){.node_type = node_type, .u.uniop = { .operand_idx = operand_idx }});
  if (res) { return -1; } 
  return astnode_stack_top_idx(parser->stack);
}


int parse_factor(struct Parser* const parser) {
  const int left_term_idx = parse_unary(parser);
  if (left_term_idx == -1) { return -1; }

  ast_node_e node_type;
  switch (parser_peek(parser).tok_type) {
    case TOKT_MUL:
      node_type = ASTN_MUL;
      break;
    case TOKT_IDIV:
      node_type = ASTN_IDIV;
      break;
    case TOKT_UDIV:
      node_type = ASTN_UDIV;
      break;
    default:
      return left_term_idx;
  }
  parser_next(parser);
  const int right_term_idx = parse_factor(parser);
  if (right_term_idx == -1) { return -1; }
  int res = astnode_stack_pusha(parser->stack, (ASTNode){.node_type = node_type, .u.binop = { .left_idx = left_term_idx, .right_idx = right_term_idx }});
  if (res) { return -1; } 
  return astnode_stack_top_idx(parser->stack);
}

int parse_bitwise(struct Parser* const parser) {
  const int left_term_idx = parse_factor(parser);
  if (left_term_idx == -1 ) { return -1; }

  ast_node_e node_type;
  switch (parser_peek(parser).tok_type) {
    case TOKT_BITAND:
      node_type = ASTN_BITAND;
      break;
    case TOKT_BITOR:
      node_type = ASTN_BITOR;
      break;
    case TOKT_BITXOR:
      node_type = ASTN_BITXOR;
      break;
    default:
      return left_term_idx;
  }
  parser_next(parser);
  const int right_term_idx = parse_bitwise(parser);
  if (right_term_idx == -1) { return -1; }
  int res = astnode_stack_pusha(parser->stack, (ASTNode){.node_type = node_type, .u.binop = { .left_idx = left_term_idx, .right_idx = right_term_idx }});
  if (res) { return -1; } 
  return astnode_stack_top_idx(parser->stack);
}

int parse_term(struct Parser* const parser) {
  const int left_term_idx = parse_bitwise(parser);
  if (left_term_idx == -1 ) { return -1; }

  ast_node_e node_type;
  switch (parser_peek(parser).tok_type) {
    case TOKT_PLUS:
      node_type = ASTN_ADD;
      break;
    case TOKT_MINUS:
      node_type = ASTN_SUB;
      break;
    case TOKT_MOD:
      node_type = ASTN_MOD;
      break;
    default:
      return left_term_idx;
  }
  parser_next(parser);
  const int right_term_idx = parse_term(parser);
  if (right_term_idx == -1) { return -1; }
  int res = astnode_stack_pusha(parser->stack, (ASTNode){.node_type = node_type, .u.binop = { .left_idx = left_term_idx, .right_idx = right_term_idx }});
  if (res) { return -1; } 
  return astnode_stack_top_idx(parser->stack);
}

int parse_comparison(struct Parser* const parser) {
  const int left_term_idx = parse_term(parser);
  if (left_term_idx == -1) { return -1; }
  ast_node_e node_type;
  Token t = parser_peek(parser);
  switch (t.tok_type) {
    case TOKT_EQ:
      node_type = ASTN_EQUALITY;
      break;
    case TOKT_LESS:
      node_type = ASTN_LESS;
      break;
    case TOKT_GREAT:
      node_type = ASTN_GREAT;
      break;
    default:
      return left_term_idx;
  }
  parser_next(parser);
  const int right_term_idx = parse_comparison(parser);
  if (right_term_idx == -1) { return -1; }
  int res = astnode_stack_pusha(parser->stack, (ASTNode){.node_type = node_type, .u.binop = { .left_idx = left_term_idx, .right_idx = right_term_idx }});
  if (res) { return -1; } 
  return astnode_stack_top_idx(parser->stack);
}

int parse_logical(struct Parser* const parser) {
  const int left_term_idx = parse_comparison(parser);
  if (left_term_idx == -1) { return -1; }
  ast_node_e node_type;
  Token t = parser_peek(parser);
  switch (t.tok_type) {
    case TOKT_LOGOR:
      node_type = ASTN_LOGOR;
      break;
    case TOKT_LOGAND:
      node_type = ASTN_LOGAND;
      break;
    default:
      return left_term_idx;
  }
  parser_next(parser);
  const int right_term_idx = parse_logical(parser);
  if (right_term_idx == -1) { return -1; }
  int res = astnode_stack_pusha(parser->stack, (ASTNode){.node_type = node_type, .u.binop = { .left_idx = left_term_idx, .right_idx = right_term_idx }});
  if (res) { return -1; } 
  return astnode_stack_top_idx(parser->stack);
}


int parse_expression(struct Parser* const parser) {
  return parse_logical(parser);
}

int parse_assignment(struct Parser* const parser) {
  assert(parser != NULL);
  Token t = parser_next(parser);
  if (t.tok_type != TOKT_ASSIGN) {
    fprintf(stderr, "Expected keyword \"Assign\" @%d:%d\n", t.line, t.col);
    return -1;
  }
  const int expr_node_idx = parse_expression(parser); 
  if (expr_node_idx == -1) { return -1; }

  t = parser_next(parser);
  if (t.tok_type != TOKT_TO) {
    fprintf(stderr, "Expected keyword \"To\" @%d:%d\n", t.line, t.col);
    return -1;
  }
  const int var_node_idx = parse_variable(parser); 

  int res = astnode_stack_pusha(parser->stack, (ASTNode){.node_type = ASTN_ASSIGNMENT, .u.assign = { .expr_idx = expr_node_idx, .var_idx = var_node_idx }});
  if (res) { return -1; } 
  return astnode_stack_top_idx(parser->stack);
}


int parse_block(struct Parser* const parser);

int parse_if_else(struct Parser* const parser) {
  Token t = parser_next(parser);
  if (t.tok_type != TOKT_IF) {
    fprintf(stderr, "Expected keyword \"If\" @%d:%d\n", t.line, t.col);
    return -1;
  }
  t = parser_peek(parser);
  const int expr_idx = parse_expression(parser);
  if (expr_idx == -1) { return -1; }
  t = parser_next(parser);
  if (t.tok_type != TOKT_LBRACE) {
    fprintf(stderr, "Expected left brace after keyword \"If\" @%d:%d\n", t.line, t.col);
    return -1;
  }
  const int statements_if_idx = parse_block(parser);
  t = parser_next(parser);
  if (t.tok_type != TOKT_RBRACE) {
    fprintf(stderr, "Expected right brace @%d:%d\n", t.line, t.col);
    return -1;
  }
  int res = astnode_stack_pusha(parser->stack, (ASTNode){.node_type = ASTN_IF_ELSE , .u.if_else = { .statements_if_idx = statements_if_idx, .expr_idx = expr_idx, .has_else = false}});
  if (res) { return -1; } 

  t = parser_peek(parser);
  const int node_idx = astnode_stack_top_idx(parser->stack);
  assert(node_idx != -1);
  if (t.tok_type == TOKT_ELSE) {
    parser_next(parser); // Consume ELSE
    t = parser_next(parser);
    if (t.tok_type != TOKT_LBRACE) {
      fprintf(stderr, "Expected left brace after keyword \"Else\" @%d:%d\n", t.line, t.col);
      return -1;
    }
    const int statements_else_idx = parse_block(parser);
    t = parser_next(parser);
    if (t.tok_type != TOKT_RBRACE) {
      fprintf(stderr, "Expected right brace @%d:%d\n", t.line, t.col);
      return -1;
    }
    t = parser_peek(parser);
    ASTNode* const node = astnode_stack_get_node(parser->stack, node_idx);
    assert(node != NULL);
    node->u.if_else.statements_else_idx = statements_else_idx;
    //node->u.if_else.statements_else = NULL;
    node->u.if_else.has_else = true;
  }
  return node_idx;
}

int parse_loop(struct Parser* const parser) {
  Token t = parser_next(parser);
  if (t.tok_type != TOKT_LOOP) {
    fprintf(stderr, "Expected keyword \"Loop\" @%d:%d\n", t.line, t.col);
    return -1;
  }
  t = parser_next(parser);
  if (t.tok_type != TOKT_LBRACE) {
    fprintf(stderr, "Expected left brace after keyword \"Loop\" @%d:%d\n", t.line, t.col);
    return -1;
  }
  int previous_loop_id = parser->current_loop_id;
  parser->current_loop_id = parser->loop_counter++;
  const int first_statement_idx = parse_block(parser);
  t = parser_next(parser);
  if (t.tok_type != TOKT_RBRACE) {
    fprintf(stderr, "Expected right brace @%d:%d\n", t.line, t.col);
    return -1;
  }
  int res = astnode_stack_pusha(parser->stack, (ASTNode){.node_type = ASTN_LOOP, .u.loop = {.first_statement_idx = first_statement_idx, .loop_id = parser->current_loop_id}});
  parser->current_loop_id = previous_loop_id;
  if (res) { return -1; } 
  const int node_idx = astnode_stack_top_idx(parser->stack);
  assert(node_idx != -1);
  return node_idx;
}

int parse_break(struct Parser* const parser) {
  assert(parser != NULL);
  Token t = parser_next(parser);
  if (t.tok_type != TOKT_BREAK) {
    fprintf(stderr, "Expected keyword \"Break\" @%d:%d\n", t.line, t.col);
    return -1;
  }
  assert(parser->current_loop_id != -1);
  ASTNode node = {.node_type = ASTN_BREAK, .u.break_stmnt.loop_id = parser->current_loop_id };
  int res = astnode_stack_pusha(parser->stack, node);
  if (res) { return -1; } 
  return astnode_stack_top_idx(parser->stack);
}


int parse_statement(struct Parser* const parser) {
  assert(parser != NULL);
  int node_idx = -1;
  Token t = parser_peek(parser);
  switch (t.tok_type) {
    case TOKT_THROW:
      node_idx = parse_throw(parser);
      break;
    case TOKT_YAP:
    case TOKT_YAP67:
      node_idx = parse_yap(parser);
      break;
    case TOKT_LOCKIN:
    case TOKT_LOCKIN67:
      node_idx = parse_lockin(parser);
      break;
    case TOKT_UBYTE:
    case TOKT_SBYTE:
    case TOKT_UQUAD:
    case TOKT_SQUAD:
      node_idx = parse_variable_declaration(parser, true);
      break;
    case TOKT_ASSIGN:
      node_idx = parse_assignment(parser);
      break;
    case TOKT_IF:
      node_idx = parse_if_else(parser);
      break;
    case TOKT_LOOP:
      node_idx = parse_loop(parser);
      break;
    case TOKT_BREAK:
      node_idx = parse_break(parser);
      break;
    case TOKT_FUNCTION:
      node_idx = parse_function_call(parser);
      break;
    default:
      fprintf(stderr, "Invalid statement @%d:%d\n", t.line, t.col);
      return -1;
  }
  t = parser_next(parser);
  if (t.tok_type != TOKT_SEMICOLON) {
    node_idx = -1;
  }
  assert(node_idx != -1);
  return node_idx;
}

int parse_block(struct Parser* const parser) {
  assert(parser != NULL);
  if (parser_peek(parser).tok_type == TOKT_RBRACE) {
    return -1;
  } else if (parser_is_end(parser)) {
    fprintf(stderr, "Expected right brace before EOF\n");
    return -1; 
  } else  {
    const int current_idx = parse_statement(parser); 
    assert(current_idx != -1);
    if (current_idx == -1) {
      return -1; // ERROR, could not parse statement
    }
    const ASTNode statement = {
      .node_type = ASTN_STATEMENT,
      .u.statement = {
        .current_idx = current_idx,
        .next_idx = parse_block(parser)
      },
    };

    int res = astnode_stack_pusha(parser->stack, statement);
    assert(res == 0);
    const int statement_idx = astnode_stack_top_idx(parser->stack);
    assert(statement_idx != -1);
    return statement_idx;
  }
}

// Returns 0 on success
int parse_parameters(struct Parser* const parser, struct VariableTable* parameters) {
  assert(parser != NULL);
  assert(parameters != NULL);
  Token t = parser_peek(parser);
  while (t.tok_type != TOKT_RPAREN) {
    parser_next(parser);
    enum datatype_e dt;
    switch (t.tok_type) {
      case TOKT_UBYTE:
        dt = DT_UBYTE;
        break;
      case TOKT_SBYTE:
        dt = DT_SBYTE;
        break;
      case TOKT_UQUAD:
        dt = DT_UQUAD;
        break;
      case TOKT_SQUAD:
        dt = DT_SQUAD;
        break;
      default:
        fprintf(stderr, "Expected type @%d:%d\n", t.line, t.col);
        return -1;
    }
    t = parser_next(parser);
    if (t.tok_type != TOKT_VAR) {
      fprintf(stderr, "Expected variable identifier after type @%d:%d\n", t.line, t.col);
      return -1;
    }
    variable_table_insert(parameters, t.data.string.ptr, t.data.string.length, dt);
    t = parser_peek(parser);
  }
  return 0;
}

int parse_function_declaration(struct Parser* const parser) {
  assert(parser != NULL);
  Token t = parser_next(parser);
  if (t.tok_type != TOKT_FUN) {
    fprintf(stderr, "Expected keyword \"Fun\" @%d:%d\n", t.line, t.col);
    return -1;
  }
  t = parser_next(parser);
  if (t.tok_type != TOKT_FUNCTION) {
    fprintf(stderr, "Expected function identifier after keyword \"Fun\" @%d:%d\n", t.line, t.col);
    return -1;
  }
  ASTNode function = {.node_type = ASTN_FUNCTION_DECL, .u.function_decl = { .name = {.ptr = t.data.string.ptr, .len = t.data.string.length }, .has_parameters = false}};
  if (parser_next(parser).tok_type != TOKT_LPAREN) { return -1; }
  if (parser_peek(parser).tok_type != TOKT_RPAREN) {
    struct VariableTable parameters = variable_table_create();
    int res = parse_parameters(parser, &parameters);
    if (res) {
      goto fail;
    } 
    function.u.function_decl.has_parameters = true;
    function.u.function_decl.parameters = parameters;
  }

  if (parser_next(parser).tok_type != TOKT_RPAREN) {
    goto fail;
  }
  t = parser_next(parser);
  switch (t.tok_type) {
    case TOKT_UBYTE:
      function.u.function_decl.return_type = DT_UBYTE;
      break;
    case TOKT_SBYTE:
      function.u.function_decl.return_type = DT_SBYTE;
      break;
    case TOKT_UQUAD:
      function.u.function_decl.return_type = DT_UQUAD;
      break;
    case TOKT_SQUAD:
      function.u.function_decl.return_type = DT_SQUAD;
      break;
    case TOKT_LBRACE:
      function.u.function_decl.return_type = DT_VOID;
      goto skip_lbrace;
    default:
      fprintf(stderr, "Expected type or left brace @%d:%d\n", t.line, t.col);
      goto fail;
  }
  if (parser_next(parser).tok_type != TOKT_LBRACE) {
    goto fail;
  }
  skip_lbrace:
  ;
  struct VariableTable local_variables = variable_table_create();
  parser->local_variable_table = &local_variables;
  const int block_idx = parse_block(parser); 
  // The block can only be empty if it is a void function
  if (block_idx == -1 && function.u.function_decl.return_type != DT_VOID) {
    fprintf(stderr, "Expected atleast 1 statement in non-void function @%d:%d\n", t.line, t.col);
    goto fail_with_locals;
  }
  t = parser_next(parser);
  if (t.tok_type != TOKT_RBRACE) {
    fprintf(stderr, "Expected right brace @%d:%d\n", t.line, t.col);
    goto fail_with_locals;
  }
  function.u.function_decl.first_statement_idx = block_idx;
  function.u.function_decl.local_variables = local_variables;
  parser->local_variable_table = NULL;
  int res = astnode_stack_pusha(parser->stack, function);
  if (res) {
    goto fail_with_locals;
  } 
  return astnode_stack_top_idx(parser->stack);

  fail_with_locals:
  variable_table_destroy(local_variables);
  fail:
  variable_table_destroy(function.u.function_decl.parameters);
  return -1;
}

int parse_localization_declaration(struct Parser* const parser) {
  assert(parser != NULL);
  Token t = parser_next(parser);
  if (t.tok_type != TOKT_LOCALIZE) {
    fprintf(stderr, "Expected keyword \"Localize\" @%d:%d\n", t.line, t.col);
    return -1;
  }
  t = parser_next(parser);
  if (t.tok_type != TOKT_GLOBAL_IDENTIFIER) {
    fprintf(stderr, "Expected global identifier after keyword \"Localize\" @%d:%d\n", t.line, t.col);
    return -1;
  }
  ASTNode localization_decl = {.node_type = ASTN_LOCAL_DECL, .u.local_decl = { .global_name = {.ptr = t.data.string.ptr, .len = t.data.string.length }}};
  t = parser_next(parser);
  if (t.tok_type != TOKT_FUNCTION) {
    if (t.tok_type == TOKT_MINUS) {
      // Helpful hint
      fprintf(stderr, "Expected function identifier after global identifier (global identifiers may only contain letters) @%d:%d\n", t.line, t.col);
    } else {
      fprintf(stderr, "Expected function identifier after global identifier @%d:%d\n", t.line, t.col);
    }
    return -1;
  }
  localization_decl.u.local_decl.function_name.ptr = t.data.string.ptr;
  localization_decl.u.local_decl.function_name.len = t.data.string.length;
  if (parser_next(parser).tok_type != TOKT_SEMICOLON) { return -1; }
  int res = astnode_stack_pusha(parser->stack, localization_decl);
  if (res) { return -1; } 
  return astnode_stack_top_idx(parser->stack);
}

int parse_globalization_declaration(struct Parser* const parser) {
  assert(parser != NULL);
  Token t = parser_next(parser);
  if (t.tok_type != TOKT_GLOBALIZE) {
    fprintf(stderr, "Expected keyword \"Globalize\" @%d:%d\n", t.line, t.col);
    return -1;
  }
  t = parser_next(parser);
  if (t.tok_type != TOKT_FUNCTION) {
    fprintf(stderr, "Expected function identifier after keyword \"Globalize\" @%d:%d\n", t.line, t.col);
    return -1;
  }
  ASTNode globalization_decl = {.node_type = ASTN_GLOBL_DECL, .u.globl_decl = { .function_name = {.ptr = t.data.string.ptr, .len = t.data.string.length }}};
  t = parser_next(parser);
  if (t.tok_type != TOKT_GLOBAL_IDENTIFIER) {
    fprintf(stderr, "Expected global identifier after function identifier @%d:%d\n", t.line, t.col);
    return -1;
  }
  globalization_decl.u.globl_decl.global_name.ptr = t.data.string.ptr;
  globalization_decl.u.globl_decl.global_name.len = t.data.string.length;
  if (parser_next(parser).tok_type != TOKT_SEMICOLON) { return -1; }
  int res = astnode_stack_pusha(parser->stack, globalization_decl);
  if (res) { return -1; } 
  return astnode_stack_top_idx(parser->stack);
}

int parse_top_level_declaration(struct Parser* const parser) {
  assert(parser != NULL);
  int node_idx = -1;
  Token t = parser_peek(parser);
  switch (t.tok_type) {
    case TOKT_GLOBALIZE:
      node_idx = parse_globalization_declaration(parser);
      break;
    case TOKT_LOCALIZE:
      node_idx = parse_localization_declaration(parser);
      break;
    case TOKT_UBYTE:
    case TOKT_SBYTE:
    case TOKT_UQUAD:
    case TOKT_SQUAD:
      node_idx = parse_variable_declaration(parser, false);
      break;
    case TOKT_FUN:
      node_idx = parse_function_declaration(parser);
      break;
    default:
      fprintf(stderr, "Invalid top level declaration @%d:%d\n", t.line, t.col);
      return -1;
  }
  assert(node_idx != -1);
  return node_idx;
}

int parse_top(struct Parser* const parser) {
  assert(parser != NULL);
  if (parser_is_end(parser)) {
    return -1; // End of file, no more declarations 
  } else  {
    const int current_idx = parse_top_level_declaration(parser); 
    assert(current_idx != -1);
    if (current_idx == -1) {
      return -1; // ERROR, could not parse statement
    }
    ASTNode decl = {
      .node_type = ASTN_TOP_DECL,
      .u.top_decl = {
        .current_idx = current_idx,
        .next_idx = parse_top(parser)
      },
    };

    int res = astnode_stack_pusha(parser->stack, decl);
    assert(res == 0);

    const int decl_idx = astnode_stack_top_idx(parser->stack);
    assert(decl_idx != -1);

    return decl_idx;
  }
}

void astnode_stack_print(struct ASTNodeStack* const stack) {
  for (size_t i = 0; i<stack->occupied; ++i) {
    printf("%d ", stack->nodes[i].node_type);
  }
}


