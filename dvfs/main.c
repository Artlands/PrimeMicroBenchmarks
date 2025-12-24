#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <likwid.h>

// Include your previous headers
#include "dvfs_config.h"
#include "dvfs_policy.h"
#include "model.h"
#include "config_loader.h"

// Define the Target CPU Core to monitor (e.g., Core 0)
// For a system-wide policy, you might want to loop over all cores or specific sockets.
#define TARGET_CPU_ID 0

// Define the Event String based on your HPC_DVFS_MODEL_INTEL group
// Note: Ensure these event names are exact matches for your specific architecture (e.g., Skylake, Icelake)
// You can check valid names with 'likwid-perfctr -e'
const char* EVENT_STRING = 
    "INSTR_RETIRED_ANY:FIXC0,"
    "CPU_CLK_UNHALTED_CORE:FIXC1,"
    "CPU_CLK_UNHALTED_REF:FIXC2,"
    "FP_ARITH_INST_RETIRED_512B_PACKED_DOUBLE:PMC0,"
    "FP_ARITH_INST_RETIRED_256B_PACKED_DOUBLE:PMC1,"
    "FP_ARITH_INST_RETIRED_SCALAR_DOUBLE:PMC2,"
    "L2_LINES_IN_ALL:PMC3,"
    "MEMORY_ACTIVITY_STALLS_L3_MISS:PMC4,"
    "BR_MISP_RETIRED_ALL_BRANCHES:PMC5";

static volatile sig_atomic_t keep_running = 1;

static void handle_signal(int sig) {
    (void)sig;
    keep_running = 0;
}

int main(int argc, char* argv[]) {
    int i, gid;
    double time_sec = 0.5; // 500ms

    (void)argc;
    (void)argv;
    
    // --- 1. Initialization ---
    load_config("dvfs_settings.conf");

    // Initialize Likwid topology and access
    topology_init();
    perfmon_init(1, (int[]){TARGET_CPU_ID}); // Monitor only TARGET_CPU_ID

    // Add the Event Set
    gid = perfmon_addEventSet((char*)EVENT_STRING);
    if (gid < 0) {
        fprintf(stderr, "Failed to add event set: %s\n", EVENT_STRING);
        return EXIT_FAILURE;
    }

    // Setup counters
    perfmon_setupCounters(gid);

    printf("[Main] DVFS Controller Started. Monitoring Core %d every %.1f s...\n", TARGET_CPU_ID, time_sec);

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // --- 2. The Control Loop ---
    while (keep_running) {
        // A. Start Counting
        perfmon_startCounters();

        // B. Wait for the sampling interval
        usleep((useconds_t)(time_sec * 1000000));

        // C. Stop Counting
        perfmon_stopCounters();
        perfmon_readCounters();

        // D. Read Raw Hardware Counters
        // We use likwid_getCmdLine helper or getResult directly.
        // indices correspond to the order in EVENT_STRING:
        // 0:FIXC0, 1:FIXC1, 2:FIXC2, 3:PMC0, ... 8:PMC5
        
        double events[9];
        for (i = 0; i < 9; i++) {
            events[i] = perfmon_getResult(gid, 0, i); // 0 = first core in list
        }

        // Map array to named variables for clarity
        double FIXC0 = events[0]; // INSTR_RETIRED
        double FIXC1 = events[1]; // CLK_CORE
        double FIXC2 = events[2]; // CLK_REF
        double PMC0  = events[3]; // FP_512
        double PMC1  = events[4]; // FP_256
        double PMC2  = events[5]; // FP_SCALAR
        double PMC3  = events[6]; // L2_LINES (BW)
        double PMC4  = events[7]; // L3_STALLS
        double PMC5  = events[8]; // BR_MISP

        // E. Compute Derived Metrics (Your Formulas)
        
        // Safety: Avoid division by zero
        if (FIXC0 == 0) FIXC0 = 1.0; 
        if (FIXC1 == 0) FIXC1 = 1.0;
        if (FIXC2 == 0) FIXC2 = 1.0;

        double CPI = FIXC1 / FIXC0;
        
        double Math_Intensity = (PMC0 * 8.0 + PMC1 * 4.0 + PMC2) / FIXC1;
        
        double Stall_Ratio = PMC4 / FIXC1;
        
        // BW Formula: Lines * 64 Bytes / time / 1e9
        double System_BW_Proxy = (PMC3 * 64.0) / time_sec / 1.0e9;
        
        double Branch_MPKI = (1000.0 * PMC5) / FIXC0;
        
        double GFLOPS_Approx = (PMC0 * 8.0 + PMC1 * 4.0 + PMC2) / time_sec / 1.0e9;
        
        double Clock_Ratio = FIXC1 / FIXC2;

        // F. Call the ML Controller
        apply_dvfs_policy(CPI, Math_Intensity, Stall_Ratio, System_BW_Proxy, Branch_MPKI, GFLOPS_Approx, Clock_Ratio);
    }

    // --- 3. Cleanup ---
    perfmon_finalize();
    topology_finalize();
    return 0;
}
