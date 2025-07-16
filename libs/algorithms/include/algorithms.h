/**
 * @file algorithms.h
 * @brief Common algorithms implementation for C programming demonstrations
 * @author dunamismax
 * @date 2025
 *
 * This header provides implementations of fundamental algorithms
 * demonstrating various programming concepts and techniques.
 */

#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Bubble sort implementation
 * @param array Pointer to the array to sort
 * @param size Number of elements in the array
 * @param element_size Size of each element in bytes
 * @param compare Comparison function
 *
 * Demonstrates: Basic sorting, nested loops, comparisons,
 * function pointers, O(n²) time complexity
 */
void bubble_sort(void *array, size_t size, size_t element_size,
                 int (*compare)(const void *a, const void *b));

/**
 * @brief Selection sort implementation
 * @param array Pointer to the array to sort
 * @param size Number of elements in the array
 * @param element_size Size of each element in bytes
 * @param compare Comparison function
 *
 * Demonstrates: Selection algorithm, minimum finding,
 * swapping, O(n²) time complexity
 */
void selection_sort(void *array, size_t size, size_t element_size,
                    int (*compare)(const void *a, const void *b));

/**
 * @brief Insertion sort implementation
 * @param array Pointer to the array to sort
 * @param size Number of elements in the array
 * @param element_size Size of each element in bytes
 * @param compare Comparison function
 *
 * Demonstrates: Insertion algorithm, shifting elements,
 * adaptive sorting, O(n²) time complexity
 */
void insertion_sort(void *array, size_t size, size_t element_size,
                    int (*compare)(const void *a, const void *b));

/**
 * @brief Quick sort implementation
 * @param array Pointer to the array to sort
 * @param size Number of elements in the array
 * @param element_size Size of each element in bytes
 * @param compare Comparison function
 *
 * Demonstrates: Divide and conquer, recursion, partitioning,
 * O(n log n) average time complexity
 */
void quick_sort(void *array, size_t size, size_t element_size,
                int (*compare)(const void *a, const void *b));

/**
 * @brief Merge sort implementation
 * @param array Pointer to the array to sort
 * @param size Number of elements in the array
 * @param element_size Size of each element in bytes
 * @param compare Comparison function
 *
 * Demonstrates: Divide and conquer, recursion, merging,
 * stable sorting, O(n log n) time complexity
 */
void merge_sort(void *array, size_t size, size_t element_size,
                int (*compare)(const void *a, const void *b));

/**
 * @brief Linear search implementation
 * @param array Pointer to the array to search
 * @param size Number of elements in the array
 * @param element_size Size of each element in bytes
 * @param target Pointer to the target element
 * @param compare Comparison function
 * @return Index of the element or SIZE_MAX if not found
 *
 * Demonstrates: Sequential search, linear time complexity,
 * O(n) time complexity
 */
size_t linear_search(const void *array, size_t size, size_t element_size,
                     const void *target,
                     int (*compare)(const void *a, const void *b));

/**
 * @brief Binary search implementation
 * @param array Pointer to the sorted array to search
 * @param size Number of elements in the array
 * @param element_size Size of each element in bytes
 * @param target Pointer to the target element
 * @param compare Comparison function
 * @return Index of the element or SIZE_MAX if not found
 *
 * Demonstrates: Binary search, divide and conquer,
 * logarithmic time complexity, O(log n) time complexity
 */
size_t binary_search(const void *array, size_t size, size_t element_size,
                     const void *target,
                     int (*compare)(const void *a, const void *b));

/**
 * @brief Find the maximum element in an array
 * @param array Pointer to the array
 * @param size Number of elements in the array
 * @param element_size Size of each element in bytes
 * @param compare Comparison function
 * @return Index of the maximum element or SIZE_MAX if array is empty
 *
 * Demonstrates: Linear scan, maximum finding, comparisons
 */
size_t find_max(const void *array, size_t size, size_t element_size,
                int (*compare)(const void *a, const void *b));

/**
 * @brief Find the minimum element in an array
 * @param array Pointer to the array
 * @param size Number of elements in the array
 * @param element_size Size of each element in bytes
 * @param compare Comparison function
 * @return Index of the minimum element or SIZE_MAX if array is empty
 *
 * Demonstrates: Linear scan, minimum finding, comparisons
 */
size_t find_min(const void *array, size_t size, size_t element_size,
                int (*compare)(const void *a, const void *b));

/**
 * @brief Reverse an array in place
 * @param array Pointer to the array to reverse
 * @param size Number of elements in the array
 * @param element_size Size of each element in bytes
 *
 * Demonstrates: In-place operations, swapping, array manipulation
 */
void reverse_array(void *array, size_t size, size_t element_size);

/**
 * @brief Shuffle an array using Fisher-Yates algorithm
 * @param array Pointer to the array to shuffle
 * @param size Number of elements in the array
 * @param element_size Size of each element in bytes
 *
 * Demonstrates: Randomization, Fisher-Yates shuffle,
 * uniform distribution
 */
void shuffle_array(void *array, size_t size, size_t element_size);

/**
 * @brief Check if an array is sorted
 * @param array Pointer to the array to check
 * @param size Number of elements in the array
 * @param element_size Size of each element in bytes
 * @param compare Comparison function
 * @return true if sorted, false otherwise
 *
 * Demonstrates: Array validation, sequential checking,
 * sorted property verification
 */
bool is_sorted(const void *array, size_t size, size_t element_size,
               int (*compare)(const void *a, const void *b));

/**
 * @brief Common comparison functions for basic types
 */

/**
 * @brief Compare two integers
 * @param a Pointer to first integer
 * @param b Pointer to second integer
 * @return -1 if a < b, 0 if a == b, 1 if a > b
 */
int compare_int(const void *a, const void *b);

/**
 * @brief Compare two doubles
 * @param a Pointer to first double
 * @param b Pointer to second double
 * @return -1 if a < b, 0 if a == b, 1 if a > b
 */
int compare_double(const void *a, const void *b);

/**
 * @brief Compare two strings
 * @param a Pointer to first string pointer
 * @param b Pointer to second string pointer
 * @return -1 if a < b, 0 if a == b, 1 if a > b
 */
int compare_string(const void *a, const void *b);

#endif // ALGORITHMS_H