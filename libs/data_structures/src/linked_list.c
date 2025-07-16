/**
 * @file linked_list.c
 * @brief Implementation of doubly linked list demonstrating pointer manipulation
 * @author dunamismax
 * @date 2025
 */

#include "linked_list.h"
#include <stdlib.h>
#include <string.h>

/**
 * Helper function to create a new node
 * Demonstrates encapsulation and node creation
 */
static ListNode* create_node(const void* element, size_t element_size) {
    ListNode* node = malloc(sizeof(ListNode));
    if (!node) {
        return NULL;
    }
    
    node->data = malloc(element_size);
    if (!node->data) {
        free(node);
        return NULL;
    }
    
    memcpy(node->data, element, element_size);
    node->next = NULL;
    node->prev = NULL;
    
    return node;
}

/**
 * Helper function to destroy a node
 * Demonstrates encapsulation and cleanup
 */
static void destroy_node(ListNode* node) {
    if (node) {
        free(node->data);
        free(node);
    }
}

/**
 * Helper function to find a node at a specific index
 * Demonstrates list traversal optimization
 */
static ListNode* find_node(const LinkedList* list, size_t index) {
    if (!list || index >= list->size) {
        return NULL;
    }
    
    ListNode* current;
    
    // Optimize traversal by starting from head or tail
    if (index < list->size / 2) {
        // Start from head
        current = list->head;
        for (size_t i = 0; i < index; i++) {
            current = current->next;
        }
    } else {
        // Start from tail
        current = list->tail;
        for (size_t i = list->size - 1; i > index; i--) {
            current = current->prev;
        }
    }
    
    return current;
}

/**
 * Create a new linked list
 * Demonstrates constructor pattern
 */
LinkedList* list_create(size_t element_size) {
    if (element_size == 0) {
        return NULL;
    }
    
    LinkedList* list = malloc(sizeof(LinkedList));
    if (!list) {
        return NULL;
    }
    
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    list->element_size = element_size;
    
    return list;
}

/**
 * Destroy a linked list and free all memory
 * Demonstrates destructor pattern and cleanup
 */
void list_destroy(LinkedList* list) {
    if (!list) {
        return;
    }
    
    ListNode* current = list->head;
    while (current) {
        ListNode* next = current->next;
        destroy_node(current);
        current = next;
    }
    
    free(list);
}

/**
 * Add an element to the front of the list
 * Demonstrates insertion at head
 */
bool list_push_front(LinkedList* list, const void* element) {
    if (!list || !element) {
        return false;
    }
    
    ListNode* node = create_node(element, list->element_size);
    if (!node) {
        return false;
    }
    
    if (list->head) {
        node->next = list->head;
        list->head->prev = node;
    } else {
        list->tail = node;
    }
    
    list->head = node;
    list->size++;
    
    return true;
}

/**
 * Add an element to the back of the list
 * Demonstrates insertion at tail
 */
bool list_push_back(LinkedList* list, const void* element) {
    if (!list || !element) {
        return false;
    }
    
    ListNode* node = create_node(element, list->element_size);
    if (!node) {
        return false;
    }
    
    if (list->tail) {
        node->prev = list->tail;
        list->tail->next = node;
    } else {
        list->head = node;
    }
    
    list->tail = node;
    list->size++;
    
    return true;
}

/**
 * Remove and return the first element
 * Demonstrates deletion from head
 */
bool list_pop_front(LinkedList* list, void* element) {
    if (!list || list->size == 0) {
        return false;
    }
    
    ListNode* node = list->head;
    
    if (element) {
        memcpy(element, node->data, list->element_size);
    }
    
    list->head = node->next;
    if (list->head) {
        list->head->prev = NULL;
    } else {
        list->tail = NULL;
    }
    
    destroy_node(node);
    list->size--;
    
    return true;
}

/**
 * Remove and return the last element
 * Demonstrates deletion from tail
 */
bool list_pop_back(LinkedList* list, void* element) {
    if (!list || list->size == 0) {
        return false;
    }
    
    ListNode* node = list->tail;
    
    if (element) {
        memcpy(element, node->data, list->element_size);
    }
    
    list->tail = node->prev;
    if (list->tail) {
        list->tail->next = NULL;
    } else {
        list->head = NULL;
    }
    
    destroy_node(node);
    list->size--;
    
    return true;
}

