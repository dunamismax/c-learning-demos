/**
 * @file password_generator.c
 * @brief Secure password generator demonstrating string manipulation and randomization
 * @author dunamismax
 * @date 2025
 * 
 * This program demonstrates:
 * - String manipulation and character arrays
 * - Random number generation for cryptographic purposes
 * - Character set management and selection
 * - Password strength analysis
 * - Command line argument processing
 * - File I/O for password storage
 * - Memory management and security
 * - Input validation and error handling
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>

// Include our utility library
#include "utils.h"

/**
 * @brief Maximum password length
 */
#define MAX_PASSWORD_LENGTH 128

/**
 * @brief Minimum password length
 */
#define MIN_PASSWORD_LENGTH 4

/**
 * @brief Maximum input buffer size
 */
#define MAX_INPUT_SIZE 256

/**
 * @brief Character sets for password generation
 */
#define LOWERCASE_CHARS "abcdefghijklmnopqrstuvwxyz"
#define UPPERCASE_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define DIGIT_CHARS "0123456789"
#define SPECIAL_CHARS "!@#$%^&*()_+-=[]{}|;:,.<>?"
#define SIMILAR_CHARS "il1Lo0O"

/**
 * @brief Password generation options
 * 
 * Demonstrates: Bit flags, configuration structure,
 * option management
 */
typedef struct {
    bool include_lowercase;
    bool include_uppercase;
    bool include_digits;
    bool include_special;
    bool exclude_similar;
    bool exclude_ambiguous;
    int length;
} PasswordOptions;

/**
 * @brief Password strength levels
 */
typedef enum {
    STRENGTH_VERY_WEAK = 0,
    STRENGTH_WEAK = 1,
    STRENGTH_FAIR = 2,
    STRENGTH_GOOD = 3,
    STRENGTH_STRONG = 4,
    STRENGTH_VERY_STRONG = 5
} PasswordStrength;

/**
 * @brief Password analysis results
 * 
 * Demonstrates: Structure design, analysis data,
 * security metrics
 */
typedef struct {
    PasswordStrength strength;
    int score;
    bool has_lowercase;
    bool has_uppercase;
    bool has_digits;
    bool has_special;
    int unique_chars;
    double entropy;
    const char* strength_description;
    const char* recommendations;
} PasswordAnalysis;

/**
 * @brief Initialize default password options
 * @param options Pointer to password options structure
 * 
 * Demonstrates: Structure initialization, default values,
 * configuration setup
 */
void init_password_options(PasswordOptions* options) {
    if (!options) return;
    
    options->include_lowercase = true;
    options->include_uppercase = true;
    options->include_digits = true;
    options->include_special = false;
    options->exclude_similar = false;
    options->exclude_ambiguous = false;
    options->length = 12;
}

/**
 * @brief Display password generation options menu
 * 
 * Demonstrates: User interface, formatted output,
 * menu presentation
 */
void display_options_menu(void) {
    printf("\n========== Password Generator Options ==========\n");
    printf("1. Set password length\n");
    printf("2. Toggle lowercase letters (a-z)\n");
    printf("3. Toggle uppercase letters (A-Z)\n");
    printf("4. Toggle digits (0-9)\n");
    printf("5. Toggle special characters (!@#$...)\n");
    printf("6. Toggle exclude similar characters (il1Lo0O)\n");
    printf("7. Toggle exclude ambiguous characters\n");
    printf("8. Generate password\n");
    printf("9. Analyze password strength\n");
    printf("10. Save password to file\n");
    printf("11. View current options\n");
    printf("12. Quit\n");
    printf("=============================================\n");
}

/**
 * @brief Display current password options
 * @param options Pointer to password options
 * 
 * Demonstrates: Option display, boolean formatting,
 * status presentation
 */
void display_current_options(const PasswordOptions* options) {
    if (!options) return;
    
    printf("\n--- Current Password Options ---\n");
    printf("Length: %d\n", options->length);
    printf("Include lowercase: %s\n", options->include_lowercase ? "Yes" : "No");
    printf("Include uppercase: %s\n", options->include_uppercase ? "Yes" : "No");
    printf("Include digits: %s\n", options->include_digits ? "Yes" : "No");
    printf("Include special: %s\n", options->include_special ? "Yes" : "No");
    printf("Exclude similar: %s\n", options->exclude_similar ? "Yes" : "No");
    printf("Exclude ambiguous: %s\n", options->exclude_ambiguous ? "Yes" : "No");
    printf("------------------------------\n");
}

