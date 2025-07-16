/**
 * @file json_parser.c
 * @brief JSON parser demonstrating parsing algorithms and data structures
 * @author dunamismax
 * @date 2025
 * 
 * This program demonstrates:
 * - Recursive descent parsing
 * - Abstract syntax tree construction
 * - Memory management for dynamic structures
 * - Error handling and recovery
 * - String escaping and unescaping
 * - Unicode handling
 * - Pretty printing algorithms
 * - Validation and type checking
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>

// Include our utility libraries
#include "utils.h"
#include "dynamic_array.h"

/**
 * @brief JSON value types
 */
typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} JSONType;

/**
 * @brief JSON value structure
 * 
 * Demonstrates: Union usage, polymorphic data structures,
 * recursive data type definition
 */
typedef struct JSONValue {
    JSONType type;
    union {
        bool boolean;
        double number;
        char* string;
        struct {
            struct JSONValue** values;
            size_t count;
            size_t capacity;
        } array;
        struct {
            char** keys;
            struct JSONValue** values;
            size_t count;
            size_t capacity;
        } object;
    } value;
} JSONValue;

/**
 * @brief JSON parser state
 * 
 * Demonstrates: Parser state management,
 * position tracking, error handling
 */
typedef struct {
    const char* input;
    size_t position;
    size_t length;
    int line;
    int column;
    char error_message[256];
} JSONParser;

/**
 * @brief Create a new JSON value
 * @param type Type of the JSON value
 * @return Pointer to new JSON value or NULL on failure
 * 
 * Demonstrates: Constructor pattern, memory allocation,
 * type-based initialization
 */
JSONValue* json_create_value(JSONType type) {
    JSONValue* value = safe_calloc(1, sizeof(JSONValue));
    if (!value) return NULL;
    
    value->type = type;
    
    switch (type) {
        case JSON_NULL:
            break;
        case JSON_BOOL:
            value->value.boolean = false;
            break;
        case JSON_NUMBER:
            value->value.number = 0.0;
            break;
        case JSON_STRING:
            value->value.string = NULL;
            break;
        case JSON_ARRAY:
            value->value.array.values = NULL;
            value->value.array.count = 0;
            value->value.array.capacity = 0;
            break;
        case JSON_OBJECT:
            value->value.object.keys = NULL;
            value->value.object.values = NULL;
            value->value.object.count = 0;
            value->value.object.capacity = 0;
            break;
    }
    
    return value;
}

/**
 * @brief Free a JSON value and all its contents
 * @param value Pointer to JSON value to free
 * 
 * Demonstrates: Destructor pattern, recursive cleanup,
 * memory deallocation
 */
void json_free_value(JSONValue* value) {
    if (!value) return;
    
    switch (value->type) {
        case JSON_STRING:
            free(value->value.string);
            break;
            
        case JSON_ARRAY:
            for (size_t i = 0; i < value->value.array.count; i++) {
                json_free_value(value->value.array.values[i]);
            }
            free(value->value.array.values);
            break;
            
        case JSON_OBJECT:
            for (size_t i = 0; i < value->value.object.count; i++) {
                free(value->value.object.keys[i]);
                json_free_value(value->value.object.values[i]);
            }
            free(value->value.object.keys);
            free(value->value.object.values);
            break;
            
        default:
            break;
    }
    
    free(value);
}

/**
 * @brief Initialize JSON parser
 * @param parser Pointer to parser structure
 * @param input Input JSON string
 * 
 * Demonstrates: Parser initialization, state setup
 */
void json_parser_init(JSONParser* parser, const char* input) {
    if (!parser || !input) return;
    
    parser->input = input;
    parser->position = 0;
    parser->length = strlen(input);
    parser->line = 1;
    parser->column = 1;
    parser->error_message[0] = '\0';
}

/**
 * @brief Set parser error message
 * @param parser Pointer to parser
 * @param message Error message
 * 
 * Demonstrates: Error reporting, format string usage
 */
void json_parser_error(JSONParser* parser, const char* message) {
    if (!parser || !message) return;
    
    snprintf(parser->error_message, sizeof(parser->error_message),
             "Line %d, Column %d: %s", parser->line, parser->column, message);
}

/**
 * @brief Get current character from input
 * @param parser Pointer to parser
 * @return Current character or '\0' at end
 * 
 * Demonstrates: Input reading, bounds checking
 */
