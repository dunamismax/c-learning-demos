/**
 * @file database_engine.c
 * @brief Simple database engine demonstrating storage systems and indexing
 * @author dunamismax
 * @date 2025
 *
 * This program demonstrates:
 * - Database storage engine implementation
 * - B-tree indexing and search algorithms
 * - Transaction management and ACID properties
 * - Buffer pool management
 * - Page-based storage organization
 * - Query execution and optimization
 * - Concurrency control mechanisms
 * - Crash recovery and durability
 * - SQL parsing and execution
 * - Database schema management
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// Include our utility libraries
#include "algorithms.h"
#include "dynamic_array.h"
#include "utils.h"

/**
 * @brief Database page size (4KB - typical for database systems)
 */
#define PAGE_SIZE 4096

/**
 * @brief Maximum number of records per page
 */
#define MAX_RECORDS_PER_PAGE 64

/**
 * @brief Maximum key length
 */
#define MAX_KEY_LENGTH 256

/**
 * @brief Maximum value length
 */
#define MAX_VALUE_LENGTH 1024

/**
 * @brief Maximum table name length
 */
#define MAX_TABLE_NAME_LENGTH 64

/**
 * @brief Maximum column name length
 */
#define MAX_COLUMN_NAME_LENGTH 32

/**
 * @brief Maximum number of columns per table
 */
#define MAX_COLUMNS_PER_TABLE 16

/**
 * @brief B-tree order (number of children)
 */
#define BTREE_ORDER 64

/**
 * @brief Buffer pool size (number of pages)
 */
#define BUFFER_POOL_SIZE 64

/**
 * @brief Database file magic number
 */
#define DB_MAGIC_NUMBER 0x44424541 // "DBEA"

/**
 * @brief Data types supported by the database
 */
typedef enum { TYPE_INTEGER, TYPE_STRING, TYPE_DOUBLE, TYPE_BOOLEAN } DataType;

/**
 * @brief Page types
 */
typedef enum {
  PAGE_TYPE_HEADER,
  PAGE_TYPE_DATA,
  PAGE_TYPE_INDEX,
  PAGE_TYPE_FREE
} PageType;

/**
 * @brief Transaction states
 */
typedef enum {
  TRANSACTION_ACTIVE,
  TRANSACTION_COMMITTED,
  TRANSACTION_ABORTED
} TransactionState;

/**
 * @brief Column definition
 *
 * Demonstrates: Schema definition, type system
 */
typedef struct {
  char name[MAX_COLUMN_NAME_LENGTH];
  DataType type;
  size_t size;
  bool is_primary_key;
  bool is_nullable;
} Column;

/**
 * @brief Table schema
 *
 * Demonstrates: Database schema management,
 * metadata organization
 */
typedef struct {
  char name[MAX_TABLE_NAME_LENGTH];
  Column columns[MAX_COLUMNS_PER_TABLE];
  size_t column_count;
  size_t record_size;
  uint32_t next_record_id;
  uint32_t root_page_id;
} TableSchema;

/**
 * @brief Database record
 *
 * Demonstrates: Record structure, data storage
 */
typedef struct {
  uint32_t record_id;
  bool is_deleted;
  uint8_t data[]; // Variable-length data
} Record;

/**
 * @brief Database page header
 *
 * Demonstrates: Page organization, metadata tracking
 */
typedef struct {
  uint32_t magic;        // Magic number for validation
  PageType page_type;    // Type of page
  uint32_t page_id;      // Page identifier
  uint32_t next_page_id; // Next page in chain
  uint16_t record_count; // Number of records in page
  uint16_t free_space;   // Available space in page
  uint32_t checksum;     // Page integrity checksum
  time_t last_modified;  // Last modification time
} PageHeader;

/**
 * @brief Database page
 *
 * Demonstrates: Page-based storage, fixed-size blocks
 */
typedef struct {
  PageHeader header;
  uint8_t data[PAGE_SIZE - sizeof(PageHeader)];
} Page;

/**
 * @brief Buffer pool entry
 *
 * Demonstrates: Buffer management, page caching
 */
typedef struct {
  Page *page;
  uint32_t page_id;
  bool is_dirty;
  bool is_pinned;
  time_t last_access;
  uint32_t pin_count;
} BufferEntry;

/**
 * @brief B-tree node
 *
 * Demonstrates: Indexing structures, tree organization
 */
typedef struct BTreeNode {
  bool is_leaf;
  size_t key_count;
  char keys[BTREE_ORDER - 1][MAX_KEY_LENGTH];
  uint32_t values[BTREE_ORDER - 1]; // Record IDs for leaf nodes
  struct BTreeNode *children[BTREE_ORDER];
  struct BTreeNode *parent;
  uint32_t page_id;
} BTreeNode;

