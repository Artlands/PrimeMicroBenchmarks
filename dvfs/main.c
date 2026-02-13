#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <likwid.h>
#include <limits.h>

// Include headers
#include "dvfs_config.h"
#include "dvfs_policy.h"
#include "model.h"
#include "config_loader.h"

#define predict_phase_level predict_phase_level_amdzen4c_edp
#include "model_amdzen4c_edp.c"
#undef predict_phase_level

#define predict_phase_level predict_phase_level_amdzen4c_energy
#include "model_amdzen4c_energy.c"
#undef predict_phase_level

#define predict_phase_level predict_phase_level_intelspr_edp
#include "model_intelspr_edp.c"
#undef predict_phase_level

#define predict_phase_level predict_phase_level_intelspr_energy
#include "model_intelspr_energy.c"
#undef predict_phase_level

// Define the CPU Core to monitor (e.g., Core 0).
// For a system-wide policy, may loop over all cores or specific sockets.
#define MONITOR_CPU_ID 0

// Define the CPU Core to run the controller on.
#define CONTROLLER_CPU_ID 0

// Define the Event Strings based on LIKWID perfgroups.
// Note: Ensure these event names are exact matches for the specific architecture.
// Check valid names with 'likwid-perfctr -e'.
const char* EVENT_STRING_INTEL =
    "INSTR_RETIRED_ANY:FIXC0,"
    "CPU_CLK_UNHALTED_CORE:FIXC1,"
    "CPU_CLK_UNHALTED_REF:FIXC2,"
    "TOPDOWN_SLOTS:FIXC3,"
    "BR_MISP_RETIRED_ALL_BRANCHES:PMC0,"
    "TOPDOWN_MEMORY_BOUND_SLOTS:PMC1,"
    "TOPDOWN_BACKEND_BOUND_SLOTS:PMC2,"
    "FP_ARITH_INST_RETIRED2_SCALAR:PMC3,"
    "FP_ARITH_INST_RETIRED2_VECTOR:PMC4";

const char* EVENT_STRING_AMD =
    "ACTUAL_CPU_CLOCK:FIXC1,"
    "MAX_CPU_CLOCK:FIXC2,"
    "RETIRED_INSTRUCTIONS:PMC0,"
    "RETIRED_SSE_AVX_FLOPS_ALL:PMC1,"
    "RETIRED_FP_OPS_BY_TYPE_SCALAR_ALL:PMC2,"
    "DEMAND_DATA_CACHE_FILLS_ALL:PMC3,"
    "RETIRED_MISP_BRANCH_INSTR:PMC4,"
    "DISPATCH_STALLS_PER_SLOT_BACKEND:PMC5";

// Performance group names (must exist in LIKWID perfgroups for this CPU).
#define PERF_GROUP_NAME_INTEL "HPC_DVFS_MODEL_INTEL"
#define PERF_GROUP_NAME_AMD "HPC_DVFS_MODEL_ZEN4"

static int is_amd_cpu(void) {
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) {
        return 0;
    }
    char line[256];
    int is_amd = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "vendor_id") && strstr(line, "AuthenticAMD")) {
            is_amd = 1;
            break;
        }
    }
    fclose(fp);
    return is_amd;
}

static int resolve_config_path(const char *requested, char *out, size_t out_size) {
    if (!requested || !out || out_size == 0) {
        return 0;
    }

    if (access(requested, R_OK) == 0) {
        snprintf(out, out_size, "%s", requested);
        return 1;
    }

    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len <= 0) {
        return 0;
    }
    exe_path[len] = '\0';

    char *last_slash = strrchr(exe_path, '/');
    if (!last_slash) {
        return 0;
    }
    *last_slash = '\0';

    size_t exe_len = strlen(exe_path);
    size_t req_len = strlen(requested);
    if (exe_len + 1 + req_len + 1 > out_size) {
        return 0;
    }
    memcpy(out, exe_path, exe_len);
    out[exe_len] = '/';
    memcpy(out + exe_len + 1, requested, req_len);
    out[exe_len + 1 + req_len] = '\0';
    if (access(out, R_OK) == 0) {
        return 1;
    }

    return 0;
}

