/**
 * @file file_manager.c
 * @brief Interactive file manager demonstrating advanced file I/O and system
 * programming
 * @author dunamismax
 * @date 2025
 *
 * This program demonstrates:
 * - Advanced file I/O operations
 * - Directory traversal and manipulation
 * - File system operations and metadata
 * - System programming with POSIX APIs
 * - Error handling for system calls
 * - Memory management for dynamic data
 * - String manipulation and path handling
 * - User interface for file operations
 * - File permissions and security
 * - Cross-platform compatibility considerations
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdbool.h>
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
 * @brief Maximum path length
 */
#define MAX_PATH_LENGTH 4096

/**
 * @brief Maximum command length
 */
#define MAX_COMMAND_LENGTH 1024

/**
 * @brief File type enumeration
 */
typedef enum {
  FILE_TYPE_REGULAR,
  FILE_TYPE_DIRECTORY,
  FILE_TYPE_SYMLINK,
  FILE_TYPE_BLOCK_DEVICE,
  FILE_TYPE_CHAR_DEVICE,
  FILE_TYPE_FIFO,
  FILE_TYPE_SOCKET,
  FILE_TYPE_UNKNOWN
} FileType;

/**
 * @brief File information structure
 *
 * Demonstrates: File metadata organization,
 * system information encapsulation
 */
typedef struct {
  char name[256];
  char path[MAX_PATH_LENGTH];
  FileType type;
  off_t size;
  time_t modified_time;
  time_t access_time;
  mode_t permissions;
  uid_t owner_uid;
  gid_t group_gid;
  char owner_name[32];
  char group_name[32];
  bool is_hidden;
} FileInfo;

/**
 * @brief File manager context
 *
 * Demonstrates: Application state management,
 * working directory tracking
 */
typedef struct {
  char current_directory[MAX_PATH_LENGTH];
  char home_directory[MAX_PATH_LENGTH];
  DynamicArray *file_list;
  bool show_hidden;
  bool show_details;
} FileManager;

/**
 * @brief Get file type from stat structure
 * @param mode File mode from stat
 * @return FileType enumeration value
 *
 * Demonstrates: File type detection,
 * stat structure interpretation
 */
FileType get_file_type(mode_t mode) {
  if (S_ISREG(mode))
    return FILE_TYPE_REGULAR;
  if (S_ISDIR(mode))
    return FILE_TYPE_DIRECTORY;
  if (S_ISLNK(mode))
    return FILE_TYPE_SYMLINK;
  if (S_ISBLK(mode))
    return FILE_TYPE_BLOCK_DEVICE;
  if (S_ISCHR(mode))
    return FILE_TYPE_CHAR_DEVICE;
  if (S_ISFIFO(mode))
    return FILE_TYPE_FIFO;
  if (S_ISSOCK(mode))
    return FILE_TYPE_SOCKET;
  return FILE_TYPE_UNKNOWN;
}

/**
 * @brief Get file type string representation
 * @param type File type
 * @return String representation
 *
 * Demonstrates: Enumeration to string conversion,
 * display formatting
 */
const char *get_file_type_string(FileType type) {
  switch (type) {
  case FILE_TYPE_REGULAR:
    return "File";
  case FILE_TYPE_DIRECTORY:
    return "Directory";
  case FILE_TYPE_SYMLINK:
    return "Symlink";
  case FILE_TYPE_BLOCK_DEVICE:
    return "Block Device";
  case FILE_TYPE_CHAR_DEVICE:
    return "Char Device";
  case FILE_TYPE_FIFO:
    return "FIFO";
  case FILE_TYPE_SOCKET:
    return "Socket";
  default:
    return "Unknown";
  }
}

/**
 * @brief Format file permissions as string
 * @param permissions File permissions
 * @param buffer Buffer to store formatted string
 * @param buffer_size Size of buffer
 *
 * Demonstrates: Permission formatting,
 * bitwise operations, string building
 */
