/******************************************************************************
 *
 * Copyright 2019, Fuzhou Rockchip Electronics Co.Ltd. All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Fuzhou Rockchip Electronics Co.Ltd .
 *
 *
 *****************************************************************************/
/**
 * @file adrc.cpp
 *
 * @brief
 *   ADD_DESCRIPTION_HERE
 *
 *****************************************************************************/
#include "math.h"
//#include "rk_aiq_types_adrc_algo_int.h"
#include "xcam_log.h"
#include "rk_aiq_types_adrc_algo_prvt.h"

/******************************************************************************
 * AdrcStart()
 *****************************************************************************/
XCamReturn AdrcStart
(
    AdrcContext_t* pAdrcCtx
) {

    LOG1_ATMO( "%s:enter!\n", __FUNCTION__);

    // initial checks
    if (pAdrcCtx == NULL) {
        return (XCAM_RETURN_ERROR_MEM);
    }

    if ((ADRC_STATE_RUNNING == pAdrcCtx->state)
            || (ADRC_STATE_LOCKED == pAdrcCtx->state)) {
        return (XCAM_RETURN_ERROR_PARAM);
    }

    pAdrcCtx->state = ADRC_STATE_RUNNING;

    LOG1_ATMO( "%s:exit!\n", __FUNCTION__);
    return (XCAM_RETURN_NO_ERROR);
}

/******************************************************************************
 * DrcGetCurrPara()
 *****************************************************************************/
