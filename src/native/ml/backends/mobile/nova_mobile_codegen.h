/**
 * nova_mobile_codegen.h - iOS & Android Code Generator
 *
 * Nova AST → Swift/SwiftUI (iOS) veya Kotlin/Compose (Android)
 * Native mobile app generation from Nova source
 */
#ifndef NOVA_MOBILE_CODEGEN_H
#define NOVA_MOBILE_CODEGEN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  NOVA_MOBILE_IOS = 0,
  NOVA_MOBILE_ANDROID,
  NOVA_MOBILE_BOTH
} NovaMobilePlatform;

typedef enum {
  NOVA_UI_SWIFTUI = 0,
  NOVA_UI_UIKIT,
  NOVA_UI_COMPOSE,
  NOVA_UI_XML_LAYOUT
} NovaUIFramework;

typedef struct {
  char app_name[128];
  char bundle_id[256];      // e.g., "com.nova.myapp"
  char package_name[256];   // Android package
  char min_ios_version[16]; // e.g., "16.0"
  int min_android_sdk;      // e.g., 26
  int target_android_sdk;   // e.g., 34
  NovaMobilePlatform platform;
  NovaUIFramework ui_framework;
  char output_dir[512];
  bool generate_tests;
  bool use_core_data; // iOS
  bool use_room;      // Android
  bool use_networking;
  bool generate_ci; // CI/CD configs
} NovaMobileConfig;

typedef struct {
  char *code;
  size_t code_len;
  char filename[256];
  char language[16]; // "swift", "kotlin", "xml"
} NovaGeneratedFile;

typedef struct {
  NovaGeneratedFile *files;
  int file_count;
  int capacity;
  char project_dir[512];
} NovaMobileProject;

// Project lifecycle
NovaMobileProject *
nova_mobile_create_project(const NovaMobileConfig *cfg);
void nova_mobile_destroy_project(NovaMobileProject *proj);

// iOS generation
int nova_mobile_gen_swift_model(NovaMobileProject *proj, const char *name,
                                  const char *fields);
int nova_mobile_gen_swiftui_view(NovaMobileProject *proj, const char *name,
                                   const char *body_spec);
int nova_mobile_gen_ios_app_delegate(NovaMobileProject *proj);
int nova_mobile_gen_xcode_project(NovaMobileProject *proj,
                                    const NovaMobileConfig *cfg);
int nova_mobile_gen_ios_info_plist(NovaMobileProject *proj,
                                     const NovaMobileConfig *cfg);

// Android generation
int nova_mobile_gen_kotlin_model(NovaMobileProject *proj, const char *name,
                                   const char *fields);
int nova_mobile_gen_compose_screen(NovaMobileProject *proj,
                                     const char *name, const char *body_spec);
int nova_mobile_gen_android_activity(NovaMobileProject *proj);
int nova_mobile_gen_android_manifest(NovaMobileProject *proj,
                                       const NovaMobileConfig *cfg);
int nova_mobile_gen_gradle_build(NovaMobileProject *proj,
                                   const NovaMobileConfig *cfg);

// Shared/Cross-platform
int nova_mobile_gen_network_layer(NovaMobileProject *proj,
                                    NovaMobilePlatform platform);
int nova_mobile_gen_storage_layer(NovaMobileProject *proj,
                                    NovaMobilePlatform platform);

// Write all generated files to disk
int nova_mobile_write_project(const NovaMobileProject *proj);

// Build commands
int nova_mobile_build_ios(const NovaMobileProject *proj);
int nova_mobile_build_android(const NovaMobileProject *proj);

#endif // NOVA_MOBILE_CODEGEN_H
