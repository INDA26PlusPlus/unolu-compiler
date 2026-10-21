#include "../src/lexer.h"
#include "tap.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char code1_str[] = "Ubyte $variable;\nFun my-func(Squad $x) Uquad {\n  Assign 42+69 To $variable;\n  Yap67 $x;\n  Throw 66;\n}";
const Token code1_tokens[] = {
  {.tok_type = TOKT_UBYTE},
  {.tok_type = TOKT_VAR, .data.string = {.ptr = "variable", .length = 8}},
  {.tok_type = TOKT_SEMICOLON},
  {.tok_type = TOKT_FUN},
  {.tok_type = TOKT_FUNCTION, .data.string = {.ptr = "my-func", .length = 7}},
  {.tok_type = TOKT_LPAREN},
  {.tok_type = TOKT_SQUAD},
  {.tok_type = TOKT_VAR, .data.string = {.ptr = "x", .length = 1}},
  {.tok_type = TOKT_RPAREN},
  {.tok_type = TOKT_UQUAD},
  {.tok_type = TOKT_LBRACE},
  {.tok_type = TOKT_ASSIGN},
  {.tok_type = TOKT_INT, .data.integer = 42},
  {.tok_type = TOKT_PLUS},
  {.tok_type = TOKT_INT, .data.integer = 69},
  {.tok_type = TOKT_TO},
  {.tok_type = TOKT_VAR, .data.string = {.ptr = "variable", .length = 8}},
  {.tok_type = TOKT_SEMICOLON},
  {.tok_type = TOKT_YAP67},
  {.tok_type = TOKT_VAR, .data.string = {.ptr = "x", .length = 1}},
  {.tok_type = TOKT_SEMICOLON},
  {.tok_type = TOKT_THROW},
  {.tok_type = TOKT_INT, .data.integer = 66},
  {.tok_type = TOKT_SEMICOLON},
  {.tok_type = TOKT_RBRACE},
};


const char code2_str[] = "\
Globalize entry-point .mymain;\n\
Fun entry-point() Squad {\n\
  Ubyte $char;\n\
  LockIn $char;\n\
  If $char = 'x' {\n\
    Yap $char;\n\
  } Else {\n\
    Uquad $n;\n\
    Assign 10u / 10u To $n;\n\
    Squad $i;\n\
    Loop {\n\
      LockIn67 $i;\n\
        If $n = 4 Or ($i*2>-1) {\n\
          Throw -5;\n\
        }\n\
        Assign $n+1 To $n;\n\
        If $i + $n _ 2u < 4 And $n = 2 {\n\
          Break;\n\
        }\n\
      }\n\
  }\n\
  Throw 0;\n\
}";


