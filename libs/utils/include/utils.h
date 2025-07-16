/**
 * @file utils.h
 * @brief Utility functions for C programming demonstrations
 * @author dunamismax
 * @date 2025
 * 
 * This header provides essential utility functions that demonstrate
 * fundamental C programming concepts and memory management.
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

/**
 * @brief Safe string duplication with error handling
 * @param str Source string to duplicate
 * @return Newly allocated string copy or NULL on failure
 * 
 * Demonstrates: Dynamic memory allocation, error handling,
 * string manipulation, defensive programming
 */
char* safe_strdup(const char* str);

/**
 * @brief Safe string concatenation with bounds checking
 * @param dest Destination buffer
 * @param src Source string to append
 * @param dest_size Size of destination buffer
 * @return true on success, false on overflow
 * 
 * Demonstrates: Buffer overflow protection, string manipulation,
 * defensive programming practices
 */
bool safe_strcat(char* dest, const char* src, size_t dest_size);

/**
 * @brief Safe memory allocation with zero initialization
 * @param count Number of elements
 * @param size Size of each element
 * @return Allocated and zeroed memory or NULL on failure
 * 
 * Demonstrates: Memory allocation, initialization, error handling
 */
void* safe_calloc(size_t count, size_t size);

/**
 * @brief Safe memory reallocation with overflow protection
 * @param ptr Pointer to reallocate
 * @param new_size New size in bytes
 * @return Reallocated memory or NULL on failure
 * 
 * Demonstrates: Memory reallocation, overflow protection,
 * memory management best practices
 */
void* safe_realloc(void* ptr, size_t new_size);

/**
 * @brief Secure memory clearing before free
 * @param ptr Pointer to memory to clear and free
 * @param size Size of memory to clear
 * 
 * Demonstrates: Security best practices, memory management,
 * preventing information leaks
 */
void secure_free(void* ptr, size_t size);

/**
 * @brief Simple logging utility with levels
 * @param level Log level (DEBUG, INFO, WARN, ERROR)
 * @param format Printf-style format string
 * @param ... Variable arguments
 * 
 * Demonstrates: Variadic functions, formatted output,
 * debugging techniques
 */
void log_message(const char* level, const char* format, ...);

/**
 * @brief Convert string to integer with error checking
 * @param str String to convert
 * @param result Pointer to store result
 * @return true on success, false on error
 * 
 * Demonstrates: String to number conversion, error handling,
 * input validation
 */
bool str_to_int(const char* str, int* result);

/**
 * @brief Trim whitespace from both ends of string
 * @param str String to trim (modified in place)
 * @return Pointer to trimmed string
 * 
 * Demonstrates: String manipulation, character classification,
 * in-place modification
 */
char* trim_whitespace(char* str);

/**
 * @brief Check if string contains only digits
 * @param str String to check
 * @return true if all characters are digits, false otherwise
 * 
 * Demonstrates: String validation, character classification,
 * input validation techniques
 */
bool is_numeric(const char* str);

/**
 * @brief Get file size safely
 * @param filename Path to file
 * @param size Pointer to store file size
 * @return true on success, false on error
 * 
 * Demonstrates: File operations, error handling,
 * system programming
 */
bool get_file_size(const char* filename, size_t* size);

#endif // UTILS_H