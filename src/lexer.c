#include "lexer.h"
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>


int token_stack_create(struct TokenStack* const stack, const size_t initial_size) {
  if (stack == NULL) { return 1; }
  if (initial_size == 0) { return 2; }
  void* const alloc = malloc(initial_size*sizeof(Token));
  if (alloc == NULL) { return 3; }
  stack->size = initial_size;
  stack->occupied = 0;
  stack->tokens = alloc;
  return 0;
}

int token_stack_destroy(struct TokenStack* const stack) {
  if (stack->tokens == NULL) { return 1; }
  free(stack->tokens);
  stack->tokens = NULL;
  stack->size = 0;
  stack->occupied = 0;
  return 0;
}


// May try to realloc
int token_stack_pusha(struct TokenStack* const stack, const Token token) {
  assert(stack->occupied <= stack->size); // Debug check, ensure the stack is never overflowed
  if (stack->tokens == NULL) { return 1; }

  if (stack->occupied == stack->size) {
    const size_t new_size = TOKEN_STACK_REALLOC_POLICY(stack->size);
    Token* const new_alloc = (Token*)malloc(new_size*sizeof(Token));
    if (new_alloc == NULL) { return 2; }

    // TODO: Make Realloc
    memcpy(new_alloc, stack->tokens, stack->size*sizeof(token));
    free(stack->tokens);

    stack->tokens = new_alloc;
    stack->size = new_size;
  }
  stack->tokens[stack->occupied++] = token;
  return 0;
}

// Pop from stack to token
int token_stack_pop(struct TokenStack* const stack, Token* const token) {
  if (stack->tokens == NULL) { return 1; }
  if (stack->occupied == 0) { return 2; }
  *token = stack->tokens[--stack->occupied];
  return 0;
}



int lexer_create(struct Lexer* const lexer, const char* const string, const size_t string_size) {
  if (lexer == NULL) { return 1; }
  if (string == NULL) { return 2; }
  if (string_size == 0) { return 3; }
  struct TokenStack stack;
  // Heuristic initial allocation
  int err = token_stack_create(&stack, TOKEN_STACK_INITIAL_ALLOC_POLICY(string_size));
  if (err) { return 100 + err; }
  lexer->string_size = string_size;
  lexer->string = string;
  lexer->current_pos = 0;
  lexer->stack = stack;
  lexer->line_start = 0;
  lexer->line_count = 1;
  return 0;
}

int lexer_destroy(struct Lexer* const lexer) {
  if (lexer == NULL) { return 1; }
  if (lexer->string == NULL) { return 2; }
  if (lexer->stack.tokens == NULL) { return 3; }
  if (lexer->stack.size == 0) { return 4; }
  if (lexer->string_size == 0) { return 5; }
  int err;
  err = token_stack_destroy(&lexer->stack);
  if (err) { return 100 + err; }
  lexer->string = NULL;
  lexer->string_size = 0;
  lexer->current_pos = 0;
  lexer->line_start = 0;
  lexer->line_count = 0;
  return 0;
}


char lexer_peek(struct Lexer* const lexer, const size_t n) {
  assert(n<lexer->string_size-lexer->current_pos);
  assert(lexer->string!=NULL);
  assert(lexer->stack.tokens!=NULL);
  return lexer->string[lexer->current_pos+n];
}


char lexer_next(struct Lexer* const lexer) {
  assert(lexer->current_pos<lexer->string_size);
  assert(lexer->string!=NULL);
  assert(lexer->stack.tokens!=NULL);
  return lexer->string[lexer->current_pos++];
}


char lexer_nextn(struct Lexer* const lexer, const size_t n) {
  assert(n<lexer->string_size-lexer->current_pos);
  assert(lexer->string!=NULL);
  assert(lexer->stack.tokens!=NULL);
  return lexer->string[lexer->current_pos+=n];
}

const char* lexer_get_ptr(struct Lexer* const lexer) {
  assert(lexer->string!=NULL);
  assert(lexer->stack.tokens!=NULL);
  return &lexer->string[lexer->current_pos];
}

bool lexer_is_end(struct Lexer* const lexer) {
  assert(lexer != NULL);
  return lexer->current_pos == lexer->string_size-1;
}

size_t lexer_find_identifier_size(struct Lexer* const lexer) {
  char c = lexer_current_character(lexer);
  if (!isalpha(c)) {
    return 0;
  }
  size_t n = 1;
  do {
    c = lexer_peek(lexer, n++);
  } while (isalnum(c) || c == '-');
  return n-1;
}

size_t lexer_find_global_identifier_size(struct Lexer* const lexer) {
  char c = lexer_current_character(lexer);
  size_t n = 0;
  while (isalpha(c)) {
    c = lexer_peek(lexer, ++n);
  }
  return n;
}

