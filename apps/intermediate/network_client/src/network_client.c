/**
 * @file network_client.c
 * @brief Network client demonstrating socket programming and network communication
 * @author dunamismax
 * @date 2025
 * 
 * This program demonstrates:
 * - Socket programming fundamentals
 * - TCP/UDP client implementation
 * - Network protocol handling
 * - Connection management and error handling
 * - Data serialization and deserialization
 * - Non-blocking I/O and timeouts
 * - DNS resolution and address handling
 * - HTTP client implementation
 * - SSL/TLS basics (conceptual)
 * - Network debugging techniques
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <ctype.h>

// Include our utility libraries
#include "utils.h"
#include "dynamic_array.h"

/**
 * @brief Maximum buffer size for network operations
 */
#define MAX_BUFFER_SIZE 8192

/**
 * @brief Maximum hostname length
 */
#define MAX_HOSTNAME_LENGTH 256

/**
 * @brief Default connection timeout in seconds
 */
#define DEFAULT_TIMEOUT 10

/**
 * @brief Network protocol types
 */
typedef enum {
    PROTOCOL_TCP,
    PROTOCOL_UDP,
    PROTOCOL_HTTP
} NetworkProtocol;

/**
 * @brief Connection status
 */
typedef enum {
    CONNECTION_DISCONNECTED,
    CONNECTION_CONNECTING,
    CONNECTION_CONNECTED,
    CONNECTION_ERROR
} ConnectionStatus;

/**
 * @brief Network client structure
 * 
 * Demonstrates: Network state management,
 * connection tracking, configuration storage
 */
typedef struct {
    char hostname[MAX_HOSTNAME_LENGTH];
    int port;
    NetworkProtocol protocol;
    int socket_fd;
    ConnectionStatus status;
    struct sockaddr_in server_addr;
    time_t connect_time;
    size_t bytes_sent;
    size_t bytes_received;
    bool debug_mode;
} NetworkClient;

/**
 * @brief HTTP response structure
 * 
 * Demonstrates: Protocol-specific data structures,
 * response parsing and storage
 */
typedef struct {
    int status_code;
    char status_message[128];
    char* headers;
    char* body;
    size_t content_length;
} HTTPResponse;

/**
 * @brief Initialize network client
 * @param client Pointer to client structure
 * @param hostname Server hostname or IP address
 * @param port Server port number
 * @param protocol Network protocol to use
 * @return true if initialization was successful
 * 
 * Demonstrates: Structure initialization,
 * parameter validation, state setup
 */
bool network_client_init(NetworkClient* client, const char* hostname, int port, NetworkProtocol protocol) {
    if (!client || !hostname) return false;
    
    // Initialize client structure
    memset(client, 0, sizeof(NetworkClient));
    
    strncpy(client->hostname, hostname, sizeof(client->hostname) - 1);
    client->hostname[sizeof(client->hostname) - 1] = '\0';
    
    client->port = port;
    client->protocol = protocol;
    client->socket_fd = -1;
    client->status = CONNECTION_DISCONNECTED;
    client->debug_mode = false;
    
    log_message("INFO", "Network client initialized for %s:%d", hostname, port);
    return true;
}

/**
 * @brief Resolve hostname to IP address
 * @param hostname Hostname to resolve
 * @param ip_address Buffer to store resolved IP
 * @param buffer_size Size of IP address buffer
 * @return true if resolution was successful
 * 
 * Demonstrates: DNS resolution, address conversion,
 * network address handling
 */
