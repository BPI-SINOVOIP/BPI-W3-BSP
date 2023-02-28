/******************************************************************************
 *
 * Copyright 2019, Fuzhou Rockchip Electronics Co.Ltd . All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Fuzhou Rockchip Electronics Co.Ltd .
 *
 *
 *****************************************************************************/
#ifndef __RK_AIQ_TYPES_ADRC_ALGO_PRVT_H__
#define __RK_AIQ_TYPES_ADRC_ALGO_PRVT_H__

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "ae/rk_aiq_types_ae_algo_int.h"
#include "af/rk_aiq_types_af_algo_int.h"
#include "rk_aiq_algo_types.h"
#include "xcam_log.h"
#include "rk_aiq_types_adrc_stat_v200.h"
#include "RkAiqCalibDbV2Helper.h"
#include "amerge/rk_aiq_types_amerge_algo_prvt.h"
#include "rk_aiq_types_adrc_algo_int.h"



#define DRCGAINMAX     (8)
#define DRCGAINMIN     (1)
#define CLIPMAX     (64.0)
#define CLIPMIN     (0.0)
#define MINOGAINMAX     (2.0)
#define MINOGAINMIN     (0.0)
#define ADRCNORMALIZEMAX     (1.0)
#define ADRCNORMALIZEMIN     (0.0)
#define ADRCNORMALIZEINTMAX     (1)
#define ADRCNORMALIZEINTMIN     (0)
#define SPACESGMMAX     (4095)
#define SPACESGMMIN     (0)
#define SCALEYMAX     (2048)
#define SCALEYMIN     (0)
#define MANUALCURVEMAX     (8192)
#define MANUALCURVEMIN     (0)
#define IIRFRAMEMAX     (1000)
#define IIRFRAMEMIN     (0)
#define INT8BITMAX     (255)
#define INT14BITMAX     (16383)
#define SW_DRC_OFFSET_POW2_FIX (8)
#define SW_DRC_MOTION_SCL_FIX (0)
#define SW_DRC_BILAT_WT_OFF_FIX (255)
#define MAX_AE_DRC_GAIN (256)

#define ISP_RAW_BIT (12)
#define MFHDR_LOG_Q_BITS (11)
#define DSTBITS (ISP_RAW_BIT << MFHDR_LOG_Q_BITS)
#define OFFSETBITS_INT (SW_DRC_OFFSET_POW2_FIX)
#define OFFSETBITS (OFFSETBITS_INT << MFHDR_LOG_Q_BITS)
#define VALIDBITS (DSTBITS - OFFSETBITS)
#define DELTA_SCALEIN_FIX ((256 << MFHDR_LOG_Q_BITS) / VALIDBITS)


typedef enum AdrcState_e {
    ADRC_STATE_INVALID       = 0,
    ADRC_STATE_INITIALIZED   = 1,
    ADRC_STATE_STOPPED       = 2,
    ADRC_STATE_RUNNING       = 3,
    ADRC_STATE_LOCKED        = 4,
    ADRC_STATE_MAX
} AdrcState_t;

typedef struct DrcExpoData_s {
    float nextLExpo;
    float nextMExpo;
    float nextSExpo;
    float nextRatioLS;
    float nextRatioLM;
} DrcExpoData_t;

typedef struct DrcHandleData_s {
    union {
        mdrcAttr_V21_t Drc_v21;
        mdrcAttr_V30_t Drc_v30;
    };
} DrcHandleData_t;

typedef struct AdrcPrevData_s
{
    float EnvLv;
    float ISO;
    float MotionCoef;
    drc_OpMode_t ApiMode;
    int frameCnt;
    DrcHandleData_t HandleData;
} AdrcPrevData_t;

typedef struct CurrAeResult_s {
    float MeanLuma[3];
    float GlobalEnvLv;
    float L2M_Ratio;
    float M2S_Ratio;
    float ISO;
    float AERatio;

    float Lv_fac;
    float DarkPdf_fac;
    float Contrast_fac;
    float BlockLumaS[225];
    float BlockLumaM[25];
    float BlockLumaL[225];
} CurrAeResult_t;

typedef struct CurrData_s
{
    bool Enable;
    float Ratio;
    float EnvLv;
    float MotionCoef;
    float ISO;
    float LumaWeight[225];
    int frameCnt;
    float damp;
    float ByPassThr;
    float Tolerance;
    DrcHandleData_t HandleData;
} CurrData_t;

typedef struct AdrcProcResData_s
{
    bool bDrcEn;
    DrcProcRes_t DrcProcRes;
} AdrcProcResData_t;

typedef struct AdrcSensorInfo_s
{
    bool  LongFrmMode;
    float HdrMinGain[MAX_HDR_FRAMENUM];
    float HdrMaxGain[MAX_HDR_FRAMENUM];
    float HdrMinIntegrationTime[MAX_HDR_FRAMENUM];
    float HdrMaxIntegrationTime[MAX_HDR_FRAMENUM];

    float MaxExpoL;
    float MinExpoL;
    float MaxExpoS;
    float MinExpoS;
} AdrcSensorInfo_t;

typedef struct pCalibData_ {
    union {
        CalibDbV2_drc_t* pCalibV21;
        CalibDbV2_drc_V2_t* pCalibV30;
    };
} pCalibData_t;

typedef struct AdrcContext_s
{
    drcAttr_t drcAttr;
    AdrcState_t state;
    pCalibData_t Calib;
    AdrcPrevData_t PrevData ;
    AdrcProcResData_t AdrcProcRes;
    CurrAeResult_t CurrAeResult;;
    CurrData_t CurrData;
    rkisp_adrc_stats_t CurrStatsData;
    AdrcSensorInfo_t SensorInfo;
    int frameCnt;
    FrameNumber_t FrameNumber;
} AdrcContext_t;

#endif
