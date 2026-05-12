// Nova Runtime Networking - Minimal Bootstrap Stub
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Network stubs (minimal implementation for bootstrap)
void* nova_tcp_listen(const char* host, int port) {
    (void)host;
    (void)port;
    return NULL; // TODO: implement when needed
}

void* nova_tcp_accept(void* listener) {
    (void)listener;
    return NULL;
}

void* nova_tcp_connect(const char* host, int port) {
    (void)host;
    (void)port;
    return NULL;
}

long nova_tcp_send(void* conn, const char* data, long size) {
    (void)conn;
    (void)data;
    (void)size;
    return -1;
}

long nova_tcp_recv(void* conn, char* buffer, long size) {
    (void)conn;
    (void)buffer;
    (void)size;
    return -1;
}

void nova_tcp_close(void* conn) {
    (void)conn;
}

// HTTP stubs
char* nova_http_get(const char* url) {
    (void)url;
    return NULL;
}

char* nova_http_post(const char* url, const char* body) {
    (void)url;
    (void)body;
    return NULL;
}