bool resolve_hostname(const char* hostname, char* ip_address, size_t buffer_size) {
    if (!hostname || !ip_address || buffer_size == 0) return false;
    
    struct hostent* host_entry;
    struct in_addr addr;
    
    // Check if hostname is already an IP address
    if (inet_aton(hostname, &addr)) {
        strncpy(ip_address, hostname, buffer_size - 1);
        ip_address[buffer_size - 1] = '\0';
        return true;
    }
    
    // Resolve hostname
    host_entry = gethostbyname(hostname);
    if (!host_entry) {
        log_message("ERROR", "Failed to resolve hostname: %s", hostname);
        return false;
    }
    
    // Convert to IP string
    char* ip = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
    if (!ip) {
        log_message("ERROR", "Failed to convert IP address");
        return false;
    }
    
    strncpy(ip_address, ip, buffer_size - 1);
    ip_address[buffer_size - 1] = '\0';
    
    log_message("INFO", "Resolved %s to %s", hostname, ip_address);
    return true;
}

/**
 * @brief Set socket timeout
 * @param socket_fd Socket file descriptor
 * @param timeout_seconds Timeout in seconds
 * @return true if timeout was set successfully
 * 
 * Demonstrates: Socket options, timeout configuration,
 * non-blocking operations
 */
bool set_socket_timeout(int socket_fd, int timeout_seconds) {
    if (socket_fd < 0 || timeout_seconds <= 0) return false;
    
    struct timeval timeout;
    timeout.tv_sec = timeout_seconds;
    timeout.tv_usec = 0;
    
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        log_message("ERROR", "Failed to set receive timeout");
        return false;
    }
    
    if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        log_message("ERROR", "Failed to set send timeout");
        return false;
    }
    
    return true;
}

/**
 * @brief Connect to server using TCP
 * @param client Pointer to client structure
 * @return true if connection was successful
 * 
 * Demonstrates: TCP socket creation, connection establishment,
 * error handling for network operations
 */
bool tcp_connect(NetworkClient* client) {
    if (!client) return false;
    
    client->status = CONNECTION_CONNECTING;
    
    // Create socket
    client->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client->socket_fd < 0) {
        log_message("ERROR", "Failed to create TCP socket");
        client->status = CONNECTION_ERROR;
        return false;
    }
    
    // Set socket timeout
    if (!set_socket_timeout(client->socket_fd, DEFAULT_TIMEOUT)) {
        close(client->socket_fd);
        client->socket_fd = -1;
        client->status = CONNECTION_ERROR;
        return false;
    }
    
    // Resolve hostname
    char ip_address[INET_ADDRSTRLEN];
    if (!resolve_hostname(client->hostname, ip_address, sizeof(ip_address))) {
        close(client->socket_fd);
        client->socket_fd = -1;
        client->status = CONNECTION_ERROR;
        return false;
    }
    
    // Setup server address
    memset(&client->server_addr, 0, sizeof(client->server_addr));
    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(client->port);
    client->server_addr.sin_addr.s_addr = inet_addr(ip_address);
    
    // Connect to server
    if (connect(client->socket_fd, (struct sockaddr*)&client->server_addr, sizeof(client->server_addr)) < 0) {
        log_message("ERROR", "Failed to connect to %s:%d - %s", 
                   client->hostname, client->port, strerror(errno));
        close(client->socket_fd);
        client->socket_fd = -1;
        client->status = CONNECTION_ERROR;
        return false;
    }
    
    client->status = CONNECTION_CONNECTED;
    client->connect_time = time(NULL);
    
    log_message("INFO", "Connected to %s:%d via TCP", client->hostname, client->port);
    return true;
}

/**
 * @brief Create UDP socket for communication
 * @param client Pointer to client structure
 * @return true if socket was created successfully
 * 
 * Demonstrates: UDP socket creation, connectionless setup,
 * datagram communication preparation
 */
