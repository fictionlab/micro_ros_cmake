#include "include/microros_allocator.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef MICROROS_HEAP_MEMORY_ALIGNMENT
#define MICROROS_HEAP_MEMORY_ALIGNMENT 4
#endif

#ifndef MICROROS_HEAP_SIZE
#define MICROROS_HEAP_SIZE 20000
#endif

typedef struct microros_heap_state_s {
  uint8_t heap[MICROROS_HEAP_SIZE];
  size_t current_pointer;
} microros_heap_state_t;

static microros_heap_state_t heap_state;

__attribute__((constructor)) static void microros_heap_state_init() {
  heap_state.current_pointer = 0;
}

void *microros_allocate(size_t size, void *state) {
  (void)state;

  if (size % MICROROS_HEAP_MEMORY_ALIGNMENT != 0) {
    size += MICROROS_HEAP_MEMORY_ALIGNMENT -
            (size % MICROROS_HEAP_MEMORY_ALIGNMENT);
  }

  if (heap_state.current_pointer + size > MICROROS_HEAP_SIZE) {
    microros_allocator_error(
        "Failed to allocate memory: not enough space left");
    return NULL;
  }

  size_t p = heap_state.current_pointer;
  heap_state.current_pointer += size;

  return (void *)&heap_state.heap[p];
}

void microros_deallocate(void *pointer, void *state) {
  (void)state;
  (void)pointer;
}

void *microros_reallocate(void *pointer, size_t size, void *state) {
  if (size % MICROROS_HEAP_MEMORY_ALIGNMENT != 0) {
    size += MICROROS_HEAP_MEMORY_ALIGNMENT -
            (size % MICROROS_HEAP_MEMORY_ALIGNMENT);
  }

  // No way of knowing if the new size is smaller than the old one so we
  // must allocate new memory either way
  void *const memory = microros_allocate(size, state);

  if (pointer != NULL && memory != NULL) {
    // Copy the old data to the new memory
    // This looks unsafe but it is actually safe even if new size is larger
    memcpy(memory, pointer, size);
  }

  return memory;
}

void *microros_zero_allocate(size_t number_of_elements, size_t size_of_element,
                             void *state) {
  (void)state;

  const size_t size = number_of_elements * size_of_element;

  void *const memory = microros_allocate(size, NULL);

  if (memory != NULL) {
    memset(memory, 0, size);
  }

  return memory;
}

bool microros_heap_is_empty(void) {
  return heap_state.current_pointer == 0;
}

void microros_heap_reset_state(void) {
  heap_state.current_pointer = 0;
}

size_t microros_heap_get_current_pointer(void) {
  return heap_state.current_pointer;
}

void microros_heap_set_current_pointer(size_t pointer) {
  if (pointer > MICROROS_HEAP_SIZE) {
    microros_allocator_fail("Pointer is out of bounds");
  }
  heap_state.current_pointer = pointer;
}