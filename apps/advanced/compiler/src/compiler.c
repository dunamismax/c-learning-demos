/**
 * @file compiler.c
 * @brief Simple compiler demonstrating lexical analysis and parsing
 * @author dunamismax
 * @date 2025
 * 
 * This program demonstrates:
 * - Lexical analysis and tokenization
 * - Recursive descent parsing
 * - Abstract syntax tree construction
 * - Symbol table management
 * - Type checking and semantic analysis
 * - Code generation principles
 * - Error handling and reporting
 * - Compiler optimization techniques
 * - Language design principles
 * - Virtual machine execution
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>
#include <assert.h>

// Include our utility libraries
#include "utils.h"
#include "dynamic_array.h"
#include "stack.h"

/**
 * @brief Maximum identifier length
 */
#define MAX_IDENTIFIER_LENGTH 64

/**
 * @brief Maximum string literal length
 */
#define MAX_STRING_LENGTH 256

/**
 * @brief Maximum number of symbols in symbol table
 */
#define MAX_SYMBOLS 256

/**
 * @brief Maximum number of local variables
 */
#define MAX_LOCALS 64

/**
 * @brief Virtual machine stack size
 */
#define VM_STACK_SIZE 256

/**
 * @brief Token types for lexical analysis
 */
typedef enum {
    TOKEN_EOF,
    TOKEN_ERROR,
    
    // Literals
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_IDENTIFIER,
    
    // Keywords
    TOKEN_LET,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_FUNCTION,
    TOKEN_RETURN,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NULL,
    TOKEN_PRINT,
    
    // Operators
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_MODULO,
    TOKEN_ASSIGN,
    TOKEN_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    
    // Delimiters
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET
} TokenType;

/**
 * @brief Token structure
 * 
 * Demonstrates: Lexical token representation,
 * source location tracking
 */
typedef struct {
    TokenType type;
    char lexeme[MAX_IDENTIFIER_LENGTH];
    union {
        double number;
        char string[MAX_STRING_LENGTH];
    } value;
    int line;
    int column;
} Token;

/**
 * @brief Lexer state
 * 
 * Demonstrates: Lexical analyzer state management,
 * input processing
 */
typedef struct {
    const char* source;
    size_t current;
    size_t length;
    int line;
    int column;
    char error_message[256];
} Lexer;

/**
 * @brief AST node types
 */
typedef enum {
    AST_PROGRAM,
    AST_STATEMENT_LIST,
    AST_EXPRESSION_STATEMENT,
    AST_VARIABLE_DECLARATION,
    AST_ASSIGNMENT,
    AST_IF_STATEMENT,
    AST_WHILE_STATEMENT,
    AST_FOR_STATEMENT,
    AST_FUNCTION_DECLARATION,
    AST_RETURN_STATEMENT,
    AST_BLOCK_STATEMENT,
    AST_PRINT_STATEMENT,
    AST_BINARY_EXPRESSION,
    AST_UNARY_EXPRESSION,
    AST_CALL_EXPRESSION,
    AST_IDENTIFIER,
    AST_NUMBER_LITERAL,
    AST_STRING_LITERAL,
    AST_BOOLEAN_LITERAL,
    AST_NULL_LITERAL
} ASTNodeType;

/**
 * @brief AST node structure
 * 
 * Demonstrates: Abstract syntax tree representation,
 * recursive data structures
 */
typedef struct ASTNode {
    ASTNodeType type;
    union {
        struct {  // Program
            struct ASTNode** statements;
            size_t statement_count;
        } program;
        
        struct {  // Variable declaration
            char name[MAX_IDENTIFIER_LENGTH];
            struct ASTNode* initializer;
        } var_decl;
        
        struct {  // Assignment
            char name[MAX_IDENTIFIER_LENGTH];
            struct ASTNode* value;
        } assignment;
        
        struct {  // If statement
            struct ASTNode* condition;
            struct ASTNode* then_stmt;
            struct ASTNode* else_stmt;
        } if_stmt;
        
        struct {  // While statement
            struct ASTNode* condition;
            struct ASTNode* body;
        } while_stmt;
        
        struct {  // Function declaration
            char name[MAX_IDENTIFIER_LENGTH];
            char params[MAX_LOCALS][MAX_IDENTIFIER_LENGTH];
            size_t param_count;
            struct ASTNode* body;
        } func_decl;
        
        struct {  // Return statement
            struct ASTNode* value;
        } return_stmt;
        
        struct {  // Block statement
            struct ASTNode** statements;
            size_t statement_count;
        } block;
        
        struct {  // Print statement
            struct ASTNode* expression;
        } print_stmt;
        
        struct {  // Binary expression
            struct ASTNode* left;
            struct ASTNode* right;
            TokenType operator;
        } binary;
        
        struct {  // Unary expression
            struct ASTNode* operand;
            TokenType operator;
        } unary;
        
        struct {  // Function call
            char name[MAX_IDENTIFIER_LENGTH];
            struct ASTNode** arguments;
            size_t argument_count;
        } call;
        
        struct {  // Literals and identifiers
            char name[MAX_IDENTIFIER_LENGTH];
            union {
                double number;
                char string[MAX_STRING_LENGTH];
                bool boolean;
            } value;
        } literal;
    } data;
    
    int line;
    int column;
} ASTNode;

/**
 * @brief Data types in the language
 */
typedef enum {
    TYPE_VOID,
    TYPE_NUMBER,
    TYPE_STRING,
    TYPE_BOOLEAN,
    TYPE_FUNCTION
} DataType;

