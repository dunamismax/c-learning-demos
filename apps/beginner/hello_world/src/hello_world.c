/**
 * @file hello_world.c
 * @brief Comprehensive Hello World program demonstrating basic C concepts
 * @author dunamismax
 * @date 2025
 * 
 * This program demonstrates fundamental C programming concepts:
 * - Basic program structure
 * - Header includes
 * - Function declarations and definitions
 * - Variable declarations and initialization
 * - Basic I/O operations
 * - String handling
 * - Command line arguments
 * - Memory management concepts
 * - Error handling
 * - Documentation practices
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * @brief Function to print a greeting message
 * @param name The name to greet
 * 
 * Demonstrates: Function definition, parameter passing,
 * string handling, formatted output
 */
void print_greeting(const char* name) {
    if (name && strlen(name) > 0) {
        printf("Hello, %s! Welcome to C programming!\n", name);
    } else {
        printf("Hello, World! Welcome to C programming!\n");
    }
}

/**
 * @brief Function to demonstrate basic variable types
 * 
 * Demonstrates: Variable declarations, different data types,
 * initialization, formatted output
 */
void demonstrate_variables(void) {
    printf("\n=== Variable Types Demonstration ===\n");
    
    // Integer types
    int integer_var = 42;
    short short_var = 100;
    long long_var = 1000000L;
    
    // Floating point types
    float float_var = 3.14f;
    double double_var = 2.718281828;
    
    // Character types
    char char_var = 'A';
    char string_var[] = "Hello, C!";
    
    // Boolean (using int in C)
    int is_true = 1;
    int is_false = 0;
    
    // Print all variables with their types and sizes
    printf("Integer: %d (size: %zu bytes)\n", integer_var, sizeof(integer_var));
    printf("Short: %hd (size: %zu bytes)\n", short_var, sizeof(short_var));
    printf("Long: %ld (size: %zu bytes)\n", long_var, sizeof(long_var));
    printf("Float: %.2f (size: %zu bytes)\n", float_var, sizeof(float_var));
    printf("Double: %.6f (size: %zu bytes)\n", double_var, sizeof(double_var));
    printf("Character: %c (size: %zu bytes)\n", char_var, sizeof(char_var));
    printf("String: %s (size: %zu bytes)\n", string_var, sizeof(string_var));
    printf("Boolean true: %d, false: %d\n", is_true, is_false);
}

/**
 * @brief Function to demonstrate basic arithmetic operations
 * 
 * Demonstrates: Arithmetic operators, operator precedence,
 * type conversion, mathematical operations
 */
void demonstrate_arithmetic(void) {
    printf("\n=== Arithmetic Operations Demonstration ===\n");
    
    int a = 15;
    int b = 4;
    
    printf("Given: a = %d, b = %d\n", a, b);
    printf("Addition: %d + %d = %d\n", a, b, a + b);
    printf("Subtraction: %d - %d = %d\n", a, b, a - b);
    printf("Multiplication: %d * %d = %d\n", a, b, a * b);
    printf("Division: %d / %d = %d\n", a, b, a / b);
    printf("Modulo: %d %% %d = %d\n", a, b, a % b);
    
    // Floating point division
    printf("Floating point division: %d / %d = %.2f\n", a, b, (float)a / b);
    
    // Compound assignment operators
    int x = 10;
    printf("x = %d\n", x);
    x += 5;
    printf("After x += 5: x = %d\n", x);
    x *= 2;
    printf("After x *= 2: x = %d\n", x);
}

/**
 * @brief Function to demonstrate control flow
 * 
 * Demonstrates: if-else statements, switch statements,
 * loops (for, while, do-while), break and continue
 */
void demonstrate_control_flow(void) {
    printf("\n=== Control Flow Demonstration ===\n");
    
    // if-else statement
    int number = 42;
    if (number > 0) {
        printf("Number %d is positive\n", number);
    } else if (number < 0) {
        printf("Number %d is negative\n", number);
    } else {
        printf("Number is zero\n");
    }
    
    // switch statement
    int day = 3;
    printf("Day %d is: ", day);
    switch (day) {
        case 1: printf("Monday\n"); break;
        case 2: printf("Tuesday\n"); break;
        case 3: printf("Wednesday\n"); break;
        case 4: printf("Thursday\n"); break;
        case 5: printf("Friday\n"); break;
        case 6: printf("Saturday\n"); break;
        case 7: printf("Sunday\n"); break;
        default: printf("Invalid day\n"); break;
    }
    
    // for loop
    printf("Counting from 1 to 5: ");
    for (int i = 1; i <= 5; i++) {
        printf("%d ", i);
    }
    printf("\n");
    
    // while loop
    printf("Countdown: ");
    int countdown = 5;
    while (countdown > 0) {
        printf("%d ", countdown);
        countdown--;
    }
    printf("Go!\n");
    
    // do-while loop
    printf("Do-while demonstration: ");
    int counter = 1;
    do {
        printf("%d ", counter);
        counter++;
    } while (counter <= 3);
    printf("\n");
}