char json_parser_current_char(JSONParser* parser) {
    if (!parser || parser->position >= parser->length) {
        return '\0';
    }
    return parser->input[parser->position];
}

/**
 * @brief Advance parser position
 * @param parser Pointer to parser
 * 
 * Demonstrates: Parser position management,
 * line/column tracking
 */
void json_parser_advance(JSONParser* parser) {
    if (!parser || parser->position >= parser->length) {
        return;
    }
    
    if (parser->input[parser->position] == '\n') {
        parser->line++;
        parser->column = 1;
    } else {
        parser->column++;
    }
    
    parser->position++;
}

/**
 * @brief Skip whitespace characters
 * @param parser Pointer to parser
 * 
 * Demonstrates: Whitespace handling, character classification
 */
void json_parser_skip_whitespace(JSONParser* parser) {
    if (!parser) return;
    
    while (parser->position < parser->length) {
        char ch = parser->input[parser->position];
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
            json_parser_advance(parser);
        } else {
            break;
        }
    }
}

/**
 * @brief Parse JSON string value
 * @param parser Pointer to parser
 * @return Parsed string or NULL on error
 * 
 * Demonstrates: String parsing, escape sequence handling,
 * dynamic string building
 */
char* json_parse_string(JSONParser* parser) {
    if (!parser) return NULL;
    
    if (json_parser_current_char(parser) != '"') {
        json_parser_error(parser, "Expected '\"' at start of string");
        return NULL;
    }
    
    json_parser_advance(parser); // Skip opening quote
    
    char* result = safe_calloc(1024, sizeof(char));
    if (!result) {
        json_parser_error(parser, "Memory allocation failed");
        return NULL;
    }
    
    size_t result_len = 0;
    size_t result_capacity = 1024;
    
    while (parser->position < parser->length) {
        char ch = json_parser_current_char(parser);
        
        if (ch == '"') {
            json_parser_advance(parser);
            result[result_len] = '\0';
            return result;
        }
        
        if (ch == '\\') {
            json_parser_advance(parser);
            if (parser->position >= parser->length) {
                json_parser_error(parser, "Unexpected end of input in string");
                free(result);
                return NULL;
            }
            
            char escaped = json_parser_current_char(parser);
            char to_add;
            
            switch (escaped) {
                case '"': to_add = '"'; break;
                case '\\': to_add = '\\'; break;
                case '/': to_add = '/'; break;
                case 'b': to_add = '\b'; break;
                case 'f': to_add = '\f'; break;
                case 'n': to_add = '\n'; break;
                case 'r': to_add = '\r'; break;
                case 't': to_add = '\t'; break;
                default:
                    json_parser_error(parser, "Invalid escape sequence");
                    free(result);
                    return NULL;
            }
            
            if (result_len >= result_capacity - 1) {
                result_capacity *= 2;
                result = safe_realloc(result, result_capacity);
                if (!result) {
                    json_parser_error(parser, "Memory allocation failed");
                    return NULL;
                }
            }
            
            result[result_len++] = to_add;
        } else {
            if (result_len >= result_capacity - 1) {
                result_capacity *= 2;
                result = safe_realloc(result, result_capacity);
                if (!result) {
                    json_parser_error(parser, "Memory allocation failed");
                    return NULL;
                }
            }
            
            result[result_len++] = ch;
        }
        
        json_parser_advance(parser);
    }
    
    json_parser_error(parser, "Unterminated string");
    free(result);
    return NULL;
}

/**
 * @brief Parse JSON number value
 * @param parser Pointer to parser
 * @param result Pointer to store parsed number
 * @return true if number was parsed successfully
 * 
 * Demonstrates: Number parsing, validation,
 * floating point conversion
 */
