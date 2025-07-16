/**
 * @file linked_list.h
 * @brief Doubly linked list implementation demonstrating pointer manipulation
 * @author dunamismax
 * @date 2025
 * 
 * This implementation demonstrates:
 * - Pointer manipulation and pointer arithmetic
 * - Dynamic memory allocation for nodes
 * - Doubly linked list operations
 * - Generic programming using void pointers
 * - Memory management best practices
 */

#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Node structure for doubly linked list
 * 
 * Demonstrates: Structure design, self-referential structures,
 * pointer relationships
 */
typedef struct ListNode {
    void* data;             // Pointer to the actual data
    struct ListNode* next;  // Pointer to next node
    struct ListNode* prev;  // Pointer to previous node
} ListNode;

/**
 * @brief Doubly linked list structure
 * 
 * Demonstrates: Container design, metadata tracking,
 * encapsulation principles
 */
typedef struct {
    ListNode* head;       // Pointer to first node
    ListNode* tail;       // Pointer to last node
    size_t size;          // Number of nodes
    size_t element_size;  // Size of each element
} LinkedList;

/**
 * @brief Create a new linked list
 * @param element_size Size of each element in bytes
 * @return Pointer to new linked list or NULL on failure
 * 
 * Demonstrates: Constructor pattern, initialization,
 * memory allocation
 */
LinkedList* list_create(size_t element_size);

/**
 * @brief Destroy a linked list and free all memory
 * @param list Pointer to the linked list
 * 
 * Demonstrates: Destructor pattern, cleanup,
 * memory deallocation, iteration
 */
void list_destroy(LinkedList* list);

/**
 * @brief Add an element to the front of the list
 * @param list Pointer to the linked list
 * @param element Pointer to the element to add
 * @return true on success, false on failure
 * 
 * Demonstrates: List insertion, pointer manipulation,
 * memory allocation
 */
bool list_push_front(LinkedList* list, const void* element);

/**
 * @brief Add an element to the back of the list
 * @param list Pointer to the linked list
 * @param element Pointer to the element to add
 * @return true on success, false on failure
 * 
 * Demonstrates: List insertion, tail pointer usage,
 * memory allocation
 */
bool list_push_back(LinkedList* list, const void* element);

/**
 * @brief Remove and return the first element
 * @param list Pointer to the linked list
 * @param element Pointer to store the removed element
 * @return true on success, false if list is empty
 * 
 * Demonstrates: List deletion, pointer manipulation,
 * memory deallocation
 */
bool list_pop_front(LinkedList* list, void* element);

/**
 * @brief Remove and return the last element
 * @param list Pointer to the linked list
 * @param element Pointer to store the removed element
 * @return true on success, false if list is empty
 * 
 * Demonstrates: List deletion, tail pointer usage,
 * memory deallocation
 */
bool list_pop_back(LinkedList* list, void* element);

/**
 * @brief Get an element at a specific index
 * @param list Pointer to the linked list
 * @param index Index of the element
 * @param element Pointer to store the element
 * @return true on success, false if index is out of bounds
 * 
 * Demonstrates: List traversal, bounds checking,
 * linear search
 */
bool list_get(const LinkedList* list, size_t index, void* element);

/**
 * @brief Set an element at a specific index
 * @param list Pointer to the linked list
 * @param index Index of the element
 * @param element Pointer to the new element value
 * @return true on success, false if index is out of bounds
 * 
 * Demonstrates: List traversal, element modification,
 * bounds checking
 */
bool list_set(LinkedList* list, size_t index, const void* element);

/**
 * @brief Insert an element at a specific index
 * @param list Pointer to the linked list
 * @param index Index to insert at
 * @param element Pointer to the element to insert
 * @return true on success, false on failure
 * 
 * Demonstrates: List insertion, pointer manipulation,
 * edge case handling
 */
bool list_insert(LinkedList* list, size_t index, const void* element);

/**
 * @brief Remove an element at a specific index
 * @param list Pointer to the linked list
 * @param index Index of the element to remove
 * @param element Pointer to store the removed element (optional)
 * @return true on success, false if index is out of bounds
 * 
 * Demonstrates: List deletion, pointer manipulation,
 * node removal
 */
bool list_remove(LinkedList* list, size_t index, void* element);

/**
 * @brief Find the first occurrence of an element
 * @param list Pointer to the linked list
 * @param element Pointer to the element to find
 * @param compare Function to compare elements
 * @return Index of the element or SIZE_MAX if not found
 * 
 * Demonstrates: Linear search, function pointers,
 * comparison functions
 */
size_t list_find(const LinkedList* list, const void* element, 
                 int (*compare)(const void* a, const void* b));

/**
 * @brief Get the current size of the list
 * @param list Pointer to the linked list
 * @return Current size of the list
 * 
 * Demonstrates: Accessor methods, metadata access
 */
size_t list_size(const LinkedList* list);

/**
 * @brief Check if the list is empty
 * @param list Pointer to the linked list
 * @return true if empty, false otherwise
 * 
 * Demonstrates: Convenience methods, boolean operations
 */
bool list_is_empty(const LinkedList* list);

/**
 * @brief Clear all elements from the list
 * @param list Pointer to the linked list
 * 
 * Demonstrates: Bulk operations, memory cleanup,
 * iteration patterns
 */
void list_clear(LinkedList* list);

/**
 * @brief Reverse the order of elements in the list
 * @param list Pointer to the linked list
 * 
 * Demonstrates: Pointer manipulation, algorithm implementation,
 * in-place operations
 */
void list_reverse(LinkedList* list);

#endif // LINKED_LIST_H