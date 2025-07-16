/**
 * @file algorithms.c
 * @brief Implementation of common algorithms for C programming demonstrations
 * @author dunamismax
 * @date 2025
 */

#include "algorithms.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/**
 * Helper function to swap two elements
 * Demonstrates generic swapping using temporary storage
 */
static void swap_elements(void *a, void *b, size_t element_size) {
  if (a == b)
    return;

  void *temp = malloc(element_size);
  if (!temp)
    return;

  memcpy(temp, a, element_size);
  memcpy(a, b, element_size);
  memcpy(b, temp, element_size);

  free(temp);
}

/**
 * Helper function to get pointer to array element
 * Demonstrates pointer arithmetic and array indexing
 */
static void *get_element(void *array, size_t index, size_t element_size) {
  return (char *)array + (index * element_size);
}

/**
 * Bubble sort implementation
 * Demonstrates basic sorting with nested loops
 */
void bubble_sort(void *array, size_t size, size_t element_size,
                 int (*compare)(const void *a, const void *b)) {
  if (!array || size <= 1 || !compare)
    return;

  for (size_t i = 0; i < size - 1; i++) {
    bool swapped = false;

    for (size_t j = 0; j < size - i - 1; j++) {
      void *elem1 = get_element(array, j, element_size);
      void *elem2 = get_element(array, j + 1, element_size);

      if (compare(elem1, elem2) > 0) {
        swap_elements(elem1, elem2, element_size);
        swapped = true;
      }
    }

    // Early termination if no swaps occurred
    if (!swapped)
      break;
  }
}

/**
 * Selection sort implementation
 * Demonstrates selection algorithm and minimum finding
 */
void selection_sort(void *array, size_t size, size_t element_size,
                    int (*compare)(const void *a, const void *b)) {
  if (!array || size <= 1 || !compare)
    return;

  for (size_t i = 0; i < size - 1; i++) {
    size_t min_idx = i;

    // Find minimum element in remaining array
    for (size_t j = i + 1; j < size; j++) {
      void *elem_j = get_element(array, j, element_size);
      void *elem_min = get_element(array, min_idx, element_size);

      if (compare(elem_j, elem_min) < 0) {
        min_idx = j;
      }
    }

    // Swap minimum element with first element
    if (min_idx != i) {
      void *elem_i = get_element(array, i, element_size);
      void *elem_min = get_element(array, min_idx, element_size);
      swap_elements(elem_i, elem_min, element_size);
    }
  }
}

/**
 * Insertion sort implementation
 * Demonstrates insertion algorithm and element shifting
 */
void insertion_sort(void *array, size_t size, size_t element_size,
                    int (*compare)(const void *a, const void *b)) {
  if (!array || size <= 1 || !compare)
    return;

  void *temp = malloc(element_size);
  if (!temp)
    return;

  for (size_t i = 1; i < size; i++) {
    void *key = get_element(array, i, element_size);
    memcpy(temp, key, element_size);

    size_t j = i;
    while (j > 0) {
      void *elem_j_minus_1 = get_element(array, j - 1, element_size);
      if (compare(elem_j_minus_1, temp) <= 0) {
        break;
      }

      void *elem_j = get_element(array, j, element_size);
      memcpy(elem_j, elem_j_minus_1, element_size);
      j--;
    }

    void *elem_j = get_element(array, j, element_size);
    memcpy(elem_j, temp, element_size);
  }

  free(temp);
}

/**
 * Quick sort helper function - partition
 * Demonstrates partitioning for divide and conquer
 */
static size_t partition(void *array, size_t low, size_t high,
                        size_t element_size,
                        int (*compare)(const void *a, const void *b)) {
  void *pivot = get_element(array, high, element_size);
  size_t i = low;

  for (size_t j = low; j < high; j++) {
    void *elem_j = get_element(array, j, element_size);
    if (compare(elem_j, pivot) <= 0) {
      if (i != j) {
        void *elem_i = get_element(array, i, element_size);
        swap_elements(elem_i, elem_j, element_size);
      }
      i++;
    }
  }

  void *elem_i = get_element(array, i, element_size);
  void *elem_high = get_element(array, high, element_size);
  swap_elements(elem_i, elem_high, element_size);

  return i;
}

