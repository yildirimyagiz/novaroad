
/*
 * Nova Native Runtime - Networking (Real Implementation)
 * Wraps POSIX/BSD Sockets for Nova Standard Library
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// ============================================
// SOCKET TYPES
// ============================================

typedef struct {
  int fd;
  int domain; // AF_INET, etc.
  int type;   // SOCK_STREAM, etc.
  int protocol;
} check_socket_t;

// ============================================
// NATIVE FUNCTIONS EXPORTED TO NOVA
// ============================================

// Create a new socket
// Returns: socket file descriptor (int64_t for Nova compatibility)
int64_t nova_net_socket_create(int64_t domain, int64_t type, int64_t protocol) {
  // Default to TCP/IPv4 if 0 passed
  if (domain == 0)
    domain = AF_INET;
  if (type == 0)
    type = SOCK_STREAM;

  int fd = socket((int)domain, (int)type, (int)protocol);
  if (fd < 0) {
    perror("nova_net_socket_create");
    return -1;
  }

  // Set REUSEADDR by default for servers
  int opt = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  return (int64_t)fd;
}

// Bind socket to address and port
int64_t nova_net_socket_bind(int64_t fd, const char *host, int64_t port) {
  if (fd < 0)
    return -1;

  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons((uint16_t)port);

  if (host == NULL || strlen(host) == 0 || strcmp(host, "0.0.0.0") == 0) {
    serv_addr.sin_addr.s_addr = INADDR_ANY;
  } else {
    if (inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0) {
      return -2; // Invalid address
    }
  }

  if (bind((int)fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("nova_net_socket_bind");
    return -1;
  }

  return 0;
}

// Listen for connections
int64_t nova_net_socket_listen(int64_t fd, int64_t backlog) {
  if (fd < 0)
    return -1;
  if (backlog <= 0)
    backlog = 10;

  if (listen((int)fd, (int)backlog) < 0) {
    perror("nova_net_socket_listen");
    return -1;
  }
  return 0;
}

// Accept a connection
// Returns: new socket fd for the connection
int64_t nova_net_socket_accept(int64_t fd) {
  if (fd < 0)
    return -1;

  struct sockaddr_in client_addr;
  socklen_t clilen = sizeof(client_addr);

  int newsockfd = accept((int)fd, (struct sockaddr *)&client_addr, &clilen);
  if (newsockfd < 0) {
    // perror("nova_net_socket_accept");
    return -1;
  }

  return (int64_t)newsockfd;
}

// Connect to remote server
int64_t nova_net_socket_connect(int64_t fd, const char *host, int64_t port) {
  if (fd < 0)
    return -1;

  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons((uint16_t)port);

  if (inet_pton(AF_INET, host, &serv_addr.sin_addr) <= 0) {
    return -2;
  }

  if (connect((int)fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    // perror("nova_net_socket_connect");
    return -1;
  }

  return 0;
}

// Read from socket
int64_t nova_net_socket_read(int64_t fd, char *buffer, int64_t len) {
  if (fd < 0 || buffer == NULL)
    return -1;

  ssize_t n = read((int)fd, buffer, (size_t)len);
  if (n < 0) {
    // perror("nova_net_socket_read");
    return -1;
  }

  buffer[n] = '\0'; // Null terminate safely
  return (int64_t)n;
}

// Write to socket
int64_t nova_net_socket_write(int64_t fd, const char *buffer, int64_t len) {
  if (fd < 0 || buffer == NULL)
    return -1;

  ssize_t n = write((int)fd, buffer, (size_t)len);
  if (n < 0) {
    // perror("nova_net_socket_write");
    return -1;
  }

  return (int64_t)n;
}

// Close socket
void nova_net_socket_close(int64_t fd) {
  if (fd >= 0) {
    close((int)fd);
  }
}
