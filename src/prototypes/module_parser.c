// Advanced Module Parser
// Token-based module declaration and import parsing

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Simple token types for module parsing
typedef enum {
    TOK_EOF,
    TOK_IDENT,
    TOK_MOD,
    TOK_IMPORT,
    TOK_PUB,
    TOK_AS,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_COLON_COLON,
    TOK_STAR,
    TOK_COMMA,
    TOK_SEMICOLON,
    TOK_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char* text;
} Token;

// Simple lexer for module syntax
typedef struct {
    const char* source;
    size_t current;
    Token current_token;
} ModuleLexer;

ModuleLexer* module_lexer_create(const char* source) {
    ModuleLexer* lexer = malloc(sizeof(ModuleLexer));
    lexer->source = source;
    lexer->current = 0;
    return lexer;
}

void module_lexer_destroy(ModuleLexer* lexer) {
    free(lexer);
}

char peek(ModuleLexer* lexer) {
    if (lexer->current >= strlen(lexer->source)) return '\0';
    return lexer->source[lexer->current];
}

char advance(ModuleLexer* lexer) {
    return lexer->source[lexer->current++];
}

void skip_whitespace(ModuleLexer* lexer) {
    while (isspace(peek(lexer))) {
        advance(lexer);
    }
}

Token module_lexer_next(ModuleLexer* lexer) {
    skip_whitespace(lexer);

    char c = peek(lexer);
    if (c == '\0') {
        return (Token){TOK_EOF, ""};
    }

    // Keywords
    if (isalpha(c)) {
        size_t start = lexer->current;
        while (isalnum(peek(lexer)) || peek(lexer) == '_') {
            advance(lexer);
        }
        char* text = strndup(lexer->source + start, lexer->current - start);

        // Check keywords
        if (strcmp(text, "mod") == 0) return (Token){TOK_MOD, text};
        if (strcmp(text, "import") == 0) return (Token){TOK_IMPORT, text};
        if (strcmp(text, "pub") == 0) return (Token){TOK_PUB, text};
        if (strcmp(text, "as") == 0) return (Token){TOK_AS, text};

        return (Token){TOK_IDENT, text};
    }

    // Symbols
    advance(lexer);
    switch (c) {
        case '{': return (Token){TOK_LBRACE, "{"};
        case '}': return (Token){TOK_RBRACE, "}"};
        case '(': return (Token){TOK_LPAREN, "("};
        case ')': return (Token){TOK_RPAREN, ")"};
        case '*': return (Token){TOK_STAR, "*"};
        case ',': return (Token){TOK_COMMA, ","};
        case ';': return (Token){TOK_SEMICOLON, ";"};
        case ':':
            if (peek(lexer) == ':') {
                advance(lexer);
                return (Token){TOK_COLON_COLON, "::"};
            }
            return (Token){TOK_UNKNOWN, ":"};
        default: return (Token){TOK_UNKNOWN, (char[]){c, '\0'}};
    }
}

// Module declarations
typedef struct {
    char* name;
} ModuleDecl;

typedef struct {
    char* module_name;
    char** imports;
    size_t import_count;
    char* alias;
} ImportDecl;

typedef struct {
    char* item_name;
} PublicDecl;

// Parse module declaration
ModuleDecl* parse_module_decl(ModuleLexer* lexer) {
    Token token = module_lexer_next(lexer);
    if (token.type != TOK_IDENT) {
        printf("Expected module name after 'mod'\n");
        free(token.text);
        return NULL;
    }

    char* module_name = token.text;

    token = module_lexer_next(lexer);
    if (token.type != TOK_SEMICOLON) {
        printf("Expected ';' after module name\n");
        free(module_name);
        free(token.text);
        return NULL;
    }
    free(token.text);

    ModuleDecl* decl = malloc(sizeof(ModuleDecl));
    decl->name = module_name;
    return decl;
}

