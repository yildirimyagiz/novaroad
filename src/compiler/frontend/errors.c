/**
 * @file errors.c
 * @brief Error reporting
 */

#include <stdio.h>
#include <stdarg.h>

void nova_error(const char *file, int line, const char *fmt, ...)
{
    fprintf(stderr, "\033[1;31merror:\033[0m %s:%d: ", file, line);
    
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    
    fprintf(stderr, "\n");
}

void nova_warning(const char *file, int line, const char *fmt, ...)
{
    fprintf(stderr, "\033[1;33mwarning:\033[0m %s:%d: ", file, line);
    
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    
    fprintf(stderr, "\n");
}

void nova_note(const char *fmt, ...)
{
    fprintf(stderr, "\033[1;36mnote:\033[0m ");
    
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    
    fprintf(stderr, "\n");
}
