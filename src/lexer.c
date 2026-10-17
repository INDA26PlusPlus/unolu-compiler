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
    void* const new_alloc = realloc(stack->tokens, new_size);
    if (new_alloc == NULL) { return 2; }
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
  free((void*)lexer->string);
  lexer->string = NULL;
  lexer->string_size = 0;
  lexer->current_pos = 0;
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

// Allows the integer to start with 0
size_t lexer_find_integer_size(struct Lexer* const lexer) {
  char c = lexer_current_character(lexer);
  printf("CURR %c\n", c);
  size_t n = 0;
  while (isdigit(c)) {
    c = lexer_peek(lexer, ++n);
  }
  return n;
}

Token lexer_tokenize(struct Lexer* const lexer) {
  while (isblank(lexer_current_character(lexer)) || lexer_current_character(lexer) == '\n') {
    lexer_next(lexer);
  }
  const char c = lexer_current_character(lexer);
  switch (c) {
      case '!':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_NOT};
      case '+':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_PLUS};
      case '_':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_MINUS};
      case '*':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_MUL};
      case '/':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_DIV};
      case '(':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_LPAREN};
      case ')':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_RPAREN};
      case '{':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_LBRACE};
      case '}':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_RBRACE};
      case ';':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_SEMICOLON};
      case '=':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_EQ};
      case '<':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_LESS};
      case '>':
        ++lexer->current_pos;
        return (Token){.tok_type=TOKT_GREAT};
      case '-':
        if (isdigit(lexer_peek(lexer, 1))) {
          const char first_digit = lexer_next(lexer);
          size_t int_sz = lexer_find_integer_size(lexer); 
          assert(int_sz != 0); // Should be impossible
          uint64_t val = atoll(lexer_get_ptr(lexer));
          if (first_digit != '0' && val == 0) {
            printf("ILLEGAL TOKEN: First digit of a negative integer cannot be 0\n");
            return (Token){.tok_type=TOKT_ILLEGAL};
          }
          if (lexer_peek(lexer, int_sz) == 'u') {
            lexer_nextn(lexer, int_sz+1);
            return (Token){.tok_type=TOKT_UINT, .data.uinteger = int_sz};
          } else {
            lexer_nextn(lexer, int_sz);
            return (Token){.tok_type=TOKT_INT, .data.integer = int_sz};
          } 
        } else {
          printf("ILLEGAL TOKEN: Integer expected after \'-\'\n");
          return (Token){.tok_type=TOKT_ILLEGAL};
        }
      case '\'':
        if (lexer_peek(lexer, 2) != '\'') {
          printf("ILLEGAL TOKEN: Closing single quote not found\n");
          return (Token){.tok_type=TOKT_ILLEGAL};
        }
        unsigned int uint = lexer_peek(lexer, 1);
        lexer->current_pos += 3;
        return (Token){.tok_type=TOKT_UINT, .data.uinteger = uint, };
      case '$':
        lexer_next(lexer);
        size_t start = lexer->current_pos;
        size_t id_sz = lexer_find_identifier_size(lexer);
        if (id_sz == 0) {
          printf("ILLEGAL TOKEN: Identifier is empty\n");
          return (Token){.tok_type=TOKT_ILLEGAL};
        }
        lexer->current_pos += id_sz;
        return (Token){.tok_type=TOKT_VAR, .data.string = { .ptr = &lexer->string[start], .length = id_sz }};
      default:
        break;
    }
    if (isalpha(c)) {
      size_t id_sz = lexer_find_identifier_size(lexer);
      const char* const str = lexer_get_ptr(lexer);
      lexer->current_pos += id_sz;
      switch (id_sz) {
        case 2:
          if (!strncmp(str, "If", 2)) {
            return (Token){.tok_type=TOKT_IF};            
          } else if (!strncmp(str, "Or", 2)) {
            return (Token){.tok_type=TOKT_OR};            
          } else if (!strncmp(str, "To", 2)) {
            return (Token){.tok_type=TOKT_TO};            
          } else {
            goto ret_func;
          }
        case 3:
          // And, Yap, 
          if (!strncmp(str, "And", 3)) {
            return (Token){.tok_type=TOKT_AND};            
          } else if (!strncmp(str, "Yap", 3)) {
            return (Token){.tok_type=TOKT_YAP};            
          } else if (!strncmp(str, "Fun", 3)) {
            return (Token){.tok_type=TOKT_FUN};            
          } else {
            goto ret_func;
          }
        case 4:
          // Loop, Else,  
          if (!strncmp(str, "Loop", 4)) {
            return (Token){.tok_type=TOKT_LOOP};            
          } else if (!strncmp(str, "Else", 4)) {
            return (Token){.tok_type=TOKT_ELSE};            
          } else {
            goto ret_func;
          }
        case 5:
          if (!strncmp(str, "Yap67", 5)) {
            return (Token){.tok_type=TOKT_YAP67};            
          } else if (!strncmp(str, "Break", 5)) {
            return (Token){.tok_type=TOKT_BREAK};            
          } else if (!strncmp(str, "Throw", 5)) {
            return (Token){.tok_type=TOKT_THROW};            
          } else if (!strncmp(str, "Squad", 5)) {
            return (Token){.tok_type=TOKT_SQUAD};            
          } else if (!strncmp(str, "Uquad", 5)) {
            return (Token){.tok_type=TOKT_UQUAD};            
          } else if (!strncmp(str, "Sbyte", 5)) {
            return (Token){.tok_type=TOKT_SBYTE};            
          } else if (!strncmp(str, "Ubyte", 5)) {
            return (Token){.tok_type=TOKT_UBYTE};            
          } else {
            goto ret_func;
          }
        case 6:
          // LockIn
          if (!strncmp(str, "LockIn", 6)) {
            return (Token){.tok_type=TOKT_LOCKIN};            
          } else if (!strncmp(str, "Assign", 6)) {
            return (Token){.tok_type=TOKT_ASSIGN};            
          } else {
            goto ret_func;
          }
        case 8:
          // LockIn67
          if (!strncmp(str, "LockIn67", 8)) {
            return (Token){.tok_type=TOKT_LOCKIN67};            
          } else {
            goto ret_func;
          }
        default:
             ret_func: return (Token){.tok_type=TOKT_FUNCTION, .data.string = {.ptr = str, id_sz} };           
      }
    } else if (isdigit(c)) {
      size_t int_sz = lexer_find_integer_size(lexer); 
      assert(int_sz != 0); // Should be impossible
      int64_t val = atoll(lexer_get_ptr(lexer));
      if (c != '0' && val == 0) {
        printf("ILLEGAL TOKEN: First digit of a positive integer cannot be 0 (excluding 0)\n");
        return (Token){.tok_type=TOKT_ILLEGAL};
      }
      if (lexer_peek(lexer, int_sz) == 'u') {
        lexer_nextn(lexer, int_sz+1);
        return (Token){.tok_type=TOKT_UINT, .data.uinteger = val};
      } else {
        lexer_nextn(lexer, int_sz);
        return (Token){.tok_type=TOKT_INT, .data.integer = val};
      }
    }
    printf("ILLEGAL TOKEN: Other\n");
    return (Token){.tok_type=TOKT_ILLEGAL};
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
  printf("# %zu %zu %d\n", lexer->current_pos, lexer->string_size, lexer->string[lexer->current_pos]);
  if (t.tok_type == TOKT_ILLEGAL) { return 3; }
  return 0;
}

