/**
 * @file text_analyzer.c
 * @brief Text analysis tool demonstrating file I/O, string processing, and statistics
 * @author dunamismax
 * @date 2025
 * 
 * This program demonstrates:
 * - File I/O operations and error handling
 * - String processing and character analysis
 * - Statistical analysis and data aggregation
 * - Dynamic memory allocation and management
 * - Hash table implementation for word counting
 * - Sorting algorithms for frequency analysis
 * - Command line argument processing
 * - Report generation and formatted output
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// Include our utility and data structure libraries
#include "utils.h"
#include "dynamic_array.h"
#include "algorithms.h"

/**
 * @brief Maximum line length for file reading
 */
#define MAX_LINE_LENGTH 1024

/**
 * @brief Maximum word length
 */
#define MAX_WORD_LENGTH 256

/**
 * @brief Hash table size for word counting
 */
#define HASH_TABLE_SIZE 1024

/**
 * @brief Text statistics structure
 * 
 * Demonstrates: Statistical data organization,
 * comprehensive text analysis metrics
 */
typedef struct {
    long total_characters;
    long total_words;
    long total_lines;
    long total_sentences;
    long total_paragraphs;
    long unique_words;
    long alphabetic_chars;
    long numeric_chars;
    long whitespace_chars;
    long punctuation_chars;
    double avg_word_length;
    double avg_sentence_length;
    double avg_paragraph_length;
    int longest_word_length;
    int shortest_word_length;
    char longest_word[MAX_WORD_LENGTH];
    char shortest_word[MAX_WORD_LENGTH];
} TextStats;

/**
 * @brief Word frequency entry
 * 
 * Demonstrates: Hash table entry structure,
 * frequency counting data
 */
typedef struct WordEntry {
    char word[MAX_WORD_LENGTH];
    int frequency;
    struct WordEntry* next;
} WordEntry;

/**
 * @brief Word frequency hash table
 * 
 * Demonstrates: Hash table structure,
 * collision handling with chaining
 */
typedef struct {
    WordEntry* buckets[HASH_TABLE_SIZE];
    int total_entries;
} WordFrequencyTable;

/**
 * @brief Initialize text statistics
 * @param stats Pointer to text statistics structure
 * 
 * Demonstrates: Structure initialization,
 * statistical data setup
 */
void init_text_stats(TextStats* stats) {
    if (!stats) return;
    
    memset(stats, 0, sizeof(TextStats));
    stats->shortest_word_length = INT_MAX;
    strcpy(stats->longest_word, "");
    strcpy(stats->shortest_word, "");
}

/**
 * @brief Initialize word frequency table
 * @param table Pointer to word frequency table
 * 
 * Demonstrates: Hash table initialization,
 * memory setup for data structures
 */
void init_word_frequency_table(WordFrequencyTable* table) {
    if (!table) return;
    
    memset(table->buckets, 0, sizeof(table->buckets));
    table->total_entries = 0;
}

/**
 * @brief Simple hash function for strings
 * @param str String to hash
 * @return Hash value
 * 
 * Demonstrates: Hash function implementation,
 * string hashing algorithms
 */
unsigned int hash_string(const char* str) {
    if (!str) return 0;
    
    unsigned int hash = 5381;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + tolower(c);
    }
    
    return hash % HASH_TABLE_SIZE;
}

/**
 * @brief Add word to frequency table
 * @param table Pointer to word frequency table
 * @param word Word to add
 * 
 * Demonstrates: Hash table insertion,
 * collision handling, frequency counting
 */
