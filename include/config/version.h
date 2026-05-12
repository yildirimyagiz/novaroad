/**
 * @file version.h
 * @brief Nova version information
 */

#ifndef NOVA_VERSION_H
#define NOVA_VERSION_H

#define NOVA_VERSION_MAJOR 0
#define NOVA_VERSION_MINOR 1
#define NOVA_VERSION_PATCH 0

#define NOVA_VERSION_STRING "0.1.0"

#define NOVA_VERSION_NUM (NOVA_VERSION_MAJOR * 10000 + \
                          NOVA_VERSION_MINOR * 100 + \
                          NOVA_VERSION_PATCH)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get Nova version string
 * @return Version string (e.g., "0.1.0")
 */
const char *nova_version(void);

/**
 * Get Nova version number
 * @return Version as integer (e.g., 100 for 0.1.0)
 */
int nova_version_num(void);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_VERSION_H */