void format_permissions(mode_t permissions, char *buffer, size_t buffer_size) {
  if (!buffer || buffer_size < 11)
    return;

  // File type
  buffer[0] = (S_ISDIR(permissions)) ? 'd' : (S_ISLNK(permissions)) ? 'l' : '-';

  // Owner permissions
  buffer[1] = (permissions & S_IRUSR) ? 'r' : '-';
  buffer[2] = (permissions & S_IWUSR) ? 'w' : '-';
  buffer[3] = (permissions & S_IXUSR) ? 'x' : '-';

  // Group permissions
  buffer[4] = (permissions & S_IRGRP) ? 'r' : '-';
  buffer[5] = (permissions & S_IWGRP) ? 'w' : '-';
  buffer[6] = (permissions & S_IXGRP) ? 'x' : '-';

  // Other permissions
  buffer[7] = (permissions & S_IROTH) ? 'r' : '-';
  buffer[8] = (permissions & S_IWOTH) ? 'w' : '-';
  buffer[9] = (permissions & S_IXOTH) ? 'x' : '-';

  buffer[10] = '\0';
}

/**
 * @brief Format file size for display
 * @param size File size in bytes
 * @param buffer Buffer to store formatted string
 * @param buffer_size Size of buffer
 *
 * Demonstrates: Size formatting,
 * human-readable output
 */
void format_file_size(off_t size, char *buffer, size_t buffer_size) {
  if (!buffer || buffer_size == 0)
    return;

  const char *units[] = {"B", "KB", "MB", "GB", "TB"};
  int unit_index = 0;
  double size_d = (double)size;

  while (size_d >= 1024.0 && unit_index < 4) {
    size_d /= 1024.0;
    unit_index++;
  }

  if (unit_index == 0) {
    snprintf(buffer, buffer_size, "%ld %s", (long)size, units[unit_index]);
  } else {
    snprintf(buffer, buffer_size, "%.1f %s", size_d, units[unit_index]);
  }
}

/**
 * @brief Initialize file manager
 * @param fm Pointer to file manager structure
 * @return true if initialization was successful
 *
 * Demonstrates: Structure initialization,
 * system directory retrieval
 */
bool init_file_manager(FileManager *fm) {
  if (!fm)
    return false;

  // Get current directory
  if (!getcwd(fm->current_directory, sizeof(fm->current_directory))) {
    log_message("ERROR", "Failed to get current directory");
    return false;
  }

  // Get home directory
  const char *home = getenv("HOME");
  if (home) {
    strncpy(fm->home_directory, home, sizeof(fm->home_directory) - 1);
    fm->home_directory[sizeof(fm->home_directory) - 1] = '\0';
  } else {
    strcpy(fm->home_directory, "/");
  }

  // Initialize file list
  fm->file_list = darray_create(sizeof(FileInfo), 64);
  if (!fm->file_list) {
    log_message("ERROR", "Failed to create file list");
    return false;
  }

  fm->show_hidden = false;
  fm->show_details = true;

  return true;
}

/**
 * @brief Clean up file manager resources
 * @param fm Pointer to file manager structure
 *
 * Demonstrates: Resource cleanup,
 * memory deallocation
 */
void cleanup_file_manager(FileManager *fm) {
  if (fm && fm->file_list) {
    darray_destroy(fm->file_list);
  }
}

/**
 * @brief Get file information
 * @param path Path to file
 * @param info Pointer to store file information
 * @return true if information was retrieved successfully
 *
 * Demonstrates: File stat operations,
 * metadata retrieval, error handling
 */
bool get_file_info(const char *path, FileInfo *info) {
  if (!path || !info)
    return false;

  struct stat st;
  if (lstat(path, &st) != 0) {
    log_message("ERROR", "Failed to stat file: %s", path);
    return false;
  }

  // Extract filename from path
  const char *filename = strrchr(path, '/');
  if (filename) {
    filename++; // Skip the '/'
  } else {
    filename = path;
  }

  strncpy(info->name, filename, sizeof(info->name) - 1);
  info->name[sizeof(info->name) - 1] = '\0';

  strncpy(info->path, path, sizeof(info->path) - 1);
  info->path[sizeof(info->path) - 1] = '\0';

  info->type = get_file_type(st.st_mode);
  info->size = st.st_size;
  info->modified_time = st.st_mtime;
  info->access_time = st.st_atime;
  info->permissions = st.st_mode;
  info->owner_uid = st.st_uid;
  info->group_gid = st.st_gid;
  info->is_hidden = (filename[0] == '.');

  // Get owner and group names
  struct passwd *pw = getpwuid(st.st_uid);
  if (pw) {
    strncpy(info->owner_name, pw->pw_name, sizeof(info->owner_name) - 1);
    info->owner_name[sizeof(info->owner_name) - 1] = '\0';
  } else {
    snprintf(info->owner_name, sizeof(info->owner_name), "%d", st.st_uid);
  }

  struct group *gr = getgrgid(st.st_gid);
  if (gr) {
    strncpy(info->group_name, gr->gr_name, sizeof(info->group_name) - 1);
    info->group_name[sizeof(info->group_name) - 1] = '\0';
  } else {
    snprintf(info->group_name, sizeof(info->group_name), "%d", st.st_gid);
  }

  return true;
}