void add_word_to_table(WordFrequencyTable* table, const char* word) {
    if (!table || !word || strlen(word) == 0) return;
    
    // Convert word to lowercase for case-insensitive counting
    char lowercase_word[MAX_WORD_LENGTH];
    int i = 0;
    while (word[i] && i < MAX_WORD_LENGTH - 1) {
        lowercase_word[i] = tolower(word[i]);
        i++;
    }
    lowercase_word[i] = '\0';
    
    unsigned int hash = hash_string(lowercase_word);
    WordEntry* entry = table->buckets[hash];
    
    // Search for existing entry
    while (entry) {
        if (strcmp(entry->word, lowercase_word) == 0) {
            entry->frequency++;
            return;
        }
        entry = entry->next;
    }
    
    // Create new entry
    WordEntry* new_entry = malloc(sizeof(WordEntry));
    if (!new_entry) return;
    
    strcpy(new_entry->word, lowercase_word);
    new_entry->frequency = 1;
    new_entry->next = table->buckets[hash];
    table->buckets[hash] = new_entry;
    table->total_entries++;
}

/**
 * @brief Clean up word frequency table
 * @param table Pointer to word frequency table
 * 
 * Demonstrates: Hash table cleanup,
 * memory deallocation, linked list traversal
 */
void cleanup_word_frequency_table(WordFrequencyTable* table) {
    if (!table) return;
    
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        WordEntry* entry = table->buckets[i];
        while (entry) {
            WordEntry* next = entry->next;
            free(entry);
            entry = next;
        }
    }
}

/**
 * @brief Check if character is sentence ending punctuation
 * @param c Character to check
 * @return true if sentence ending punctuation
 * 
 * Demonstrates: Character classification,
 * punctuation analysis
 */
bool is_sentence_ending(char c) {
    return c == '.' || c == '!' || c == '?';
}

/**
 * @brief Extract word from line starting at position
 * @param line Input line
 * @param pos Starting position
 * @param word Buffer to store extracted word
 * @param word_size Size of word buffer
 * @return New position after word extraction
 * 
 * Demonstrates: String parsing, word extraction,
 * character processing
 */
int extract_word(const char* line, int pos, char* word, size_t word_size) {
    if (!line || !word || word_size == 0) return pos;
    
    // Skip non-alphabetic characters
    while (line[pos] && !isalpha(line[pos])) {
        pos++;
    }
    
    // Extract word
    int word_len = 0;
    while (line[pos] && isalpha(line[pos]) && word_len < word_size - 1) {
        word[word_len++] = line[pos++];
    }
    word[word_len] = '\0';
    
    return pos;
}

/**
 * @brief Analyze a single line of text
 * @param line Line to analyze
 * @param stats Pointer to text statistics
 * @param word_table Pointer to word frequency table
 * @param line_number Current line number
 * 
 * Demonstrates: Line-by-line text processing,
 * statistical analysis, word counting
 */
void analyze_line(const char* line, TextStats* stats, WordFrequencyTable* word_table, int line_number) {
    if (!line || !stats || !word_table) return;
    
    int line_len = strlen(line);
    bool in_word = false;
    int word_count = 0;
    int sentence_count = 0;
    char current_word[MAX_WORD_LENGTH];
    
    // Check if line is empty (paragraph separator)
    bool is_empty_line = true;
    for (int i = 0; i < line_len; i++) {
        if (!isspace(line[i])) {
            is_empty_line = false;
            break;
        }
    }
    
    if (is_empty_line) {
        stats->total_paragraphs++;
        return;
    }
    
    // Analyze each character
    for (int i = 0; i < line_len; i++) {
        char c = line[i];
        
        // Count character types
        if (isalpha(c)) {
            stats->alphabetic_chars++;
        } else if (isdigit(c)) {
            stats->numeric_chars++;
        } else if (isspace(c)) {
            stats->whitespace_chars++;
        } else if (ispunct(c)) {
            stats->punctuation_chars++;
            if (is_sentence_ending(c)) {
                sentence_count++;
            }
        }
        
        // Word extraction
        if (isalpha(c)) {
            if (!in_word) {
                in_word = true;
            }
        } else {
            if (in_word) {
                // End of word
                in_word = false;
                word_count++;
            }
        }
    }
    
    // Handle word at end of line
    if (in_word) {
        word_count++;
    }
    
    // Extract and count words
    int pos = 0;
    while (pos < line_len) {
        pos = extract_word(line, pos, current_word, sizeof(current_word));
        
        if (strlen(current_word) > 0) {
            add_word_to_table(word_table, current_word);
            
            // Track longest and shortest words
            int word_len = strlen(current_word);
            if (word_len > stats->longest_word_length) {
                stats->longest_word_length = word_len;
                strcpy(stats->longest_word, current_word);
            }
            if (word_len < stats->shortest_word_length) {
                stats->shortest_word_length = word_len;
                strcpy(stats->shortest_word, current_word);
            }
        }
    }
    
    // Update statistics
    stats->total_characters += line_len;
    stats->total_words += word_count;
    stats->total_sentences += sentence_count;
}