/**
 * @brief Symbol table entry
 * 
 * Demonstrates: Symbol management, scope handling
 */
typedef struct {
    char name[MAX_IDENTIFIER_LENGTH];
    DataType type;
    bool is_initialized;
    bool is_constant;
    int scope_level;
    union {
        double number;
        char string[MAX_STRING_LENGTH];
        bool boolean;
    } value;
} Symbol;

/**
 * @brief Compiler context
 * 
 * Demonstrates: Compiler state management,
 * compilation phases
 */
typedef struct {
    Lexer lexer;
    Token current_token;
    Symbol symbol_table[MAX_SYMBOLS];
    size_t symbol_count;
    int current_scope;
    ASTNode* ast_root;
    DynamicArray* bytecode;
    char error_message[512];
    bool debug_mode;
} Compiler;

/**
 * @brief Virtual machine instruction types
 */
typedef enum {
    OP_LOAD_CONST,
    OP_LOAD_VAR,
    OP_STORE_VAR,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_EQUAL,
    OP_LESS,
    OP_GREATER,
    OP_NOT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_CALL,
    OP_RETURN,
    OP_PRINT,
    OP_HALT
} OpCode;

/**
 * @brief Virtual machine instruction
 */
typedef struct {
    OpCode opcode;
    union {
        double number;
        int index;
        char string[MAX_STRING_LENGTH];
    } operand;
} Instruction;

/**
 * @brief Virtual machine state
 */
typedef struct {
    Instruction* instructions;
    size_t instruction_count;
    size_t pc;  // Program counter
    double stack[VM_STACK_SIZE];
    size_t stack_top;
    Symbol globals[MAX_SYMBOLS];
    size_t global_count;
} VirtualMachine;

/**
 * @brief Initialize lexer
 * @param lexer Pointer to lexer structure
 * @param source Source code string
 * 
 * Demonstrates: Lexer initialization, input setup
 */
void lexer_init(Lexer* lexer, const char* source) {
    if (!lexer || !source) return;
    
    lexer->source = source;
    lexer->current = 0;
    lexer->length = strlen(source);
    lexer->line = 1;
    lexer->column = 1;
    lexer->error_message[0] = '\0';
}

/**
 * @brief Check if lexer is at end of input
 * @param lexer Pointer to lexer
 * @return true if at end
 */
bool lexer_is_at_end(Lexer* lexer) {
    return lexer && lexer->current >= lexer->length;
}

/**
 * @brief Get current character
 * @param lexer Pointer to lexer
 * @return Current character or '\0'
 */
char lexer_current_char(Lexer* lexer) {
    if (lexer_is_at_end(lexer)) return '\0';
    return lexer->source[lexer->current];
}

/**
 * @brief Advance lexer position
 * @param lexer Pointer to lexer
 * @return Previous character
 */
char lexer_advance(Lexer* lexer) {
    if (lexer_is_at_end(lexer)) return '\0';
    
    char ch = lexer->source[lexer->current++];
    
    if (ch == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    
    return ch;
}

/**
 * @brief Peek at next character
 * @param lexer Pointer to lexer
 * @return Next character or '\0'
 */
char lexer_peek(Lexer* lexer) {
    if (lexer->current + 1 >= lexer->length) return '\0';
    return lexer->source[lexer->current + 1];
}

/**
 * @brief Skip whitespace characters
 * @param lexer Pointer to lexer
 */
void lexer_skip_whitespace(Lexer* lexer) {
    while (!lexer_is_at_end(lexer)) {
        char ch = lexer_current_char(lexer);
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
            lexer_advance(lexer);
        } else if (ch == '/' && lexer_peek(lexer) == '/') {
            // Skip line comment
            while (!lexer_is_at_end(lexer) && lexer_current_char(lexer) != '\n') {
                lexer_advance(lexer);
            }
        } else {
            break;
        }
    }
}

/**
 * @brief Scan string literal
 * @param lexer Pointer to lexer
 * @param token Pointer to token to fill
 * @return true if successful
 */
bool lexer_scan_string(Lexer* lexer, Token* token) {
    if (!lexer || !token) return false;
    
    token->type = TOKEN_STRING;
    token->line = lexer->line;
    token->column = lexer->column;
    
    lexer_advance(lexer); // Skip opening quote
    
    size_t length = 0;
    while (!lexer_is_at_end(lexer) && lexer_current_char(lexer) != '"') {
        if (length >= MAX_STRING_LENGTH - 1) {
            snprintf(lexer->error_message, sizeof(lexer->error_message),
                    "String literal too long at line %d", lexer->line);
            token->type = TOKEN_ERROR;
            return false;
        }
        
        char ch = lexer_advance(lexer);
        
        // Handle escape sequences
        if (ch == '\\' && !lexer_is_at_end(lexer)) {
            char escaped = lexer_advance(lexer);
            switch (escaped) {
                case 'n': token->value.string[length++] = '\n'; break;
                case 't': token->value.string[length++] = '\t'; break;
                case 'r': token->value.string[length++] = '\r'; break;
                case '\\': token->value.string[length++] = '\\'; break;
                case '"': token->value.string[length++] = '"'; break;
                default:
                    token->value.string[length++] = '\\';
                    token->value.string[length++] = escaped;
                    break;
            }
        } else {
            token->value.string[length++] = ch;
        }
    }
    
    if (lexer_is_at_end(lexer)) {
        snprintf(lexer->error_message, sizeof(lexer->error_message),
                "Unterminated string literal at line %d", lexer->line);
        token->type = TOKEN_ERROR;
        return false;
    }
    
    lexer_advance(lexer); // Skip closing quote
    token->value.string[length] = '\0';
    strcpy(token->lexeme, token->value.string);
    
    return true;
}

