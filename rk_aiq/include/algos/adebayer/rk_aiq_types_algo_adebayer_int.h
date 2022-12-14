
#ifndef __RKAIQ_TYPES_ALGO_ADEBAYER_INT_H__
#define __RKAIQ_TYPES_ALGO_ADEBAYER_INT_H__

#include "rk_aiq_types_algo_adebayer.h"
#include "rk_aiq_types_adebayer_ext.h"
#include "debayer_head.h"

typedef struct AdebayerConfig_s {
    unsigned char enable;
    unsigned char filter_c_en;
    unsigned char filter_g_en;
    unsigned char gain_offset;
    unsigned short hf_offset;
    unsigned char thed1;
    unsigned char thed0;
    unsigned char dist_scale;
    unsigned char max_ratio;
    unsigned char clip_en;
    signed char filter1_coe[5];
    signed char filter2_coe[5];
    unsigned char offset;
    unsigned char shift_num;
    unsigned char order_max;
    unsigned char order_min;
    bool updatecfg;
} AdebayerConfig_t;

typedef struct AdebayerProcResult_s {
    AdebayerConfig_t config;
} AdebayerProcResult_t;

#endif //__RKAIQ_TYPES_ALGO_ADEBAYER_INT_H__