/**
 * @brief Analyze text from file
 * @param filename File to analyze
 * @param stats Pointer to text statistics
 * @param word_table Pointer to word frequency table
 * @return true if analysis was successful
 * 
 * Demonstrates: File I/O, error handling,
 * line-by-line processing
 */
bool analyze_text_file(const char* filename, TextStats* stats, WordFrequencyTable* word_table) {
    if (!filename || !stats || !word_table) return false;
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        log_message("ERROR", "Could not open file: %s", filename);
        return false;
    }
    
    char line[MAX_LINE_LENGTH];
    int line_number = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        analyze_line(line, stats, word_table, line_number);
    }
    
    fclose(file);
    
    // Final calculations
    stats->total_lines = line_number;
    stats->unique_words = word_table->total_entries;
    
    if (stats->total_words > 0) {
        stats->avg_word_length = (double)stats->alphabetic_chars / stats->total_words;
    }
    
    if (stats->total_sentences > 0) {
        stats->avg_sentence_length = (double)stats->total_words / stats->total_sentences;
    }
    
    if (stats->total_paragraphs > 0) {
        stats->avg_paragraph_length = (double)stats->total_words / stats->total_paragraphs;
    }
    
    return true;
}

/**
 * @brief Structure for word frequency sorting
 * 
 * Demonstrates: Data structure for sorting,
 * frequency analysis data
 */
typedef struct {
    char word[MAX_WORD_LENGTH];
    int frequency;
} WordFrequency;

/**
 * @brief Comparison function for word frequency sorting
 * @param a First word frequency entry
 * @param b Second word frequency entry
 * @return Comparison result
 * 
 * Demonstrates: Comparison functions,
 * sorting by frequency
 */
int compare_word_frequency(const void* a, const void* b) {
    const WordFrequency* wf1 = (const WordFrequency*)a;
    const WordFrequency* wf2 = (const WordFrequency*)b;
    
    // Sort by frequency (descending), then by word (ascending)
    if (wf1->frequency != wf2->frequency) {
        return wf2->frequency - wf1->frequency;
    }
    return strcmp(wf1->word, wf2->word);
}

/**
 * @brief Generate frequency analysis report
 * @param word_table Pointer to word frequency table
 * @param top_n Number of top words to show
 * 
 * Demonstrates: Report generation, sorting,
 * frequency analysis display
 */
void generate_frequency_report(WordFrequencyTable* word_table, int top_n) {
    if (!word_table || top_n <= 0) return;
    
    // Create array of word frequencies
    WordFrequency* frequencies = malloc(word_table->total_entries * sizeof(WordFrequency));
    if (!frequencies) {
        log_message("ERROR", "Memory allocation failed for frequency report");
        return;
    }
    
    // Collect all words and frequencies
    int index = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        WordEntry* entry = word_table->buckets[i];
        while (entry) {
            strcpy(frequencies[index].word, entry->word);
            frequencies[index].frequency = entry->frequency;
            index++;
            entry = entry->next;
        }
    }
    
    // Sort by frequency
    quick_sort(frequencies, word_table->total_entries, sizeof(WordFrequency), compare_word_frequency);
    
    // Display top N words
    printf("\n========== Word Frequency Analysis ==========\n");
    printf("Top %d most frequent words:\n", top_n);
    printf("%-20s %s\n", "Word", "Frequency");
    printf("%-20s %s\n", "----", "---------");
    
    int display_count = (top_n < word_table->total_entries) ? top_n : word_table->total_entries;
    for (int i = 0; i < display_count; i++) {
        printf("%-20s %d\n", frequencies[i].word, frequencies[i].frequency);
    }
    
    printf("==========================================\n");
    
    free(frequencies);
}

