#ifndef LEXER_H
#define LEXER_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

/*
lexer conversions
char -> int
*/

enum tok_type_e {
  TOKT_ILLEGAL = 0,
  TOKT_LBRACE,
  TOKT_RBRACE,
  TOKT_LPAREN,
  TOKT_RPAREN,
  TOKT_COMMA,
  TOKT_SEMICOLON,
  TOKT_IF,
  TOKT_ELSE,
  TOKT_VAR,
  TOKT_GLOBALIZE,
  TOKT_GLOBAL_IDENTIFIER,
  TOKT_LOCALIZE,
  TOKT_LOOP,
  TOKT_INT,
  TOKT_UINT,
  TOKT_UBYTE,
  TOKT_SBYTE,
  TOKT_UQUAD,
  TOKT_SQUAD,
  TOKT_PATH,
  TOKT_YAP,
  TOKT_YAP67,
  TOKT_LOCKIN,
  TOKT_LOCKIN67,
  TOKT_PLUS,
  TOKT_MINUS,
  TOKT_MOD,
  TOKT_MUL,
  TOKT_IDIV,
  TOKT_UDIV,
  TOKT_LOGNOT,
  TOKT_LOGOR,
  TOKT_LOGAND,
  TOKT_BITNOT,
  TOKT_BITOR,
  TOKT_BITXOR,
  TOKT_BITAND,
  TOKT_EQ,
  TOKT_GREAT,
  TOKT_LESS,
  TOKT_FUN,
  TOKT_BREAK,
  TOKT_THROW,
  TOKT_FUNCTION,
  TOKT_ASSIGN,
  TOKT_TO,
};

typedef struct {
  union {
    uint32_t uinteger;
    int32_t integer;
    struct {
      const char* ptr;
      size_t length;
    } string;
  } data;
  enum tok_type_e tok_type;
  uint32_t line;
  uint16_t col;
} Token;

struct TokenStack {
  size_t occupied;
  size_t size;
  Token* tokens;
};


struct Lexer {
  const char* string;
  size_t string_size;
  size_t current_pos;
  struct TokenStack stack;
  size_t line_start;
  uint32_t line_count;
};


#define TOKEN_STACK_INITIAL_ALLOC_POLICY(str_sz) (str_sz/4+1)
#define TOKEN_STACK_REALLOC_POLICY(previous_size) (previous_size*2)

int token_stack_create(struct TokenStack* const stack, const size_t initial_size); 
int token_stack_destroy(struct TokenStack* const stack);
int token_stack_pusha(struct TokenStack* const stack, const Token token);
int token_stack_pop(struct TokenStack* const stack, Token* const token);

int lexer_create(struct Lexer* const lexer, const char* const string, const size_t string_size);
int lexer_destroy(struct Lexer* const lexer);
// Includes debug assertion
#define lexer_current_character(lexer) (lexer->string[lexer->current_pos])
char lexer_peek(struct Lexer* const lexer, const size_t n);
char lexer_next(struct Lexer* const lexer);
const char* lexer_capture(struct Lexer* const lexer);
Token lexer_tokenize(struct Lexer* const lexer);
int lexer_tokenize_all(struct Lexer* const lexer, struct TokenStack* const stack);

/*
struct Lexer {
  FILE* file;
  size_t read_pos; // read_pos may be at most be file_size (EOF)
  size_t file_size;
  char current_character; 
};
#define lexer_is_eof(lexer) ((lexer)->current_character == EOF)
struct Lexer create_lexer(FILE* const file);
char lexer_peek(struct Lexer* const lexer);
char lexer_forward(struct Lexer* const lexer);
void lexer_skip_whitespace(struct Lexer* const lexer);
// Capture NON-null terminated string, forwards read_pos to char capture_size+1 
char* lexer_capture_non_nt_string(struct Lexer *const lexer, const size_t capture_size, char* const buf);
struct TokenArray lexer_tokenize(struct Lexer* const lexer);
*/


#endif
