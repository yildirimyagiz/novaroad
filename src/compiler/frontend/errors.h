/**
 * @file errors.h
 * @brief Compiler error reporting
 */

#ifndef NOVA_COMPILER_FRONTEND_ERRORS_H
#define NOVA_COMPILER_FRONTEND_ERRORS_H

/* Emit a coloured error/warning/note to stderr.
 * file / line refer to the Nova *source* location, not the C file. */
void nova_error  (const char *file, int line, const char *fmt, ...);
void nova_warning(const char *file, int line, const char *fmt, ...);
void nova_note   (const char *fmt, ...);

#endif /* NOVA_COMPILER_FRONTEND_ERRORS_H */
