// model.h
#ifndef MODEL_H
#define MODEL_H

#include "dvfs_config.h" // Needed for FreqLevel enum

/**
 * Predicts the execution phase level based on profiling metrics.
 * Returns: FREQ_LEVEL_LOW, FREQ_LEVEL_MED, or FREQ_LEVEL_HIGH
 */
FreqLevel predict_phase_level(double CPI, 
                              double Math_Intensity, 
                              double Stall_Ratio, 
                              double System_BW_Proxy, 
                              double Branch_MPKI, 
                              double GFLOPS_Approx, 
                              double Clock_Ratio);

#endif // MODEL_H