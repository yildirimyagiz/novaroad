/**
 * Nova Native Standard Library - System Module
 * Provides filesystem, process, and network operations
 */

#ifndef NOVA_STDLIB_SYSTEM_H
#define NOVA_STDLIB_SYSTEM_H

#include <stdbool.h>
#include <stddef.h>

// ═══════════════════════════════════════════════════════════════════════════
// FILESYSTEM
// ═══════════════════════════════════════════════════════════════════════════

// Read entire file contents (caller must free)
char *nova_read_file(const char *path);

// Write content to file
bool nova_write_file(const char *path, const char *content);

// Append content to file
bool nova_append_file(const char *path, const char *content);

// Check if file exists
bool nova_file_exists(const char *path);

// Get file size
size_t nova_file_size(const char *path);

// Delete file
bool nova_delete_file(const char *path);

// Create directory
bool nova_mkdir(const char *path);

// List directory contents (NULL-terminated array, caller frees)
char **nova_list_dir(const char *path);

// ═══════════════════════════════════════════════════════════════════════════
// PROCESS
// ═══════════════════════════════════════════════════════════════════════════

// Execute command and wait for completion
int nova_exec(const char *cmd);

// Execute and capture output (caller must free)
char *nova_exec_output(const char *cmd);

// Get environment variable
char *nova_getenv(const char *name);

// Set environment variable
bool nova_setenv(const char *name, const char *value);

// Get current working directory (caller must free)
char *nova_getcwd(void);

// Change directory
bool nova_chdir(const char *path);

// ═══════════════════════════════════════════════════════════════════════════
// NETWORK
// ═══════════════════════════════════════════════════════════════════════════

// TCP client connection
typedef struct {
  int fd;
  char *host;
  int port;
  bool connected;
} NovaSocket;

NovaSocket *nova_tcp_connect(const char *host, int port);
void nova_socket_close(NovaSocket *sock);
int nova_socket_send(NovaSocket *sock, const char *data, size_t len);
int nova_socket_recv(NovaSocket *sock, char *buffer, size_t max_len);

// TCP server
typedef struct {
  int fd;
  int port;
  bool listening;
} NovaServer;

NovaServer *nova_tcp_listen(int port);
NovaSocket *nova_server_accept(NovaServer *server);
void nova_server_close(NovaServer *server);

#endif // NOVA_STDLIB_SYSTEM_H
