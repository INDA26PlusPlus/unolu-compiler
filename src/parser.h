#ifndef PARSER_H
#define PARSER_H
#include "src/lexer.h"
#include "src/symbols.h"

typedef enum {
  ASTN_NONE = 0,
  ASTN_UINT,
  ASTN_INT,
  ASTN_FUNCTION_DECL,
  ASTN_FUNCTION_CALL,
  ASTN_GLOBL_DECL,
  ASTN_THROW,
  ASTN_YAP,
  ASTN_YAP67,
  ASTN_TOP,
  ASTN_TOP_DECL,
  ASTN_STATEMENT,
  ASTN_VAR_DECL,
  ASTN_VAR,
  ASTN_ASSIGNMENT,
  ASTN_NOT,
  ASTN_DIV,
  ASTN_MUL,
  ASTN_ADD,
  ASTN_SUB,
  ASTN_EQUALITY,
  ASTN_LESS,
  ASTN_GREAT,
  ASTN_IF_ELSE,
  ASTN_LOCKIN,
  ASTN_LOCKIN67,
  ASTN_LOOP,
  ASTN_BREAK,
} ast_node_e;



typedef struct ASTNode ASTNode;

struct ASTNode {
  ast_node_e node_type;
  union {
    struct {
      enum datatype_e return_type;
      struct {
        const char* ptr; 
        size_t len;
      } name;
      struct VariableTable local_variables;
      int first_statement_idx;
      // TODO: Arguments, you could implement similarly to a variable table but in the begining of a function you turn the args into local variables 
    } function_decl;
    struct {
      struct {
        const char* ptr; 
        size_t len;
      } name;
      // TODO: Arguments 
    } function_call;
    struct {
      int statements_if_idx;
      int statements_else_idx;
      int expr_idx;
      bool has_else;
    } if_else;
    struct {
      int first_statement_idx;
      int loop_id;
    } loop;
    struct {
      int loop_id;
    } break_stmnt;
    struct {
      enum datatype_e type;
      struct {
        const char* ptr; 
        size_t len;
      } name;
    } var_decl;
    struct {
      enum datatype_e type;
      struct {
        const char* ptr; 
        size_t len;
      } function_name;
      struct {
        const char* ptr; 
        size_t len;
      } global_name;
    } globl_decl;
    struct {
      int expr_idx;
      int var_idx;
    } assign;
    struct {
      struct {
        const char* ptr; 
        size_t len;
      } name;
    } var;
    struct {
      int current_idx;
      int next_idx; // May be empty (-1)
    } top_decl;
    struct {
      int value_idx; // May be empty (-1) if it is just a return
    } throw;
    struct {
      int value_idx; 
    } yap;
    struct {
      int var_idx; 
    } lockin; // Used by both lockin and lockin67
    struct {
      int current_idx;
      int next_idx; // May be empty (-1)
    } statement;
    struct {
      int left_idx;
      int right_idx; 
    } binop;
    struct {
      int operand_idx;
    } uniop;
    uint32_t uinteger;
    int32_t integer;
  } u;
} ;

#define ASTNODE_STACK_INITIAL_ALLOC_POLICY(token_count) (token_count/4+1)
#define ASTNODE_STACK_REALLOC_POLICY(previous_size) (previous_size*2)

struct ASTNodeStack {
  size_t occupied;
  size_t size;
  ASTNode* nodes;
};

struct Parser {
  size_t next_pos;
  struct TokenStack* tok_stack;
  struct ASTNodeStack* stack;
  struct VariableTable global_variable_table;
  struct VariableTable* local_variable_table;
  int current_loop_id;
  int loop_counter;
};

int astnode_stack_create(struct ASTNodeStack* const stack, const size_t initial_size);
int astnode_stack_destroy(struct ASTNodeStack* const stack);
int astnode_stack_pusha(struct ASTNodeStack* const stack, const ASTNode node);
int astnode_stack_pop(struct ASTNodeStack* const stack, ASTNode* const node);
void astnode_stack_removen(struct ASTNodeStack* const stack, const size_t n);
ASTNode* astnode_stack_get_node(struct ASTNodeStack* const stack, const int index);

struct Parser parser_create(struct TokenStack* const token_stack, struct ASTNodeStack* const node_stack);
Token parser_next(struct Parser* const parser);
Token parser_peek(struct Parser* const parser);
bool parser_is_end(struct Parser* const parser);

int parse_top(struct Parser* const parser);

void astnode_stack_print(struct ASTNodeStack* const stack);

#endif
