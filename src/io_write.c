#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "bench_args.h"

#define CHUNK_SIZE (1024 * 1024 * 10) // 10 MB chunks
#define DEFAULT_CHUNK_SIZE CHUNK_SIZE

int main(int argc, char **argv) {
    double t0 = bench_now_sec();
    size_t chunk_size = bench_parse_size(argc, argv, DEFAULT_CHUNK_SIZE);
    char *buffer = (char*)malloc(chunk_size);
    // Fill buffer to prevent OS zero-page optimization
    for (size_t i = 0; i < chunk_size; i++) buffer[i] = (char)i;

    double warmup = bench_parse_warmup(argc, argv, 0.0);
    double duration = bench_parse_duration(argc, argv, 60.0);

    if (warmup > 0.0) {
        double warm_start = bench_now_sec();
        while ((bench_now_sec() - warm_start) < warmup) {
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
    fprintf(stderr, "LOOP_START_REL %f\n", bench_now_sec() - t0);
    
    while ((bench_now_sec() - start) < duration) {
        FILE *fp = fopen("/tmp/test_io_file.bin", "wb"); // Ensure this path is on actual disk/NVMe
        if (!fp) exit(1);
        
        // Write 1GB then close
        for (int i = 0; i < 100; i++) {
            fwrite(buffer, 1, chunk_size, fp);
        }
        
        // Sync to force physical I/O (CPU waits here)
        fsync(fileno(fp));
        fclose(fp);
    }
    fprintf(stderr, "LOOP_END_REL %f\n", bench_now_sec() - t0);
    remove("/tmp/test_io_file.bin");
    free(buffer);
    return 0;
}
