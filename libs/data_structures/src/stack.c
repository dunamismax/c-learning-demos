/**
 * @file stack.c
 * @brief Implementation of stack demonstrating LIFO data structure
 * @author dunamismax
 * @date 2025
 */

#include "stack.h"
#include <stdlib.h>
#include <string.h>

// Default initial capacity
#define DEFAULT_CAPACITY 16

// Growth factor for reallocation
#define GROWTH_FACTOR 2

/**
 * Helper function to grow the stack capacity
 * Demonstrates encapsulation and growth strategies
 */
static bool stack_grow(Stack* stack) {
    if (!stack) {
        return false;
    }
    
    size_t new_capacity = stack->capacity * GROWTH_FACTOR;
    if (new_capacity < stack->capacity) {
        // Overflow check
        return false;
    }
    
    void* new_data = realloc(stack->data, new_capacity * stack->element_size);
    if (!new_data) {
        return false;
    }
    
    stack->data = new_data;
    stack->capacity = new_capacity;
    return true;
}

/**
 * Create a new stack
 * Demonstrates constructor pattern and initialization
 */
Stack* stack_create(size_t element_size, size_t initial_capacity) {
    if (element_size == 0) {
        return NULL;
    }
    
    Stack* stack = malloc(sizeof(Stack));
    if (!stack) {
        return NULL;
    }
    
    // Use default capacity if not specified
    if (initial_capacity == 0) {
        initial_capacity = DEFAULT_CAPACITY;
    }
    
    stack->data = malloc(initial_capacity * element_size);
    if (!stack->data) {
        free(stack);
        return NULL;
    }
    
    stack->top = 0;
    stack->capacity = initial_capacity;
    stack->element_size = element_size;
    
    return stack;
}

/**
 * Destroy a stack and free its memory
 * Demonstrates destructor pattern and cleanup
 */
void stack_destroy(Stack* stack) {
    if (stack) {
        free(stack->data);
        free(stack);
    }
}

/**
 * Push an element onto the stack
 * Demonstrates LIFO operations and stack growth
 */
bool stack_push(Stack* stack, const void* element) {
    if (!stack || !element) {
        return false;
    }
    
    // Grow stack if necessary
    if (stack->top >= stack->capacity) {
        if (!stack_grow(stack)) {
            return false;
        }
    }
    
    // Copy element to the top
    void* dest = (char*)stack->data + (stack->top * stack->element_size);
    memcpy(dest, element, stack->element_size);
    stack->top++;
    
    return true;
}

/**
 * Pop an element from the stack
 * Demonstrates LIFO operations and error handling
 */
bool stack_pop(Stack* stack, void* element) {
    if (!stack || stack->top == 0) {
        return false;
    }
    
    stack->top--;
    
    if (element) {
        void* src = (char*)stack->data + (stack->top * stack->element_size);
        memcpy(element, src, stack->element_size);
    }
    
    return true;
}

/**
 * Peek at the top element without removing it
 * Demonstrates non-destructive access
 */
bool stack_peek(const Stack* stack, void* element) {
    if (!stack || !element || stack->top == 0) {
        return false;
    }
    
    void* src = (char*)stack->data + ((stack->top - 1) * stack->element_size);
    memcpy(element, src, stack->element_size);
    return true;
}

/**
 * Get the current size of the stack
 * Demonstrates accessor methods
 */
size_t stack_size(const Stack* stack) {
    return stack ? stack->top : 0;
}

/**
 * Get the current capacity of the stack
 * Demonstrates accessor methods
 */
size_t stack_capacity(const Stack* stack) {
    return stack ? stack->capacity : 0;
}

/**
 * Check if the stack is empty
 * Demonstrates convenience methods
 */
bool stack_is_empty(const Stack* stack) {
    return stack ? stack->top == 0 : true;
}

/**
 * Check if the stack is full
 * Demonstrates capacity checking
 */
bool stack_is_full(const Stack* stack) {
    return stack ? stack->top == stack->capacity : true;
}

/**
 * Clear all elements from the stack
 * Demonstrates bulk operations
 */
void stack_clear(Stack* stack) {
    if (stack) {
        stack->top = 0;
    }
}

/**
 * Duplicate the top element of the stack
 * Demonstrates stack manipulation
 */
bool stack_dup(Stack* stack) {
    if (!stack || stack->top == 0) {
        return false;
    }
    
    // Get the top element
    void* top_element = (char*)stack->data + ((stack->top - 1) * stack->element_size);
    
    // Push it again
    return stack_push(stack, top_element);
}

/**
 * Swap the top two elements of the stack
 * Demonstrates element manipulation
 */
bool stack_swap(Stack* stack) {
    if (!stack || stack->top < 2) {
        return false;
    }
    
    // Allocate temporary storage
    void* temp = malloc(stack->element_size);
    if (!temp) {
        return false;
    }
    
    // Get pointers to top two elements
    void* top1 = (char*)stack->data + ((stack->top - 1) * stack->element_size);
    void* top2 = (char*)stack->data + ((stack->top - 2) * stack->element_size);
    
    // Swap the elements
    memcpy(temp, top1, stack->element_size);
    memcpy(top1, top2, stack->element_size);
    memcpy(top2, temp, stack->element_size);
    
    free(temp);
    return true;
}