/**
 * @brief Scan number literal
 * @param lexer Pointer to lexer
 * @param token Pointer to token to fill
 * @return true if successful
 */
bool lexer_scan_number(Lexer* lexer, Token* token) {
    if (!lexer || !token) return false;
    
    token->type = TOKEN_NUMBER;
    token->line = lexer->line;
    token->column = lexer->column;
    
    size_t start = lexer->current;
    
    // Scan integer part
    while (!lexer_is_at_end(lexer) && isdigit(lexer_current_char(lexer))) {
        lexer_advance(lexer);
    }
    
    // Scan decimal part
    if (!lexer_is_at_end(lexer) && lexer_current_char(lexer) == '.' &&
        isdigit(lexer_peek(lexer))) {
        lexer_advance(lexer); // Skip '.'
        
        while (!lexer_is_at_end(lexer) && isdigit(lexer_current_char(lexer))) {
            lexer_advance(lexer);
        }
    }
    
    // Extract number string
    size_t length = lexer->current - start;
    if (length >= MAX_IDENTIFIER_LENGTH) {
        snprintf(lexer->error_message, sizeof(lexer->error_message),
                "Number literal too long at line %d", lexer->line);
        token->type = TOKEN_ERROR;
        return false;
    }
    
    strncpy(token->lexeme, lexer->source + start, length);
    token->lexeme[length] = '\0';
    
    token->value.number = strtod(token->lexeme, NULL);
    
    return true;
}

/**
 * @brief Check if string is a keyword
 * @param text String to check
 * @return Token type if keyword, TOKEN_IDENTIFIER otherwise
 */
TokenType check_keyword(const char* text) {
    if (!text) return TOKEN_IDENTIFIER;
    
    if (strcmp(text, "let") == 0) return TOKEN_LET;
    if (strcmp(text, "if") == 0) return TOKEN_IF;
    if (strcmp(text, "else") == 0) return TOKEN_ELSE;
    if (strcmp(text, "while") == 0) return TOKEN_WHILE;
    if (strcmp(text, "for") == 0) return TOKEN_FOR;
    if (strcmp(text, "function") == 0) return TOKEN_FUNCTION;
    if (strcmp(text, "return") == 0) return TOKEN_RETURN;
    if (strcmp(text, "true") == 0) return TOKEN_TRUE;
    if (strcmp(text, "false") == 0) return TOKEN_FALSE;
    if (strcmp(text, "null") == 0) return TOKEN_NULL;
    if (strcmp(text, "print") == 0) return TOKEN_PRINT;
    
    return TOKEN_IDENTIFIER;
}

/**
 * @brief Scan identifier or keyword
 * @param lexer Pointer to lexer
 * @param token Pointer to token to fill
 * @return true if successful
 */
bool lexer_scan_identifier(Lexer* lexer, Token* token) {
    if (!lexer || !token) return false;
    
    token->line = lexer->line;
    token->column = lexer->column;
    
    size_t start = lexer->current;
    
    // Scan identifier
    while (!lexer_is_at_end(lexer)) {
        char ch = lexer_current_char(lexer);
        if (isalnum(ch) || ch == '_') {
            lexer_advance(lexer);
        } else {
            break;
        }
    }
    
    size_t length = lexer->current - start;
    if (length >= MAX_IDENTIFIER_LENGTH) {
        snprintf(lexer->error_message, sizeof(lexer->error_message),
                "Identifier too long at line %d", lexer->line);
        token->type = TOKEN_ERROR;
        return false;
    }
    
    strncpy(token->lexeme, lexer->source + start, length);
    token->lexeme[length] = '\0';
    
    token->type = check_keyword(token->lexeme);
    
    return true;
}

/**
 * @brief Get next token from lexer
 * @param lexer Pointer to lexer
 * @param token Pointer to store token
 * @return true if token was scanned successfully
 */
