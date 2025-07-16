/**
 * @file web_server.c
 * @brief HTTP web server demonstrating network programming and protocol
 * implementation
 * @author dunamismax
 * @date 2025
 *
 * This program demonstrates:
 * - HTTP protocol implementation
 * - Multi-threaded server architecture
 * - Socket programming and network I/O
 * - Request parsing and response generation
 * - Static file serving
 * - URL routing and handler dispatch
 * - Connection management and keep-alive
 * - Security considerations and validation
 * - Performance optimization techniques
 * - Logging and monitoring
 */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// Include our utility libraries
#include "dynamic_array.h"
#include "utils.h"

/**
 * @brief Maximum request size (16KB)
 */
#define MAX_REQUEST_SIZE 16384

/**
 * @brief Maximum response size (64KB)
 */
#define MAX_RESPONSE_SIZE 65536

/**
 * @brief Maximum URL length
 */
#define MAX_URL_LENGTH 512

/**
 * @brief Maximum header name/value length
 */
#define MAX_HEADER_LENGTH 256

/**
 * @brief Maximum number of headers per request
 */
#define MAX_HEADERS 32

/**
 * @brief Maximum number of concurrent connections
 */
#define MAX_CONNECTIONS 100

/**
 * @brief Default server port
 */
#define DEFAULT_PORT 8080

/**
 * @brief Connection timeout in seconds
 */
#define CONNECTION_TIMEOUT 30

/**
 * @brief HTTP methods
 */
typedef enum {
  HTTP_GET,
  HTTP_POST,
  HTTP_PUT,
  HTTP_DELETE,
  HTTP_HEAD,
  HTTP_OPTIONS,
  HTTP_UNKNOWN
} HTTPMethod;

/**
 * @brief HTTP status codes
 */
typedef enum {
  HTTP_200_OK = 200,
  HTTP_201_CREATED = 201,
  HTTP_400_BAD_REQUEST = 400,
  HTTP_401_UNAUTHORIZED = 401,
  HTTP_403_FORBIDDEN = 403,
  HTTP_404_NOT_FOUND = 404,
  HTTP_405_METHOD_NOT_ALLOWED = 405,
  HTTP_500_INTERNAL_SERVER_ERROR = 500,
  HTTP_501_NOT_IMPLEMENTED = 501
} HTTPStatus;

/**
 * @brief HTTP header structure
 */
typedef struct {
  char name[MAX_HEADER_LENGTH];
  char value[MAX_HEADER_LENGTH];
} HTTPHeader;

/**
 * @brief HTTP request structure
 *
 * Demonstrates: Request parsing, data organization
 */
typedef struct {
  HTTPMethod method;
  char url[MAX_URL_LENGTH];
  char version[16];
  HTTPHeader headers[MAX_HEADERS];
  size_t header_count;
  char *body;
  size_t body_length;
  char client_ip[INET_ADDRSTRLEN];
  time_t timestamp;
} HTTPRequest;

/**
 * @brief HTTP response structure
 *
 * Demonstrates: Response building, content management
 */
typedef struct {
  HTTPStatus status;
  char status_message[64];
  HTTPHeader headers[MAX_HEADERS];
  size_t header_count;
  char *body;
  size_t body_length;
  char content_type[64];
} HTTPResponse;

/**
 * @brief Route handler function pointer
 */
typedef void (*RouteHandler)(const HTTPRequest *request,
                             HTTPResponse *response);

/**
 * @brief URL route structure
 *
 * Demonstrates: URL routing, handler dispatch
 */
typedef struct {
  char path[MAX_URL_LENGTH];
  HTTPMethod method;
  RouteHandler handler;
  char description[128];
} Route;

/**
 * @brief Client connection structure
 *
 * Demonstrates: Connection management, client tracking
 */
typedef struct {
  int socket_fd;
  struct sockaddr_in address;
  char ip_address[INET_ADDRSTRLEN];
  time_t connect_time;
  time_t last_activity;
  bool keep_alive;
  size_t requests_served;
} ClientConnection;

/**
 * @brief Server statistics
 *
 * Demonstrates: Performance monitoring, metrics collection
 */
typedef struct {
  size_t total_requests;
  size_t total_responses;
  size_t bytes_sent;
  size_t bytes_received;
  size_t active_connections;
  size_t total_connections;
  time_t start_time;
  size_t errors_4xx;
  size_t errors_5xx;
} ServerStats;

/**
 * @brief Web server structure
 *
 * Demonstrates: Server architecture, state management
 */
typedef struct {
  int server_fd;
  int port;
  char document_root[512];
  Route routes[64];
  size_t route_count;
  ClientConnection connections[MAX_CONNECTIONS];
  pthread_mutex_t connections_mutex;
  ServerStats stats;
  pthread_mutex_t stats_mutex;
  bool running;
  bool debug_mode;
  char server_name[64];
} WebServer;

// Global server instance for signal handling
static WebServer *g_server = NULL;

/**
 * @brief Signal handler for graceful shutdown
 * @param signum Signal number
 */