bool udp_setup(NetworkClient* client) {
    if (!client) return false;
    
    // Create UDP socket
    client->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client->socket_fd < 0) {
        log_message("ERROR", "Failed to create UDP socket");
        client->status = CONNECTION_ERROR;
        return false;
    }
    
    // Set socket timeout
    if (!set_socket_timeout(client->socket_fd, DEFAULT_TIMEOUT)) {
        close(client->socket_fd);
        client->socket_fd = -1;
        client->status = CONNECTION_ERROR;
        return false;
    }
    
    // Resolve hostname
    char ip_address[INET_ADDRSTRLEN];
    if (!resolve_hostname(client->hostname, ip_address, sizeof(ip_address))) {
        close(client->socket_fd);
        client->socket_fd = -1;
        client->status = CONNECTION_ERROR;
        return false;
    }
    
    // Setup server address
    memset(&client->server_addr, 0, sizeof(client->server_addr));
    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(client->port);
    client->server_addr.sin_addr.s_addr = inet_addr(ip_address);
    
    client->status = CONNECTION_CONNECTED;
    
    log_message("INFO", "UDP socket created for %s:%d", client->hostname, client->port);
    return true;
}

/**
 * @brief Send data over network connection
 * @param client Pointer to client structure
 * @param data Data to send
 * @param data_length Length of data to send
 * @return Number of bytes sent, or -1 on error
 * 
 * Demonstrates: Network data transmission, error handling,
 * protocol-specific sending
 */
ssize_t network_send(NetworkClient* client, const void* data, size_t data_length) {
    if (!client || !data || data_length == 0 || client->socket_fd < 0) {
        return -1;
    }
    
    ssize_t bytes_sent;
    
    if (client->protocol == PROTOCOL_UDP) {
        // UDP sendto
        bytes_sent = sendto(client->socket_fd, data, data_length, 0,
                           (struct sockaddr*)&client->server_addr, sizeof(client->server_addr));
    } else {
        // TCP send
        bytes_sent = send(client->socket_fd, data, data_length, 0);
    }
    
    if (bytes_sent < 0) {
        log_message("ERROR", "Failed to send data: %s", strerror(errno));
        return -1;
    }
    
    client->bytes_sent += bytes_sent;
    
    if (client->debug_mode) {
        log_message("DEBUG", "Sent %zd bytes", bytes_sent);
    }
    
    return bytes_sent;
}

/**
 * @brief Receive data from network connection
 * @param client Pointer to client structure
 * @param buffer Buffer to store received data
 * @param buffer_size Size of receive buffer
 * @return Number of bytes received, or -1 on error
 * 
 * Demonstrates: Network data reception, buffer management,
 * protocol-specific receiving
 */
ssize_t network_receive(NetworkClient* client, void* buffer, size_t buffer_size) {
    if (!client || !buffer || buffer_size == 0 || client->socket_fd < 0) {
        return -1;
    }
    
    ssize_t bytes_received;
    
    if (client->protocol == PROTOCOL_UDP) {
        // UDP recvfrom
        socklen_t addr_len = sizeof(client->server_addr);
        bytes_received = recvfrom(client->socket_fd, buffer, buffer_size - 1, 0,
                                 (struct sockaddr*)&client->server_addr, &addr_len);
    } else {
        // TCP recv
        bytes_received = recv(client->socket_fd, buffer, buffer_size - 1, 0);
    }
    
    if (bytes_received < 0) {
        log_message("ERROR", "Failed to receive data: %s", strerror(errno));
        return -1;
    }
    
    if (bytes_received == 0) {
        log_message("INFO", "Connection closed by server");
        client->status = CONNECTION_DISCONNECTED;
        return 0;
    }
    
    client->bytes_received += bytes_received;
    
    // Null-terminate for string operations
    ((char*)buffer)[bytes_received] = '\0';
    
    if (client->debug_mode) {
        log_message("DEBUG", "Received %zd bytes", bytes_received);
    }
    
    return bytes_received;
}

/**
 * @brief Parse HTTP response
 * @param response_data Raw HTTP response data
 * @param response Pointer to store parsed response
 * @return true if parsing was successful
 * 
 * Demonstrates: Protocol parsing, string manipulation,
 * structured data extraction
 */
