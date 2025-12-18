#include <stdio.h>
#include <stdlib.h>
#include "bench_args.h"

typedef struct Node {
    int value;
    struct Node *left;
    struct Node *right;
} Node;

// Create a new node
Node* newNode(int value) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->value = value;
    node->left = NULL;
    node->right = NULL;
    return node;
}

// Insert (Standard BST)
Node* insert(Node* node, int value) {
    if (node == NULL) return newNode(value);
    if (value < node->value)
        node->left = insert(node->left, value);
    else if (value > node->value)
        node->right = insert(node->right, value);
    return node;
}

// Search (The hot kernel)
int search(Node* root, int key) {
    Node* current = root;
    while (current != NULL) {
        if (key == current->value) return 1; // Found
        
        // This logic requires ALU + Branching + Memory Load
        if (key < current->value)
            current = current->left;
        else
            current = current->right;
    }
    return 0; // Not found
}

#define NODES 1000000
#define DEFAULT_SEED 1u

int main(int argc, char **argv) {
    srand(bench_parse_seed(argc, argv, DEFAULT_SEED));
    Node *root = NULL;

    // 1. Build a Random Tree
    // Random insertion creates an unbalanced tree (deeper paths), 
    // which is good for stressing the walk.
    for (int i = 0; i < NODES; i++) {
        root = insert(root, rand());
    }

    printf("Tree built. Starting walk...\n");

    double start = bench_now_sec();
    double duration = bench_parse_duration(argc, argv, 60.0);
    long found_count = 0;

    // 2. The Walk Loop
    while ((bench_now_sec() - start) < duration) {
        // Search for a random value
        int key = rand(); 
        found_count += search(root, key);
    }

    printf("Searches completed. Found: %ld\n", found_count);
    return 0;
}