void signal_handler(int signum) {
  if (g_server && (signum == SIGINT || signum == SIGTERM)) {
    log_message("INFO", "Received signal %d, shutting down server", signum);
    g_server->running = false;
  }
}

/**
 * @brief Get HTTP method string
 * @param method HTTP method enum
 * @return Method string
 */
const char *http_method_string(HTTPMethod method) {
  switch (method) {
  case HTTP_GET:
    return "GET";
  case HTTP_POST:
    return "POST";
  case HTTP_PUT:
    return "PUT";
  case HTTP_DELETE:
    return "DELETE";
  case HTTP_HEAD:
    return "HEAD";
  case HTTP_OPTIONS:
    return "OPTIONS";
  default:
    return "UNKNOWN";
  }
}

/**
 * @brief Parse HTTP method from string
 * @param method_str Method string
 * @return HTTP method enum
 */
HTTPMethod parse_http_method(const char *method_str) {
  if (!method_str)
    return HTTP_UNKNOWN;

  if (strcmp(method_str, "GET") == 0)
    return HTTP_GET;
  if (strcmp(method_str, "POST") == 0)
    return HTTP_POST;
  if (strcmp(method_str, "PUT") == 0)
    return HTTP_PUT;
  if (strcmp(method_str, "DELETE") == 0)
    return HTTP_DELETE;
  if (strcmp(method_str, "HEAD") == 0)
    return HTTP_HEAD;
  if (strcmp(method_str, "OPTIONS") == 0)
    return HTTP_OPTIONS;

  return HTTP_UNKNOWN;
}

/**
 * @brief Get HTTP status message
 * @param status HTTP status code
 * @return Status message string
 */
const char *http_status_message(HTTPStatus status) {
  switch (status) {
  case HTTP_200_OK:
    return "OK";
  case HTTP_201_CREATED:
    return "Created";
  case HTTP_400_BAD_REQUEST:
    return "Bad Request";
  case HTTP_401_UNAUTHORIZED:
    return "Unauthorized";
  case HTTP_403_FORBIDDEN:
    return "Forbidden";
  case HTTP_404_NOT_FOUND:
    return "Not Found";
  case HTTP_405_METHOD_NOT_ALLOWED:
    return "Method Not Allowed";
  case HTTP_500_INTERNAL_SERVER_ERROR:
    return "Internal Server Error";
  case HTTP_501_NOT_IMPLEMENTED:
    return "Not Implemented";
  default:
    return "Unknown";
  }
}

/**
 * @brief Initialize HTTP request structure
 * @param request Pointer to request structure
 */
void http_request_init(HTTPRequest *request) {
  if (!request)
    return;

  memset(request, 0, sizeof(HTTPRequest));
  request->method = HTTP_UNKNOWN;
  request->timestamp = time(NULL);
}

/**
 * @brief Initialize HTTP response structure
 * @param response Pointer to response structure
 */
void http_response_init(HTTPResponse *response) {
  if (!response)
    return;

  memset(response, 0, sizeof(HTTPResponse));
  response->status = HTTP_200_OK;
  strcpy(response->status_message, http_status_message(HTTP_200_OK));
  strcpy(response->content_type, "text/html; charset=utf-8");
}

/**
 * @brief Add header to HTTP response
 * @param response Pointer to response structure
 * @param name Header name
 * @param value Header value
 * @return true if header was added
 */
bool http_response_add_header(HTTPResponse *response, const char *name,
                              const char *value) {
  if (!response || !name || !value || response->header_count >= MAX_HEADERS) {
    return false;
  }

  HTTPHeader *header = &response->headers[response->header_count++];
  strncpy(header->name, name, sizeof(header->name) - 1);
  header->name[sizeof(header->name) - 1] = '\0';
  strncpy(header->value, value, sizeof(header->value) - 1);
  header->value[sizeof(header->value) - 1] = '\0';

  return true;
}

/**
 * @brief Set HTTP response body
 * @param response Pointer to response structure
 * @param body Response body content
 * @param length Body length
 * @return true if body was set
 */
bool http_response_set_body(HTTPResponse *response, const char *body,
                            size_t length) {
  if (!response || !body)
    return false;

  if (response->body) {
    free(response->body);
  }

  response->body = safe_calloc(length + 1, sizeof(char));
  if (!response->body)
    return false;

  memcpy(response->body, body, length);
  response->body_length = length;

  // Update content-length header
  char content_length[32];
  snprintf(content_length, sizeof(content_length), "%zu", length);
  http_response_add_header(response, "Content-Length", content_length);

  return true;
}

/**
 * @brief Parse HTTP request from raw data
 * @param raw_request Raw request string
 * @param request Pointer to store parsed request
 * @return true if parsing was successful
 *
 * Demonstrates: Protocol parsing, string manipulation
 */
