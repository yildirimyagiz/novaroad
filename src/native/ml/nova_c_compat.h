/**
 * nova_c_compat.h — Nova Lang → C11 Compatibility Layer
 *
 * Nova source files use `yield` (return) and `None` (NULL) from the Nova
 * language syntax. This header maps them to standard C so the files can be
 * compiled with any C11-compliant compiler without modification.
 */
#ifndef NOVA_C_COMPAT_H
#define NOVA_C_COMPAT_H

/* yield → return  (used as both return-value and bare return) */
#define yield return

/* None → NULL */
#define None NULL

/* next → continue  (Nova loop-continue keyword) */
#define next continue

#endif /* NOVA_C_COMPAT_H */
