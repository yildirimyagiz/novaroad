#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ALPHABET_SIZE 128

typedef struct TrieNode {
    struct TrieNode *children[ALPHABET_SIZE];
    char *translation;
    char *category;
    bool is_end_of_word;
} TrieNode;

static TrieNode *trie_root = NULL;

static TrieNode *create_helper_node(void) {
    TrieNode *node = (TrieNode *)malloc(sizeof(TrieNode));
    if (node) {
        node->is_end_of_word = false;
        node->translation = NULL;
        node->category = NULL;
        for (int i = 0; i < ALPHABET_SIZE; i++) {
            node->children[i] = NULL;
        }
    }
    return node;
}

static void insert_helper_word(TrieNode *root, const char *key, const char *translation, const char *category) {
    TrieNode *crawler = root;
    int length = strlen(key);
    for (int level = 0; level < length; level++) {
        unsigned char index = (unsigned char)key[level];
        if (index >= ALPHABET_SIZE) index = '_';
        
        if (!crawler->children[index]) {
            crawler->children[index] = create_helper_node();
        }
        crawler = crawler->children[index];
    }
    crawler->is_end_of_word = true;
    if (crawler->translation) free(crawler->translation);
    crawler->translation = strdup(translation);
    if (crawler->category) free(crawler->category);
    crawler->category = strdup(category ? category : "GENERAL");
}

int64_t init_linguist(const char* dict_path) {
    FILE *file = fopen(dict_path, "r");
    if (!file) {
        printf("⚠️ Helper warning: Could not open dictionary file: %s\n", dict_path);
        return 0;
    }
    
    trie_root = create_helper_node();
    char line[1024];
    int count = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = 0;
        char *word = strtok(line, "\t");
        char *translation = strtok(NULL, "\t");
        char *category = strtok(NULL, "\t");
        
        if (word && translation) {
            insert_helper_word(trie_root, word, translation, category);
            count++;
        }
    }
    fclose(file);
    return count;
}

const char* lookup_linguist(const char* word) {
    if (!trie_root || !word) return "";
    
    TrieNode *crawler = trie_root;
    int length = strlen(word);
    for (int level = 0; level < length; level++) {
        unsigned char index = (unsigned char)word[level];
        if (index >= ALPHABET_SIZE) return "";
        
        if (!crawler->children[index]) {
            return "";
        }
        crawler = crawler->children[index];
    }
    if (crawler != NULL && crawler->is_end_of_word && crawler->translation) {
        return crawler->translation;
    }
    return "";
}

const char* lookup_linguist_category(const char* word) {
    if (!trie_root || !word) return "NONE";
    
    TrieNode *crawler = trie_root;
    int length = strlen(word);
    for (int level = 0; level < length; level++) {
        unsigned char index = (unsigned char)word[level];
        if (index >= ALPHABET_SIZE) return "NONE";
        
        if (!crawler->children[index]) {
            return "NONE";
        }
        crawler = crawler->children[index];
    }
    if (crawler != NULL && crawler->is_end_of_word && crawler->category) {
        return crawler->category;
    }
    return "NONE";
}