bool parse_http_request(const char *raw_request, HTTPRequest *request) {
  if (!raw_request || !request)
    return false;

  http_request_init(request);

  // Make a copy for parsing
  size_t request_length = strlen(raw_request);
  char *request_copy = safe_calloc(request_length + 1, sizeof(char));
  if (!request_copy)
    return false;
  strcpy(request_copy, raw_request);

  // Parse request line
  char *line = strtok(request_copy, "\r\n");
  if (!line) {
    free(request_copy);
    return false;
  }

  char method_str[16], url[MAX_URL_LENGTH], version[16];
  if (sscanf(line, "%15s %511s %15s", method_str, url, version) != 3) {
    free(request_copy);
    return false;
  }

  request->method = parse_http_method(method_str);
  strncpy(request->url, url, sizeof(request->url) - 1);
  request->url[sizeof(request->url) - 1] = '\0';
  strncpy(request->version, version, sizeof(request->version) - 1);
  request->version[sizeof(request->version) - 1] = '\0';

  // Parse headers
  while ((line = strtok(NULL, "\r\n")) != NULL && strlen(line) > 0) {
    if (request->header_count >= MAX_HEADERS)
      break;

    char *colon = strchr(line, ':');
    if (!colon)
      continue;

    *colon = '\0';
    char *name = trim_whitespace(line);
    char *value = trim_whitespace(colon + 1);

    HTTPHeader *header = &request->headers[request->header_count++];
    strncpy(header->name, name, sizeof(header->name) - 1);
    header->name[sizeof(header->name) - 1] = '\0';
    strncpy(header->value, value, sizeof(header->value) - 1);
    header->value[sizeof(header->value) - 1] = '\0';
  }

  // For POST requests, parse body (simplified)
  if (request->method == HTTP_POST) {
    const char *body_start = strstr(raw_request, "\r\n\r\n");
    if (body_start) {
      body_start += 4; // Skip \r\n\r\n
      size_t body_length = strlen(body_start);
      if (body_length > 0) {
        request->body = safe_calloc(body_length + 1, sizeof(char));
        if (request->body) {
          strcpy(request->body, body_start);
          request->body_length = body_length;
        }
      }
    }
  }

  free(request_copy);
  return true;
}

/**
 * @brief Build HTTP response string
 * @param response Pointer to response structure
 * @param buffer Buffer to store response
 * @param buffer_size Size of buffer
 * @return Number of bytes written
 *
 * Demonstrates: Response formatting, protocol compliance
 */
size_t build_http_response(const HTTPResponse *response, char *buffer,
                           size_t buffer_size) {
  if (!response || !buffer || buffer_size == 0)
    return 0;

  size_t written = 0;

  // Status line
  written +=
      snprintf(buffer + written, buffer_size - written, "HTTP/1.1 %d %s\r\n",
               response->status, response->status_message);

  // Standard headers
  written += snprintf(buffer + written, buffer_size - written,
                      "Content-Type: %s\r\n", response->content_type);

  time_t now = time(NULL);
  char *time_str = ctime(&now);
  if (time_str) {
    time_str[strlen(time_str) - 1] = '\0'; // Remove newline
    written += snprintf(buffer + written, buffer_size - written,
                        "Date: %s GMT\r\n", time_str);
  }

  written += snprintf(buffer + written, buffer_size - written,
                      "Server: WebServer/1.0\r\n");

  // Custom headers
  for (size_t i = 0; i < response->header_count; i++) {
    const HTTPHeader *header = &response->headers[i];
    written += snprintf(buffer + written, buffer_size - written, "%s: %s\r\n",
                        header->name, header->value);
  }

  // End of headers
  written += snprintf(buffer + written, buffer_size - written, "\r\n");

  // Body
  if (response->body && response->body_length > 0) {
    size_t body_space = buffer_size - written;
    size_t body_to_copy =
        response->body_length < body_space ? response->body_length : body_space;

    memcpy(buffer + written, response->body, body_to_copy);
    written += body_to_copy;
  }

  return written;
}

/**
 * @brief Get MIME type for file extension
 * @param filename File name
 * @return MIME type string
 *
 * Demonstrates: File type detection, content negotiation
 */
const char *get_mime_type(const char *filename) {
  if (!filename)
    return "application/octet-stream";

  const char *ext = strrchr(filename, '.');
  if (!ext)
    return "application/octet-stream";

  ext++; // Skip the dot

  if (strcmp(ext, "html") == 0 || strcmp(ext, "htm") == 0)
    return "text/html";
  if (strcmp(ext, "css") == 0)
    return "text/css";
  if (strcmp(ext, "js") == 0)
    return "application/javascript";
  if (strcmp(ext, "json") == 0)
    return "application/json";
  if (strcmp(ext, "xml") == 0)
    return "application/xml";
  if (strcmp(ext, "txt") == 0)
    return "text/plain";
  if (strcmp(ext, "png") == 0)
    return "image/png";
  if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0)
    return "image/jpeg";
  if (strcmp(ext, "gif") == 0)
    return "image/gif";
  if (strcmp(ext, "ico") == 0)
    return "image/x-icon";
  if (strcmp(ext, "pdf") == 0)
    return "application/pdf";
  if (strcmp(ext, "zip") == 0)
    return "application/zip";

  return "application/octet-stream";
}

