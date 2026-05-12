/**
 * @file lsp_protocol.c
 * @brief Nova Language Server Protocol Implementation
 */

#include "lsp_protocol.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ═══════════════════════════════════════════════════════════════════════════
// Global Server State
// ═══════════════════════════════════════════════════════════════════════════

static nova_lsp_server_t g_server = {0};

// ═══════════════════════════════════════════════════════════════════════════
// Simple JSON Utilities
// ═══════════════════════════════════════════════════════════════════════════

static char* find_json_value(const char *json, const char *key)
{
    char search[256];
    snprintf(search, sizeof(search), "\"%s\":", key);
    
    const char *pos = strstr(json, search);
    if (!pos) return NULL;
    
    pos += strlen(search);
    while (*pos && isspace(*pos)) pos++;
    
    if (*pos == '"') {
        pos++;
        const char *end = strchr(pos, '"');
        if (!end) return NULL;
        
        size_t len = end - pos;
        char *result = malloc(len + 1);
        memcpy(result, pos, len);
        result[len] = '\0';
        return result;
    }
    
    return NULL;
}

static int find_json_int(const char *json, const char *key)
{
    char search[256];
    snprintf(search, sizeof(search), "\"%s\":", key);
    
    const char *pos = strstr(json, search);
    if (!pos) return -1;
    
    pos += strlen(search);
    while (*pos && isspace(*pos)) pos++;
    
    return atoi(pos);
}

// ═══════════════════════════════════════════════════════════════════════════
// Initialization
// ═══════════════════════════════════════════════════════════════════════════

void nova_lsp_init(void)
{
    memset(&g_server, 0, sizeof(g_server));
    
    // Enable all capabilities
    g_server.supports_completion = true;
    g_server.supports_hover = true;
    g_server.supports_goto_definition = true;
    g_server.supports_find_references = true;
    g_server.supports_formatting = true;
    g_server.supports_rename = true;
    
    fprintf(stderr, "[Nova LSP] Initialized\n");
}

nova_lsp_server_t* nova_lsp_get_server(void)
{
    return &g_server;
}

// ═══════════════════════════════════════════════════════════════════════════
// Message Parsing
// ═══════════════════════════════════════════════════════════════════════════

