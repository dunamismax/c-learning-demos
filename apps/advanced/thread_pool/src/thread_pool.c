/**
 * @file thread_pool.c
 * @brief Thread pool implementation demonstrating concurrent programming
 * @author dunamismax
 * @date 2025
 *
 * This program demonstrates:
 * - Thread pool pattern for concurrent task execution
 * - Producer-consumer pattern with work queues
 * - Thread synchronization using mutexes and condition variables
 * - Dynamic thread management and load balancing
 * - Task scheduling and work distribution
 * - Thread-safe data structures
 * - Resource management in multithreaded environments
 * - Performance monitoring and statistics
 * - Graceful shutdown and cleanup
 * - POSIX threads (pthreads) programming
 */

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// Include our utility libraries
#include "dynamic_array.h"
#include "utils.h"

/**
 * @brief Maximum number of threads in pool
 */
#define MAX_THREADS 32

/**
 * @brief Default number of threads
 */
#define DEFAULT_THREADS 4

/**
 * @brief Maximum number of queued tasks
 */
#define MAX_QUEUE_SIZE 1000

/**
 * @brief Task execution timeout in seconds
 */
#define TASK_TIMEOUT 30

/**
 * @brief Statistics update interval in seconds
 */
#define STATS_INTERVAL 5

/**
 * @brief Task function pointer type
 */
typedef void (*task_func_t)(void *arg);

/**
 * @brief Task structure
 *
 * Demonstrates: Task encapsulation, function pointers,
 * work unit representation
 */
typedef struct {
  task_func_t function;    // Function to execute
  void *argument;          // Argument to pass to function
  char name[64];           // Task name for debugging
  struct timespec created; // Task creation time
  int priority;            // Task priority (higher = more important)
} Task;

/**
 * @brief Thread pool statistics
 *
 * Demonstrates: Performance monitoring, metrics collection
 */
typedef struct {
  size_t tasks_completed;     // Total tasks completed
  size_t tasks_failed;        // Total tasks that failed
  size_t tasks_queued;        // Current tasks in queue
  size_t active_threads;      // Currently active threads
  size_t idle_threads;        // Currently idle threads
  double avg_task_time;       // Average task execution time
  struct timespec start_time; // Pool start time
} ThreadPoolStats;

/**
 * @brief Worker thread information
 *
 * Demonstrates: Thread metadata, state tracking
 */
typedef struct {
  pthread_t thread_id;         // Thread ID
  int thread_index;            // Thread index in pool
  bool is_active;              // Currently executing task
  size_t tasks_completed;      // Tasks completed by this thread
  struct timespec last_active; // Last activity time
} WorkerThread;

/**
 * @brief Thread pool structure
 *
 * Demonstrates: Complex data structure design, thread management,
 * synchronization primitives
 */
typedef struct {
  WorkerThread *threads; // Array of worker threads
  size_t thread_count;   // Number of threads
  size_t min_threads;    // Minimum number of threads
  size_t max_threads;    // Maximum number of threads

  Task *task_queue;   // Circular task queue
  size_t queue_size;  // Maximum queue size
  size_t queue_head;  // Queue head index
  size_t queue_tail;  // Queue tail index
  size_t queue_count; // Current number of tasks in queue

  pthread_mutex_t queue_mutex;    // Queue access mutex
  pthread_cond_t queue_not_empty; // Signal when queue has tasks
  pthread_cond_t queue_not_full;  // Signal when queue has space

  pthread_mutex_t stats_mutex; // Statistics mutex
  ThreadPoolStats stats;       // Pool statistics

  bool shutdown;       // Shutdown flag
  bool force_shutdown; // Force immediate shutdown

  pthread_t monitor_thread; // Statistics monitoring thread
  bool debug_mode;          // Debug output enabled
} ThreadPool;

// Global thread pool instance
static ThreadPool *g_thread_pool = NULL;
static bool g_running = true;

/**
 * @brief Signal handler for graceful shutdown
 * @param sig Signal number
 */