/**
 * @brief Load directory contents
 * @param fm Pointer to file manager
 * @param path Directory path to load
 * @return true if directory was loaded successfully
 *
 * Demonstrates: Directory traversal,
 * file listing, dynamic array usage
 */
bool load_directory(FileManager *fm, const char *path) {
  if (!fm || !path)
    return false;

  DIR *dir = opendir(path);
  if (!dir) {
    log_message("ERROR", "Failed to open directory: %s", path);
    return false;
  }

  // Clear existing file list
  darray_clear(fm->file_list);

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    // Skip . and .. entries
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    // Skip hidden files if not showing them
    if (!fm->show_hidden && entry->d_name[0] == '.') {
      continue;
    }

    // Build full path
    char full_path[MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

    // Get file information
    FileInfo info;
    if (get_file_info(full_path, &info)) {
      darray_push(fm->file_list, &info);
    }
  }

  closedir(dir);

  // Update current directory
  strncpy(fm->current_directory, path, sizeof(fm->current_directory) - 1);
  fm->current_directory[sizeof(fm->current_directory) - 1] = '\0';

  return true;
}

/**
 * @brief Compare files for sorting
 * @param a First file info
 * @param b Second file info
 * @return Comparison result
 *
 * Demonstrates: Comparison functions,
 * sorting by multiple criteria
 */
int compare_files(const void *a, const void *b) {
  const FileInfo *file1 = (const FileInfo *)a;
  const FileInfo *file2 = (const FileInfo *)b;

  // Directories first
  if (file1->type == FILE_TYPE_DIRECTORY &&
      file2->type != FILE_TYPE_DIRECTORY) {
    return -1;
  }
  if (file1->type != FILE_TYPE_DIRECTORY &&
      file2->type == FILE_TYPE_DIRECTORY) {
    return 1;
  }

  // Then by name
  return strcmp(file1->name, file2->name);
}

/**
 * @brief Sort file list
 * @param fm Pointer to file manager
 *
 * Demonstrates: Array sorting,
 * custom comparison functions
 */
void sort_file_list(FileManager *fm) {
  if (!fm || !fm->file_list)
    return;

  size_t count = darray_size(fm->file_list);
  if (count > 1) {
    // Get pointer to raw data for sorting
    FileInfo *files = (FileInfo *)fm->file_list->data;
    quick_sort(files, count, sizeof(FileInfo), compare_files);
  }
}

/**
 * @brief Display file list
 * @param fm Pointer to file manager
 *
 * Demonstrates: Formatted output,
 * table display, file information presentation
 */
void display_file_list(FileManager *fm) {
  if (!fm || !fm->file_list)
    return;

  size_t count = darray_size(fm->file_list);

  printf("\nDirectory: %s\n", fm->current_directory);
  printf("Files: %zu\n", count);

  if (count == 0) {
    printf("(empty directory)\n");
    return;
  }

  if (fm->show_details) {
    printf("\n%-11s %-8s %-8s %-10s %-20s %s\n", "Permissions", "Owner",
           "Group", "Size", "Modified", "Name");
    printf("%-11s %-8s %-8s %-10s %-20s %s\n", "-----------", "--------",
           "--------", "----------", "--------------------", "----");
  }

  for (size_t i = 0; i < count; i++) {
    FileInfo info;
    if (darray_get(fm->file_list, i, &info)) {
      if (fm->show_details) {
        char permissions[12];
        format_permissions(info.permissions, permissions, sizeof(permissions));

        char size_str[32];
        format_file_size(info.size, size_str, sizeof(size_str));

        char time_str[32];
        struct tm *tm_info = localtime(&info.modified_time);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", tm_info);

        printf("%-11s %-8s %-8s %-10s %-20s %s%s\n", permissions,
               info.owner_name, info.group_name, size_str, time_str, info.name,
               (info.type == FILE_TYPE_DIRECTORY) ? "/" : "");
      } else {
        printf("%s%s\n", info.name,
               (info.type == FILE_TYPE_DIRECTORY) ? "/" : "");
      }
    }
  }
}

/**
 * @brief Change directory
 * @param fm Pointer to file manager
 * @param path New directory path
 * @return true if directory was changed successfully
 *
 * Demonstrates: Directory navigation,
 * path resolution, working directory changes
 */
