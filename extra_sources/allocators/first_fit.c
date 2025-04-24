#include "include/microros_allocator.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef MICROROS_ALLOCATOR_FAIL_FAST
#define MICROROS_ALLOCATOR_FAIL_FAST 1
#endif

#ifndef MICROROS_HEAP_MEMORY_ALIGNMENT
#define MICROROS_HEAP_MEMORY_ALIGNMENT 4
#endif

#ifndef MICROROS_HEAP_SIZE
#define MICROROS_HEAP_SIZE 20000
#endif

typedef struct microros_heap_block_s {
  size_t start;
  size_t size;
  struct microros_heap_block_s *prev;
  struct microros_heap_block_s *next;
  bool allocated;
}
__attribute__((aligned(MICROROS_HEAP_MEMORY_ALIGNMENT))) microros_heap_block_t;

_Static_assert(sizeof(microros_heap_block_t) < MICROROS_HEAP_SIZE,
               "Heap size is too small to fit a single block");

typedef struct microros_heap_state_s {
  uint8_t heap[MICROROS_HEAP_SIZE];
  microros_heap_block_t *first_block;
  microros_heap_block_t *last_block;
  size_t block_count;
} microros_heap_state_t;

static microros_heap_state_t heap_state;

__attribute__((constructor)) static void microros_heap_state_init() {
  heap_state.first_block = (microros_heap_block_t *)heap_state.heap;
  heap_state.last_block = heap_state.first_block;
  heap_state.block_count = 1;
  heap_state.first_block->start = sizeof(microros_heap_block_t);
  heap_state.first_block->size =
      MICROROS_HEAP_SIZE - sizeof(microros_heap_block_t);
  heap_state.first_block->prev = NULL;
  heap_state.first_block->next = NULL;
  heap_state.first_block->allocated = false;
}

/**
 * @brief Splits a block into two
 *
 * The first block will have the requested size and the second block will have
 * the remaining size minus the size of the block header.
 *
 * @param block The block to be split.
 * @param size The new size of the first block. Must be smaller than the current
 * size of the block minus the size of the block header.
 */
static void split_block(microros_heap_block_t *block, size_t size) {
  microros_heap_block_t *new_block =
      (microros_heap_block_t *)(heap_state.heap + block->start + size);
  new_block->start = block->start + size + sizeof(microros_heap_block_t);
  new_block->size = block->size - size - sizeof(microros_heap_block_t);
  new_block->prev = block;
  new_block->next = block->next;
  new_block->allocated = false;

  heap_state.block_count++;

  block->size = size;
  block->next = new_block;

  if (block == heap_state.last_block) {
    heap_state.last_block = new_block;
  } else if (new_block->next) {
    new_block->next->prev = new_block;
  }
}

/**
 * @brief Combines adjacent free blocks into a single block.
 *
 * If the previous or next block is free, the current block will be combined
 * with it.
 *
 * @param block The block to be combined with adjacent free blocks.
 */
static void combine_free_blocks(microros_heap_block_t *block) {
  if (block->prev != NULL && !block->prev->allocated) {
    block->prev->size += block->size + sizeof(microros_heap_block_t);
    block->prev->next = block->next;
    if (block->next != NULL) {
      block->next->prev = block->prev;
    }

    if (block == heap_state.last_block) {
      heap_state.last_block = block->prev;
    }

    block = block->prev;
    heap_state.block_count--;
  }
  if (block->next != NULL && !block->next->allocated) {
    block->size += block->next->size + sizeof(microros_heap_block_t);
    block->next = block->next->next;
    if (block->next != NULL) {
      block->next->prev = block;
    } else {
      heap_state.last_block = block;
    }
    heap_state.block_count--;
  }
}

/**
 * @brief Finds the block that corresponds to the given pointer.
 *
 * @param pointer The pointer to find the corresponding block for.
 * @return UrosHeapBlock* The block that corresponds to the pointer, or NULL if
 * not found.
 */
static microros_heap_block_t *find_block(void *pointer) {
  microros_heap_block_t *block = heap_state.first_block;
  while (block != NULL) {
    if ((void *)(heap_state.heap + block->start) == pointer) {
      return block;
    }
    block = block->next;
  }
  return NULL;
}

void *microros_allocate(size_t size, void *state) {
  (void)state;

  if (size % MICROROS_HEAP_MEMORY_ALIGNMENT != 0) {
    size += MICROROS_HEAP_MEMORY_ALIGNMENT -
            (size % MICROROS_HEAP_MEMORY_ALIGNMENT);
  }

  // Find a free block that fits the requested size
  microros_heap_block_t *block = heap_state.first_block;
  while (block != NULL) {
    if (!block->allocated && block->size >= size) {
      // Split the block if it's possible
      if (size + sizeof(microros_heap_block_t) < block->size) {
        split_block(block, size);
      }
      block->allocated = true;
      return (void *)(heap_state.heap + block->start);
    }
    block = block->next;
  }

  microros_allocator_error("Failed to allocate memory: no free block found");

  return NULL;
}

void microros_deallocate(void *pointer, void *state) {
  (void)state;

  if (pointer == NULL) {
    return;
  }

  // Find the block that corresponds to the pointer
  microros_heap_block_t *block = find_block(pointer);
  if (block == NULL) {
#if MICROROS_ALLOCATOR_FAIL_FAST
    microros_allocator_fail("No block found for the pointer");
#endif
    return;
  }

  if (!block->allocated) {
#if MICROROS_ALLOCATOR_FAIL_FAST
    microros_allocator_fail("Block is already deallocated");
#endif
    return;
  }

  block->allocated = false;

  // Combine adjacent free blocks
  combine_free_blocks(block);
}

void *microros_reallocate(void *pointer, size_t size, void *state) {
  if (pointer == NULL) {
    return microros_allocate(size, state);
  }

  // Find the block that corresponds to the pointer
  microros_heap_block_t *block = find_block(pointer);
  if (block == NULL) {
#if MICROROS_ALLOCATOR_FAIL_FAST
    microros_allocator_fail("Invalid pointer");
#endif
    return NULL;
  }

  if (!block->allocated) {
#if MICROROS_ALLOCATOR_FAIL_FAST
    microros_allocator_fail("Block is already deallocated");
#endif
    return NULL;
  }

  if (size <= block->size) {
    // Split the block if possible
    if (size + sizeof(microros_heap_block_t) < block->size) {
      split_block(block, size);
      combine_free_blocks(block->next);
    }
    return pointer;
  }

  void *memory = microros_allocate(size, NULL);
  if (memory == NULL) {
    return NULL;
  }

  memcpy(memory, pointer, block->size);
  memset((uint8_t *)memory + block->size, 0, size - block->size);

  microros_deallocate(pointer, NULL);

  return memory;
}

void *microros_zero_allocate(size_t number_of_elements, size_t size_of_element,
                             void *state) {
  (void)state;

  size_t size = number_of_elements * size_of_element;

  void *memory = microros_allocate(size, NULL);
  if (memory == NULL) {
    return NULL;
  }

  memset(memory, 0, size);
  return memory;
}

__attribute__((weak)) void microros_allocator_error(const char *msg) {
  (void)msg;
}

__attribute__((weak)) void microros_allocator_fail(const char *msg) {
  (void)msg;
}

bool microros_is_heap_empty() {
  microros_heap_block_t *block = heap_state.first_block;
  return !block->allocated && block->next == NULL;
}

void microros_reset_heap_state() {
  microros_heap_state_init();
}