void signal_handler(int sig) {
  (void)sig; // Suppress unused parameter warning
  printf("\nReceived shutdown signal. Gracefully shutting down...\n");
  g_running = false;

  if (g_thread_pool) {
    g_thread_pool->shutdown = true;
    pthread_cond_broadcast(&g_thread_pool->queue_not_empty);
  }
}

/**
 * @brief Get current time as timespec
 * @param ts Pointer to timespec structure
 */
void get_current_time(struct timespec *ts) {
  if (clock_gettime(CLOCK_MONOTONIC, ts) != 0) {
    // Fallback for systems without CLOCK_MONOTONIC
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;
  }
}

/**
 * @brief Calculate time difference in seconds
 * @param start Start time
 * @param end End time
 * @return Time difference in seconds
 */
double timespec_diff(const struct timespec *start, const struct timespec *end) {
  return (end->tv_sec - start->tv_sec) +
         (end->tv_nsec - start->tv_nsec) / 1000000000.0;
}

/**
 * @brief Create a new task
 * @param function Task function to execute
 * @param argument Argument to pass to function
 * @param name Task name for debugging
 * @param priority Task priority
 * @return Pointer to new task or NULL on failure
 */
Task *task_create(task_func_t function, void *argument, const char *name,
                  int priority) {
  if (!function)
    return NULL;

  Task *task = safe_calloc(1, sizeof(Task));
  if (!task)
    return NULL;

  task->function = function;
  task->argument = argument;
  task->priority = priority;

  if (name) {
    strncpy(task->name, name, sizeof(task->name) - 1);
    task->name[sizeof(task->name) - 1] = '\0';
  } else {
    strcpy(task->name, "unnamed");
  }

  get_current_time(&task->created);

  return task;
}

/**
 * @brief Worker thread function
 * @param arg Pointer to WorkerThread structure
 * @return NULL
 *
 * Demonstrates: Thread main function, work loop,
 * synchronization, task execution
 */
void *worker_thread(void *arg) {
  WorkerThread *worker = (WorkerThread *)arg;
  ThreadPool *pool = g_thread_pool;

  if (pool->debug_mode) {
    printf("Worker thread %d started\n", worker->thread_index);
  }

  while (!pool->shutdown) {
    Task task = {0};
    bool has_task = false;

    // Wait for task or shutdown signal
    pthread_mutex_lock(&pool->queue_mutex);

    while (pool->queue_count == 0 && !pool->shutdown) {
      pthread_cond_wait(&pool->queue_not_empty, &pool->queue_mutex);
    }

    if (pool->shutdown) {
      pthread_mutex_unlock(&pool->queue_mutex);
      break;
    }

    // Get task from queue
    if (pool->queue_count > 0) {
      task = pool->task_queue[pool->queue_head];
      pool->queue_head = (pool->queue_head + 1) % pool->queue_size;
      pool->queue_count--;
      has_task = true;

      // Signal that queue has space
      pthread_cond_signal(&pool->queue_not_full);
    }

    pthread_mutex_unlock(&pool->queue_mutex);

    if (has_task) {
      // Update worker state
      worker->is_active = true;
      get_current_time(&worker->last_active);

      if (pool->debug_mode) {
        printf("Thread %d executing task: %s\n", worker->thread_index,
               task.name);
      }

      // Execute task
      struct timespec start_time, end_time;
      get_current_time(&start_time);

      // Execute the task function
      if (task.function) {
        task.function(task.argument);
      }

      get_current_time(&end_time);
      double execution_time = timespec_diff(&start_time, &end_time);

      // Update statistics
      pthread_mutex_lock(&pool->stats_mutex);
      pool->stats.tasks_completed++;
      worker->tasks_completed++;

      // Update average task time
      double total_time =
          pool->stats.avg_task_time * (pool->stats.tasks_completed - 1);
      pool->stats.avg_task_time =
          (total_time + execution_time) / pool->stats.tasks_completed;

      pthread_mutex_unlock(&pool->stats_mutex);

      worker->is_active = false;

      if (pool->debug_mode) {
        printf("Thread %d completed task: %s (%.3fs)\n", worker->thread_index,
               task.name, execution_time);
      }
    }
  }

  if (pool->debug_mode) {
    printf("Worker thread %d shutting down\n", worker->thread_index);
  }

  return NULL;
}