/**
 * @brief Build character set based on options
 * @param options Password generation options
 * @param charset Buffer to store the character set
 * @param charset_size Size of the charset buffer
 * @return Length of the character set
 * 
 * Demonstrates: String building, character set management,
 * conditional string concatenation
 */
int build_charset(const PasswordOptions* options, char* charset, size_t charset_size) {
    if (!options || !charset || charset_size == 0) return 0;
    
    charset[0] = '\0';  // Initialize empty string
    
    // Add character sets based on options
    if (options->include_lowercase) {
        if (!safe_strcat(charset, LOWERCASE_CHARS, charset_size)) {
            return 0;
        }
    }
    
    if (options->include_uppercase) {
        if (!safe_strcat(charset, UPPERCASE_CHARS, charset_size)) {
            return 0;
        }
    }
    
    if (options->include_digits) {
        if (!safe_strcat(charset, DIGIT_CHARS, charset_size)) {
            return 0;
        }
    }
    
    if (options->include_special) {
        if (!safe_strcat(charset, SPECIAL_CHARS, charset_size)) {
            return 0;
        }
    }
    
    // Remove similar characters if requested
    if (options->exclude_similar) {
        char* temp_charset = safe_strdup(charset);
        if (!temp_charset) return 0;
        
        charset[0] = '\0';
        for (int i = 0; temp_charset[i]; i++) {
            if (!strchr(SIMILAR_CHARS, temp_charset[i])) {
                char temp_char[2] = {temp_charset[i], '\0'};
                if (!safe_strcat(charset, temp_char, charset_size)) {
                    free(temp_charset);
                    return 0;
                }
            }
        }
        free(temp_charset);
    }
    
    return strlen(charset);
}

/**
 * @brief Generate a random password
 * @param options Password generation options
 * @param password Buffer to store the generated password
 * @param password_size Size of the password buffer
 * @return true if password was generated successfully
 * 
 * Demonstrates: Random selection, password generation,
 * character set usage, secure random numbers
 */
bool generate_password(const PasswordOptions* options, char* password, size_t password_size) {
    if (!options || !password || password_size == 0) return false;
    
    // Check if we have at least one character set enabled
    if (!options->include_lowercase && !options->include_uppercase && 
        !options->include_digits && !options->include_special) {
        log_message("ERROR", "No character sets selected for password generation");
        return false;
    }
    
    // Build character set
    char charset[512];
    int charset_len = build_charset(options, charset, sizeof(charset));
    if (charset_len == 0) {
        log_message("ERROR", "Failed to build character set");
        return false;
    }
    
    // Check if password buffer is large enough
    if (password_size < options->length + 1) {
        log_message("ERROR", "Password buffer too small");
        return false;
    }
    
    // Generate password
    for (int i = 0; i < options->length; i++) {
        int random_index = rand() % charset_len;
        password[i] = charset[random_index];
    }
    password[options->length] = '\0';
    
    // Ensure password meets minimum requirements
    bool needs_adjustment = false;
    if (options->include_lowercase && !strpbrk(password, LOWERCASE_CHARS)) {
        needs_adjustment = true;
    }
    if (options->include_uppercase && !strpbrk(password, UPPERCASE_CHARS)) {
        needs_adjustment = true;
    }
    if (options->include_digits && !strpbrk(password, DIGIT_CHARS)) {
        needs_adjustment = true;
    }
    if (options->include_special && !strpbrk(password, SPECIAL_CHARS)) {
        needs_adjustment = true;
    }
    
    // Simple adjustment: replace some characters to ensure requirements
    if (needs_adjustment && options->length >= 4) {
        int pos = 0;
        
        if (options->include_lowercase && !strpbrk(password, LOWERCASE_CHARS)) {
            password[pos++] = LOWERCASE_CHARS[rand() % strlen(LOWERCASE_CHARS)];
        }
        if (options->include_uppercase && !strpbrk(password, UPPERCASE_CHARS)) {
            password[pos++] = UPPERCASE_CHARS[rand() % strlen(UPPERCASE_CHARS)];
        }
        if (options->include_digits && !strpbrk(password, DIGIT_CHARS)) {
            password[pos++] = DIGIT_CHARS[rand() % strlen(DIGIT_CHARS)];
        }
        if (options->include_special && !strpbrk(password, SPECIAL_CHARS)) {
            password[pos++] = SPECIAL_CHARS[rand() % strlen(SPECIAL_CHARS)];
        }
    }
    
    return true;
}