bool parse_http_response(const char* response_data, HTTPResponse* response) {
    if (!response_data || !response) return false;
    
    memset(response, 0, sizeof(HTTPResponse));
    
    // Parse status line
    const char* line_end = strstr(response_data, "\r\n");
    if (!line_end) {
        log_message("ERROR", "Invalid HTTP response format");
        return false;
    }
    
    // Extract status code and message
    if (sscanf(response_data, "HTTP/%*s %d %127s", 
               &response->status_code, response->status_message) != 2) {
        log_message("ERROR", "Failed to parse HTTP status line");
        return false;
    }
    
    // Find headers section
    const char* headers_start = line_end + 2;
    const char* headers_end = strstr(headers_start, "\r\n\r\n");
    if (!headers_end) {
        log_message("ERROR", "Invalid HTTP response format - no header/body separator");
        return false;
    }
    
    // Copy headers
    size_t headers_length = headers_end - headers_start;
    response->headers = safe_calloc(headers_length + 1, sizeof(char));
    if (response->headers) {
        memcpy(response->headers, headers_start, headers_length);
        response->headers[headers_length] = '\0';
    }
    
    // Extract content length from headers
    const char* content_length_header = strstr(response->headers, "Content-Length:");
    if (content_length_header) {
        sscanf(content_length_header, "Content-Length: %zu", &response->content_length);
    }
    
    // Copy body
    const char* body_start = headers_end + 4;
    size_t body_length = strlen(body_start);
    
    if (body_length > 0) {
        response->body = safe_calloc(body_length + 1, sizeof(char));
        if (response->body) {
            strcpy(response->body, body_start);
        }
    }
    
    return true;
}

/**
 * @brief Send HTTP GET request
 * @param client Pointer to client structure
 * @param path Request path
 * @param response Pointer to store HTTP response
 * @return true if request was successful
 * 
 * Demonstrates: HTTP protocol implementation,
 * request formatting, response handling
 */
bool http_get_request(NetworkClient* client, const char* path, HTTPResponse* response) {
    if (!client || !path || !response) return false;
    
    // Build HTTP GET request
    char request[MAX_BUFFER_SIZE];
    int request_length = snprintf(request, sizeof(request),
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "User-Agent: NetworkClient/1.0\r\n"
        "\r\n",
        path, client->hostname);
    
    if (request_length >= sizeof(request)) {
        log_message("ERROR", "HTTP request too large");
        return false;
    }
    
    // Send request
    if (network_send(client, request, request_length) < 0) {
        return false;
    }
    
    log_message("INFO", "Sent HTTP GET request for %s", path);
    
    // Receive response
    char response_buffer[MAX_BUFFER_SIZE * 4]; // Larger buffer for responses
    ssize_t total_received = 0;
    ssize_t bytes_received;
    
    // Receive response in chunks
    while (total_received < sizeof(response_buffer) - 1) {
        bytes_received = network_receive(client, 
                                       response_buffer + total_received,
                                       sizeof(response_buffer) - total_received);
        
        if (bytes_received <= 0) break;
        
        total_received += bytes_received;
        
        // Check if we have received the complete headers
        if (strstr(response_buffer, "\r\n\r\n")) {
            // For simplicity, assume we got the complete response
            break;
        }
    }
    
    if (total_received == 0) {
        log_message("ERROR", "No HTTP response received");
        return false;
    }
    
    response_buffer[total_received] = '\0';
    
    // Parse HTTP response
    if (!parse_http_response(response_buffer, response)) {
        return false;
    }
    
    log_message("INFO", "Received HTTP response: %d %s", 
               response->status_code, response->status_message);
    
    return true;
}

/**
 * @brief Free HTTP response resources
 * @param response Pointer to HTTP response structure
 * 
 * Demonstrates: Resource cleanup, memory management
 */
void free_http_response(HTTPResponse* response) {
    if (!response) return;
    
    if (response->headers) {
        free(response->headers);
        response->headers = NULL;
    }
    
    if (response->body) {
        free(response->body);
        response->body = NULL;
    }
}