// Parse import declaration
ImportDecl* parse_import_decl(ModuleLexer* lexer) {
    Token token = module_lexer_next(lexer);
    if (token.type != TOK_IDENT) {
        printf("Expected module name after 'import'\n");
        free(token.text);
        return NULL;
    }

    ImportDecl* decl = calloc(1, sizeof(ImportDecl));
    decl->module_name = token.text;

    // Check for :: syntax
    token = module_lexer_next(lexer);
    if (token.type == TOK_COLON_COLON) {
        free(token.text);

        token = module_lexer_next(lexer);
        if (token.type == TOK_LBRACE) {
            free(token.text);
            // Parse {item1, item2, ...}
            decl->imports = malloc(sizeof(char*) * 10); // Max 10 for now
            decl->import_count = 0;

            while (1) {
                token = module_lexer_next(lexer);
                if (token.type == TOK_RBRACE) {
                    free(token.text);
                    break;
                }
                if (token.type == TOK_IDENT) {
                    decl->imports[decl->import_count++] = token.text;
                } else {
                    free(token.text);
                }

                token = module_lexer_next(lexer);
                if (token.type != TOK_COMMA && token.type != TOK_RBRACE) {
                    free(token.text);
                    break;
                }
                if (token.type == TOK_RBRACE) {
                    free(token.text);
                    break;
                }
                free(token.text);
            }
        } else if (token.type == TOK_STAR) {
            free(token.text);
            decl->imports = malloc(sizeof(char*));
            decl->imports[0] = strdup("*");
            decl->import_count = 1;
        } else {
            free(token.text);
        }
    } else if (token.type == TOK_AS) {
        // import foo as bar;
        free(token.text);
        token = module_lexer_next(lexer);
        if (token.type == TOK_IDENT) {
            decl->alias = token.text;
        } else {
            free(token.text);
        }
    } else {
        free(token.text);
    }

    // Expect semicolon
    token = module_lexer_next(lexer);
    if (token.type != TOK_SEMICOLON) {
        printf("Expected ';' after import\n");
        free(token.text);
        // Don't free here, will be cleaned up by caller
    } else {
        free(token.text);
    }

    return decl;
}

// Parse public declaration
PublicDecl* parse_public_decl(ModuleLexer* lexer) {
    // Skip until we find a recognizable pattern
    Token token;
    while ((token = module_lexer_next(lexer)).type != TOK_SEMICOLON &&
           token.type != TOK_EOF) {
        free(token.text);
    }
    free(token.text);

    PublicDecl* decl = malloc(sizeof(PublicDecl));
    decl->item_name = strdup("public_item");
    return decl;
}

// Main parser
int parse_module_file(const char* filename) {
    printf("=== Advanced Module Parser ===\n");
    printf("Parsing: %s\n\n", filename);

    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Failed to open file: %s\n", filename);
        return 1;
    }

    // Read entire file
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* source = malloc(length + 1);
    fread(source, 1, length, file);
    source[length] = '\0';
    fclose(file);

    // Create lexer
    ModuleLexer* lexer = module_lexer_create(source);

    Token token;
    int decl_count = 0;

    while ((token = module_lexer_next(lexer)).type != TOK_EOF) {
        switch (token.type) {
            case TOK_MOD: {
                free(token.text);
                printf("📦 Module Declaration:\n");
                ModuleDecl* decl = parse_module_decl(lexer);
                if (decl) {
                    printf("   Name: %s\n", decl->name);
                    free(decl->name);
                    free(decl);
                    decl_count++;
                }
                break;
            }

            case TOK_IMPORT: {
                free(token.text);
                printf("📥 Import Declaration:\n");
                ImportDecl* decl = parse_import_decl(lexer);
                if (decl) {
                    printf("   Module: %s\n", decl->module_name);
                    if (decl->imports && decl->import_count > 0) {
                        printf("   Items: ");
                        for (size_t i = 0; i < decl->import_count; i++) {
                            printf("%s", decl->imports[i]);
                            if (i < decl->import_count - 1) printf(", ");
                            free(decl->imports[i]);
                        }
                        printf("\n");
                        free(decl->imports);
                    }
                    if (decl->alias) {
                        printf("   Alias: %s\n", decl->alias);
                        free(decl->alias);
                    }
                    free(decl->module_name);
                    free(decl);
                    decl_count++;
                }
                break;
            }

            case TOK_PUB: {
                free(token.text);
                printf("🔓 Public Declaration:\n");
                PublicDecl* decl = parse_public_decl(lexer);
                if (decl) {
                    printf("   Item: %s\n", decl->item_name);
                    free(decl->item_name);
                    free(decl);
                    decl_count++;
                }
                break;
            }

            default:
                free(token.text);
                break;
        }
    }

    module_lexer_destroy(lexer);
    free(source);

    printf("\n✅ Parsed %d module declarations\n", decl_count);
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <module_file>\n", argv[0]);
        return 1;
    }

    return parse_module_file(argv[1]);
}
