#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
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

// Performance group name (must exist in LIKWID perfgroups for this CPU).
#define PERF_GROUP_NAME "HPC_DVFS_MODEL_INTEL"

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
    int debug = 0;

    int opt;
    while ((opt = getopt(argc, argv, "m:c:d")) != -1) {
        switch (opt) {
            case 'm':
                monitor_cpu_id = atoi(optarg);
                break;
            case 'c':
                controller_cpu_id = atoi(optarg);
                break;
            case 'd':
                debug = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-m monitor_cpu] [-c controller_cpu] [-d]\n", argv[0]);
                return EXIT_FAILURE;
        }
    }
    
    // --- 1. Initialization ---
    load_config("dvfs_settings.conf");

    // Initialize Likwid topology and access
    HPMmode(ACCESSMODE_DIRECT);
    if (HPMinit() < 0) {
        fprintf(stderr, "HPMinit failed\n");
        return EXIT_FAILURE;
    }
    topology_init();
    numa_init();
    affinity_init();
    if (perfmon_init_maps() < 0) {
        fprintf(stderr, "perfmon_init_maps failed\n");
        return EXIT_FAILURE;
    }

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
    int threads_count = num_threads;
    int *apic_ids = malloc((size_t)threads_count * sizeof(*apic_ids));
    if (!apic_ids) {
        fprintf(stderr, "Failed to allocate apic_ids\n");
        return EXIT_FAILURE;
    }
    for (i = 0; i < threads_count; i++) {
        apic_ids[i] = (int)topo->threadPool[i].apicId;
    }
    if (perfmon_init(threads_count, apic_ids) < 0) {
        fprintf(stderr, "perfmon_init failed\n");
        free(apic_ids);
        return EXIT_FAILURE;
    }

    int monitor_thread_idx = -1;
    int monitor_apic = (int)topo->threadPool[monitor_cpu_id].apicId;
    for (i = 0; i < threads_count; i++) {
        if (apic_ids[i] == monitor_apic) {
            monitor_thread_idx = i;
            break;
        }
    }
    if (monitor_thread_idx < 0) {
        fprintf(stderr, "Failed to map monitor CPU %d to thread index\n",
                monitor_cpu_id);
        free(apic_ids);
        return EXIT_FAILURE;
    }

    // Add the Event Set (prefer group file for metric definitions)
    perfmon_check_counter_map(monitor_cpu_id);
    int use_metrics = 0;
    const char *group_path = getenv("LIKWID_GROUPPATH");
    if (group_path && *group_path) {
        CpuInfo_t cpu_info = get_cpuInfo();
        GroupInfo ginfo;
        perfgroup_new(&ginfo);
        int grp_ret = perfgroup_readGroup(group_path, cpu_info->short_name,
                                          PERF_GROUP_NAME, &ginfo);
        if (grp_ret != 0) {
            fprintf(stderr, "perfgroup_readGroup failed (ret=%d path=%s arch=%s group=%s)\n",
                    grp_ret, group_path, cpu_info->short_name, PERF_GROUP_NAME);
        } else {
            char *event_str = perfgroup_getEventStr(&ginfo);
            if (event_str) {
                gid = perfmon_addEventSet(event_str);
                perfgroup_returnEventStr(event_str);
                use_metrics = 1;
            }
        }
        perfgroup_returnGroup(&ginfo);
    }
    if (!use_metrics) {
        gid = perfmon_addEventSet((char*)EVENT_STRING);
    }
    if (gid < 0) {
        fprintf(stderr, "Failed to add event set\n");
        free(apic_ids);
        return EXIT_FAILURE;
    }

    // Setup counters
    if (perfmon_setupCounters(gid) < 0) {
        fprintf(stderr, "perfmon_setupCounters failed\n");
        return EXIT_FAILURE;
    }

    if (use_metrics) {
        int metric_count = perfmon_getNumberOfMetrics(gid);
        if (metric_count < 7) {
            if (debug) {
                fprintf(stderr, "Group metrics not available, falling back to raw counters\n");
            }
            use_metrics = 0;
        }
    } else if (debug) {
        fprintf(stderr, "Group metrics not available, falling back to raw counters\n");
    }

    printf("[Main] DVFS Controller Started. Monitoring Core %d every %.1f s...\n",
           monitor_cpu_id, time_sec);

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // --- 2. The Control Loop ---
    while (keep_running) {
        // A. Start Counting
        int ret = perfmon_startCounters();
        if (ret < 0) {
            fprintf(stderr, "perfmon_startCounters failed (ret=%d)\n", ret);
            break;
        }

        // B. Wait for the sampling interval
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        usleep((useconds_t)(time_sec * 1000000));
        clock_gettime(CLOCK_MONOTONIC, &t1);

        // C. Stop Counting
        ret = perfmon_stopCounters();
        if (ret < 0) {
            fprintf(stderr, "perfmon_stopCounters failed (ret=%d)\n", ret);
            break;
        }

        // D. Read Raw Hardware Counters
        // We use likwid_getCmdLine helper or getResult directly.
        // indices correspond to the order in EVENT_STRING:
        // 0:FIXC0, 1:FIXC1, 2:FIXC2, 3:PMC0, ... 8:PMC5
        
        double CPI, Math_Intensity, Stall_Ratio, System_BW_Proxy;
        double Branch_MPKI, GFLOPS_Approx, Clock_Ratio;

        if (use_metrics) {
            CPI = perfmon_getMetric(gid, 0, monitor_thread_idx);
            Math_Intensity = perfmon_getMetric(gid, 1, monitor_thread_idx);
            Stall_Ratio = perfmon_getMetric(gid, 2, monitor_thread_idx);
            System_BW_Proxy = perfmon_getMetric(gid, 3, monitor_thread_idx);
            Branch_MPKI = perfmon_getMetric(gid, 4, monitor_thread_idx);
            GFLOPS_Approx = perfmon_getMetric(gid, 5, monitor_thread_idx);
            Clock_Ratio = perfmon_getMetric(gid, 6, monitor_thread_idx);
        } else {
            double events[9];
            for (i = 0; i < 9; i++) {
                events[i] = perfmon_getLastResult(gid, i, monitor_thread_idx);
            }

            double dt = (double)(t1.tv_sec - t0.tv_sec) +
                        (double)(t1.tv_nsec - t0.tv_nsec) / 1.0e9;
            if (dt <= 0.0) dt = time_sec;

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

            // Safety: Avoid division by zero
            if (FIXC0 == 0) FIXC0 = 1.0;
            if (FIXC1 == 0) FIXC1 = 1.0;
            if (FIXC2 == 0) FIXC2 = 1.0;

            CPI = FIXC1 / FIXC0;
            Math_Intensity = (PMC0 * 8.0 + PMC1 * 4.0 + PMC2) / FIXC1;
            Stall_Ratio = PMC4 / FIXC1;
            System_BW_Proxy = (PMC3 * 64.0) / dt / 1.0e9;
            Branch_MPKI = (1000.0 * PMC5) / FIXC0;
            GFLOPS_Approx = (PMC0 * 8.0 + PMC1 * 4.0 + PMC2) / dt / 1.0e9;
            Clock_Ratio = FIXC1 / FIXC2;
        }
        
        if (debug) {
            if (!use_metrics) {
                double events[9];
                for (i = 0; i < 9; i++) {
                    events[i] = perfmon_getLastResult(gid, i, monitor_thread_idx);
                }
                printf("[RAW] FIXC0=%.0f FIXC1=%.0f FIXC2=%.0f PMC0=%.0f PMC1=%.0f PMC2=%.0f PMC3=%.0f PMC4=%.0f PMC5=%.0f\n",
                       events[0], events[1], events[2], events[3], events[4],
                       events[5], events[6], events[7], events[8]);
            }
            printf("[DEBUG] Core %d | CPI: %.8f | MathInt: %.8f | Stall: %.8f | System_BW_Proxy: %.8f | Branch_MPKI: %.8f | GFLOPS: %.8f | Clock_Ratio: %.8f\n",
                   monitor_cpu_id, CPI, Math_Intensity, Stall_Ratio, System_BW_Proxy,
                   Branch_MPKI, GFLOPS_Approx, Clock_Ratio);
        }

        // F. Call the ML Controller
        apply_dvfs_policy(CPI, Math_Intensity, Stall_Ratio, System_BW_Proxy, Branch_MPKI, GFLOPS_Approx, Clock_Ratio);
    }

    // --- 3. Cleanup ---
    free(apic_ids);
    perfmon_finalize();
    affinity_finalize();
    numa_finalize();
    topology_finalize();
    return 0;
}
