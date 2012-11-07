#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "memory.h"

char* memory_get_identity() {
  return "copying garbage collector";
}

void memory_setup(size_t total_byte_size) {
  // TODO
}

void memory_cleanup() {
  // TODO
}

void* memory_get_start() {
  // TODO
}

void* memory_get_end() {
  // TODO
}

void memory_set_heap_start(void* start) {
  // TODO
}

void* memory_allocate(tag_t tag, unsigned int size) {
  // TODO
}

size_t memory_get_block_size(value_t* block) {
  // TODO
}

tag_t memory_get_block_tag(value_t* block) {
  // TODO
}
