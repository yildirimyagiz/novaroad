/**
 * @file lsp_protocol.h
 * @brief Nova Language Server Protocol Implementation
 * 
 * Implements LSP 3.17 specification for Nova language
 * https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/
 */

#ifndef NOVA_LSP_PROTOCOL_H
#define NOVA_LSP_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// ═══════════════════════════════════════════════════════════════════════════
// LSP Message Types
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
    LSP_METHOD_INITIALIZE,
    LSP_METHOD_INITIALIZED,
    LSP_METHOD_SHUTDOWN,
    LSP_METHOD_EXIT,
    LSP_METHOD_TEXT_DOCUMENT_DID_OPEN,
    LSP_METHOD_TEXT_DOCUMENT_DID_CHANGE,
    LSP_METHOD_TEXT_DOCUMENT_DID_SAVE,
    LSP_METHOD_TEXT_DOCUMENT_DID_CLOSE,
    LSP_METHOD_TEXT_DOCUMENT_COMPLETION,
    LSP_METHOD_TEXT_DOCUMENT_HOVER,
    LSP_METHOD_TEXT_DOCUMENT_SIGNATURE_HELP,
    LSP_METHOD_TEXT_DOCUMENT_DEFINITION,
    LSP_METHOD_TEXT_DOCUMENT_REFERENCES,
    LSP_METHOD_TEXT_DOCUMENT_DOCUMENT_SYMBOL,
    LSP_METHOD_TEXT_DOCUMENT_FORMATTING,
    LSP_METHOD_TEXT_DOCUMENT_RENAME,
    LSP_METHOD_WORKSPACE_SYMBOL,
    LSP_METHOD_UNKNOWN
} lsp_method_t;

typedef struct {
    int line;
    int character;
} lsp_position_t;

typedef struct {
    lsp_position_t start;
    lsp_position_t end;
} lsp_range_t;

typedef struct {
    char *uri;
    int version;
    char *text;
} lsp_text_document_t;

typedef struct {
    char *label;
    char *detail;
    char *documentation;
    int kind; // 1=Text, 2=Method, 3=Function, etc.
} lsp_completion_item_t;

typedef struct {
    char *contents;
    lsp_range_t range;
} lsp_hover_t;

typedef struct {
    char *uri;
    lsp_range_t range;
} lsp_location_t;

typedef enum {
    LSP_DIAG_ERROR = 1,
    LSP_DIAG_WARNING = 2,
    LSP_DIAG_INFO = 3,
    LSP_DIAG_HINT = 4
} lsp_diagnostic_severity_t;

typedef struct {
    lsp_range_t range;
    lsp_diagnostic_severity_t severity;
    char *message;
    char *source;
} lsp_diagnostic_t;

// ═══════════════════════════════════════════════════════════════════════════
// LSP Server State
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
    bool initialized;
    bool shutdown_requested;
    
    // Capabilities
    bool supports_completion;
    bool supports_hover;
    bool supports_goto_definition;
    bool supports_find_references;
    bool supports_formatting;
    bool supports_rename;
    
    // Workspace
    char *workspace_root;
    
    // Statistics
    uint64_t requests_handled;
    uint64_t errors_reported;
} nova_lsp_server_t;

// ═══════════════════════════════════════════════════════════════════════════
// API Functions
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Initialize LSP server
 */
void nova_lsp_init(void);

/**
 * Start LSP server (reads from stdin, writes to stdout)
 */
int nova_lsp_start(void);

/**
 * Process a single LSP message
 */
int nova_lsp_process_message(const char *message);

/**
 * Send LSP response
 */
void nova_lsp_send_response(int id, const char *result);

/**
 * Send LSP notification
 */
void nova_lsp_send_notification(const char *method, const char *params);

/**
 * Send diagnostics for a document
 */
void nova_lsp_send_diagnostics(const char *uri, lsp_diagnostic_t *diagnostics, int count);

// ═══════════════════════════════════════════════════════════════════════════
// Feature Handlers
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Handle initialize request
 */
char* nova_lsp_handle_initialize(const char *params);

/**
 * Handle completion request
 */
lsp_completion_item_t* nova_lsp_handle_completion(const char *uri, lsp_position_t pos, int *count);

/**
 * Handle hover request
 */
lsp_hover_t* nova_lsp_handle_hover(const char *uri, lsp_position_t pos);

/**
 * Handle go-to-definition request
 */
lsp_location_t* nova_lsp_handle_definition(const char *uri, lsp_position_t pos);

/**
 * Handle find references request
 */
lsp_location_t* nova_lsp_handle_references(const char *uri, lsp_position_t pos, int *count);

/**
 * Handle formatting request
 */
char* nova_lsp_handle_formatting(const char *uri);

// ═══════════════════════════════════════════════════════════════════════════
// Utilities
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Parse LSP method from message
 */
lsp_method_t nova_lsp_parse_method(const char *message);

/**
 * Get server instance
 */
nova_lsp_server_t* nova_lsp_get_server(void);

/**
 * Cleanup and shutdown
 */
void nova_lsp_shutdown(void);

#endif // NOVA_LSP_PROTOCOL_H