/**
 * Quick sort recursive helper
 * Demonstrates divide and conquer recursion
 */
static void quick_sort_recursive(void *array, size_t low, size_t high,
                                 size_t element_size,
                                 int (*compare)(const void *a, const void *b)) {
  if (low < high) {
    size_t pi = partition(array, low, high, element_size, compare);

    if (pi > 0) {
      quick_sort_recursive(array, low, pi - 1, element_size, compare);
    }
    quick_sort_recursive(array, pi + 1, high, element_size, compare);
  }
}

/**
 * Quick sort implementation
 * Demonstrates divide and conquer algorithm
 */
void quick_sort(void *array, size_t size, size_t element_size,
                int (*compare)(const void *a, const void *b)) {
  if (!array || size <= 1 || !compare)
    return;

  quick_sort_recursive(array, 0, size - 1, element_size, compare);
}

/**
 * Merge sort helper function - merge
 * Demonstrates merging of sorted arrays
 */
static void merge(void *array, size_t left, size_t mid, size_t right,
                  size_t element_size,
                  int (*compare)(const void *a, const void *b)) {
  size_t left_size = mid - left + 1;
  size_t right_size = right - mid;

  void *left_array = malloc(left_size * element_size);
  void *right_array = malloc(right_size * element_size);

  if (!left_array || !right_array) {
    free(left_array);
    free(right_array);
    return;
  }

  // Copy data to temporary arrays
  memcpy(left_array, get_element(array, left, element_size),
         left_size * element_size);
  memcpy(right_array, get_element(array, mid + 1, element_size),
         right_size * element_size);

  // Merge the temporary arrays back
  size_t i = 0, j = 0, k = left;

  while (i < left_size && j < right_size) {
    void *left_elem = get_element(left_array, i, element_size);
    void *right_elem = get_element(right_array, j, element_size);
    void *dest = get_element(array, k, element_size);

    if (compare(left_elem, right_elem) <= 0) {
      memcpy(dest, left_elem, element_size);
      i++;
    } else {
      memcpy(dest, right_elem, element_size);
      j++;
    }
    k++;
  }

  // Copy remaining elements
  while (i < left_size) {
    void *left_elem = get_element(left_array, i, element_size);
    void *dest = get_element(array, k, element_size);
    memcpy(dest, left_elem, element_size);
    i++;
    k++;
  }

  while (j < right_size) {
    void *right_elem = get_element(right_array, j, element_size);
    void *dest = get_element(array, k, element_size);
    memcpy(dest, right_elem, element_size);
    j++;
    k++;
  }

  free(left_array);
  free(right_array);
}

/**
 * Merge sort recursive helper
 * Demonstrates divide and conquer recursion
 */
static void merge_sort_recursive(void *array, size_t left, size_t right,
                                 size_t element_size,
                                 int (*compare)(const void *a, const void *b)) {
  if (left < right) {
    size_t mid = left + (right - left) / 2;

    merge_sort_recursive(array, left, mid, element_size, compare);
    merge_sort_recursive(array, mid + 1, right, element_size, compare);

    merge(array, left, mid, right, element_size, compare);
  }
}

/**
 * Merge sort implementation
 * Demonstrates stable sorting algorithm
 */
void merge_sort(void *array, size_t size, size_t element_size,
                int (*compare)(const void *a, const void *b)) {
  if (!array || size <= 1 || !compare)
    return;

  merge_sort_recursive(array, 0, size - 1, element_size, compare);
}

/**
 * Linear search implementation
 * Demonstrates sequential search
 */
size_t linear_search(const void *array, size_t size, size_t element_size,
                     const void *target,
                     int (*compare)(const void *a, const void *b)) {
  if (!array || !target || !compare)
    return SIZE_MAX;

  for (size_t i = 0; i < size; i++) {
    const void *elem = get_element((void *)array, i, element_size);
    if (compare(elem, target) == 0) {
      return i;
    }
  }

  return SIZE_MAX;
}

