/*
 * Nova Runtime — CLI Support Functions
 * FFI bindings for stdlib/cli.zn
 *
 * Provides:
 *   - Command-line argument access (argc/argv)
 *   - Environment variable access
 *   - Terminal capability detection
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

/* ═══════════════════════════════════════════════════════════════════════════════
 * §1  GLOBAL ARGC/ARGV STORAGE
 *
 * These are set by nova_runtime_init() at program startup.
 * ═══════════════════════════════════════════════════════════════════════════════
 */

static int g_argc = 0;
static char **g_argv = NULL;

void nova_runtime_cli_init(int argc, char **argv) {
  g_argc = argc;
  g_argv = argv;
}

/* ═══════════════════════════════════════════════════════════════════════════════
 * §2  ARGC / ARGV ACCESS
 * ═══════════════════════════════════════════════════════════════════════════════
 */

int64_t nova_runtime_get_argc(void) { return (int64_t)g_argc; }

/* Returns a pointer to the argv[index] string.
 * The caller must NOT free this — it is the original argv pointer. */
const char *nova_runtime_get_argv(int64_t index) {
  if (index < 0 || index >= (int64_t)g_argc || !g_argv) {
    return "";
  }
  return g_argv[index];
}

/* ═══════════════════════════════════════════════════════════════════════════════
 * §3  ENVIRONMENT VARIABLES
 * ═══════════════════════════════════════════════════════════════════════════════
 */

const char *nova_runtime_getenv(const char *name, int64_t len) {
  if (!name || len <= 0)
    return NULL;

  /* Create null-terminated copy */
  char *key = strndup(name, (size_t)len);
  if (!key)
    return NULL;

  const char *value = getenv(key);
  free(key);

  return value; /* NULL if not found, otherwise points to env storage */
}

int64_t nova_runtime_setenv(const char *name, int64_t name_len,
                            const char *value, int64_t value_len) {
  if (!name || name_len <= 0)
    return -1;

  char *key = strndup(name, (size_t)name_len);
  char *val = strndup(value, (size_t)value_len);
  if (!key || !val) {
    free(key);
    free(val);
    return -1;
  }

#ifdef _WIN32
  int result = _putenv_s(key, val) == 0 ? 0 : -1;
#else
  int result = setenv(key, val, 1);
#endif

  free(key);
  free(val);
  return (int64_t)result;
}

/* ═══════════════════════════════════════════════════════════════════════════════
 * §4  TERMINAL CAPABILITIES
 * ═══════════════════════════════════════════════════════════════════════════════
 */

/* Returns 1 if stdout is a TTY (supports colors, cursor movement) */
int64_t nova_runtime_is_tty(void) {
#ifdef _WIN32
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode;
  return GetConsoleMode(hOut, &mode) ? 1 : 0;
#else
  return isatty(fileno(stdout)) ? 1 : 0;
#endif
}

/* Get terminal width in columns (default 80 if unknown) */
int64_t nova_runtime_term_width(void) {
#ifdef _WIN32
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
    return (int64_t)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
  }
  return 80;
#else
  struct winsize w;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
    return (int64_t)w.ws_col;
  }
  /* Try COLUMNS env var */
  const char *cols = getenv("COLUMNS");
  if (cols)
    return (int64_t)atoi(cols);
  return 80;
#endif
}

/* Get terminal height in rows (default 24 if unknown) */
int64_t nova_runtime_term_height(void) {
#ifdef _WIN32
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
    return (int64_t)(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
  }
  return 24;
#else
  struct winsize w;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_row > 0) {
    return (int64_t)w.ws_row;
  }
  const char *lines = getenv("LINES");
  if (lines)
    return (int64_t)atoi(lines);
  return 24;
#endif
}

/* ═══════════════════════════════════════════════════════════════════════════════
 * §5  PROCESS CONTROL
 * ═══════════════════════════════════════════════════════════════════════════════
 */

void nova_runtime_exit(int64_t code) { exit((int)code); }

int64_t nova_runtime_getpid(void) {
#ifdef _WIN32
  return (int64_t)GetCurrentProcessId();
#else
  return (int64_t)getpid();
#endif
}

/* ═══════════════════════════════════════════════════════════════════════════════
 * §6  CURSOR / ANSI CONTROL (for progress bars, spinners)
 * ═══════════════════════════════════════════════════════════════════════════════
 */

void nova_runtime_cursor_hide(void) {
  fprintf(stdout, "\033[?25l");
  fflush(stdout);
}

void nova_runtime_cursor_show(void) {
  fprintf(stdout, "\033[?25h");
  fflush(stdout);
}

void nova_runtime_cursor_move_up(int64_t lines) {
  fprintf(stdout, "\033[%lldA", (long long)lines);
  fflush(stdout);
}

void nova_runtime_cursor_move_to_col(int64_t col) {
  fprintf(stdout, "\033[%lldG", (long long)col);
  fflush(stdout);
}

void nova_runtime_clear_line(void) {
  fprintf(stdout, "\033[2K\r");
  fflush(stdout);
}