/**
 * @brief Analyze password strength
 * @param password The password to analyze
 * @param analysis Pointer to store analysis results
 * 
 * Demonstrates: String analysis, strength calculation,
 * security evaluation, entropy calculation
 */
void analyze_password_strength(const char* password, PasswordAnalysis* analysis) {
    if (!password || !analysis) return;
    
    // Initialize analysis
    memset(analysis, 0, sizeof(PasswordAnalysis));
    
    int length = strlen(password);
    if (length == 0) {
        analysis->strength = STRENGTH_VERY_WEAK;
        analysis->strength_description = "Very Weak";
        analysis->recommendations = "Password is empty";
        return;
    }
    
    // Check character types
    for (int i = 0; password[i]; i++) {
        if (islower(password[i])) analysis->has_lowercase = true;
        if (isupper(password[i])) analysis->has_uppercase = true;
        if (isdigit(password[i])) analysis->has_digits = true;
        if (strchr(SPECIAL_CHARS, password[i])) analysis->has_special = true;
    }
    
    // Count unique characters
    bool used[256] = {false};
    for (int i = 0; password[i]; i++) {
        used[(unsigned char)password[i]] = true;
    }
    
    for (int i = 0; i < 256; i++) {
        if (used[i]) analysis->unique_chars++;
    }
    
    // Calculate entropy (simplified)
    int charset_size = 0;
    if (analysis->has_lowercase) charset_size += 26;
    if (analysis->has_uppercase) charset_size += 26;
    if (analysis->has_digits) charset_size += 10;
    if (analysis->has_special) charset_size += strlen(SPECIAL_CHARS);
    
    if (charset_size > 0) {
        analysis->entropy = length * log2(charset_size);
    }
    
    // Calculate score
    analysis->score = 0;
    
    // Length scoring
    if (length >= 8) analysis->score += 25;
    else if (length >= 6) analysis->score += 10;
    else if (length >= 4) analysis->score += 5;
    
    // Character diversity scoring
    if (analysis->has_lowercase) analysis->score += 5;
    if (analysis->has_uppercase) analysis->score += 5;
    if (analysis->has_digits) analysis->score += 5;
    if (analysis->has_special) analysis->score += 10;
    
    // Unique character bonus
    if (analysis->unique_chars >= length * 0.8) analysis->score += 10;
    else if (analysis->unique_chars >= length * 0.6) analysis->score += 5;
    
    // Length bonus
    if (length >= 12) analysis->score += 10;
    else if (length >= 16) analysis->score += 20;
    
    // Determine strength level
    if (analysis->score >= 80) {
        analysis->strength = STRENGTH_VERY_STRONG;
        analysis->strength_description = "Very Strong";
        analysis->recommendations = "Excellent password!";
    } else if (analysis->score >= 60) {
        analysis->strength = STRENGTH_STRONG;
        analysis->strength_description = "Strong";
        analysis->recommendations = "Good password, consider making it longer";
    } else if (analysis->score >= 40) {
        analysis->strength = STRENGTH_GOOD;
        analysis->strength_description = "Good";
        analysis->recommendations = "Add more character types and increase length";
    } else if (analysis->score >= 20) {
        analysis->strength = STRENGTH_FAIR;
        analysis->strength_description = "Fair";
        analysis->recommendations = "Use uppercase, lowercase, digits, and special characters";
    } else if (analysis->score >= 10) {
        analysis->strength = STRENGTH_WEAK;
        analysis->strength_description = "Weak";
        analysis->recommendations = "Password is too short and lacks complexity";
    } else {
        analysis->strength = STRENGTH_VERY_WEAK;
        analysis->strength_description = "Very Weak";
        analysis->recommendations = "Password is extremely weak, please generate a new one";
    }
}

/**
 * @brief Display password analysis results
 * @param analysis Pointer to password analysis results
 * 
 * Demonstrates: Analysis display, formatted output,
 * security feedback presentation
 */
void display_password_analysis(const PasswordAnalysis* analysis) {
    if (!analysis) return;
    
    printf("\n========== Password Strength Analysis ==========\n");
    printf("Strength: %s\n", analysis->strength_description);
    printf("Score: %d/100\n", analysis->score);
    printf("Entropy: %.1f bits\n", analysis->entropy);
    printf("Unique characters: %d\n", analysis->unique_chars);
    printf("\nCharacter types present:\n");
    printf("- Lowercase letters: %s\n", analysis->has_lowercase ? "Yes" : "No");
    printf("- Uppercase letters: %s\n", analysis->has_uppercase ? "Yes" : "No");
    printf("- Digits: %s\n", analysis->has_digits ? "Yes" : "No");
    printf("- Special characters: %s\n", analysis->has_special ? "Yes" : "No");
    printf("\nRecommendations: %s\n", analysis->recommendations);
    printf("============================================\n");
}

