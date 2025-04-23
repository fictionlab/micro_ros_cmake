#ifndef MICROROS_ALLOCATOR_H
#define MICROROS_ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void *microros_allocate(size_t size, void *state);
void microros_deallocate(void *pointer, void *state);
void *microros_reallocate(void *pointer, size_t size, void *state);
void *microros_zero_allocate(size_t number_of_elements, size_t size_of_element, void *state);

void microros_allocator_error(const char *msg);
void microros_allocator_fail(const char *msg);

bool microros_is_heap_empty(void);
void microros_reset_heap_state(void);

#ifdef __cplusplus
}
#endif

#endif // MICROROS_ALLOCATOR_H