/**
 * @brief Display comprehensive text statistics
 * @param stats Pointer to text statistics
 * @param filename Source filename
 * 
 * Demonstrates: Statistical report generation,
 * formatted output, comprehensive analysis display
 */
void display_text_statistics(const TextStats* stats, const char* filename) {
    if (!stats || !filename) return;
    
    printf("\n========== Text Analysis Report ==========\n");
    printf("File: %s\n", filename);
    printf("=========================================\n");
    
    printf("\nBasic Statistics:\n");
    printf("  Total characters: %ld\n", stats->total_characters);
    printf("  Total words: %ld\n", stats->total_words);
    printf("  Total lines: %ld\n", stats->total_lines);
    printf("  Total sentences: %ld\n", stats->total_sentences);
    printf("  Total paragraphs: %ld\n", stats->total_paragraphs);
    printf("  Unique words: %ld\n", stats->unique_words);
    
    printf("\nCharacter Analysis:\n");
    printf("  Alphabetic characters: %ld (%.1f%%)\n", stats->alphabetic_chars,
           (double)stats->alphabetic_chars / stats->total_characters * 100);
    printf("  Numeric characters: %ld (%.1f%%)\n", stats->numeric_chars,
           (double)stats->numeric_chars / stats->total_characters * 100);
    printf("  Whitespace characters: %ld (%.1f%%)\n", stats->whitespace_chars,
           (double)stats->whitespace_chars / stats->total_characters * 100);
    printf("  Punctuation characters: %ld (%.1f%%)\n", stats->punctuation_chars,
           (double)stats->punctuation_chars / stats->total_characters * 100);
    
    printf("\nWord Analysis:\n");
    printf("  Average word length: %.2f characters\n", stats->avg_word_length);
    printf("  Longest word: \"%s\" (%d characters)\n", stats->longest_word, stats->longest_word_length);
    printf("  Shortest word: \"%s\" (%d characters)\n", stats->shortest_word, stats->shortest_word_length);
    
    printf("\nSentence Analysis:\n");
    printf("  Average sentence length: %.2f words\n", stats->avg_sentence_length);
    printf("  Average paragraph length: %.2f words\n", stats->avg_paragraph_length);
    
    printf("\nReadability Metrics:\n");
    if (stats->total_sentences > 0) {
        double avg_words_per_sentence = (double)stats->total_words / stats->total_sentences;
        printf("  Average words per sentence: %.2f\n", avg_words_per_sentence);
        
        // Simple readability score (higher = more complex)
        double complexity_score = avg_words_per_sentence * stats->avg_word_length / 10.0;
        printf("  Complexity score: %.2f ", complexity_score);
        
        if (complexity_score < 5.0) {
            printf("(Easy)\n");
        } else if (complexity_score < 10.0) {
            printf("(Moderate)\n");
        } else if (complexity_score < 15.0) {
            printf("(Difficult)\n");
        } else {
            printf("(Very Difficult)\n");
        }
    }
    
    printf("=========================================\n");
}

/**
 * @brief Display usage information
 * @param program_name Name of the program
 * 
 * Demonstrates: Help text display,
 * usage information formatting
 */
