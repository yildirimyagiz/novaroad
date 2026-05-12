/**
 * @file net.h
 * @brief Network I/O (TCP, UDP, async networking)
 *
 * Nova networking provides:
 * - TCP and UDP sockets
 * - IPv4 and IPv6 support
 * - Async networking with event loop integration
 * - DNS resolution
 * - TLS/SSL support (optional)
 * - HTTP client (basic)
 */

#ifndef NOVA_NET_H
#define NOVA_NET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * Socket Types and Constants
 * ======================================================================== */

typedef struct nova_socket nova_socket_t;
typedef struct nova_addr nova_addr_t;

/* Socket types */
typedef enum {
  NOVA_SOCK_STREAM, /**< TCP stream socket */
  NOVA_SOCK_DGRAM,  /**< UDP datagram socket */
  NOVA_SOCK_RAW     /**< Raw socket */
} nova_socket_type_t;

/* Address families */
typedef enum {
  NOVA_AF_INET,  /**< IPv4 */
  NOVA_AF_INET6, /**< IPv6 */
  NOVA_AF_UNIX   /**< Unix domain sockets */
} nova_addr_family_t;

/* Socket options */
#define NOVA_SO_REUSEADDR (1 << 0) /**< Reuse address */
#define NOVA_SO_KEEPALIVE (1 << 1) /**< Keep connection alive */
#define NOVA_SO_NODELAY (1 << 2)   /**< Disable Nagle's algorithm */
#define NOVA_SO_NONBLOCK (1 << 3)  /**< Non-blocking mode */

/* Shutdown modes */
typedef enum {
  NOVA_SHUT_RD,  /**< Shutdown read */
  NOVA_SHUT_WR,  /**< Shutdown write */
  NOVA_SHUT_RDWR /**< Shutdown both */
} nova_shutdown_t;

/* ========================================================================
 * Address Handling
 * ======================================================================== */

/**
 * @brief Create IPv4 address
 * @param ip IP address string (e.g., "127.0.0.1")
 * @param port Port number
 * @return Address structure, or NULL on failure
 */
nova_addr_t *nova_addr_ipv4(const char *ip, uint16_t port);

/**
 * @brief Create IPv6 address
 * @param ip IP address string (e.g., "::1")
 * @param port Port number
 * @return Address structure, or NULL on failure
 */
nova_addr_t *nova_addr_ipv6(const char *ip, uint16_t port);

/**
 * @brief Parse address from string
 * @param addr Address string (e.g., "127.0.0.1:8080", "[::1]:8080")
 * @return Address structure, or NULL on failure
 */
nova_addr_t *nova_addr_parse(const char *addr);

/**
 * @brief Convert address to string
 * @param addr Address structure
 * @param buffer Output buffer
 * @param size Buffer size
 * @return Number of bytes written
 */
int nova_addr_to_string(const nova_addr_t *addr, char *buffer, size_t size);

/**
 * @brief Get port from address
 * @param addr Address structure
 * @return Port number
 */
uint16_t nova_addr_port(const nova_addr_t *addr);

/**
 * @brief Destroy address
 * @param addr Address structure
 */
void nova_addr_destroy(nova_addr_t *addr);

/* ========================================================================
 * Socket Creation
 * ======================================================================== */

/**
 * @brief Create TCP socket
 * @return Socket handle, or NULL on failure
 */
nova_socket_t *nova_tcp_socket(void);

/**
 * @brief Create UDP socket
 * @return Socket handle, or NULL on failure
 */
nova_socket_t *nova_udp_socket(void);

/**
 * @brief Create socket with specific type and family
 * @param type Socket type
 * @param family Address family
 * @return Socket handle, or NULL on failure
 */
nova_socket_t *nova_socket_create(nova_socket_type_t type,
                                  nova_addr_family_t family);

/* ========================================================================
 * Socket Options
 * ======================================================================== */

/**
 * @brief Set socket options
 * @param sock Socket handle
 * @param options Bitwise OR of NOVA_SO_* flags
 * @return 0 on success, -1 on failure
 */
int nova_socket_set_options(nova_socket_t *sock, int options);

/**
 * @brief Set receive timeout
 * @param sock Socket handle
 * @param timeout_ms Timeout in milliseconds (0 for no timeout)
 * @return 0 on success, -1 on failure
 */
int nova_socket_set_recv_timeout(nova_socket_t *sock, uint32_t timeout_ms);

/**
 * @brief Set send timeout
 * @param sock Socket handle
 * @param timeout_ms Timeout in milliseconds (0 for no timeout)
 * @return 0 on success, -1 on failure
 */
int nova_socket_set_send_timeout(nova_socket_t *sock, uint32_t timeout_ms);