/**
 * @brief Statistics monitoring thread
 * @param arg Pointer to ThreadPool structure
 * @return NULL
 */
void *monitor_thread(void *arg) {
  ThreadPool *pool = (ThreadPool *)arg;

  while (!pool->shutdown) {
    sleep(STATS_INTERVAL);

    if (pool->shutdown)
      break;

    // Update statistics
    pthread_mutex_lock(&pool->stats_mutex);

    size_t active_count = 0;
    size_t idle_count = 0;

    for (size_t i = 0; i < pool->thread_count; i++) {
      if (pool->threads[i].is_active) {
        active_count++;
      } else {
        idle_count++;
      }
    }

    pool->stats.active_threads = active_count;
    pool->stats.idle_threads = idle_count;
    pool->stats.tasks_queued = pool->queue_count;

    pthread_mutex_unlock(&pool->stats_mutex);

    if (pool->debug_mode) {
      printf("\n=== Thread Pool Statistics ===\n");
      printf("Active threads: %zu/%zu\n", active_count, pool->thread_count);
      printf("Queued tasks: %zu\n", pool->queue_count);
      printf("Completed tasks: %zu\n", pool->stats.tasks_completed);
      printf("Average task time: %.3fs\n", pool->stats.avg_task_time);
      printf("==============================\n\n");
    }
  }

  return NULL;
}

/**
 * @brief Create thread pool
 * @param thread_count Number of worker threads
 * @param queue_size Maximum queue size
 * @param debug_mode Enable debug output
 * @return Pointer to thread pool or NULL on failure
 */
ThreadPool *thread_pool_create(size_t thread_count, size_t queue_size,
                               bool debug_mode) {
  if (thread_count == 0 || thread_count > MAX_THREADS) {
    thread_count = DEFAULT_THREADS;
  }

  if (queue_size == 0 || queue_size > MAX_QUEUE_SIZE) {
    queue_size = MAX_QUEUE_SIZE;
  }

  ThreadPool *pool = safe_calloc(1, sizeof(ThreadPool));
  if (!pool) {
    log_message("ERROR", "Failed to allocate thread pool");
    return NULL;
  }

  // Initialize basic fields
  pool->thread_count = thread_count;
  pool->min_threads = thread_count;
  pool->max_threads = thread_count;
  pool->queue_size = queue_size;
  pool->debug_mode = debug_mode;

  // Allocate arrays
  pool->threads = safe_calloc(thread_count, sizeof(WorkerThread));
  pool->task_queue = safe_calloc(queue_size, sizeof(Task));

  if (!pool->threads || !pool->task_queue) {
    log_message("ERROR", "Failed to allocate thread pool arrays");
    free(pool->threads);
    free(pool->task_queue);
    free(pool);
    return NULL;
  }

  // Initialize synchronization primitives
  if (pthread_mutex_init(&pool->queue_mutex, NULL) != 0 ||
      pthread_mutex_init(&pool->stats_mutex, NULL) != 0 ||
      pthread_cond_init(&pool->queue_not_empty, NULL) != 0 ||
      pthread_cond_init(&pool->queue_not_full, NULL) != 0) {

    log_message("ERROR", "Failed to initialize synchronization primitives");
    free(pool->threads);
    free(pool->task_queue);
    free(pool);
    return NULL;
  }

  // Initialize statistics
  get_current_time(&pool->stats.start_time);

  // Create worker threads
  for (size_t i = 0; i < thread_count; i++) {
    pool->threads[i].thread_index = i;

    if (pthread_create(&pool->threads[i].thread_id, NULL, worker_thread,
                       &pool->threads[i]) != 0) {
      log_message("ERROR", "Failed to create worker thread %zu", i);

      // Cleanup already created threads
      pool->shutdown = true;
      pthread_cond_broadcast(&pool->queue_not_empty);

      for (size_t j = 0; j < i; j++) {
        pthread_join(pool->threads[j].thread_id, NULL);
      }

      pthread_mutex_destroy(&pool->queue_mutex);
      pthread_mutex_destroy(&pool->stats_mutex);
      pthread_cond_destroy(&pool->queue_not_empty);
      pthread_cond_destroy(&pool->queue_not_full);
      free(pool->threads);
      free(pool->task_queue);
      free(pool);
      return NULL;
    }
  }

  // Create monitoring thread
  if (pthread_create(&pool->monitor_thread, NULL, monitor_thread, pool) != 0) {
    log_message("WARN", "Failed to create monitoring thread");
  }

  if (debug_mode) {
    printf("Thread pool created with %zu threads and queue size %zu\n",
           thread_count, queue_size);
  }

  return pool;
}