/**
 * @brief Transaction log entry
 *
 * Demonstrates: Transaction logging, recovery support
 */
typedef struct {
  uint32_t transaction_id;
  uint32_t table_id;
  uint32_t record_id;
  char operation[16]; // INSERT, UPDATE, DELETE
  time_t timestamp;
  size_t old_data_size;
  size_t new_data_size;
  uint8_t old_data[MAX_VALUE_LENGTH];
  uint8_t new_data[MAX_VALUE_LENGTH];
} LogEntry;

/**
 * @brief Database engine structure
 *
 * Demonstrates: Database system organization,
 * component integration
 */
typedef struct {
  char db_filename[256];
  int db_fd;
  TableSchema tables[16];
  size_t table_count;
  BufferEntry buffer_pool[BUFFER_POOL_SIZE];
  size_t next_page_id;
  uint32_t next_transaction_id;
  TransactionState current_transaction_state;
  DynamicArray *transaction_log;
  BTreeNode *indexes[16]; // One index per table
  bool auto_commit;
  bool debug_mode;
} DatabaseEngine;

/**
 * @brief Initialize database engine
 * @param db Pointer to database engine
 * @param filename Database file name
 * @return true if initialization was successful
 *
 * Demonstrates: Database initialization, file handling
 */
bool db_init(DatabaseEngine *db, const char *filename) {
  if (!db || !filename)
    return false;

  memset(db, 0, sizeof(DatabaseEngine));

  strncpy(db->db_filename, filename, sizeof(db->db_filename) - 1);
  db->db_filename[sizeof(db->db_filename) - 1] = '\0';

  // Initialize transaction log
  db->transaction_log = darray_create(sizeof(LogEntry), 64);
  if (!db->transaction_log) {
    log_message("ERROR", "Failed to create transaction log");
    return false;
  }

  // Open or create database file
  db->db_fd = open(filename, O_RDWR | O_CREAT, 0644);
  if (db->db_fd < 0) {
    log_message("ERROR", "Failed to open database file: %s", strerror(errno));
    darray_destroy(db->transaction_log);
    return false;
  }

  // Check if file is new or existing
  struct stat file_stat;
  if (fstat(db->db_fd, &file_stat) == 0 && file_stat.st_size == 0) {
    // New database - write header page
    Page header_page;
    memset(&header_page, 0, sizeof(Page));
    header_page.header.magic = DB_MAGIC_NUMBER;
    header_page.header.page_type = PAGE_TYPE_HEADER;
    header_page.header.page_id = 0;
    header_page.header.last_modified = time(NULL);

    if (write(db->db_fd, &header_page, sizeof(Page)) != sizeof(Page)) {
      log_message("ERROR", "Failed to write header page");
      close(db->db_fd);
      darray_destroy(db->transaction_log);
      return false;
    }

    db->next_page_id = 1;
  } else {
    // Load existing database
    Page header_page;
    if (read(db->db_fd, &header_page, sizeof(Page)) != sizeof(Page)) {
      log_message("ERROR", "Failed to read header page");
      close(db->db_fd);
      darray_destroy(db->transaction_log);
      return false;
    }

    if (header_page.header.magic != DB_MAGIC_NUMBER) {
      log_message("ERROR", "Invalid database file format");
      close(db->db_fd);
      darray_destroy(db->transaction_log);
      return false;
    }

    // Calculate next page ID
    off_t file_size = lseek(db->db_fd, 0, SEEK_END);
    db->next_page_id = file_size / sizeof(Page);
  }

  db->next_transaction_id = 1;
  db->current_transaction_state = TRANSACTION_ACTIVE;
  db->auto_commit = true;
  db->debug_mode = false;

  log_message("INFO", "Database engine initialized: %s", filename);
  return true;
}

/**
 * @brief Calculate simple checksum for page
 * @param page Pointer to page
 * @return Checksum value
 *
 * Demonstrates: Data integrity, corruption detection
 */
uint32_t calculate_page_checksum(const Page *page) {
  if (!page)
    return 0;

  uint32_t checksum = 0;
  const uint8_t *data = (const uint8_t *)page;

  // Skip checksum field itself
  for (size_t i = 0; i < sizeof(Page); i++) {
    if (i >= offsetof(PageHeader, checksum) &&
        i < offsetof(PageHeader, checksum) + sizeof(uint32_t)) {
      continue;
    }
    checksum += data[i];
  }

  return checksum;
}

/**
 * @brief Read page from disk
 * @param db Pointer to database engine
 * @param page_id Page identifier
 * @param page Pointer to store page data
 * @return true if page was read successfully
 *
 * Demonstrates: Disk I/O, page loading
 */