/**
 * @brief Function to demonstrate array operations
 * 
 * Demonstrates: Array declaration, initialization,
 * indexing, iteration, array bounds
 */
void demonstrate_arrays(void) {
    printf("\n=== Array Operations Demonstration ===\n");
    
    // Array declaration and initialization
    int numbers[] = {1, 2, 3, 4, 5};
    int size = sizeof(numbers) / sizeof(numbers[0]);
    
    printf("Array elements: ");
    for (int i = 0; i < size; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    
    // String as character array
    char message[] = "C Programming";
    printf("Message: %s\n", message);
    printf("Message length: %zu\n", strlen(message));
    
    // Character by character access
    printf("Characters: ");
    for (int i = 0; message[i] != '\0'; i++) {
        printf("'%c' ", message[i]);
    }
    printf("\n");
}

/**
 * @brief Function to demonstrate basic pointer concepts
 * 
 * Demonstrates: Pointer declaration, address-of operator,
 * dereference operator, pointer arithmetic
 */
void demonstrate_pointers(void) {
    printf("\n=== Basic Pointer Demonstration ===\n");
    
    int value = 100;
    int* ptr = &value;
    
    printf("Value: %d\n", value);
    printf("Address of value: %p\n", (void*)&value);
    printf("Pointer ptr: %p\n", (void*)ptr);
    printf("Value through pointer: %d\n", *ptr);
    
    // Modifying value through pointer
    *ptr = 200;
    printf("After modifying through pointer: %d\n", value);
    
    // Array and pointer relationship
    int arr[] = {10, 20, 30, 40, 50};
    int* arr_ptr = arr;
    
    printf("Array elements using pointer arithmetic: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", *(arr_ptr + i));
    }
    printf("\n");
}

/**
 * @brief Function to show current date and time
 * 
 * Demonstrates: time functions, struct tm,
 * formatted time output
 */
void show_current_time(void) {
    printf("\n=== Current Date and Time ===\n");
    
    time_t rawtime;
    struct tm* timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    printf("Current date and time: %s", asctime(timeinfo));
    printf("Year: %d, Month: %d, Day: %d\n",
           timeinfo->tm_year + 1900,
           timeinfo->tm_mon + 1,
           timeinfo->tm_mday);
}

/**
 * @brief Function to demonstrate command line argument processing
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * 
 * Demonstrates: Command line arguments, string processing,
 * argument validation
 */
void process_arguments(int argc, char* argv[]) {
    printf("\n=== Command Line Arguments ===\n");
    printf("Number of arguments: %d\n", argc);
    
    for (int i = 0; i < argc; i++) {
        printf("Argument %d: %s\n", i, argv[i]);
    }
    
    if (argc > 1) {
        printf("You provided %d additional argument(s)\n", argc - 1);
    } else {
        printf("No additional arguments provided\n");
        printf("Try running: %s YourName\n", argv[0]);
    }
}

/**
 * @brief Main function - entry point of the program
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @return Exit status (0 for success)
 * 
 * Demonstrates: Main function signature, program entry point,
 * function calls, return values
 */
int main(int argc, char* argv[]) {
    printf("=== Comprehensive Hello World Program ===\n");
    printf("This program demonstrates fundamental C programming concepts.\n");
    
    // Print greeting based on command line argument
    if (argc > 1) {
        print_greeting(argv[1]);
    } else {
        print_greeting(NULL);
    }
    
    // Demonstrate various C concepts
    demonstrate_variables();
    demonstrate_arithmetic();
    demonstrate_control_flow();
    demonstrate_arrays();
    demonstrate_pointers();
    show_current_time();
    process_arguments(argc, argv);
    
    printf("\n=== Program completed successfully! ===\n");
    printf("This program demonstrated:\n");
    printf("- Basic program structure and main function\n");
    printf("- Variable types and declarations\n");
    printf("- Arithmetic operations\n");
    printf("- Control flow statements\n");
    printf("- Arrays and string handling\n");
    printf("- Basic pointer concepts\n");
    printf("- Time and date functions\n");
    printf("- Command line argument processing\n");
    printf("- Function definition and calling\n");
    printf("- Documentation and code organization\n");
    
    return 0;
}

/**
 * Educational Notes:
 * 
 * 1. Program Structure:
 *    - Headers at the top
 *    - Function prototypes (optional for simple programs)
 *    - Function definitions
 *    - Main function as entry point
 * 
 * 2. Memory Management:
 *    - Automatic variables (local variables)
 *    - Stack allocation for local variables
 *    - String literals in read-only memory
 * 
 * 3. Best Practices Demonstrated:
 *    - Comprehensive documentation
 *    - Descriptive variable names
 *    - Error checking for function parameters
 *    - Clear output formatting
 *    - Organized code structure
 * 
 * 4. Compilation:
 *    - Compile with: gcc -o hello_world hello_world.c
 *    - Run with: ./hello_world [optional_name]
 * 
 * 5. Key Learning Points:
 *    - Understanding of basic C syntax
 *    - Variable types and memory usage
 *    - Control flow constructs
 *    - Basic I/O operations
 *    - Introduction to pointers
 *    - Command line argument handling
 */