/**
 * @brief Set send buffer size
 * @param sock Socket handle
 * @param size Buffer size in bytes
 * @return 0 on success, -1 on failure
 */
int nova_socket_set_send_buffer(nova_socket_t *sock, size_t size);

/**
 * @brief Set receive buffer size
 * @param sock Socket handle
 * @param size Buffer size in bytes
 * @return 0 on success, -1 on failure
 */
int nova_socket_set_recv_buffer(nova_socket_t *sock, size_t size);

/* ========================================================================
 * TCP Server Operations
 * ======================================================================== */

/**
 * @brief Bind socket to address
 * @param sock Socket handle
 * @param addr Address string (IP:port)
 * @param port Port number (if addr is just IP)
 * @return 0 on success, -1 on failure
 */
int nova_socket_bind(nova_socket_t *sock, const char *addr, uint16_t port);

/**
 * @brief Bind socket to address structure
 * @param sock Socket handle
 * @param addr Address structure
 * @return 0 on success, -1 on failure
 */
int nova_socket_bind_addr(nova_socket_t *sock, const nova_addr_t *addr);

/**
 * @brief Listen for incoming connections
 * @param sock Socket handle
 * @param backlog Maximum pending connections
 * @return 0 on success, -1 on failure
 */
int nova_socket_listen(nova_socket_t *sock, int backlog);

/**
 * @brief Accept incoming connection
 * @param sock Listening socket
 * @return New socket for the connection, or NULL on failure
 */
nova_socket_t *nova_socket_accept(nova_socket_t *sock);

/**
 * @brief Accept with timeout
 * @param sock Listening socket
 * @param timeout_ms Timeout in milliseconds
 * @return New socket, or NULL on timeout/failure
 */
nova_socket_t *nova_socket_accept_timeout(nova_socket_t *sock,
                                          uint32_t timeout_ms);

/* ========================================================================
 * TCP Client Operations
 * ======================================================================== */

/**
 * @brief Connect to remote host
 * @param sock Socket handle
 * @param addr Address string (e.g., "example.com:80")
 * @param port Port number (if addr is just hostname)
 * @return 0 on success, -1 on failure
 */
int nova_socket_connect(nova_socket_t *sock, const char *addr, uint16_t port);

/**
 * @brief Connect to address structure
 * @param sock Socket handle
 * @param addr Address structure
 * @return 0 on success, -1 on failure
 */
int nova_socket_connect_addr(nova_socket_t *sock, const nova_addr_t *addr);

/**
 * @brief Connect with timeout
 * @param sock Socket handle
 * @param addr Address string
 * @param port Port number
 * @param timeout_ms Timeout in milliseconds
 * @return 0 on success, -1 on failure/timeout
 */
int nova_socket_connect_timeout(nova_socket_t *sock, const char *addr,
                                uint16_t port, uint32_t timeout_ms);

/* ========================================================================
 * Data Transfer (TCP)
 * ======================================================================== */

/**
 * @brief Send data
 * @param sock Socket handle
 * @param data Data to send
 * @param len Length in bytes
 * @return Bytes sent, or -1 on failure
 */
size_t nova_socket_send(nova_socket_t *sock, const void *data, size_t len);

/**
 * @brief Send all data (blocks until all sent)
 * @param sock Socket handle
 * @param data Data to send
 * @param len Length in bytes
 * @return 0 on success, -1 on failure
 */
int nova_socket_send_all(nova_socket_t *sock, const void *data, size_t len);

/**
 * @brief Receive data
 * @param sock Socket handle
 * @param buffer Output buffer
 * @param len Buffer size
 * @return Bytes received, 0 on EOF, -1 on failure
 */
size_t nova_socket_recv(nova_socket_t *sock, void *buffer, size_t len);

/**
 * @brief Receive exact amount of data
 * @param sock Socket handle
 * @param buffer Output buffer
 * @param len Bytes to receive
 * @return 0 on success, -1 on failure/EOF
 */
int nova_socket_recv_exact(nova_socket_t *sock, void *buffer, size_t len);

/**
 * @brief Peek at data without removing from queue
 * @param sock Socket handle
 * @param buffer Output buffer
 * @param len Buffer size
 * @return Bytes peeked, or -1 on failure
 */
size_t nova_socket_peek(nova_socket_t *sock, void *buffer, size_t len);

/* ========================================================================
 * Data Transfer (UDP)
 * ======================================================================== */

/**
 * @brief Send datagram
 * @param sock Socket handle
 * @param data Data to send
 * @param len Length in bytes
 * @param addr Destination address
 * @return Bytes sent, or -1 on failure
 */
