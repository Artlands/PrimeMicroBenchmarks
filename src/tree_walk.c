/* 
 * Tree-walk benchmark.
 * Random BST searches to stress pointer chasing, branch behavior,
 * and cache miss latency.
 */

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
#define DEFAULT_ITERS 80000000ULL

int main(int argc, char **argv) {
    double t0 = bench_now_sec();

    BENCH_PRINTF("Tree walk start\n");

    srand(bench_parse_seed(argc, argv, DEFAULT_SEED));
    Node *root = NULL;

    // 1. Build a Random Tree
    // Random insertion creates an unbalanced tree (deeper paths), 
    // which is good for stressing the walk.
    for (int i = 0; i < NODES; i++) {
        root = insert(root, rand());
    }

    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 8000ULL);
    unsigned long long iterations = bench_parse_iterations(argc, argv, DEFAULT_ITERS);
    long found_count = 0;

    // 2. The Walk Loop
    if (warmup_iters > 0ULL) {

        BENCH_PRINTF("Tree walk warmup start\n");

        for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
            int key = rand();
            found_count += search(root, key);
        }
        found_count = 0;
    }

    double start = bench_now_sec();

    BENCH_PRINTF("Tree walk loop start\n");

    BENCH_EPRINTF("LOOP_START_REL %f\n", bench_now_sec() - t0);
    for (unsigned long long iter = 0; iter < iterations; iter++) {
        int key = rand();
        found_count += search(root, key);
    }
    BENCH_EPRINTF("LOOP_END_REL %f\n", bench_now_sec() - t0);

    BENCH_PRINTF("Searches completed. Found: %ld\n", found_count);
    BENCH_PRINTF("Tree walk complete\n");

    BENCH_PRINTF("Loop iterations: %llu\n", iterations);
    BENCH_PRINTF("Loop time: %f seconds\n", bench_now_sec() - start);
    
    return 0;
}