/**
 * @brief Serve static file
 * @param server Pointer to server structure
 * @param request Pointer to request
 * @param response Pointer to response
 *
 * Demonstrates: File serving, security validation
 */
void serve_static_file(WebServer *server, const HTTPRequest *request,
                       HTTPResponse *response) {
  if (!server || !request || !response)
    return;

  // Build file path
  char file_path[1024];
  snprintf(file_path, sizeof(file_path), "%s%s", server->document_root,
           request->url);

  // Security check - prevent directory traversal
  if (strstr(request->url, "..") || strstr(request->url, "//")) {
    response->status = HTTP_403_FORBIDDEN;
    strcpy(response->status_message, http_status_message(HTTP_403_FORBIDDEN));
    http_response_set_body(response, "<h1>403 Forbidden</h1>", 21);
    return;
  }

  // If URL ends with /, serve index.html
  if (request->url[strlen(request->url) - 1] == '/') {
    strcat(file_path, "index.html");
  }

  // Check if file exists and is readable
  struct stat file_stat;
  if (stat(file_path, &file_stat) != 0 || !S_ISREG(file_stat.st_mode)) {
    response->status = HTTP_404_NOT_FOUND;
    strcpy(response->status_message, http_status_message(HTTP_404_NOT_FOUND));

    char error_body[256];
    snprintf(
        error_body, sizeof(error_body),
        "<h1>404 Not Found</h1><p>The requested file '%s' was not found.</p>",
        request->url);
    http_response_set_body(response, error_body, strlen(error_body));
    return;
  }

  // Open and read file
  FILE *file = fopen(file_path, "rb");
  if (!file) {
    response->status = HTTP_500_INTERNAL_SERVER_ERROR;
    strcpy(response->status_message,
           http_status_message(HTTP_500_INTERNAL_SERVER_ERROR));
    http_response_set_body(response, "<h1>500 Internal Server Error</h1>", 34);
    return;
  }

  // Get file size
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (file_size <= 0 || file_size > MAX_RESPONSE_SIZE) {
    fclose(file);
    response->status = HTTP_500_INTERNAL_SERVER_ERROR;
    strcpy(response->status_message,
           http_status_message(HTTP_500_INTERNAL_SERVER_ERROR));
    http_response_set_body(response, "<h1>500 Internal Server Error</h1>", 34);
    return;
  }

  // Read file content
  char *file_content = safe_calloc(file_size + 1, sizeof(char));
  if (!file_content) {
    fclose(file);
    response->status = HTTP_500_INTERNAL_SERVER_ERROR;
    strcpy(response->status_message,
           http_status_message(HTTP_500_INTERNAL_SERVER_ERROR));
    http_response_set_body(response, "<h1>500 Internal Server Error</h1>", 34);
    return;
  }

  size_t bytes_read = fread(file_content, 1, file_size, file);
  fclose(file);

  if (bytes_read != (size_t)file_size) {
    free(file_content);
    response->status = HTTP_500_INTERNAL_SERVER_ERROR;
    strcpy(response->status_message,
           http_status_message(HTTP_500_INTERNAL_SERVER_ERROR));
    http_response_set_body(response, "<h1>500 Internal Server Error</h1>", 34);
    return;
  }

  // Set content type based on file extension
  strcpy(response->content_type, get_mime_type(file_path));

  // Set response body
  response->body = file_content;
  response->body_length = file_size;

  // Add content-length header
  char content_length[32];
  snprintf(content_length, sizeof(content_length), "%ld", file_size);
  http_response_add_header(response, "Content-Length", content_length);

  if (server->debug_mode) {
    log_message("DEBUG", "Served file: %s (%ld bytes)", file_path, file_size);
  }
}

/**
 * @brief Default route handler for root path
 * @param request Pointer to request
 * @param response Pointer to response
 */