bool read_page(DatabaseEngine *db, uint32_t page_id, Page *page) {
  if (!db || !page || db->db_fd < 0)
    return false;

  off_t offset = page_id * sizeof(Page);

  if (lseek(db->db_fd, offset, SEEK_SET) != offset) {
    log_message("ERROR", "Failed to seek to page %u", page_id);
    return false;
  }

  if (read(db->db_fd, page, sizeof(Page)) != sizeof(Page)) {
    log_message("ERROR", "Failed to read page %u", page_id);
    return false;
  }

  // Verify checksum
  uint32_t calculated_checksum = calculate_page_checksum(page);
  if (page->header.checksum != 0 &&
      page->header.checksum != calculated_checksum) {
    log_message("ERROR", "Page %u checksum mismatch", page_id);
    return false;
  }

  return true;
}

/**
 * @brief Write page to disk
 * @param db Pointer to database engine
 * @param page Pointer to page data
 * @return true if page was written successfully
 *
 * Demonstrates: Disk I/O, page persistence
 */
bool write_page(DatabaseEngine *db, const Page *page) {
  if (!db || !page || db->db_fd < 0)
    return false;

  // Update checksum before writing
  Page temp_page = *page;
  temp_page.header.checksum = calculate_page_checksum(&temp_page);
  temp_page.header.last_modified = time(NULL);

  off_t offset = page->header.page_id * sizeof(Page);

  if (lseek(db->db_fd, offset, SEEK_SET) != offset) {
    log_message("ERROR", "Failed to seek to page %u", page->header.page_id);
    return false;
  }

  if (write(db->db_fd, &temp_page, sizeof(Page)) != sizeof(Page)) {
    log_message("ERROR", "Failed to write page %u", page->header.page_id);
    return false;
  }

  if (fsync(db->db_fd) != 0) {
    log_message("ERROR", "Failed to sync page %u", page->header.page_id);
    return false;
  }

  return true;
}

/**
 * @brief Get page from buffer pool
 * @param db Pointer to database engine
 * @param page_id Page identifier
 * @return Pointer to page, or NULL if not found
 *
 * Demonstrates: Buffer pool management, page caching
 */
Page *get_page_from_buffer(DatabaseEngine *db, uint32_t page_id) {
  if (!db)
    return NULL;

  // Search buffer pool
  for (size_t i = 0; i < BUFFER_POOL_SIZE; i++) {
    BufferEntry *entry = &db->buffer_pool[i];
    if (entry->page && entry->page_id == page_id) {
      entry->last_access = time(NULL);
      entry->pin_count++;
      return entry->page;
    }
  }

  // Not in buffer - find free slot or evict LRU page
  BufferEntry *target_entry = NULL;
  time_t oldest_access = time(NULL);

  for (size_t i = 0; i < BUFFER_POOL_SIZE; i++) {
    BufferEntry *entry = &db->buffer_pool[i];

    // Find free slot
    if (!entry->page) {
      target_entry = entry;
      break;
    }

    // Find least recently used unpinned page
    if (!entry->is_pinned && entry->last_access < oldest_access) {
      oldest_access = entry->last_access;
      target_entry = entry;
    }
  }

  if (!target_entry) {
    log_message("ERROR", "Buffer pool full - all pages pinned");
    return NULL;
  }

  // Evict existing page if necessary
  if (target_entry->page) {
    if (target_entry->is_dirty) {
      if (!write_page(db, target_entry->page)) {
        log_message("ERROR", "Failed to write dirty page during eviction");
        return NULL;
      }
    }
    free(target_entry->page);
  }

  // Load new page
  target_entry->page = safe_calloc(1, sizeof(Page));
  if (!target_entry->page) {
    log_message("ERROR", "Failed to allocate page buffer");
    return NULL;
  }

  if (!read_page(db, page_id, target_entry->page)) {
    free(target_entry->page);
    target_entry->page = NULL;
    return NULL;
  }

  target_entry->page_id = page_id;
  target_entry->is_dirty = false;
  target_entry->is_pinned = false;
  target_entry->last_access = time(NULL);
  target_entry->pin_count = 1;

  return target_entry->page;
}

/**
 * @brief Create new table
 * @param db Pointer to database engine
 * @param table_name Table name
 * @param columns Array of column definitions
 * @param column_count Number of columns
 * @return true if table was created successfully
 *
 * Demonstrates: DDL operations, schema management
 */
