#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <likwid.h>

// Include headers
#include "dvfs_config.h"
#include "dvfs_policy.h"
#include "model.h"
#include "config_loader.h"

// Define the CPU Core to monitor (e.g., Core 0).
// For a system-wide policy, may loop over all cores or specific sockets.
#define MONITOR_CPU_ID 0

// Define the CPU Core to run the controller on.
#define CONTROLLER_CPU_ID 0

// Define the Event String based on the HPC_DVFS_MODEL_INTEL group
// Note: Ensure these event names are exact matches for the specific architecture
// Check valid names with 'likwid-perfctr -e'
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
    int monitor_cpu_id = MONITOR_CPU_ID;
    int controller_cpu_id = CONTROLLER_CPU_ID;

    int opt;
    while ((opt = getopt(argc, argv, "m:c:")) != -1) {
        switch (opt) {
            case 'm':
                monitor_cpu_id = atoi(optarg);
                break;
            case 'c':
                controller_cpu_id = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-m monitor_cpu] [-c controller_cpu]\n", argv[0]);
                return EXIT_FAILURE;
        }
    }
    
    // --- 1. Initialization ---
    load_config("dvfs_settings.conf");

    // Initialize Likwid topology and access
    topology_init();
    affinity_init();

    CpuTopology_t topo = get_cpuTopology();
    int num_threads = (int)topo->activeHWThreads;
    if (monitor_cpu_id < 0 || monitor_cpu_id >= num_threads) {
        fprintf(stderr, "Invalid MONITOR_CPU_ID=%d (active HW threads=%u)\n",
                monitor_cpu_id, topo->activeHWThreads);
        return EXIT_FAILURE;
    }
    if (controller_cpu_id < 0 || controller_cpu_id >= num_threads) {
        fprintf(stderr, "Invalid CONTROLLER_CPU_ID=%d (active HW threads=%u)\n",
                controller_cpu_id, topo->activeHWThreads);
        return EXIT_FAILURE;
    }

    // Pin controller and register all CPUs for perfmon.
    affinity_pinThread(controller_cpu_id);
    int *threads_to_cpu = malloc((size_t)num_threads * sizeof(*threads_to_cpu));
    if (!threads_to_cpu) {
        fprintf(stderr, "Failed to allocate threads_to_cpu\n");
        return EXIT_FAILURE;
    }
    for (i = 0; i < num_threads; i++) {
        threads_to_cpu[i] = i;
    }
    perfmon_init(num_threads, threads_to_cpu);

    int monitor_thread_idx = monitor_cpu_id;

    // Add the Event Set
    gid = perfmon_addEventSet((char*)EVENT_STRING);
    if (gid < 0) {
        fprintf(stderr, "Failed to add event set: %s\n", EVENT_STRING);
        return EXIT_FAILURE;
    }

    // Setup counters
    perfmon_setupCounters(gid);

    printf("[Main] DVFS Controller Started. Monitoring Core %d every %.1f s...\n",
           monitor_cpu_id, time_sec);

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
            events[i] = perfmon_getResult(gid, monitor_thread_idx, i);
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
    free(threads_to_cpu);
    perfmon_finalize();
    affinity_finalize();
    topology_finalize();
    return 0;
}