void handle_root(const HTTPRequest *request, HTTPResponse *response) {
  (void)request; // Suppress unused parameter warning

  const char *html =
      "<!DOCTYPE html>\n"
      "<html>\n"
      "<head>\n"
      "    <title>Web Server Demo</title>\n"
      "    <style>\n"
      "        body { font-family: Arial, sans-serif; margin: 40px; }\n"
      "        .container { max-width: 800px; margin: 0 auto; }\n"
      "        .header { color: #333; border-bottom: 2px solid #007acc; "
      "padding-bottom: 10px; }\n"
      "        .section { margin: 20px 0; }\n"
      "        .endpoint { background: #f4f4f4; padding: 10px; margin: 10px 0; "
      "border-left: 4px solid #007acc; }\n"
      "        .method { font-weight: bold; color: #007acc; }\n"
      "    </style>\n"
      "</head>\n"
      "<body>\n"
      "    <div class=\"container\">\n"
      "        <h1 class=\"header\">Web Server Demo</h1>\n"
      "        <div class=\"section\">\n"
      "            <h2>Welcome to the C Web Server!</h2>\n"
      "            <p>This is a demonstration web server built in C, "
      "showcasing:</p>\n"
      "            <ul>\n"
      "                <li>HTTP protocol implementation</li>\n"
      "                <li>Multi-threaded request handling</li>\n"
      "                <li>Static file serving</li>\n"
      "                <li>URL routing and handlers</li>\n"
      "                <li>Connection management</li>\n"
      "            </ul>\n"
      "        </div>\n"
      "        <div class=\"section\">\n"
      "            <h2>Available Endpoints</h2>\n"
      "            <div class=\"endpoint\">\n"
      "                <span class=\"method\">GET</span> / - This welcome "
      "page\n"
      "            </div>\n"
      "            <div class=\"endpoint\">\n"
      "                <span class=\"method\">GET</span> /status - Server "
      "status information\n"
      "            </div>\n"
      "            <div class=\"endpoint\">\n"
      "                <span class=\"method\">GET</span> /api/time - Current "
      "server time (JSON)\n"
      "            </div>\n"
      "            <div class=\"endpoint\">\n"
      "                <span class=\"method\">GET</span> /api/stats - Server "
      "statistics (JSON)\n"
      "            </div>\n"
      "        </div>\n"
      "    </div>\n"
      "</body>\n"
      "</html>\n";

  http_response_set_body(response, html, strlen(html));
}

/**
 * @brief Status page handler
 * @param request Pointer to request
 * @param response Pointer to response
 */
void handle_status(const HTTPRequest *request, HTTPResponse *response) {
  (void)request;

  time_t now = time(NULL);
  char time_str[64];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S UTC", gmtime(&now));

  char html[2048];
  snprintf(html, sizeof(html),
           "<!DOCTYPE html>\n"
           "<html>\n"
           "<head>\n"
           "    <title>Server Status</title>\n"
           "    <style>body { font-family: Arial, sans-serif; margin: 40px; "
           "}</style>\n"
           "</head>\n"
           "<body>\n"
           "    <h1>Server Status</h1>\n"
           "    <p><strong>Status:</strong> Running</p>\n"
           "    <p><strong>Time:</strong> %s</p>\n"
           "    <p><strong>Server:</strong> WebServer/1.0</p>\n"
           "    <p><a href=\"/\">Back to Home</a></p>\n"
           "</body>\n"
           "</html>\n",
           time_str);

  http_response_set_body(response, html, strlen(html));
}

/**
 * @brief API time endpoint handler
 * @param request Pointer to request
 * @param response Pointer to response
 */
void handle_api_time(const HTTPRequest *request, HTTPResponse *response) {
  (void)request;

  time_t now = time(NULL);
  char json[256];
  snprintf(json, sizeof(json),
           "{\n"
           "  \"timestamp\": %ld,\n"
           "  \"iso_time\": \"%s\",\n"
           "  \"server\": \"WebServer/1.0\"\n"
           "}",
           now, ctime(&now));

  // Remove newline from ctime
  char *newline = strchr(json, '\n');
  if (newline && *(newline - 1) == '"') {
    *(newline - 1) = '"';
    *newline = '\0';
    strcat(json, "\n}");
  }

  strcpy(response->content_type, "application/json");
  http_response_set_body(response, json, strlen(json));
}

/**
 * @brief API stats endpoint handler
 * @param request Pointer to request
 * @param response Pointer to response
 */
void handle_api_stats(const HTTPRequest *request, HTTPResponse *response) {
  (void)request;

  if (!g_server)
    return;

  pthread_mutex_lock(&g_server->stats_mutex);

  time_t uptime = time(NULL) - g_server->stats.start_time;

  char json[1024];
  snprintf(json, sizeof(json),
           "{\n"
           "  \"total_requests\": %zu,\n"
           "  \"total_responses\": %zu,\n"
           "  \"bytes_sent\": %zu,\n"
           "  \"bytes_received\": %zu,\n"
           "  \"active_connections\": %zu,\n"
           "  \"total_connections\": %zu,\n"
           "  \"uptime_seconds\": %ld,\n"
           "  \"errors_4xx\": %zu,\n"
           "  \"errors_5xx\": %zu\n"
           "}",
           g_server->stats.total_requests, g_server->stats.total_responses,
           g_server->stats.bytes_sent, g_server->stats.bytes_received,
           g_server->stats.active_connections,
           g_server->stats.total_connections, uptime,
           g_server->stats.errors_4xx, g_server->stats.errors_5xx);

  pthread_mutex_unlock(&g_server->stats_mutex);

  strcpy(response->content_type, "application/json");
  http_response_set_body(response, json, strlen(json));
}

/**
 * @brief Initialize web server
 * @param server Pointer to server structure
 * @param port Server port
 * @param document_root Document root directory
 * @return true if initialization was successful
 */