/**
 * Binary search implementation
 * Demonstrates divide and conquer search
 */
size_t binary_search(const void *array, size_t size, size_t element_size,
                     const void *target,
                     int (*compare)(const void *a, const void *b)) {
  if (!array || !target || !compare)
    return SIZE_MAX;

  size_t left = 0;
  size_t right = size - 1;

  while (left <= right) {
    size_t mid = left + (right - left) / 2;
    const void *mid_elem = get_element((void *)array, mid, element_size);

    int cmp = compare(mid_elem, target);
    if (cmp == 0) {
      return mid;
    } else if (cmp < 0) {
      left = mid + 1;
    } else {
      if (mid == 0)
        break;
      right = mid - 1;
    }
  }

  return SIZE_MAX;
}

/**
 * Find maximum element
 * Demonstrates linear scan for maximum
 */
size_t find_max(const void *array, size_t size, size_t element_size,
                int (*compare)(const void *a, const void *b)) {
  if (!array || size == 0 || !compare)
    return SIZE_MAX;

  size_t max_idx = 0;
  for (size_t i = 1; i < size; i++) {
    const void *elem_i = get_element((void *)array, i, element_size);
    const void *elem_max = get_element((void *)array, max_idx, element_size);

    if (compare(elem_i, elem_max) > 0) {
      max_idx = i;
    }
  }

  return max_idx;
}

/**
 * Find minimum element
 * Demonstrates linear scan for minimum
 */
size_t find_min(const void *array, size_t size, size_t element_size,
                int (*compare)(const void *a, const void *b)) {
  if (!array || size == 0 || !compare)
    return SIZE_MAX;

  size_t min_idx = 0;
  for (size_t i = 1; i < size; i++) {
    const void *elem_i = get_element((void *)array, i, element_size);
    const void *elem_min = get_element((void *)array, min_idx, element_size);

    if (compare(elem_i, elem_min) < 0) {
      min_idx = i;
    }
  }

  return min_idx;
}

/**
 * Reverse array in place
 * Demonstrates in-place array manipulation
 */
void reverse_array(void *array, size_t size, size_t element_size) {
  if (!array || size <= 1)
    return;

  for (size_t i = 0; i < size / 2; i++) {
    void *elem_i = get_element(array, i, element_size);
    void *elem_j = get_element(array, size - 1 - i, element_size);
    swap_elements(elem_i, elem_j, element_size);
  }
}

/**
 * Shuffle array using Fisher-Yates algorithm
 * Demonstrates randomization
 */
void shuffle_array(void *array, size_t size, size_t element_size) {
  if (!array || size <= 1)
    return;

  for (size_t i = size - 1; i > 0; i--) {
    size_t j = rand() % (i + 1);
    void *elem_i = get_element(array, i, element_size);
    void *elem_j = get_element(array, j, element_size);
    swap_elements(elem_i, elem_j, element_size);
  }
}

/**
 * Check if array is sorted
 * Demonstrates sorted property verification
 */
bool is_sorted(const void *array, size_t size, size_t element_size,
               int (*compare)(const void *a, const void *b)) {
  if (!array || size <= 1 || !compare)
    return true;

  for (size_t i = 1; i < size; i++) {
    const void *elem_prev = get_element((void *)array, i - 1, element_size);
    const void *elem_curr = get_element((void *)array, i, element_size);

    if (compare(elem_prev, elem_curr) > 0) {
      return false;
    }
  }

  return true;
}

/**
 * Compare two integers
 * Demonstrates integer comparison
 */
int compare_int(const void *a, const void *b) {
  int ia = *(const int *)a;
  int ib = *(const int *)b;
  return (ia > ib) - (ia < ib);
}

/**
 * Compare two doubles
 * Demonstrates floating point comparison
 */
int compare_double(const void *a, const void *b) {
  double da = *(const double *)a;
  double db = *(const double *)b;
  return (da > db) - (da < db);
}

/**
 * Compare two strings
 * Demonstrates string comparison
 */
int compare_string(const void *a, const void *b) {
  const char *sa = *(const char **)a;
  const char *sb = *(const char **)b;
  return strcmp(sa, sb);
}