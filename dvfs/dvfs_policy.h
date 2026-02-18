// dvfs_policy.h
#ifndef DVFS_POLICY_H
#define DVFS_POLICY_H

#include "dvfs_config.h" // Needed for FreqLevel enum

typedef FreqLevel (*model_predict_fn_t)(double CPI,
                                        double Compute_Density,
                                        double Mem_Boundness,
                                        double Stall_Ratio,
                                        double Branch_MPKI,
                                        double Vector_Intensity);



void dvfs_set_model_predictor(model_predict_fn_t fn);
void dvfs_set_debug(int enabled);

/**
 * Applies the DVFS policy based on the provided profiling metrics.
 */
void apply_dvfs_policy(double CPI, 
                       double Compute_Density, 
                       double Mem_Boundness, 
                       double Stall_Ratio, 
                       double Branch_MPKI, 
                       double Vector_Intensity);
#endif // DVFS_POLICY_H
