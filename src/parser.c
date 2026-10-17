#include "parser.h"
#include "src/lexer.h"
#include "src/symbols.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
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
  };

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
  printf("# parse_integer\n");
  assert(parser != NULL);
  Token t = parser_next(parser);
  ASTNode node;
  if (t.tok_type == TOKT_UINT) {
    node = (ASTNode){.node_type = ASTN_UINT, .u.uinteger = t.data.uinteger};
  } else if (t.tok_type == TOKT_INT) {
    node = (ASTNode){.node_type = ASTN_INT, .u.integer = t.data.integer};
  } else {
    return -1;
  }
  int res = astnode_stack_pusha(parser->stack, node);
  if (res != 0) {
    return -2;
  }
  return astnode_stack_top_idx(parser->stack);
}

int parse_variable(struct Parser* const parser) {
  printf("# parse_variable\n");
  assert(parser != NULL);
  Token t = parser_next(parser);
  ASTNode node;
  if (t.tok_type == TOKT_VAR) {
    node = (ASTNode){.node_type = ASTN_VAR, .u = {.var.name = {.ptr = t.data.string.ptr, .len = t.data.string.length}}};
  } else {
    return -1;
  }
  int res = astnode_stack_pusha(parser->stack, node);
  if (res != 0) {
    return -1;
  }
  return astnode_stack_top_idx(parser->stack);
}

int parse_throw(struct Parser* const parser) {
  printf("# parse_throw\n");
  assert(parser != NULL);
  if (parser_next(parser).tok_type != TOKT_THROW) {
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
      return -1;
  }
  if (value_node_idx == -1 ) { return -1; }
  ASTNode node = {.node_type = ASTN_THROW, .u.throw.value_idx = value_node_idx };
  int res = astnode_stack_pusha(parser->stack, node);
  if (res) { return -1; } 
  return astnode_stack_top_idx(parser->stack);
}

int parse_yap(struct Parser* const parser) {
  printf("# parse_yap\n");
  assert(parser != NULL);
  if (parser_next(parser).tok_type != TOKT_YAP) {
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
      return -1;
  }
  if (value_node_idx == -1) { return -1; }
  ASTNode node = {.node_type = ASTN_YAP, .u.yap.value_idx = value_node_idx };
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
      return -1;
  }
  t = parser_next(parser);
  if (t.tok_type != TOKT_VAR) {
    return -1;
  }

  if (parser_next(parser).tok_type != TOKT_SEMICOLON) {
    return -1;
  }
  variable_table_insert(is_local ? parser->local_variable_table : &parser->global_variable_table, t.data.string.ptr, t.data.string.length, dt);
  int res = astnode_stack_pusha(parser->stack, (ASTNode){.node_type = ASTN_VAR_DECL, .u.var_decl = {.name = { .ptr = t.data.string.ptr, .len = t.data.string.length }, .type = dt }});
  assert(res == 0);
  if (res) { return -1; } 
  return astnode_stack_top_idx(parser->stack);
}

int parse_comparison(struct Parser* const parser);

int parse_primary(struct Parser* const parser) {
  printf("# parse_primary\n");
  switch (parser_peek(parser).tok_type) {
    case TOKT_VAR:
      return parse_variable(parser);
    case TOKT_UINT:
    case TOKT_INT:
      return parse_integer(parser);
    case TOKT_LPAREN:
      ;
      const int node = parse_comparison(parser);
      if (node == -1) { return -1; }
      if (parser_peek(parser).tok_type != TOKT_RPAREN) { return -1; }
      return node;
    default:
      return -1;
  }
}

int parse_unary(struct Parser* const parser) {
  printf("# parse_unary\n");
  if (parser_peek(parser).tok_type == TOKT_NOT) {
    parser_next(parser);

    int const operand_idx = parse_unary(parser);
    if (operand_idx == -1) { return -1; }

    int res = astnode_stack_pusha(parser->stack, (ASTNode){.node_type = ASTN_NOT, .u.uniop = { .operand_idx = operand_idx }});
    if (res) { return -1; } 
    return astnode_stack_top_idx(parser->stack);
  } else {
    return parse_primary(parser);
  }

}