/**
 * Get an element at a specific index
 * Demonstrates list traversal and bounds checking
 */
bool list_get(const LinkedList* list, size_t index, void* element) {
    if (!list || !element) {
        return false;
    }
    
    ListNode* node = find_node(list, index);
    if (!node) {
        return false;
    }
    
    memcpy(element, node->data, list->element_size);
    return true;
}

/**
 * Set an element at a specific index
 * Demonstrates element modification
 */
bool list_set(LinkedList* list, size_t index, const void* element) {
    if (!list || !element) {
        return false;
    }
    
    ListNode* node = find_node(list, index);
    if (!node) {
        return false;
    }
    
    memcpy(node->data, element, list->element_size);
    return true;
}

/**
 * Insert an element at a specific index
 * Demonstrates insertion at arbitrary position
 */
bool list_insert(LinkedList* list, size_t index, const void* element) {
    if (!list || !element) {
        return false;
    }
    
    // Handle edge cases
    if (index == 0) {
        return list_push_front(list, element);
    }
    if (index == list->size) {
        return list_push_back(list, element);
    }
    if (index > list->size) {
        return false;
    }
    
    ListNode* next_node = find_node(list, index);
    if (!next_node) {
        return false;
    }
    
    ListNode* new_node = create_node(element, list->element_size);
    if (!new_node) {
        return false;
    }
    
    ListNode* prev_node = next_node->prev;
    
    new_node->next = next_node;
    new_node->prev = prev_node;
    next_node->prev = new_node;
    prev_node->next = new_node;
    
    list->size++;
    return true;
}

/**
 * Remove an element at a specific index
 * Demonstrates deletion from arbitrary position
 */
bool list_remove(LinkedList* list, size_t index, void* element) {
    if (!list) {
        return false;
    }
    
    // Handle edge cases
    if (index == 0) {
        return list_pop_front(list, element);
    }
    if (index == list->size - 1) {
        return list_pop_back(list, element);
    }
    
    ListNode* node = find_node(list, index);
    if (!node) {
        return false;
    }
    
    if (element) {
        memcpy(element, node->data, list->element_size);
    }
    
    ListNode* prev_node = node->prev;
    ListNode* next_node = node->next;
    
    prev_node->next = next_node;
    next_node->prev = prev_node;
    
    destroy_node(node);
    list->size--;
    
    return true;
}

/**
 * Find the first occurrence of an element
 * Demonstrates linear search and function pointers
 */
size_t list_find(const LinkedList* list, const void* element, 
                 int (*compare)(const void* a, const void* b)) {
    if (!list || !element || !compare) {
        return SIZE_MAX;
    }
    
    ListNode* current = list->head;
    size_t index = 0;
    
    while (current) {
        if (compare(current->data, element) == 0) {
            return index;
        }
        current = current->next;
        index++;
    }
    
    return SIZE_MAX;
}

/**
 * Get the current size of the list
 * Demonstrates accessor methods
 */
size_t list_size(const LinkedList* list) {
    return list ? list->size : 0;
}

/**
 * Check if the list is empty
 * Demonstrates convenience methods
 */
bool list_is_empty(const LinkedList* list) {
    return list ? list->size == 0 : true;
}

/**
 * Clear all elements from the list
 * Demonstrates bulk operations
 */
void list_clear(LinkedList* list) {
    if (!list) {
        return;
    }
    
    ListNode* current = list->head;
    while (current) {
        ListNode* next = current->next;
        destroy_node(current);
        current = next;
    }
    
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

/**
 * Reverse the order of elements in the list
 * Demonstrates pointer manipulation and algorithm implementation
 */
void list_reverse(LinkedList* list) {
    if (!list || list->size <= 1) {
        return;
    }
    
    ListNode* current = list->head;
    ListNode* temp = NULL;
    
    // Swap next and prev pointers for each node
    while (current) {
        temp = current->prev;
        current->prev = current->next;
        current->next = temp;
        current = current->prev;
    }
    
    // Swap head and tail
    temp = list->head;
    list->head = list->tail;
    list->tail = temp;
}