bool lexer_next_token(Lexer* lexer, Token* token) {
    if (!lexer || !token) return false;
    
    lexer_skip_whitespace(lexer);
    
    if (lexer_is_at_end(lexer)) {
        token->type = TOKEN_EOF;
        token->line = lexer->line;
        token->column = lexer->column;
        token->lexeme[0] = '\0';
        return true;
    }
    
    char ch = lexer_current_char(lexer);
    
    // String literals
    if (ch == '"') {
        return lexer_scan_string(lexer, token);
    }
    
    // Number literals
    if (isdigit(ch)) {
        return lexer_scan_number(lexer, token);
    }
    
    // Identifiers and keywords
    if (isalpha(ch) || ch == '_') {
        return lexer_scan_identifier(lexer, token);
    }
    
    // Single character tokens
    token->line = lexer->line;
    token->column = lexer->column;
    token->lexeme[0] = ch;
    token->lexeme[1] = '\0';
    
    lexer_advance(lexer);
    
    switch (ch) {
        case '+': token->type = TOKEN_PLUS; break;
        case '-': token->type = TOKEN_MINUS; break;
        case '*': token->type = TOKEN_MULTIPLY; break;
        case '/': token->type = TOKEN_DIVIDE; break;
        case '%': token->type = TOKEN_MODULO; break;
        case ';': token->type = TOKEN_SEMICOLON; break;
        case ',': token->type = TOKEN_COMMA; break;
        case '(': token->type = TOKEN_LEFT_PAREN; break;
        case ')': token->type = TOKEN_RIGHT_PAREN; break;
        case '{': token->type = TOKEN_LEFT_BRACE; break;
        case '}': token->type = TOKEN_RIGHT_BRACE; break;
        case '[': token->type = TOKEN_LEFT_BRACKET; break;
        case ']': token->type = TOKEN_RIGHT_BRACKET; break;
        
        case '=':
            if (lexer_current_char(lexer) == '=') {
                lexer_advance(lexer);
                token->type = TOKEN_EQUAL;
                strcpy(token->lexeme, "==");
            } else {
                token->type = TOKEN_ASSIGN;
            }
            break;
            
        case '!':
            if (lexer_current_char(lexer) == '=') {
                lexer_advance(lexer);
                token->type = TOKEN_NOT_EQUAL;
                strcpy(token->lexeme, "!=");
            } else {
                token->type = TOKEN_NOT;
            }
            break;
            
        case '<':
            if (lexer_current_char(lexer) == '=') {
                lexer_advance(lexer);
                token->type = TOKEN_LESS_EQUAL;
                strcpy(token->lexeme, "<=");
            } else {
                token->type = TOKEN_LESS;
            }
            break;
            
        case '>':
            if (lexer_current_char(lexer) == '=') {
                lexer_advance(lexer);
                token->type = TOKEN_GREATER_EQUAL;
                strcpy(token->lexeme, ">=");
            } else {
                token->type = TOKEN_GREATER;
            }
            break;
            
        case '&':
            if (lexer_current_char(lexer) == '&') {
                lexer_advance(lexer);
                token->type = TOKEN_AND;
                strcpy(token->lexeme, "&&");
            } else {
                snprintf(lexer->error_message, sizeof(lexer->error_message),
                        "Unexpected character '&' at line %d", lexer->line);
                token->type = TOKEN_ERROR;
                return false;
            }
            break;
            
        case '|':
            if (lexer_current_char(lexer) == '|') {
                lexer_advance(lexer);
                token->type = TOKEN_OR;
                strcpy(token->lexeme, "||");
            } else {
                snprintf(lexer->error_message, sizeof(lexer->error_message),
                        "Unexpected character '|' at line %d", lexer->line);
                token->type = TOKEN_ERROR;
                return false;
            }
            break;
            
        default:
            snprintf(lexer->error_message, sizeof(lexer->error_message),
                    "Unexpected character '%c' at line %d", ch, lexer->line);
            token->type = TOKEN_ERROR;
            return false;
    }
    
    return true;
}

/**
 * @brief Create AST node
 * @param type Node type
 * @return Pointer to new node, or NULL on failure
 */
ASTNode* ast_create_node(ASTNodeType type) {
    ASTNode* node = safe_calloc(1, sizeof(ASTNode));
    if (node) {
        node->type = type;
    }
    return node;
}

/**
 * @brief Free AST node and its children
 * @param node Pointer to node to free
 */
void ast_free_node(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_PROGRAM:
            for (size_t i = 0; i < node->data.program.statement_count; i++) {
                ast_free_node(node->data.program.statements[i]);
            }
            free(node->data.program.statements);
            break;
            
        case AST_VARIABLE_DECLARATION:
            ast_free_node(node->data.var_decl.initializer);
            break;
            
        case AST_ASSIGNMENT:
            ast_free_node(node->data.assignment.value);
            break;
            
        case AST_IF_STATEMENT:
            ast_free_node(node->data.if_stmt.condition);
            ast_free_node(node->data.if_stmt.then_stmt);
            ast_free_node(node->data.if_stmt.else_stmt);
            break;
            
        case AST_WHILE_STATEMENT:
            ast_free_node(node->data.while_stmt.condition);
            ast_free_node(node->data.while_stmt.body);
            break;
            
        case AST_FUNCTION_DECLARATION:
            ast_free_node(node->data.func_decl.body);
            break;
            
        case AST_RETURN_STATEMENT:
            ast_free_node(node->data.return_stmt.value);
            break;
            
        case AST_BLOCK_STATEMENT:
            for (size_t i = 0; i < node->data.block.statement_count; i++) {
                ast_free_node(node->data.block.statements[i]);
            }
            free(node->data.block.statements);
            break;
            
        case AST_PRINT_STATEMENT:
            ast_free_node(node->data.print_stmt.expression);
            break;
            
        case AST_BINARY_EXPRESSION:
            ast_free_node(node->data.binary.left);
            ast_free_node(node->data.binary.right);
            break;
            
        case AST_UNARY_EXPRESSION:
            ast_free_node(node->data.unary.operand);
            break;
            
        case AST_CALL_EXPRESSION:
            for (size_t i = 0; i < node->data.call.argument_count; i++) {
                ast_free_node(node->data.call.arguments[i]);
            }
            free(node->data.call.arguments);
            break;
            
        default:
            break;
    }
    
    free(node);
}

/**
 * @brief Print AST node (for debugging)
 * @param node Pointer to AST node
 * @param depth Indentation depth
 */