/**
 * @brief Submit task to thread pool
 * @param pool Pointer to thread pool
 * @param function Task function
 * @param argument Task argument
 * @param name Task name
 * @param priority Task priority
 * @return true on success, false on failure
 */
bool thread_pool_submit(ThreadPool *pool, task_func_t function, void *argument,
                        const char *name, int priority) {
  if (!pool || !function || pool->shutdown) {
    return false;
  }

  Task *task = task_create(function, argument, name, priority);
  if (!task) {
    return false;
  }

  pthread_mutex_lock(&pool->queue_mutex);

  // Wait for space in queue
  while (pool->queue_count >= pool->queue_size && !pool->shutdown) {
    pthread_cond_wait(&pool->queue_not_full, &pool->queue_mutex);
  }

  if (pool->shutdown) {
    pthread_mutex_unlock(&pool->queue_mutex);
    free(task);
    return false;
  }

  // Add task to queue
  pool->task_queue[pool->queue_tail] = *task;
  pool->queue_tail = (pool->queue_tail + 1) % pool->queue_size;
  pool->queue_count++;

  // Signal waiting threads
  pthread_cond_signal(&pool->queue_not_empty);

  pthread_mutex_unlock(&pool->queue_mutex);

  free(task);

  if (pool->debug_mode) {
    printf("Task submitted: %s (priority: %d)\n", name ? name : "unnamed",
           priority);
  }

  return true;
}

/**
 * @brief Destroy thread pool
 * @param pool Pointer to thread pool
 * @param force_shutdown Force immediate shutdown
 */
void thread_pool_destroy(ThreadPool *pool, bool force_shutdown) {
  if (!pool)
    return;

  printf("Shutting down thread pool...\n");

  // Signal shutdown
  pool->shutdown = true;
  pool->force_shutdown = force_shutdown;

  // Wake up all waiting threads
  pthread_cond_broadcast(&pool->queue_not_empty);
  pthread_cond_broadcast(&pool->queue_not_full);

  // Wait for worker threads to finish
  for (size_t i = 0; i < pool->thread_count; i++) {
    if (pthread_join(pool->threads[i].thread_id, NULL) != 0) {
      log_message("WARN", "Failed to join worker thread %zu", i);
    }
  }

  // Wait for monitor thread
  if (pthread_join(pool->monitor_thread, NULL) != 0) {
    log_message("WARN", "Failed to join monitor thread");
  }

  // Print final statistics
  printf("\n=== Final Thread Pool Statistics ===\n");
  printf("Total tasks completed: %zu\n", pool->stats.tasks_completed);
  printf("Total tasks failed: %zu\n", pool->stats.tasks_failed);
  printf("Average task time: %.3fs\n", pool->stats.avg_task_time);

  struct timespec end_time;
  get_current_time(&end_time);
  double total_time = timespec_diff(&pool->stats.start_time, &end_time);
  printf("Total runtime: %.3fs\n", total_time);
  printf("====================================\n");

  // Cleanup
  pthread_mutex_destroy(&pool->queue_mutex);
  pthread_mutex_destroy(&pool->stats_mutex);
  pthread_cond_destroy(&pool->queue_not_empty);
  pthread_cond_destroy(&pool->queue_not_full);

  free(pool->threads);
  free(pool->task_queue);
  free(pool);
}

// Example task functions for demonstration

/**
 * @brief CPU-intensive task
 * @param arg Task argument (unused)
 */
