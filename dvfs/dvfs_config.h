// dvfs_config.h
#ifndef DVFS_CONFIG_H
#define DVFS_CONFIG_H

// 1. Abstract States returned by the ML model
typedef enum {
    FREQ_LEVEL_LOW = 0,
    FREQ_LEVEL_MED = 1,
    FREQ_LEVEL_HIGH = 2,
    FREQ_LEVEL_UNKNOWN = 99
} FreqLevel;

// 2. Struct to hold the User Configuration
typedef struct {
    unsigned long low_freq_khz;
    unsigned long med_freq_khz;
    unsigned long high_freq_khz;
} DvfsConfig;

// Global config instance (defined in config_loader.c)
extern DvfsConfig global_dvfs_config;

#endif // DVFS_CONFIG_H
