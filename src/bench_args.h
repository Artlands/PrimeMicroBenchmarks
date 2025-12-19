#ifndef BENCH_ARGS_H
#define BENCH_ARGS_H

#include <stdlib.h>
#include <string.h>
#include <time.h>

static inline double bench_now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static inline double bench_parse_duration(int argc, char **argv, double def) {
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--duration") == 0) {
            return atof(argv[i + 1]);
        }
    }
    return def;
}

static inline double bench_parse_warmup(int argc, char **argv, double def) {
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--warmup") == 0) {
            return atof(argv[i + 1]);
        }
    }
    return def;
}

static inline unsigned int bench_parse_seed(int argc, char **argv, unsigned int def) {
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--seed") == 0) {
            return (unsigned int)strtoul(argv[i + 1], NULL, 10);
        }
    }
    return def;
}

static inline size_t bench_parse_size(int argc, char **argv, size_t def) {
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--size") == 0) {
            return (size_t)strtoull(argv[i + 1], NULL, 10);
        }
    }
    return def;
}

#endif