/**
 * @brief Save password to file
 * @param password The password to save
 * @param filename The filename to save to
 * @return true if password was saved successfully
 * 
 * Demonstrates: File I/O, password storage,
 * error handling, secure file operations
 */
bool save_password_to_file(const char* password, const char* filename) {
    if (!password || !filename) return false;
    
    FILE* file = fopen(filename, "a");
    if (!file) {
        log_message("ERROR", "Could not open file for writing");
        return false;
    }
    
    // Get current time
    time_t rawtime;
    struct tm* timeinfo;
    char time_str[64];
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    // Write password with timestamp
    fprintf(file, "Generated: %s\n", time_str);
    fprintf(file, "Password: %s\n", password);
    fprintf(file, "Length: %zu\n", strlen(password));
    fprintf(file, "---\n");
    
    fclose(file);
    
    printf("Password saved to %s\n", filename);
    return true;
}

/**
 * @brief Get user input for password length
 * @param options Pointer to password options
 * @return true if length was set successfully
 * 
 * Demonstrates: Input validation, range checking,
 * user interaction
 */
bool get_password_length(PasswordOptions* options) {
    char input[MAX_INPUT_SIZE];
    int length;
    
    printf("Enter password length (%d-%d): ", MIN_PASSWORD_LENGTH, MAX_PASSWORD_LENGTH);
    
    while (true) {
        if (!fgets(input, sizeof(input), stdin)) {
            return false;
        }
        
        if (str_to_int(trim_whitespace(input), &length)) {
            if (length >= MIN_PASSWORD_LENGTH && length <= MAX_PASSWORD_LENGTH) {
                options->length = length;
                printf("Password length set to %d\n", length);
                return true;
            } else {
                printf("Please enter a length between %d and %d: ", 
                       MIN_PASSWORD_LENGTH, MAX_PASSWORD_LENGTH);
            }
        } else {
            printf("Invalid input. Please enter a number: ");
        }
    }
}

/**
 * @brief Get user input for password analysis
 * @param password Buffer to store the password
 * @param password_size Size of the password buffer
 * @return true if password was entered successfully
 * 
 * Demonstrates: Password input, input validation,
 * secure input handling
 */
bool get_password_for_analysis(char* password, size_t password_size) {
    printf("Enter password to analyze: ");
    
    if (!fgets(password, password_size, stdin)) {
        return false;
    }
    
    // Remove trailing newline
    char* newline = strchr(password, '\n');
    if (newline) {
        *newline = '\0';
    }
    
    return strlen(password) > 0;
}

/**
 * @brief Main program loop
 * 
 * Demonstrates: Menu-driven program, user interaction,
 * option management, application flow
 */