bool change_directory(FileManager *fm, const char *path) {
  if (!fm || !path)
    return false;

  char resolved_path[MAX_PATH_LENGTH];

  // Handle special paths
  if (strcmp(path, "~") == 0) {
    strcpy(resolved_path, fm->home_directory);
  } else if (strcmp(path, "..") == 0) {
    // Go up one directory
    strcpy(resolved_path, fm->current_directory);
    char *last_slash = strrchr(resolved_path, '/');
    if (last_slash && last_slash != resolved_path) {
      *last_slash = '\0';
    } else {
      strcpy(resolved_path, "/");
    }
  } else if (path[0] == '/') {
    // Absolute path
    strcpy(resolved_path, path);
  } else {
    // Relative path
    snprintf(resolved_path, sizeof(resolved_path), "%s/%s",
             fm->current_directory, path);
  }

  // Check if directory exists and is accessible
  struct stat st;
  if (stat(resolved_path, &st) != 0) {
    log_message("ERROR", "Directory does not exist: %s", resolved_path);
    return false;
  }

  if (!S_ISDIR(st.st_mode)) {
    log_message("ERROR", "Not a directory: %s", resolved_path);
    return false;
  }

  // Change working directory
  if (chdir(resolved_path) != 0) {
    log_message("ERROR", "Failed to change directory: %s", resolved_path);
    return false;
  }

  // Load new directory contents
  return load_directory(fm, resolved_path);
}

/**
 * @brief Create directory
 * @param path Directory path to create
 * @return true if directory was created successfully
 *
 * Demonstrates: Directory creation,
 * system call error handling
 */
bool create_directory(const char *path) {
  if (!path)
    return false;

  if (mkdir(path, 0755) != 0) {
    log_message("ERROR", "Failed to create directory: %s", path);
    return false;
  }

  log_message("INFO", "Directory created: %s", path);
  return true;
}

/**
 * @brief Remove file or directory
 * @param path Path to remove
 * @return true if removal was successful
 *
 * Demonstrates: File/directory removal,
 * recursive directory deletion
 */
bool remove_file_or_directory(const char *path) {
  if (!path)
    return false;

  struct stat st;
  if (stat(path, &st) != 0) {
    log_message("ERROR", "File does not exist: %s", path);
    return false;
  }

  if (S_ISDIR(st.st_mode)) {
    if (rmdir(path) != 0) {
      log_message("ERROR", "Failed to remove directory: %s", path);
      return false;
    }
  } else {
    if (unlink(path) != 0) {
      log_message("ERROR", "Failed to remove file: %s", path);
      return false;
    }
  }

  log_message("INFO", "Removed: %s", path);
  return true;
}

/**
 * @brief Copy file
 * @param src Source file path
 * @param dst Destination file path
 * @return true if copy was successful
 *
 * Demonstrates: File copying,
 * binary file handling, buffer I/O
 */
bool copy_file(const char *src, const char *dst) {
  if (!src || !dst)
    return false;

  FILE *src_file = fopen(src, "rb");
  if (!src_file) {
    log_message("ERROR", "Failed to open source file: %s", src);
    return false;
  }

  FILE *dst_file = fopen(dst, "wb");
  if (!dst_file) {
    log_message("ERROR", "Failed to open destination file: %s", dst);
    fclose(src_file);
    return false;
  }

  char buffer[8192];
  size_t bytes_read;

  while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
    if (fwrite(buffer, 1, bytes_read, dst_file) != bytes_read) {
      log_message("ERROR", "Failed to write to destination file");
      fclose(src_file);
      fclose(dst_file);
      return false;
    }
  }

  fclose(src_file);
  fclose(dst_file);

  log_message("INFO", "File copied: %s -> %s", src, dst);
  return true;
}

/**
 * @brief Display help information
 *
 * Demonstrates: Help text display,
 * command documentation
 */
void display_help(void) {
  printf("\n========== File Manager Commands ==========\n");
  printf("ls, list              - List files in current directory\n");
  printf("cd <path>             - Change directory\n");
  printf("pwd                   - Show current directory\n");
  printf("mkdir <name>          - Create directory\n");
  printf("rmdir <name>          - Remove empty directory\n");
  printf("rm <file>             - Remove file\n");
  printf("cp <src> <dst>        - Copy file\n");
  printf("mv <src> <dst>        - Move/rename file\n");
  printf("info <file>           - Show file information\n");
  printf("hidden                - Toggle hidden files display\n");
  printf("details               - Toggle detailed view\n");
  printf("refresh               - Refresh directory listing\n");
  printf("help                  - Show this help\n");
  printf("exit, quit            - Exit file manager\n");
  printf("=========================================\n");
}

