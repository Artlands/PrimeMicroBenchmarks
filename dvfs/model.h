// model.h
#ifndef MODEL_H
#define MODEL_H

#include "dvfs_config.h" // Needed for FreqLevel enum

/**
 * Predicts the execution phase level based on profiling metrics.
 * Returns: FREQ_LEVEL_LOW, FREQ_LEVEL_MED, or FREQ_LEVEL_HIGH
 */
FreqLevel predict_phase_level(double CPI, 
                              double Compute_Density, 
                              double Mem_Boundness, 
                              double Stall_Ratio, 
                              double Branch_MPKI, 
                              double Vector_Intensity);

#endif // MODEL_H