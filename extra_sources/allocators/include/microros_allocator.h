#ifndef MICROROS_ALLOCATOR_H
#define MICROROS_ALLOCATOR_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *microros_allocate(size_t size, void *state);
void microros_deallocate(void *pointer, void *state);
void *microros_reallocate(void *pointer, size_t size, void *state);
void *microros_zero_allocate(size_t number_of_elements, size_t size_of_element,
                             void *state);

void microros_allocator_error(const char *msg);
void microros_allocator_fail(const char *msg);

bool microros_heap_is_empty(void);
void microros_heap_reset_state(void);

// These functions are only implemented in the linear allocator
size_t microros_heap_get_current_pointer(void);
void microros_heap_set_current_pointer(size_t pointer);

#ifdef __cplusplus
}
#endif

#endif  // MICROROS_ALLOCATOR_H