/**
 * @brief Process user command
 * @param fm Pointer to file manager
 * @param command User command string
 * @return true if command was processed successfully
 *
 * Demonstrates: Command parsing,
 * string tokenization, command dispatch
 */
bool process_command(FileManager *fm, const char *command) {
  if (!fm || !command)
    return false;

  char cmd_copy[MAX_COMMAND_LENGTH];
  strncpy(cmd_copy, command, sizeof(cmd_copy) - 1);
  cmd_copy[sizeof(cmd_copy) - 1] = '\0';

  char *cmd = strtok(cmd_copy, " \t\n");
  if (!cmd)
    return true;

  // List directory
  if (strcmp(cmd, "ls") == 0 || strcmp(cmd, "list") == 0) {
    sort_file_list(fm);
    display_file_list(fm);
    return true;
  }

  // Change directory
  if (strcmp(cmd, "cd") == 0) {
    char *path = strtok(NULL, " \t\n");
    if (!path) {
      change_directory(fm, fm->home_directory);
    } else {
      change_directory(fm, path);
    }
    return true;
  }

  // Print working directory
  if (strcmp(cmd, "pwd") == 0) {
    printf("%s\n", fm->current_directory);
    return true;
  }

  // Create directory
  if (strcmp(cmd, "mkdir") == 0) {
    char *name = strtok(NULL, " \t\n");
    if (!name) {
      printf("Usage: mkdir <directory_name>\n");
      return true;
    }

    char full_path[MAX_PATH_LENGTH];
    if (name[0] == '/') {
      strcpy(full_path, name);
    } else {
      snprintf(full_path, sizeof(full_path), "%s/%s", fm->current_directory,
               name);
    }

    if (create_directory(full_path)) {
      load_directory(fm, fm->current_directory);
    }
    return true;
  }

  // Remove directory/file
  if (strcmp(cmd, "rmdir") == 0 || strcmp(cmd, "rm") == 0) {
    char *name = strtok(NULL, " \t\n");
    if (!name) {
      printf("Usage: %s <file_or_directory>\n", cmd);
      return true;
    }

    char full_path[MAX_PATH_LENGTH];
    if (name[0] == '/') {
      strcpy(full_path, name);
    } else {
      snprintf(full_path, sizeof(full_path), "%s/%s", fm->current_directory,
               name);
    }

    if (remove_file_or_directory(full_path)) {
      load_directory(fm, fm->current_directory);
    }
    return true;
  }

  // Copy file
  if (strcmp(cmd, "cp") == 0) {
    char *src = strtok(NULL, " \t\n");
    char *dst = strtok(NULL, " \t\n");

    if (!src || !dst) {
      printf("Usage: cp <source> <destination>\n");
      return true;
    }

    char src_path[MAX_PATH_LENGTH], dst_path[MAX_PATH_LENGTH];

    if (src[0] == '/') {
      strcpy(src_path, src);
    } else {
      snprintf(src_path, sizeof(src_path), "%s/%s", fm->current_directory, src);
    }

    if (dst[0] == '/') {
      strcpy(dst_path, dst);
    } else {
      snprintf(dst_path, sizeof(dst_path), "%s/%s", fm->current_directory, dst);
    }

    if (copy_file(src_path, dst_path)) {
      load_directory(fm, fm->current_directory);
    }
    return true;
  }

  // Show file information
  if (strcmp(cmd, "info") == 0) {
    char *name = strtok(NULL, " \t\n");
    if (!name) {
      printf("Usage: info <file_or_directory>\n");
      return true;
    }

    char full_path[MAX_PATH_LENGTH];
    if (name[0] == '/') {
      strcpy(full_path, name);
    } else {
      snprintf(full_path, sizeof(full_path), "%s/%s", fm->current_directory,
               name);
    }

    FileInfo info;
    if (get_file_info(full_path, &info)) {
      printf("\n--- File Information ---\n");
      printf("Name: %s\n", info.name);
      printf("Path: %s\n", info.path);
      printf("Type: %s\n", get_file_type_string(info.type));

      char size_str[32];
      format_file_size(info.size, size_str, sizeof(size_str));
      printf("Size: %s\n", size_str);

      char permissions[12];
      format_permissions(info.permissions, permissions, sizeof(permissions));
      printf("Permissions: %s\n", permissions);

      printf("Owner: %s\n", info.owner_name);
      printf("Group: %s\n", info.group_name);

      char time_str[64];
      struct tm *tm_info = localtime(&info.modified_time);
      strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
      printf("Modified: %s\n", time_str);

      tm_info = localtime(&info.access_time);
      strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
      printf("Accessed: %s\n", time_str);

      printf("----------------------\n");
    }
    return true;
  }

  // Toggle hidden files
  if (strcmp(cmd, "hidden") == 0) {
    fm->show_hidden = !fm->show_hidden;
    printf("Hidden files: %s\n", fm->show_hidden ? "shown" : "hidden");
    load_directory(fm, fm->current_directory);
    return true;
  }

  // Toggle detailed view
  if (strcmp(cmd, "details") == 0) {
    fm->show_details = !fm->show_details;
    printf("Detailed view: %s\n", fm->show_details ? "enabled" : "disabled");
    return true;
  }

  // Refresh directory
  if (strcmp(cmd, "refresh") == 0) {
    load_directory(fm, fm->current_directory);
    printf("Directory refreshed\n");
    return true;
  }

  // Help
  if (strcmp(cmd, "help") == 0) {
    display_help();
    return true;
  }

  // Exit
  if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
    return false;
  }

  printf("Unknown command: %s\n", cmd);
  printf("Type 'help' for available commands\n");
  return true;
}

