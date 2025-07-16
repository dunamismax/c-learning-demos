/**
 * @file utils.c
 * @brief Implementation of utility functions for C programming demonstrations
 * @author dunamismax
 * @date 2025
 */

#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

/**
 * Safe string duplication with error handling
 * Demonstrates proper error checking and memory management
 */
char* safe_strdup(const char* str) {
    if (!str) {
        return NULL;
    }
    
    size_t len = strlen(str) + 1;
    char* copy = malloc(len);
    if (!copy) {
        return NULL;
    }
    
    memcpy(copy, str, len);
    return copy;
}

/**
 * Safe string concatenation with bounds checking
 * Demonstrates buffer overflow protection
 */
bool safe_strcat(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        return false;
    }
    
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    
    // Check if concatenation would overflow
    if (dest_len + src_len >= dest_size) {
        return false;
    }
    
    strcat(dest, src);
    return true;
}

/**
 * Safe memory allocation with zero initialization
 * Demonstrates defensive programming and error handling
 */
void* safe_calloc(size_t count, size_t size) {
    // Check for multiplication overflow
    if (count != 0 && size > SIZE_MAX / count) {
        return NULL;
    }
    
    void* ptr = calloc(count, size);
    return ptr;
}

/**
 * Safe memory reallocation with overflow protection
 * Demonstrates proper realloc usage and error handling
 */
void* safe_realloc(void* ptr, size_t new_size) {
    if (new_size == 0) {
        free(ptr);
        return NULL;
    }
    
    void* new_ptr = realloc(ptr, new_size);
    if (!new_ptr && new_size != 0) {
        // Original pointer is still valid on failure
        return NULL;
    }
    
    return new_ptr;
}

/**
 * Secure memory clearing before free
 * Demonstrates security best practices
 */
void secure_free(void* ptr, size_t size) {
    if (ptr && size > 0) {
        // Clear memory before freeing to prevent information leaks
        memset(ptr, 0, size);
        free(ptr);
    }
}

/**
 * Simple logging utility with levels
 * Demonstrates variadic functions and formatted output
 */
void log_message(const char* level, const char* format, ...) {
    if (!level || !format) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    printf("[%s] ", level);
    vprintf(format, args);
    printf("\n");
    
    va_end(args);
}

/**
 * Convert string to integer with error checking
 * Demonstrates proper string to number conversion
 */
bool str_to_int(const char* str, int* result) {
    if (!str || !result) {
        return false;
    }
    
    char* endptr;
    errno = 0;
    long val = strtol(str, &endptr, 10);
    
    // Check for various error conditions
    if (errno == ERANGE || val < INT_MIN || val > INT_MAX) {
        return false;
    }
    
    if (endptr == str || *endptr != '\0') {
        return false;
    }
    
    *result = (int)val;
    return true;
}

/**
 * Trim whitespace from both ends of string
 * Demonstrates string manipulation and character classification
 */
char* trim_whitespace(char* str) {
    if (!str) {
        return NULL;
    }
    
    // Trim leading whitespace
    while (isspace((unsigned char)*str)) {
        str++;
    }
    
    if (*str == '\0') {
        return str;
    }
    
    // Trim trailing whitespace
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }
    
    end[1] = '\0';
    return str;
}

/**
 * Check if string contains only digits
 * Demonstrates input validation
 */
bool is_numeric(const char* str) {
    if (!str || *str == '\0') {
        return false;
    }
    
    // Allow leading minus sign
    if (*str == '-') {
        str++;
        if (*str == '\0') {
            return false;
        }
    }
    
    while (*str) {
        if (!isdigit((unsigned char)*str)) {
            return false;
        }
        str++;
    }
    
    return true;
}

/**
 * Get file size safely
 * Demonstrates file operations and system programming
 */
bool get_file_size(const char* filename, size_t* size) {
    if (!filename || !size) {
        return false;
    }
    
    struct stat st;
    if (stat(filename, &st) != 0) {
        return false;
    }
    
    if (!S_ISREG(st.st_mode)) {
        return false; // Not a regular file
    }
    
    *size = (size_t)st.st_size;
    return true;
}