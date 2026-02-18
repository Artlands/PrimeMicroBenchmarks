#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <likwid.h>

#include "dvfs_config.h"
#include "model.h"
#include "dvfs_policy.h"


static unsigned long current_freq_khz = 0;
static model_predict_fn_t predict_fn = NULL;
static int debug_enabled = 0;

void dvfs_set_model_predictor(model_predict_fn_t fn) {
    if (fn) {
        predict_fn = fn;
    }
}

void dvfs_set_debug(int enabled) {
    debug_enabled = enabled ? 1 : 0;
}

void apply_dvfs_policy(double CPI, double Compute_Density, double Mem_Boundness, double Stall_Ratio, double Branch_MPKI, double Vector_Intensity) {
    
    // 1. Get the Abstract Level from ML Model
    if (!predict_fn) {
        fprintf(stderr, "[DVFS] Model predictor not set\n");
        return;
    }
    struct timespec t0, t1;
    if (debug_enabled) {
        (void)clock_gettime(CLOCK_MONOTONIC, &t0);
    }
    FreqLevel level = predict_fn(CPI, Compute_Density, Mem_Boundness, Stall_Ratio, Branch_MPKI, Vector_Intensity);
    if (debug_enabled) {
        (void)clock_gettime(CLOCK_MONOTONIC, &t1);
        long long dt_ns = (long long)(t1.tv_sec - t0.tv_sec) * 1000000000LL +
                          (long long)(t1.tv_nsec - t0.tv_nsec);
        printf("[DVFS][DEBUG] predict_fn overhead: %lld ns (%.3f us)\n",
               dt_ns, (double)dt_ns / 1000.0);
    }
    
    // 2. Map Level to Configured Frequency
    unsigned long target_freq_khz = 0;
    
    switch (level) {
        case FREQ_LEVEL_LOW:
            target_freq_khz = global_dvfs_config.low_freq_khz;
            break;
        case FREQ_LEVEL_MED:
            target_freq_khz = global_dvfs_config.med_freq_khz;
            break;
        case FREQ_LEVEL_HIGH:
        default:
            target_freq_khz = global_dvfs_config.high_freq_khz;
            break;
    }

    // 3. Apply Frequency (Same logic as before)
    if (target_freq_khz == current_freq_khz) return;

    char cmd[512];
    // Note: Assuming 'cpupower' is available and configured
    snprintf(cmd, sizeof(cmd), "cpupower frequency-set -f %lu > /dev/null 2>&1", target_freq_khz);

    int ret = system(cmd);
    if (ret == 0) {
        printf("[DVFS] Switching to Level %d (%lu kHz)\n", level, target_freq_khz);
        current_freq_khz = target_freq_khz;
    } else {
        fprintf(stderr, "[DVFS] Failed to set frequency to %lu kHz (ret=%d)\n", target_freq_khz, ret);
    }
}