bool json_parse_number(JSONParser* parser, double* result) {
    if (!parser || !result) return false;
    
    char number_str[64];
    size_t number_len = 0;
    
    // Handle negative sign
    if (json_parser_current_char(parser) == '-') {
        number_str[number_len++] = '-';
        json_parser_advance(parser);
    }
    
    // Parse integer part
    if (json_parser_current_char(parser) == '0') {
        number_str[number_len++] = '0';
        json_parser_advance(parser);
    } else if (isdigit(json_parser_current_char(parser))) {
        while (isdigit(json_parser_current_char(parser)) && number_len < sizeof(number_str) - 1) {
            number_str[number_len++] = json_parser_current_char(parser);
            json_parser_advance(parser);
        }
    } else {
        json_parser_error(parser, "Invalid number format");
        return false;
    }
    
    // Parse decimal part
    if (json_parser_current_char(parser) == '.') {
        number_str[number_len++] = '.';
        json_parser_advance(parser);
        
        if (!isdigit(json_parser_current_char(parser))) {
            json_parser_error(parser, "Invalid number format");
            return false;
        }
        
        while (isdigit(json_parser_current_char(parser)) && number_len < sizeof(number_str) - 1) {
            number_str[number_len++] = json_parser_current_char(parser);
            json_parser_advance(parser);
        }
    }
    
    // Parse exponent part
    if (json_parser_current_char(parser) == 'e' || json_parser_current_char(parser) == 'E') {
        number_str[number_len++] = json_parser_current_char(parser);
        json_parser_advance(parser);
        
        if (json_parser_current_char(parser) == '+' || json_parser_current_char(parser) == '-') {
            number_str[number_len++] = json_parser_current_char(parser);
            json_parser_advance(parser);
        }
        
        if (!isdigit(json_parser_current_char(parser))) {
            json_parser_error(parser, "Invalid number format");
            return false;
        }
        
        while (isdigit(json_parser_current_char(parser)) && number_len < sizeof(number_str) - 1) {
            number_str[number_len++] = json_parser_current_char(parser);
            json_parser_advance(parser);
        }
    }
    
    number_str[number_len] = '\0';
    
    char* endptr;
    *result = strtod(number_str, &endptr);
    
    if (*endptr != '\0') {
        json_parser_error(parser, "Invalid number format");
        return false;
    }
    
    return true;
}

// Forward declaration for recursive parsing
JSONValue* json_parse_value(JSONParser* parser);

/**
 * @brief Parse JSON array
 * @param parser Pointer to parser
 * @return Parsed JSON array or NULL on error
 * 
 * Demonstrates: Array parsing, recursive parsing,
 * dynamic array management
 */
JSONValue* json_parse_array(JSONParser* parser) {
    if (!parser) return NULL;
    
    if (json_parser_current_char(parser) != '[') {
        json_parser_error(parser, "Expected '[' at start of array");
        return NULL;
    }
    
    JSONValue* array = json_create_value(JSON_ARRAY);
    if (!array) {
        json_parser_error(parser, "Memory allocation failed");
        return NULL;
    }
    
    json_parser_advance(parser); // Skip '['
    json_parser_skip_whitespace(parser);
    
    // Handle empty array
    if (json_parser_current_char(parser) == ']') {
        json_parser_advance(parser);
        return array;
    }
    
    while (true) {
        JSONValue* element = json_parse_value(parser);
        if (!element) {
            json_free_value(array);
            return NULL;
        }
        
        // Expand array if needed
        if (array->value.array.count >= array->value.array.capacity) {
            size_t new_capacity = array->value.array.capacity == 0 ? 4 : array->value.array.capacity * 2;
            JSONValue** new_values = safe_realloc(array->value.array.values, 
                                                  new_capacity * sizeof(JSONValue*));
            if (!new_values) {
                json_parser_error(parser, "Memory allocation failed");
                json_free_value(element);
                json_free_value(array);
                return NULL;
            }
            array->value.array.values = new_values;
            array->value.array.capacity = new_capacity;
        }
        
        array->value.array.values[array->value.array.count++] = element;
        
        json_parser_skip_whitespace(parser);
        
        if (json_parser_current_char(parser) == ']') {
            json_parser_advance(parser);
            break;
        } else if (json_parser_current_char(parser) == ',') {
            json_parser_advance(parser);
            json_parser_skip_whitespace(parser);
        } else {
            json_parser_error(parser, "Expected ',' or ']' in array");
            json_free_value(array);
            return NULL;
        }
    }
    
    return array;
}

/**
 * @brief Parse JSON object
 * @param parser Pointer to parser
 * @return Parsed JSON object or NULL on error
 * 
 * Demonstrates: Object parsing, key-value pairs,
 * hash table-like structure
 */