/**
 * @brief Disconnect from server
 * @param client Pointer to client structure
 * 
 * Demonstrates: Connection cleanup, resource management,
 * graceful disconnection
 */
void network_disconnect(NetworkClient* client) {
    if (!client) return;
    
    if (client->socket_fd >= 0) {
        close(client->socket_fd);
        client->socket_fd = -1;
    }
    
    client->status = CONNECTION_DISCONNECTED;
    
    log_message("INFO", "Disconnected from %s:%d", client->hostname, client->port);
    log_message("INFO", "Session stats - Sent: %zu bytes, Received: %zu bytes", 
               client->bytes_sent, client->bytes_received);
}

/**
 * @brief Display client statistics
 * @param client Pointer to client structure
 * 
 * Demonstrates: Statistics display, time calculations,
 * performance monitoring
 */
void display_client_stats(NetworkClient* client) {
    if (!client) return;
    
    printf("\n=== Network Client Statistics ===\n");
    printf("Server: %s:%d\n", client->hostname, client->port);
    printf("Protocol: %s\n", 
           client->protocol == PROTOCOL_TCP ? "TCP" :
           client->protocol == PROTOCOL_UDP ? "UDP" : "HTTP");
    printf("Status: %s\n",
           client->status == CONNECTION_CONNECTED ? "Connected" :
           client->status == CONNECTION_CONNECTING ? "Connecting" :
           client->status == CONNECTION_ERROR ? "Error" : "Disconnected");
    
    if (client->status == CONNECTION_CONNECTED && client->connect_time > 0) {
        time_t current_time = time(NULL);
        printf("Connected for: %ld seconds\n", current_time - client->connect_time);
    }
    
    printf("Bytes sent: %zu\n", client->bytes_sent);
    printf("Bytes received: %zu\n", client->bytes_received);
    printf("===============================\n");
}

/**
 * @brief Interactive command processor for network operations
 * @param client Pointer to client structure
 * 
 * Demonstrates: Interactive applications, command parsing,
 * user interface design
 */
void run_interactive_mode(NetworkClient* client) {
    if (!client) return;
    
    printf("\n=== Interactive Network Client ===\n");
    printf("Commands:\n");
    printf("  send <message>  - Send message to server\n");
    printf("  receive         - Receive data from server\n");
    printf("  http <path>     - Send HTTP GET request\n");
    printf("  stats           - Show client statistics\n");
    printf("  debug           - Toggle debug mode\n");
    printf("  help            - Show this help\n");
    printf("  quit            - Exit client\n");
    printf("================================\n");
    
    char command[MAX_BUFFER_SIZE];
    
    while (true) {
        printf("\nnetwork> ");
        fflush(stdout);
        
        if (!fgets(command, sizeof(command), stdin)) {
            break;
        }
        
        // Remove trailing newline
        char* newline = strchr(command, '\n');
        if (newline) *newline = '\0';
        
        // Skip empty commands
        if (strlen(trim_whitespace(command)) == 0) {
            continue;
        }
        
        // Parse command
        char* cmd = strtok(command, " \t");
        if (!cmd) continue;
        
        if (strcmp(cmd, "send") == 0) {
            char* message = strtok(NULL, "");
            if (!message) {
                printf("Usage: send <message>\n");
                continue;
            }
            
            message = trim_whitespace(message);
            if (strlen(message) == 0) {
                printf("Error: Empty message\n");
                continue;
            }
            
            if (network_send(client, message, strlen(message)) < 0) {
                printf("Error: Failed to send message\n");
            } else {
                printf("Message sent successfully\n");
            }
            
        } else if (strcmp(cmd, "receive") == 0) {
            char buffer[MAX_BUFFER_SIZE];
            ssize_t bytes = network_receive(client, buffer, sizeof(buffer));
            
            if (bytes > 0) {
                printf("Received: %s\n", buffer);
            } else if (bytes == 0) {
                printf("Connection closed by server\n");
            } else {
                printf("Error: Failed to receive data\n");
            }
            
        } else if (strcmp(cmd, "http") == 0) {
            if (client->protocol != PROTOCOL_HTTP) {
                printf("Error: Client not configured for HTTP\n");
                continue;
            }
            
            char* path = strtok(NULL, " \t");
            if (!path) {
                path = "/";
            }
            
            HTTPResponse response;
            if (http_get_request(client, path, &response)) {
                printf("\n--- HTTP Response ---\n");
                printf("Status: %d %s\n", response.status_code, response.status_message);
                
                if (response.headers) {
                    printf("\nHeaders:\n%s\n", response.headers);
                }
                
                if (response.body) {
                    printf("\nBody:\n%s\n", response.body);
                }
                
                free_http_response(&response);
            } else {
                printf("Error: HTTP request failed\n");
            }
            
        } else if (strcmp(cmd, "stats") == 0) {
            display_client_stats(client);
            
        } else if (strcmp(cmd, "debug") == 0) {
            client->debug_mode = !client->debug_mode;
            printf("Debug mode: %s\n", client->debug_mode ? "enabled" : "disabled");
            
        } else if (strcmp(cmd, "help") == 0) {
            printf("\nCommands:\n");
            printf("  send <message>  - Send message to server\n");
            printf("  receive         - Receive data from server\n");
            printf("  http <path>     - Send HTTP GET request\n");
            printf("  stats           - Show client statistics\n");
            printf("  debug           - Toggle debug mode\n");
            printf("  help            - Show this help\n");
            printf("  quit            - Exit client\n");
            
        } else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            break;
            
        } else {
            printf("Unknown command: %s\n", cmd);
            printf("Type 'help' for available commands\n");
        }
    }
}