bool web_server_init(WebServer *server, int port, const char *document_root) {
  if (!server)
    return false;

  memset(server, 0, sizeof(WebServer));

  server->port = port;
  server->running = false;
  server->debug_mode = false;
  strcpy(server->server_name, "WebServer/1.0");

  if (document_root) {
    strncpy(server->document_root, document_root,
            sizeof(server->document_root) - 1);
    server->document_root[sizeof(server->document_root) - 1] = '\0';
  } else {
    strcpy(server->document_root, "./www");
  }

  // Initialize mutexes
  if (pthread_mutex_init(&server->connections_mutex, NULL) != 0) {
    log_message("ERROR", "Failed to initialize connections mutex");
    return false;
  }

  if (pthread_mutex_init(&server->stats_mutex, NULL) != 0) {
    log_message("ERROR", "Failed to initialize stats mutex");
    pthread_mutex_destroy(&server->connections_mutex);
    return false;
  }

  // Initialize statistics
  server->stats.start_time = time(NULL);

  // Register default routes
  server->routes[server->route_count++] =
      (Route){"/", HTTP_GET, handle_root, "Home page"};
  server->routes[server->route_count++] =
      (Route){"/status", HTTP_GET, handle_status, "Server status"};
  server->routes[server->route_count++] =
      (Route){"/api/time", HTTP_GET, handle_api_time, "Current time API"};
  server->routes[server->route_count++] = (Route){
      "/api/stats", HTTP_GET, handle_api_stats, "Server statistics API"};

  log_message("INFO", "Web server initialized on port %d, document root: %s",
              port, server->document_root);
  return true;
}

/**
 * @brief Find route handler for request
 * @param server Pointer to server structure
 * @param request Pointer to request
 * @return Route handler function or NULL
 */
RouteHandler find_route_handler(WebServer *server, const HTTPRequest *request) {
  if (!server || !request)
    return NULL;

  for (size_t i = 0; i < server->route_count; i++) {
    const Route *route = &server->routes[i];
    if (route->method == request->method &&
        strcmp(route->path, request->url) == 0) {
      return route->handler;
    }
  }

  return NULL;
}

/**
 * @brief Find available connection slot
 * @param server Pointer to server structure
 * @return Connection index or -1 if none available
 */
int find_connection_slot(WebServer *server) {
  if (!server)
    return -1;

  pthread_mutex_lock(&server->connections_mutex);

  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    if (server->connections[i].socket_fd == 0) {
      pthread_mutex_unlock(&server->connections_mutex);
      return i;
    }
  }

  pthread_mutex_unlock(&server->connections_mutex);
  return -1;
}

/**
 * @brief Client connection thread
 * @param arg Pointer to connection data
 * @return NULL
 */
void *handle_client_connection(void *arg) {
  ClientConnection *conn = (ClientConnection *)arg;
  WebServer *server = g_server;

  if (!conn || !server) {
    pthread_exit(NULL);
  }

  char buffer[MAX_REQUEST_SIZE];

  while (server->running) {
    // Set socket timeout
    struct timeval timeout;
    timeout.tv_sec = CONNECTION_TIMEOUT;
    timeout.tv_usec = 0;

    if (setsockopt(conn->socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                   sizeof(timeout)) < 0) {
      if (server->debug_mode) {
        log_message("DEBUG", "Failed to set socket timeout for %s",
                    conn->ip_address);
      }
      break;
    }

    // Receive request
    ssize_t bytes_received =
        recv(conn->socket_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0) {
      if (bytes_received == 0) {
        if (server->debug_mode) {
          log_message("DEBUG", "Client %s disconnected", conn->ip_address);
        }
      } else {
        if (server->debug_mode) {
          log_message("DEBUG", "Receive error from %s: %s", conn->ip_address,
                      strerror(errno));
        }
      }
      break;
    }

    buffer[bytes_received] = '\0';
    conn->last_activity = time(NULL);

    // Update statistics
    pthread_mutex_lock(&server->stats_mutex);
    server->stats.bytes_received += bytes_received;
    server->stats.total_requests++;
    pthread_mutex_unlock(&server->stats_mutex);

    if (server->debug_mode) {
      log_message("DEBUG", "Received request from %s (%zd bytes)",
                  conn->ip_address, bytes_received);
    }

    // Parse HTTP request
    HTTPRequest request;
    if (!parse_http_request(buffer, &request)) {
      // Send 400 Bad Request
      const char *error_response = "HTTP/1.1 400 Bad Request\r\n"
                                   "Content-Type: text/html\r\n"
                                   "Content-Length: 21\r\n"
                                   "\r\n"
                                   "<h1>400 Bad Request</h1>";

      send(conn->socket_fd, error_response, strlen(error_response), 0);

      pthread_mutex_lock(&server->stats_mutex);
      server->stats.errors_4xx++;
      pthread_mutex_unlock(&server->stats_mutex);

      continue;
    }

    strcpy(request.client_ip, conn->ip_address);

    if (server->debug_mode) {
      log_message("DEBUG", "%s %s from %s", http_method_string(request.method),
                  request.url, conn->ip_address);
    }

    // Prepare response
    HTTPResponse response;
    http_response_init(&response);

    // Find and execute route handler
    RouteHandler handler = find_route_handler(server, &request);
    if (handler) {
      handler(&request, &response);
    } else {
      // Try to serve static file
      serve_static_file(server, &request, &response);
    }

    // Build and send response
    char response_buffer[MAX_RESPONSE_SIZE];
    size_t response_length = build_http_response(&response, response_buffer,
                                                 sizeof(response_buffer));

    if (response_length > 0) {
      ssize_t bytes_sent =
          send(conn->socket_fd, response_buffer, response_length, 0);

      if (bytes_sent > 0) {
        pthread_mutex_lock(&server->stats_mutex);
        server->stats.bytes_sent += bytes_sent;
        server->stats.total_responses++;

        if (response.status >= 400 && response.status < 500) {
          server->stats.errors_4xx++;
        } else if (response.status >= 500) {
          server->stats.errors_5xx++;
        }
        pthread_mutex_unlock(&server->stats_mutex);

        if (server->debug_mode) {
          log_message("DEBUG", "Sent response to %s (%zd bytes, status %d)",
                      conn->ip_address, bytes_sent, response.status);
        }
      }
    }

    // Cleanup request
    if (request.body) {
      free(request.body);
    }

    // Cleanup response
    if (response.body) {
      free(response.body);
    }

    conn->requests_served++;

    // Check for Connection: close header or HTTP/1.0
    bool close_connection = false;
    for (size_t i = 0; i < request.header_count; i++) {
      if (strcasecmp(request.headers[i].name, "Connection") == 0) {
        if (strcasecmp(request.headers[i].value, "close") == 0) {
          close_connection = true;
          break;
        }
      }
    }

    if (strncmp(request.version, "HTTP/1.0", 8) == 0) {
      close_connection = true;
    }

    if (close_connection) {
      break;
    }
  }

  // Cleanup connection
  close(conn->socket_fd);

  pthread_mutex_lock(&server->connections_mutex);
  conn->socket_fd = 0;
  server->stats.active_connections--;
  pthread_mutex_unlock(&server->connections_mutex);

  if (server->debug_mode) {
    log_message("DEBUG", "Connection closed for %s (%zu requests served)",
                conn->ip_address, conn->requests_served);
  }

  pthread_exit(NULL);
}

