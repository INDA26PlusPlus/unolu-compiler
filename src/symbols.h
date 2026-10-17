#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <stddef.h>

enum datatype_e {
  DT_VOID,
  DT_UBYTE,
  DT_SBYTE,
  DT_UQUAD,
  DT_SQUAD,
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
  int* if_id_counter;
};

struct VariableTable variable_table_create(void);
void variable_table_insert(struct VariableTable* const table, const char* const name_ptr, const size_t name_len, const enum datatype_e type);
int variable_table_get_offset(const struct VariableTable* const table, const char* const name_ptr, const size_t name_len);
int variable_table_get_max_offset(const struct VariableTable* const table);
int variable_table_get_total_size(const struct VariableTable* const table);
struct Variable* variable_table_get_var(const struct VariableTable* const table, const char* const name_ptr, const size_t name_len);

#endif
