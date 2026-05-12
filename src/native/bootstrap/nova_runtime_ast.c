#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Simple AST Node structure for bootstrap Stage 1
typedef struct ASTNode {
    int64_t kind;
    int64_t token_val; 
    struct ASTNode* left;
    struct ASTNode* right;
} ASTNode;

int64_t ast_node_new(int64_t kind, int64_t token_val, int64_t left_ptr, int64_t right_ptr) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->kind = kind;
    node->token_val = token_val;
    node->left = (ASTNode*)left_ptr;
    node->right = (ASTNode*)right_ptr;
    return (int64_t)node;
}

int64_t ast_node_get_kind(int64_t ptr) { return ptr ? ((ASTNode*)ptr)->kind : 0; }
int64_t ast_node_get_left(int64_t ptr) { return ptr ? (int64_t)((ASTNode*)ptr)->left : 0; }
int64_t ast_node_get_right(int64_t ptr) { return ptr ? (int64_t)((ASTNode*)ptr)->right : 0; }

void ast_node_print(int64_t ptr, int64_t depth) {
    if (!ptr) return;
    ASTNode* node = (ASTNode*)ptr;
    for (int i=0; i<depth; i++) printf("  ");
    printf("├── Node(kind: %lld, val: %lld)\n", (long long)node->kind, (long long)node->token_val);
    ast_node_print((int64_t)node->left, depth + 1);
    ast_node_print((int64_t)node->right, depth + 1);
}
