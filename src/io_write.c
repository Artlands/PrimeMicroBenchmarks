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
#define DEFAULT_ITERS 10ULL

int main(int argc, char **argv) {
    double t0 = bench_now_sec();

    BENCH_PRINTF("I/O write start\n");

    size_t chunk_size = bench_parse_size(argc, argv, DEFAULT_CHUNK_SIZE);
    char *buffer = (char*)malloc(chunk_size);
    // Fill buffer to prevent OS zero-page optimization
    for (size_t i = 0; i < chunk_size; i++) buffer[i] = (char)i;

    unsigned long long warmup_iters = bench_parse_warmup_iterations(argc, argv, 1ULL);
    unsigned long long iterations = bench_parse_iterations(argc, argv, DEFAULT_ITERS);

    if (warmup_iters > 0ULL) {

        BENCH_PRINTF("I/O write warmup start\n");

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

    BENCH_PRINTF("I/O write loop start\n");

    BENCH_EPRINTF("LOOP_START_REL %f\n", bench_now_sec() - t0);

    for (unsigned long long iter = 0; iter < iterations; iter++) {
        FILE *fp = fopen("/tmp/test_io_file.bin", "wb");
        if (!fp) exit(1);
        for (int i = 0; i < 100; i++) {
            fwrite(buffer, 1, chunk_size, fp);
        }
        fsync(fileno(fp));
        fclose(fp);
    }
    BENCH_EPRINTF("LOOP_END_REL %f\n", bench_now_sec() - t0);

    BENCH_PRINTF("I/O write complete\n");

    BENCH_PRINTF("Loop iterations: %llu\n", iterations);
    BENCH_PRINTF("Loop time: %f seconds\n", bench_now_sec() - start);

    remove("/tmp/test_io_file.bin");
    free(buffer);
    return 0;
}
