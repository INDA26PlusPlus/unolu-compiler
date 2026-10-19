#include "src/symbols.h"
#include <assert.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

// Highly ineffective but just a placeholder, you should instead use a hash table


int variable_stack_create(struct VariableStack* const stack, const size_t initial_size) {
  if (stack == NULL) { return 1; }
  if (initial_size == 0) { return 2; }
  void* const alloc = malloc(initial_size*sizeof(struct Variable));
  if (alloc == NULL) { return 3; }
  stack->size = initial_size;
  stack->occupied = 0;
  stack->nodes = alloc;
  return 0;
}

int variable_stack_destroy(struct VariableStack* const stack) {
  if (stack->nodes == NULL) { return 1; }
  free(stack->nodes);
  stack->nodes = NULL;
  stack->size = 0;
  stack->occupied = 0;
  return 0;
}


int variable_stack_pusha(struct VariableStack* const stack, const struct Variable var) {
  assert(stack->occupied <= stack->size); // Debug check, ensure the stack is never overflowed
  if (stack->nodes == NULL) { return 1; }

  if (stack->occupied == stack->size) {
    const size_t new_size = 2*(stack->size);
    struct Variable* const new_alloc = (struct Variable*)realloc(stack->nodes, new_size*sizeof(struct Variable));
    if (new_alloc == NULL) { return 2; }
    stack->nodes = new_alloc;
    stack->size = new_size;
  }
  stack->nodes[stack->occupied++] = var;
  return 0;
}

struct Variable* variable_stack_top(struct VariableStack* const stack) {
  assert(stack->occupied>=0);
  if (stack->occupied == 0) {
    return NULL;
  }
  return &stack->nodes[stack->occupied-1];
}


struct VariableTable variable_table_create(void) {
  struct VariableStack stack;
  int res = variable_stack_create(&stack, 10);
  assert(res == 0);
  return (struct VariableTable){.stack = stack};
}

void variable_table_insert(struct VariableTable* const table, const char* const name_ptr, const size_t name_len, const enum datatype_e type) {
  struct Variable var = (struct Variable){.name = { .ptr = name_ptr, .len = name_len }, .type = type, .offset = 0};
  // If it is not the first variable then it need to increment offset appropriately
  if (table->stack.occupied != 0) {
    struct Variable* const top = variable_stack_top(&table->stack);
    switch (top->type) {
      case DT_SBYTE:
      case DT_UBYTE:
      case DT_SQUAD:
      case DT_UQUAD:
        // everything is 4 byte aligned
        var.offset = top->offset + 4;
        break;
      case DT_VOID:
        exit(100);
    }
  }
  variable_stack_pusha(&table->stack, var);
}

int variable_table_get_offset(const struct VariableTable* const table, const char* const name_ptr, const size_t name_len) {
  for (size_t i = 0; i<table->stack.occupied; ++i) {
    struct Variable var = table->stack.nodes[i];
    if (var.name.len == name_len && !strncmp(var.name.ptr, name_ptr, name_len)) {
      return var.offset;
    }
  }
  return -1;
}

int variable_table_get_max_offset(const struct VariableTable* const table) {
  struct Variable* const top = variable_stack_top((struct VariableStack*)&table->stack);
  if (top == NULL) {
    return 0;
  }
  return top->offset;
}

int variable_table_get_total_size(const struct VariableTable* const table) {
  struct Variable* var = variable_stack_top((struct VariableStack*)&table->stack);
  if (var == NULL) {
    return 0;
  }
  switch (var->type) {
    case DT_SBYTE:
    case DT_UBYTE:
    case DT_SQUAD:
    case DT_UQUAD:
      // everything is 4 byte aligned
      return var->offset + 4; 
    case DT_VOID:
      exit(-101);
  }
}

struct Variable* variable_table_get_var(const struct VariableTable* const table, const char* const name_ptr, const size_t name_len) {
  for (size_t i = 0; i<table->stack.occupied; ++i) {
    struct Variable* var = &table->stack.nodes[i];
    if (var->name.len == name_len && !strncmp(var->name.ptr, name_ptr, name_len)) {
      return var;
    }
  }
  return NULL;
}