static model_predict_fn_t get_model_fn(const char *name, int is_amd, const char **out_name) {
    struct model_entry {
        const char *name;
        model_predict_fn_t fn;
    };

    static const struct model_entry models[] = {
        {"amd_edp", predict_phase_level_amdzen4c_edp},
        {"amd_energy", predict_phase_level_amdzen4c_energy},
        {"intel_edp", predict_phase_level_intelspr_edp},
        {"intel_energy", predict_phase_level_intelspr_energy},
    };

    const char *default_name = is_amd ? "amd_energy" : "intel_energy";
    const char *selected = name ? name : default_name;
    size_t i;

    for (i = 0; i < sizeof(models) / sizeof(models[0]); i++) {
        if (strcmp(models[i].name, selected) == 0) {
            if (out_name) {
                *out_name = models[i].name;
            }
            return models[i].fn;
        }
    }

    return NULL;
}

static volatile sig_atomic_t keep_running = 1;

static void handle_signal(int sig) {
    (void)sig;
    keep_running = 0;
}

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    int i, gid;
    double time_sec = 0.5; // 500ms
    int monitor_cpu_id = MONITOR_CPU_ID;
    int controller_cpu_id = CONTROLLER_CPU_ID;
    int debug = 0;
    const char *config_path = "dvfs/dvfs_settings.conf";
    int config_path_set = 0;
    const char *model_name = NULL;
    int is_amd = 0;
    char resolved_config_path[PATH_MAX];

    int opt;
    while ((opt = getopt(argc, argv, "m:c:df:p:t:")) != -1) {
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
            case 'f':
                config_path = optarg;
                config_path_set = 1;
                break;
            case 'p':
                model_name = optarg;
                break;
            case 't': {
                char *end = NULL;
                double parsed = strtod(optarg, &end);
                if (!end || *end != '\0' || parsed <= 0.0) {
                    fprintf(stderr, "Invalid time_sec value: %s\n", optarg);
                    return EXIT_FAILURE;
                }
                time_sec = parsed;
                break;
            }
            default:
                fprintf(stderr, "Usage: %s [-m monitor_cpu] [-c controller_cpu] [-f config] [-p model] [-t time_sec] [-d]\n", argv[0]);
                return EXIT_FAILURE;
        }
    }
    
    is_amd = is_amd_cpu();
    if (!config_path_set) {
        config_path = is_amd ? "dvfs_amd_settings.conf"
                             : "dvfs_intel_settings.conf";
    }
    if (resolve_config_path(config_path, resolved_config_path, sizeof(resolved_config_path))) {
        config_path = resolved_config_path;
    }

    // --- 1. Initialization ---
    load_config(config_path);

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

    // Pin controller and register only the monitored CPU for perfmon.
    affinity_pinThread(controller_cpu_id);
    int threads_count = 1;
    int apic_ids[1];
    int monitor_apic = (int)topo->threadPool[monitor_cpu_id].apicId;
    apic_ids[0] = monitor_apic;
    if (perfmon_init(threads_count, apic_ids) < 0) {
        fprintf(stderr, "perfmon_init failed\n");
        return EXIT_FAILURE;
    }

    int monitor_thread_idx = 0;

    // Add the Event Set (prefer group file for metric definitions)
    perfmon_check_counter_map(monitor_cpu_id);
    int use_metrics = 0;
    int event_count = is_amd ? 8 : 9;
    const char *perf_group_name = is_amd ? PERF_GROUP_NAME_AMD : PERF_GROUP_NAME_INTEL;
    const char *event_string = is_amd ? EVENT_STRING_AMD : EVENT_STRING_INTEL;
    const char *selected_model_name = NULL;
    model_predict_fn_t selected_model = get_model_fn(model_name, is_amd, &selected_model_name);
    if (!selected_model) {
        fprintf(stderr, "Unknown model '%s'. Valid options: amd_edp, amd_energy, intel_edp, intel_energy\n",
                model_name ? model_name : "");
        return EXIT_FAILURE;
    }
    dvfs_set_model_predictor(selected_model);
    if (debug) {
        printf("[Main] Model selected: %s\n", selected_model_name);
    }
    const char *group_path = getenv("LIKWID_GROUPPATH");
    if (group_path && *group_path) {
        CpuInfo_t cpu_info = get_cpuInfo();
        GroupInfo ginfo;
        perfgroup_new(&ginfo);
        int grp_ret = perfgroup_readGroup(group_path, cpu_info->short_name,
                                          perf_group_name, &ginfo);
        if (grp_ret != 0) {
            fprintf(stderr, "perfgroup_readGroup failed (ret=%d path=%s arch=%s group=%s)\n",
                    grp_ret, group_path, cpu_info->short_name, perf_group_name);
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
        gid = perfmon_addEventSet((char*)event_string);
    }
    if (gid < 0) {
        fprintf(stderr, "Failed to add event set\n");
        return EXIT_FAILURE;
    }

    // Setup counters
    if (perfmon_setupCounters(gid) < 0) {
        fprintf(stderr, "perfmon_setupCounters failed\n");
        return EXIT_FAILURE;
    }

    int metric_offset = 0;
    if (use_metrics) {
        int metric_count = perfmon_getNumberOfMetrics(gid);
        if (metric_count < 6) {
            if (debug) {
                fprintf(stderr, "Group metrics not available, falling back to raw counters\n");
            }
            use_metrics = 0;
        } else {
            // If Runtime [s] is provided as the first metric, skip it.
            if (is_amd) {
                metric_offset = (metric_count >= 8) ? 1 : 0;
            } else {
                metric_offset = (metric_count >= 7) ? 1 : 0;
            }
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
        usleep((useconds_t)(time_sec * 1000000));

        // C. Stop Counting
        ret = perfmon_stopCounters();
        if (ret < 0) {
            fprintf(stderr, "perfmon_stopCounters failed (ret=%d)\n", ret);
            break;
        }

        // D. Read Raw Hardware Counters
        // We use likwid_getCmdLine helper or getResult directly.
        // indices correspond to the order in EVENT_STRING.
        // Intel: 0:FIXC0 1:FIXC1 2:FIXC2 3:FIXC3 4:PMC0 5:PMC1 6:PMC2 7:PMC3 8:PMC4
        // AMD:   0:FIXC1 1:FIXC2 2:PMC0 3:PMC1 4:PMC2 5:PMC3 6:PMC4 7:PMC5
        double CPI, Compute_Density, Mem_Boundness, Stall_Ratio;
        double Branch_MPKI, Vector_Intensity, Clock_Ratio;
        double events[9];
        for (i = 0; i < event_count; i++) {
            events[i] = perfmon_getLastResult(gid, i, monitor_thread_idx);
        }

        if (use_metrics) {
            CPI = perfmon_getMetric(gid, metric_offset + 0, monitor_thread_idx);
            Compute_Density = perfmon_getMetric(gid, metric_offset + 1, monitor_thread_idx);
            Mem_Boundness = perfmon_getMetric(gid, metric_offset + 2, monitor_thread_idx);
            Stall_Ratio = perfmon_getMetric(gid, metric_offset + 3, monitor_thread_idx);
            Branch_MPKI = perfmon_getMetric(gid, metric_offset + 4, monitor_thread_idx);
            Vector_Intensity = perfmon_getMetric(gid, metric_offset + 5, monitor_thread_idx);
            if (is_amd) {
                double FIXC1 = events[0];
                double FIXC2 = events[1];
                if (FIXC2 == 0) FIXC2 = 1.0;
                Clock_Ratio = FIXC1 / FIXC2;
            } else {
                double FIXC1 = events[1];
                double FIXC2 = events[2];
                if (FIXC2 == 0) FIXC2 = 1.0;
                Clock_Ratio = FIXC1 / FIXC2;
            }
        } else {
            if (is_amd) {
                // Map array to named variables for clarity (AMD Zen4)
                double FIXC1 = events[0]; // ACTUAL_CPU_CLOCK
                double FIXC2 = events[1]; // MAX_CPU_CLOCK
                double PMC0  = events[2]; // RETIRED_INSTRUCTIONS
                double PMC1  = events[3]; // RETIRED_SSE_AVX_FLOPS_ALL
                double PMC2  = events[4]; // RETIRED_FP_OPS_BY_TYPE_SCALAR_ALL
                double PMC3  = events[5]; // DEMAND_DATA_CACHE_FILLS_ALL
                double PMC4  = events[6]; // RETIRED_MISP_BRANCH_INSTR
                double PMC5  = events[7]; // DISPATCH_STALLS_PER_SLOT_BACKEND

                // Safety: Avoid division by zero
                if (FIXC1 == 0) FIXC1 = 1.0;
                if (FIXC2 == 0) FIXC2 = 1.0;
                if (PMC0 == 0) PMC0 = 1.0;

                // [CPI, Compute_Density, Mem_Boundness, Stall_Ratio, Branch_MPKI, Vector_Intensity]
                CPI = FIXC1 / PMC0;
                Compute_Density = (PMC1 + PMC2) / FIXC1;
                Mem_Boundness = PMC3 / PMC0;
                Stall_Ratio = PMC5 / (6.0 * FIXC1);
                Branch_MPKI = (1000.0 * PMC4) / PMC0;
                Vector_Intensity = PMC1 / (PMC1 + PMC2 + 1.0e-9);
                Clock_Ratio = FIXC1 / FIXC2;
            } else {
                // Map array to named variables for clarity (Intel Sapphire Rapids)
                double FIXC0 = events[0]; // INSTR_RETIRED
                double FIXC1 = events[1]; // CLK_CORE
                double FIXC2 = events[2]; // CLK_REF
                double FIXC3 = events[3]; // TOPDOWN_SLOTS
                double PMC0  = events[4]; // BR_MISP_RETIRED_ALL_BRANCHES
                double PMC1  = events[5]; // TOPDOWN_MEMORY_BOUND_SLOTS
                double PMC2  = events[6]; // TOPDOWN_BACKEND_BOUND_SLOTS
                double PMC3  = events[7]; // FP_ARITH_INST_RETIRED2_SCALAR
                double PMC4  = events[8]; // FP_ARITH_INST_RETIRED2_VECTOR

                // Safety: Avoid division by zero
                if (FIXC0 == 0) FIXC0 = 1.0;
                if (FIXC1 == 0) FIXC1 = 1.0;
                if (FIXC2 == 0) FIXC2 = 1.0;
                if (FIXC3 == 0) FIXC3 = 1.0;

                CPI = FIXC1 / FIXC0;
                Compute_Density = (PMC3 + PMC4) / FIXC0;
                Mem_Boundness = PMC1 / FIXC3;
                Stall_Ratio = PMC2 / FIXC3;
                Branch_MPKI = (1000.0 * PMC0) / FIXC0;
                Vector_Intensity = PMC4 / (PMC3 + PMC4 + 1.0e-9);
                Clock_Ratio = FIXC1 / FIXC2;
            }
        }
        
        if (debug) {
            if (!use_metrics) {
                if (is_amd) {
                    printf("[RAW] FIXC1=%.0f FIXC2=%.0f PMC0=%.0f PMC1=%.0f PMC2=%.0f PMC3=%.0f PMC4=%.0f PMC5=%.0f\n",
                           events[0], events[1], events[2], events[3], events[4],
                           events[5], events[6], events[7]);
                } else {
                    printf("[RAW] FIXC0=%.0f FIXC1=%.0f FIXC2=%.0f FIXC3=%.0f PMC0=%.0f PMC1=%.0f PMC2=%.0f PMC3=%.0f PMC4=%.0f\n",
                           events[0], events[1], events[2], events[3], events[4],
                           events[5], events[6], events[7], events[8]);
                }
            }
            printf("[DEBUG] Core %d | CPI: %.8f | Compute_Density: %.8f | Mem_Boundness: %.8f | Stall_Ratio: %.8f | Branch_MPKI: %.8f | Vector_Intensity: %.8f | Clock_Ratio: %.8f\n",
                   monitor_cpu_id, CPI, Compute_Density, Mem_Boundness, Stall_Ratio,
                   Branch_MPKI, Vector_Intensity, Clock_Ratio);
        }

        // F. Call the ML Controller
        // apply_dvfs_policy(CPI,
        //                   Compute_Density,
        //                   Stall_Ratio,
        //                   Mem_Boundness,
        //                   Branch_MPKI,
        //                   Vector_Intensity,
        //                   Clock_Ratio);
    }

    // --- 3. Cleanup ---
    perfmon_finalize();
    affinity_finalize();
    numa_finalize();
    topology_finalize();
    return 0;
}