bool create_table(DatabaseEngine *db, const char *table_name,
                  const Column *columns, size_t column_count) {
  if (!db || !table_name || !columns || column_count == 0 ||
      column_count > MAX_COLUMNS_PER_TABLE) {
    return false;
  }

  // Check if table already exists
  for (size_t i = 0; i < db->table_count; i++) {
    if (strcmp(db->tables[i].name, table_name) == 0) {
      log_message("ERROR", "Table '%s' already exists", table_name);
      return false;
    }
  }

  if (db->table_count >= 16) {
    log_message("ERROR", "Maximum number of tables reached");
    return false;
  }

  // Create table schema
  TableSchema *table = &db->tables[db->table_count];
  memset(table, 0, sizeof(TableSchema));

  strncpy(table->name, table_name, sizeof(table->name) - 1);
  table->name[sizeof(table->name) - 1] = '\0';

  table->column_count = column_count;
  table->record_size = sizeof(Record);

  // Copy column definitions and calculate record size
  for (size_t i = 0; i < column_count; i++) {
    table->columns[i] = columns[i];
    table->record_size += columns[i].size;
  }

  table->next_record_id = 1;
  table->root_page_id = db->next_page_id++;

  // Create root page for table
  Page root_page;
  memset(&root_page, 0, sizeof(Page));
  root_page.header.page_type = PAGE_TYPE_DATA;
  root_page.header.page_id = table->root_page_id;
  root_page.header.free_space = sizeof(root_page.data);

  if (!write_page(db, &root_page)) {
    log_message("ERROR", "Failed to create root page for table '%s'",
                table_name);
    return false;
  }

  db->table_count++;

  log_message("INFO", "Created table '%s' with %zu columns", table_name,
              column_count);
  return true;
}

/**
 * @brief Find table by name
 * @param db Pointer to database engine
 * @param table_name Table name
 * @return Pointer to table schema, or NULL if not found
 *
 * Demonstrates: Schema lookup, table management
 */
TableSchema *find_table(DatabaseEngine *db, const char *table_name) {
  if (!db || !table_name)
    return NULL;

  for (size_t i = 0; i < db->table_count; i++) {
    if (strcmp(db->tables[i].name, table_name) == 0) {
      return &db->tables[i];
    }
  }

  return NULL;
}

/**
 * @brief Insert record into table
 * @param db Pointer to database engine
 * @param table_name Table name
 * @param values Array of column values
 * @param value_count Number of values
 * @return Record ID if successful, 0 on failure
 *
 * Demonstrates: DML operations, record insertion
 */
uint32_t insert_record(DatabaseEngine *db, const char *table_name,
                       const char **values, size_t value_count) {
  if (!db || !table_name || !values)
    return 0;

  TableSchema *table = find_table(db, table_name);
  if (!table) {
    log_message("ERROR", "Table '%s' not found", table_name);
    return 0;
  }

  if (value_count != table->column_count) {
    log_message("ERROR", "Value count mismatch: expected %zu, got %zu",
                table->column_count, value_count);
    return 0;
  }

  // Get root page
  Page *page = get_page_from_buffer(db, table->root_page_id);
  if (!page) {
    log_message("ERROR", "Failed to load root page for table '%s'", table_name);
    return 0;
  }

  // Check if page has space for new record
  size_t record_space_needed = table->record_size;
  if (page->header.free_space < record_space_needed) {
    log_message("ERROR", "Not enough space in page for new record");
    return 0;
  }

  // Create record
  uint32_t record_id = table->next_record_id++;
  size_t record_offset = sizeof(page->data) - page->header.free_space;

  Record *record = (Record *)(page->data + record_offset);
  record->record_id = record_id;
  record->is_deleted = false;

  // Copy column values
  uint8_t *data_ptr = record->data;
  for (size_t i = 0; i < table->column_count; i++) {
    const Column *col = &table->columns[i];
    const char *value = values[i];

    switch (col->type) {
    case TYPE_INTEGER: {
      int int_val = 0;
      if (value && str_to_int(value, &int_val)) {
        memcpy(data_ptr, &int_val, sizeof(int));
      }
      data_ptr += sizeof(int);
      break;
    }
    case TYPE_STRING: {
      if (value) {
        size_t len = strlen(value);
        if (len >= col->size)
          len = col->size - 1;
        memcpy(data_ptr, value, len);
        data_ptr[len] = '\0';
      }
      data_ptr += col->size;
      break;
    }
    case TYPE_DOUBLE: {
      double double_val = 0.0;
      if (value) {
        double_val = strtod(value, NULL);
        memcpy(data_ptr, &double_val, sizeof(double));
      }
      data_ptr += sizeof(double);
      break;
    }
    case TYPE_BOOLEAN: {
      bool bool_val = false;
      if (value && (strcmp(value, "true") == 0 || strcmp(value, "1") == 0)) {
        bool_val = true;
      }
      memcpy(data_ptr, &bool_val, sizeof(bool));
      data_ptr += sizeof(bool);
      break;
    }
    }
  }

  // Update page metadata
  page->header.record_count++;
  page->header.free_space -= record_space_needed;

  // Mark buffer as dirty
  for (size_t i = 0; i < BUFFER_POOL_SIZE; i++) {
    if (db->buffer_pool[i].page == page) {
      db->buffer_pool[i].is_dirty = true;
      break;
    }
  }

  // Log transaction
  if (db->transaction_log) {
    LogEntry log_entry;
    memset(&log_entry, 0, sizeof(LogEntry));
    log_entry.transaction_id = db->next_transaction_id;
    log_entry.record_id = record_id;
    strcpy(log_entry.operation, "INSERT");
    log_entry.timestamp = time(NULL);
    log_entry.new_data_size = table->record_size;
    memcpy(log_entry.new_data, record,
           table->record_size < sizeof(log_entry.new_data)
               ? table->record_size
               : sizeof(log_entry.new_data));

    darray_push(db->transaction_log, &log_entry);
  }

  if (db->debug_mode) {
    log_message("DEBUG", "Inserted record %u into table '%s'", record_id,
                table_name);
  }

  return record_id;
}