int parse_factor(struct Parser* const parser) {
  printf("# parse_factor\n");
  const int left_term_idx = parse_unary(parser);
  if (left_term_idx == -1) { return -1; }

  ast_node_e node_type;
  switch (parser_peek(parser).tok_type) {
    case TOKT_MUL:
      node_type = ASTN_MUL;
      break;
    case TOKT_DIV:
      node_type = ASTN_DIV;
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

int parse_term(struct Parser* const parser) {
  printf("# parse_term\n");
  const int left_term_idx = parse_factor(parser);
  if (left_term_idx == -1 ) { return -1; }

  ast_node_e node_type;
  switch (parser_peek(parser).tok_type) {
    case TOKT_PLUS:
      node_type = ASTN_ADD;
      break;
    case TOKT_MINUS:
      node_type = ASTN_SUB;
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
  printf("# parse_comparison\n");
  const int left_term_idx = parse_term(parser);
  if (left_term_idx == -1) { return -1; }
  ast_node_e node_type;
  switch (parser_peek(parser).tok_type) {
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

int parse_expression(struct Parser* const parser) {
  printf("# parse_expression\n");
  return parse_comparison(parser);
}

int parse_assignment(struct Parser* const parser) {
  printf("# parse_statement\n");
  assert(parser != NULL);
  Token t = parser_next(parser);
  if (t.tok_type != TOKT_ASSIGN) { return -1; }
  const int expr_node_idx = parse_expression(parser); 
  if (expr_node_idx == -1) { return -1; }

  t = parser_next(parser);
  if (t.tok_type != TOKT_TO) { return -1; }
  const int var_node_idx = parse_variable(parser); 

  int res = astnode_stack_pusha(parser->stack, (ASTNode){.node_type = ASTN_ASSIGNMENT, .u.assign = { .expr_idx = expr_node_idx, .var_idx = var_node_idx }});
  if (res) { return -1; } 
  return astnode_stack_top_idx(parser->stack);
}


int parse_block(struct Parser* const parser);

int parse_if_else(struct Parser* const parser) {
  printf("# parse_if_else\n");
  Token t = parser_next(parser);
  if (t.tok_type != TOKT_IF) { return -1; }
  t = parser_peek(parser);
  if (t.tok_type != TOKT_VAR) { return -1; }
  const int var_idx = parse_variable(parser);
  t = parser_next(parser);
  if (t.tok_type != TOKT_LBRACE) { return -1; }
  const int statements_if_idx = parse_block(parser);
  t = parser_next(parser);
  if (t.tok_type != TOKT_RBRACE) { return -1; }
  int res = astnode_stack_pusha(parser->stack, (ASTNode){.node_type = ASTN_IF_ELSE , .u.if_else = { .statements_if_idx = statements_if_idx, .var_idx = var_idx, .has_else = false}});
  if (res) { return -1; } 

  t = parser_peek(parser);
  const int node_idx = astnode_stack_top_idx(parser->stack);
  assert(node_idx != -1);
  if (t.tok_type == TOKT_ELSE) {
    parser_next(parser); // Consume ELSE
    t = parser_next(parser);
    if (t.tok_type != TOKT_LBRACE) { return -1; }
    const int statements_else_idx = parse_block(parser);
    t = parser_next(parser);
    if (t.tok_type != TOKT_RBRACE) { return -1; }
    t = parser_peek(parser);
    ASTNode* const node = astnode_stack_get_node(parser->stack, node_idx);
    assert(node != NULL);
    node->u.if_else.statements_else_idx = statements_else_idx;
    //node->u.if_else.statements_else = NULL;
    node->u.if_else.has_else = true;
  }
  return node_idx;
}

int parse_statement(struct Parser* const parser) {
  printf("# parse_statement\n");
  assert(parser != NULL);
  int node_idx = -1;
  Token t = parser_peek(parser);
  switch (t.tok_type) {
    case TOKT_THROW:
      node_idx = parse_throw(parser);
      break;
    case TOKT_YAP:
      node_idx = parse_yap(parser);
      break;
    case TOKT_UBYTE:
    case TOKT_SBYTE:
    case TOKT_UQUAD:
    case TOKT_SQUAD:
      // TODO: Add local variable declarations
      return -1;
      break;
    case TOKT_ASSIGN:
      node_idx = parse_assignment(parser);
      break;
    case TOKT_IF:
      node_idx = parse_if_else(parser);
      break;
    default:
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
  printf("# parse_block\n");
  assert(parser != NULL);
  if (parser_peek(parser).tok_type == TOKT_RBRACE) {
    return -1;
  } else if (parser_is_end(parser)) {
    return -1; // ERROR, Parser should not be end without right brace
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


int parse_function(struct Parser* const parser) {
  printf("# parse_function\n");
  assert(parser != NULL);
  Token t = parser_next(parser);
  if (t.tok_type != TOKT_FUN) { return -1; }
  t = parser_next(parser);
  if (t.tok_type != TOKT_FUNCTION) { return -1; }
  ASTNode function = {.node_type = ASTN_FUNCTION, .u.function = { .name = {.ptr = t.data.string.ptr, .len = t.data.string.length }}};

  // TODO: Parse arguments
   
  if (parser_next(parser).tok_type != TOKT_LPAREN) { return -1; }
  if (parser_next(parser).tok_type != TOKT_RPAREN) { return -1; }
  t = parser_next(parser);
  switch (t.tok_type) {
    case TOKT_UBYTE:
      function.u.function.return_type = DT_UBYTE;
      break;
    case TOKT_SBYTE:
      function.u.function.return_type = DT_SBYTE;
      break;
    case TOKT_UQUAD:
      function.u.function.return_type = DT_UQUAD;
      break;
    case TOKT_SQUAD:
      function.u.function.return_type = DT_SQUAD;
      break;
    case TOKT_LBRACE:
      function.u.function.return_type = DT_VOID;
      goto skip_lbrace;
    default:
      return -1;
  }
  if (parser_next(parser).tok_type != TOKT_LBRACE) { return -1; }
  skip_lbrace:
  ;
  struct VariableTable local_variables = variable_table_create();
  parser->local_variable_table = &local_variables;
  const int block_idx = parse_block(parser); 
  // The block can only be empty if it is a void function
  if (block_idx == -1 && function.u.function.return_type != DT_VOID) {
    return -1;
  }
  t = parser_next(parser);
  if (t.tok_type != TOKT_RBRACE) { return -1; }
  function.u.function.first_statement_idx = block_idx;
  function.u.function.local_variables = local_variables;
  parser->local_variable_table = NULL;
  int res = astnode_stack_pusha(parser->stack, function);
  if (res) { return -1; } 
  return astnode_stack_top_idx(parser->stack);
}


int parse_top_level_declaration(struct Parser* const parser) {
  printf("# parse_top_level_declaration\n");
  assert(parser != NULL);
  int node_idx = -1;
  Token t = parser_peek(parser);
  switch (t.tok_type) {
    case TOKT_UBYTE:
    case TOKT_SBYTE:
    case TOKT_UQUAD:
    case TOKT_SQUAD:
      node_idx = parse_variable_declaration(parser, false);
      break;
    case TOKT_FUN:
      node_idx = parse_function(parser);
      break;
    // TODO: Add inclusionn
    default:
      return -1;
  }
  assert(node_idx != -1);
  return node_idx;
}

int parse_top(struct Parser* const parser) {
  printf("# parse_top\n");
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