JSONValue* json_parse_object(JSONParser* parser) {
    if (!parser) return NULL;
    
    if (json_parser_current_char(parser) != '{') {
        json_parser_error(parser, "Expected '{' at start of object");
        return NULL;
    }
    
    JSONValue* object = json_create_value(JSON_OBJECT);
    if (!object) {
        json_parser_error(parser, "Memory allocation failed");
        return NULL;
    }
    
    json_parser_advance(parser); // Skip '{'
    json_parser_skip_whitespace(parser);
    
    // Handle empty object
    if (json_parser_current_char(parser) == '}') {
        json_parser_advance(parser);
        return object;
    }
    
    while (true) {
        // Parse key
        if (json_parser_current_char(parser) != '"') {
            json_parser_error(parser, "Expected string key in object");
            json_free_value(object);
            return NULL;
        }
        
        char* key = json_parse_string(parser);
        if (!key) {
            json_free_value(object);
            return NULL;
        }
        
        json_parser_skip_whitespace(parser);
        
        if (json_parser_current_char(parser) != ':') {
            json_parser_error(parser, "Expected ':' after key in object");
            free(key);
            json_free_value(object);
            return NULL;
        }
        
        json_parser_advance(parser); // Skip ':'
        json_parser_skip_whitespace(parser);
        
        // Parse value
        JSONValue* value = json_parse_value(parser);
        if (!value) {
            free(key);
            json_free_value(object);
            return NULL;
        }
        
        // Expand object if needed
        if (object->value.object.count >= object->value.object.capacity) {
            size_t new_capacity = object->value.object.capacity == 0 ? 4 : object->value.object.capacity * 2;
            
            char** new_keys = safe_realloc(object->value.object.keys, 
                                          new_capacity * sizeof(char*));
            JSONValue** new_values = safe_realloc(object->value.object.values, 
                                                  new_capacity * sizeof(JSONValue*));
            
            if (!new_keys || !new_values) {
                json_parser_error(parser, "Memory allocation failed");
                free(key);
                json_free_value(value);
                json_free_value(object);
                return NULL;
            }
            
            object->value.object.keys = new_keys;
            object->value.object.values = new_values;
            object->value.object.capacity = new_capacity;
        }
        
        object->value.object.keys[object->value.object.count] = key;
        object->value.object.values[object->value.object.count] = value;
        object->value.object.count++;
        
        json_parser_skip_whitespace(parser);
        
        if (json_parser_current_char(parser) == '}') {
            json_parser_advance(parser);
            break;
        } else if (json_parser_current_char(parser) == ',') {
            json_parser_advance(parser);
            json_parser_skip_whitespace(parser);
        } else {
            json_parser_error(parser, "Expected ',' or '}' in object");
            json_free_value(object);
            return NULL;
        }
    }
    
    return object;
}

/**
 * @brief Parse JSON value (recursive entry point)
 * @param parser Pointer to parser
 * @return Parsed JSON value or NULL on error
 * 
 * Demonstrates: Recursive descent parsing,
 * value type discrimination
 */
JSONValue* json_parse_value(JSONParser* parser) {
    if (!parser) return NULL;
    
    json_parser_skip_whitespace(parser);
    
    char ch = json_parser_current_char(parser);
    
    if (ch == '"') {
        // String
        char* str = json_parse_string(parser);
        if (!str) return NULL;
        
        JSONValue* value = json_create_value(JSON_STRING);
        if (!value) {
            free(str);
            return NULL;
        }
        
        value->value.string = str;
        return value;
    } else if (ch == '{') {
        // Object
        return json_parse_object(parser);
    } else if (ch == '[') {
        // Array
        return json_parse_array(parser);
    } else if (ch == 't' && strncmp(parser->input + parser->position, "true", 4) == 0) {
        // True
        JSONValue* value = json_create_value(JSON_BOOL);
        if (!value) return NULL;
        
        value->value.boolean = true;
        parser->position += 4;
        parser->column += 4;
        return value;
    } else if (ch == 'f' && strncmp(parser->input + parser->position, "false", 5) == 0) {
        // False
        JSONValue* value = json_create_value(JSON_BOOL);
        if (!value) return NULL;
        
        value->value.boolean = false;
        parser->position += 5;
        parser->column += 5;
        return value;
    } else if (ch == 'n' && strncmp(parser->input + parser->position, "null", 4) == 0) {
        // Null
        JSONValue* value = json_create_value(JSON_NULL);
        if (!value) return NULL;
        
        parser->position += 4;
        parser->column += 4;
        return value;
    } else if (ch == '-' || isdigit(ch)) {
        // Number
        double number;
        if (!json_parse_number(parser, &number)) {
            return NULL;
        }
        
        JSONValue* value = json_create_value(JSON_NUMBER);
        if (!value) return NULL;
        
        value->value.number = number;
        return value;
    } else {
        json_parser_error(parser, "Unexpected character");
        return NULL;
    }
}

