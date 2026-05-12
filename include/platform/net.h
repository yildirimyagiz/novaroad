/**
 * @file net.h
 * @brief Cross-platform Networking Abstraction
 */

#ifndef NOVA_NET_H
#define NOVA_NET_H

#include "../config/config.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Network address family */
typedef enum {
    NOVA_AF_UNSPEC,
    NOVA_AF_INET,      // IPv4
    NOVA_AF_INET6,     // IPv6
    NOVA_AF_UNIX,      // Unix domain sockets
} nova_address_family_t;

/* Socket type */
typedef enum {
    NOVA_SOCK_STREAM,  // TCP
    NOVA_SOCK_DGRAM,   // UDP
    NOVA_SOCK_RAW,     // Raw sockets
} nova_socket_type_t;

/* Socket address */
typedef struct {
    nova_address_family_t family;
    char ip[46];       // IPv4/IPv6 string
    uint16_t port;
} nova_sockaddr_t;

/* Socket handle */
typedef struct nova_socket nova_socket_t;

/* TCP connection handle */
typedef struct nova_tcp_conn nova_tcp_conn_t;

/* UDP socket handle */
typedef struct nova_udp_socket nova_udp_socket_t;

/* ============================================================================
 * Socket Operations
 * ========================================================================== */

/**
 * Create socket
 */
nova_socket_t *nova_socket_create(nova_address_family_t family, nova_socket_type_t type);

/**
 * Close socket
 */
void nova_socket_close(nova_socket_t *socket);

/**
 * Bind socket to address
 */
int nova_socket_bind(nova_socket_t *socket, const nova_sockaddr_t *addr);

/**
 * Listen for connections
 */
int nova_socket_listen(nova_socket_t *socket, int backlog);

/**
 * Accept connection
 */
nova_tcp_conn_t *nova_socket_accept(nova_socket_t *socket);

/**
 * Connect to remote address
 */
int nova_socket_connect(nova_socket_t *socket, const nova_sockaddr_t *addr);

/**
 * Send data
 */
size_t nova_socket_send(nova_socket_t *socket, const void *data, size_t size);

/**
 * Receive data
 */
size_t nova_socket_recv(nova_socket_t *socket, void *buffer, size_t size);

/* ============================================================================
 * TCP Operations
 * ========================================================================== */

/**
 * Create TCP connection
 */
nova_tcp_conn_t *nova_tcp_connect(const char *host, uint16_t port);

/**
 * Close TCP connection
 */
void nova_tcp_close(nova_tcp_conn_t *conn);

/**
 * Send data over TCP
 */
size_t nova_tcp_send(nova_tcp_conn_t *conn, const void *data, size_t size);

/**
 * Receive data over TCP
 */
size_t nova_tcp_recv(nova_tcp_conn_t *conn, void *buffer, size_t size);

/* ============================================================================
 * UDP Operations
 * ========================================================================== */

/**
 * Create UDP socket
 */
nova_udp_socket_t *nova_udp_create(void);

/**
 * Close UDP socket
 */
void nova_udp_close(nova_udp_socket_t *socket);

/**
 * Send UDP datagram
 */
size_t nova_udp_send_to(nova_udp_socket_t *socket, const void *data, size_t size,
                       const nova_sockaddr_t *addr);

/**
 * Receive UDP datagram
 */
size_t nova_udp_recv_from(nova_udp_socket_t *socket, void *buffer, size_t size,
                         nova_sockaddr_t *addr);

/* ============================================================================
 * Address Operations
 * ========================================================================== */

/**
 * Parse IP address and port
 */
int nova_addr_parse(const char *str, nova_sockaddr_t *addr);

/**
 * Convert address to string
 */
const char *nova_addr_to_string(const nova_sockaddr_t *addr, char *buffer, size_t size);

/**
 * Get local hostname
 */
const char *nova_net_hostname(void);

/**
 * Resolve hostname to IP
 */
int nova_net_resolve(const char *hostname, nova_sockaddr_t *addr);

/* ============================================================================
 * HTTP Client (Basic)
 * ========================================================================== */

typedef struct nova_http_request nova_http_request_t;
typedef struct nova_http_response nova_http_response_t;

/**
 * Create HTTP request
 */
nova_http_request_t *nova_http_request_create(const char *method, const char *url);

/**
 * Destroy HTTP request
 */
void nova_http_request_destroy(nova_http_request_t *req);

/**
 * Set request header
 */
void nova_http_request_set_header(nova_http_request_t *req, const char *key, const char *value);

/**
 * Set request body
 */
void nova_http_request_set_body(nova_http_request_t *req, const void *data, size_t size);

/**
 * Execute HTTP request
 */
nova_http_response_t *nova_http_execute(nova_http_request_t *req);

/**
 * Get response status code
 */
int nova_http_response_status(const nova_http_response_t *resp);

/**
 * Get response headers
 */
const char *nova_http_response_header(const nova_http_response_t *resp, const char *key);

/**
 * Get response body
 */
const void *nova_http_response_body(const nova_http_response_t *resp, size_t *size);

/**
 * Destroy HTTP response
 */
void nova_http_response_destroy(nova_http_response_t *resp);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_NET_H */