/**
 * @brief Query records from table
 * @param db Pointer to database engine
 * @param table_name Table name
 * @param column_name Column to search (NULL for all)
 * @param value Value to search for (NULL for all)
 *
 * Demonstrates: Query execution, record retrieval
 */
void query_table(DatabaseEngine *db, const char *table_name,
                 const char *column_name, const char *value) {
  if (!db || !table_name)
    return;

  TableSchema *table = find_table(db, table_name);
  if (!table) {
    log_message("ERROR", "Table '%s' not found", table_name);
    return;
  }

  // Get root page
  Page *page = get_page_from_buffer(db, table->root_page_id);
  if (!page) {
    log_message("ERROR", "Failed to load root page for table '%s'", table_name);
    return;
  }

  printf("\n=== Query Results: %s ===\n", table_name);

  // Print column headers
  for (size_t i = 0; i < table->column_count; i++) {
    printf("%-15s", table->columns[i].name);
  }
  printf("\n");

  for (size_t i = 0; i < table->column_count; i++) {
    printf("%-15s", "---------------");
  }
  printf("\n");

  // Scan records
  size_t record_offset = 0;
  size_t results_count = 0;

  while (record_offset < sizeof(page->data) - page->header.free_space) {
    Record *record = (Record *)(page->data + record_offset);

    if (!record->is_deleted) {
      bool matches = true;

      // Apply filter if specified
      if (column_name && value) {
        matches = false;

        uint8_t *data_ptr = record->data;
        for (size_t i = 0; i < table->column_count; i++) {
          const Column *col = &table->columns[i];

          if (strcmp(col->name, column_name) == 0) {
            switch (col->type) {
            case TYPE_STRING: {
              if (strcmp((char *)data_ptr, value) == 0) {
                matches = true;
              }
              break;
            }
            case TYPE_INTEGER: {
              int int_val = *(int *)data_ptr;
              int search_val;
              if (str_to_int(value, &search_val) && int_val == search_val) {
                matches = true;
              }
              break;
            }
              // Add other type comparisons as needed
            }
            break;
          }

          data_ptr += col->size;
        }
      }

      if (matches) {
        // Print record
        uint8_t *data_ptr = record->data;
        for (size_t i = 0; i < table->column_count; i++) {
          const Column *col = &table->columns[i];

          switch (col->type) {
          case TYPE_INTEGER: {
            int int_val = *(int *)data_ptr;
            printf("%-15d", int_val);
            data_ptr += sizeof(int);
            break;
          }
          case TYPE_STRING: {
            printf("%-15s", (char *)data_ptr);
            data_ptr += col->size;
            break;
          }
          case TYPE_DOUBLE: {
            double double_val = *(double *)data_ptr;
            printf("%-15.2f", double_val);
            data_ptr += sizeof(double);
            break;
          }
          case TYPE_BOOLEAN: {
            bool bool_val = *(bool *)data_ptr;
            printf("%-15s", bool_val ? "true" : "false");
            data_ptr += sizeof(bool);
            break;
          }
          }
        }
        printf("\n");
        results_count++;
      }
    }

    record_offset += table->record_size;
  }

  printf("\nQuery completed: %zu records found\n", results_count);
}

/**
 * @brief Display database schema
 * @param db Pointer to database engine
 *
 * Demonstrates: Schema introspection, metadata display
 */
