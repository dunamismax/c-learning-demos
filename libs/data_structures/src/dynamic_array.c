/**
 * @file dynamic_array.c
 * @brief Implementation of dynamic array demonstrating memory management
 * @author dunamismax
 * @date 2025
 */

#include "dynamic_array.h"
#include <stdlib.h>
#include <string.h>

// Default initial capacity
#define DEFAULT_CAPACITY 8

// Growth factor for reallocation
#define GROWTH_FACTOR 2

/**
 * Helper function to grow the array capacity
 * Demonstrates encapsulation and growth strategies
 */
static bool darray_grow(DynamicArray* arr) {
    if (!arr) {
        return false;
    }
    
    size_t new_capacity = arr->capacity * GROWTH_FACTOR;
    if (new_capacity < arr->capacity) {
        // Overflow check
        return false;
    }
    
    void* new_data = realloc(arr->data, new_capacity * arr->element_size);
    if (!new_data) {
        return false;
    }
    
    arr->data = new_data;
    arr->capacity = new_capacity;
    return true;
}

/**
 * Create a new dynamic array
 * Demonstrates constructor pattern and initialization
 */
DynamicArray* darray_create(size_t element_size, size_t initial_capacity) {
    if (element_size == 0) {
        return NULL;
    }
    
    DynamicArray* arr = malloc(sizeof(DynamicArray));
    if (!arr) {
        return NULL;
    }
    
    // Use default capacity if not specified
    if (initial_capacity == 0) {
        initial_capacity = DEFAULT_CAPACITY;
    }
    
    arr->data = malloc(initial_capacity * element_size);
    if (!arr->data) {
        free(arr);
        return NULL;
    }
    
    arr->size = 0;
    arr->capacity = initial_capacity;
    arr->element_size = element_size;
    
    return arr;
}

/**
 * Destroy a dynamic array and free its memory
 * Demonstrates destructor pattern and cleanup
 */
void darray_destroy(DynamicArray* arr) {
    if (arr) {
        free(arr->data);
        free(arr);
    }
}

/**
 * Add an element to the end of the array
 * Demonstrates array growth and amortized analysis
 */
bool darray_push(DynamicArray* arr, const void* element) {
    if (!arr || !element) {
        return false;
    }
    
    // Grow array if necessary
    if (arr->size >= arr->capacity) {
        if (!darray_grow(arr)) {
            return false;
        }
    }
    
    // Copy element to the end
    void* dest = (char*)arr->data + (arr->size * arr->element_size);
    memcpy(dest, element, arr->element_size);
    arr->size++;
    
    return true;
}

/**
 * Remove and return the last element
 * Demonstrates stack-like operations
 */
bool darray_pop(DynamicArray* arr, void* element) {
    if (!arr || arr->size == 0) {
        return false;
    }
    
    arr->size--;
    
    if (element) {
        void* src = (char*)arr->data + (arr->size * arr->element_size);
        memcpy(element, src, arr->element_size);
    }
    
    return true;
}

/**
 * Get an element at a specific index
 * Demonstrates bounds checking and array access
 */
bool darray_get(const DynamicArray* arr, size_t index, void* element) {
    if (!arr || !element || index >= arr->size) {
        return false;
    }
    
    void* src = (char*)arr->data + (index * arr->element_size);
    memcpy(element, src, arr->element_size);
    return true;
}

/**
 * Set an element at a specific index
 * Demonstrates array modification
 */
bool darray_set(DynamicArray* arr, size_t index, const void* element) {
    if (!arr || !element || index >= arr->size) {
        return false;
    }
    
    void* dest = (char*)arr->data + (index * arr->element_size);
    memcpy(dest, element, arr->element_size);
    return true;
}

/**
 * Insert an element at a specific index
 * Demonstrates array insertion and element shifting
 */
bool darray_insert(DynamicArray* arr, size_t index, const void* element) {
    if (!arr || !element || index > arr->size) {
        return false;
    }
    
    // Grow array if necessary
    if (arr->size >= arr->capacity) {
        if (!darray_grow(arr)) {
            return false;
        }
    }
    
    // Shift elements to the right
    if (index < arr->size) {
        void* src = (char*)arr->data + (index * arr->element_size);
        void* dest = (char*)arr->data + ((index + 1) * arr->element_size);
        size_t bytes_to_move = (arr->size - index) * arr->element_size;
        memmove(dest, src, bytes_to_move);
    }
    
    // Insert the new element
    void* dest = (char*)arr->data + (index * arr->element_size);
    memcpy(dest, element, arr->element_size);
    arr->size++;
    
    return true;
}

/**
 * Remove an element at a specific index
 * Demonstrates array deletion and element shifting
 */
bool darray_remove(DynamicArray* arr, size_t index, void* element) {
    if (!arr || index >= arr->size) {
        return false;
    }
    
    // Copy the element being removed if requested
    if (element) {
        void* src = (char*)arr->data + (index * arr->element_size);
        memcpy(element, src, arr->element_size);
    }
    
    // Shift elements to the left
    if (index < arr->size - 1) {
        void* dest = (char*)arr->data + (index * arr->element_size);
        void* src = (char*)arr->data + ((index + 1) * arr->element_size);
        size_t bytes_to_move = (arr->size - index - 1) * arr->element_size;
        memmove(dest, src, bytes_to_move);
    }
    
    arr->size--;
    return true;
}

/**
 * Get the current size of the array
 * Demonstrates accessor methods
 */
size_t darray_size(const DynamicArray* arr) {
    return arr ? arr->size : 0;
}

/**
 * Get the current capacity of the array
 * Demonstrates accessor methods
 */
size_t darray_capacity(const DynamicArray* arr) {
    return arr ? arr->capacity : 0;
}

/**
 * Check if the array is empty
 * Demonstrates convenience methods
 */
bool darray_is_empty(const DynamicArray* arr) {
    return arr ? arr->size == 0 : true;
}

/**
 * Clear all elements from the array
 * Demonstrates bulk operations
 */
void darray_clear(DynamicArray* arr) {
    if (arr) {
        arr->size = 0;
    }
}

/**
 * Shrink the array to fit its current size
 * Demonstrates memory optimization
 */
bool darray_shrink_to_fit(DynamicArray* arr) {
    if (!arr || arr->size == 0) {
        return false;
    }
    
    if (arr->size == arr->capacity) {
        return true; // Already optimal
    }
    
    void* new_data = realloc(arr->data, arr->size * arr->element_size);
    if (!new_data) {
        return false;
    }
    
    arr->data = new_data;
    arr->capacity = arr->size;
    return true;
}