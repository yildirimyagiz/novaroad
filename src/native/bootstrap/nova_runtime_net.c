// Nova Runtime Networking - Production POSIX Socket Implementation
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

// --- LOW-LEVEL SOCKET APIs (for nova_stage0.py generated code) ---

int64_t nova_net_socket_create(int64_t domain, int64_t type, int64_t protocol) {
    int fd = socket((int)domain, (int)type, (int)protocol);
    // Set SO_REUSEADDR by default for servers
    if (fd >= 0) {
        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    }
    return (int64_t)fd;
}

int64_t nova_net_socket_bind(int64_t fd, const char* host, int64_t port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
#ifdef __APPLE__
    addr.sin_len = sizeof(struct sockaddr_in);
#endif
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (host && strcmp(host, "0.0.0.0") != 0 && strcmp(host, "") != 0) {
        addr.sin_addr.s_addr = inet_addr(host);
    } else {
        addr.sin_addr.s_addr = INADDR_ANY;
    }
    int res = bind((int)fd, (struct sockaddr*)&addr, sizeof(addr));
    if (res < 0) {
        fprintf(stderr, "   ⚠️ socket_bind error: %s (errno=%d, fd=%lld, port=%lld)\n", strerror(errno), errno, (long long)fd, (long long)port);
    }
    return (int64_t)res;
}

int64_t nova_net_socket_listen(int64_t fd, int64_t backlog) {
    return (int64_t)listen((int)fd, (int)backlog);
}

int64_t nova_net_socket_accept(int64_t fd) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int client_fd = accept((int)fd, (struct sockaddr*)&addr, &len);
    return (int64_t)client_fd;
}

int64_t nova_net_socket_connect(int64_t fd, const char* host, int64_t port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
#ifdef __APPLE__
    addr.sin_len = sizeof(struct sockaddr_in);
#endif
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = inet_addr(host);
    return (int64_t)connect((int)fd, (struct sockaddr*)&addr, sizeof(addr));
}

int64_t nova_net_socket_read(int64_t fd, char* buf, int64_t len) {
    if (!buf || len <= 0) return 0;
    return (int64_t)read((int)fd, buf, (size_t)len);
}

int64_t nova_net_socket_write(int64_t fd, const char* buf, int64_t len) {
    if (!buf || len <= 0) return 0;
    return (int64_t)write((int)fd, buf, (size_t)len);
}

void nova_net_socket_close(int64_t fd) {
    close((int)fd);
}

// --- HIGH-LEVEL WRAPPERS (for simple TCP usages) ---

void* nova_tcp_listen(const char* host, int port) {
    int64_t fd = nova_net_socket_create(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return NULL;
    if (nova_net_socket_bind(fd, host, port) < 0) {
        close((int)fd);
        return NULL;
    }
    if (nova_net_socket_listen(fd, 10) < 0) {
        close((int)fd);
        return NULL;
    }
    return (void*)(uintptr_t)fd;
}

void* nova_tcp_accept(void* listener) {
    if (!listener) return NULL;
    int64_t server_fd = (int64_t)(uintptr_t)listener;
    int64_t client_fd = nova_net_socket_accept(server_fd);
    if (client_fd < 0) return NULL;
    return (void*)(uintptr_t)client_fd;
}

void* nova_tcp_connect(const char* host, int port) {
    int64_t fd = nova_net_socket_create(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return NULL;
    if (nova_net_socket_connect(fd, host, port) < 0) {
        close((int)fd);
        return NULL;
    }
    return (void*)(uintptr_t)fd;
}

long nova_tcp_send(void* conn, const char* data, long size) {
    if (!conn) return -1;
    return (long)nova_net_socket_write((int64_t)(uintptr_t)conn, data, size);
}

long nova_tcp_recv(void* conn, char* buffer, long size) {
    if (!conn) return -1;
    return (long)nova_net_socket_read((int64_t)(uintptr_t)conn, buffer, size);
}

void nova_tcp_close(void* conn) {
    if (conn) {
        nova_net_socket_close((int64_t)(uintptr_t)conn);
    }
}

// HTTP client stubs (simple wget-like implementation using sockets if wanted, or basic mock/stubs)
char* nova_http_get(const char* url) {
    (void)url;
    return NULL;
}

char* nova_http_post(const char* url, const char* body) {
    (void)url;
    (void)body;
    return NULL;
}

// --- HIGH-LEVEL HTTP SERVER HELPERS ---

char* nova_net_get_request_path(int64_t client_fd) {
    static char buf[4096];
    memset(buf, 0, sizeof(buf));
    ssize_t n = recv((int)client_fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) return strdup("/");
    
    // Find the first line (e.g., "GET /path?query HTTP/1.1")
    char *method_end = strchr(buf, ' ');
    if (!method_end) return strdup("/");
    char *path_start = method_end + 1;
    char *path_end = strchr(path_start, ' ');
    if (!path_end) return strdup("/");
    
    size_t path_len = path_end - path_start;
    char *path = malloc(path_len + 1);
    memcpy(path, path_start, path_len);
    path[path_len] = '\0';
    return path;
}

char* nova_net_get_query_param(const char* url, const char* param_name) {
    if (!url || !param_name) return strdup("");
    char *query_start = strchr(url, '?');
    if (!query_start) return strdup("");
    query_start++; // Skip '?'
    
    size_t param_len = strlen(param_name);
    char *p = query_start;
    while (p && *p) {
        if (strncmp(p, param_name, param_len) == 0 && p[param_len] == '=') {
            char *val_start = p + param_len + 1;
            char *val_end = strchr(val_start, '&');
            size_t val_len = val_end ? (size_t)(val_end - val_start) : strlen(val_start);
            char *val = malloc(val_len + 1);
            memcpy(val, val_start, val_len);
            val[val_len] = '\0';
            
            // Simple URL decode
            char *dst = val;
            for (char *src = val; *src; src++) {
                if (*src == '+') {
                    *dst++ = ' ';
                } else if (*src == '%' && src[1] && src[2]) {
                    char hex[3] = {src[1], src[2], '\0'};
                    *dst++ = (char)strtol(hex, NULL, 16);
                    src += 2;
                } else {
                    *dst++ = *src;
                }
            }
            *dst = '\0';
            
            return val;
        }
        p = strchr(p, '&');
        if (p) p++; // Skip '&'
    }
    return strdup("");
}

void nova_net_send_http_response(int64_t client_fd, int64_t status_code, const char* content_type, const char* body) {
    if (!body) body = "";
    size_t body_len = strlen(body);
    
    char headers[2048];
    int header_len = snprintf(headers, sizeof(headers),
        "HTTP/1.1 %lld OK\r\n"
        "Server: NovaSovereign/2.0\r\n"
        "Content-Type: %s; charset=utf-8\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n"
        "\r\n",
        (long long)status_code, content_type, body_len);
        
    write((int)client_fd, headers, (size_t)header_len);
    write((int)client_fd, body, body_len);
}
