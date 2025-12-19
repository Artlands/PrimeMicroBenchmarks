/* 
 * I/O write wait benchmark.
 * Repeated buffered writes followed by fsync to force storage flushes and
 * emphasize I/O wait and sync overhead.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "bench_args.h"
#define CHUNK_SIZE (1024 * 1024 * 10) // 10 MB chunks
#define DEFAULT_CHUNK_SIZE CHUNK_SIZE
#define DEFAULT_ITERS 5ULL

int main(int argc, char **argv) {
    double t0 = bench_now_sec();
    printf("I/O write start\n");
    size_t chunk_size = bench_parse_size(argc, argv, DEFAULT_CHUNK_SIZE);
    char *buffer = (char*)malloc(chunk_size);
    // Fill buffer to prevent OS zero-page optimization
    for (size_t i = 0; i < chunk_size; i++) buffer[i] = (char)i;

    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 0ULL);
    unsigned long long iterations = bench_parse_iterations(argc, argv, DEFAULT_ITERS);

    if (warmup_iters > 0ULL) {
        printf("I/O write warmup start\n");
        for (unsigned long long iter = 0; iter < warmup_iters; iter++) {
            FILE *fp = fopen("/tmp/test_io_file.bin", "wb");
            if (!fp) exit(1);
            for (int i = 0; i < 100; i++) {
                fwrite(buffer, 1, chunk_size, fp);
            }
            fsync(fileno(fp));
            fclose(fp);
        }
        remove("/tmp/test_io_file.bin");
    }
    double start = bench_now_sec();
    printf("I/O write loop start\n");
    fprintf(stderr, "LOOP_START_REL %f\n", bench_now_sec() - t0);
    
    for (unsigned long long iter = 0; iter < iterations; iter++) {
        FILE *fp = fopen("/tmp/test_io_file.bin", "wb");
        if (!fp) exit(1);
        for (int i = 0; i < 100; i++) {
            fwrite(buffer, 1, chunk_size, fp);
        }
        fsync(fileno(fp));
        fclose(fp);
    }
    fprintf(stderr, "LOOP_END_REL %f\n", bench_now_sec() - t0);
    printf("I/O write complete\n");
    printf("Loop iterations: %llu\n", iterations);
    printf("Loop time: %f seconds\n", bench_now_sec() - start);
    remove("/tmp/test_io_file.bin");
    free(buffer);
    return 0;
}