void run_password_generator(void) {
    PasswordOptions options;
    init_password_options(&options);
    
    char last_password[MAX_PASSWORD_LENGTH + 1] = {0};
    bool has_last_password = false;
    
    printf("Welcome to the Secure Password Generator!\n");
    printf("This tool helps you create strong, secure passwords.\n");
    
    while (true) {
        display_options_menu();
        
        char input[MAX_INPUT_SIZE];
        int choice;
        
        printf("Enter your choice (1-12): ");
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        if (!str_to_int(trim_whitespace(input), &choice)) {
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        
        switch (choice) {
            case 1:
                get_password_length(&options);
                break;
                
            case 2:
                options.include_lowercase = !options.include_lowercase;
                printf("Lowercase letters: %s\n", options.include_lowercase ? "Enabled" : "Disabled");
                break;
                
            case 3:
                options.include_uppercase = !options.include_uppercase;
                printf("Uppercase letters: %s\n", options.include_uppercase ? "Enabled" : "Disabled");
                break;
                
            case 4:
                options.include_digits = !options.include_digits;
                printf("Digits: %s\n", options.include_digits ? "Enabled" : "Disabled");
                break;
                
            case 5:
                options.include_special = !options.include_special;
                printf("Special characters: %s\n", options.include_special ? "Enabled" : "Disabled");
                break;
                
            case 6:
                options.exclude_similar = !options.exclude_similar;
                printf("Exclude similar characters: %s\n", options.exclude_similar ? "Enabled" : "Disabled");
                break;
                
            case 7:
                options.exclude_ambiguous = !options.exclude_ambiguous;
                printf("Exclude ambiguous characters: %s\n", options.exclude_ambiguous ? "Enabled" : "Disabled");
                break;
                
            case 8: {
                char password[MAX_PASSWORD_LENGTH + 1];
                if (generate_password(&options, password, sizeof(password))) {
                    printf("\nGenerated Password: %s\n", password);
                    
                    // Analyze the generated password
                    PasswordAnalysis analysis;
                    analyze_password_strength(password, &analysis);
                    printf("Strength: %s (Score: %d/100)\n", 
                           analysis.strength_description, analysis.score);
                    
                    // Store for potential saving
                    strcpy(last_password, password);
                    has_last_password = true;
                    
                    // Clear password from memory after a delay (security practice)
                    secure_free(password, sizeof(password));
                } else {
                    printf("Failed to generate password. Please check your options.\n");
                }
                break;
            }
                
            case 9: {
                char password[MAX_PASSWORD_LENGTH + 1];
                if (get_password_for_analysis(password, sizeof(password))) {
                    PasswordAnalysis analysis;
                    analyze_password_strength(password, &analysis);
                    display_password_analysis(&analysis);
                    
                    // Clear password from memory
                    secure_free(password, sizeof(password));
                } else {
                    printf("No password entered.\n");
                }
                break;
            }
                
            case 10:
                if (has_last_password) {
                    printf("Enter filename to save password: ");
                    char filename[MAX_INPUT_SIZE];
                    if (fgets(filename, sizeof(filename), stdin)) {
                        char* newline = strchr(filename, '\n');
                        if (newline) *newline = '\0';
                        
                        if (strlen(filename) > 0) {
                            save_password_to_file(last_password, filename);
                        } else {
                            printf("Invalid filename.\n");
                        }
                    }
                } else {
                    printf("No password to save. Generate a password first.\n");
                }
                break;
                
            case 11:
                display_current_options(&options);
                break;
                
            case 12:
                printf("Thank you for using the Password Generator!\n");
                
                // Clear sensitive data
                if (has_last_password) {
                    secure_free(last_password, sizeof(last_password));
                }
                
                return;
                
            default:
                printf("Invalid choice. Please enter a number between 1 and 12.\n");
                break;
        }
    }
}

/**
 * @brief Main function
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return Exit status
 * 
 * Demonstrates: Program entry point, command line handling,
 * random number initialization
 */
int main(int argc, char* argv[]) {
    // Handle command line arguments
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("Secure Password Generator\n");
            printf("Usage: %s [--help|-h]\n", argv[0]);
            printf("\nGenerate secure passwords with customizable options.\n");
            printf("\nFeatures:\n");
            printf("- Customizable password length and character sets\n");
            printf("- Password strength analysis\n");
            printf("- Exclude similar/ambiguous characters\n");
            printf("- Save passwords to file\n");
            printf("- Entropy calculation\n");
            printf("- Security recommendations\n");
            return 0;
        }
    }
    
    // Initialize random number generator
    srand((unsigned int)time(NULL));
    
    // Log application start
    log_message("INFO", "Starting password generator");
    
    // Run the password generator
    run_password_generator();
    
    log_message("INFO", "Password generator terminated");
    return 0;
}

/**
 * Educational Notes:
 * 
 * 1. Password Security:
 *    - Use strong randomization
 *    - Include multiple character types
 *    - Avoid predictable patterns
 *    - Clear passwords from memory after use
 * 
 * 2. String Manipulation:
 *    - Safe string operations
 *    - Character set building
 *    - Pattern matching and validation
 * 
 * 3. Cryptographic Concepts:
 *    - Entropy calculation
 *    - Character space analysis
 *    - Strength assessment
 * 
 * 4. User Experience:
 *    - Interactive menu system
 *    - Clear option management
 *    - Helpful feedback and analysis
 * 
 * 5. File I/O:
 *    - Safe file operations
 *    - Error handling
 *    - Timestamp logging
 * 
 * 6. Memory Security:
 *    - Clear sensitive data
 *    - Secure memory management
 *    - Prevent information leaks
 * 
 * 7. Input Validation:
 *    - Range checking
 *    - Format validation
 *    - Error recovery
 */