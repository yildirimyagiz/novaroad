#include <stdio.h>
#include <string.h>

const char* read_console_input() {
    static char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    printf("\n👉 Enter a Turkish word (or 'q' to quit): ");
    fflush(stdout);
    if (fgets(buffer, sizeof(buffer), stdin)) {
        buffer[strcspn(buffer, "\r\n")] = 0;
        return buffer;
    }
    return "";
}

const char* advance_pointer(const char* ptr, int offset) {
    if (!ptr) return "";
    return ptr + offset;
}
