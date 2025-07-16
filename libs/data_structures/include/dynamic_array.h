/**
 * @file dynamic_array.h
 * @brief Dynamic array implementation demonstrating memory management
 * @author dunamismax
 * @date 2025
 * 
 * This implementation demonstrates:
 * - Dynamic memory allocation and reallocation
 * - Growth strategies and amortized analysis
 * - Generic programming in C using void pointers
 * - Error handling and defensive programming
 */

#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Dynamic array structure
 * 
 * Demonstrates structure design, encapsulation,
 * and memory management concepts
 */
typedef struct {
    void* data;           // Pointer to the actual data
    size_t size;          // Current number of elements
    size_t capacity;      // Maximum number of elements before reallocation
    size_t element_size;  // Size of each element in bytes
} DynamicArray;

/**
 * @brief Create a new dynamic array
 * @param element_size Size of each element in bytes
 * @param initial_capacity Initial capacity (0 for default)
 * @return Pointer to new dynamic array or NULL on failure
 * 
 * Demonstrates: Constructor pattern, memory allocation,
 * initialization strategies
 */
DynamicArray* darray_create(size_t element_size, size_t initial_capacity);

/**
 * @brief Destroy a dynamic array and free its memory
 * @param arr Pointer to the dynamic array
 * 
 * Demonstrates: Destructor pattern, memory deallocation,
 * resource cleanup
 */
void darray_destroy(DynamicArray* arr);

/**
 * @brief Add an element to the end of the array
 * @param arr Pointer to the dynamic array
 * @param element Pointer to the element to add
 * @return true on success, false on failure
 * 
 * Demonstrates: Array growth, memory reallocation,
 * amortized time complexity
 */
bool darray_push(DynamicArray* arr, const void* element);

/**
 * @brief Remove and return the last element
 * @param arr Pointer to the dynamic array
 * @param element Pointer to store the removed element
 * @return true on success, false if array is empty
 * 
 * Demonstrates: Array shrinking, element removal,
 * stack-like operations
 */
bool darray_pop(DynamicArray* arr, void* element);

/**
 * @brief Get an element at a specific index
 * @param arr Pointer to the dynamic array
 * @param index Index of the element
 * @param element Pointer to store the element
 * @return true on success, false if index is out of bounds
 * 
 * Demonstrates: Bounds checking, array access,
 * defensive programming
 */
bool darray_get(const DynamicArray* arr, size_t index, void* element);

/**
 * @brief Set an element at a specific index
 * @param arr Pointer to the dynamic array
 * @param index Index of the element
 * @param element Pointer to the new element value
 * @return true on success, false if index is out of bounds
 * 
 * Demonstrates: Array modification, bounds checking,
 * element assignment
 */
bool darray_set(DynamicArray* arr, size_t index, const void* element);

/**
 * @brief Insert an element at a specific index
 * @param arr Pointer to the dynamic array
 * @param index Index to insert at
 * @param element Pointer to the element to insert
 * @return true on success, false on failure
 * 
 * Demonstrates: Array insertion, element shifting,
 * memory management
 */
bool darray_insert(DynamicArray* arr, size_t index, const void* element);

/**
 * @brief Remove an element at a specific index
 * @param arr Pointer to the dynamic array
 * @param index Index of the element to remove
 * @param element Pointer to store the removed element (optional)
 * @return true on success, false if index is out of bounds
 * 
 * Demonstrates: Array deletion, element shifting,
 * memory management
 */
bool darray_remove(DynamicArray* arr, size_t index, void* element);

/**
 * @brief Get the current size of the array
 * @param arr Pointer to the dynamic array
 * @return Current size of the array
 * 
 * Demonstrates: Accessor methods, encapsulation
 */
size_t darray_size(const DynamicArray* arr);

/**
 * @brief Get the current capacity of the array
 * @param arr Pointer to the dynamic array
 * @return Current capacity of the array
 * 
 * Demonstrates: Accessor methods, capacity management
 */
size_t darray_capacity(const DynamicArray* arr);

/**
 * @brief Check if the array is empty
 * @param arr Pointer to the dynamic array
 * @return true if empty, false otherwise
 * 
 * Demonstrates: Convenience methods, boolean operations
 */
bool darray_is_empty(const DynamicArray* arr);

/**
 * @brief Clear all elements from the array
 * @param arr Pointer to the dynamic array
 * 
 * Demonstrates: Bulk operations, memory management
 */
void darray_clear(DynamicArray* arr);

/**
 * @brief Shrink the array to fit its current size
 * @param arr Pointer to the dynamic array
 * @return true on success, false on failure
 * 
 * Demonstrates: Memory optimization, capacity management
 */
bool darray_shrink_to_fit(DynamicArray* arr);

#endif // DYNAMIC_ARRAY_H