#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <stddef.h>

enum datatype_e {
  DT_VOID = 1,
  DT_UBYTE = 2,
  DT_SBYTE = 4,
  DT_UQUAD = 8,
  DT_SQUAD = 16,
};

struct Variable {
  enum datatype_e type;
  struct {
    const char* ptr;
    size_t len;
  } name;
  int offset;
};

struct VariableStack {
  size_t occupied;
  size_t size;
  struct Variable* nodes;
};


struct VariableTable {
  struct VariableStack stack;
};

struct SymbolTable {
  struct VariableTable global_variables;
  struct VariableTable* local_variables;
  int* if_id_counter;
};

struct VariableTable variable_table_create(void);
void variable_table_destroy(struct VariableTable table);
void variable_table_insert(struct VariableTable* const table, const char* const name_ptr, const size_t name_len, const enum datatype_e type);
int variable_table_get_offset(const struct VariableTable* const table, const char* const name_ptr, const size_t name_len);
int variable_table_get_max_offset(const struct VariableTable* const table);
int variable_table_get_total_size(const struct VariableTable* const table);
struct Variable* variable_table_get_var(const struct VariableTable* const table, const char* const name_ptr, const size_t name_len);
struct Variable* variable_table_get_var_at_index(const struct VariableTable* const table, const size_t index);

#endif
