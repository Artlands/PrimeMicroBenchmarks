#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "bench_args.h"

#define CHUNK_SIZE (1024 * 1024 * 10) // 10 MB chunks
#define DEFAULT_CHUNK_SIZE CHUNK_SIZE

int main(int argc, char **argv) {
    size_t chunk_size = bench_parse_size(argc, argv, DEFAULT_CHUNK_SIZE);
    char *buffer = (char*)malloc(chunk_size);
    // Fill buffer to prevent OS zero-page optimization
    for (size_t i = 0; i < chunk_size; i++) buffer[i] = (char)i;

    double start = bench_now_sec();
    double duration = bench_parse_duration(argc, argv, 60.0);
    
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
    remove("/tmp/test_io_file.bin");
    free(buffer);
    return 0;
}