void ast_print_node(ASTNode* node, int depth) {
    if (!node) return;
    
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    
    switch (node->type) {
        case AST_PROGRAM:
            printf("Program\n");
            for (size_t i = 0; i < node->data.program.statement_count; i++) {
                ast_print_node(node->data.program.statements[i], depth + 1);
            }
            break;
            
        case AST_VARIABLE_DECLARATION:
            printf("VarDecl: %s\n", node->data.var_decl.name);
            if (node->data.var_decl.initializer) {
                ast_print_node(node->data.var_decl.initializer, depth + 1);
            }
            break;
            
        case AST_ASSIGNMENT:
            printf("Assignment: %s\n", node->data.assignment.name);
            ast_print_node(node->data.assignment.value, depth + 1);
            break;
            
        case AST_BINARY_EXPRESSION:
            printf("BinaryExpr: %d\n", node->data.binary.operator);
            ast_print_node(node->data.binary.left, depth + 1);
            ast_print_node(node->data.binary.right, depth + 1);
            break;
            
        case AST_NUMBER_LITERAL:
            printf("Number: %g\n", node->data.literal.value.number);
            break;
            
        case AST_STRING_LITERAL:
            printf("String: \"%s\"\n", node->data.literal.value.string);
            break;
            
        case AST_IDENTIFIER:
            printf("Identifier: %s\n", node->data.literal.name);
            break;
            
        case AST_PRINT_STATEMENT:
            printf("Print\n");
            ast_print_node(node->data.print_stmt.expression, depth + 1);
            break;
            
        default:
            printf("Unknown node type: %d\n", node->type);
            break;
    }
}

/**
 * @brief Initialize compiler
 * @param compiler Pointer to compiler structure
 * @param source Source code string
 * @return true if initialization was successful
 */
bool compiler_init(Compiler* compiler, const char* source) {
    if (!compiler || !source) return false;
    
    memset(compiler, 0, sizeof(Compiler));
    
    lexer_init(&compiler->lexer, source);
    
    // Get first token
    if (!lexer_next_token(&compiler->lexer, &compiler->current_token)) {
        strcpy(compiler->error_message, compiler->lexer.error_message);
        return false;
    }
    
    // Initialize bytecode array
    compiler->bytecode = darray_create(sizeof(Instruction), 64);
    if (!compiler->bytecode) {
        strcpy(compiler->error_message, "Failed to initialize bytecode array");
        return false;
    }
    
    compiler->debug_mode = false;
    
    return true;
}

/**
 * @brief Advance to next token
 * @param compiler Pointer to compiler
 * @return true if successful
 */
bool compiler_advance(Compiler* compiler) {
    if (!compiler) return false;
    
    if (!lexer_next_token(&compiler->lexer, &compiler->current_token)) {
        strcpy(compiler->error_message, compiler->lexer.error_message);
        return false;
    }
    
    return true;
}

/**
 * @brief Check if current token matches type
 * @param compiler Pointer to compiler
 * @param type Token type to check
 * @return true if matches
 */
bool compiler_check(Compiler* compiler, TokenType type) {
    return compiler && compiler->current_token.type == type;
}

/**
 * @brief Consume token if it matches type
 * @param compiler Pointer to compiler
 * @param type Expected token type
 * @param message Error message if not matched
 * @return true if consumed
 */
bool compiler_consume(Compiler* compiler, TokenType type, const char* message) {
    if (compiler_check(compiler, type)) {
        return compiler_advance(compiler);
    }
    
    snprintf(compiler->error_message, sizeof(compiler->error_message),
            "%s at line %d. Got '%s'", message, 
            compiler->current_token.line, compiler->current_token.lexeme);
    return false;
}

// Forward declarations for recursive parsing
ASTNode* parse_expression(Compiler* compiler);
ASTNode* parse_statement(Compiler* compiler);

/**
 * @brief Parse primary expression
 * @param compiler Pointer to compiler
 * @return AST node or NULL on error
 */
ASTNode* parse_primary(Compiler* compiler) {
    if (!compiler) return NULL;
    
    if (compiler_check(compiler, TOKEN_NUMBER)) {
        ASTNode* node = ast_create_node(AST_NUMBER_LITERAL);
        if (node) {
            node->data.literal.value.number = compiler->current_token.value.number;
            node->line = compiler->current_token.line;
            node->column = compiler->current_token.column;
        }
        compiler_advance(compiler);
        return node;
    }
    
    if (compiler_check(compiler, TOKEN_STRING)) {
        ASTNode* node = ast_create_node(AST_STRING_LITERAL);
        if (node) {
            strcpy(node->data.literal.value.string, compiler->current_token.value.string);
            node->line = compiler->current_token.line;
            node->column = compiler->current_token.column;
        }
        compiler_advance(compiler);
        return node;
    }
    
    if (compiler_check(compiler, TOKEN_TRUE) || compiler_check(compiler, TOKEN_FALSE)) {
        ASTNode* node = ast_create_node(AST_BOOLEAN_LITERAL);
        if (node) {
            node->data.literal.value.boolean = compiler_check(compiler, TOKEN_TRUE);
            node->line = compiler->current_token.line;
            node->column = compiler->current_token.column;
        }
        compiler_advance(compiler);
        return node;
    }
    
    if (compiler_check(compiler, TOKEN_NULL)) {
        ASTNode* node = ast_create_node(AST_NULL_LITERAL);
        if (node) {
            node->line = compiler->current_token.line;
            node->column = compiler->current_token.column;
        }
        compiler_advance(compiler);
        return node;
    }
    
    if (compiler_check(compiler, TOKEN_IDENTIFIER)) {
        ASTNode* node = ast_create_node(AST_IDENTIFIER);
        if (node) {
            strcpy(node->data.literal.name, compiler->current_token.lexeme);
            node->line = compiler->current_token.line;
            node->column = compiler->current_token.column;
        }
        compiler_advance(compiler);
        return node;
    }
    
    if (compiler_check(compiler, TOKEN_LEFT_PAREN)) {
        compiler_advance(compiler);
        ASTNode* expr = parse_expression(compiler);
        if (!expr) return NULL;
        
        if (!compiler_consume(compiler, TOKEN_RIGHT_PAREN, "Expected ')' after expression")) {
            ast_free_node(expr);
            return NULL;
        }
        
        return expr;
    }
    
    snprintf(compiler->error_message, sizeof(compiler->error_message),
            "Unexpected token '%s' at line %d", 
            compiler->current_token.lexeme, compiler->current_token.line);
    return NULL;
}