/**
 * @brief Display help information
 * @param program_name Program name from argv[0]
 * 
 * Demonstrates: Help text formatting, usage documentation
 */
void display_help(const char* program_name) {
    printf("Network Client - Socket Programming Demonstration\n");
    printf("Usage: %s [options] <hostname> <port>\n\n", program_name);
    printf("Options:\n");
    printf("  -t, --tcp       Use TCP protocol (default)\n");
    printf("  -u, --udp       Use UDP protocol\n");
    printf("  -h, --http      Use HTTP protocol\n");
    printf("  -i, --interactive  Run in interactive mode\n");
    printf("  -d, --debug     Enable debug output\n");
    printf("  --help          Show this help\n\n");
    printf("Examples:\n");
    printf("  %s google.com 80                 # TCP connection\n", program_name);
    printf("  %s -u 8.8.8.8 53               # UDP connection\n", program_name);
    printf("  %s -h google.com 80             # HTTP connection\n", program_name);
    printf("  %s -i google.com 80             # Interactive mode\n", program_name);
    printf("\nFeatures demonstrated:\n");
    printf("- TCP and UDP socket programming\n");
    printf("- DNS resolution and address handling\n");
    printf("- HTTP client implementation\n");
    printf("- Network error handling and timeouts\n");
    printf("- Connection management and statistics\n");
    printf("- Interactive network operations\n");
}

/**
 * @brief Main function
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return Exit status
 * 
 * Demonstrates: Command line processing, network client setup,
 * protocol selection and connection management
 */