// Allows the integer to start with 0
size_t lexer_find_integer_size(struct Lexer* const lexer) {
  char c = lexer_current_character(lexer);
  size_t n = 0;
  while (isdigit(c)) {
    c = lexer_peek(lexer, ++n);
  }
  return n;
}

Token lexer_tokenize(struct Lexer* const lexer) {
  while (isblank(lexer_current_character(lexer)) || lexer_current_character(lexer) == '\n') {
    if (lexer_current_character(lexer) == '\n') {
      lexer->line_start = lexer->current_pos + 1;
      ++lexer->line_count;
    }
    lexer_next(lexer);
  }
  const char c = lexer_current_character(lexer);
  size_t start, id_sz;
  switch (c) {
      case '@':
        while (lexer_current_character(lexer) != '\n') {
          lexer_next(lexer);
        }
        lexer_next(lexer);
        ++lexer->line_count;
        lexer->line_start = lexer->current_pos + 1;
        return lexer_tokenize(lexer);
      case '!':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_LOGNOT, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      case '~':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_BITNOT, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      case '+':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_PLUS, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      case '_':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_MINUS, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      case '*':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_MUL, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      case '/':
        ;
        enum tok_type_e tokt = TOKT_IDIV;
        if (lexer_peek(lexer, 1) == '/') {
          ++lexer->current_pos;
          tokt = TOKT_UDIV;
        }
        ++lexer->current_pos;
        return (Token){.tok_type=tokt, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      case '&':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_BITAND, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      case '|':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_BITOR, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      case '^':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_BITXOR, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      case '(':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_LPAREN, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      case ')':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_RPAREN, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      case '{':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_LBRACE, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      case '}':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_RBRACE, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      case ';':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_SEMICOLON, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      case '=':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_EQ, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      case '.':
        lexer_next(lexer);
        start = lexer->current_pos;
        id_sz = lexer_find_global_identifier_size(lexer);
        if (id_sz == 0) {
          fprintf(stderr, "Lexer: Global Identifier is empty (illegal token)\n");
          return (Token){.tok_type=TOKT_ILLEGAL, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
        }
        lexer->current_pos += id_sz;
        return (Token){.tok_type=TOKT_GLOBAL_IDENTIFIER, .data.string = { .ptr = &lexer->string[start], .length = id_sz }};
      case '<':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_LESS, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      case '>':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_GREAT, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      case '-':
        if (isdigit(lexer_peek(lexer, 1))) {
          const char first_digit = lexer_next(lexer);
          size_t int_sz = lexer_find_integer_size(lexer); 
          assert(int_sz != 0); // Should be impossible
          uint64_t val = atoll(lexer_get_ptr(lexer));
          if (first_digit != '0' && val == 0) {
            fprintf(stderr, "Lexer: First digit of a negative number cannot be 0 (illegal token)\n");
            return (Token){.tok_type=TOKT_ILLEGAL, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
          }
          lexer_nextn(lexer, int_sz);
          return (Token){.tok_type=TOKT_INT, .data.integer = -1*val, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
        } else {
          fprintf(stderr, "Lexer: Integer expected after \'-\' (illegal token)\n");
          return (Token){.tok_type=TOKT_ILLEGAL, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
        }
      case '\'':
        if (lexer_peek(lexer, 2) != '\'') {
          fprintf(stderr, "Lexer: Closing single quote not found (illegal token)\n");
          return (Token){.tok_type=TOKT_ILLEGAL, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
        }
        unsigned int uint = lexer_peek(lexer, 1);
        lexer->current_pos += 3;
        return (Token){.tok_type=TOKT_UINT, .data.uinteger = uint, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      case '$':
        lexer_next(lexer);
        start = lexer->current_pos;
        id_sz = lexer_find_identifier_size(lexer);
        if (id_sz == 0) {
          fprintf(stderr, "Lexer: Identifier is empty (illegal token)\n");
          return (Token){.tok_type=TOKT_ILLEGAL, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
        }
        lexer->current_pos += id_sz;
        return (Token){.tok_type=TOKT_VAR, .data.string = { .ptr = &lexer->string[start], .length = id_sz }, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      default:
        break;
    }
    if (isalpha(c)) {
      size_t id_sz = lexer_find_identifier_size(lexer);
      const char* const str = lexer_get_ptr(lexer);
      lexer->current_pos += id_sz;
      switch (id_sz) {
        case 2:
          // If, Or, To
          if (!strncmp(str, "If", 2)) {
            return (Token){.tok_type=TOKT_IF, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else if (!strncmp(str, "Or", 2)) {
            return (Token){.tok_type=TOKT_LOGOR, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else if (!strncmp(str, "To", 2)) {
            return (Token){.tok_type=TOKT_TO, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else {
            goto ret_func;
          }
        case 3:
          // And, Yap, Mod 
          if (!strncmp(str, "And", 3)) {
            return (Token){.tok_type=TOKT_LOGAND, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else if (!strncmp(str, "Yap", 3)) {
            return (Token){.tok_type=TOKT_YAP, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else if (!strncmp(str, "Fun", 3)) {
            return (Token){.tok_type=TOKT_FUN, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else if (!strncmp(str, "Mod", 3)) {
            return (Token){.tok_type=TOKT_MOD, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else {
            goto ret_func;
          }
        case 4:
          // Loop, Else,  
          if (!strncmp(str, "Loop", 4)) {
            return (Token){.tok_type=TOKT_LOOP, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else if (!strncmp(str, "Else", 4)) {
            return (Token){.tok_type=TOKT_ELSE, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else {
            goto ret_func;
          }
        case 5:
          if (!strncmp(str, "Yap67", 5)) {
            return (Token){.tok_type=TOKT_YAP67, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else if (!strncmp(str, "Break", 5)) {
            return (Token){.tok_type=TOKT_BREAK, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else if (!strncmp(str, "Throw", 5)) {
            return (Token){.tok_type=TOKT_THROW, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else if (!strncmp(str, "Squad", 5)) {
            return (Token){.tok_type=TOKT_SQUAD, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else if (!strncmp(str, "Uquad", 5)) {
            return (Token){.tok_type=TOKT_UQUAD, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else if (!strncmp(str, "Sbyte", 5)) {
            return (Token){.tok_type=TOKT_SBYTE, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else if (!strncmp(str, "Ubyte", 5)) {
            return (Token){.tok_type=TOKT_UBYTE, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else {
            goto ret_func;
          }
        case 6:
          // LockIn
          if (!strncmp(str, "LockIn", 6)) {
            return (Token){.tok_type=TOKT_LOCKIN, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else if (!strncmp(str, "Assign", 6)) {
            return (Token){.tok_type=TOKT_ASSIGN, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else {
            goto ret_func;
          }
        case 8:
          // LockIn67, Localize
          if (!strncmp(str, "LockIn67", 8)) {
            return (Token){.tok_type=TOKT_LOCKIN67, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else if (!strncmp(str, "Localize", 8)) {
            return (Token){.tok_type=TOKT_LOCALIZE, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else {
            goto ret_func;
          }
        case 9:
          // Globalize
          if (!strncmp(str, "Globalize", 9)) {
            return (Token){.tok_type=TOKT_GLOBALIZE, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};            
          } else {
            goto ret_func;
          }
        default:
           ret_func: return (Token){.tok_type=TOKT_FUNCTION, .data.string = {.ptr = str, id_sz}, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};           
      }
    } else if (isdigit(c)) {
      size_t int_sz = lexer_find_integer_size(lexer); 
      assert(int_sz != 0); // Should be impossible
      int64_t val = atoll(lexer_get_ptr(lexer));
      if (c != '0' && val == 0) {
        fprintf(stderr, "Lexer: First digit of a positive integer that is not 0 cannot be 0 (illegal token)\n");
        return (Token){.tok_type=TOKT_ILLEGAL, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      }
      if (lexer_peek(lexer, int_sz) == 'u') {
        lexer_nextn(lexer, int_sz+1);
        return (Token){.tok_type=TOKT_UINT, .data.uinteger = val, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      } else {
        lexer_nextn(lexer, int_sz);
        return (Token){.tok_type=TOKT_INT, .data.integer = val, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
      }
    }
    fprintf(stderr, "Lexer: could not tokenize (illegal token)\n");
    return (Token){.tok_type=TOKT_ILLEGAL, .col = lexer->current_pos - lexer->line_start, .line = lexer->line_count};
}


int lexer_tokenize_all(struct Lexer* const lexer, struct TokenStack* const stack) {
  if (stack == NULL) { return 1; }
  int res = token_stack_create(stack, TOKEN_STACK_INITIAL_ALLOC_POLICY(lexer->string_size));
  if (res) { return 100 + res; }
  Token t;
  while (!lexer_is_end(lexer) && (t = lexer_tokenize(lexer)).tok_type != TOKT_ILLEGAL) {
    res = token_stack_pusha(stack, t);
    if (res) { return 2; }
  }
  if (t.tok_type == TOKT_ILLEGAL) {
    fprintf(stderr, "Illegal token @%d:%d\n", t.line, t.col);
    return 3;
  }
  return 0;
}