const Token code2_tokens[] = {
  {.tok_type = TOKT_GLOBALIZE},
  {.tok_type = TOKT_FUNCTION, .data.string = {.ptr = "entry-point", .length = 11}},
  {.tok_type = TOKT_GLOBAL_IDENTIFIER, .data.string = {.ptr = "mymain", .length = 6}},
  {.tok_type = TOKT_SEMICOLON},
  {.tok_type = TOKT_FUN},
  {.tok_type = TOKT_FUNCTION, .data.string = {.ptr = "entry-point", .length = 11}},
  {.tok_type = TOKT_LPAREN},
  {.tok_type = TOKT_RPAREN},
  {.tok_type = TOKT_SQUAD},
  {.tok_type = TOKT_LBRACE},
  {.tok_type = TOKT_UBYTE},
  {.tok_type = TOKT_VAR, .data.string = {.ptr = "char", .length = 4}},
  {.tok_type = TOKT_SEMICOLON},
  {.tok_type = TOKT_LOCKIN},
  {.tok_type = TOKT_VAR, .data.string = {.ptr = "char", .length = 4}},
  {.tok_type = TOKT_SEMICOLON},
  {.tok_type = TOKT_IF},
  {.tok_type = TOKT_VAR, .data.string = {.ptr = "char", .length = 4}},
  {.tok_type = TOKT_EQ},
  {.tok_type = TOKT_UINT, .data.uinteger = 'x' },
  {.tok_type = TOKT_LBRACE},
  {.tok_type = TOKT_YAP},
  {.tok_type = TOKT_VAR, .data.string = {.ptr = "char", .length = 4}},
  {.tok_type = TOKT_SEMICOLON},
  {.tok_type = TOKT_RBRACE},
  {.tok_type = TOKT_ELSE},
  {.tok_type = TOKT_LBRACE},
  {.tok_type = TOKT_UQUAD},
  {.tok_type = TOKT_VAR, .data.string = {.ptr = "n", .length = 1}},
  {.tok_type = TOKT_SEMICOLON},
  {.tok_type = TOKT_ASSIGN},
  {.tok_type = TOKT_UINT, .data.uinteger = 10 },
  {.tok_type = TOKT_IDIV},
  {.tok_type = TOKT_UINT, .data.uinteger = 10 },
  {.tok_type = TOKT_TO},
  {.tok_type = TOKT_VAR, .data.string = {.ptr = "n", .length = 1}},
  {.tok_type = TOKT_SEMICOLON},
  {.tok_type = TOKT_SQUAD},
  {.tok_type = TOKT_VAR, .data.string = {.ptr = "i", .length = 1}},
  {.tok_type = TOKT_SEMICOLON},
  {.tok_type = TOKT_LOOP},
  {.tok_type = TOKT_LBRACE},
  {.tok_type = TOKT_LOCKIN67},
  {.tok_type = TOKT_VAR, .data.string = {.ptr = "i", .length = 1}},
  {.tok_type = TOKT_SEMICOLON},
  {.tok_type = TOKT_IF},
  {.tok_type = TOKT_VAR, .data.string = {.ptr = "n", .length = 1}},
  {.tok_type = TOKT_EQ},
  {.tok_type = TOKT_INT, .data.integer = 4 },
  {.tok_type = TOKT_LOGOR},
  {.tok_type = TOKT_LPAREN},
  {.tok_type = TOKT_VAR, .data.string = {.ptr = "i", .length = 1}},
  {.tok_type = TOKT_MUL},
  {.tok_type = TOKT_INT, .data.integer = 2 },
  {.tok_type = TOKT_GREAT},
  {.tok_type = TOKT_INT, .data.integer = -1 },
  {.tok_type = TOKT_RPAREN},
  {.tok_type = TOKT_LBRACE},
  {.tok_type = TOKT_THROW},
  {.tok_type = TOKT_INT, .data.integer = -5 },
  {.tok_type = TOKT_SEMICOLON},
  {.tok_type = TOKT_RBRACE},
  {.tok_type = TOKT_ASSIGN},
  {.tok_type = TOKT_VAR, .data.string = {.ptr = "n", .length = 1}},
  {.tok_type = TOKT_PLUS},
  {.tok_type = TOKT_INT, .data.integer = 1 },
  {.tok_type = TOKT_TO},
  {.tok_type = TOKT_VAR, .data.string = {.ptr = "n", .length = 1}},
  {.tok_type = TOKT_SEMICOLON},
  {.tok_type = TOKT_IF},
  {.tok_type = TOKT_VAR, .data.string = {.ptr = "i", .length = 1}},
  {.tok_type = TOKT_PLUS},
  {.tok_type = TOKT_VAR, .data.string = {.ptr = "n", .length = 1}},
  {.tok_type = TOKT_MINUS},
  {.tok_type = TOKT_UINT, .data.uinteger = 2 },
  {.tok_type = TOKT_LESS},
  {.tok_type = TOKT_INT, .data.integer = 4 },
  {.tok_type = TOKT_LOGAND},
  {.tok_type = TOKT_VAR, .data.string = {.ptr = "n", .length = 1}},
  {.tok_type = TOKT_EQ},
  {.tok_type = TOKT_INT, .data.integer = 2 },
  {.tok_type = TOKT_LBRACE},
  {.tok_type = TOKT_BREAK},
  {.tok_type = TOKT_SEMICOLON},
  {.tok_type = TOKT_RBRACE},
  {.tok_type = TOKT_RBRACE},
  {.tok_type = TOKT_RBRACE},
  {.tok_type = TOKT_THROW},
  {.tok_type = TOKT_INT, .data.integer = 0 },
  {.tok_type = TOKT_SEMICOLON},
  {.tok_type = TOKT_RBRACE},
};
 

