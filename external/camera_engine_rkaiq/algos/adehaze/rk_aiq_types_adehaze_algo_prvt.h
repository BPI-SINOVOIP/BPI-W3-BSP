/*
 *rk_aiq_types_adehaze_algo_prvt.h
 *
 *  Copyright (c) 2019 Rockchip Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef _RK_AIQ_TYPES_ADEHAZE_ALGO_PRVT_H_
#define _RK_AIQ_TYPES_ADEHAZE_ALGO_PRVT_H_

#include "RkAiqCalibDbTypes.h"
#include "RkAiqCalibDbTypesV2.h"
#include "adehaze/rk_aiq_types_adehaze_algo_int.h"
#include "amerge/rk_aiq_types_amerge_algo_prvt.h"
#include "rk_aiq_algo_types.h"
#include "rk_aiq_types_adehaze_stat.h"
#include "xcam_log.h"
#include "ynr_uapi_head_v3.h"

#define ADHZ10BITMAX     (1023)
#define ADHZ10BITMIN     (0)
#define DEHAZE_API_MANUAL_DEFAULT_LEVEL     (50)
#define DEHAZE_API_ENHANCE_MANUAL_DEFAULT_LEVEL     (50)

//define for dehaze local gain
#define YNR_BIT_CALIB (12)
#define YNR_ISO_CURVE_POINT_BIT             (4)
#define YNR_ISO_CURVE_SECT_VALUE   (1 << (YNR_BIT_CALIB - YNR_ISO_CURVE_POINT_BIT))
#define YNR_ISO_CURVE_SECT_VALUE1   (1 << YNR_BIT_CALIB)
#define YNR_CURVE_STEP             (16)

typedef enum YnrSnrMode_e {
    YNRSNRMODE_LSNR     = 0,
    YNRSNRMODE_HSNR     = 1,
} YnrSnrMode_t;

typedef struct AdehazeAePreResV20_s {
    float ISO;
    dehaze_api_mode_t ApiMode;
} AdehazeAePreResV20_t;

typedef struct AdehazeAePreResV21_s {
    float EnvLv;
    float ISO;
    YnrSnrMode_t SnrMode;
    dehaze_api_mode_t ApiMode;
} AdehazeAePreResV21_t;

typedef struct AdehazeAePreRes_s {
    union {
        AdehazeAePreResV20_t V20;
        AdehazeAePreResV21_t V21;
        AdehazeAePreResV21_t V30;
    };
} AdehazeAePreRes_t;

typedef struct AdehazeHandle_s {
    adehaze_sw_V2_t AdehazeAtrr;
    CalibDbV2_dehaze_V21_t* pCalib;
    RkAiqYnrV3Res YnrPorcRes;
    RkAiqAdehazeProcResult_t ProcRes;
    rkisp_adehaze_stats_t stats;
    AdehazeVersion_t HWversion;
    AdehazeAePreRes_t CurrData;
    AdehazeAePreRes_t PreData;
    bool byPassProc;
    bool is_multi_isp_mode;
    int width;
    int height;
    int strength;
    int working_mode;
    FrameNumber_t FrameNumber;
    int FrameID;
} AdehazeHandle_t;

#endif
