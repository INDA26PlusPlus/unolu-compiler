#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "src/lexer.h"
#include "src/parser.h"
#include "src/codegen.h"
#ifdef POPTIMIZER
#include "src/popt.h"
#endif

struct Arguments {
  char* input_path;
  char* output_path;
  bool help;
  #ifdef POPTIMIZER
  bool optimize;
  #endif
  bool object;
  bool assembly;
};

#define BINARY_NAME "einaplc"
#define DEFAULT_ASM_NAME "ZWluYXBsIGlzIG5vdCBhIHByb2dyYW1taW5nIGxhbmd1YWdl.S"
#define IS_FLAG(short, long, str) (!strncmp("-"short, str, sizeof("-"short)) || !strncmp("--"long, str, sizeof("--"long)))

#define HELP_TEXT  "Usage: "BINARY_NAME" [OPTIONS] <input-file>\n\
\n\
Options:\n\
  -a, --assembly        Generate assembly output\n\
  -P, --peep-optimize   Perform peephole optimizations\n\
  -o, --output          Specify the output file\n\
  -O, --object          Compile to linkable object\n\
  -h, --help            Show this help message\n\
\n\
Arguments:\n\
  <input-file>      Input file to compile\n"

int handle_arguments(struct Arguments* const args, const int argc, char** const argv) {
  for (int i = 1; i<argc; ++i) {
    char* const arg_str = argv[i];
    if (arg_str == NULL) {
      perror("Argument string is null\n");
      return 1;
    }
    if (arg_str[0] == '-') {
      if (IS_FLAG("h", "help", arg_str)) {
        args->help = true;
      } else if (IS_FLAG("P", "peep-optimize", arg_str)) {
        #ifdef POPTIMIZER
        args->optimize = true;
        #else
        perror("Peephole optimizer is not on\n");
        return 1;
        #endif
      } else if (IS_FLAG("O", "object", arg_str)) {
        args->object = true;
      } else if (IS_FLAG("a", "assembly", arg_str)) {
        args->assembly = true;
      } else if (IS_FLAG("o", "output", arg_str)) {
        if (argc == i+1) {
          perror("Output file not specified\n");
          return 1;
        } else if (argv[i+1][0] == '-') {
          perror("Output file expected, got flag\n");
          return 1;
        } else if (args->output_path != NULL) {
          perror("Output path already specified\n");
          return 1;
        } else {
          args->output_path = argv[i+1];
          i += 2;
          continue;
        }
      } else {
        fprintf(stderr, "Unrecognized flag \"%s\"\n", arg_str);
      }
    } else {
      if (args->input_path != NULL) {
        perror("Input path already specified\n");
        return 1;
      } else {
        args->input_path = arg_str;
      }
    }
  }
  return 0;
}