void test_tokenize(const char* const name, int test_id, const char* const str, const size_t str_len, const Token* const tokens, const size_t num_tokens) {
  struct Lexer lexer;
  TAP_TP_BEGIN(test_id, "Lexer can tokenize");
  TAP_TP_COMMENT("code: %s", name);
  int res = lexer_create(&lexer, str, str_len);
  if (res != 0) {
    TAP_TP_COMMENT("Could not create lexer (error %d)", res);
    TAP_TP_FAIL();
  }
  for (size_t i = 0; i<num_tokens; ++i) {
    Token token = lexer_tokenize(&lexer);
    if (tokens[i].tok_type != token.tok_type) {
      TAP_TP_COMMENT("(token index %zu) expected token type %d, got %d", i, tokens[i].tok_type, token.tok_type);
      TAP_TP_FAIL();
    }
  }
  TAP_TP_END();
}

void test_tokenize_all(const char* const name, int test_id, const char* const str, const size_t str_len, const Token* const tokens, const size_t num_tokens) {
  struct Lexer lexer;
  TAP_TP_BEGIN(test_id, "Lexer can tokenize");
  TAP_TP_COMMENT("code: %s", name);
  int res = lexer_create(&lexer, str, str_len);
  if (res != 0) {
    TAP_TP_COMMENT("Could not create lexer (error %d)", res);
    TAP_TP_FAIL();
  }
  struct TokenStack stack;
  res = lexer_tokenize_all(&lexer, &stack);
  if (res) {
      TAP_TP_COMMENT("could not tokenize all, got result %d", res);
      TAP_TP_FAIL();
  }
  if (stack.occupied != num_tokens) {
    TAP_TP_COMMENT("Tokens occupied on stack (%zu) is not equal to the number of expected tokens (%zu)", stack.occupied, num_tokens);
    TAP_TP_FAIL();
  }
  for (size_t i = 0; i<num_tokens; ++i) {
    Token token = stack.tokens[i];
    TAP_TP_COMMENT("Token %d", token.tok_type);
    if (tokens[i].tok_type != token.tok_type) {
      TAP_TP_COMMENT("(token index %zu) expected token type %d, got %d", i, tokens[i].tok_type, token.tok_type);
      TAP_TP_FAIL();
    }
  }
  TAP_TP_END();
}


int main() {
  TAP_INIT(6, "Lexer");

  struct Lexer lexer;
  TAP_ASSERT(lexer_create(&lexer, code1_str, sizeof(code1_str)) == 0, 1, "Create lexer (code1)");

  TAP_TP_BEGIN(2, "Lexer can forward (code1)");
  for (size_t i = 0; i<sizeof(code1_str); ++i) {
    char c = lexer_next(&lexer);
    if (code1_str[i] != c) {
      TAP_TP_COMMENT("expected \'%c\', got \'%c\'", code1_str[i], c);
      TAP_TP_FAIL();
    }
  }
  TAP_TP_END();
  test_tokenize("code1", 3, code1_str, sizeof(code1_str), code1_tokens, sizeof(code1_tokens)/sizeof(Token));
  test_tokenize("code2", 4, code2_str, sizeof(code2_str), code2_tokens, sizeof(code2_tokens)/sizeof(Token));

  test_tokenize_all("code1", 5, code1_str, sizeof(code1_str), code1_tokens, sizeof(code1_tokens)/sizeof(Token));
  test_tokenize_all("code2", 6, code2_str, sizeof(code2_str), code2_tokens, sizeof(code2_tokens)/sizeof(Token));

  return 0;
}