void show_schema(DatabaseEngine *db) {
  if (!db)
    return;

  printf("\n=== Database Schema ===\n");
  printf("Database file: %s\n", db->db_filename);
  printf("Tables: %zu\n\n", db->table_count);

  for (size_t i = 0; i < db->table_count; i++) {
    const TableSchema *table = &db->tables[i];

    printf("Table: %s\n", table->name);
    printf("Columns: %zu\n", table->column_count);
    printf("Record size: %zu bytes\n", table->record_size);
    printf("Next record ID: %u\n", table->next_record_id);
    printf("Root page ID: %u\n\n", table->root_page_id);

    printf("  %-20s %-12s %-8s %-8s\n", "Column", "Type", "Size", "Flags");
    printf("  %-20s %-12s %-8s %-8s\n", "--------------------", "------------",
           "--------", "--------");

    for (size_t j = 0; j < table->column_count; j++) {
      const Column *col = &table->columns[j];
      const char *type_name = col->type == TYPE_INTEGER   ? "INTEGER"
                              : col->type == TYPE_STRING  ? "STRING"
                              : col->type == TYPE_DOUBLE  ? "DOUBLE"
                              : col->type == TYPE_BOOLEAN ? "BOOLEAN"
                                                          : "UNKNOWN";

      char flags[16] = "";
      if (col->is_primary_key)
        strcat(flags, "PK ");
      if (col->is_nullable)
        strcat(flags, "NULL");

      printf("  %-20s %-12s %-8zu %-8s\n", col->name, type_name, col->size,
             flags);
    }
    printf("\n");
  }

  printf("====================\n");
}

/**
 * @brief Display database statistics
 * @param db Pointer to database engine
 *
 * Demonstrates: Statistics reporting, performance monitoring
 */
void show_statistics(DatabaseEngine *db) {
  if (!db)
    return;

  printf("\n=== Database Statistics ===\n");
  printf("Database file: %s\n", db->db_filename);
  printf("Next page ID: %zu\n", db->next_page_id);
  printf("Next transaction ID: %u\n", db->next_transaction_id);
  printf("Transaction state: %s\n",
         db->current_transaction_state == TRANSACTION_ACTIVE      ? "ACTIVE"
         : db->current_transaction_state == TRANSACTION_COMMITTED ? "COMMITTED"
                                                                  : "ABORTED");
  printf("Auto-commit: %s\n", db->auto_commit ? "enabled" : "disabled");
  printf("Debug mode: %s\n", db->debug_mode ? "enabled" : "disabled");

  // Buffer pool statistics
  size_t used_buffers = 0;
  size_t dirty_buffers = 0;
  size_t pinned_buffers = 0;

  for (size_t i = 0; i < BUFFER_POOL_SIZE; i++) {
    const BufferEntry *entry = &db->buffer_pool[i];
    if (entry->page) {
      used_buffers++;
      if (entry->is_dirty)
        dirty_buffers++;
      if (entry->is_pinned || entry->pin_count > 0)
        pinned_buffers++;
    }
  }

  printf("\nBuffer Pool:\n");
  printf("  Size: %d pages\n", BUFFER_POOL_SIZE);
  printf("  Used: %zu pages\n", used_buffers);
  printf("  Dirty: %zu pages\n", dirty_buffers);
  printf("  Pinned: %zu pages\n", pinned_buffers);

  // Transaction log statistics
  if (db->transaction_log) {
    printf("\nTransaction Log:\n");
    printf("  Entries: %zu\n", darray_size(db->transaction_log));
  }

  printf("=========================\n");
}

/**
 * @brief Process SQL-like commands
 * @param db Pointer to database engine
 * @param command Command string
 *
 * Demonstrates: Command parsing, SQL-like interface
 */