size_t nova_socket_sendto(nova_socket_t *sock, const void *data, size_t len,
                          const nova_addr_t *addr);

/**
 * @brief Receive datagram
 * @param sock Socket handle
 * @param buffer Output buffer
 * @param len Buffer size
 * @param addr Output: sender address (optional, can be NULL)
 * @return Bytes received, or -1 on failure
 */
size_t nova_socket_recvfrom(nova_socket_t *sock, void *buffer, size_t len,
                            nova_addr_t **addr);

/* ========================================================================
 * Socket Control
 * ======================================================================== */

/**
 * @brief Shutdown socket (stop sending/receiving)
 * @param sock Socket handle
 * @param how Shutdown mode
 * @return 0 on success, -1 on failure
 */
int nova_socket_shutdown(nova_socket_t *sock, nova_shutdown_t how);

/**
 * @brief Close socket
 * @param sock Socket handle
 */
void nova_socket_close(nova_socket_t *sock);

/**
 * @brief Get local address
 * @param sock Socket handle
 * @return Local address, or NULL on failure
 */
nova_addr_t *nova_socket_local_addr(nova_socket_t *sock);

/**
 * @brief Get remote address
 * @param sock Socket handle
 * @return Remote address, or NULL on failure
 */
nova_addr_t *nova_socket_remote_addr(nova_socket_t *sock);

/**
 * @brief Check if socket is connected
 * @param sock Socket handle
 * @return true if connected
 */
bool nova_socket_is_connected(nova_socket_t *sock);

/* ========================================================================
 * DNS Resolution
 * ======================================================================== */

/**
 * @brief Resolve hostname to IP addresses
 * @param hostname Hostname to resolve
 * @param count Output: number of addresses
 * @return Array of addresses (caller must free), or NULL on failure
 */
nova_addr_t **nova_dns_resolve(const char *hostname, size_t *count);

/**
 * @brief Reverse DNS lookup
 * @param addr IP address
 * @return Hostname (caller must free), or NULL on failure
 */
char *nova_dns_reverse(const nova_addr_t *addr);

/* ========================================================================
 * Async Networking (Event-driven)
 * ======================================================================== */

typedef struct nova_net_poller nova_net_poller_t;
typedef void (*nova_net_callback_t)(nova_socket_t *sock, void *userdata);

/**
 * @brief Create network poller (event loop for sockets)
 * @return Poller handle, or NULL on failure
 */
nova_net_poller_t *nova_net_poller_create(void);

/**
 * @brief Add socket to poller
 * @param poller Poller handle
 * @param sock Socket to monitor
 * @param readable Callback for read events
 * @param writable Callback for write events
 * @param userdata User data passed to callbacks
 * @return 0 on success, -1 on failure
 */
int nova_net_poller_add(nova_net_poller_t *poller, nova_socket_t *sock,
                        nova_net_callback_t readable,
                        nova_net_callback_t writable, void *userdata);

/**
 * @brief Remove socket from poller
 * @param poller Poller handle
 * @param sock Socket to remove
 * @return 0 on success, -1 on failure
 */
int nova_net_poller_remove(nova_net_poller_t *poller, nova_socket_t *sock);

/**
 * @brief Poll for events
 * @param poller Poller handle
 * @param timeout_ms Timeout in milliseconds (-1 for infinite)
 * @return Number of events, or -1 on failure
 */
int nova_net_poller_poll(nova_net_poller_t *poller, int timeout_ms);

/**
 * @brief Destroy poller
 * @param poller Poller handle
 */
void nova_net_poller_destroy(nova_net_poller_t *poller);

/* ========================================================================
 * High-level Helpers
 * ======================================================================== */

/**
 * @brief Create TCP server (bind + listen)
 * @param addr Address to bind to
 * @param port Port number
 * @param backlog Listen backlog
 * @return Server socket, or NULL on failure
 */
nova_socket_t *nova_tcp_server(const char *addr, uint16_t port, int backlog);

/**
 * @brief Create TCP client (connect)
 * @param addr Remote address
 * @param port Remote port
 * @return Connected socket, or NULL on failure
 */
nova_socket_t *nova_tcp_client(const char *addr, uint16_t port);

/**
 * @brief Send string over TCP
 * @param sock Socket handle
 * @param str String to send
 * @return 0 on success, -1 on failure
 */
int nova_tcp_send_string(nova_socket_t *sock, const char *str);

/**
 * @brief Receive line from TCP socket
 * @param sock Socket handle
 * @param buffer Output buffer
 * @param size Buffer size
 * @return Number of bytes read, or -1 on failure
 */
size_t nova_tcp_recv_line(nova_socket_t *sock, char *buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_NET_H */
