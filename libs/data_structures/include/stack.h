/**
 * @file stack.h
 * @brief Stack implementation demonstrating LIFO data structure
 * @author dunamismax
 * @date 2025
 * 
 * This implementation demonstrates:
 * - LIFO (Last In, First Out) principle
 * - Stack operations: push, pop, peek
 * - Dynamic memory allocation for stack growth
 * - Error handling and edge cases
 * - Generic programming using void pointers
 */

#ifndef STACK_H
#define STACK_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Stack structure using dynamic array
 * 
 * Demonstrates: Structure design, encapsulation,
 * dynamic memory management
 */
typedef struct {
    void* data;           // Pointer to the stack data
    size_t top;           // Index of the top element
    size_t capacity;      // Maximum capacity
    size_t element_size;  // Size of each element
} Stack;

/**
 * @brief Create a new stack
 * @param element_size Size of each element in bytes
 * @param initial_capacity Initial capacity (0 for default)
 * @return Pointer to new stack or NULL on failure
 * 
 * Demonstrates: Constructor pattern, initialization,
 * dynamic memory allocation
 */
Stack* stack_create(size_t element_size, size_t initial_capacity);

/**
 * @brief Destroy a stack and free its memory
 * @param stack Pointer to the stack
 * 
 * Demonstrates: Destructor pattern, cleanup,
 * memory deallocation
 */
void stack_destroy(Stack* stack);

/**
 * @brief Push an element onto the stack
 * @param stack Pointer to the stack
 * @param element Pointer to the element to push
 * @return true on success, false on failure
 * 
 * Demonstrates: LIFO operations, stack growth,
 * memory management
 */
bool stack_push(Stack* stack, const void* element);

/**
 * @brief Pop an element from the stack
 * @param stack Pointer to the stack
 * @param element Pointer to store the popped element
 * @return true on success, false if stack is empty
 * 
 * Demonstrates: LIFO operations, stack shrinking,
 * error handling
 */
bool stack_pop(Stack* stack, void* element);

/**
 * @brief Peek at the top element without removing it
 * @param stack Pointer to the stack
 * @param element Pointer to store the top element
 * @return true on success, false if stack is empty
 * 
 * Demonstrates: Non-destructive access, stack inspection,
 * bounds checking
 */
bool stack_peek(const Stack* stack, void* element);

/**
 * @brief Get the current size of the stack
 * @param stack Pointer to the stack
 * @return Current number of elements in the stack
 * 
 * Demonstrates: Accessor methods, metadata access
 */
size_t stack_size(const Stack* stack);

/**
 * @brief Get the current capacity of the stack
 * @param stack Pointer to the stack
 * @return Current capacity of the stack
 * 
 * Demonstrates: Accessor methods, capacity management
 */
size_t stack_capacity(const Stack* stack);

/**
 * @brief Check if the stack is empty
 * @param stack Pointer to the stack
 * @return true if empty, false otherwise
 * 
 * Demonstrates: Convenience methods, boolean operations
 */
bool stack_is_empty(const Stack* stack);

/**
 * @brief Check if the stack is full
 * @param stack Pointer to the stack
 * @return true if full, false otherwise
 * 
 * Demonstrates: Capacity checking, stack state
 */
bool stack_is_full(const Stack* stack);

/**
 * @brief Clear all elements from the stack
 * @param stack Pointer to the stack
 * 
 * Demonstrates: Bulk operations, stack reset
 */
void stack_clear(Stack* stack);

/**
 * @brief Duplicate the top element of the stack
 * @param stack Pointer to the stack
 * @return true on success, false on failure
 * 
 * Demonstrates: Stack manipulation, element duplication,
 * common stack operations
 */
bool stack_dup(Stack* stack);

/**
 * @brief Swap the top two elements of the stack
 * @param stack Pointer to the stack
 * @return true on success, false if less than 2 elements
 * 
 * Demonstrates: Stack manipulation, element swapping,
 * common stack operations
 */
bool stack_swap(Stack* stack);

#endif // STACK_H