lsp_method_t nova_lsp_parse_method(const char *message)
{
    char *method = find_json_value(message, "method");
    if (!method) return LSP_METHOD_UNKNOWN;
    
    lsp_method_t result = LSP_METHOD_UNKNOWN;
    
    if (strcmp(method, "initialize") == 0) {
        result = LSP_METHOD_INITIALIZE;
    } else if (strcmp(method, "initialized") == 0) {
        result = LSP_METHOD_INITIALIZED;
    } else if (strcmp(method, "shutdown") == 0) {
        result = LSP_METHOD_SHUTDOWN;
    } else if (strcmp(method, "exit") == 0) {
        result = LSP_METHOD_EXIT;
    } else if (strcmp(method, "textDocument/didOpen") == 0) {
        result = LSP_METHOD_TEXT_DOCUMENT_DID_OPEN;
    } else if (strcmp(method, "textDocument/didChange") == 0) {
        result = LSP_METHOD_TEXT_DOCUMENT_DID_CHANGE;
    } else if (strcmp(method, "textDocument/didSave") == 0) {
        result = LSP_METHOD_TEXT_DOCUMENT_DID_SAVE;
    } else if (strcmp(method, "textDocument/didClose") == 0) {
        result = LSP_METHOD_TEXT_DOCUMENT_DID_CLOSE;
    } else if (strcmp(method, "textDocument/completion") == 0) {
        result = LSP_METHOD_TEXT_DOCUMENT_COMPLETION;
    } else if (strcmp(method, "textDocument/hover") == 0) {
        result = LSP_METHOD_TEXT_DOCUMENT_HOVER;
    } else if (strcmp(method, "textDocument/definition") == 0) {
        result = LSP_METHOD_TEXT_DOCUMENT_DEFINITION;
    } else if (strcmp(method, "textDocument/references") == 0) {
        result = LSP_METHOD_TEXT_DOCUMENT_REFERENCES;
    } else if (strcmp(method, "textDocument/formatting") == 0) {
        result = LSP_METHOD_TEXT_DOCUMENT_FORMATTING;
    } else if (strcmp(method, "textDocument/rename") == 0) {
        result = LSP_METHOD_TEXT_DOCUMENT_RENAME;
    }
    
    free(method);
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// Message Sending
// ═══════════════════════════════════════════════════════════════════════════

void nova_lsp_send_response(int id, const char *result)
{
    char response[8192];
    snprintf(response, sizeof(response),
             "{\"jsonrpc\":\"2.0\",\"id\":%d,\"result\":%s}\r\n",
             id, result);
    
    printf("Content-Length: %zu\r\n\r\n%s", strlen(response), response);
    fflush(stdout);
}

void nova_lsp_send_notification(const char *method, const char *params)
{
    char notification[8192];
    snprintf(notification, sizeof(notification),
             "{\"jsonrpc\":\"2.0\",\"method\":\"%s\",\"params\":%s}\r\n",
             method, params);
    
    printf("Content-Length: %zu\r\n\r\n%s", strlen(notification), notification);
    fflush(stdout);
}

void nova_lsp_send_diagnostics(const char *uri, lsp_diagnostic_t *diagnostics, int count)
{
    char diags[4096] = "[";
    
    for (int i = 0; i < count; i++) {
        char diag[512];
        snprintf(diag, sizeof(diag),
                 "{\"range\":{\"start\":{\"line\":%d,\"character\":%d},"
                 "\"end\":{\"line\":%d,\"character\":%d}},"
                 "\"severity\":%d,\"message\":\"%s\",\"source\":\"nova\"}%s",
                 diagnostics[i].range.start.line,
                 diagnostics[i].range.start.character,
                 diagnostics[i].range.end.line,
                 diagnostics[i].range.end.character,
                 diagnostics[i].severity,
                 diagnostics[i].message,
                 (i < count - 1) ? "," : "");
        strcat(diags, diag);
    }
    strcat(diags, "]");
    
    char params[4096];
    snprintf(params, sizeof(params),
             "{\"uri\":\"%s\",\"diagnostics\":%s}",
             uri, diags);
    
    nova_lsp_send_notification("textDocument/publishDiagnostics", params);
}

// ═══════════════════════════════════════════════════════════════════════════
// Feature Handlers
// ═══════════════════════════════════════════════════════════════════════════

char* nova_lsp_handle_initialize(const char *params)
{
    g_server.initialized = true;
    g_server.workspace_root = find_json_value(params, "rootUri");
    
    fprintf(stderr, "[Nova LSP] Workspace: %s\n", 
            g_server.workspace_root ? g_server.workspace_root : "none");
    
    // Return server capabilities
    static char result[2048];
    snprintf(result, sizeof(result),
             "{"
             "\"capabilities\":{"
             "\"textDocumentSync\":1,"
             "\"completionProvider\":{\"triggerCharacters\":[\".\",\":\"]},"
             "\"hoverProvider\":true,"
             "\"definitionProvider\":true,"
             "\"referencesProvider\":true,"
             "\"documentFormattingProvider\":true,"
             "\"renameProvider\":true"
             "},"
             "\"serverInfo\":{\"name\":\"nova-lsp\",\"version\":\"0.1.0\"}"
             "}");
    
    return result;
}

lsp_completion_item_t* nova_lsp_handle_completion(const char *uri, lsp_position_t pos, int *count)
{
    // Simple completion: Nova keywords + stdlib
    static lsp_completion_item_t items[] = {
        {"fn", "function", "Declare a function", 3},
        {"let", "variable", "Declare a variable", 6},
        {"const", "constant", "Declare a constant", 6},
        {"struct", "structure", "Declare a struct", 7},
        {"impl", "implementation", "Implement methods", 8},
        {"if", "conditional", "If statement", 14},
        {"for", "loop", "For loop", 14},
        {"while", "loop", "While loop", 14},
        {"return", "keyword", "Return from function", 14},
        {"import", "keyword", "Import module", 9},
        {"export", "keyword", "Export symbol", 9},
        {"println", "function", "Print with newline", 2},
        {"Vec", "type", "Dynamic array", 7},
        {"HashMap", "type", "Hash map", 7},
        {"String", "type", "String type", 7}
    };
    
    *count = sizeof(items) / sizeof(items[0]);
    return items;
}

lsp_hover_t* nova_lsp_handle_hover(const char *uri, lsp_position_t pos)
{
    static lsp_hover_t hover;
    hover.contents = "**Nova Language**\n\nHover information for symbol at this position.";
    hover.range.start = pos;
    hover.range.end = pos;
    hover.range.end.character += 5;
    
    return &hover;
}

lsp_location_t* nova_lsp_handle_definition(const char *uri, lsp_position_t pos)
{
    // Stub: would parse and find actual definition
    static lsp_location_t location;
    location.uri = strdup(uri);
    location.range.start.line = 0;
    location.range.start.character = 0;
    location.range.end.line = 0;
    location.range.end.character = 10;
    
    return &location;
}

lsp_location_t* nova_lsp_handle_references(const char *uri, lsp_position_t pos, int *count)
{
    // Stub: would find all references
    static lsp_location_t refs[10];
    *count = 1;
    
    refs[0].uri = strdup(uri);
    refs[0].range.start.line = pos.line;
    refs[0].range.start.character = pos.character;
    refs[0].range.end = refs[0].range.start;
    
    return refs;
}

char* nova_lsp_handle_formatting(const char *uri)
{
    // Stub: would call znfmt
    static char result[64];
    snprintf(result, sizeof(result), "[]");
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
// Message Processing
// ═══════════════════════════════════════════════════════════════════════════

int nova_lsp_process_message(const char *message)
{
    lsp_method_t method = nova_lsp_parse_method(message);
    int id = find_json_int(message, "id");
    
    g_server.requests_handled++;
    
    fprintf(stderr, "[Nova LSP] Method: %d, ID: %d\n", method, id);
    
    switch (method) {
        case LSP_METHOD_INITIALIZE: {
            char *result = nova_lsp_handle_initialize(message);
            nova_lsp_send_response(id, result);
            break;
        }
        
        case LSP_METHOD_SHUTDOWN:
            g_server.shutdown_requested = true;
            nova_lsp_send_response(id, "null");
            break;
            
        case LSP_METHOD_EXIT:
            return 0; // Exit main loop
            
        case LSP_METHOD_TEXT_DOCUMENT_COMPLETION: {
            int count;
            lsp_completion_item_t *items = nova_lsp_handle_completion("", (lsp_position_t){0,0}, &count);
            
            char result[4096] = "[";
            for (int i = 0; i < count; i++) {
                char item[256];
                snprintf(item, sizeof(item),
                         "{\"label\":\"%s\",\"detail\":\"%s\",\"documentation\":\"%s\",\"kind\":%d}%s",
                         items[i].label, items[i].detail, items[i].documentation, items[i].kind,
                         (i < count - 1) ? "," : "");
                strcat(result, item);
            }
            strcat(result, "]");
            
            nova_lsp_send_response(id, result);
            break;
        }
        
        case LSP_METHOD_TEXT_DOCUMENT_HOVER: {
            lsp_hover_t *hover = nova_lsp_handle_hover("", (lsp_position_t){0,0});
            char result[512];
            snprintf(result, sizeof(result),
                     "{\"contents\":{\"kind\":\"markdown\",\"value\":\"%s\"}}",
                     hover->contents);
            nova_lsp_send_response(id, result);
            break;
        }
        
        case LSP_METHOD_INITIALIZED:
        case LSP_METHOD_TEXT_DOCUMENT_DID_OPEN:
        case LSP_METHOD_TEXT_DOCUMENT_DID_CHANGE:
        case LSP_METHOD_TEXT_DOCUMENT_DID_SAVE:
        case LSP_METHOD_TEXT_DOCUMENT_DID_CLOSE:
            // Notifications - no response needed
            break;
            
        default:
            fprintf(stderr, "[Nova LSP] Unknown method: %d\n", method);
            if (id >= 0) {
                nova_lsp_send_response(id, "{\"error\":{\"code\":-32601,\"message\":\"Method not found\"}}");
            }
    }
    
    return 1; // Continue
}

// ═══════════════════════════════════════════════════════════════════════════
// Main Server Loop
// ═══════════════════════════════════════════════════════════════════════════

int nova_lsp_start(void)
{
    nova_lsp_init();
    
    fprintf(stderr, "[Nova LSP] Server started\n");
    
    char line[8192];
    int content_length = 0;
    
    while (fgets(line, sizeof(line), stdin)) {
        // Parse Content-Length header
        if (strncmp(line, "Content-Length:", 15) == 0) {
            content_length = atoi(line + 15);
        }
        
        // Empty line signals end of headers
        if (strcmp(line, "\r\n") == 0 || strcmp(line, "\n") == 0) {
            if (content_length > 0) {
                char *message = malloc(content_length + 1);
                fread(message, 1, content_length, stdin);
                message[content_length] = '\0';
                
                if (!nova_lsp_process_message(message)) {
                    free(message);
                    break;
                }
                
                free(message);
                content_length = 0;
            }
        }
    }
    
    fprintf(stderr, "[Nova LSP] Server stopped\n");
    return 0;
}

void nova_lsp_shutdown(void)
{
    if (g_server.workspace_root) {
        free(g_server.workspace_root);
    }
    fprintf(stderr, "[Nova LSP] Shutdown complete\n");
}