/**
 * @brief Start web server
 * @param server Pointer to server structure
 * @return true if server started successfully
 */
bool web_server_start(WebServer *server) {
  if (!server)
    return false;

  // Create socket
  server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server->server_fd < 0) {
    log_message("ERROR", "Failed to create server socket: %s", strerror(errno));
    return false;
  }

  // Set socket options
  int opt = 1;
  if (setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt,
                 sizeof(opt)) < 0) {
    log_message("ERROR", "Failed to set socket options: %s", strerror(errno));
    close(server->server_fd);
    return false;
  }

  // Bind socket
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(server->port);

  if (bind(server->server_fd, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    log_message("ERROR", "Failed to bind server socket: %s", strerror(errno));
    close(server->server_fd);
    return false;
  }

  // Listen for connections
  if (listen(server->server_fd, MAX_CONNECTIONS) < 0) {
    log_message("ERROR", "Failed to listen on server socket: %s",
                strerror(errno));
    close(server->server_fd);
    return false;
  }

  server->running = true;
  g_server = server; // Set global reference for signal handling

  // Set up signal handlers
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE

  log_message("INFO", "Web server started on port %d", server->port);
  log_message("INFO", "Document root: %s", server->document_root);
  log_message("INFO", "Server is ready to accept connections");

  // Main server loop
  while (server->running) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Accept connection with timeout
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(server->server_fd, &read_fds);

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    int activity =
        select(server->server_fd + 1, &read_fds, NULL, NULL, &timeout);

    if (activity < 0) {
      if (errno != EINTR) {
        log_message("ERROR", "Select error: %s", strerror(errno));
      }
      continue;
    }

    if (activity == 0) {
      // Timeout - check if server should continue running
      continue;
    }

    if (!FD_ISSET(server->server_fd, &read_fds)) {
      continue;
    }

    int client_fd = accept(server->server_fd, (struct sockaddr *)&client_addr,
                           &client_addr_len);

    if (client_fd < 0) {
      if (errno != EINTR) {
        log_message("ERROR", "Failed to accept connection: %s",
                    strerror(errno));
      }
      continue;
    }

    // Find available connection slot
    int conn_index = find_connection_slot(server);
    if (conn_index < 0) {
      log_message("WARN", "Maximum connections reached, rejecting client");
      close(client_fd);
      continue;
    }

    // Initialize connection
    ClientConnection *conn = &server->connections[conn_index];
    conn->socket_fd = client_fd;
    conn->address = client_addr;
    inet_ntop(AF_INET, &client_addr.sin_addr, conn->ip_address,
              INET_ADDRSTRLEN);
    conn->connect_time = time(NULL);
    conn->last_activity = conn->connect_time;
    conn->keep_alive = true;
    conn->requests_served = 0;

    // Update statistics
    pthread_mutex_lock(&server->stats_mutex);
    server->stats.active_connections++;
    server->stats.total_connections++;
    pthread_mutex_unlock(&server->stats_mutex);

    if (server->debug_mode) {
      log_message("DEBUG", "New connection from %s", conn->ip_address);
    }

    // Create thread to handle client
    pthread_t client_thread;
    if (pthread_create(&client_thread, NULL, handle_client_connection, conn) !=
        0) {
      log_message("ERROR", "Failed to create client thread");
      close(client_fd);

      pthread_mutex_lock(&server->connections_mutex);
      conn->socket_fd = 0;
      server->stats.active_connections--;
      pthread_mutex_unlock(&server->connections_mutex);
    } else {
      pthread_detach(client_thread);
    }
  }

  log_message("INFO", "Server shutting down...");

  // Close server socket
  close(server->server_fd);

  // Close all client connections
  pthread_mutex_lock(&server->connections_mutex);
  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    if (server->connections[i].socket_fd > 0) {
      close(server->connections[i].socket_fd);
      server->connections[i].socket_fd = 0;
    }
  }
  pthread_mutex_unlock(&server->connections_mutex);

  // Clean up mutexes
  pthread_mutex_destroy(&server->connections_mutex);
  pthread_mutex_destroy(&server->stats_mutex);

  log_message("INFO", "Web server stopped");
  return true;
}