const char* analyze_morphology(const char* word) {
    static char result_buf[1024];
    memset(result_buf, 0, sizeof(result_buf));
    
    // First, look up directly
    const char* direct = lookup_linguist(word);
    if (direct && strcmp(direct, "") != 0) {
        snprintf(result_buf, sizeof(result_buf), "{\"root\":\"%s\",\"translation\":\"%s\",\"suffix\":\"None\",\"desc\":\"Direct match in dictionary\",\"type\":\"root\",\"synth\":\"%s\"}", word, direct, direct);
        return result_buf;
    }
    
    int len = strlen(word);
    if (len < 4) return ""; // Too short to analyze safely
    
    // Check suffix -sel / -sal (derivative denoting association)
    if ((len > 3 && strcmp(word + len - 3, "sel") == 0) || (len > 3 && strcmp(word + len - 3, "sal") == 0)) {
        char root[256];
        strncpy(root, word, len - 3);
        root[len - 3] = '\0';
        const char* r_trans = lookup_linguist(root);
        if (r_trans && strcmp(r_trans, "") != 0) {
            snprintf(result_buf, sizeof(result_buf), 
                     "{\"root\":\"%s\",\"translation\":\"%s\",\"suffix\":\"+sel/sal\",\"desc\":\"Derivative suffix denoting association (related to/scientific)\",\"type\":\"derivative\",\"synth\":\"related to %s; scientific (%s)\"}", 
                     root, r_trans, root, r_trans);
            return result_buf;
        }
    }
    
    // Check suffix -ler / -lar (plural)
    if ((len > 3 && strcmp(word + len - 3, "ler") == 0) || (len > 3 && strcmp(word + len - 3, "lar") == 0)) {
        char root[256];
        strncpy(root, word, len - 3);
        root[len - 3] = '\0';
        const char* r_trans = lookup_linguist(root);
        if (r_trans && strcmp(r_trans, "") != 0) {
            snprintf(result_buf, sizeof(result_buf), 
                     "{\"root\":\"%s\",\"translation\":\"%s\",\"suffix\":\"+ler/lar\",\"desc\":\"Plural suffix (denoting multiple items)\",\"type\":\"plural\",\"synth\":\"plural of %s (%s)\"}", 
                     root, r_trans, root, r_trans);
            return result_buf;
        }
    }
    
    // Check suffix -lik / -lık / -luk / -lük (abstract noun / collective state)
    if (len > 3 && (strcmp(word + len - 3, "lik") == 0 || strcmp(word + len - 3, "lık") == 0 || strcmp(word + len - 3, "luk") == 0 || strcmp(word + len - 3, "lük") == 0)) {
        char root[256];
        strncpy(root, word, len - 3);
        root[len - 3] = '\0';
        const char* r_trans = lookup_linguist(root);
        if (r_trans && strcmp(r_trans, "") != 0) {
            snprintf(result_buf, sizeof(result_buf), 
                     "{\"root\":\"%s\",\"translation\":\"%s\",\"suffix\":\"+lik/lık/luk/lük\",\"desc\":\"Abstract noun or collective state suffix\",\"type\":\"abstract\",\"synth\":\"state of being %s; a place/tool for %s\"}", 
                     root, r_trans, root, root);
            return result_buf;
        }
    }
    
    // Check suffix -ci / -cı / -cu / -cü / -çi / -çı / -çu / -çü (doer / profession)
    if (len > 2 && (strcmp(word + len - 2, "ci") == 0 || strcmp(word + len - 2, "cı") == 0 || strcmp(word + len - 2, "cu") == 0 || strcmp(word + len - 2, "cü") == 0 ||
                   strcmp(word + len - 2, "çi") == 0 || strcmp(word + len - 2, "çı") == 0 || strcmp(word + len - 2, "çu") == 0 || strcmp(word + len - 2, "çü") == 0)) {
        char root[256];
        strncpy(root, word, len - 2);
        root[len - 2] = '\0';
        const char* r_trans = lookup_linguist(root);
        if (r_trans && strcmp(r_trans, "") != 0) {
            snprintf(result_buf, sizeof(result_buf), 
                     "{\"root\":\"%s\",\"translation\":\"%s\",\"suffix\":\"+ci/cı/cu/cü\",\"desc\":\"Profession / agent suffix (one who does/works with)\",\"type\":\"profession\",\"synth\":\"one who practices/works with %s (%s)\"}", 
                     root, r_trans, root, r_trans);
            return result_buf;
        }
    }
    
    return "";
}

const char* read_console_input() {
    static char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    printf("\n👉 Enter input (or 'q' to quit): ");
    fflush(stdout);
    if (fgets(buffer, sizeof(buffer), stdin)) {
        buffer[strcspn(buffer, "\r\n")] = 0;
        return buffer;
    }
    return "";
}

const char* get_app_mode_env() {
    const char* mode = getenv("NOVA_MODE");
    if (mode) return mode;
    return "";
}

const char* get_http_body(const char* response) {
    if (!response) return "";
    const char* body = strstr(response, "\r\n\r\n");
    if (body) {
        return body + 4;
    }
    body = strstr(response, "\n\n");
    if (body) {
        return body + 2;
    }
    return response;
}

const char* advance_pointer(const char* ptr, int offset) {
    if (!ptr) return "";
    return ptr + offset;
}