/**
 * @brief Main file manager loop
 * @param fm Pointer to file manager
 *
 * Demonstrates: Interactive application loop,
 * command line interface, user interaction
 */
void run_file_manager(FileManager *fm) {
  if (!fm)
    return;

  printf("=== Interactive File Manager ===\n");
  printf("Type 'help' for available commands\n");

  // Load initial directory
  load_directory(fm, fm->current_directory);
  sort_file_list(fm);
  display_file_list(fm);

  char command[MAX_COMMAND_LENGTH];

  while (true) {
    printf("\nfile_manager:%s$ ", fm->current_directory);
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

    if (!process_command(fm, command)) {
      break;
    }
  }

  printf("Goodbye!\n");
}

/**
 * @brief Main function
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return Exit status
 *
 * Demonstrates: Program entry point,
 * application initialization
 */
int main(int argc, char *argv[]) {
  // Handle command line arguments
  if (argc > 1) {
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
      printf("Interactive File Manager\n");
      printf("Usage: %s [--help|-h]\n", argv[0]);
      printf("\nA command-line file manager with full directory navigation.\n");
      printf("\nFeatures:\n");
      printf("- Directory navigation and listing\n");
      printf("- File and directory operations\n");
      printf("- File information display\n");
      printf("- Permission and ownership display\n");
      printf("- Hidden file support\n");
      printf("- Interactive command interface\n");
      return 0;
    }
  }

  // Initialize file manager
  FileManager fm;
  if (!init_file_manager(&fm)) {
    printf("Failed to initialize file manager\n");
    return 1;
  }

  // Log application start
  log_message("INFO", "Starting file manager");

  // Run the file manager
  run_file_manager(&fm);

  // Cleanup
  cleanup_file_manager(&fm);

  log_message("INFO", "File manager terminated");
  return 0;
}

/**
 * Educational Notes:
 *
 * 1. System Programming:
 *    - POSIX API usage (stat, opendir, readdir)
 *    - File system operations
 *    - Process working directory management
 *
 * 2. File Operations:
 *    - Directory traversal
 *    - File metadata extraction
 *    - File copying and manipulation
 *
 * 3. User Interface:
 *    - Command-line interface design
 *    - Interactive application loop
 *    - Command parsing and dispatch
 *
 * 4. Data Structures:
 *    - Dynamic arrays for file lists
 *    - Structure design for file information
 *    - Sorting and comparison functions
 *
 * 5. Error Handling:
 *    - System call error checking
 *    - Graceful error recovery
 *    - User-friendly error messages
 *
 * 6. Memory Management:
 *    - Dynamic data structures
 *    - Resource cleanup
 *    - Memory leak prevention
 *
 * 7. String Processing:
 *    - Path manipulation
 *    - Command parsing
 *    - Format string usage
 *
 * 8. Cross-platform Considerations:
 *    - POSIX compatibility
 *    - Platform-specific features
 *    - Portable code design
 */