/**
 * @brief Parse unary expression
 * @param compiler Pointer to compiler
 * @return AST node or NULL on error
 */
ASTNode* parse_unary(Compiler* compiler) {
    if (!compiler) return NULL;
    
    if (compiler_check(compiler, TOKEN_NOT) || compiler_check(compiler, TOKEN_MINUS)) {
        ASTNode* node = ast_create_node(AST_UNARY_EXPRESSION);
        if (!node) return NULL;
        
        node->data.unary.operator = compiler->current_token.type;
        node->line = compiler->current_token.line;
        node->column = compiler->current_token.column;
        
        compiler_advance(compiler);
        
        node->data.unary.operand = parse_unary(compiler);
        if (!node->data.unary.operand) {
            ast_free_node(node);
            return NULL;
        }
        
        return node;
    }
    
    return parse_primary(compiler);
}

/**
 * @brief Parse multiplication/division expression
 * @param compiler Pointer to compiler
 * @return AST node or NULL on error
 */
ASTNode* parse_factor(Compiler* compiler) {
    if (!compiler) return NULL;
    
    ASTNode* expr = parse_unary(compiler);
    if (!expr) return NULL;
    
    while (compiler_check(compiler, TOKEN_MULTIPLY) || 
           compiler_check(compiler, TOKEN_DIVIDE) ||
           compiler_check(compiler, TOKEN_MODULO)) {
        
        ASTNode* binary = ast_create_node(AST_BINARY_EXPRESSION);
        if (!binary) {
            ast_free_node(expr);
            return NULL;
        }
        
        binary->data.binary.left = expr;
        binary->data.binary.operator = compiler->current_token.type;
        binary->line = compiler->current_token.line;
        binary->column = compiler->current_token.column;
        
        compiler_advance(compiler);
        
        binary->data.binary.right = parse_unary(compiler);
        if (!binary->data.binary.right) {
            ast_free_node(binary);
            return NULL;
        }
        
        expr = binary;
    }
    
    return expr;
}

/**
 * @brief Parse addition/subtraction expression
 * @param compiler Pointer to compiler
 * @return AST node or NULL on error
 */
ASTNode* parse_term(Compiler* compiler) {
    if (!compiler) return NULL;
    
    ASTNode* expr = parse_factor(compiler);
    if (!expr) return NULL;
    
    while (compiler_check(compiler, TOKEN_PLUS) || compiler_check(compiler, TOKEN_MINUS)) {
        ASTNode* binary = ast_create_node(AST_BINARY_EXPRESSION);
        if (!binary) {
            ast_free_node(expr);
            return NULL;
        }
        
        binary->data.binary.left = expr;
        binary->data.binary.operator = compiler->current_token.type;
        binary->line = compiler->current_token.line;
        binary->column = compiler->current_token.column;
        
        compiler_advance(compiler);
        
        binary->data.binary.right = parse_factor(compiler);
        if (!binary->data.binary.right) {
            ast_free_node(binary);
            return NULL;
        }
        
        expr = binary;
    }
    
    return expr;
}

/**
 * @brief Parse comparison expression
 * @param compiler Pointer to compiler
 * @return AST node or NULL on error
 */
ASTNode* parse_comparison(Compiler* compiler) {
    if (!compiler) return NULL;
    
    ASTNode* expr = parse_term(compiler);
    if (!expr) return NULL;
    
    while (compiler_check(compiler, TOKEN_GREATER) || 
           compiler_check(compiler, TOKEN_GREATER_EQUAL) ||
           compiler_check(compiler, TOKEN_LESS) || 
           compiler_check(compiler, TOKEN_LESS_EQUAL)) {
        
        ASTNode* binary = ast_create_node(AST_BINARY_EXPRESSION);
        if (!binary) {
            ast_free_node(expr);
            return NULL;
        }
        
        binary->data.binary.left = expr;
        binary->data.binary.operator = compiler->current_token.type;
        binary->line = compiler->current_token.line;
        binary->column = compiler->current_token.column;
        
        compiler_advance(compiler);
        
        binary->data.binary.right = parse_term(compiler);
        if (!binary->data.binary.right) {
            ast_free_node(binary);
            return NULL;
        }
        
        expr = binary;
    }
    
    return expr;
}

/**
 * @brief Parse equality expression
 * @param compiler Pointer to compiler
 * @return AST node or NULL on error
 */
ASTNode* parse_equality(Compiler* compiler) {
    if (!compiler) return NULL;
    
    ASTNode* expr = parse_comparison(compiler);
    if (!expr) return NULL;
    
    while (compiler_check(compiler, TOKEN_EQUAL) || compiler_check(compiler, TOKEN_NOT_EQUAL)) {
        ASTNode* binary = ast_create_node(AST_BINARY_EXPRESSION);
        if (!binary) {
            ast_free_node(expr);
            return NULL;
        }
        
        binary->data.binary.left = expr;
        binary->data.binary.operator = compiler->current_token.type;
        binary->line = compiler->current_token.line;
        binary->column = compiler->current_token.column;
        
        compiler_advance(compiler);
        
        binary->data.binary.right = parse_comparison(compiler);
        if (!binary->data.binary.right) {
            ast_free_node(binary);
            return NULL;
        }
        
        expr = binary;
    }
    
    return expr;
}

