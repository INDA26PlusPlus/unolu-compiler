#include <stdint.h>
#include <stdlib.h>


uint32_t utilsMalloc(uint32_t size) {
  return (uint32_t)malloc(size);
}

void utilsFree(uint32_t ptr) {
  free((void*)ptr);
}

uint32_t utilsReadByte(uint32_t ptr, uint32_t offset) {
  return ((uint8_t*)ptr)[offset];
}


void utilsWriteByte(uint32_t ptr, uint32_t offset, uint32_t value) {
  ((uint8_t*)ptr)[offset] = (uint8_t)value;
}

/*
@ size -> ptr
Localize .utilsMalloc malloc;
@ ptr
Localize .utilsFree free;
@ ptr, offset -> value
Localize .utilsReadByte read-byte;
@ ptr, offset, value
Localize .utilsWriteByte write-byte;

*/