void display_usage(const char* program_name) {
    printf("Text Analyzer - Comprehensive text analysis tool\n");
    printf("Usage: %s [options] <filename>\n", program_name);
    printf("\nOptions:\n");
    printf("  -h, --help        Show this help message\n");
    printf("  -f, --frequency   Show word frequency analysis\n");
    printf("  -t, --top N       Show top N most frequent words (default: 10)\n");
    printf("  -s, --stats       Show detailed statistics (default)\n");
    printf("\nExamples:\n");
    printf("  %s document.txt\n", program_name);
    printf("  %s -f -t 20 document.txt\n", program_name);
    printf("  %s --frequency --top 5 document.txt\n", program_name);
    printf("\nFeatures:\n");
    printf("  - Character type analysis\n");
    printf("  - Word frequency counting\n");
    printf("  - Readability metrics\n");
    printf("  - Statistical analysis\n");
    printf("  - Sentence and paragraph counting\n");
}

/**
 * @brief Main function
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return Exit status
 * 
 * Demonstrates: Command line processing,
 * argument parsing, main application flow
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        display_usage(argv[0]);
        return 1;
    }
    
    // Parse command line arguments
    bool show_frequency = false;
    bool show_stats = true;
    int top_n = 10;
    char* filename = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            display_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--frequency") == 0) {
            show_frequency = true;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--stats") == 0) {
            show_stats = true;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--top") == 0) {
            if (i + 1 < argc) {
                if (!str_to_int(argv[++i], &top_n) || top_n <= 0) {
                    printf("Error: Invalid number for --top option\n");
                    return 1;
                }
            } else {
                printf("Error: --top option requires a number\n");
                return 1;
            }
        } else if (argv[i][0] != '-') {
            filename = argv[i];
        } else {
            printf("Error: Unknown option %s\n", argv[i]);
            return 1;
        }
    }
    
    if (!filename) {
        printf("Error: No filename provided\n");
        display_usage(argv[0]);
        return 1;
    }
    
    // Check if file exists
    size_t file_size;
    if (!get_file_size(filename, &file_size)) {
        printf("Error: Cannot access file %s\n", filename);
        return 1;
    }
    
    printf("Analyzing file: %s (%.2f KB)\n", filename, file_size / 1024.0);
    
    // Initialize analysis structures
    TextStats stats;
    WordFrequencyTable word_table;
    
    init_text_stats(&stats);
    init_word_frequency_table(&word_table);
    
    // Perform analysis
    log_message("INFO", "Starting text analysis of %s", filename);
    
    if (!analyze_text_file(filename, &stats, &word_table)) {
        printf("Error: Failed to analyze file %s\n", filename);
        cleanup_word_frequency_table(&word_table);
        return 1;
    }
    
    // Display results
    if (show_stats) {
        display_text_statistics(&stats, filename);
    }
    
    if (show_frequency) {
        generate_frequency_report(&word_table, top_n);
    }
    
    // Cleanup
    cleanup_word_frequency_table(&word_table);
    
    log_message("INFO", "Text analysis completed successfully");
    return 0;
}

/**
 * Educational Notes:
 * 
 * 1. File I/O Operations:
 *    - Safe file opening and closing
 *    - Line-by-line reading
 *    - Error handling for file operations
 * 
 * 2. String Processing:
 *    - Character classification
 *    - Word extraction and parsing
 *    - Case-insensitive processing
 * 
 * 3. Data Structures:
 *    - Hash table for word frequency
 *    - Collision handling with chaining
 *    - Dynamic arrays for sorting
 * 
 * 4. Statistical Analysis:
 *    - Frequency counting
 *    - Average calculations
 *    - Percentage computations
 * 
 * 5. Memory Management:
 *    - Dynamic allocation for hash table
 *    - Proper cleanup and deallocation
 *    - Memory leak prevention
 * 
 * 6. Command Line Processing:
 *    - Argument parsing
 *    - Option handling
 *    - Help text generation
 * 
 * 7. Algorithm Implementation:
 *    - Hash function design
 *    - Sorting algorithms
 *    - Search and analysis algorithms
 * 
 * 8. Report Generation:
 *    - Formatted output
 *    - Statistical presentation
 *    - User-friendly displays
 */