void cpu_intensive_task(void *arg) {
  int *task_id = (int *)arg;

  // Simulate CPU-intensive work
  volatile long sum = 0;
  for (long i = 0; i < 1000000; i++) {
    sum += i * i;
  }

  printf("CPU task %d completed (sum: %ld)\n", *task_id, sum);
}

/**
 * @brief I/O simulation task
 * @param arg Task argument (sleep duration)
 */
void io_simulation_task(void *arg) {
  int *sleep_ms = (int *)arg;

  printf("I/O task starting (sleep: %dms)\n", *sleep_ms);
  usleep(*sleep_ms * 1000); // Convert to microseconds
  printf("I/O task completed\n");
}

/**
 * @brief File processing task
 * @param arg Filename to process
 */
void file_processing_task(void *arg) {
  char *filename = (char *)arg;

  FILE *file = fopen(filename, "r");
  if (!file) {
    printf("Failed to open file: %s\n", filename);
    return;
  }

  size_t line_count = 0;
  size_t char_count = 0;
  int ch;

  while ((ch = fgetc(file)) != EOF) {
    char_count++;
    if (ch == '\n') {
      line_count++;
    }
  }

  fclose(file);

  printf("File %s processed: %zu lines, %zu characters\n", filename, line_count,
         char_count);
}

/**
 * @brief Print usage information
 * @param program_name Program name
 */
void print_usage(const char *program_name) {
  printf("Usage: %s [options]\n", program_name);
  printf("Options:\n");
  printf("  -t <threads>    Number of worker threads (default: %d)\n",
         DEFAULT_THREADS);
  printf("  -q <size>       Queue size (default: %d)\n", MAX_QUEUE_SIZE);
  printf("  -d              Enable debug mode\n");
  printf("  -h              Show this help message\n");
  printf("\nDemonstration modes:\n");
  printf("  -c              CPU-intensive tasks\n");
  printf("  -i              I/O simulation tasks\n");
  printf("  -f              File processing tasks\n");
  printf("  -m              Mixed workload (default)\n");
}

/**
 * @brief Main function demonstrating thread pool usage
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit status
 */
