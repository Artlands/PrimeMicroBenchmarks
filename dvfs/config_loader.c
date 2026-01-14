#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dvfs_config.h"

// Set defaults
DvfsConfig global_dvfs_config = {
    .low_freq_khz = 800000,
    .med_freq_khz = 1800000,
    .high_freq_khz = 2500000
};

void load_config(const char* filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "[Config] Warning: Could not open %s. Using defaults.\n", filename);
        return;
    }

    char line[256];
    char key[50];
    unsigned long value;

    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#' || line[0] == '\n') continue;

        if (sscanf(line, "%49s %lu", key, &value) == 2) {
            if (strcmp(key, "LOW_FREQ_KHZ") == 0) {
                global_dvfs_config.low_freq_khz = value;
            } else if (strcmp(key, "MED_FREQ_KHZ") == 0) {
                global_dvfs_config.med_freq_khz = value;
            } else if (strcmp(key, "HIGH_FREQ_KHZ") == 0) {
                global_dvfs_config.high_freq_khz = value;
            }
        }
    }
    fclose(file);
    printf("[Config] Loaded: Low=%lu, Med=%lu, High=%lu (kHz)\n", 
           global_dvfs_config.low_freq_khz, 
           global_dvfs_config.med_freq_khz, 
           global_dvfs_config.high_freq_khz);
}