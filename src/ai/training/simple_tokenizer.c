/**
 * simple_tokenizer.c - Character-Level Tokenizer
 * 
 * Simple character-level tokenizer for demo/testing
 * Perfect for Shakespeare, code, etc.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define MAX_VOCAB_SIZE 256  // All ASCII characters

typedef struct {
    char *vocab[MAX_VOCAB_SIZE];
    int vocab_size;
    int char_to_id[256];  // ASCII lookup table
} SimpleTokenizer;

/**
 * Create character-level tokenizer
 */
SimpleTokenizer *simple_tokenizer_create() {
    SimpleTokenizer *tok = calloc(1, sizeof(SimpleTokenizer));
    
    // Initialize with printable ASCII characters
    tok->vocab_size = 0;
    
    for (int i = 0; i < 256; i++) {
        tok->char_to_id[i] = -1;  // Invalid by default
    }
    
    // Add all printable ASCII + common control chars
    for (int c = 0; c < 128; c++) {
        if (c >= 32 && c < 127) {  // Printable ASCII
            tok->vocab[tok->vocab_size] = malloc(2);
            tok->vocab[tok->vocab_size][0] = (char)c;
            tok->vocab[tok->vocab_size][1] = '\0';
            tok->char_to_id[c] = tok->vocab_size;
            tok->vocab_size++;
        } else if (c == '\n' || c == '\t' || c == '\r') {  // Control chars
            tok->vocab[tok->vocab_size] = malloc(2);
            tok->vocab[tok->vocab_size][0] = (char)c;
            tok->vocab[tok->vocab_size][1] = '\0';
            tok->char_to_id[c] = tok->vocab_size;
            tok->vocab_size++;
        }
    }
    
    printf("✅ Simple tokenizer created: %d tokens\n", tok->vocab_size);
    
    return tok;
}

/**
 * Encode text to token IDs
 */
int64_t *simple_tokenizer_encode(SimpleTokenizer *tok, const char *text, int64_t *num_tokens) {
    if (!tok || !text) return NULL;
    
    size_t len = strlen(text);
    int64_t *tokens = malloc(len * sizeof(int64_t));
    int64_t count = 0;
    
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)text[i];
        int token_id = tok->char_to_id[c];
        
        if (token_id >= 0) {
            tokens[count++] = token_id;
        } else {
            // Unknown character, use space as fallback
            tokens[count++] = tok->char_to_id[' '];
        }
    }
    
    *num_tokens = count;
    return tokens;
}

/**
 * Decode token IDs to text
 */
char *simple_tokenizer_decode(SimpleTokenizer *tok, const int64_t *tokens, int64_t num_tokens) {
    if (!tok || !tokens) return NULL;
    
    char *text = malloc((num_tokens + 1) * sizeof(char));
    
    for (int64_t i = 0; i < num_tokens; i++) {
        int64_t token_id = tokens[i];
        if (token_id >= 0 && token_id < tok->vocab_size) {
            text[i] = tok->vocab[token_id][0];
        } else {
            text[i] = '?';
        }
    }
    
    text[num_tokens] = '\0';
    return text;
}

/**
 * Free tokenizer
 */
void simple_tokenizer_destroy(SimpleTokenizer *tok) {
    if (!tok) return;
    
    for (int i = 0; i < tok->vocab_size; i++) {
        free(tok->vocab[i]);
    }
    
    free(tok);
}