int main(int argc, char* argv[]) {
  struct Arguments args = {
    .input_path = NULL,
    .output_path = NULL,
    .help = false,
    .assembly = false,
    .object = false,
    #ifdef POPTIMIZER
    .optimize = false,
    #endif
  };
  int res = handle_arguments(&args, argc, argv);
  if (res) { return 1; }
  if (args.help) {
    fputs(HELP_TEXT, stdout);
    #ifndef POPTIMIZER
    fputs("Peephole optimizer is available\n", stdout);
    #endif
    return 0;
  }
  if (args.input_path == NULL) {
    perror("No input path specified\n");
    return -1;
  }
  if (args.object && args.assembly) {
    perror("Conflicting flags: -O/--object and -a/--assembly\n");
    return -1;
  }
  if (args.output_path == NULL) {
    if (args.assembly) {
      args.output_path = "out.S";
    } else if (args.object) {
      args.output_path = "out.o";
    } else {
      args.output_path = "out";
    }
  }
  FILE* const input_file = fopen(args.input_path, "r");
  if (input_file == NULL) {
    fprintf(stderr, "Could not open input file \"%s\"\n", args.input_path);
    return -1;
  }
  FILE* output_file = NULL;
  if (!args.assembly) {
    output_file = fopen(args.output_path, "w+");
    if (output_file == NULL) {
      fclose(input_file);
      fprintf(stderr, "Could not open output file \"%s\"\n", args.output_path);
      return -1;
    }
  }

  char* const assembly_file_name = args.assembly ? args.output_path : DEFAULT_ASM_NAME;
  FILE* const assembly_file = fopen(assembly_file_name, args.assembly ? "w+" : "w");
  if (assembly_file == NULL) {
    fclose(input_file);
    if (output_file != NULL) { fclose(output_file); }
    fprintf(stderr, "Could not open assembly file \"%s\"\n", assembly_file_name);
    return -1;
  }

  size_t input_file_size = 0;
  fseek(input_file, 0, SEEK_END);
  input_file_size = ftell(input_file);
  fseek(input_file, 0, SEEK_SET);
  if (input_file_size < strlen(assembly_file_name) + strlen(args.output_path) + sizeof("gcc ") + sizeof(" -o ") + sizeof(" -C ") - 2) {
    input_file_size = strlen(assembly_file_name) + strlen(args.output_path) + sizeof("gcc ") + sizeof(" -o ") + sizeof(" -C ") - 2;
  }
  char* const input_file_buffer = (char*)malloc(input_file_size);
  if (input_file_buffer == NULL) {
    fclose(input_file);
    if (output_file != NULL) { fclose(output_file); }
    fclose(assembly_file);
    perror("Failed to input file buffer\n");
    return -1;
  }
  fread(input_file_buffer, sizeof(char), input_file_size, input_file);
  struct Lexer lexer;
  struct TokenStack token_stack;
  res = lexer_create(&lexer, input_file_buffer, input_file_size);
  if (res != 0) {
    free(input_file_buffer);
    fclose(input_file);
    if (output_file != NULL) { fclose(output_file); }
    fclose(assembly_file);
    perror("Failed to create lexer\n");
    return -1;
  }
  res = lexer_tokenize_all(&lexer, &token_stack);
  if (res != 0) {
    lexer_destroy(&lexer);
    free(input_file_buffer);
    fclose(input_file);
    if (output_file != NULL) { fclose(output_file); }
    fclose(assembly_file);
    perror("Failed to tokenize\n");
    return -1;
  }

  struct ASTNodeStack node_stack;
  res = astnode_stack_create(&node_stack, ASTNODE_STACK_INITIAL_ALLOC_POLICY(lexer.stack.occupied)); 
  if (res != 0) {
    lexer_destroy(&lexer);
    free(input_file_buffer);
    fclose(input_file);
    if (output_file != NULL) { fclose(output_file); }
    fclose(assembly_file);
    perror("Failed to create create node stack\n");
    return -1;
  }
  struct Parser parser = parser_create(&token_stack, &node_stack);
  const int top_node_idx = parse_top(&parser);
  if (top_node_idx == -1) {
    lexer_destroy(&lexer);
    parser_destroy(&parser);
    astnode_stack_destroy(&node_stack);
    free(input_file_buffer);
    fclose(input_file);
    if (output_file != NULL) { fclose(output_file); }
    fclose(assembly_file);
    perror("Top node not found\n");
    return -1;
  }
  ASTNode* const top_node = astnode_stack_get_node(parser.stack, top_node_idx);
  if (top_node == NULL) {
    lexer_destroy(&lexer);
    parser_destroy(&parser);
    astnode_stack_destroy(&node_stack);
    free(input_file_buffer);
    fclose(input_file);
    if (output_file != NULL) { fclose(output_file); }
    fclose(assembly_file);
    perror("Top node is null\n");
    return -1;
  }
  res = codegen_generate(assembly_file, top_node, parser.stack, (struct SymbolTable){.global_variables = parser.global_variable_table});
  if (res != 0) {
    lexer_destroy(&lexer);
    parser_destroy(&parser);
    astnode_stack_destroy(&node_stack);
    free(input_file_buffer);
    fclose(input_file);
    if (output_file != NULL) { fclose(output_file); }
    fclose(assembly_file);
    perror("Failed to create generate code\n");
    return -1;
  }
  // Input file buffer has to be freed after code generation since pointers to it exist in AST nodes
  fclose(input_file);
  fclose(assembly_file);

  #ifdef POPTIMIZER
  if (args.optimize) {
    poptimize(assembly_file_name);
  }
  #endif
  if (!args.assembly) {
    size_t current_buf_offset = sizeof("gcc ") - 1;
    strncpy(input_file_buffer, "gcc ", sizeof("gcc "));
    strcpy(&input_file_buffer[current_buf_offset], assembly_file_name);
    while (input_file_buffer[current_buf_offset] != '\0') {
      ++current_buf_offset;
    }
    if (args.object) {
      strncpy(&input_file_buffer[current_buf_offset], " -c ", sizeof(" -c "));
      current_buf_offset += sizeof(" -c ") - 1;
    }
    strncpy(&input_file_buffer[current_buf_offset], " -o ", sizeof(" -o "));
    current_buf_offset += sizeof(" -o ") - 1;
    strcpy(&input_file_buffer[current_buf_offset], args.output_path);
    system(input_file_buffer);
    remove(assembly_file_name);
  }
  lexer_destroy(&lexer);
  parser_destroy(&parser);
  astnode_stack_destroy(&node_stack);
  free(input_file_buffer);
  return 0;
}
