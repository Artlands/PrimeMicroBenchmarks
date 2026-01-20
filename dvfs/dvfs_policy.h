// dvfs_policy.h
#ifndef DVFS_POLICY_H
#define DVFS_POLICY_H

#include "dvfs_config.h" // Needed for FreqLevel enum

typedef FreqLevel (*model_predict_fn_t)(double CPI,
                                        double Math_Intensity,
                                        double Stall_Ratio,
                                        double System_BW_Proxy,
                                        double Branch_MPKI,
                                        double GFLOPS_Approx,
                                        double Clock_Ratio);

void dvfs_set_model_predictor(model_predict_fn_t fn);

/**
 * Applies the DVFS policy based on the provided profiling metrics.
 */
void apply_dvfs_policy(double CPI, 
                       double Math_Intensity, 
                       double Stall_Ratio, 
                       double System_BW_Proxy, 
                       double Branch_MPKI, 
                       double GFLOPS_Approx, 
                       double Clock_Ratio);
#endif // DVFS_POLICY_H