int main(int argc, char* argv[]) {
    NetworkClient client;
    NetworkProtocol protocol = PROTOCOL_TCP;
    bool interactive_mode = false;
    bool debug_mode = false;
    char* hostname = NULL;
    int port = 0;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            display_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tcp") == 0) {
            protocol = PROTOCOL_TCP;
        } else if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--udp") == 0) {
            protocol = PROTOCOL_UDP;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--http") == 0) {
            protocol = PROTOCOL_HTTP;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interactive") == 0) {
            interactive_mode = true;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            debug_mode = true;
        } else if (!hostname) {
            hostname = argv[i];
        } else if (port == 0) {
            if (!str_to_int(argv[i], &port) || port <= 0 || port > 65535) {
                printf("Error: Invalid port number: %s\n", argv[i]);
                return 1;
            }
        } else {
            printf("Error: Too many arguments\n");
            display_help(argv[0]);
            return 1;
        }
    }
    
    if (!hostname || port == 0) {
        printf("Error: Hostname and port are required\n");
        display_help(argv[0]);
        return 1;
    }
    
    // Initialize client
    if (!network_client_init(&client, hostname, port, protocol)) {
        printf("Error: Failed to initialize network client\n");
        return 1;
    }
    
    client.debug_mode = debug_mode;
    
    // Connect to server
    bool connected = false;
    if (protocol == PROTOCOL_UDP) {
        connected = udp_setup(&client);
    } else {
        connected = tcp_connect(&client);
    }
    
    if (!connected) {
        printf("Error: Failed to connect to %s:%d\n", hostname, port);
        return 1;
    }
    
    log_message("INFO", "Network client started");
    
    // Run client
    if (interactive_mode) {
        run_interactive_mode(&client);
    } else {
        // Simple test mode
        printf("Connected to %s:%d using %s\n", hostname, port,
               protocol == PROTOCOL_TCP ? "TCP" :
               protocol == PROTOCOL_UDP ? "UDP" : "HTTP");
        
        if (protocol == PROTOCOL_HTTP) {
            HTTPResponse response;
            if (http_get_request(&client, "/", &response)) {
                printf("\nHTTP Response:\n");
                printf("Status: %d %s\n", response.status_code, response.status_message);
                
                if (response.body && strlen(response.body) > 0) {
                    // Show first 500 characters of body
                    printf("\nBody preview:\n");
                    if (strlen(response.body) > 500) {
                        printf("%.500s...\n", response.body);
                    } else {
                        printf("%s\n", response.body);
                    }
                }
                
                free_http_response(&response);
            }
        } else {
            // Simple echo test
            const char* test_message = "Hello from network client!";
            printf("Sending test message: %s\n", test_message);
            
            if (network_send(&client, test_message, strlen(test_message)) > 0) {
                char response[MAX_BUFFER_SIZE];
                ssize_t bytes = network_receive(&client, response, sizeof(response));
                
                if (bytes > 0) {
                    printf("Server response: %s\n", response);
                } else {
                    printf("No response from server\n");
                }
            }
        }
        
        display_client_stats(&client);
    }
    
    // Cleanup
    network_disconnect(&client);
    
    log_message("INFO", "Network client terminated");
    return 0;
}

/**
 * Educational Notes:
 * 
 * 1. Socket Programming:
 *    - TCP and UDP socket creation and management
 *    - Socket options and timeout configuration
 *    - Connection establishment and data transfer
 * 
 * 2. Network Protocols:
 *    - Protocol-specific implementation (TCP/UDP/HTTP)
 *    - Request/response patterns
 *    - Data formatting and parsing
 * 
 * 3. Address Resolution:
 *    - DNS hostname resolution
 *    - IP address handling and conversion
 *    - Network byte order considerations
 * 
 * 4. Error Handling:
 *    - Network-specific error conditions
 *    - Timeout and connection management
 *    - Graceful error recovery
 * 
 * 5. Data Management:
 *    - Buffer management for network I/O
 *    - Dynamic memory allocation for responses
 *    - Protocol-specific data structures
 * 
 * 6. Performance Monitoring:
 *    - Connection statistics tracking
 *    - Bandwidth monitoring
 *    - Performance optimization techniques
 * 
 * 7. User Interface:
 *    - Interactive command processing
 *    - Command line argument parsing
 *    - User-friendly error messages
 * 
 * 8. Security Considerations:
 *    - Input validation and sanitization
 *    - Buffer overflow protection
 *    - Connection security awareness
 */