int main(int argc, char *argv[]) {
  size_t thread_count = DEFAULT_THREADS;
  size_t queue_size = MAX_QUEUE_SIZE;
  bool debug_mode = false;
  char demo_mode = 'm'; // mixed by default

  // Parse command line arguments
  int opt;
  while ((opt = getopt(argc, argv, "t:q:dcifmh")) != -1) {
    switch (opt) {
    case 't':
      if (!str_to_int(optarg, (int *)&thread_count) || thread_count == 0) {
        fprintf(stderr, "Invalid thread count: %s\n", optarg);
        return 1;
      }
      break;
    case 'q':
      if (!str_to_int(optarg, (int *)&queue_size) || queue_size == 0) {
        fprintf(stderr, "Invalid queue size: %s\n", optarg);
        return 1;
      }
      break;
    case 'd':
      debug_mode = true;
      break;
    case 'c':
      demo_mode = 'c';
      break;
    case 'i':
      demo_mode = 'i';
      break;
    case 'f':
      demo_mode = 'f';
      break;
    case 'm':
      demo_mode = 'm';
      break;
    case 'h':
      print_usage(argv[0]);
      return 0;
    default:
      print_usage(argv[0]);
      return 1;
    }
  }

  // Setup signal handlers
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  printf("=== Thread Pool Demonstration ===\n");
  printf("Threads: %zu, Queue Size: %zu, Debug: %s\n", thread_count, queue_size,
         debug_mode ? "ON" : "OFF");

  // Create thread pool
  g_thread_pool = thread_pool_create(thread_count, queue_size, debug_mode);
  if (!g_thread_pool) {
    fprintf(stderr, "Failed to create thread pool\n");
    return 1;
  }

  printf("Thread pool created successfully\n");
  printf("Press Ctrl+C to shutdown gracefully\n\n");

  // Submit tasks based on demo mode
  switch (demo_mode) {
  case 'c': {
    printf("Running CPU-intensive tasks...\n");
    for (int i = 0; i < 20; i++) {
      int *task_id = malloc(sizeof(int));
      *task_id = i;

      char task_name[32];
      snprintf(task_name, sizeof(task_name), "cpu_task_%d", i);

      thread_pool_submit(g_thread_pool, cpu_intensive_task, task_id, task_name,
                         1);
      usleep(100000); // 100ms delay between submissions
    }
    break;
  }

  case 'i': {
    printf("Running I/O simulation tasks...\n");
    for (int i = 0; i < 15; i++) {
      int *sleep_time = malloc(sizeof(int));
      *sleep_time = 500 + (rand() % 1000); // 500-1500ms

      char task_name[32];
      snprintf(task_name, sizeof(task_name), "io_task_%d", i);

      thread_pool_submit(g_thread_pool, io_simulation_task, sleep_time,
                         task_name, 2);
      usleep(200000); // 200ms delay between submissions
    }
    break;
  }

  case 'f': {
    printf("Running file processing tasks...\n");
    // Create some test files first
    for (int i = 0; i < 5; i++) {
      char filename[32];
      snprintf(filename, sizeof(filename), "test_file_%d.txt", i);

      FILE *file = fopen(filename, "w");
      if (file) {
        for (int j = 0; j < 100 + (rand() % 200); j++) {
          fprintf(file, "This is line %d in file %d\n", j, i);
        }
        fclose(file);
      }

      char *file_arg = malloc(strlen(filename) + 1);
      strcpy(file_arg, filename);

      char task_name[32];
      snprintf(task_name, sizeof(task_name), "file_task_%d", i);

      thread_pool_submit(g_thread_pool, file_processing_task, file_arg,
                         task_name, 3);
    }
    break;
  }

  case 'm':
  default: {
    printf("Running mixed workload...\n");

    // Submit various types of tasks
    for (int i = 0; i < 30 && g_running; i++) {
      int task_type = rand() % 3;

      switch (task_type) {
      case 0: {
        int *task_id = malloc(sizeof(int));
        *task_id = i;

        char task_name[32];
        snprintf(task_name, sizeof(task_name), "mixed_cpu_%d", i);

        thread_pool_submit(g_thread_pool, cpu_intensive_task, task_id,
                           task_name, 1);
        break;
      }

      case 1: {
        int *sleep_time = malloc(sizeof(int));
        *sleep_time = 200 + (rand() % 800);

        char task_name[32];
        snprintf(task_name, sizeof(task_name), "mixed_io_%d", i);

        thread_pool_submit(g_thread_pool, io_simulation_task, sleep_time,
                           task_name, 2);
        break;
      }

      case 2: {
        // Create a temporary file
        char filename[32];
        snprintf(filename, sizeof(filename), "temp_%d.txt", i);

        FILE *file = fopen(filename, "w");
        if (file) {
          for (int j = 0; j < 50; j++) {
            fprintf(file, "Temporary content line %d\n", j);
          }
          fclose(file);
        }

        char *file_arg = malloc(strlen(filename) + 1);
        strcpy(file_arg, filename);

        char task_name[32];
        snprintf(task_name, sizeof(task_name), "mixed_file_%d", i);

        thread_pool_submit(g_thread_pool, file_processing_task, file_arg,
                           task_name, 3);
        break;
      }
      }

      usleep(150000); // 150ms delay between submissions
    }
    break;
  }
  }

  // Wait for user interrupt or completion
  while (g_running) {
    sleep(1);

    // Check if all tasks are completed
    pthread_mutex_lock(&g_thread_pool->stats_mutex);
    bool all_done = (g_thread_pool->queue_count == 0 &&
                     g_thread_pool->stats.active_threads == 0);
    pthread_mutex_unlock(&g_thread_pool->stats_mutex);

    if (all_done && demo_mode != 'm') {
      printf("All tasks completed. Shutting down...\n");
      break;
    }
  }

  // Cleanup
  thread_pool_destroy(g_thread_pool, false);
  g_thread_pool = NULL;

  // Clean up temporary files
  system("rm -f test_file_*.txt temp_*.txt");

  printf("Thread pool demonstration completed\n");
  return 0;
}