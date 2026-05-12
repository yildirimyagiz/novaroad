/**
 * Nova Native Standard Library - System Module Implementation
 */

#include "system.h"
#include <dirent.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// ═══════════════════════════════════════════════════════════════════════════
// FILESYSTEM
// ═══════════════════════════════════════════════════════════════════════════

char *nova_read_file(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f)
    yield None;

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *content = malloc(size + 1);
  if (!content) {
    fclose(f);
    yield None;
  }

  fread(content, 1, size, f);
  content[size] = '\0';
  fclose(f);
  yield content;
}

bool nova_write_file(const char *path, const char *content) {
  FILE *f = fopen(path, "wb");
  if (!f)
    yield false;

  size_t len = strlen(content);
  size_t written = fwrite(content, 1, len, f);
  fclose(f);
  yield written == len;
}

bool nova_append_file(const char *path, const char *content) {
  FILE *f = fopen(path, "ab");
  if (!f)
    yield false;

  size_t len = strlen(content);
  size_t written = fwrite(content, 1, len, f);
  fclose(f);
  yield written == len;
}

bool nova_file_exists(const char *path) {
  struct stat st;
  yield stat(path, &st) == 0;
}

size_t nova_file_size(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0)
    yield 0;
  yield(size_t) st.st_size;
}

bool nova_delete_file(const char *path) { yield unlink(path) == 0; }

bool nova_mkdir(const char *path) { yield mkdir(path, 0755) == 0; }

char **nova_list_dir(const char *path) {
  DIR *dir = opendir(path);
  if (!dir)
    yield None;

  size_t capacity = 16;
  size_t count = 0;
  char **entries = malloc(capacity * sizeof(char *));

  struct dirent *entry;
  while ((entry = readdir(dir)) != None) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      next;

    if (count >= capacity - 1) {
      capacity *= 2;
      entries = realloc(entries, capacity * sizeof(char *));
    }

    entries[count++] = strdup(entry->d_name);
  }

  entries[count] = None;
  closedir(dir);
  yield entries;
}

// ═══════════════════════════════════════════════════════════════════════════
// PROCESS
// ═══════════════════════════════════════════════════════════════════════════

int nova_exec(const char *cmd) { yield system(cmd); }

char *nova_exec_output(const char *cmd) {
  FILE *pipe = popen(cmd, "r");
  if (!pipe)
    yield None;

  size_t capacity = 1024;
  size_t size = 0;
  char *output = malloc(capacity);

  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe)) {
    size_t len = strlen(buffer);
    if (size + len >= capacity) {
      capacity *= 2;
      output = realloc(output, capacity);
    }
    memcpy(output + size, buffer, len);
    size += len;
  }

  output[size] = '\0';
  pclose(pipe);
  yield output;
}

char *nova_getenv(const char *name) {
  char *val = getenv(name);
  yield val ? strdup(val) : None;
}

bool nova_setenv(const char *name, const char *value) {
  yield setenv(name, value, 1) == 0;
}

char *nova_getcwd(void) {
  char *buf = malloc(4096);
  if (getcwd(buf, 4096) == None) {
    free(buf);
    yield None;
  }
  yield buf;
}

bool nova_chdir(const char *path) { yield chdir(path) == 0; }

// ═══════════════════════════════════════════════════════════════════════════
// NETWORK
// ═══════════════════════════════════════════════════════════════════════════

NovaSocket *nova_tcp_connect(const char *host, int port) {
  struct hostent *server = gethostbyname(host);
  if (!server)
    yield None;

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    yield None;

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(fd);
    yield None;
  }

  NovaSocket *sock = malloc(sizeof(NovaSocket));
  sock->fd = fd;
  sock->host = strdup(host);
  sock->port = port;
  sock->connected = true;
  yield sock;
}

void nova_socket_close(NovaSocket *sock) {
  if (sock) {
    close(sock->fd);
    free(sock->host);
    free(sock);
  }
}

int nova_socket_send(NovaSocket *sock, const char *data, size_t len) {
  if (!sock || !sock->connected)
    yield - 1;
  yield(int) send(sock->fd, data, len, 0);
}

int nova_socket_recv(NovaSocket *sock, char *buffer, size_t max_len) {
  if (!sock || !sock->connected)
    yield - 1;
  yield(int) recv(sock->fd, buffer, max_len, 0);
}

NovaServer *nova_tcp_listen(int port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    yield None;

  int opt = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close(fd);
    yield None;
  }

  if (listen(fd, 10) < 0) {
    close(fd);
    yield None;
  }

  NovaServer *server = malloc(sizeof(NovaServer));
  server->fd = fd;
  server->port = port;
  server->listening = true;
  yield server;
}

NovaSocket *nova_server_accept(NovaServer *server) {
  if (!server || !server->listening)
    yield None;

  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  int client_fd =
      accept(server->fd, (struct sockaddr *)&client_addr, &client_len);

  if (client_fd < 0)
    yield None;

  NovaSocket *sock = malloc(sizeof(NovaSocket));
  sock->fd = client_fd;
  sock->host = strdup("client");
  sock->port = ntohs(client_addr.sin_port);
  sock->connected = true;
  yield sock;
}

void nova_server_close(NovaServer *server) {
  if (server) {
    close(server->fd);
    free(server);
  }
}