float DrcGetCurrPara
(
    float           inPara,
    float*         inMatrixX,
    float*         inMatrixY,
    int Max_Knots
) {
    LOG1_ATMO( "%s:enter!\n", __FUNCTION__);
    float x1 = 0.0f;
    float x2 = 0.0f;
    float value1 = 0.0f;
    float value2 = 0.0f;
    float outPara = 0.0f;

    if(inPara < inMatrixX[0])
        outPara = inMatrixY[0];
    else if (inPara >= inMatrixX[Max_Knots - 1])
        outPara = inMatrixY[Max_Knots - 1];
    else
        for(int i = 0; i < Max_Knots - 1; i++)
        {
            if(inPara >= inMatrixX[i] && inPara < inMatrixX[i + 1])
            {
                x1 = inMatrixX[i];
                x2 = inMatrixX[i + 1];
                value1 = inMatrixY[i];
                value2 = inMatrixY[i + 1];
                outPara = value1 + (inPara - x1) * (value1 - value2) / (x1 - x2);
                break;
            }
            else
                continue;
        }

    return outPara;
    LOG1_ATMO( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * DrcGetCurrPara()
 *****************************************************************************/
int DrcGetCurrParaInt
(
    float           inPara,
    float*         inMatrixX,
    int*         inMatrixY,
    int Max_Knots
) {
    LOG1_ATMO( "%s:enter!\n", __FUNCTION__);
    float x1 = 0.0f;
    float x2 = 0.0f;
    float value1 = 0.0f;
    float value2 = 0.0f;
    int outPara = 0;

    if(inPara < inMatrixX[0])
        outPara = inMatrixY[0];
    else if (inPara >= inMatrixX[Max_Knots - 1])
        outPara = inMatrixY[Max_Knots - 1];
    else
        for(int i = 0; i < Max_Knots - 1; i++)
        {
            if(inPara >= inMatrixX[i] && inPara < inMatrixX[i + 1])
            {
                x1 = inMatrixX[i];
                x2 = inMatrixX[i + 1];
                value1 = inMatrixY[i];
                value2 = inMatrixY[i + 1];
                outPara = (int)(value1 + (inPara - x1) * (value1 - value2) / (x1 - x2));
                break;
            }
            else
                continue;
        }

    return outPara;
    LOG1_ATMO( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AdrcStop()
 *****************************************************************************/
XCamReturn AdrcStop
(
    AdrcContext_t* pAdrcCtx
) {

    LOG1_ATMO( "%s:enter!\n", __FUNCTION__);

    // initial checks
    if (pAdrcCtx == NULL) {
        return (XCAM_RETURN_ERROR_MEM);
    }

    // before stopping, unlock the AHDR if locked
    if (ADRC_STATE_LOCKED == pAdrcCtx->state) {
        return (XCAM_RETURN_ERROR_PARAM);
    }

    pAdrcCtx->state = ADRC_STATE_STOPPED;

    LOG1_ATMO( "%s:exit!\n", __FUNCTION__);

    return (XCAM_RETURN_NO_ERROR);
}

/******************************************************************************
* CalibrateDrcGainY()
*****************************************************************************/
void CalibrateDrcGainY( DrcProcRes_t *para, float DraGain, float alpha)
{
    LOG1_ATMO("%s:Enter!\n", __FUNCTION__);

    float tmp = 0;
    float luma[17] = { 0, 256, 512, 768, 1024, 1280, 1536, 1792, 2048, 2304, 2560, 2816, 3072, 3328, 3584, 3840, 4096 };
    float gainTable[17];

    for(int i = 0; i < 17; ++i) {
        tmp = luma[i];
        tmp = 1 - alpha * pow((1 - tmp / 4096), 2);
        gainTable[i] = 1024 * pow(DraGain, tmp);
        if(CHECK_ISP_HW_V21())
            para->Drc_v21.sw_drc_gain_y[i] = (int)(gainTable[i]) ;
        else if(CHECK_ISP_HW_V30())
            para->Drc_v30.gain_y[i] = (int)(gainTable[i]) ;
    }

    LOG1_ATMO("%s:Eixt!\n", __FUNCTION__);
}

/******************************************************************************
 * ConfigV21()
 *set default Config data
 *****************************************************************************/
void ConfigV21
(
    AdrcContext_t*           pAdrcCtx
) {
    LOGI_ATMO( "%s:enter!\n", __FUNCTION__);

    // initial checks
    DCT_ASSERT(pAdrcCtx != NULL);

    //config default PrevData data
    pAdrcCtx->PrevData.frameCnt = 0;
    pAdrcCtx->PrevData.EnvLv = 0;
    pAdrcCtx->PrevData.ApiMode                                                  = DRC_OPMODE_AUTO;
    pAdrcCtx->PrevData.HandleData.Drc_v21.DrcGain.DrcGain                       = 1.0;
    pAdrcCtx->PrevData.HandleData.Drc_v21.DrcGain.Alpha                         = 0.0;
    pAdrcCtx->PrevData.HandleData.Drc_v21.DrcGain.Clip                          = 16.0;
    pAdrcCtx->PrevData.HandleData.Drc_v21.HiLit.Strength                        = 0.0;
    pAdrcCtx->PrevData.HandleData.Drc_v21.LocalSetting.LocalData.LocalWeit      = 1.00;
    pAdrcCtx->PrevData.HandleData.Drc_v21.LocalSetting.LocalData.GlobalContrast = 0;
    pAdrcCtx->PrevData.HandleData.Drc_v21.LocalSetting.LocalData.LoLitContrast  = 0;

    //set default value for api
    pAdrcCtx->drcAttr.stManualV21.Enable = true;
    pAdrcCtx->drcAttr.stManualV21.DrcGain.Alpha = 0.1;
    pAdrcCtx->drcAttr.stManualV21.DrcGain.DrcGain = 1;
    pAdrcCtx->drcAttr.stManualV21.DrcGain.Clip = 1;
    pAdrcCtx->drcAttr.stManualV21.HiLit.Strength = 0.0;
    pAdrcCtx->drcAttr.stManualV21.LocalSetting.LocalData.LocalWeit = 1;
    pAdrcCtx->drcAttr.stManualV21.LocalSetting.LocalData.GlobalContrast = 0.0;
    pAdrcCtx->drcAttr.stManualV21.LocalSetting.LocalData.LoLitContrast = 0.0;
    pAdrcCtx->drcAttr.stManualV21.LocalSetting.curPixWeit = 0.376471;
    pAdrcCtx->drcAttr.stManualV21.LocalSetting.preFrameWeit = 0.8;
    pAdrcCtx->drcAttr.stManualV21.LocalSetting.Range_force_sgm = 0.0;
    pAdrcCtx->drcAttr.stManualV21.LocalSetting.Range_sgm_cur = 0.2;
    pAdrcCtx->drcAttr.stManualV21.LocalSetting.Range_sgm_pre = 0.2;
    pAdrcCtx->drcAttr.stManualV21.LocalSetting.Space_sgm_cur = 4068;
    pAdrcCtx->drcAttr.stManualV21.LocalSetting.Space_sgm_pre = 3068;
    pAdrcCtx->drcAttr.stManualV21.LocalSetting.Space_sgm_pre = 3068;
    pAdrcCtx->drcAttr.stManualV21.Compress.Mode = COMPRESS_AUTO;
    pAdrcCtx->drcAttr.stManualV21.Compress.Manual_curve[0] = 0;
    pAdrcCtx->drcAttr.stManualV21.Compress.Manual_curve[1] = 558;
    pAdrcCtx->drcAttr.stManualV21.Compress.Manual_curve[2] = 1087;
    pAdrcCtx->drcAttr.stManualV21.Compress.Manual_curve[3] = 1588;
    pAdrcCtx->drcAttr.stManualV21.Compress.Manual_curve[4] = 2063;
    pAdrcCtx->drcAttr.stManualV21.Compress.Manual_curve[5] = 2515;
    pAdrcCtx->drcAttr.stManualV21.Compress.Manual_curve[6] = 2944;
    pAdrcCtx->drcAttr.stManualV21.Compress.Manual_curve[7] = 3353;
    pAdrcCtx->drcAttr.stManualV21.Compress.Manual_curve[8] = 3744;
    pAdrcCtx->drcAttr.stManualV21.Compress.Manual_curve[9] = 4473;
    pAdrcCtx->drcAttr.stManualV21.Compress.Manual_curve[10] = 5139;
    pAdrcCtx->drcAttr.stManualV21.Compress.Manual_curve[11] = 5751;
    pAdrcCtx->drcAttr.stManualV21.Compress.Manual_curve[12] = 6316;
    pAdrcCtx->drcAttr.stManualV21.Compress.Manual_curve[13] = 6838;
    pAdrcCtx->drcAttr.stManualV21.Compress.Manual_curve[14] = 7322;
    pAdrcCtx->drcAttr.stManualV21.Compress.Manual_curve[15] = 7772;
    pAdrcCtx->drcAttr.stManualV21.Compress.Manual_curve[16] = 8192;
    pAdrcCtx->drcAttr.stManualV21.Scale_y[0] = 0;
    pAdrcCtx->drcAttr.stManualV21.Scale_y[1] = 2;
    pAdrcCtx->drcAttr.stManualV21.Scale_y[2] = 20;
    pAdrcCtx->drcAttr.stManualV21.Scale_y[3] = 76;
    pAdrcCtx->drcAttr.stManualV21.Scale_y[4] = 193;
    pAdrcCtx->drcAttr.stManualV21.Scale_y[5] = 381;
    pAdrcCtx->drcAttr.stManualV21.Scale_y[6] = 631;
    pAdrcCtx->drcAttr.stManualV21.Scale_y[7] = 772;
    pAdrcCtx->drcAttr.stManualV21.Scale_y[8] = 919;
    pAdrcCtx->drcAttr.stManualV21.Scale_y[9] = 1066;
    pAdrcCtx->drcAttr.stManualV21.Scale_y[10] = 1211;
    pAdrcCtx->drcAttr.stManualV21.Scale_y[11] = 1479;
    pAdrcCtx->drcAttr.stManualV21.Scale_y[12] = 1700;
    pAdrcCtx->drcAttr.stManualV21.Scale_y[13] = 1863;
    pAdrcCtx->drcAttr.stManualV21.Scale_y[14] = 1968;
    pAdrcCtx->drcAttr.stManualV21.Scale_y[15] = 2024;
    pAdrcCtx->drcAttr.stManualV21.Scale_y[16] = 2048;
    pAdrcCtx->drcAttr.stManualV21.Edge_Weit = 0.02;
    pAdrcCtx->drcAttr.stManualV21.OutPutLongFrame = false;
    pAdrcCtx->drcAttr.stManualV21.IIR_frame = 2;

    LOG1_ATMO( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * Config()
 *set default Config data
 *****************************************************************************/
void ConfigV30
(
    AdrcContext_t*           pAdrcCtx
) {
    LOGI_ATMO( "%s:enter!\n", __FUNCTION__);

    // initial checks
    DCT_ASSERT(pAdrcCtx != NULL);

    //config default PrevData data
    pAdrcCtx->PrevData.frameCnt = 0;
    pAdrcCtx->PrevData.EnvLv = 0;
    pAdrcCtx->PrevData.MotionCoef = 0;
    pAdrcCtx->PrevData.ApiMode                                                   = DRC_OPMODE_AUTO;
    pAdrcCtx->PrevData.HandleData.Drc_v30.DrcGain.DrcGain                        = 1.0;
    pAdrcCtx->PrevData.HandleData.Drc_v30.DrcGain.Alpha                          = 0.0;
    pAdrcCtx->PrevData.HandleData.Drc_v30.DrcGain.Clip                           = 16.0;
    pAdrcCtx->PrevData.HandleData.Drc_v30.HiLight.Strength                       = 0.01;
    pAdrcCtx->PrevData.HandleData.Drc_v30.LocalSetting.LocalData.LocalWeit       = 1.00;
    pAdrcCtx->PrevData.HandleData.Drc_v30.LocalSetting.LocalData.LocalAutoEnable = 1;
    pAdrcCtx->PrevData.HandleData.Drc_v30.LocalSetting.LocalData.LocalAutoWeit   = 0.037477;
    pAdrcCtx->PrevData.HandleData.Drc_v30.LocalSetting.LocalData.GlobalContrast  = 0;
    pAdrcCtx->PrevData.HandleData.Drc_v30.LocalSetting.LocalData.LoLitContrast   = 0;

    //set default value for api
    pAdrcCtx->drcAttr.stManualV30.Enable = true;
    pAdrcCtx->drcAttr.stManualV30.DrcGain.Alpha = 0.1;
    pAdrcCtx->drcAttr.stManualV30.DrcGain.DrcGain = 1;
    pAdrcCtx->drcAttr.stManualV30.DrcGain.Clip = 1;
    pAdrcCtx->drcAttr.stManualV30.HiLight.Strength = 0.0;
    pAdrcCtx->drcAttr.stManualV30.LocalSetting.LocalData.LocalWeit = 1;
    pAdrcCtx->drcAttr.stManualV30.LocalSetting.LocalData.LocalAutoEnable = 1;
    pAdrcCtx->drcAttr.stManualV30.LocalSetting.LocalData.LocalAutoWeit = 0.5;
    pAdrcCtx->drcAttr.stManualV30.LocalSetting.LocalData.GlobalContrast = 0.0;
    pAdrcCtx->drcAttr.stManualV30.LocalSetting.LocalData.LoLitContrast = 0.0;
    pAdrcCtx->drcAttr.stManualV30.LocalSetting.curPixWeit = 0.376471;
    pAdrcCtx->drcAttr.stManualV30.LocalSetting.preFrameWeit = 0.8;
    pAdrcCtx->drcAttr.stManualV30.LocalSetting.Range_force_sgm = 0.0;
    pAdrcCtx->drcAttr.stManualV30.LocalSetting.Range_sgm_cur = 0.2;
    pAdrcCtx->drcAttr.stManualV30.LocalSetting.Range_sgm_pre = 0.2;
    pAdrcCtx->drcAttr.stManualV30.LocalSetting.Space_sgm_cur = 4068;
    pAdrcCtx->drcAttr.stManualV30.LocalSetting.Space_sgm_pre = 3068;
    pAdrcCtx->drcAttr.stManualV30.LocalSetting.Space_sgm_pre = 3068;
    pAdrcCtx->drcAttr.stManualV30.CompressSetting.Mode = COMPRESS_AUTO;
    pAdrcCtx->drcAttr.stManualV30.CompressSetting.Manual_curve[0] = 0;
    pAdrcCtx->drcAttr.stManualV30.CompressSetting.Manual_curve[1] = 558;
    pAdrcCtx->drcAttr.stManualV30.CompressSetting.Manual_curve[2] = 1087;
    pAdrcCtx->drcAttr.stManualV30.CompressSetting.Manual_curve[3] = 1588;
    pAdrcCtx->drcAttr.stManualV30.CompressSetting.Manual_curve[4] = 2063;
    pAdrcCtx->drcAttr.stManualV30.CompressSetting.Manual_curve[5] = 2515;
    pAdrcCtx->drcAttr.stManualV30.CompressSetting.Manual_curve[6] = 2944;
    pAdrcCtx->drcAttr.stManualV30.CompressSetting.Manual_curve[7] = 3353;
    pAdrcCtx->drcAttr.stManualV30.CompressSetting.Manual_curve[8] = 3744;
    pAdrcCtx->drcAttr.stManualV30.CompressSetting.Manual_curve[9] = 4473;
    pAdrcCtx->drcAttr.stManualV30.CompressSetting.Manual_curve[10] = 5139;
    pAdrcCtx->drcAttr.stManualV30.CompressSetting.Manual_curve[11] = 5751;
    pAdrcCtx->drcAttr.stManualV30.CompressSetting.Manual_curve[12] = 6316;
    pAdrcCtx->drcAttr.stManualV30.CompressSetting.Manual_curve[13] = 6838;
    pAdrcCtx->drcAttr.stManualV30.CompressSetting.Manual_curve[14] = 7322;
    pAdrcCtx->drcAttr.stManualV30.CompressSetting.Manual_curve[15] = 7772;
    pAdrcCtx->drcAttr.stManualV30.CompressSetting.Manual_curve[16] = 8192;
    pAdrcCtx->drcAttr.stManualV30.Scale_y[0] = 0;
    pAdrcCtx->drcAttr.stManualV30.Scale_y[1] = 2;
    pAdrcCtx->drcAttr.stManualV30.Scale_y[2] = 20;
    pAdrcCtx->drcAttr.stManualV30.Scale_y[3] = 76;
    pAdrcCtx->drcAttr.stManualV30.Scale_y[4] = 193;
    pAdrcCtx->drcAttr.stManualV30.Scale_y[5] = 381;
    pAdrcCtx->drcAttr.stManualV30.Scale_y[6] = 631;
    pAdrcCtx->drcAttr.stManualV30.Scale_y[7] = 772;
    pAdrcCtx->drcAttr.stManualV30.Scale_y[8] = 919;
    pAdrcCtx->drcAttr.stManualV30.Scale_y[9] = 1066;
    pAdrcCtx->drcAttr.stManualV30.Scale_y[10] = 1211;
    pAdrcCtx->drcAttr.stManualV30.Scale_y[11] = 1479;
    pAdrcCtx->drcAttr.stManualV30.Scale_y[12] = 1700;
    pAdrcCtx->drcAttr.stManualV30.Scale_y[13] = 1863;
    pAdrcCtx->drcAttr.stManualV30.Scale_y[14] = 1968;
    pAdrcCtx->drcAttr.stManualV30.Scale_y[15] = 2024;
    pAdrcCtx->drcAttr.stManualV30.Scale_y[16] = 2048;
    pAdrcCtx->drcAttr.stManualV30.Edge_Weit = 0.02;
    pAdrcCtx->drcAttr.stManualV30.OutPutLongFrame = false;
    pAdrcCtx->drcAttr.stManualV30.IIR_frame = 2;

    LOG1_ATMO( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * Calib2stAutoV21()
 *set default Config data
 *****************************************************************************/
void Calib2stAutoV21(AdrcContext_t* pAdrcCtx) {
    LOGI_ATMO("%s:enter!\n", __FUNCTION__);

    // initial checks
    DCT_ASSERT(pAdrcCtx != NULL);

    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.Enable =
        pAdrcCtx->Calib.pCalibV21->DrcTuningPara.Enable;
    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.CtrlDataType =
        pAdrcCtx->Calib.pCalibV21->DrcTuningPara.CtrlDataType;
    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.ByPassThr =
        pAdrcCtx->Calib.pCalibV21->DrcTuningPara.ByPassThr;
    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.Edge_Weit =
        pAdrcCtx->Calib.pCalibV21->DrcTuningPara.Edge_Weit;
    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.OutPutLongFrame =
        pAdrcCtx->Calib.pCalibV21->DrcTuningPara.OutPutLongFrame;
    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.IIR_frame =
        pAdrcCtx->Calib.pCalibV21->DrcTuningPara.IIR_frame;
    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.Tolerance =
        pAdrcCtx->Calib.pCalibV21->DrcTuningPara.Tolerance;
    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.damp = pAdrcCtx->Calib.pCalibV21->DrcTuningPara.damp;
    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.curPixWeit =
        pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.curPixWeit;
    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.preFrameWeit =
        pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.preFrameWeit;
    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.Range_force_sgm =
        pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.Range_force_sgm;
    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.Range_sgm_cur =
        pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.Range_sgm_cur;
    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.Range_sgm_pre =
        pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.Range_sgm_pre;
    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.Space_sgm_cur =
        pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.Space_sgm_cur;
    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.Space_sgm_pre =
        pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.Space_sgm_pre;
    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.CompressSetting.Mode =
        pAdrcCtx->Calib.pCalibV21->DrcTuningPara.CompressSetting.Mode;
    for (int i = 0; i < ADRC_Y_NUM; i++)
        pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.CompressSetting.Manual_curve[i] =
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.CompressSetting.Manual_curve[i];
    for (int i = 0; i < ADRC_Y_NUM; i++)
        pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.Scale_y[i] =
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.Scale_y[i];
    if (pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.CtrlData_len >= ADRC_STEP_MAX) {
        for (int i = 0; i < ADRC_STEP_MAX; i++) {
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.CtrlData[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.CtrlData[i];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.DrcGain[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.DrcGain[i];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.Alpha[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.Alpha[i];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.Clip[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.Clip[i];
        }
    } else {
        for (int i = 0; i < pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.CtrlData_len; i++) {
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.CtrlData[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.CtrlData[i];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.DrcGain[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.DrcGain[i];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.Alpha[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.Alpha[i];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.Clip[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.Clip[i];
        }
        for (int i = pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.CtrlData_len;
             i < ADRC_STEP_MAX; i++) {
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.CtrlData[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain
                    .CtrlData[pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.CtrlData_len - 1];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.DrcGain[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain
                    .DrcGain[pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.CtrlData_len - 1];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.Alpha[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain
                    .Alpha[pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.CtrlData_len - 1];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.Clip[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain
                    .Clip[pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.CtrlData_len - 1];
        }
    }
    if (pAdrcCtx->Calib.pCalibV21->DrcTuningPara.HiLight.CtrlData_len >= ADRC_STEP_MAX) {
        for (int i = 0; i < ADRC_STEP_MAX; i++) {
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.HiLight.CtrlData[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.HiLight.CtrlData[i];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.HiLight.Strength[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.HiLight.Strength[i];
        }
    } else {
        for (int i = 0; i < pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.CtrlData_len; i++) {
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.HiLight.CtrlData[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.HiLight.CtrlData[i];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.HiLight.Strength[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.HiLight.Strength[i];
        }
        for (int i = pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.CtrlData_len;
             i < ADRC_STEP_MAX; i++) {
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.HiLight.CtrlData[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.HiLight
                    .CtrlData[pAdrcCtx->Calib.pCalibV21->DrcTuningPara.HiLight.CtrlData_len - 1];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.HiLight.Strength[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.HiLight
                    .Strength[pAdrcCtx->Calib.pCalibV21->DrcTuningPara.HiLight.CtrlData_len - 1];
        }
    }
    if (pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData.CtrlData_len >=
        ADRC_STEP_MAX) {
        for (int i = 0; i < ADRC_STEP_MAX; i++) {
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData.CtrlData[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData.CtrlData[i];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData.LocalWeit[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData.LocalWeit[i];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData
                .GlobalContrast[i] = pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting
                                         .LocalTMOData.GlobalContrast[i];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData
                .LoLitContrast[i] = pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting
                                        .LocalTMOData.LoLitContrast[i];
        }
    } else {
        for (int i = 0;
             i < pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData.CtrlData_len;
             i++) {
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData.CtrlData[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData.CtrlData[i];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData.LocalWeit[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData.LocalWeit[i];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData
                .GlobalContrast[i] = pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting
                                         .LocalTMOData.GlobalContrast[i];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData
                .LoLitContrast[i] = pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting
                                        .LocalTMOData.LoLitContrast[i];
        }
        for (int i =
                 pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData.CtrlData_len;
             i < ADRC_STEP_MAX; i++) {
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData.CtrlData[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData
                    .CtrlData[pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData
                                  .CtrlData_len -
                              1];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData.LocalWeit[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData
                    .LocalWeit[pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData
                                   .CtrlData_len -
                               1];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData
                .GlobalContrast[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData
                    .GlobalContrast[pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting
                                        .LocalTMOData.CtrlData_len -
                                    1];
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData
                .LoLitContrast[i] =
                pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData
                    .LoLitContrast[pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting
                                       .LocalTMOData.CtrlData_len -
                                   1];
        }
    }

    LOG1_ATMO("%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * Calib2stAutoV30()
 *set default Config data
 *****************************************************************************/
void Calib2stAutoV30(AdrcContext_t* pAdrcCtx) {
    LOGI_ATMO("%s:enter!\n", __FUNCTION__);

    // initial checks
    DCT_ASSERT(pAdrcCtx != NULL);

    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.Enable =
        pAdrcCtx->Calib.pCalibV30->DrcTuningPara.Enable;
    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.CtrlDataType =
        pAdrcCtx->Calib.pCalibV30->DrcTuningPara.CtrlDataType;
    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.ByPassThr =
        pAdrcCtx->Calib.pCalibV30->DrcTuningPara.ByPassThr;
    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.Edge_Weit =
        pAdrcCtx->Calib.pCalibV30->DrcTuningPara.Edge_Weit;
    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.OutPutLongFrame =
        pAdrcCtx->Calib.pCalibV30->DrcTuningPara.OutPutLongFrame;
    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.IIR_frame =
        pAdrcCtx->Calib.pCalibV30->DrcTuningPara.IIR_frame;
    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.Tolerance =
        pAdrcCtx->Calib.pCalibV30->DrcTuningPara.Tolerance;
    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.damp = pAdrcCtx->Calib.pCalibV30->DrcTuningPara.damp;
    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.curPixWeit =
        pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.curPixWeit;
    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.preFrameWeit =
        pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.preFrameWeit;
    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.Range_force_sgm =
        pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.Range_force_sgm;
    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.Range_sgm_cur =
        pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.Range_sgm_cur;
    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.Range_sgm_pre =
        pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.Range_sgm_pre;
    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.Space_sgm_cur =
        pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.Space_sgm_cur;
    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.Space_sgm_pre =
        pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.Space_sgm_pre;
    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.CompressSetting.Mode =
        pAdrcCtx->Calib.pCalibV30->DrcTuningPara.CompressSetting.Mode;
    for (int i = 0; i < ADRC_Y_NUM; i++)
        pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.CompressSetting.Manual_curve[i] =
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.CompressSetting.Manual_curve[i];
    for (int i = 0; i < ADRC_Y_NUM; i++)
        pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.Scale_y[i] =
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.Scale_y[i];
    if (pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.CtrlData_len >= ADRC_STEP_MAX) {
        for (int i = 0; i < ADRC_STEP_MAX; i++) {
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.CtrlData[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.CtrlData[i];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.DrcGain[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.DrcGain[i];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.Alpha[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.Alpha[i];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.Clip[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.Clip[i];
        }
    } else {
        for (int i = 0; i < pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.CtrlData_len; i++) {
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.CtrlData[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.CtrlData[i];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.DrcGain[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.DrcGain[i];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.Alpha[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.Alpha[i];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.Clip[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.Clip[i];
        }
        for (int i = pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.CtrlData_len;
             i < ADRC_STEP_MAX; i++) {
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.CtrlData[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain
                    .CtrlData[pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.CtrlData_len - 1];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.DrcGain[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain
                    .DrcGain[pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.CtrlData_len - 1];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.Alpha[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain
                    .Alpha[pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.CtrlData_len - 1];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.Clip[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain
                    .Clip[pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.CtrlData_len - 1];
        }
    }
    if (pAdrcCtx->Calib.pCalibV30->DrcTuningPara.HiLight.CtrlData_len >= ADRC_STEP_MAX) {
        for (int i = 0; i < ADRC_STEP_MAX; i++) {
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.HiLight.CtrlData[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.HiLight.CtrlData[i];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.HiLight.Strength[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.HiLight.Strength[i];
        }
    } else {
        for (int i = 0; i < pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.CtrlData_len; i++) {
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.HiLight.CtrlData[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.HiLight.CtrlData[i];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.HiLight.Strength[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.HiLight.Strength[i];
        }
        for (int i = pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.CtrlData_len;
             i < ADRC_STEP_MAX; i++) {
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.HiLight.CtrlData[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.HiLight
                    .CtrlData[pAdrcCtx->Calib.pCalibV30->DrcTuningPara.HiLight.CtrlData_len - 1];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.HiLight.Strength[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.HiLight
                    .Strength[pAdrcCtx->Calib.pCalibV30->DrcTuningPara.HiLight.CtrlData_len - 1];
        }
    }
    if (pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.CtrlData_len >=
        ADRC_STEP_MAX) {
        for (int i = 0; i < ADRC_STEP_MAX; i++) {
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.CtrlData[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.CtrlData[i];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LocalWeit[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LocalWeit[i];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LocalAutoEnable[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LocalAutoEnable[i];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LocalAutoWeit[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LocalAutoWeit[i];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.GlobalContrast[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.GlobalContrast[i];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LoLitContrast[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LoLitContrast[i];
        }
    } else {
        for (int i = 0;
             i < pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.CtrlData_len;
             i++) {
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.CtrlData[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.CtrlData[i];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LocalWeit[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LocalWeit[i];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LocalAutoEnable[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LocalAutoEnable[i];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LocalAutoWeit[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LocalAutoWeit[i];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.GlobalContrast[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.GlobalContrast[i];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LoLitContrast[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LoLitContrast[i];
        }
        for (int i = pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.CtrlData_len;
             i < ADRC_STEP_MAX; i++) {
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.CtrlData[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.CtrlData
                    [pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.CtrlData_len -
                     1];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LocalWeit[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LocalWeit
                    [pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.CtrlData_len -
                     1];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LocalAutoEnable[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LocalAutoEnable
                    [pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.CtrlData_len -
                     1];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LocalAutoWeit[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LocalAutoWeit
                    [pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.CtrlData_len -
                     1];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.GlobalContrast[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.GlobalContrast
                    [pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.CtrlData_len -
                     1];
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LoLitContrast[i] =
                pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LoLitContrast
                    [pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.CtrlData_len -
                     1];
        }
    }

    LOG1_ATMO("%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * stAuto2CalibV21()
 *set default Config data
 *****************************************************************************/
void stAuto2CalibV21(AdrcContext_t* pAdrcCtx) {
    LOGI_ATMO("%s:enter!\n", __FUNCTION__);

    // initial checks
    DCT_ASSERT(pAdrcCtx != NULL);

    pAdrcCtx->Calib.pCalibV21->DrcTuningPara.Enable =
        pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.Enable;
    pAdrcCtx->Calib.pCalibV21->DrcTuningPara.CtrlDataType =
        pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.CtrlDataType;
    pAdrcCtx->Calib.pCalibV21->DrcTuningPara.ByPassThr =
        pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.ByPassThr;
    pAdrcCtx->Calib.pCalibV21->DrcTuningPara.Edge_Weit =
        pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.Edge_Weit;
    pAdrcCtx->Calib.pCalibV21->DrcTuningPara.OutPutLongFrame =
        pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.OutPutLongFrame;
    pAdrcCtx->Calib.pCalibV21->DrcTuningPara.IIR_frame =
        pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.IIR_frame;
    pAdrcCtx->Calib.pCalibV21->DrcTuningPara.Tolerance =
        pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.Tolerance;
    pAdrcCtx->Calib.pCalibV21->DrcTuningPara.damp = pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.damp;
    pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.curPixWeit =
        pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.curPixWeit;
    pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.preFrameWeit =
        pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.preFrameWeit;
    pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.Range_force_sgm =
        pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.Range_force_sgm;
    pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.Range_sgm_cur =
        pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.Range_sgm_cur;
    pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.Range_sgm_pre =
        pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.Range_sgm_pre;
    pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.Space_sgm_cur =
        pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.Space_sgm_cur;
    pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.Space_sgm_pre =
        pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.Space_sgm_pre;
    pAdrcCtx->Calib.pCalibV21->DrcTuningPara.CompressSetting.Mode =
        pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.CompressSetting.Mode;
    for (int i = 0; i < ADRC_Y_NUM; i++)
        pAdrcCtx->Calib.pCalibV21->DrcTuningPara.CompressSetting.Manual_curve[i] =
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.CompressSetting.Manual_curve[i];
    for (int i = 0; i < ADRC_Y_NUM; i++)
        pAdrcCtx->Calib.pCalibV21->DrcTuningPara.Scale_y[i] =
            pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.Scale_y[i];
    if (pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.CtrlData_len <= ADRC_STEP_MAX) {
        for (int i = 0; i < pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.CtrlData_len; i++) {
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.CtrlData[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.CtrlData[i];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.DrcGain[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.DrcGain[i];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.Alpha[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.Alpha[i];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.Clip[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.Clip[i];
        }
    } else {
        for (int i = 0; i < ADRC_STEP_MAX; i++) {
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.CtrlData[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.CtrlData[i];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.DrcGain[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.DrcGain[i];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.Alpha[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.Alpha[i];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.Clip[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.Clip[i];
        }
        for (int i = ADRC_STEP_MAX;
             i < pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.CtrlData_len; i++) {
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.CtrlData[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.CtrlData[ADRC_STEP_MAX - 1];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.DrcGain[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.DrcGain[ADRC_STEP_MAX - 1];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.Alpha[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.Alpha[ADRC_STEP_MAX - 1];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.DrcGain.Clip[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.Clip[ADRC_STEP_MAX - 1];
        }
    }
    if (pAdrcCtx->Calib.pCalibV21->DrcTuningPara.HiLight.CtrlData_len <= ADRC_STEP_MAX) {
        for (int i = 0; i < pAdrcCtx->Calib.pCalibV21->DrcTuningPara.HiLight.CtrlData_len; i++) {
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.HiLight.CtrlData[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.HiLight.CtrlData[i];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.HiLight.Strength[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.HiLight.Strength[i];
        }
    } else {
        for (int i = 0; i < ADRC_STEP_MAX; i++) {
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.HiLight.CtrlData[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.HiLight.CtrlData[i];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.HiLight.Strength[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.HiLight.Strength[i];
        }
        for (int i = ADRC_STEP_MAX;
             i < pAdrcCtx->Calib.pCalibV21->DrcTuningPara.HiLight.CtrlData_len; i++) {
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.HiLight.CtrlData[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.HiLight.CtrlData[ADRC_STEP_MAX - 1];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.HiLight.Strength[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.HiLight.Strength[ADRC_STEP_MAX - 1];
        }
    }
    if (pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData.CtrlData_len <=
        ADRC_STEP_MAX) {
        for (int i = 0;
             i < pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData.CtrlData_len;
             i++) {
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData.CtrlData[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData.CtrlData[i];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData.LocalWeit[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData.LocalWeit[i];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData
                .GlobalContrast[i] = pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting
                                         .LocalTMOData.GlobalContrast[i];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData.LoLitContrast[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData
                    .LoLitContrast[i];
        }
    } else {
        for (int i = 0; i < ADRC_STEP_MAX; i++) {
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData.CtrlData[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData.CtrlData[i];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData.LocalWeit[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData.LocalWeit[i];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData
                .GlobalContrast[i] = pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting
                                         .LocalTMOData.GlobalContrast[i];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData.LoLitContrast[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData
                    .LoLitContrast[i];
        }
        for (int i = ADRC_STEP_MAX;
             i < pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData.CtrlData_len;
             i++) {
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData.CtrlData[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData
                    .CtrlData[ADRC_STEP_MAX - 1];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData.LocalWeit[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData
                    .LocalWeit[ADRC_STEP_MAX - 1];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData
                .GlobalContrast[i] = pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting
                                         .LocalTMOData.GlobalContrast[ADRC_STEP_MAX - 1];
            pAdrcCtx->Calib.pCalibV21->DrcTuningPara.LocalTMOSetting.LocalTMOData.LoLitContrast[i] =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData
                    .LoLitContrast[ADRC_STEP_MAX - 1];
        }
    }

    LOG1_ATMO("%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * stAuto2CalibV30()
 *set default Config data
 *****************************************************************************/
void stAuto2CalibV30(AdrcContext_t* pAdrcCtx) {
    LOGI_ATMO("%s:enter!\n", __FUNCTION__);

    // initial checks
    DCT_ASSERT(pAdrcCtx != NULL);

    pAdrcCtx->Calib.pCalibV30->DrcTuningPara.Enable =
        pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.Enable;
    pAdrcCtx->Calib.pCalibV30->DrcTuningPara.CtrlDataType =
        pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.CtrlDataType;
    pAdrcCtx->Calib.pCalibV30->DrcTuningPara.ByPassThr =
        pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.ByPassThr;
    pAdrcCtx->Calib.pCalibV30->DrcTuningPara.Edge_Weit =
        pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.Edge_Weit;
    pAdrcCtx->Calib.pCalibV30->DrcTuningPara.OutPutLongFrame =
        pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.OutPutLongFrame;
    pAdrcCtx->Calib.pCalibV30->DrcTuningPara.IIR_frame =
        pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.IIR_frame;
    pAdrcCtx->Calib.pCalibV30->DrcTuningPara.Tolerance =
        pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.Tolerance;
    pAdrcCtx->Calib.pCalibV30->DrcTuningPara.damp = pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.damp;
    pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.curPixWeit =
        pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.curPixWeit;
    pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.preFrameWeit =
        pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.preFrameWeit;
    pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.Range_force_sgm =
        pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.Range_force_sgm;
    pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.Range_sgm_cur =
        pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.Range_sgm_cur;
    pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.Range_sgm_pre =
        pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.Range_sgm_pre;
    pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.Space_sgm_cur =
        pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.Space_sgm_cur;
    pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.Space_sgm_pre =
        pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.Space_sgm_pre;
    pAdrcCtx->Calib.pCalibV30->DrcTuningPara.CompressSetting.Mode =
        pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.CompressSetting.Mode;
    for (int i = 0; i < ADRC_Y_NUM; i++)
        pAdrcCtx->Calib.pCalibV30->DrcTuningPara.CompressSetting.Manual_curve[i] =
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.CompressSetting.Manual_curve[i];
    for (int i = 0; i < ADRC_Y_NUM; i++)
        pAdrcCtx->Calib.pCalibV30->DrcTuningPara.Scale_y[i] =
            pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.Scale_y[i];
    if (pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.CtrlData_len <= ADRC_STEP_MAX) {
        for (int i = 0; i < pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.CtrlData_len; i++) {
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.CtrlData[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.CtrlData[i];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.DrcGain[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.DrcGain[i];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.Alpha[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.Alpha[i];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.Clip[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.Clip[i];
        }
    } else {
        for (int i = 0; i < ADRC_STEP_MAX; i++) {
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.CtrlData[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.CtrlData[i];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.DrcGain[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.DrcGain[i];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.Alpha[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.Alpha[i];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.Clip[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.Clip[i];
        }
        for (int i = ADRC_STEP_MAX;
             i < pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.CtrlData_len; i++) {
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.CtrlData[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.CtrlData[ADRC_STEP_MAX - 1];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.DrcGain[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.DrcGain[ADRC_STEP_MAX - 1];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.Alpha[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.Alpha[ADRC_STEP_MAX - 1];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.DrcGain.Clip[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.Clip[ADRC_STEP_MAX - 1];
        }
    }
    if (pAdrcCtx->Calib.pCalibV30->DrcTuningPara.HiLight.CtrlData_len <= ADRC_STEP_MAX) {
        for (int i = 0; i < pAdrcCtx->Calib.pCalibV30->DrcTuningPara.HiLight.CtrlData_len; i++) {
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.HiLight.CtrlData[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.HiLight.CtrlData[i];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.HiLight.Strength[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.HiLight.Strength[i];
        }
    } else {
        for (int i = 0; i < ADRC_STEP_MAX; i++) {
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.HiLight.CtrlData[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.HiLight.CtrlData[i];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.HiLight.Strength[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.HiLight.Strength[i];
        }
        for (int i = ADRC_STEP_MAX;
             i < pAdrcCtx->Calib.pCalibV30->DrcTuningPara.HiLight.CtrlData_len; i++) {
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.HiLight.CtrlData[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.HiLight.CtrlData[ADRC_STEP_MAX - 1];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.HiLight.Strength[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.HiLight.Strength[ADRC_STEP_MAX - 1];
        }
    }
    if (pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.CtrlData_len <=
        ADRC_STEP_MAX) {
        for (int i = 0;
             i < pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.CtrlData_len;
             i++) {
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.CtrlData[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.CtrlData[i];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LocalWeit[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LocalWeit[i];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LocalAutoEnable[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LocalAutoEnable[i];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LocalAutoWeit[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LocalAutoWeit[i];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.GlobalContrast[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.GlobalContrast[i];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LoLitContrast[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LoLitContrast[i];
        }
    } else {
        for (int i = 0; i < ADRC_STEP_MAX; i++) {
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.CtrlData[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.CtrlData[i];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LocalWeit[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LocalWeit[i];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LocalAutoEnable[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LocalAutoEnable[i];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LocalAutoWeit[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LocalAutoWeit[i];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.GlobalContrast[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.GlobalContrast[i];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LoLitContrast[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LoLitContrast[i];
        }
        for (int i = ADRC_STEP_MAX;
             i < pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.CtrlData_len;
             i++) {
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.CtrlData[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData
                    .CtrlData[ADRC_STEP_MAX - 1];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LocalWeit[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData
                    .LocalWeit[ADRC_STEP_MAX - 1];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LocalAutoEnable[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData
                    .LocalAutoEnable[ADRC_STEP_MAX - 1];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LocalAutoWeit[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData
                    .LocalAutoWeit[ADRC_STEP_MAX - 1];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.GlobalContrast[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData
                    .GlobalContrast[ADRC_STEP_MAX - 1];
            pAdrcCtx->Calib.pCalibV30->DrcTuningPara.LocalSetting.LocalData.LoLitContrast[i] =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData
                    .LoLitContrast[ADRC_STEP_MAX - 1];
        }
    }

    LOG1_ATMO("%s:exit!\n", __FUNCTION__);
}

void AdrcGetEnvLv
(
    AdrcContext_t*           pAdrcCtx,
    AecPreResult_t  AecHdrPreResult
) {
    LOG1_ATMO( "%s:enter!\n", __FUNCTION__);

    //transfer CurrAeResult data into AhdrHandle
    switch (pAdrcCtx->FrameNumber)
    {
    case LINEAR_NUM:
        pAdrcCtx->CurrAeResult.GlobalEnvLv = AecHdrPreResult.GlobalEnvLv[0];
        break;
    case HDR_2X_NUM:
        pAdrcCtx->CurrAeResult.GlobalEnvLv = AecHdrPreResult.GlobalEnvLv[1];
        break;
    case HDR_3X_NUM:
        if(CHECK_ISP_HW_V30()) {
            pAdrcCtx->CurrAeResult.GlobalEnvLv = AecHdrPreResult.GlobalEnvLv[1];
        }
        break;
    default:
        LOGE_ATMO("%s:  Wrong frame number in HDR mode!!!\n", __FUNCTION__);
        break;
    }

    //Normalize the current envLv for AEC
    pAdrcCtx->CurrData.EnvLv = (pAdrcCtx->CurrAeResult.GlobalEnvLv  - MIN_ENV_LV) / (MAX_ENV_LV - MIN_ENV_LV);
    pAdrcCtx->CurrData.EnvLv = LIMIT_VALUE(pAdrcCtx->CurrData.EnvLv, ENVLVMAX, ENVLVMIN);

    LOG1_ATMO( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * DrcEnableSetting()
 *
 *****************************************************************************/
void DrcEnableSetting
(
    AdrcContext_t* pAdrcCtx
)
{
    LOG1_ATMO("%s:enter!\n", __FUNCTION__);

    if(CHECK_ISP_HW_V21()) {
        if(pAdrcCtx->FrameNumber == HDR_2X_NUM)
            pAdrcCtx->AdrcProcRes.bDrcEn = true;
        else if(pAdrcCtx->FrameNumber == LINEAR_NUM) {
            if (pAdrcCtx->drcAttr.opMode == DRC_OPMODE_AUTO)
                pAdrcCtx->AdrcProcRes.bDrcEn = pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.Enable;
            else if(pAdrcCtx->drcAttr.opMode == DRC_OPMODE_MANU)
                pAdrcCtx->AdrcProcRes.bDrcEn = pAdrcCtx->drcAttr.stManualV21.Enable;
            else {
                LOGE_ATMO( "%s: Drc api in WRONG MODE!!!, drc by pass!!!\n", __FUNCTION__);
                pAdrcCtx->AdrcProcRes.bDrcEn = false;
            }
        }
    }
    else if(CHECK_ISP_HW_V30()) {
        if(pAdrcCtx->FrameNumber == HDR_2X_NUM || pAdrcCtx->FrameNumber == HDR_3X_NUM)
            pAdrcCtx->AdrcProcRes.bDrcEn = true;
        else if(pAdrcCtx->FrameNumber == LINEAR_NUM) {
            if (pAdrcCtx->drcAttr.opMode == DRC_OPMODE_AUTO)
                pAdrcCtx->AdrcProcRes.bDrcEn = pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.Enable;
            else if(pAdrcCtx->drcAttr.opMode == DRC_OPMODE_MANU)
                pAdrcCtx->AdrcProcRes.bDrcEn = pAdrcCtx->drcAttr.stManualV30.Enable;
            else {
                LOGE_ATMO( "%s: Drc api in WRONG MODE!!!, drc by pass!!!\n", __FUNCTION__);
                pAdrcCtx->AdrcProcRes.bDrcEn = false;
            }
        }
    }

    LOG1_ATMO( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AdrcDampingV21()
 *****************************************************************************/
void AdrcDampingV21
(
    CurrData_t* pCurrData,
    AdrcPrevData_t* pPreData,
    drc_OpMode_t opMode,
    int FrameCnt
)
{
    LOG1_ATMO("%s:Enter!\n", __FUNCTION__);
    bool enDamp          = true;
    bool ifHDRModeChange = pCurrData->frameCnt == pPreData->frameCnt ? false : true;

    if (opMode != DRC_OPMODE_MANU && FrameCnt != 0 && !ifHDRModeChange) {
        /*
float diff = 0.0;
diff = ABS(pCurrData->EnvLv - pPreData->EnvLv);
diff = diff / pPreData->EnvLv;
if (diff < pCurrData->Others.Tolerance)
    enDamp = false;
else
    enDamp = true;*/

        //get finnal cfg data by damp
        if (enDamp) {
            pCurrData->HandleData.Drc_v21.DrcGain.DrcGain =
                pCurrData->damp * pCurrData->HandleData.Drc_v21.DrcGain.DrcGain +
                (1 - pCurrData->damp) * pPreData->HandleData.Drc_v21.DrcGain.DrcGain;
            pCurrData->HandleData.Drc_v21.DrcGain.Alpha =
                pCurrData->damp * pCurrData->HandleData.Drc_v21.DrcGain.Alpha +
                (1 - pCurrData->damp) * pPreData->HandleData.Drc_v21.DrcGain.Alpha;
            pCurrData->HandleData.Drc_v21.DrcGain.Clip =
                pCurrData->damp * pCurrData->HandleData.Drc_v21.DrcGain.Clip +
                (1 - pCurrData->damp) * pPreData->HandleData.Drc_v21.DrcGain.Clip;

            pCurrData->HandleData.Drc_v21.HiLit.Strength =
                pCurrData->damp * pCurrData->HandleData.Drc_v21.HiLit.Strength +
                (1 - pCurrData->damp) * pPreData->HandleData.Drc_v21.HiLit.Strength;

            pCurrData->HandleData.Drc_v21.LocalSetting.LocalData.LocalWeit =
                pCurrData->damp * pCurrData->HandleData.Drc_v21.LocalSetting.LocalData.LocalWeit +
                (1 - pCurrData->damp) *
                    pPreData->HandleData.Drc_v21.LocalSetting.LocalData.LocalWeit;
            pCurrData->HandleData.Drc_v21.LocalSetting.LocalData.GlobalContrast =
                pCurrData->damp *
                    pCurrData->HandleData.Drc_v21.LocalSetting.LocalData.GlobalContrast +
                (1 - pCurrData->damp) *
                    pPreData->HandleData.Drc_v21.LocalSetting.LocalData.GlobalContrast;
            pCurrData->HandleData.Drc_v21.LocalSetting.LocalData.LoLitContrast =
                pCurrData->damp *
                    pCurrData->HandleData.Drc_v21.LocalSetting.LocalData.LoLitContrast +
                (1 - pCurrData->damp) *
                    pPreData->HandleData.Drc_v21.LocalSetting.LocalData.LoLitContrast;
        }
    }

    LOGD_ATMO(
        "%s:%d: Current damp DrcGain:%f Alpha:%f Clip:%f Strength:%f LocalWeit:%f "
        "GlobalContrast:%f LoLitContrast:%f\n",
        __FUNCTION__, __LINE__, pCurrData->HandleData.Drc_v21.DrcGain.DrcGain,
        pCurrData->HandleData.Drc_v21.DrcGain.Alpha, pCurrData->HandleData.Drc_v21.DrcGain.Clip,
        pCurrData->HandleData.Drc_v21.HiLit.Strength,
        pCurrData->HandleData.Drc_v21.LocalSetting.LocalData.LocalWeit,
        pCurrData->HandleData.Drc_v21.LocalSetting.LocalData.GlobalContrast,
        pCurrData->HandleData.Drc_v21.LocalSetting.LocalData.LoLitContrast);

    LOG1_ATMO("%s:Eixt!\n", __FUNCTION__);
}
/******************************************************************************
 * AdrcDampingV30()
 *****************************************************************************/
void AdrcDampingV30
(
    CurrData_t* pCurrData,
    AdrcPrevData_t* pPreData,
    drc_OpMode_t opMode,
    int FrameCnt
)
{
    LOG1_ATMO("%s:Enter!\n", __FUNCTION__);
    bool enDamp          = true;
    bool ifHDRModeChange = pCurrData->frameCnt == pPreData->frameCnt ? false : true;

    if (opMode != DRC_OPMODE_MANU && FrameCnt != 0 && !ifHDRModeChange) {
        /*
            float diff = 0.0;
            diff = ABS(pCurrData->EnvLv - pPreData->EnvLv);
            diff = diff / pPreData->EnvLv;
            if (diff < pCurrData->Others.Tolerance)
                enDamp = false;
            else
                enDamp = true;*/

        //get finnal cfg data by damp
        if (enDamp) {
            pCurrData->HandleData.Drc_v30.DrcGain.DrcGain =
                pCurrData->damp * pCurrData->HandleData.Drc_v30.DrcGain.DrcGain +
                (1 - pCurrData->damp) * pPreData->HandleData.Drc_v30.DrcGain.DrcGain;
            pCurrData->HandleData.Drc_v30.DrcGain.Alpha =
                pCurrData->damp * pCurrData->HandleData.Drc_v30.DrcGain.Alpha +
                (1 - pCurrData->damp) * pPreData->HandleData.Drc_v30.DrcGain.Alpha;
            pCurrData->HandleData.Drc_v30.DrcGain.Clip =
                pCurrData->damp * pCurrData->HandleData.Drc_v30.DrcGain.Clip +
                (1 - pCurrData->damp) * pPreData->HandleData.Drc_v30.DrcGain.Clip;

            pCurrData->HandleData.Drc_v30.HiLight.Strength =
                pCurrData->damp * pCurrData->HandleData.Drc_v30.HiLight.Strength +
                (1 - pCurrData->damp) * pPreData->HandleData.Drc_v30.HiLight.Strength;

            pCurrData->HandleData.Drc_v30.LocalSetting.LocalData.LocalWeit =
                pCurrData->damp * pCurrData->HandleData.Drc_v30.LocalSetting.LocalData.LocalWeit +
                (1 - pCurrData->damp) *
                    pPreData->HandleData.Drc_v30.LocalSetting.LocalData.LocalWeit;
            pCurrData->HandleData.Drc_v30.LocalSetting.LocalData.LocalAutoWeit =
                pCurrData->damp *
                    pCurrData->HandleData.Drc_v30.LocalSetting.LocalData.LocalAutoWeit +
                (1 - pCurrData->damp) *
                    pPreData->HandleData.Drc_v30.LocalSetting.LocalData.LocalAutoWeit;
            pCurrData->HandleData.Drc_v30.LocalSetting.LocalData.GlobalContrast =
                pCurrData->damp *
                    pCurrData->HandleData.Drc_v30.LocalSetting.LocalData.GlobalContrast +
                (1 - pCurrData->damp) *
                    pPreData->HandleData.Drc_v30.LocalSetting.LocalData.GlobalContrast;
            pCurrData->HandleData.Drc_v30.LocalSetting.LocalData.LoLitContrast =
                pCurrData->damp *
                    pCurrData->HandleData.Drc_v30.LocalSetting.LocalData.LoLitContrast +
                (1 - pCurrData->damp) *
                    pPreData->HandleData.Drc_v30.LocalSetting.LocalData.LoLitContrast;
        }
    }

    LOGD_ATMO(
        "%s: Current damp DrcGain:%f Alpha:%f Clip:%f Strength:%f \n", __FUNCTION__,
        pCurrData->HandleData.Drc_v30.DrcGain.DrcGain, pCurrData->HandleData.Drc_v30.DrcGain.Alpha,
        pCurrData->HandleData.Drc_v30.DrcGain.Clip, pCurrData->HandleData.Drc_v30.HiLight.Strength);
    LOGD_ATMO(
        "%s: Current damp LocalWeit:%f LocalAutoEnable:%d LocalAutoWeit:%f GlobalContrast:%f "
        "LoLitContrast:%f\n",
        __FUNCTION__, pCurrData->HandleData.Drc_v30.LocalSetting.LocalData.LocalWeit,
        pCurrData->HandleData.Drc_v30.LocalSetting.LocalData.LocalAutoEnable,
        pCurrData->HandleData.Drc_v30.LocalSetting.LocalData.LocalAutoWeit,
        pCurrData->HandleData.Drc_v30.LocalSetting.LocalData.GlobalContrast,
        pCurrData->HandleData.Drc_v30.LocalSetting.LocalData.LoLitContrast);

    LOG1_ATMO("%s:Eixt!\n", __FUNCTION__);
}

/******************************************************************************
 * AdrcGetTuningProcResV21()
 *****************************************************************************/
void AdrcGetTuningProcResV21
(
    AdrcProcResData_t*           pAdrcProcRes,
    CurrData_t*          pCurrData,
    bool LongFrmMode,
    int FrameNumber,
    int FrameCnt
) {
    LOG1_ATMO( "%s:enter!\n", __FUNCTION__);

    //DrcProcRes
    pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_offset_pow2 = SW_DRC_OFFSET_POW2_FIX;
    pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_position =
        (int)(SHIFT8BIT(pCurrData->HandleData.Drc_v21.DrcGain.Clip) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_hpdetail_ratio =
        (int)(SHIFT12BIT(pCurrData->HandleData.Drc_v21.LocalSetting.LocalData.LoLitContrast) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_lpdetail_ratio =
        (int)(SHIFT12BIT(pCurrData->HandleData.Drc_v21.LocalSetting.LocalData.GlobalContrast) +
              0.5);
    pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_weicur_pix =
        (int)(SHIFT8BIT(pCurrData->HandleData.Drc_v21.LocalSetting.curPixWeit) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_weipre_frame =
        (int)(SHIFT8BIT(pCurrData->HandleData.Drc_v21.LocalSetting.preFrameWeit) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_weipre_frame = LIMIT_VALUE(pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_weipre_frame, 255, 0);
    pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_force_sgm_inv0 =
        (int)(SHIFT13BIT(pCurrData->HandleData.Drc_v21.LocalSetting.Range_force_sgm) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_edge_scl =
        (int)(SHIFT8BIT(pCurrData->HandleData.Drc_v21.Edge_Weit) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_motion_scl = SW_DRC_MOTION_SCL_FIX;
    pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_space_sgm_inv1 =
        (int)(pCurrData->HandleData.Drc_v21.LocalSetting.Space_sgm_cur);
    pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_space_sgm_inv0 =
        (int)(pCurrData->HandleData.Drc_v21.LocalSetting.Space_sgm_pre);
    pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_range_sgm_inv1 =
        (int)(SHIFT13BIT(pCurrData->HandleData.Drc_v21.LocalSetting.Range_sgm_cur) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_range_sgm_inv0 =
        (int)(SHIFT13BIT(pCurrData->HandleData.Drc_v21.LocalSetting.Range_sgm_pre) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_weig_maxl =
        (int)(SHIFT4BIT(pCurrData->HandleData.Drc_v21.HiLit.Strength) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_weig_bilat =
        (int)(SHIFT4BIT(pCurrData->HandleData.Drc_v21.LocalSetting.LocalData.LocalWeit) + 0.5);
    for(int i = 0; i < ADRC_Y_NUM; ++i) {
        pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_scale_y[i] =
            (int)(pCurrData->HandleData.Drc_v21.Scale_y[i]);
        pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_compres_y[i] =
            pCurrData->HandleData.Drc_v21.Compress.Manual_curve[i];
    }

    //get sw_drc_gain_y
    CalibrateDrcGainY(&pAdrcProcRes->DrcProcRes, pCurrData->HandleData.Drc_v21.DrcGain.DrcGain,
                      pCurrData->HandleData.Drc_v21.DrcGain.Alpha);

    float iir_frame = (float)(MIN(FrameCnt + 1, pCurrData->HandleData.Drc_v21.IIR_frame));
    pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_iir_weight = (int)(SHIFT6BIT((iir_frame - 1) / iir_frame) + 0.5);

    //sw_drc_delta_scalein FIX
    pAdrcProcRes->DrcProcRes.Drc_v21.sw_drc_delta_scalein = DELTA_SCALEIN_FIX;

    LOG1_ATMO( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AdrcGetTuningProcResV30()
 *****************************************************************************/
void AdrcGetTuningProcResV30
(
    AdrcProcResData_t*           pAdrcProcRes,
    CurrData_t*          pCurrData,
    bool LongFrmMode,
    int FrameNumber,
    int FrameCnt
) {
    LOG1_ATMO( "%s:enter!\n", __FUNCTION__);

    //DrcProcRes
    pAdrcProcRes->DrcProcRes.Drc_v30.bypass_en = FUNCTION_DISABLE;
    pAdrcProcRes->DrcProcRes.Drc_v30.offset_pow2 = SW_DRC_OFFSET_POW2_FIX;
    pAdrcProcRes->DrcProcRes.Drc_v30.position =
        (int)(SHIFT8BIT(pCurrData->HandleData.Drc_v30.DrcGain.Clip) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v30.hpdetail_ratio =
        (int)(SHIFT12BIT(pCurrData->HandleData.Drc_v30.LocalSetting.LocalData.LoLitContrast) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v30.lpdetail_ratio =
        (int)(SHIFT12BIT(pCurrData->HandleData.Drc_v30.LocalSetting.LocalData.GlobalContrast) +
              0.5);
    pAdrcProcRes->DrcProcRes.Drc_v30.weicur_pix =
        (int)(SHIFT8BIT(pCurrData->HandleData.Drc_v30.LocalSetting.curPixWeit) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v30.weipre_frame =
        (int)(SHIFT8BIT(pCurrData->HandleData.Drc_v30.LocalSetting.preFrameWeit) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v30.weipre_frame = LIMIT_VALUE(pAdrcProcRes->DrcProcRes.Drc_v30.weipre_frame, INT8BITMAX, 0);
    pAdrcProcRes->DrcProcRes.Drc_v30.bilat_wt_off = SW_DRC_BILAT_WT_OFF_FIX;//LIMIT_VALUE(pCurrData->HandleData.Drc_v30.MotionStr * INT8BITMAX, INT8BITMAX, 0);
    pAdrcProcRes->DrcProcRes.Drc_v30.force_sgm_inv0 =
        (int)(SHIFT13BIT(pCurrData->HandleData.Drc_v30.LocalSetting.Range_force_sgm) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v30.edge_scl =
        (int)(SHIFT8BIT(pCurrData->HandleData.Drc_v30.Edge_Weit) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v30.motion_scl = SW_DRC_MOTION_SCL_FIX;
    pAdrcProcRes->DrcProcRes.Drc_v30.space_sgm_inv1 =
        (int)(pCurrData->HandleData.Drc_v30.LocalSetting.Space_sgm_cur);
    pAdrcProcRes->DrcProcRes.Drc_v30.space_sgm_inv0 =
        (int)(pCurrData->HandleData.Drc_v30.LocalSetting.Space_sgm_pre);
    pAdrcProcRes->DrcProcRes.Drc_v30.range_sgm_inv1 =
        (int)(SHIFT13BIT(pCurrData->HandleData.Drc_v30.LocalSetting.Range_sgm_cur) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v30.range_sgm_inv0 =
        (int)(SHIFT13BIT(pCurrData->HandleData.Drc_v30.LocalSetting.Range_sgm_pre) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v30.weig_maxl =
        (int)(SHIFT4BIT(pCurrData->HandleData.Drc_v30.HiLight.Strength) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v30.weig_bilat =
        (int)(SHIFT4BIT(pCurrData->HandleData.Drc_v30.LocalSetting.LocalData.LocalWeit) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v30.enable_soft_thd =
        pCurrData->HandleData.Drc_v30.LocalSetting.LocalData.LocalAutoEnable;
    pAdrcProcRes->DrcProcRes.Drc_v30.bilat_soft_thd =
        (int)(SHIFT14BIT(pCurrData->HandleData.Drc_v30.LocalSetting.LocalData.LocalAutoWeit) + 0.5);
    pAdrcProcRes->DrcProcRes.Drc_v30.bilat_soft_thd = LIMIT_VALUE(pAdrcProcRes->DrcProcRes.Drc_v30.bilat_soft_thd, INT14BITMAX, 0);
    for(int i = 0; i < ADRC_Y_NUM; ++i) {
        pAdrcProcRes->DrcProcRes.Drc_v30.scale_y[i] =
            (int)(pCurrData->HandleData.Drc_v30.Scale_y[i]);
        pAdrcProcRes->DrcProcRes.Drc_v30.compres_y[i] =
            pCurrData->HandleData.Drc_v30.CompressSetting.Manual_curve[i];
    }

    //get sw_drc_gain_y
    CalibrateDrcGainY(&pAdrcProcRes->DrcProcRes, pCurrData->HandleData.Drc_v30.DrcGain.DrcGain,
                      pCurrData->HandleData.Drc_v30.DrcGain.Alpha);

    pAdrcProcRes->DrcProcRes.Drc_v30.wr_cycle = FUNCTION_DISABLE;
    float iir_frame = (float)(MIN(FrameCnt + 1, pCurrData->HandleData.Drc_v30.IIR_frame));
    pAdrcProcRes->DrcProcRes.Drc_v30.iir_weight = (int)(SHIFT6BIT((iir_frame - 1) / iir_frame) + 0.5);

    //sw_drc_delta_scalein FIX
    pAdrcProcRes->DrcProcRes.Drc_v30.delta_scalein = DELTA_SCALEIN_FIX;

    LOG1_ATMO( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AdrcTuningParaProcessing()
 *get handle para by config and current variate
 *****************************************************************************/
void AdrcTuningParaProcessing(AdrcContext_t* pAdrcCtx) {
    LOG1_ATMO("%s:enter!\n", __FUNCTION__);
    pAdrcCtx->CurrData.frameCnt = pAdrcCtx->frameCnt;

    if (CHECK_ISP_HW_V21()) {
        pAdrcCtx->CurrData.frameCnt = pAdrcCtx->frameCnt;

        // para setting
        pAdrcCtx->CurrData.HandleData.Drc_v21.Enable = pAdrcCtx->AdrcProcRes.bDrcEn;
        if (pAdrcCtx->drcAttr.opMode == DRC_OPMODE_AUTO) {
            float CtrlData = pAdrcCtx->CurrData.EnvLv;
            if (pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.CtrlDataType == CTRLDATATYPE_ISO)
                CtrlData = pAdrcCtx->CurrData.ISO;

            // get Drc gain
            pAdrcCtx->CurrData.HandleData.Drc_v21.DrcGain.DrcGain = DrcGetCurrPara(
                CtrlData, pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.CtrlData,
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.DrcGain, ADRC_STEP_MAX);
            pAdrcCtx->CurrData.HandleData.Drc_v21.DrcGain.Alpha = DrcGetCurrPara(
                CtrlData, pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.CtrlData,
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.Alpha, ADRC_STEP_MAX);
            pAdrcCtx->CurrData.HandleData.Drc_v21.DrcGain.Clip = DrcGetCurrPara(
                CtrlData, pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.CtrlData,
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.DrcGain.Clip, ADRC_STEP_MAX);

            // get hi lit
            pAdrcCtx->CurrData.HandleData.Drc_v21.HiLit.Strength = DrcGetCurrPara(
                CtrlData, pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.HiLight.CtrlData,
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.HiLight.Strength, ADRC_STEP_MAX);

            // get local
            pAdrcCtx->CurrData.HandleData.Drc_v21.LocalSetting.LocalData.LocalWeit = DrcGetCurrPara(
                CtrlData,
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData.CtrlData,
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData.LocalWeit,
                ADRC_STEP_MAX);
            pAdrcCtx->CurrData.HandleData.Drc_v21.LocalSetting.LocalData.GlobalContrast =
                DrcGetCurrPara(
                    CtrlData,
                    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData.CtrlData,
                    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData
                        .GlobalContrast,
                    ADRC_STEP_MAX);
            pAdrcCtx->CurrData.HandleData.Drc_v21.LocalSetting.LocalData.LoLitContrast =
                DrcGetCurrPara(
                    CtrlData,
                    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData.CtrlData,
                    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.LocalTMOData
                        .LoLitContrast,
                    ADRC_STEP_MAX);

            pAdrcCtx->CurrData.HandleData.Drc_v21.LocalSetting.curPixWeit =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.curPixWeit;
            pAdrcCtx->CurrData.HandleData.Drc_v21.LocalSetting.preFrameWeit =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.preFrameWeit;
            pAdrcCtx->CurrData.HandleData.Drc_v21.LocalSetting.Range_force_sgm =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.Range_force_sgm;
            pAdrcCtx->CurrData.HandleData.Drc_v21.LocalSetting.Range_sgm_cur =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.Range_sgm_cur;
            pAdrcCtx->CurrData.HandleData.Drc_v21.LocalSetting.Range_sgm_pre =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.Range_sgm_pre;
            pAdrcCtx->CurrData.HandleData.Drc_v21.LocalSetting.Space_sgm_cur =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.Space_sgm_cur;
            pAdrcCtx->CurrData.HandleData.Drc_v21.LocalSetting.Space_sgm_pre =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.LocalTMOSetting.Space_sgm_pre;

            // compress
            pAdrcCtx->CurrData.HandleData.Drc_v21.Compress.Mode =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.CompressSetting.Mode;
            for (int i = 0; i < ADRC_Y_NUM; i++)
                pAdrcCtx->CurrData.HandleData.Drc_v21.Compress.Manual_curve[i] =
                    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.CompressSetting.Manual_curve[i];

            for (int i = 0; i < ADRC_Y_NUM; i++)
                pAdrcCtx->CurrData.HandleData.Drc_v21.Scale_y[i] =
                    pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.Scale_y[i];

            pAdrcCtx->CurrData.HandleData.Drc_v21.Edge_Weit =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.Edge_Weit;
            pAdrcCtx->CurrData.HandleData.Drc_v21.OutPutLongFrame =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.OutPutLongFrame;
            pAdrcCtx->CurrData.HandleData.Drc_v21.IIR_frame =
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.IIR_frame;
            pAdrcCtx->CurrData.ByPassThr = pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.ByPassThr;
            pAdrcCtx->CurrData.Tolerance = pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.Tolerance;
            pAdrcCtx->CurrData.damp      = pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.damp;
        } else if (pAdrcCtx->drcAttr.opMode == DRC_OPMODE_MANU) {
            if (0 != memcmp(&pAdrcCtx->CurrData.HandleData.Drc_v21, &pAdrcCtx->drcAttr.stManualV21,
                            sizeof(mdrcAttr_V21_t)))
                memcpy(&pAdrcCtx->CurrData.HandleData.Drc_v21, &pAdrcCtx->drcAttr.stManualV21,
                       sizeof(mdrcAttr_V21_t));
        }

        // clip drc gain
        if (pAdrcCtx->CurrAeResult.AERatio * pAdrcCtx->CurrData.HandleData.Drc_v21.DrcGain.DrcGain >
            MAX_AE_DRC_GAIN) {
            LOGE_ATMO("%s:  AERatio*DrcGain > 256!!!\n", __FUNCTION__);
            pAdrcCtx->CurrData.HandleData.Drc_v21.DrcGain.DrcGain =
                MAX_AE_DRC_GAIN / pAdrcCtx->CurrAeResult.AERatio;
        }

        LOGD_ATMO(
            "%s:Current Enable:%d DrcGain:%f Alpha:%f Clip:%f Strength:%f LocalWeit:%f "
            "GlobalContrast:%f LoLitContrast:%f CompressMode:%d\n",
            __FUNCTION__, pAdrcCtx->AdrcProcRes.bDrcEn,
            pAdrcCtx->CurrData.HandleData.Drc_v21.DrcGain.DrcGain,
            pAdrcCtx->CurrData.HandleData.Drc_v21.DrcGain.Alpha,
            pAdrcCtx->CurrData.HandleData.Drc_v21.DrcGain.Clip,
            pAdrcCtx->CurrData.HandleData.Drc_v21.HiLit.Strength,
            pAdrcCtx->CurrData.HandleData.Drc_v21.LocalSetting.LocalData.LocalWeit,
            pAdrcCtx->CurrData.HandleData.Drc_v21.LocalSetting.LocalData.GlobalContrast,
            pAdrcCtx->CurrData.HandleData.Drc_v21.LocalSetting.LocalData.LoLitContrast,
            pAdrcCtx->CurrData.HandleData.Drc_v21.Compress.Mode);

        // transfer data to api
        // info
        pAdrcCtx->drcAttr.Info.CtrlInfo.EnvLv = pAdrcCtx->CurrData.EnvLv;
        pAdrcCtx->drcAttr.Info.CtrlInfo.ISO   = pAdrcCtx->CurrData.ISO;
        memcpy(&pAdrcCtx->drcAttr.Info.ValidParamsV21, &pAdrcCtx->CurrData.HandleData.Drc_v21,
               sizeof(mdrcAttr_V21_t));
        stAuto2CalibV21(pAdrcCtx);

        // damp
        AdrcDampingV21(&pAdrcCtx->CurrData, &pAdrcCtx->PrevData, pAdrcCtx->drcAttr.opMode,
                       pAdrcCtx->frameCnt);

        // get tuning proc res
        AdrcGetTuningProcResV21(&pAdrcCtx->AdrcProcRes, &pAdrcCtx->CurrData,
                                pAdrcCtx->SensorInfo.LongFrmMode, pAdrcCtx->FrameNumber,
                                pAdrcCtx->frameCnt);

        // store current handle data to pre data for next loop
        pAdrcCtx->PrevData.EnvLv = pAdrcCtx->CurrData.EnvLv;
        pAdrcCtx->PrevData.ISO   = pAdrcCtx->CurrData.ISO;
        if (0 != memcmp(&pAdrcCtx->PrevData.HandleData, &pAdrcCtx->CurrData.HandleData,
                        sizeof(DrcHandleData_t)))
            memcpy(&pAdrcCtx->PrevData.HandleData, &pAdrcCtx->CurrData.HandleData,
                   sizeof(DrcHandleData_t));
    } else if (CHECK_ISP_HW_V30()) {
        pAdrcCtx->CurrData.frameCnt = pAdrcCtx->frameCnt;

        // para setting
        pAdrcCtx->CurrData.HandleData.Drc_v30.Enable = pAdrcCtx->AdrcProcRes.bDrcEn;
        if (pAdrcCtx->drcAttr.opMode == DRC_OPMODE_AUTO) {
            float CtrlData = pAdrcCtx->CurrData.EnvLv;
            if (pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.CtrlDataType == CTRLDATATYPE_ISO)
                CtrlData = pAdrcCtx->CurrData.ISO;

            // get Drc gain
            pAdrcCtx->CurrData.HandleData.Drc_v30.DrcGain.DrcGain = DrcGetCurrPara(
                CtrlData, pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.CtrlData,
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.DrcGain, ADRC_STEP_MAX);
            pAdrcCtx->CurrData.HandleData.Drc_v30.DrcGain.Alpha = DrcGetCurrPara(
                CtrlData, pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.CtrlData,
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.Alpha, ADRC_STEP_MAX);
            pAdrcCtx->CurrData.HandleData.Drc_v30.DrcGain.Clip = DrcGetCurrPara(
                CtrlData, pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.CtrlData,
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.DrcGain.Clip, ADRC_STEP_MAX);

            // get hi lit
            pAdrcCtx->CurrData.HandleData.Drc_v30.HiLight.Strength = DrcGetCurrPara(
                CtrlData, pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.HiLight.CtrlData,
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.HiLight.Strength, ADRC_STEP_MAX);

            // get local
            pAdrcCtx->CurrData.HandleData.Drc_v30.LocalSetting.LocalData.LocalWeit = DrcGetCurrPara(
                CtrlData, pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.CtrlData,
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LocalWeit,
                ADRC_STEP_MAX);
            pAdrcCtx->CurrData.HandleData.Drc_v30.LocalSetting.LocalData.GlobalContrast =
                DrcGetCurrPara(
                    CtrlData,
                    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.CtrlData,
                    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.GlobalContrast,
                    ADRC_STEP_MAX);
            pAdrcCtx->CurrData.HandleData.Drc_v30.LocalSetting.LocalData.LoLitContrast =
                DrcGetCurrPara(
                    CtrlData,
                    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.CtrlData,
                    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LoLitContrast,
                    ADRC_STEP_MAX);
            pAdrcCtx->CurrData.HandleData.Drc_v30.LocalSetting.LocalData
                .LocalAutoEnable = DrcGetCurrParaInt(
                CtrlData, pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.CtrlData,
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LocalAutoEnable,
                ADRC_STEP_MAX);
            pAdrcCtx->CurrData.HandleData.Drc_v30.LocalSetting.LocalData.LocalAutoWeit =
                DrcGetCurrPara(
                    CtrlData,
                    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.CtrlData,
                    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.LocalData.LocalAutoWeit,
                    ADRC_STEP_MAX);

            pAdrcCtx->CurrData.HandleData.Drc_v30.LocalSetting.curPixWeit =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.curPixWeit;
            pAdrcCtx->CurrData.HandleData.Drc_v30.LocalSetting.preFrameWeit =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.preFrameWeit;
            pAdrcCtx->CurrData.HandleData.Drc_v30.LocalSetting.Range_force_sgm =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.Range_force_sgm;
            pAdrcCtx->CurrData.HandleData.Drc_v30.LocalSetting.Range_sgm_cur =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.Range_sgm_cur;
            pAdrcCtx->CurrData.HandleData.Drc_v30.LocalSetting.Range_sgm_pre =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.Range_sgm_pre;
            pAdrcCtx->CurrData.HandleData.Drc_v30.LocalSetting.Space_sgm_cur =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.Space_sgm_cur;
            pAdrcCtx->CurrData.HandleData.Drc_v30.LocalSetting.Space_sgm_pre =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.LocalSetting.Space_sgm_pre;

            // compress
            pAdrcCtx->CurrData.HandleData.Drc_v30.CompressSetting.Mode =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.CompressSetting.Mode;
            for (int i = 0; i < ADRC_Y_NUM; i++)
                pAdrcCtx->CurrData.HandleData.Drc_v30.CompressSetting.Manual_curve[i] =
                    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.CompressSetting.Manual_curve[i];

            for (int i = 0; i < ADRC_Y_NUM; i++)
                pAdrcCtx->CurrData.HandleData.Drc_v30.Scale_y[i] =
                    pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.Scale_y[i];

            pAdrcCtx->CurrData.HandleData.Drc_v30.Edge_Weit =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.Edge_Weit;
            pAdrcCtx->CurrData.HandleData.Drc_v30.OutPutLongFrame =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.OutPutLongFrame;
            pAdrcCtx->CurrData.HandleData.Drc_v30.IIR_frame =
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.IIR_frame;
            pAdrcCtx->CurrData.ByPassThr = pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.ByPassThr;
            pAdrcCtx->CurrData.Tolerance = pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.Tolerance;
            pAdrcCtx->CurrData.damp      = pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.damp;
        } else if (pAdrcCtx->drcAttr.opMode == DRC_OPMODE_MANU) {
            if (0 != memcmp(&pAdrcCtx->CurrData.HandleData.Drc_v30, &pAdrcCtx->drcAttr.stManualV30,
                            sizeof(mdrcAttr_V30_t)))
                memcpy(&pAdrcCtx->CurrData.HandleData.Drc_v30, &pAdrcCtx->drcAttr.stManualV30,
                       sizeof(mdrcAttr_V30_t));
        }

        // clip drc gain
        if (pAdrcCtx->CurrAeResult.AERatio * pAdrcCtx->CurrData.HandleData.Drc_v30.DrcGain.DrcGain >
            MAX_AE_DRC_GAIN) {
            LOGE_ATMO("%s:  AERatio*DrcGain > 256!!!\n", __FUNCTION__);
            pAdrcCtx->CurrData.HandleData.Drc_v30.DrcGain.DrcGain =
                MAX_AE_DRC_GAIN / pAdrcCtx->CurrAeResult.AERatio;
        }

        LOGD_ATMO("%s: Current Enable:%d DrcGain:%f Alpha:%f Clip:%f Strength:%f CompressMode:%d\n",
                  __FUNCTION__, pAdrcCtx->AdrcProcRes.bDrcEn,
                  pAdrcCtx->CurrData.HandleData.Drc_v30.DrcGain.DrcGain,
                  pAdrcCtx->CurrData.HandleData.Drc_v30.DrcGain.Alpha,
                  pAdrcCtx->CurrData.HandleData.Drc_v30.DrcGain.Clip,
                  pAdrcCtx->CurrData.HandleData.Drc_v30.HiLight.Strength,
                  pAdrcCtx->CurrData.HandleData.Drc_v30.CompressSetting.Mode);
        LOGD_ATMO(
            "%s: Current LocalWeit:%f LocalAutoEnable:%d LocalAutoWeit:%f GlobalContrast:%f "
            "LoLitContrast:%f\n",
            __FUNCTION__, pAdrcCtx->CurrData.HandleData.Drc_v30.LocalSetting.LocalData.LocalWeit,
            pAdrcCtx->CurrData.HandleData.Drc_v30.LocalSetting.LocalData.LocalAutoEnable,
            pAdrcCtx->CurrData.HandleData.Drc_v30.LocalSetting.LocalData.LocalAutoWeit,
            pAdrcCtx->CurrData.HandleData.Drc_v30.LocalSetting.LocalData.GlobalContrast,
            pAdrcCtx->CurrData.HandleData.Drc_v30.LocalSetting.LocalData.LoLitContrast);

        // transfer data to api
        // info
        pAdrcCtx->drcAttr.Info.CtrlInfo.EnvLv = pAdrcCtx->CurrData.EnvLv;
        pAdrcCtx->drcAttr.Info.CtrlInfo.ISO   = pAdrcCtx->CurrData.ISO;
        memcpy(&pAdrcCtx->drcAttr.Info.ValidParamsV30, &pAdrcCtx->CurrData.HandleData.Drc_v30,
               sizeof(mdrcAttr_V30_t));
        stAuto2CalibV30(pAdrcCtx);

        // damp
        AdrcDampingV30(&pAdrcCtx->CurrData, &pAdrcCtx->PrevData, pAdrcCtx->drcAttr.opMode,
                       pAdrcCtx->frameCnt);

        // get io data
        AdrcGetTuningProcResV30(&pAdrcCtx->AdrcProcRes, &pAdrcCtx->CurrData,
                                pAdrcCtx->SensorInfo.LongFrmMode, pAdrcCtx->FrameNumber,
                                pAdrcCtx->frameCnt);

        // store current handle data to pre data for next loop
        pAdrcCtx->PrevData.EnvLv      = pAdrcCtx->CurrData.EnvLv;
        pAdrcCtx->PrevData.ISO        = pAdrcCtx->CurrData.ISO;
        pAdrcCtx->PrevData.MotionCoef = pAdrcCtx->CurrData.MotionCoef;
        if (0 != memcmp(&pAdrcCtx->PrevData.HandleData, &pAdrcCtx->CurrData.HandleData,
                        sizeof(DrcHandleData_t)))
            memcpy(&pAdrcCtx->PrevData.HandleData, &pAdrcCtx->CurrData.HandleData,
                   sizeof(DrcHandleData_t));
    }

    LOG1_ATMO( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AdrcExpoParaProcessing()
 *get handle para by config and current variate
 *****************************************************************************/
void AdrcExpoParaProcessing
(
    AdrcContext_t*     pAdrcCtx,
    DrcExpoData_t*     pExpoData
)
{
    LOG1_ATMO("%s:enter!\n", __FUNCTION__);

    if(CHECK_ISP_HW_V21()) {
        //get sw_drc_compres_scl
        float adrc_gain      = pAdrcCtx->CurrData.HandleData.Drc_v21.DrcGain.DrcGain;
        float log_ratio2 = log(pExpoData->nextRatioLS * adrc_gain) / log(2.0f) + 12;
        float offsetbits_int = (float)(pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_offset_pow2);
        float offsetbits = offsetbits_int * pow(2, MFHDR_LOG_Q_BITS);
        float hdrbits = log_ratio2 * pow(2, MFHDR_LOG_Q_BITS);
        float hdrvalidbits = hdrbits - offsetbits;
        float compres_scl = (12 * pow(2, MFHDR_LOG_Q_BITS * 2)) / hdrvalidbits;
        pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_scl = (int)(compres_scl);

        //get sw_drc_min_ogain
        if (pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.OutPutLongFrame)
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_min_ogain = 1 << 15;
        else {
            float sw_drc_min_ogain = 1 / (pExpoData->nextRatioLS * adrc_gain);
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_min_ogain = (int)(sw_drc_min_ogain * pow(2, 15) + 0.5);
        }

        //get sw_drc_compres_y
        if (pAdrcCtx->CurrData.HandleData.Drc_v21.Compress.Mode == COMPRESS_AUTO) {
            float curveparam, curveparam2, curveparam3, tmp;
            float luma2[17] = { 0, 1024, 2048, 3072, 4096, 5120, 6144, 7168, 8192, 10240, 12288, 14336, 16384, 18432, 20480, 22528, 24576 };
            float curveTable[17];
            float dstbits = ISP_RAW_BIT * pow(2, MFHDR_LOG_Q_BITS);
            float validbits = dstbits - offsetbits;
            for(int i = 0; i < ISP21_DRC_Y_NUM; ++i)
            {
                curveparam = (float)(validbits - 0) / (hdrvalidbits - validbits + pow(2, -6));
                curveparam2 = validbits * (1 + curveparam);
                curveparam3 = hdrvalidbits * curveparam;
                tmp = luma2[i] * hdrvalidbits / 24576;
                curveTable[i] = (tmp * curveparam2 / (tmp + curveparam3));
                pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_y[i] = (int)(curveTable[i]) ;
            }
        }

        LOGV_ATMO("%s: nextRatioLS:%f sw_drc_position:%d sw_drc_compres_scl:%d sw_drc_offset_pow2:%d\n", __FUNCTION__,
                  pExpoData->nextRatioLS, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_position, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_scl,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_offset_pow2);
        LOGV_ATMO("%s: sw_drc_lpdetail_ratio:%d sw_drc_hpdetail_ratio:%d sw_drc_delta_scalein:%d\n", __FUNCTION__, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_lpdetail_ratio,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_hpdetail_ratio, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_delta_scalein);
        LOGV_ATMO("%s: sw_drc_weipre_frame:%d sw_drc_weicur_pix:%d\n", __FUNCTION__,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_weipre_frame, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_weicur_pix);
        LOGV_ATMO("%s: sw_drc_edge_scl:%d sw_drc_motion_scl:%d sw_drc_force_sgm_inv0:%d\n", __FUNCTION__, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_edge_scl,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_motion_scl, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_force_sgm_inv0);
        LOGV_ATMO("%s: sw_drc_space_sgm_inv0:%d sw_drc_space_sgm_inv1:%d\n", __FUNCTION__,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_space_sgm_inv0, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_space_sgm_inv1);
        LOGV_ATMO("%s: sw_drc_range_sgm_inv0:%d sw_drc_range_sgm_inv1:%d\n", __FUNCTION__, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_range_sgm_inv0,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_range_sgm_inv1);
        LOGV_ATMO("%s: sw_drc_weig_bilat:%d sw_drc_weig_maxl:%d\n", __FUNCTION__, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_weig_bilat,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_weig_maxl);
        LOGV_ATMO("%s: sw_drc_min_ogain:%d sw_drc_iir_weight:%d\n", __FUNCTION__, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_min_ogain,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_iir_weight);
        LOGV_ATMO("%s: sw_drc_gain_y: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", __FUNCTION__,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_gain_y[0], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_gain_y[1],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_gain_y[2], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_gain_y[3],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_gain_y[4], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_gain_y[5],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_gain_y[6], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_gain_y[7],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_gain_y[8], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_gain_y[9],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_gain_y[10], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_gain_y[11],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_gain_y[12], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_gain_y[13],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_gain_y[14], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_gain_y[15],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_gain_y[16]);
        LOGV_ATMO("%s: sw_drc_scale_y: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", __FUNCTION__,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_scale_y[0], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_scale_y[1],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_scale_y[2], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_scale_y[3],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_scale_y[4], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_scale_y[5],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_scale_y[6], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_scale_y[7],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_scale_y[8], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_scale_y[9],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_scale_y[10], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_scale_y[11],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_scale_y[12], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_scale_y[13],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_scale_y[14], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_scale_y[15],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_scale_y[16]);
        LOGV_ATMO(
            "%s: CompressMode:%d sw_drc_compres_y: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d "
            "%d\n",
            __FUNCTION__, pAdrcCtx->CurrData.HandleData.Drc_v21.Compress.Mode,
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_y[0],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_y[1],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_y[2],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_y[3],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_y[4],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_y[5],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_y[6],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_y[7],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_y[8],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_y[9],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_y[10],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_y[11],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_y[12],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_y[13],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_y[14],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_y[15],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v21.sw_drc_compres_y[16]);
    }
    else if(CHECK_ISP_HW_V30()) {
        //get sw_drc_compres_scl
        float adrc_gain      = pAdrcCtx->CurrData.HandleData.Drc_v30.DrcGain.DrcGain;
        float log_ratio2 = log(pExpoData->nextRatioLS * adrc_gain) / log(2.0f) + 12;
        float offsetbits_int = (float)(pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.offset_pow2);
        float offsetbits = offsetbits_int * pow(2, MFHDR_LOG_Q_BITS);
        float hdrbits = log_ratio2 * pow(2, MFHDR_LOG_Q_BITS);
        float hdrvalidbits = hdrbits - offsetbits;
        float compres_scl = (12 * pow(2, MFHDR_LOG_Q_BITS * 2)) / hdrvalidbits;
        pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_scl = (int)(compres_scl);

        //get sw_drc_min_ogain
        if (pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.OutPutLongFrame)
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.min_ogain = 1 << 15;
        else {
            float sw_drc_min_ogain = 1 / (pExpoData->nextRatioLS * adrc_gain);
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.min_ogain = (int)(sw_drc_min_ogain * pow(2, 15) + 0.5);
        }

        //get sw_drc_compres_y
        if (pAdrcCtx->CurrData.HandleData.Drc_v30.CompressSetting.Mode == COMPRESS_AUTO) {
            float curveparam, curveparam2, curveparam3, tmp;
            float luma2[17] = { 0, 1024, 2048, 3072, 4096, 5120, 6144, 7168, 8192, 10240, 12288, 14336, 16384, 18432, 20480, 22528, 24576 };
            float curveTable[17];
            float dstbits = ISP_RAW_BIT * pow(2, MFHDR_LOG_Q_BITS);
            float validbits = dstbits - offsetbits;
            for(int i = 0; i < ISP21_DRC_Y_NUM; ++i)
            {
                curveparam = (float)(validbits - 0) / (hdrvalidbits - validbits + pow(2, -6));
                curveparam2 = validbits * (1 + curveparam);
                curveparam3 = hdrvalidbits * curveparam;
                tmp = luma2[i] * hdrvalidbits / 24576;
                curveTable[i] = (tmp * curveparam2 / (tmp + curveparam3));
                pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_y[i] = (int)(curveTable[i]) ;
            }
        }

        LOGV_ATMO("%s: nextRatioLS:%f sw_drc_position:%d sw_drc_compres_scl:%d sw_drc_offset_pow2:%d\n", __FUNCTION__, pExpoData->nextRatioLS,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.position, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_scl, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.offset_pow2);
        LOGV_ATMO("%s: sw_drc_lpdetail_ratio:%d sw_drc_hpdetail_ratio:%d sw_drc_delta_scalein:%d\n", __FUNCTION__, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.lpdetail_ratio,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.hpdetail_ratio, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.delta_scalein);
        LOGV_ATMO("%s: sw_drc_bilat_wt_off:%d sw_drc_weipre_frame:%d sw_drc_weicur_pix:%d\n", __FUNCTION__, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.bilat_wt_off,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.weipre_frame, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.weicur_pix);
        LOGV_ATMO("%s: sw_drc_edge_scl:%d sw_drc_motion_scl:%d sw_drc_force_sgm_inv0:%d\n", __FUNCTION__, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.edge_scl,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.motion_scl, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.force_sgm_inv0);
        LOGV_ATMO("%s: sw_drc_space_sgm_inv0:%d sw_drc_space_sgm_inv1:%d\n", __FUNCTION__,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.space_sgm_inv0, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.space_sgm_inv1);
        LOGV_ATMO("%s: sw_drc_range_sgm_inv0:%d sw_drc_range_sgm_inv1:%d\n", __FUNCTION__, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.range_sgm_inv0,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.range_sgm_inv1);
        LOGV_ATMO("%s: sw_drc_weig_bilat:%d sw_drc_weig_maxl:%d sw_drc_bilat_soft_thd:%d sw_drc_enable_soft_thd:%d\n", __FUNCTION__, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.weig_bilat,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.weig_maxl, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.bilat_soft_thd, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.enable_soft_thd);
        LOGV_ATMO("%s: sw_drc_min_ogain:%d sw_drc_iir_weight:%d\n", __FUNCTION__, pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.min_ogain,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.iir_weight);
        LOGV_ATMO("%s: sw_drc_gain_y: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", __FUNCTION__,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.gain_y[0], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.gain_y[1],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.gain_y[2], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.gain_y[3],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.gain_y[4], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.gain_y[5],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.gain_y[6], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.gain_y[7],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.gain_y[8], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.gain_y[9],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.gain_y[10], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.gain_y[11],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.gain_y[12], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.gain_y[13],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.gain_y[14], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.gain_y[15],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.gain_y[16]);
        LOGV_ATMO("%s: sw_drc_scale_y: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", __FUNCTION__,
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.scale_y[0], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.scale_y[1],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.scale_y[2], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.scale_y[3],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.scale_y[4], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.scale_y[5],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.scale_y[6], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.scale_y[7],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.scale_y[8], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.scale_y[9],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.scale_y[10], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.scale_y[11],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.scale_y[12], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.scale_y[13],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.scale_y[14], pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.scale_y[15],
                  pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.scale_y[16]);
        LOGV_ATMO(
            "%s: CompressMode:%d sw_drc_compres_y: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d "
            "%d\n",
            __FUNCTION__, pAdrcCtx->CurrData.HandleData.Drc_v30.CompressSetting.Mode,
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_y[0],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_y[1],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_y[2],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_y[3],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_y[4],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_y[5],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_y[6],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_y[7],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_y[8],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_y[9],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_y[10],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_y[11],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_y[12],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_y[13],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_y[14],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_y[15],
            pAdrcCtx->AdrcProcRes.DrcProcRes.Drc_v30.compres_y[16]);
    }

    LOG1_ATMO( "%s:exit!\n", __FUNCTION__);
}

/******************************************************************************
 * AdrcByPassProcessing()
 *get handle para by config and current variate
 *****************************************************************************/
bool AdrcByPassProcessing
(
    AdrcContext_t*     pAdrcCtx,
    AecPreResult_t  AecHdrPreResult
)
{
    LOG1_ATMO("%s:enter!\n", __FUNCTION__);

    bool bypass = false;
    float diff = 0.0;
    float ByPassThr = 0.0f;

    // get current EnvLv from AecPreRes
    AdrcGetEnvLv(pAdrcCtx, AecHdrPreResult);

    // motion coef
    pAdrcCtx->CurrData.MotionCoef = MOVE_COEF_DEFAULT;

    // transfer ae data to CurrHandle
    pAdrcCtx->CurrData.EnvLv =
        LIMIT_VALUE(pAdrcCtx->CurrData.EnvLv, ADRCNORMALIZEMAX, ADRCNORMALIZEMIN);

    pAdrcCtx->CurrData.ISO = pAdrcCtx->CurrAeResult.ISO;
    pAdrcCtx->CurrData.ISO = LIMIT_VALUE(pAdrcCtx->CurrData.ISO, ISOMAX, ISOMIN);

    if (pAdrcCtx->frameCnt <= 2)  // start frame
        bypass = false;
    else if (pAdrcCtx->drcAttr.opMode == DRC_OPMODE_MANU)  // api
        bypass = false;
    else if (pAdrcCtx->drcAttr.opMode != pAdrcCtx->PrevData.ApiMode)  // api change
        bypass = false;
    else {  // EnvLv change
        if (CHECK_ISP_HW_V21()) {
            ByPassThr = pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.ByPassThr;
            if (pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.CtrlDataType == CTRLDATATYPE_ENVLV) {
                diff = pAdrcCtx->PrevData.EnvLv - pAdrcCtx->CurrData.EnvLv;
                if (pAdrcCtx->PrevData.EnvLv == 0.0) {
                    diff = pAdrcCtx->CurrData.EnvLv;
                    if (diff == 0.0)
                        bypass = true;
                    else
                        bypass = false;
                } else {
                    diff /= pAdrcCtx->PrevData.EnvLv;
                    if (diff >= ByPassThr || diff <= (0 - ByPassThr))
                        bypass = false;
                    else
                        bypass = true;
                }
            } else if (pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.CtrlDataType == CTRLDATATYPE_ISO) {
                diff = pAdrcCtx->PrevData.ISO - pAdrcCtx->CurrData.ISO;
                diff /= pAdrcCtx->PrevData.ISO;
                if (diff >= ByPassThr || diff <= (0 - ByPassThr))
                    bypass = false;
                else
                    bypass = true;
            }
            LOGD_ATMO(
                "%s: FrameID:%d HDRFrameNum:%d LongFrmMode:%d DRCApiMode:%d CtrlDataType:%d "
                "EnvLv:%f ISO:%f bypass:%d\n",
                __FUNCTION__, pAdrcCtx->frameCnt, pAdrcCtx->FrameNumber,
                pAdrcCtx->SensorInfo.LongFrmMode, pAdrcCtx->drcAttr.opMode,
                pAdrcCtx->drcAttr.stAutoV21.DrcTuningPara.CtrlDataType, pAdrcCtx->CurrData.EnvLv,
                pAdrcCtx->CurrData.ISO, bypass);
        } else if (CHECK_ISP_HW_V30()) {
            ByPassThr = pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.ByPassThr;
            if (pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.CtrlDataType == CTRLDATATYPE_ENVLV) {
                diff = pAdrcCtx->PrevData.EnvLv - pAdrcCtx->CurrData.EnvLv;
                if (pAdrcCtx->PrevData.EnvLv == 0.0) {
                    diff = pAdrcCtx->CurrData.EnvLv;
                    if (diff == 0.0)
                        bypass = true;
                    else
                        bypass = false;
                } else {
                    diff /= pAdrcCtx->PrevData.EnvLv;
                    if (diff >= ByPassThr || diff <= (0 - ByPassThr))
                        bypass = false;
                    else
                        bypass = true;
                }
            } else if (pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.CtrlDataType == CTRLDATATYPE_ISO) {
                diff = pAdrcCtx->PrevData.ISO - pAdrcCtx->CurrData.ISO;
                diff /= pAdrcCtx->PrevData.ISO;
                if (diff >= ByPassThr || diff <= (0 - ByPassThr))
                    bypass = false;
                else
                    bypass = true;
            }
            LOGD_ATMO(
                "%s: FrameID:%d HDRFrameNum:%d LongFrmMode:%d DRCApiMode:%d CtrlDataType:%d "
                "EnvLv:%f ISO:%f bypass:%d\n",
                __FUNCTION__, pAdrcCtx->frameCnt, pAdrcCtx->FrameNumber,
                pAdrcCtx->SensorInfo.LongFrmMode, pAdrcCtx->drcAttr.opMode,
                pAdrcCtx->drcAttr.stAutoV30.DrcTuningPara.CtrlDataType, pAdrcCtx->CurrData.EnvLv,
                pAdrcCtx->CurrData.ISO, bypass);
        }
    }

    LOG1_ATMO( "%s: CtrlEnvLv:%f PrevEnvLv:%f diff:%f ByPassThr:%f opMode:%d bypass:%d!\n", __FUNCTION__, pAdrcCtx->CurrData.EnvLv,
               pAdrcCtx->PrevData.EnvLv, diff, ByPassThr, pAdrcCtx->drcAttr.opMode, bypass);

    LOG1_ATMO( "%s:exit!\n", __FUNCTION__);
    return bypass;
}

/******************************************************************************
 * AdrcInit()
 *****************************************************************************/
XCamReturn AdrcInit
(
    AdrcContext_t **ppAdrcCtx,
    CamCalibDbV2Context_t *pCalibDb
) {
    LOG1_ATMO("%s:enter!\n", __FUNCTION__);
    XCamReturn result = XCAM_RETURN_NO_ERROR;

    AdrcContext_t * pAdrcCtx;
    pAdrcCtx = (AdrcContext_t *)malloc(sizeof(AdrcContext_t));
    if(pAdrcCtx == NULL) {
        LOGE_ATMO("%s(%d): invalid inputparams\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_MEM;
    }

    memset(pAdrcCtx, 0x00, sizeof(AdrcContext_t));
    *ppAdrcCtx = pAdrcCtx;
    pAdrcCtx->state = ADRC_STATE_INITIALIZED;

    if(CHECK_ISP_HW_V21()) {
        CalibDbV2_drc_t* calibv2_adrc_calib =
            (CalibDbV2_drc_t*)(CALIBDBV2_GET_MODULE_PTR(pCalibDb, adrc_calib));

        // pre-initialize context
        memset(pAdrcCtx, 0x00, sizeof(*pAdrcCtx));

        pAdrcCtx->drcAttr.Version = ADRC_VERSION_356X;
        pAdrcCtx->drcAttr.opMode  = DRC_OPMODE_AUTO;
        ConfigV21(pAdrcCtx); //set default para
        pAdrcCtx->Calib.pCalibV21 = calibv2_adrc_calib;  // load iq paras
        Calib2stAutoV21(pAdrcCtx);

    }
    else if(CHECK_ISP_HW_V30()) {
        CalibDbV2_drc_V2_t* calibv2_adrc_calib =
            (CalibDbV2_drc_V2_t*)(CALIBDBV2_GET_MODULE_PTR(pCalibDb, adrc_calib));

        // pre-initialize context
        memset(pAdrcCtx, 0x00, sizeof(*pAdrcCtx));

        pAdrcCtx->drcAttr.Version = ADRC_VERSION_3588;
        pAdrcCtx->drcAttr.opMode  = DRC_OPMODE_AUTO;
        ConfigV30(pAdrcCtx); //set default para
        pAdrcCtx->Calib.pCalibV30 = calibv2_adrc_calib;  // load iq paras
        Calib2stAutoV30(pAdrcCtx);
    }

    LOG1_ATMO( "%s:exit!\n", __FUNCTION__);

    return (XCAM_RETURN_NO_ERROR);
}
/******************************************************************************
 * AhdrRelease()
 *****************************************************************************/
XCamReturn AdrcRelease
(
    AdrcContext_t* pAdrcCtx
) {

    LOG1_ATMO( "%s:enter!\n", __FUNCTION__);
    XCamReturn result = XCAM_RETURN_NO_ERROR;

    // initial checks
    if (NULL == pAdrcCtx) {
        return (XCAM_RETURN_ERROR_PARAM);
    }

    result = AdrcStop(pAdrcCtx);
    if (result != XCAM_RETURN_NO_ERROR) {
        LOGE_ATMO( "%s: AHDRStop() failed!\n", __FUNCTION__);
        return (result);
    }

    // check state
    if ((ADRC_STATE_RUNNING == pAdrcCtx->state)
            || (ADRC_STATE_LOCKED == pAdrcCtx->state)) {
        return (XCAM_RETURN_ERROR_MEM);
    }

    memset(pAdrcCtx, 0, sizeof(AdrcContext_t));
    free(pAdrcCtx);
    pAdrcCtx = NULL;

    LOG1_ATMO( "%s:exit!\n", __FUNCTION__);

    return (XCAM_RETURN_NO_ERROR);
}