/**
 * @brief Display server help information
 * @param program_name Program name from argv[0]
 */
void display_help(const char *program_name) {
  printf("Web Server - HTTP Server Implementation\n");
  printf("Usage: %s [options]\n\n", program_name);
  printf("Options:\n");
  printf("  -p, --port <port>       Server port (default: 8080)\n");
  printf("  -d, --document-root <path>  Document root directory (default: "
         "./www)\n");
  printf("  --debug                 Enable debug output\n");
  printf("  --help                  Show this help\n\n");
  printf("Features demonstrated:\n");
  printf("- HTTP/1.1 protocol implementation\n");
  printf("- Multi-threaded connection handling\n");
  printf("- Static file serving with MIME types\n");
  printf("- URL routing and custom handlers\n");
  printf("- Connection management and keep-alive\n");
  printf("- Server statistics and monitoring\n");
  printf("- Security considerations (path traversal protection)\n");
  printf("- Graceful shutdown handling\n");
}

/**
 * @brief Main function
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return Exit status
 */
int main(int argc, char *argv[]) {
  int port = DEFAULT_PORT;
  char document_root[512] = "./www";
  bool debug_mode = false;

  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0) {
      display_help(argv[0]);
      return 0;
    } else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) {
      if (++i >= argc) {
        printf("Error: Port value required\n");
        return 1;
      }
      if (!str_to_int(argv[i], &port) || port <= 0 || port > 65535) {
        printf("Error: Invalid port number\n");
        return 1;
      }
    } else if (strcmp(argv[i], "-d") == 0 ||
               strcmp(argv[i], "--document-root") == 0) {
      if (++i >= argc) {
        printf("Error: Document root path required\n");
        return 1;
      }
      strncpy(document_root, argv[i], sizeof(document_root) - 1);
      document_root[sizeof(document_root) - 1] = '\0';
    } else if (strcmp(argv[i], "--debug") == 0) {
      debug_mode = true;
    } else {
      printf("Error: Unknown option: %s\n", argv[i]);
      display_help(argv[0]);
      return 1;
    }
  }

  // Initialize and start server
  WebServer server;
  if (!web_server_init(&server, port, document_root)) {
    printf("Error: Failed to initialize web server\n");
    return 1;
  }

  server.debug_mode = debug_mode;

  if (!web_server_start(&server)) {
    printf("Error: Failed to start web server\n");
    return 1;
  }

  log_message("INFO", "Web server application terminated");
  return 0;
}

/**
 * Educational Notes:
 *
 * 1. Network Programming:
 *    - Socket creation, binding, and listening
 *    - Connection acceptance and management
 *    - Non-blocking I/O and timeouts
 *
 * 2. HTTP Protocol:
 *    - Request parsing and validation
 *    - Response formatting and headers
 *    - Status codes and error handling
 *
 * 3. Multi-threading:
 *    - Thread creation and management
 *    - Mutex synchronization for shared data
 *    - Thread-safe programming practices
 *
 * 4. File Operations:
 *    - Static file serving
 *    - MIME type detection
 *    - Security validation (path traversal protection)
 *
 * 5. Server Architecture:
 *    - Request routing and handler dispatch
 *    - Connection pooling and management
 *    - Performance monitoring and statistics
 *
 * 6. Memory Management:
 *    - Dynamic allocation for variable-sized data
 *    - Proper cleanup and resource management
 *    - Buffer management for network I/O
 *
 * 7. Error Handling:
 *    - Network error handling and recovery
 *    - HTTP error responses
 *    - Graceful shutdown procedures
 *
 * 8. Security Considerations:
 *    - Input validation and sanitization
 *    - Path traversal attack prevention
 *    - Resource limits and DoS protection
 */