/**
 * @brief Pretty print JSON value
 * @param value JSON value to print
 * @param indent Current indentation level
 * 
 * Demonstrates: Pretty printing, recursive output,
 * indentation management
 */
void json_print_value(JSONValue* value, int indent) {
    if (!value) return;
    
    switch (value->type) {
        case JSON_NULL:
            printf("null");
            break;
            
        case JSON_BOOL:
            printf("%s", value->value.boolean ? "true" : "false");
            break;
            
        case JSON_NUMBER:
            if (value->value.number == floor(value->value.number)) {
                printf("%.0f", value->value.number);
            } else {
                printf("%g", value->value.number);
            }
            break;
            
        case JSON_STRING:
            printf("\"%s\"", value->value.string);
            break;
            
        case JSON_ARRAY:
            printf("[\n");
            for (size_t i = 0; i < value->value.array.count; i++) {
                for (int j = 0; j < indent + 2; j++) printf(" ");
                json_print_value(value->value.array.values[i], indent + 2);
                if (i < value->value.array.count - 1) printf(",");
                printf("\n");
            }
            for (int j = 0; j < indent; j++) printf(" ");
            printf("]");
            break;
            
        case JSON_OBJECT:
            printf("{\n");
            for (size_t i = 0; i < value->value.object.count; i++) {
                for (int j = 0; j < indent + 2; j++) printf(" ");
                printf("\"%s\": ", value->value.object.keys[i]);
                json_print_value(value->value.object.values[i], indent + 2);
                if (i < value->value.object.count - 1) printf(",");
                printf("\n");
            }
            for (int j = 0; j < indent; j++) printf(" ");
            printf("}");
            break;
    }
}

/**
 * @brief Main function
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return Exit status
 * 
 * Demonstrates: Command line processing,
 * file reading, JSON parsing
 */
int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <json_file>\n", argv[0]);
        return 1;
    }
    
    // Read JSON file
    FILE* file = fopen(argv[1], "r");
    if (!file) {
        log_message("ERROR", "Cannot open file: %s", argv[1]);
        return 1;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Read file content
    char* content = safe_calloc(file_size + 1, sizeof(char));
    if (!content) {
        log_message("ERROR", "Memory allocation failed");
        fclose(file);
        return 1;
    }
    
    fread(content, 1, file_size, file);
    fclose(file);
    
    // Parse JSON
    JSONParser parser;
    json_parser_init(&parser, content);
    
    JSONValue* root = json_parse_value(&parser);
    
    if (!root) {
        printf("JSON parsing failed: %s\n", parser.error_message);
        free(content);
        return 1;
    }
    
    // Print parsed JSON
    printf("Parsed JSON:\n");
    json_print_value(root, 0);
    printf("\n");
    
    // Cleanup
    json_free_value(root);
    free(content);
    
    return 0;
}

/**
 * Educational Notes:
 * 
 * 1. Parsing Algorithms:
 *    - Recursive descent parsing
 *    - Tokenization and lexical analysis
 *    - Error handling and recovery
 * 
 * 2. Data Structures:
 *    - Union types for polymorphism
 *    - Recursive data structures
 *    - Dynamic arrays and objects
 * 
 * 3. Memory Management:
 *    - Dynamic allocation for variable-sized data
 *    - Recursive cleanup
 *    - Memory leak prevention
 * 
 * 4. String Processing:
 *    - Escape sequence handling
 *    - Dynamic string building
 *    - Unicode considerations
 * 
 * 5. Error Handling:
 *    - Position tracking
 *    - Error message generation
 *    - Graceful failure
 * 
 * 6. Algorithm Implementation:
 *    - State machine design
 *    - Recursive algorithms
 *    - Look-ahead parsing
 * 
 * 7. Pretty Printing:
 *    - Indentation management
 *    - Recursive output
 *    - Format preservation
 */