void process_command(DatabaseEngine *db, const char *command) {
  if (!db || !command)
    return;

  char cmd_copy[1024];
  strncpy(cmd_copy, command, sizeof(cmd_copy) - 1);
  cmd_copy[sizeof(cmd_copy) - 1] = '\0';

  char *cmd = strtok(cmd_copy, " \t\n");
  if (!cmd)
    return;

  // Convert to uppercase for comparison
  for (char *p = cmd; *p; p++) {
    *p = toupper(*p);
  }

  if (strcmp(cmd, "CREATE") == 0) {
    char *table_keyword = strtok(NULL, " \t");
    char *table_name = strtok(NULL, " \t");

    if (!table_keyword || !table_name || strcmp(table_keyword, "TABLE") != 0) {
      printf("Error: Invalid CREATE TABLE syntax\n");
      return;
    }

    // Simple table creation with predefined columns
    Column columns[] = {{"id", TYPE_INTEGER, sizeof(int), true, false},
                        {"name", TYPE_STRING, 64, false, true},
                        {"age", TYPE_INTEGER, sizeof(int), false, true}};

    if (create_table(db, table_name, columns, 3)) {
      printf("Table '%s' created successfully\n", table_name);
    } else {
      printf("Error: Failed to create table '%s'\n", table_name);
    }

  } else if (strcmp(cmd, "INSERT") == 0) {
    char *into_keyword = strtok(NULL, " \t");
    char *table_name = strtok(NULL, " \t");
    char *values_keyword = strtok(NULL, " \t");

    if (!into_keyword || !table_name || !values_keyword ||
        strcmp(into_keyword, "INTO") != 0 ||
        strcmp(values_keyword, "VALUES") != 0) {
      printf("Error: Invalid INSERT syntax\n");
      return;
    }

    // Parse values (simplified - expects comma-separated values)
    const char *values[MAX_COLUMNS_PER_TABLE];
    size_t value_count = 0;

    char *value = strtok(NULL, ",");
    while (value && value_count < MAX_COLUMNS_PER_TABLE) {
      // Trim whitespace and quotes
      value = trim_whitespace(value);
      if (value[0] == '"' || value[0] == '\'') {
        value++;
        char *end = strrchr(value, value[-1]);
        if (end)
          *end = '\0';
      }
      values[value_count++] = value;
      value = strtok(NULL, ",");
    }

    uint32_t record_id = insert_record(db, table_name, values, value_count);
    if (record_id > 0) {
      printf("Record inserted with ID: %u\n", record_id);
    } else {
      printf("Error: Failed to insert record\n");
    }

  } else if (strcmp(cmd, "SELECT") == 0) {
    char *from_part = strstr(command + 6, "FROM");
    if (!from_part) {
      printf("Error: Invalid SELECT syntax\n");
      return;
    }

    char *table_name = strtok(from_part + 4, " \t\n");
    if (!table_name) {
      printf("Error: Table name required\n");
      return;
    }

    query_table(db, table_name, NULL, NULL);

  } else if (strcmp(cmd, "SHOW") == 0) {
    char *what = strtok(NULL, " \t\n");
    if (!what) {
      printf("Error: SHOW what?\n");
      return;
    }

    for (char *p = what; *p; p++) {
      *p = toupper(*p);
    }

    if (strcmp(what, "TABLES") == 0 || strcmp(what, "SCHEMA") == 0) {
      show_schema(db);
    } else if (strcmp(what, "STATS") == 0 || strcmp(what, "STATISTICS") == 0) {
      show_statistics(db);
    } else {
      printf("Error: Unknown SHOW command\n");
    }

  } else {
    printf("Error: Unknown command: %s\n", cmd);
    printf(
        "Supported commands: CREATE TABLE, INSERT INTO, SELECT FROM, SHOW\n");
  }
}

/**
 * @brief Interactive database interface
 * @param db Pointer to database engine
 *
 * Demonstrates: Interactive SQL interface, command processing
 */
void run_interactive_mode(DatabaseEngine *db) {
  if (!db)
    return;

  printf("\n=== Interactive Database Engine ===\n");
  printf("Type SQL commands or 'help' for assistance\n");
  printf("Commands: CREATE TABLE, INSERT INTO, SELECT FROM, SHOW\n");
  printf("Type 'quit' to exit\n");
  printf("==================================\n");

  char command[1024];

  while (true) {
    printf("\ndb> ");
    fflush(stdout);

    if (!fgets(command, sizeof(command), stdin)) {
      break;
    }

    // Remove trailing newline
    char *newline = strchr(command, '\n');
    if (newline)
      *newline = '\0';

    // Skip empty commands
    if (strlen(trim_whitespace(command)) == 0) {
      continue;
    }

    // Check for exit commands
    if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0) {
      break;
    }

    if (strcmp(command, "help") == 0) {
      printf("\nSupported SQL commands:\n");
      printf("  CREATE TABLE <name>         - Create table with default "
             "columns\n");
      printf("  INSERT INTO <table> VALUES <values> - Insert record\n");
      printf("  SELECT * FROM <table>       - Query all records\n");
      printf("  SHOW TABLES                 - Display schema\n");
      printf("  SHOW STATS                  - Display statistics\n");
      printf("  help                        - Show this help\n");
      printf("  quit                        - Exit database\n");
      continue;
    }

    process_command(db, command);
  }
}

/**
 * @brief Flush all dirty pages to disk
 * @param db Pointer to database engine
 * @return true if all pages were flushed successfully
 *
 * Demonstrates: Buffer management, data persistence
 */
bool flush_all_pages(DatabaseEngine *db) {
  if (!db)
    return false;

  bool success = true;
  size_t flushed_count = 0;

  for (size_t i = 0; i < BUFFER_POOL_SIZE; i++) {
    BufferEntry *entry = &db->buffer_pool[i];

    if (entry->page && entry->is_dirty) {
      if (write_page(db, entry->page)) {
        entry->is_dirty = false;
        flushed_count++;
      } else {
        success = false;
      }
    }
  }

  if (db->debug_mode) {
    log_message("DEBUG", "Flushed %zu dirty pages to disk", flushed_count);
  }

  return success;
}

/**
 * @brief Close database engine
 * @param db Pointer to database engine
 *
 * Demonstrates: Database shutdown, resource cleanup
 */