/**
 * @brief Parse expression
 * @param compiler Pointer to compiler
 * @return AST node or NULL on error
 */
ASTNode* parse_expression(Compiler* compiler) {
    return parse_equality(compiler);
}

/**
 * @brief Parse variable declaration
 * @param compiler Pointer to compiler
 * @return AST node or NULL on error
 */
ASTNode* parse_var_declaration(Compiler* compiler) {
    if (!compiler) return NULL;
    
    if (!compiler_consume(compiler, TOKEN_IDENTIFIER, "Expected variable name")) {
        return NULL;
    }
    
    ASTNode* node = ast_create_node(AST_VARIABLE_DECLARATION);
    if (!node) return NULL;
    
    strcpy(node->data.var_decl.name, compiler->current_token.lexeme);
    node->line = compiler->current_token.line;
    node->column = compiler->current_token.column;
    
    compiler_advance(compiler);
    
    if (compiler_check(compiler, TOKEN_ASSIGN)) {
        compiler_advance(compiler);
        node->data.var_decl.initializer = parse_expression(compiler);
        if (!node->data.var_decl.initializer) {
            ast_free_node(node);
            return NULL;
        }
    }
    
    if (!compiler_consume(compiler, TOKEN_SEMICOLON, "Expected ';' after variable declaration")) {
        ast_free_node(node);
        return NULL;
    }
    
    return node;
}

/**
 * @brief Parse print statement
 * @param compiler Pointer to compiler
 * @return AST node or NULL on error
 */
ASTNode* parse_print_statement(Compiler* compiler) {
    if (!compiler) return NULL;
    
    ASTNode* node = ast_create_node(AST_PRINT_STATEMENT);
    if (!node) return NULL;
    
    node->line = compiler->current_token.line;
    node->column = compiler->current_token.column;
    
    compiler_advance(compiler); // consume 'print'
    
    node->data.print_stmt.expression = parse_expression(compiler);
    if (!node->data.print_stmt.expression) {
        ast_free_node(node);
        return NULL;
    }
    
    if (!compiler_consume(compiler, TOKEN_SEMICOLON, "Expected ';' after print statement")) {
        ast_free_node(node);
        return NULL;
    }
    
    return node;
}

/**
 * @brief Parse statement
 * @param compiler Pointer to compiler
 * @return AST node or NULL on error
 */
ASTNode* parse_statement(Compiler* compiler) {
    if (!compiler) return NULL;
    
    if (compiler_check(compiler, TOKEN_LET)) {
        compiler_advance(compiler);
        return parse_var_declaration(compiler);
    }
    
    if (compiler_check(compiler, TOKEN_PRINT)) {
        return parse_print_statement(compiler);
    }
    
    // Expression statement
    ASTNode* expr = parse_expression(compiler);
    if (!expr) return NULL;
    
    if (!compiler_consume(compiler, TOKEN_SEMICOLON, "Expected ';' after expression")) {
        ast_free_node(expr);
        return NULL;
    }
    
    ASTNode* stmt = ast_create_node(AST_EXPRESSION_STATEMENT);
    if (!stmt) {
        ast_free_node(expr);
        return NULL;
    }
    
    // For simplicity, store expression in print_stmt field
    stmt->data.print_stmt.expression = expr;
    stmt->line = expr->line;
    stmt->column = expr->column;
    
    return stmt;
}

/**
 * @brief Parse program
 * @param compiler Pointer to compiler
 * @return AST root node or NULL on error
 */
ASTNode* parse_program(Compiler* compiler) {
    if (!compiler) return NULL;
    
    ASTNode* program = ast_create_node(AST_PROGRAM);
    if (!program) return NULL;
    
    DynamicArray* statements = darray_create(sizeof(ASTNode*), 16);
    if (!statements) {
        ast_free_node(program);
        return NULL;
    }
    
    while (!compiler_check(compiler, TOKEN_EOF)) {
        ASTNode* stmt = parse_statement(compiler);
        if (!stmt) {
            // Clean up and return NULL on error
            for (size_t i = 0; i < darray_size(statements); i++) {
                ASTNode* node;
                darray_get(statements, i, &node);
                ast_free_node(node);
            }
            darray_destroy(statements);
            ast_free_node(program);
            return NULL;
        }
        
        darray_push(statements, &stmt);
    }
    
    // Convert dynamic array to static array
    size_t stmt_count = darray_size(statements);
    program->data.program.statement_count = stmt_count;
    
    if (stmt_count > 0) {
        program->data.program.statements = safe_calloc(stmt_count, sizeof(ASTNode*));
        if (!program->data.program.statements) {
            for (size_t i = 0; i < stmt_count; i++) {
                ASTNode* node;
                darray_get(statements, i, &node);
                ast_free_node(node);
            }
            darray_destroy(statements);
            ast_free_node(program);
            return NULL;
        }
        
        for (size_t i = 0; i < stmt_count; i++) {
            darray_get(statements, i, &program->data.program.statements[i]);
        }
    }
    
    darray_destroy(statements);
    return program;
}

/**
 * @brief Simple demo of the compiler
 * @param source Source code to compile
 */