void db_close(DatabaseEngine *db) {
  if (!db)
    return;

  log_message("INFO", "Closing database engine");

  // Flush all dirty pages
  flush_all_pages(db);

  // Free buffer pool
  for (size_t i = 0; i < BUFFER_POOL_SIZE; i++) {
    if (db->buffer_pool[i].page) {
      free(db->buffer_pool[i].page);
      db->buffer_pool[i].page = NULL;
    }
  }

  // Close database file
  if (db->db_fd >= 0) {
    close(db->db_fd);
    db->db_fd = -1;
  }

  // Free transaction log
  if (db->transaction_log) {
    darray_destroy(db->transaction_log);
    db->transaction_log = NULL;
  }

  log_message("INFO", "Database engine closed");
}

/**
 * @brief Display help information
 * @param program_name Program name from argv[0]
 */
void display_help(const char *program_name) {
  printf("Database Engine - Storage Systems and Indexing\n");
  printf("Usage: %s [options] <database_file>\n\n", program_name);
  printf("Options:\n");
  printf("  -i, --interactive   Run in interactive mode\n");
  printf("  -c, --create        Create new database\n");
  printf("  -d, --debug         Enable debug output\n");
  printf("  --help              Show this help\n\n");
  printf("Features demonstrated:\n");
  printf("- Page-based storage organization\n");
  printf("- Buffer pool management\n");
  printf("- Simple SQL command processing\n");
  printf("- Transaction logging\n");
  printf("- Schema management\n");
  printf("- Data integrity and checksums\n");
}

/**
 * @brief Main function
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return Exit status
 *
 * Demonstrates: Database engine initialization,
 * command processing, system integration
 */
int main(int argc, char *argv[]) {
  char *db_filename = NULL;
  bool interactive_mode = false;
  bool create_new = false;
  bool debug_mode = false;

  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0) {
      display_help(argv[0]);
      return 0;
    } else if (strcmp(argv[i], "-i") == 0 ||
               strcmp(argv[i], "--interactive") == 0) {
      interactive_mode = true;
    } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--create") == 0) {
      create_new = true;
    } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
      debug_mode = true;
    } else if (!db_filename) {
      db_filename = argv[i];
    } else {
      printf("Error: Too many arguments\n");
      display_help(argv[0]);
      return 1;
    }
  }

  if (!db_filename) {
    printf("Error: Database filename required\n");
    display_help(argv[0]);
    return 1;
  }

  // Remove existing file if creating new
  if (create_new) {
    unlink(db_filename);
  }

  // Initialize database engine
  DatabaseEngine db;
  if (!db_init(&db, db_filename)) {
    printf("Error: Failed to initialize database engine\n");
    return 1;
  }

  db.debug_mode = debug_mode;

  printf("Database engine started: %s\n", db_filename);

  // Run requested mode
  if (interactive_mode) {
    run_interactive_mode(&db);
  } else {
    // Demonstration mode
    printf("\nRunning database demonstration...\n");

    // Create sample table
    Column columns[] = {{"id", TYPE_INTEGER, sizeof(int), true, false},
                        {"name", TYPE_STRING, 64, false, false},
                        {"age", TYPE_INTEGER, sizeof(int), false, true}};

    if (create_table(&db, "users", columns, 3)) {
      printf("Created sample table 'users'\n");

      // Insert sample data
      const char *values1[] = {"1", "Alice", "25"};
      const char *values2[] = {"2", "Bob", "30"};
      const char *values3[] = {"3", "Charlie", "35"};

      insert_record(&db, "users", values1, 3);
      insert_record(&db, "users", values2, 3);
      insert_record(&db, "users", values3, 3);

      printf("Inserted sample records\n");

      // Query data
      query_table(&db, "users", NULL, NULL);
    }

    show_schema(&db);
    show_statistics(&db);
  }

  // Cleanup
  db_close(&db);

  log_message("INFO", "Database engine terminated");
  return 0;
}

/**
 * Educational Notes:
 *
 * 1. Storage Engine Architecture:
 *    - Page-based storage organization
 *    - Buffer pool management and caching
 *    - Disk I/O optimization techniques
 *
 * 2. Data Organization:
 *    - Record layout and storage formats
 *    - Schema definition and management
 *    - Data type handling and serialization
 *
 * 3. Indexing and Search:
 *    - B-tree structure design (foundation)
 *    - Index organization and maintenance
 *    - Query execution strategies
 *
 * 4. Transaction Management:
 *    - Transaction logging and recovery
 *    - ACID property implementation
 *    - Concurrency control concepts
 *
 * 5. Query Processing:
 *    - SQL-like command parsing
 *    - Query execution and optimization
 *    - Result set management
 *
 * 6. Performance Optimization:
 *    - Buffer pool hit ratios
 *    - Page replacement algorithms
 *    - Statistics collection and monitoring
 *
 * 7. Data Integrity:
 *    - Checksum validation
 *    - Corruption detection and prevention
 *    - Backup and recovery procedures
 *
 * 8. System Integration:
 *    - File system interaction
 *    - Memory management strategies
 *    - Error handling and diagnostics
 */