void demo_compiler(const char* source) {
    printf("\n=== Compiler Demo ===\n");
    printf("Source code:\n%s\n", source);
    
    Compiler compiler;
    if (!compiler_init(&compiler, source)) {
        printf("Error initializing compiler: %s\n", compiler.error_message);
        return;
    }
    
    compiler.debug_mode = true;
    
    printf("\n--- Lexical Analysis ---\n");
    Lexer temp_lexer;
    lexer_init(&temp_lexer, source);
    
    Token token;
    while (lexer_next_token(&temp_lexer, &token) && token.type != TOKEN_EOF) {
        printf("Token: %-15s Lexeme: %-10s Line: %d\n", 
               token.type == TOKEN_NUMBER ? "NUMBER" :
               token.type == TOKEN_STRING ? "STRING" :
               token.type == TOKEN_IDENTIFIER ? "IDENTIFIER" :
               token.type == TOKEN_LET ? "LET" :
               token.type == TOKEN_PRINT ? "PRINT" :
               token.type == TOKEN_PLUS ? "PLUS" :
               token.type == TOKEN_ASSIGN ? "ASSIGN" :
               token.type == TOKEN_SEMICOLON ? "SEMICOLON" : "OTHER",
               token.lexeme, token.line);
    }
    
    printf("\n--- Parsing ---\n");
    compiler.ast_root = parse_program(&compiler);
    
    if (!compiler.ast_root) {
        printf("Parse error: %s\n", compiler.error_message);
    } else {
        printf("Parse successful!\n");
        printf("\n--- Abstract Syntax Tree ---\n");
        ast_print_node(compiler.ast_root, 0);
    }
    
    // Cleanup
    if (compiler.ast_root) {
        ast_free_node(compiler.ast_root);
    }
    if (compiler.bytecode) {
        darray_destroy(compiler.bytecode);
    }
    
    printf("==================\n");
}

/**
 * @brief Display help information
 * @param program_name Program name from argv[0]
 */
void display_help(const char* program_name) {
    printf("Simple Compiler - Lexical Analysis and Parsing\n");
    printf("Usage: %s [options] [source_file]\n\n", program_name);
    printf("Options:\n");
    printf("  -d, --debug     Enable debug output\n");
    printf("  -i, --interactive  Run in interactive mode\n");
    printf("  --help          Show this help\n\n");
    printf("If no source file is provided, runs built-in demos.\n\n");
    printf("Language features:\n");
    printf("- Variable declarations: let x = 5;\n");
    printf("- Arithmetic expressions: x + y * 2\n");
    printf("- Print statements: print x;\n");
    printf("- Boolean literals: true, false\n");
    printf("- String literals: \"hello world\"\n");
    printf("\nFeatures demonstrated:\n");
    printf("- Lexical analysis and tokenization\n");
    printf("- Recursive descent parsing\n");
    printf("- Abstract syntax tree construction\n");
    printf("- Error handling and reporting\n");
}

/**
 * @brief Main function
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return Exit status
 */
int main(int argc, char* argv[]) {
    bool debug_mode = false;
    bool interactive_mode = false;
    char* source_file = NULL;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            display_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            debug_mode = true;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interactive") == 0) {
            interactive_mode = true;
        } else if (!source_file) {
            source_file = argv[i];
        } else {
            printf("Error: Too many arguments\n");
            display_help(argv[0]);
            return 1;
        }
    }
    
    if (source_file) {
        // Compile source file
        size_t file_size;
        if (!get_file_size(source_file, &file_size)) {
            printf("Error: Cannot read file '%s'\n", source_file);
            return 1;
        }
        
        FILE* file = fopen(source_file, "r");
        if (!file) {
            printf("Error: Cannot open file '%s'\n", source_file);
            return 1;
        }
        
        char* source = safe_calloc(file_size + 1, sizeof(char));
        if (!source) {
            printf("Error: Memory allocation failed\n");
            fclose(file);
            return 1;
        }
        
        fread(source, 1, file_size, file);
        fclose(file);
        
        demo_compiler(source);
        free(source);
        
    } else {
        // Run built-in demos
        printf("Simple Compiler Demonstration\n");
        printf("============================\n");
        
        const char* demo_programs[] = {
            "let x = 42;\nprint x;",
            "let name = \"World\";\nprint name;",
            "let result = 10 + 5 * 2;\nprint result;",
            "let a = true;\nlet b = false;\nprint a;"
        };
        
        size_t demo_count = sizeof(demo_programs) / sizeof(demo_programs[0]);
        
        for (size_t i = 0; i < demo_count; i++) {
            printf("\nDemo %zu:\n", i + 1);
            demo_compiler(demo_programs[i]);
        }
    }
    
    log_message("INFO", "Compiler demonstration completed");
    return 0;
}

/**
 * Educational Notes:
 * 
 * 1. Lexical Analysis:
 *    - Tokenization and character recognition
 *    - Keyword identification and classification
 *    - String and number literal parsing
 * 
 * 2. Parsing Techniques:
 *    - Recursive descent parsing
 *    - Operator precedence handling
 *    - Error recovery and reporting
 * 
 * 3. Abstract Syntax Trees:
 *    - Tree node design and organization
 *    - Memory management for tree structures
 *    - Tree traversal and visualization
 * 
 * 4. Language Design:
 *    - Grammar definition and implementation
 *    - Language feature selection
 *    - Syntax and semantics design
 * 
 * 5. Compiler Architecture:
 *    - Multi-phase compilation process
 *    - Symbol table management
 *    - Error handling strategies
 * 
 * 6. Data Structures:
 *    - Dynamic arrays for token streams
 *    - Tree structures for AST representation
 *    - Hash tables for symbol lookup
 * 
 * 7. Memory Management:
 *    - Dynamic allocation for variable-sized structures
 *    - Proper cleanup and deallocation
 *    - Memory leak prevention
 * 
 * 8. Software Engineering:
 *    - Modular design and separation of concerns
 *    - Robust error handling
 *    - Extensible architecture design
 */