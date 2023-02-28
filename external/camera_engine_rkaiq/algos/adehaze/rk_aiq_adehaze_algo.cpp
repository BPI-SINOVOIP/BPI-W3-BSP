/*
 * rk_aiq_adehaze_algo.cpp
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
#include <string.h>
#include "rk_aiq_adehaze_algo.h"
#include "xcam_log.h"

float  LinearInterp(const float *pX, const float *pY, float posx, int XSize)
{
    int index;
    float yOut = 0;

    if (posx >= pX[XSize - 1])
    {
        yOut = pY[XSize - 1];
    }
    else if (posx <= pX[0])
    {
        yOut = pY[0];
    }
    else
    {
        index = 0;
        while((posx >= pX[index]) && (index < XSize))
        {
            index++;
        }
        index -= 1;
        yOut = ((pY[index + 1] - pY[index]) / (pX[index + 1] - pX[index]) * (posx - pX[index]))
               + pY[index];
    }

    return yOut;
}

int LinearInterpEnable(const float *pX, const unsigned char *pY, float posx, int XSize)
{
    int index;
    float out;
    float yOut = 0;
    if (posx >= pX[XSize - 1])
    {
        out = (float)pY[XSize - 1];
    }
    else if (posx <= pX[0])
    {
        out = pY[0];
    }
    else
    {
        index = 0;
        while((posx >= pX[index]) && (index < XSize))
        {
            index++;
        }
        index -= 1;
        out = ((pY[index + 1] - pY[index]) / (pX[index + 1] - pX[index]) * (posx - pX[index]))
              + pY[index];
    }
    yOut = out > 0.5 ? 1 : 0;

    return yOut;
}

void EnableSettingV21(aDehazeAttrData_t* pCalibV21, RkAiqAdehazeProcResult_t* ProcRes) {
    LOG1_ADEHAZE("ENTER: %s \n", __func__);

    ProcRes->ProcResV21.enable = pCalibV21->Enable;

    bool dehaze_enable = false;
    bool enhance_enable = false;
    if(pCalibV21->Enable) {
        if(pCalibV21->dehaze_setting.en && pCalibV21->enhance_setting.en)
        {
            ProcRes->ProcResV21.dc_en = FUNCTION_ENABLE;
            ProcRes->ProcResV21.enhance_en = FUNCTION_ENABLE;
        }
        else if(pCalibV21->dehaze_setting.en && !pCalibV21->enhance_setting.en)
        {
            ProcRes->ProcResV21.dc_en = FUNCTION_ENABLE;
            ProcRes->ProcResV21.enhance_en = FUNCTION_DISABLE;
        }
        else if(!pCalibV21->dehaze_setting.en && pCalibV21->enhance_setting.en)
        {
            ProcRes->ProcResV21.dc_en = FUNCTION_ENABLE;
            ProcRes->ProcResV21.enhance_en = FUNCTION_ENABLE;
        }
        else
        {
            ProcRes->ProcResV21.dc_en = FUNCTION_DISABLE;
            ProcRes->ProcResV21.enhance_en = FUNCTION_DISABLE;
        }

        if(pCalibV21->hist_setting.en)
            ProcRes->ProcResV21.hist_en = FUNCTION_ENABLE;
        else
            ProcRes->ProcResV21.hist_en = FUNCTION_DISABLE;
    }
    else {
        ProcRes->ProcResV21.dc_en = FUNCTION_DISABLE;
        ProcRes->ProcResV21.enhance_en = FUNCTION_DISABLE;
        ProcRes->ProcResV21.hist_en = FUNCTION_DISABLE;
    }
    dehaze_enable = (ProcRes->ProcResV21.dc_en & FUNCTION_ENABLE) && (!(ProcRes->ProcResV21.enhance_en & FUNCTION_ENABLE));
    enhance_enable = (ProcRes->ProcResV21.dc_en & FUNCTION_ENABLE) && (ProcRes->ProcResV21.enhance_en & FUNCTION_ENABLE);

    LOGD_ADEHAZE(" %s: Dehaze module en:%d Dehaze en:%d, Enhance en:%d, Hist en:%d\n", __func__,
                 ProcRes->ProcResV21.enable, dehaze_enable, enhance_enable, ProcRes->ProcResV21.hist_en);

    LOG1_ADEHAZE("EIXT: %s \n", __func__);
}

void EnableSettingV30(aDehazeAttrData_t* pCalibV21, RkAiqAdehazeProcResult_t* ProcRes) {
    LOG1_ADEHAZE("ENTER: %s \n", __func__);

    ProcRes->ProcResV30.enable = pCalibV21->Enable;

    bool dehaze_enable = false;
    bool enhance_enable = false;
    if(pCalibV21->Enable) {
        if(pCalibV21->dehaze_setting.en && pCalibV21->enhance_setting.en)
        {
            ProcRes->ProcResV30.dc_en = FUNCTION_ENABLE;
            ProcRes->ProcResV30.enhance_en = FUNCTION_ENABLE;
        }
        else if(pCalibV21->dehaze_setting.en && !pCalibV21->enhance_setting.en)
        {
            ProcRes->ProcResV30.dc_en = FUNCTION_ENABLE;
            ProcRes->ProcResV30.enhance_en = FUNCTION_DISABLE;
        }
        else if(!pCalibV21->dehaze_setting.en && pCalibV21->enhance_setting.en)
        {
            ProcRes->ProcResV30.dc_en = FUNCTION_ENABLE;
            ProcRes->ProcResV30.enhance_en = FUNCTION_ENABLE;
        }
        else
        {
            ProcRes->ProcResV30.dc_en = FUNCTION_DISABLE;
            ProcRes->ProcResV30.enhance_en = FUNCTION_DISABLE;
        }

        if(pCalibV21->hist_setting.en)
            ProcRes->ProcResV30.hist_en = FUNCTION_ENABLE;
        else
            ProcRes->ProcResV30.hist_en = FUNCTION_DISABLE;
    }
    else {
        ProcRes->ProcResV30.dc_en = FUNCTION_DISABLE;
        ProcRes->ProcResV30.enhance_en = FUNCTION_DISABLE;
        ProcRes->ProcResV30.hist_en = FUNCTION_DISABLE;
    }
    dehaze_enable = (ProcRes->ProcResV30.dc_en & FUNCTION_ENABLE) && (!(ProcRes->ProcResV30.enhance_en & FUNCTION_ENABLE));
    enhance_enable = (ProcRes->ProcResV30.dc_en & FUNCTION_ENABLE) && (ProcRes->ProcResV30.enhance_en & FUNCTION_ENABLE);

    LOGD_ADEHAZE(" %s: Dehaze module en:%d Dehaze en:%d, Enhance en:%d, Hist en:%d\n", __func__,
                 ProcRes->ProcResV30.enable, dehaze_enable, enhance_enable, ProcRes->ProcResV30.hist_en);

    LOG1_ADEHAZE("EIXT: %s \n", __func__);
}

void stManuEnableSettingV21(mDehazeAttr_t* pStManu, RkAiqAdehazeProcResult_t* ProcRes)
{
    LOG1_ADEHAZE("ENTER: %s \n", __func__);

    ProcRes->ProcResV21.enable = pStManu->Enable;

    bool dehaze_enable = false;
    bool enhance_enable = false;
    if(pStManu->Enable) {
        if(pStManu->dehaze_setting.en && pStManu->enhance_setting.en)
        {
            ProcRes->ProcResV21.dc_en = FUNCTION_ENABLE;
            ProcRes->ProcResV21.enhance_en = FUNCTION_ENABLE;
        }
        else if(pStManu->dehaze_setting.en && !pStManu->enhance_setting.en)
        {
            ProcRes->ProcResV21.dc_en = FUNCTION_ENABLE;
            ProcRes->ProcResV21.enhance_en = FUNCTION_DISABLE;
        }
        else if(!pStManu->dehaze_setting.en && pStManu->enhance_setting.en)
        {
            ProcRes->ProcResV21.dc_en = FUNCTION_ENABLE;
            ProcRes->ProcResV21.enhance_en = FUNCTION_ENABLE;
        }
        else
        {
            ProcRes->ProcResV21.dc_en = FUNCTION_DISABLE;
            ProcRes->ProcResV21.enhance_en = FUNCTION_DISABLE;
        }

        if(pStManu->hist_setting.en)
            ProcRes->ProcResV21.hist_en = FUNCTION_ENABLE;
        else
            ProcRes->ProcResV21.hist_en = FUNCTION_DISABLE;
    }
    else {
        ProcRes->ProcResV21.dc_en = FUNCTION_DISABLE;
        ProcRes->ProcResV21.enhance_en = FUNCTION_DISABLE;
        ProcRes->ProcResV21.hist_en = FUNCTION_DISABLE;
    }
    dehaze_enable = (ProcRes->ProcResV21.dc_en & FUNCTION_ENABLE) && (!(ProcRes->ProcResV21.enhance_en & FUNCTION_ENABLE));
    enhance_enable = (ProcRes->ProcResV21.dc_en & FUNCTION_ENABLE) && (ProcRes->ProcResV21.enhance_en & FUNCTION_ENABLE);

    LOGD_ADEHAZE(" %s: Dehaze module en:%d Dehaze en:%d, Enhance en:%d, Hist en:%d\n", __func__,
                 ProcRes->ProcResV21.enable, dehaze_enable, enhance_enable, ProcRes->ProcResV21.hist_en);

    LOG1_ADEHAZE("EIXT: %s \n", __func__);
}

void stManuEnableSettingV30(mDehazeAttr_t* pStManu, RkAiqAdehazeProcResult_t* ProcRes)
{
    LOG1_ADEHAZE("ENTER: %s \n", __func__);

    ProcRes->ProcResV30.enable = pStManu->Enable;

    bool dehaze_enable = false;
    bool enhance_enable = false;
    if(pStManu->Enable) {
        if(pStManu->dehaze_setting.en && pStManu->enhance_setting.en)
        {
            ProcRes->ProcResV30.dc_en = FUNCTION_ENABLE;
            ProcRes->ProcResV30.enhance_en = FUNCTION_ENABLE;
        }
        else if(pStManu->dehaze_setting.en && !pStManu->enhance_setting.en)
        {
            ProcRes->ProcResV30.dc_en = FUNCTION_ENABLE;
            ProcRes->ProcResV30.enhance_en = FUNCTION_DISABLE;
        }
        else if(!pStManu->dehaze_setting.en && pStManu->enhance_setting.en)
        {
            ProcRes->ProcResV30.dc_en = FUNCTION_ENABLE;
            ProcRes->ProcResV30.enhance_en = FUNCTION_ENABLE;
        }
        else
        {
            ProcRes->ProcResV30.dc_en = FUNCTION_DISABLE;
            ProcRes->ProcResV30.enhance_en = FUNCTION_DISABLE;
        }

        if(pStManu->hist_setting.en)
            ProcRes->ProcResV30.hist_en = FUNCTION_ENABLE;
        else
            ProcRes->ProcResV30.hist_en = FUNCTION_DISABLE;
    }
    else {
        ProcRes->ProcResV30.dc_en = FUNCTION_DISABLE;
        ProcRes->ProcResV30.enhance_en = FUNCTION_DISABLE;
        ProcRes->ProcResV30.hist_en = FUNCTION_DISABLE;
    }
    dehaze_enable = (ProcRes->ProcResV30.dc_en & FUNCTION_ENABLE) && (!(ProcRes->ProcResV30.enhance_en & FUNCTION_ENABLE));
    enhance_enable = (ProcRes->ProcResV30.dc_en & FUNCTION_ENABLE) && (ProcRes->ProcResV30.enhance_en & FUNCTION_ENABLE);

    LOGD_ADEHAZE(" %s: Dehaze module en:%d Dehaze en:%d, Enhance en:%d, Hist en:%d\n", __func__,
                 ProcRes->ProcResV30.enable, dehaze_enable, enhance_enable, ProcRes->ProcResV30.hist_en);

    LOG1_ADEHAZE("EIXT: %s \n", __func__);
}

void stManuGetDehazeParamsV21(mDehazeAttr_t* pStManu, int DehazeLevel,
                              RkAiqAdehazeProcResult_t* ProcRes) {
    LOG1_ADEHAZE("ENTER: %s \n", __func__);

    bool air_lc_en = pStManu->dehaze_setting.air_lc_en;

    // dehaze_self_adp[7]
    float dc_min_th = pStManu->dehaze_setting.DehazeData.dc_min_th;
    float dc_max_th = pStManu->dehaze_setting.DehazeData.dc_max_th;
    float yhist_th = pStManu->dehaze_setting.DehazeData.yhist_th;
    float yblk_th = pStManu->dehaze_setting.DehazeData.yblk_th;
    float dark_th = pStManu->dehaze_setting.DehazeData.dark_th;
    float bright_min = pStManu->dehaze_setting.DehazeData.bright_min;
    float bright_max = pStManu->dehaze_setting.DehazeData.bright_max;

    // dehaze_range_adj[6]
    float wt_max = pStManu->dehaze_setting.DehazeData.wt_max;
    float air_max = pStManu->dehaze_setting.DehazeData.air_max;
    float air_min = pStManu->dehaze_setting.DehazeData.air_min;
    float tmax_base = pStManu->dehaze_setting.DehazeData.tmax_base;
    float tmax_off = pStManu->dehaze_setting.DehazeData.tmax_off;
    float tmax_max = pStManu->dehaze_setting.DehazeData.tmax_max;

    // dehaze_iir_control[5]
    float stab_fnum = pStManu->dehaze_setting.stab_fnum;
    float sigma = pStManu->dehaze_setting.sigma;
    float wt_sigma = pStManu->dehaze_setting.wt_sigma;
    float air_sigma = pStManu->dehaze_setting.air_sigma;
    float tmax_sigma = pStManu->dehaze_setting.tmax_sigma;
    float pre_wet = pStManu->dehaze_setting.pre_wet;

    float cfg_wt = pStManu->dehaze_setting.DehazeData.cfg_wt;
    float cfg_air = pStManu->dehaze_setting.DehazeData.cfg_air;
    float cfg_tmax = pStManu->dehaze_setting.DehazeData.cfg_tmax;

    float range_sigma = pStManu->dehaze_setting.DehazeData.range_sigma;
    float space_sigma_cur = pStManu->dehaze_setting.DehazeData.space_sigma_cur;
    float space_sigma_pre = pStManu->dehaze_setting.DehazeData.space_sigma_pre;

    // dehaze_bi_pAdehazeCtx[4]
    float bf_weight = pStManu->dehaze_setting.DehazeData.bf_weight;
    float dc_weitcur = pStManu->dehaze_setting.DehazeData.dc_weitcur;

    // dehaze_air_bf_h[9],dehaze_gaus_h[9]
    float gaus_h[9] = {2.0000, 4.0000, 2.0000,
                       4.0000, 8.0000, 4.0000,
                       2.0000, 4.0000, 2.0000
                      };

    int rawWidth = 1920;
    int rawHeight = 1080;
    ProcRes->ProcResV21.air_lc_en    = air_lc_en ? 1 : 0; // air_lc_en
    ProcRes->ProcResV21.dc_min_th    = int(dc_min_th); //0~255, (8bit) dc_min_th
    ProcRes->ProcResV21.dc_max_th    = int(dc_max_th);  //0~255, (8bit) dc_max_th
    ProcRes->ProcResV21.yhist_th    = int(yhist_th);  //0~255, (8bit) yhist_th
    ProcRes->ProcResV21.yblk_th    = int(yblk_th * ((rawWidth + 15) / 16) * ((rawHeight + 15) / 16)); //default:28,(9bit) yblk_th
    ProcRes->ProcResV21.dark_th    = int(dark_th);  //0~255, (8bit) dark_th
    ProcRes->ProcResV21.bright_min   = int(bright_min);  //0~255, (8bit) bright_min
    ProcRes->ProcResV21.bright_max   = int(bright_max);  //0~255, (8bit) bright_max
    ProcRes->ProcResV21.wt_max   = int(wt_max * 256); //0~255, (8bit) wt_max
    ProcRes->ProcResV21.air_min   = int(air_min);  //0~255, (8bit) air_min
    ProcRes->ProcResV21.air_max   = int(air_max);  //0~256, (8bit) air_max
    ProcRes->ProcResV21.tmax_base   = int(tmax_base);  //0~255, (8bit) tmax_base
    ProcRes->ProcResV21.tmax_off   = int(tmax_off * 1024); //0~1024,(10bit) tmax_off
    ProcRes->ProcResV21.tmax_max   = int(tmax_max * 1024); //0~1024,(10bit) tmax_max
    ProcRes->ProcResV21.stab_fnum = int(stab_fnum);  //1~31,  (5bit) stab_fnum
    ProcRes->ProcResV21.iir_sigma = int(sigma);  //0~255, (8bit) sigma
    ProcRes->ProcResV21.iir_wt_sigma = int(wt_sigma * 8 + 0.5); //       (11bit),8bit+3bit, wt_sigma
    ProcRes->ProcResV21.iir_air_sigma = int(air_sigma);  //       (8bit) air_sigma
    ProcRes->ProcResV21.iir_tmax_sigma = int(tmax_sigma * 1024 + 0.5);  //       (11bit) tmax_sigma
    ProcRes->ProcResV21.iir_pre_wet = int(pre_wet * 15 + 0.5);  //       (4bit) iir_pre_wet
    ProcRes->ProcResV21.cfg_wt = int(cfg_wt * 256); //0~256, (9bit) cfg_wt
    ProcRes->ProcResV21.cfg_air = int(cfg_air);  //0~255, (8bit) cfg_air
    ProcRes->ProcResV21.cfg_tmax = int(cfg_tmax * 1024); //0~1024,(11bit) cfg_tmax
    ProcRes->ProcResV21.range_sima = int(range_sigma * 512); //0~512,(9bit) range_sima
    ProcRes->ProcResV21.space_sigma_cur = int(space_sigma_cur * 256); //0~256,(8bit) space_sigma_cur
    ProcRes->ProcResV21.space_sigma_pre = int(space_sigma_pre * 256); //0~256,(8bit) space_sigma_pre
    ProcRes->ProcResV21.bf_weight      = int(bf_weight * 256); //0~512, (8bit) dc_thed
    ProcRes->ProcResV21.dc_weitcur       = int(dc_weitcur * 256 + 0.5); //0~256, (9bit) dc_weitcur
    ProcRes->ProcResV21.gaus_h0    = int(gaus_h[4]);//h0~h2  浠庡ぇ鍒板皬
    ProcRes->ProcResV21.gaus_h1    = int(gaus_h[1]);
    ProcRes->ProcResV21.gaus_h2    = int(gaus_h[0]);

    if (DehazeLevel != DEHAZE_API_MANUAL_DEFAULT_LEVEL) {
        ProcRes->ProcResV21.cfg_alpha = 255;
        float level_diff              = (float)(DehazeLevel - DEHAZE_API_MANUAL_DEFAULT_LEVEL);

        // sw_dhaz_cfg_wt
        float sw_dhaz_cfg_wt = (float)ProcRes->ProcResV21.cfg_wt;
        sw_dhaz_cfg_wt += level_diff * 0.005 * 256;
        sw_dhaz_cfg_wt             = LIMIT_VALUE(sw_dhaz_cfg_wt, 256, 1);
        ProcRes->ProcResV21.cfg_wt = (int)sw_dhaz_cfg_wt;

        // sw_dhaz_cfg_air
        float sw_dhaz_cfg_air = (float)ProcRes->ProcResV21.cfg_air;
        sw_dhaz_cfg_air += level_diff * 0.5;
        sw_dhaz_cfg_air             = LIMIT_VALUE(sw_dhaz_cfg_air, 255, 1);
        ProcRes->ProcResV21.cfg_air = (int)sw_dhaz_cfg_air;

        // sw_dhaz_cfg_tmax
        float sw_dhaz_cfg_tmax = (float)ProcRes->ProcResV21.cfg_tmax;
        sw_dhaz_cfg_tmax += level_diff * 0.005 * 1024;
        sw_dhaz_cfg_tmax             = LIMIT_VALUE(sw_dhaz_cfg_tmax, 1023, 1);
        ProcRes->ProcResV21.cfg_tmax = (int)sw_dhaz_cfg_tmax;

        LOGD_ADEHAZE(" %s: Adehaze munual level:%f level_diff:%f\n", __func__, DehazeLevel,
                     level_diff);
    }

    if(ProcRes->ProcResV21.dc_en && !(ProcRes->ProcResV21.enhance_en)) {
        if(ProcRes->ProcResV21.cfg_alpha == 255) {
            LOGD_ADEHAZE("%s cfg_alpha:255 cfg_air:%f cfg_tmax:%f cfg_wt:%f\n", __func__, cfg_air, cfg_tmax, cfg_wt);
            LOGD_ADEHAZE("%s cfg_alpha_reg:0x0 cfg_air:0x%x cfg_tmax:0x%x cfg_wt:0x%x\n", __func__, ProcRes->ProcResV21.cfg_air, ProcRes->ProcResV21.cfg_tmax,
                         ProcRes->ProcResV21.cfg_wt);
        }
        else if(ProcRes->ProcResV21.cfg_alpha == 0) {
            LOGD_ADEHAZE("%s cfg_alpha:0 air_max:%f air_min:%f tmax_base:%f wt_max:%f\n", __func__, air_max, air_min, tmax_base, wt_max);
            LOGD_ADEHAZE("%s cfg_alpha_reg:0x0 air_max:0x%x air_min:0x%x tmax_base:0x%x wt_max:0x%x\n", __func__, ProcRes->ProcResV21.air_max, ProcRes->ProcResV21.air_min,
                         ProcRes->ProcResV21.tmax_base, ProcRes->ProcResV21.wt_max);
        }
    }

    LOG1_ADEHAZE("EIXT: %s \n", __func__);
}

void stManuGetDehazeParamsV30(mDehazeAttr_t* pStManu, int DehazeLevel,
                              RkAiqAdehazeProcResult_t* ProcRes) {
    LOG1_ADEHAZE("ENTER: %s \n", __func__);

    bool air_lc_en = pStManu->dehaze_setting.air_lc_en;

    // dehaze_self_adp[7]
    float dc_min_th = pStManu->dehaze_setting.DehazeData.dc_min_th;
    float dc_max_th = pStManu->dehaze_setting.DehazeData.dc_max_th;
    float yhist_th = pStManu->dehaze_setting.DehazeData.yhist_th;
    float yblk_th = pStManu->dehaze_setting.DehazeData.yblk_th;
    float dark_th = pStManu->dehaze_setting.DehazeData.dark_th;
    float bright_min = pStManu->dehaze_setting.DehazeData.bright_min;
    float bright_max = pStManu->dehaze_setting.DehazeData.bright_max;

    // dehaze_range_adj[6]
    float wt_max = pStManu->dehaze_setting.DehazeData.wt_max;
    float air_max = pStManu->dehaze_setting.DehazeData.air_max;
    float air_min = pStManu->dehaze_setting.DehazeData.air_min;
    float tmax_base = pStManu->dehaze_setting.DehazeData.tmax_base;
    float tmax_off = pStManu->dehaze_setting.DehazeData.tmax_off;
    float tmax_max = pStManu->dehaze_setting.DehazeData.tmax_max;

    // dehaze_iir_control[5]
    float stab_fnum = pStManu->dehaze_setting.stab_fnum;
    float sigma = pStManu->dehaze_setting.sigma;
    float wt_sigma = pStManu->dehaze_setting.wt_sigma;
    float air_sigma = pStManu->dehaze_setting.air_sigma;
    float tmax_sigma = pStManu->dehaze_setting.tmax_sigma;
    float pre_wet = pStManu->dehaze_setting.pre_wet;

    float cfg_wt = pStManu->dehaze_setting.DehazeData.cfg_wt;
    float cfg_air = pStManu->dehaze_setting.DehazeData.cfg_air;
    float cfg_tmax = pStManu->dehaze_setting.DehazeData.cfg_tmax;

    float range_sigma = pStManu->dehaze_setting.DehazeData.range_sigma;
    float space_sigma_cur = pStManu->dehaze_setting.DehazeData.space_sigma_cur;
    float space_sigma_pre = pStManu->dehaze_setting.DehazeData.space_sigma_pre;

    // dehaze_bi_pAdehazeCtx[4]
    float bf_weight = pStManu->dehaze_setting.DehazeData.bf_weight;
    float dc_weitcur = pStManu->dehaze_setting.DehazeData.dc_weitcur;

    // dehaze_air_bf_h[9],dehaze_gaus_h[9]
    float gaus_h[9] = {2.0000, 4.0000, 2.0000,
                       4.0000, 8.0000, 4.0000,
                       2.0000, 4.0000, 2.0000
                      };

    int rawWidth = 1920;
    int rawHeight = 1080;
    ProcRes->ProcResV30.air_lc_en    = air_lc_en ? 1 : 0; // air_lc_en
    ProcRes->ProcResV30.dc_min_th    = int(dc_min_th); //0~255, (8bit) dc_min_th
    ProcRes->ProcResV30.dc_max_th    = int(dc_max_th);  //0~255, (8bit) dc_max_th
    ProcRes->ProcResV30.yhist_th    = int(yhist_th);  //0~255, (8bit) yhist_th
    ProcRes->ProcResV30.yblk_th    = int(yblk_th * ((rawWidth + 15) / 16) * ((rawHeight + 15) / 16)); //default:28,(9bit) yblk_th
    ProcRes->ProcResV30.dark_th    = int(dark_th);  //0~255, (8bit) dark_th
    ProcRes->ProcResV30.bright_min   = int(bright_min);  //0~255, (8bit) bright_min
    ProcRes->ProcResV30.bright_max   = int(bright_max);  //0~255, (8bit) bright_max
    ProcRes->ProcResV30.wt_max   = int(wt_max * 256); //0~255, (8bit) wt_max
    ProcRes->ProcResV30.air_min   = int(air_min);  //0~255, (8bit) air_min
    ProcRes->ProcResV30.air_max   = int(air_max);  //0~256, (8bit) air_max
    ProcRes->ProcResV30.tmax_base   = int(tmax_base);  //0~255, (8bit) tmax_base
    ProcRes->ProcResV30.tmax_off   = int(tmax_off * 1024); //0~1024,(10bit) tmax_off
    ProcRes->ProcResV30.tmax_max   = int(tmax_max * 1024); //0~1024,(10bit) tmax_max
    ProcRes->ProcResV30.stab_fnum = int(stab_fnum);  //1~31,  (5bit) stab_fnum
    ProcRes->ProcResV30.iir_sigma = int(sigma);  //0~255, (8bit) sigma
    ProcRes->ProcResV30.iir_wt_sigma = int(wt_sigma * 8 + 0.5); //       (11bit),8bit+3bit, wt_sigma
    ProcRes->ProcResV30.iir_air_sigma = int(air_sigma);  //       (8bit) air_sigma
    ProcRes->ProcResV30.iir_tmax_sigma = int(tmax_sigma * 1024 + 0.5);  //       (11bit) tmax_sigma
    ProcRes->ProcResV30.iir_pre_wet = int(pre_wet * 15 + 0.5);  //       (4bit) iir_pre_wet
    ProcRes->ProcResV30.cfg_wt = int(cfg_wt * 256); //0~256, (9bit) cfg_wt
    ProcRes->ProcResV30.cfg_air = int(cfg_air);  //0~255, (8bit) cfg_air
    ProcRes->ProcResV30.cfg_tmax = int(cfg_tmax * 1024); //0~1024,(11bit) cfg_tmax
    ProcRes->ProcResV30.range_sima = int(range_sigma * 512); //0~512,(9bit) range_sima
    ProcRes->ProcResV30.space_sigma_cur = int(space_sigma_cur * 256); //0~256,(8bit) space_sigma_cur
    ProcRes->ProcResV30.space_sigma_pre = int(space_sigma_pre * 256); //0~256,(8bit) space_sigma_pre
    ProcRes->ProcResV30.bf_weight      = int(bf_weight * 256); //0~512, (8bit) dc_thed
    ProcRes->ProcResV30.dc_weitcur       = int(dc_weitcur * 256 + 0.5); //0~256, (9bit) dc_weitcur
    ProcRes->ProcResV30.gaus_h0    = int(gaus_h[4]);//h0~h2  浠庡ぇ鍒板皬
    ProcRes->ProcResV30.gaus_h1    = int(gaus_h[1]);
    ProcRes->ProcResV30.gaus_h2    = int(gaus_h[0]);

    if (DehazeLevel != DEHAZE_API_MANUAL_DEFAULT_LEVEL) {
        ProcRes->ProcResV30.cfg_alpha = 255;
        float level_diff              = (float)(DehazeLevel - DEHAZE_API_MANUAL_DEFAULT_LEVEL);

        // sw_dhaz_cfg_wt
        float sw_dhaz_cfg_wt = (float)ProcRes->ProcResV30.cfg_wt;
        sw_dhaz_cfg_wt += level_diff * 0.005 * 256;
        sw_dhaz_cfg_wt             = LIMIT_VALUE(sw_dhaz_cfg_wt, 256, 1);
        ProcRes->ProcResV30.cfg_wt = (int)sw_dhaz_cfg_wt;

        // sw_dhaz_cfg_air
        float sw_dhaz_cfg_air = (float)ProcRes->ProcResV30.cfg_air;
        sw_dhaz_cfg_air += level_diff * 0.5;
        sw_dhaz_cfg_air             = LIMIT_VALUE(sw_dhaz_cfg_air, 255, 1);
        ProcRes->ProcResV30.cfg_air = (int)sw_dhaz_cfg_air;

        // sw_dhaz_cfg_tmax
        float sw_dhaz_cfg_tmax = (float)ProcRes->ProcResV30.cfg_tmax;
        sw_dhaz_cfg_tmax += level_diff * 0.005 * 1024;
        sw_dhaz_cfg_tmax             = LIMIT_VALUE(sw_dhaz_cfg_tmax, 1023, 1);
        ProcRes->ProcResV30.cfg_tmax = (int)sw_dhaz_cfg_tmax;

        LOGD_ADEHAZE(" %s: Adehaze munual level:%f level_diff:%f\n", __func__, DehazeLevel,
                     level_diff);
    }

    if(ProcRes->ProcResV30.dc_en && !(ProcRes->ProcResV30.enhance_en)) {
        if(ProcRes->ProcResV30.cfg_alpha == 255) {
            LOGD_ADEHAZE("%s cfg_alpha:255 cfg_air:%f cfg_tmax:%f cfg_wt:%f\n", __func__, cfg_air, cfg_tmax, cfg_wt);
            LOGD_ADEHAZE("%s cfg_alpha_reg:0x0 cfg_air:0x%x cfg_tmax:0x%x cfg_wt:0x%x\n", __func__, ProcRes->ProcResV30.cfg_air, ProcRes->ProcResV30.cfg_tmax,
                         ProcRes->ProcResV30.cfg_wt);
        }
        else if(ProcRes->ProcResV30.cfg_alpha == 0) {
            LOGD_ADEHAZE("%s cfg_alpha:0 air_max:%f air_min:%f tmax_base:%f wt_max:%f\n", __func__, air_max, air_min, tmax_base, wt_max);
            LOGD_ADEHAZE("%s cfg_alpha_reg:0x0 air_max:0x%x air_min:0x%x tmax_base:0x%x wt_max:0x%x\n", __func__, ProcRes->ProcResV30.air_max, ProcRes->ProcResV30.air_min,
                         ProcRes->ProcResV30.tmax_base, ProcRes->ProcResV30.wt_max);
        }
    }

    LOG1_ADEHAZE("EIXT: %s \n", __func__);
}

void stManuGetEnhanceParamsV21(mDehazeAttr_t* pStManu, int EnhanceLevel,
                               RkAiqAdehazeProcResult_t* ProcRes) {
    LOG1_ADEHAZE("ENTER: %s \n", __func__);

    ProcRes->ProcResV21.enhance_value =
        int(pStManu->enhance_setting.EnhanceData.enhance_value * 1024 + 0.5);
    if (EnhanceLevel != DEHAZE_API_ENHANCE_MANUAL_DEFAULT_LEVEL) {
        ProcRes->ProcResV21.enhance_value =
            ProcRes->ProcResV21.enhance_value +
            50 * (EnhanceLevel - DEHAZE_API_ENHANCE_MANUAL_DEFAULT_LEVEL);
    }
    ProcRes->ProcResV21.enhance_value  = LIMIT_VALUE(ProcRes->ProcResV21.enhance_value, 0x3fff, 0);
    ProcRes->ProcResV21.enhance_chroma = LIMIT_VALUE(
        int(pStManu->enhance_setting.EnhanceData.enhance_chroma * 1024.0f + 0.5), 0x3fff, 0);

    for(int i = 0; i < ISP21_DHAZ_ENH_CURVE_NUM; i++)
        ProcRes->ProcResV21.enh_curve[i] = (int)(pStManu->enhance_setting.enhance_curve[i]);

    if(ProcRes->ProcResV21.dc_en && ProcRes->ProcResV21.enhance_en) {
        LOGD_ADEHAZE("%s enhance_value:%f enhance_chroma:%f\n", __func__,
                     ProcRes->ProcResV21.enhance_value / 1024.0f,
                     ProcRes->ProcResV21.enhance_chroma / 1024.0f);
        LOGD_ADEHAZE("%s enhance_value_reg:0x%x enhance_chroma_reg:0x%x\n", __func__,
                     ProcRes->ProcResV21.enhance_value, ProcRes->ProcResV21.enhance_chroma);
    }

    LOG1_ADEHAZE("EIXT: %s \n", __func__);
}

void stManuGetEnhanceParamsV30(mDehazeAttr_t* pStManu, int EnhanceLevel,
                               RkAiqAdehazeProcResult_t* ProcRes) {
    LOG1_ADEHAZE("ENTER: %s \n", __func__);

    ProcRes->ProcResV30.enhance_value =
        int(pStManu->enhance_setting.EnhanceData.enhance_value * 1024 + 0.5);
    if (EnhanceLevel != DEHAZE_API_ENHANCE_MANUAL_DEFAULT_LEVEL) {
        ProcRes->ProcResV30.enhance_value =
            ProcRes->ProcResV30.enhance_value +
            50 * (EnhanceLevel - DEHAZE_API_ENHANCE_MANUAL_DEFAULT_LEVEL);
    }
    ProcRes->ProcResV30.enhance_value  = LIMIT_VALUE(ProcRes->ProcResV30.enhance_value, 0x3fff, 0);
    ProcRes->ProcResV30.enhance_chroma = LIMIT_VALUE(
        int(pStManu->enhance_setting.EnhanceData.enhance_chroma * 1024.0f + 0.5), 0x3fff, 0);

    for(int i = 0; i < ISP21_DHAZ_ENH_CURVE_NUM; i++)
        ProcRes->ProcResV30.enh_curve[i] = (int)(pStManu->enhance_setting.enhance_curve[i]);

    if(ProcRes->ProcResV30.dc_en && ProcRes->ProcResV30.enhance_en) {
        LOGD_ADEHAZE("%s enhance_value:%f enhance_chroma:%f\n", __func__,
                     ProcRes->ProcResV30.enhance_value / 1024.0f,
                     ProcRes->ProcResV30.enhance_chroma / 1024.0f);
        LOGD_ADEHAZE("%s enhance_value_reg:0x%x enhance_chroma_reg:0x%x\n", __func__,
                     ProcRes->ProcResV30.enhance_value, ProcRes->ProcResV30.enhance_chroma);
    }

    LOG1_ADEHAZE("EIXT: %s \n", __func__);
}

void stManuGetHistParamsV21(mDehazeAttr_t* pStManu, RkAiqAdehazeProcResult_t* ProcRes)
{
    LOG1_ADEHAZE("ENTER: %s \n", __func__);

    bool hist_para_en = pStManu->hist_setting.hist_para_en;
    float hist_gratio = pStManu->hist_setting.HistData.hist_gratio;
    float hist_th_off = pStManu->hist_setting.HistData.hist_th_off;
    float hist_k = pStManu->hist_setting.HistData.hist_k;
    float hist_min = pStManu->hist_setting.HistData.hist_min;
    float hist_scale = pStManu->hist_setting.HistData.hist_scale;
    float cfg_gratio = pStManu->hist_setting.HistData.cfg_gratio;

    ProcRes->ProcResV21.hpara_en = hist_para_en ? FUNCTION_ENABLE : FUNCTION_DISABLE; //  hist_para_en
    //clip hpara_en
    ProcRes->ProcResV21.hpara_en = ProcRes->ProcResV21.dc_en ? ProcRes->ProcResV21.hpara_en : FUNCTION_ENABLE; //  dc en 关闭，hpara必需开

    ProcRes->ProcResV21.hist_gratio   = int(hist_gratio * 8); //       (8bit) hist_gratio
    ProcRes->ProcResV21.hist_th_off   = int(hist_th_off);  //       (8bit) hist_th_off
    ProcRes->ProcResV21.hist_k   = int(hist_k * 4 + 0.5); //0~7    (5bit),3bit+2bit, hist_k
    ProcRes->ProcResV21.hist_min   = int(hist_min * 256); //       (9bit) hist_min
    ProcRes->ProcResV21.cfg_gratio = int(cfg_gratio * 256); //       (13bit),5bit+8bit, cfg_gratio
    ProcRes->ProcResV21.hist_scale       = int(hist_scale *  256 + 0.5 );  //       (13bit),5bit + 8bit, sw_hist_scale

    if(ProcRes->ProcResV21.hist_en) {
        LOGD_ADEHAZE("%s cfg_alpha:%f hist_para_en:%d hist_gratio:%f hist_th_off:%f hist_k:%f hist_min:%f hist_scale:%f cfg_gratio:%f\n", __func__,
                     ProcRes->ProcResV21.cfg_alpha / 255.0, ProcRes->ProcResV21.hpara_en, hist_gratio, hist_th_off, hist_k, hist_min, hist_scale, cfg_gratio);
        LOGD_ADEHAZE("%s cfg_alpha_reg:0x%x hist_gratio_reg:0x%x hist_th_off_reg:0x%x hist_k_reg:0x%x hist_min_reg:0x%x hist_scale_reg:0x%x cfg_gratio_reg:0x%x\n", __func__,
                     ProcRes->ProcResV21.cfg_alpha, ProcRes->ProcResV21.hist_gratio, ProcRes->ProcResV21.hist_th_off, ProcRes->ProcResV21.hist_k,
                     ProcRes->ProcResV21.hist_min, ProcRes->ProcResV21.hist_scale, ProcRes->ProcResV21.cfg_gratio);
    }

    LOG1_ADEHAZE("EIXT: %s \n", __func__);
}

void stManuGetHistParamsV30(mDehazeAttr_t* pStManu, RkAiqAdehazeProcResult_t* ProcRes)
{
    LOG1_ADEHAZE("ENTER: %s \n", __func__);

    bool hist_para_en = pStManu->hist_setting.hist_para_en;
    float hist_gratio = pStManu->hist_setting.HistData.hist_gratio;
    float hist_th_off = pStManu->hist_setting.HistData.hist_th_off;
    float hist_k = pStManu->hist_setting.HistData.hist_k;
    float hist_min = pStManu->hist_setting.HistData.hist_min;
    float hist_scale = pStManu->hist_setting.HistData.hist_scale;
    float cfg_gratio = pStManu->hist_setting.HistData.cfg_gratio;

    ProcRes->ProcResV30.hpara_en = hist_para_en ? FUNCTION_ENABLE : FUNCTION_DISABLE; //  hist_para_en
    //clip hpara_en
    ProcRes->ProcResV30.hpara_en = ProcRes->ProcResV30.dc_en ? ProcRes->ProcResV30.hpara_en : FUNCTION_ENABLE; //  dc en 关闭，hpara必需开

    ProcRes->ProcResV30.hist_gratio   = int(hist_gratio * 8); //       (8bit) hist_gratio
    ProcRes->ProcResV30.hist_th_off   = int(hist_th_off);  //       (8bit) hist_th_off
    ProcRes->ProcResV30.hist_k   = int(hist_k * 4 + 0.5); //0~7    (5bit),3bit+2bit, hist_k
    ProcRes->ProcResV30.hist_min   = int(hist_min * 256); //       (9bit) hist_min
    ProcRes->ProcResV30.cfg_gratio = int(cfg_gratio * 256); //       (13bit),5bit+8bit, cfg_gratio
    ProcRes->ProcResV30.hist_scale       = int(hist_scale *  256 + 0.5 );  //       (13bit),5bit + 8bit, sw_hist_scale

    if(ProcRes->ProcResV30.hist_en) {
        LOGD_ADEHAZE("%s cfg_alpha:%f hist_para_en:%d hist_gratio:%f hist_th_off:%f hist_k:%f hist_min:%f hist_scale:%f cfg_gratio:%f\n", __func__,
                     ProcRes->ProcResV30.cfg_alpha / 255.0, ProcRes->ProcResV30.hpara_en, hist_gratio, hist_th_off, hist_k, hist_min, hist_scale, cfg_gratio);
        LOGD_ADEHAZE("%s cfg_alpha_reg:0x%x hist_gratio_reg:0x%x hist_th_off_reg:0x%x hist_k_reg:0x%x hist_min_reg:0x%x hist_scale_reg:0x%x cfg_gratio_reg:0x%x\n", __func__,
                     ProcRes->ProcResV30.cfg_alpha, ProcRes->ProcResV30.hist_gratio, ProcRes->ProcResV30.hist_th_off, ProcRes->ProcResV30.hist_k,
                     ProcRes->ProcResV30.hist_min, ProcRes->ProcResV30.hist_scale, ProcRes->ProcResV30.cfg_gratio);
    }

    LOG1_ADEHAZE("EIXT: %s \n", __func__);
}

void GetDehazeParamsV21(aDehazeAttrData_t* pCalibV21, int DehazeLevel,
                        RkAiqAdehazeProcResult_t* ProcRes, float CtrlValue) {
    LOG1_ADEHAZE("ENTER: %s \n", __func__);

    bool air_lc_en = pCalibV21->dehaze_setting.air_lc_en;

    // dehaze_self_adp[7]
    float dc_min_th  = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                   pCalibV21->dehaze_setting.DehazeData.dc_min_th, CtrlValue,
                                   DHAZ_CTRL_DATA_STEP_MAX);
    float dc_max_th  = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                   pCalibV21->dehaze_setting.DehazeData.dc_max_th, CtrlValue,
                                   DHAZ_CTRL_DATA_STEP_MAX);
    float yhist_th   = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                  pCalibV21->dehaze_setting.DehazeData.yhist_th, CtrlValue,
                                  DHAZ_CTRL_DATA_STEP_MAX);
    float yblk_th    = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                 pCalibV21->dehaze_setting.DehazeData.yblk_th, CtrlValue,
                                 DHAZ_CTRL_DATA_STEP_MAX);
    float dark_th    = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                 pCalibV21->dehaze_setting.DehazeData.dark_th, CtrlValue,
                                 DHAZ_CTRL_DATA_STEP_MAX);
    float bright_min = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                    pCalibV21->dehaze_setting.DehazeData.bright_min, CtrlValue,
                                    DHAZ_CTRL_DATA_STEP_MAX);
    float bright_max = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                    pCalibV21->dehaze_setting.DehazeData.bright_max, CtrlValue,
                                    DHAZ_CTRL_DATA_STEP_MAX);

    // dehaze_range_adj[6]
    float wt_max    = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                pCalibV21->dehaze_setting.DehazeData.wt_max, CtrlValue,
                                DHAZ_CTRL_DATA_STEP_MAX);
    float air_max   = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                 pCalibV21->dehaze_setting.DehazeData.air_max, CtrlValue,
                                 DHAZ_CTRL_DATA_STEP_MAX);
    float air_min   = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                 pCalibV21->dehaze_setting.DehazeData.air_min, CtrlValue,
                                 DHAZ_CTRL_DATA_STEP_MAX);
    float tmax_base = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                   pCalibV21->dehaze_setting.DehazeData.tmax_base, CtrlValue,
                                   DHAZ_CTRL_DATA_STEP_MAX);
    float tmax_off  = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                  pCalibV21->dehaze_setting.DehazeData.tmax_off, CtrlValue,
                                  DHAZ_CTRL_DATA_STEP_MAX);
    float tmax_max  = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                  pCalibV21->dehaze_setting.DehazeData.tmax_max, CtrlValue,
                                  DHAZ_CTRL_DATA_STEP_MAX);

    // dehaze_iir_control[5]
    float stab_fnum = pCalibV21->dehaze_setting.stab_fnum;
    float sigma = pCalibV21->dehaze_setting.sigma;
    float wt_sigma = pCalibV21->dehaze_setting.wt_sigma;
    float air_sigma = pCalibV21->dehaze_setting.air_sigma;
    float tmax_sigma = pCalibV21->dehaze_setting.tmax_sigma;
    float pre_wet = pCalibV21->dehaze_setting.pre_wet;

    float cfg_wt   = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                pCalibV21->dehaze_setting.DehazeData.cfg_wt, CtrlValue,
                                DHAZ_CTRL_DATA_STEP_MAX);
    float cfg_air  = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                 pCalibV21->dehaze_setting.DehazeData.cfg_air, CtrlValue,
                                 DHAZ_CTRL_DATA_STEP_MAX);
    float cfg_tmax = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                  pCalibV21->dehaze_setting.DehazeData.cfg_tmax, CtrlValue,
                                  DHAZ_CTRL_DATA_STEP_MAX);

    float range_sigma     = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                     pCalibV21->dehaze_setting.DehazeData.range_sigma, CtrlValue,
                                     DHAZ_CTRL_DATA_STEP_MAX);
    float space_sigma_cur = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                         pCalibV21->dehaze_setting.DehazeData.space_sigma_cur,
                                         CtrlValue, DHAZ_CTRL_DATA_STEP_MAX);
    float space_sigma_pre = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                         pCalibV21->dehaze_setting.DehazeData.space_sigma_pre,
                                         CtrlValue, DHAZ_CTRL_DATA_STEP_MAX);

    // dehaze_bi_pAdehazeCtx[4]
    float bf_weight  = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                   pCalibV21->dehaze_setting.DehazeData.bf_weight, CtrlValue,
                                   DHAZ_CTRL_DATA_STEP_MAX);
    float dc_weitcur = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                    pCalibV21->dehaze_setting.DehazeData.dc_weitcur, CtrlValue,
                                    DHAZ_CTRL_DATA_STEP_MAX);

    // dehaze_air_bf_h[9],dehaze_gaus_h[9]
    float gaus_h[9] = {2.0000, 4.0000, 2.0000,
                       4.0000, 8.0000, 4.0000,
                       2.0000, 4.0000, 2.0000
                      };

    int rawWidth = 1920;
    int rawHeight = 1080;
    ProcRes->ProcResV21.air_lc_en    = air_lc_en ? 1 : 0; // air_lc_en
    ProcRes->ProcResV21.dc_min_th    = int(dc_min_th); //0~255, (8bit) dc_min_th
    ProcRes->ProcResV21.dc_max_th    = int(dc_max_th);  //0~255, (8bit) dc_max_th
    ProcRes->ProcResV21.yhist_th    = int(yhist_th);  //0~255, (8bit) yhist_th
    ProcRes->ProcResV21.yblk_th    = int(yblk_th * ((rawWidth + 15) / 16) * ((rawHeight + 15) / 16)); //default:28,(9bit) yblk_th
    ProcRes->ProcResV21.dark_th    = int(dark_th);  //0~255, (8bit) dark_th
    ProcRes->ProcResV21.bright_min   = int(bright_min);  //0~255, (8bit) bright_min
    ProcRes->ProcResV21.bright_max   = int(bright_max);  //0~255, (8bit) bright_max
    ProcRes->ProcResV21.wt_max   = int(wt_max * 256); //0~255, (8bit) wt_max
    ProcRes->ProcResV21.air_min   = int(air_min);  //0~255, (8bit) air_min
    ProcRes->ProcResV21.air_max   = int(air_max);  //0~256, (8bit) air_max
    ProcRes->ProcResV21.tmax_base   = int(tmax_base);  //0~255, (8bit) tmax_base
    ProcRes->ProcResV21.tmax_off   = int(tmax_off * 1024); //0~1024,(10bit) tmax_off
    ProcRes->ProcResV21.tmax_max   = int(tmax_max * 1024); //0~1024,(10bit) tmax_max
    ProcRes->ProcResV21.stab_fnum = int(stab_fnum);  //1~31,  (5bit) stab_fnum
    ProcRes->ProcResV21.iir_sigma = int(sigma);  //0~255, (8bit) sigma
    ProcRes->ProcResV21.iir_wt_sigma = int(wt_sigma * 8 + 0.5); //       (11bit),8bit+3bit, wt_sigma
    ProcRes->ProcResV21.iir_air_sigma = int(air_sigma);  //       (8bit) air_sigma
    ProcRes->ProcResV21.iir_tmax_sigma = int(tmax_sigma * 1024 + 0.5);  //       (11bit) tmax_sigma
    ProcRes->ProcResV21.iir_pre_wet = int(pre_wet * 15 + 0.5);  //       (4bit) iir_pre_wet
    ProcRes->ProcResV21.cfg_wt = int(cfg_wt * 256); //0~256, (9bit) cfg_wt
    ProcRes->ProcResV21.cfg_air = int(cfg_air);  //0~255, (8bit) cfg_air
    ProcRes->ProcResV21.cfg_tmax = int(cfg_tmax * 1024); //0~1024,(11bit) cfg_tmax
    ProcRes->ProcResV21.range_sima = int(range_sigma * 512); //0~512,(9bit) range_sima
    ProcRes->ProcResV21.space_sigma_cur = int(space_sigma_cur * 256); //0~256,(8bit) space_sigma_cur
    ProcRes->ProcResV21.space_sigma_pre = int(space_sigma_pre * 256); //0~256,(8bit) space_sigma_pre
    ProcRes->ProcResV21.bf_weight      = int(bf_weight * 256); //0~512, (8bit) dc_thed
    ProcRes->ProcResV21.dc_weitcur       = int(dc_weitcur * 256 + 0.5); //0~256, (9bit) dc_weitcur
    ProcRes->ProcResV21.gaus_h0    = int(gaus_h[4]);//h0~h2  浠庡ぇ鍒板皬
    ProcRes->ProcResV21.gaus_h1    = int(gaus_h[1]);
    ProcRes->ProcResV21.gaus_h2    = int(gaus_h[0]);

    if (DehazeLevel != DEHAZE_API_MANUAL_DEFAULT_LEVEL) {
        ProcRes->ProcResV21.cfg_alpha = 255;
        float level_diff              = (float)(DehazeLevel - DEHAZE_API_MANUAL_DEFAULT_LEVEL);

        // sw_dhaz_cfg_wt
        float sw_dhaz_cfg_wt = (float)ProcRes->ProcResV21.cfg_wt;
        sw_dhaz_cfg_wt += level_diff * 0.005 * 256;
        sw_dhaz_cfg_wt             = LIMIT_VALUE(sw_dhaz_cfg_wt, 256, 1);
        ProcRes->ProcResV21.cfg_wt = (int)sw_dhaz_cfg_wt;

        // sw_dhaz_cfg_air
        float sw_dhaz_cfg_air = (float)ProcRes->ProcResV21.cfg_air;
        sw_dhaz_cfg_air += level_diff * 0.5;
        sw_dhaz_cfg_air             = LIMIT_VALUE(sw_dhaz_cfg_air, 255, 1);
        ProcRes->ProcResV21.cfg_air = (int)sw_dhaz_cfg_air;

        // sw_dhaz_cfg_tmax
        float sw_dhaz_cfg_tmax = (float)ProcRes->ProcResV21.cfg_tmax;
        sw_dhaz_cfg_tmax += level_diff * 0.005 * 1024;
        sw_dhaz_cfg_tmax             = LIMIT_VALUE(sw_dhaz_cfg_tmax, 1023, 1);
        ProcRes->ProcResV21.cfg_tmax = (int)sw_dhaz_cfg_tmax;

        LOGD_ADEHAZE(" %s: Adehaze munual level:%f level_diff:%f\n", __func__, DehazeLevel,
                     level_diff);
    }

    if(ProcRes->ProcResV21.dc_en && !(ProcRes->ProcResV21.enhance_en)) {
        if(ProcRes->ProcResV21.cfg_alpha == 255) {
            LOGD_ADEHAZE("%s cfg_alpha:255 CtrlData:%f cfg_air:%f cfg_tmax:%f cfg_wt:%f\n",
                         __func__, CtrlValue, cfg_air, cfg_tmax, cfg_wt);
            LOGD_ADEHAZE("%s cfg_alpha_reg:0x0 cfg_air:0x%x cfg_tmax:0x%x cfg_wt:0x%x\n", __func__, ProcRes->ProcResV21.cfg_air, ProcRes->ProcResV21.cfg_tmax,
                         ProcRes->ProcResV21.cfg_wt);
        }
        else if(ProcRes->ProcResV21.cfg_alpha == 0) {
            LOGD_ADEHAZE(
                "%s cfg_alpha:0 CtrlData:%f air_max:%f air_min:%f tmax_base:%f wt_max:%f\n",
                __func__, CtrlValue, air_max, air_min, tmax_base, wt_max);
            LOGD_ADEHAZE("%s cfg_alpha_reg:0x0 air_max:0x%x air_min:0x%x tmax_base:0x%x wt_max:0x%x\n", __func__, ProcRes->ProcResV21.air_max, ProcRes->ProcResV21.air_min,
                         ProcRes->ProcResV21.tmax_base, ProcRes->ProcResV21.wt_max);
        }
    }


    LOG1_ADEHAZE("EIXT: %s \n", __func__);
}

void GetDehazeParamsV30(aDehazeAttrData_t* pCalibV21, int DehazeLevel,
                        RkAiqAdehazeProcResult_t* ProcRes, float CtrlValue) {
    LOG1_ADEHAZE("ENTER: %s \n", __func__);

    bool air_lc_en = pCalibV21->dehaze_setting.air_lc_en;

    // dehaze_self_adp[7]
    float dc_min_th  = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                   pCalibV21->dehaze_setting.DehazeData.dc_min_th, CtrlValue,
                                   DHAZ_CTRL_DATA_STEP_MAX);
    float dc_max_th  = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                   pCalibV21->dehaze_setting.DehazeData.dc_max_th, CtrlValue,
                                   DHAZ_CTRL_DATA_STEP_MAX);
    float yhist_th   = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                  pCalibV21->dehaze_setting.DehazeData.yhist_th, CtrlValue,
                                  DHAZ_CTRL_DATA_STEP_MAX);
    float yblk_th    = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                 pCalibV21->dehaze_setting.DehazeData.yblk_th, CtrlValue,
                                 DHAZ_CTRL_DATA_STEP_MAX);
    float dark_th    = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                 pCalibV21->dehaze_setting.DehazeData.dark_th, CtrlValue,
                                 DHAZ_CTRL_DATA_STEP_MAX);
    float bright_min = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                    pCalibV21->dehaze_setting.DehazeData.bright_min, CtrlValue,
                                    DHAZ_CTRL_DATA_STEP_MAX);
    float bright_max = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                    pCalibV21->dehaze_setting.DehazeData.bright_max, CtrlValue,
                                    DHAZ_CTRL_DATA_STEP_MAX);

    // dehaze_range_adj[6]
    float wt_max    = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                pCalibV21->dehaze_setting.DehazeData.wt_max, CtrlValue,
                                DHAZ_CTRL_DATA_STEP_MAX);
    float air_max   = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                 pCalibV21->dehaze_setting.DehazeData.air_max, CtrlValue,
                                 DHAZ_CTRL_DATA_STEP_MAX);
    float air_min   = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                 pCalibV21->dehaze_setting.DehazeData.air_min, CtrlValue,
                                 DHAZ_CTRL_DATA_STEP_MAX);
    float tmax_base = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                   pCalibV21->dehaze_setting.DehazeData.tmax_base, CtrlValue,
                                   DHAZ_CTRL_DATA_STEP_MAX);
    float tmax_off  = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                  pCalibV21->dehaze_setting.DehazeData.tmax_off, CtrlValue,
                                  DHAZ_CTRL_DATA_STEP_MAX);
    float tmax_max  = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                  pCalibV21->dehaze_setting.DehazeData.tmax_max, CtrlValue,
                                  DHAZ_CTRL_DATA_STEP_MAX);

    // dehaze_iir_control[5]
    float stab_fnum = pCalibV21->dehaze_setting.stab_fnum;
    float sigma = pCalibV21->dehaze_setting.sigma;
    float wt_sigma = pCalibV21->dehaze_setting.wt_sigma;
    float air_sigma = pCalibV21->dehaze_setting.air_sigma;
    float tmax_sigma = pCalibV21->dehaze_setting.tmax_sigma;
    float pre_wet = pCalibV21->dehaze_setting.pre_wet;

    float cfg_wt   = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                pCalibV21->dehaze_setting.DehazeData.cfg_wt, CtrlValue,
                                DHAZ_CTRL_DATA_STEP_MAX);
    float cfg_air  = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                 pCalibV21->dehaze_setting.DehazeData.cfg_air, CtrlValue,
                                 DHAZ_CTRL_DATA_STEP_MAX);
    float cfg_tmax = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                  pCalibV21->dehaze_setting.DehazeData.cfg_tmax, CtrlValue,
                                  DHAZ_CTRL_DATA_STEP_MAX);

    float range_sigma     = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                     pCalibV21->dehaze_setting.DehazeData.range_sigma, CtrlValue,
                                     DHAZ_CTRL_DATA_STEP_MAX);
    float space_sigma_cur = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                         pCalibV21->dehaze_setting.DehazeData.space_sigma_cur,
                                         CtrlValue, DHAZ_CTRL_DATA_STEP_MAX);
    float space_sigma_pre = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                         pCalibV21->dehaze_setting.DehazeData.space_sigma_pre,
                                         CtrlValue, DHAZ_CTRL_DATA_STEP_MAX);

    // dehaze_bi_pAdehazeCtx[4]
    float bf_weight  = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                   pCalibV21->dehaze_setting.DehazeData.bf_weight, CtrlValue,
                                   DHAZ_CTRL_DATA_STEP_MAX);
    float dc_weitcur = LinearInterp(pCalibV21->dehaze_setting.DehazeData.CtrlData,
                                    pCalibV21->dehaze_setting.DehazeData.dc_weitcur, CtrlValue,
                                    DHAZ_CTRL_DATA_STEP_MAX);

    // dehaze_air_bf_h[9],dehaze_gaus_h[9]
    float gaus_h[9] = {2.0000, 4.0000, 2.0000,
                       4.0000, 8.0000, 4.0000,
                       2.0000, 4.0000, 2.0000
                      };

    int rawWidth = 1920;
    int rawHeight = 1080;
    ProcRes->ProcResV30.air_lc_en    = air_lc_en ? 1 : 0; // air_lc_en
    ProcRes->ProcResV30.dc_min_th    = int(dc_min_th); //0~255, (8bit) dc_min_th
    ProcRes->ProcResV30.dc_max_th    = int(dc_max_th);  //0~255, (8bit) dc_max_th
    ProcRes->ProcResV30.yhist_th    = int(yhist_th);  //0~255, (8bit) yhist_th
    ProcRes->ProcResV30.yblk_th    = int(yblk_th * ((rawWidth + 15) / 16) * ((rawHeight + 15) / 16)); //default:28,(9bit) yblk_th
    ProcRes->ProcResV30.dark_th    = int(dark_th);  //0~255, (8bit) dark_th
    ProcRes->ProcResV30.bright_min   = int(bright_min);  //0~255, (8bit) bright_min
    ProcRes->ProcResV30.bright_max   = int(bright_max);  //0~255, (8bit) bright_max
    ProcRes->ProcResV30.wt_max   = int(wt_max * 256); //0~255, (8bit) wt_max
    ProcRes->ProcResV30.air_min   = int(air_min);  //0~255, (8bit) air_min
    ProcRes->ProcResV30.air_max   = int(air_max);  //0~256, (8bit) air_max
    ProcRes->ProcResV30.tmax_base   = int(tmax_base);  //0~255, (8bit) tmax_base
    ProcRes->ProcResV30.tmax_off   = int(tmax_off * 1024); //0~1024,(10bit) tmax_off
    ProcRes->ProcResV30.tmax_max   = int(tmax_max * 1024); //0~1024,(10bit) tmax_max
    ProcRes->ProcResV30.stab_fnum = int(stab_fnum);  //1~31,  (5bit) stab_fnum
    ProcRes->ProcResV30.iir_sigma = int(sigma);  //0~255, (8bit) sigma
    ProcRes->ProcResV30.iir_wt_sigma = int(wt_sigma * 8 + 0.5); //       (11bit),8bit+3bit, wt_sigma
    ProcRes->ProcResV30.iir_air_sigma = int(air_sigma);  //       (8bit) air_sigma
    ProcRes->ProcResV30.iir_tmax_sigma = int(tmax_sigma * 1024 + 0.5);  //       (11bit) tmax_sigma
    ProcRes->ProcResV30.iir_pre_wet = int(pre_wet * 15 + 0.5);  //       (4bit) iir_pre_wet
    ProcRes->ProcResV30.cfg_wt = int(cfg_wt * 256); //0~256, (9bit) cfg_wt
    ProcRes->ProcResV30.cfg_air = int(cfg_air);  //0~255, (8bit) cfg_air
    ProcRes->ProcResV30.cfg_tmax = int(cfg_tmax * 1024); //0~1024,(11bit) cfg_tmax
    ProcRes->ProcResV30.range_sima = int(range_sigma * 512); //0~512,(9bit) range_sima
    ProcRes->ProcResV30.space_sigma_cur = int(space_sigma_cur * 256); //0~256,(8bit) space_sigma_cur
    ProcRes->ProcResV30.space_sigma_pre = int(space_sigma_pre * 256); //0~256,(8bit) space_sigma_pre
    ProcRes->ProcResV30.bf_weight      = int(bf_weight * 256); //0~512, (8bit) dc_thed
    ProcRes->ProcResV30.dc_weitcur       = int(dc_weitcur * 256 + 0.5); //0~256, (9bit) dc_weitcur
    ProcRes->ProcResV30.gaus_h0    = int(gaus_h[4]);//h0~h2  浠庡ぇ鍒板皬
    ProcRes->ProcResV30.gaus_h1    = int(gaus_h[1]);
    ProcRes->ProcResV30.gaus_h2    = int(gaus_h[0]);

    if (DehazeLevel != DEHAZE_API_MANUAL_DEFAULT_LEVEL) {
        ProcRes->ProcResV30.cfg_alpha = 255;
        float level_diff              = (float)(DehazeLevel - DEHAZE_API_MANUAL_DEFAULT_LEVEL);

        // sw_dhaz_cfg_wt
        float sw_dhaz_cfg_wt = (float)ProcRes->ProcResV30.cfg_wt;
        sw_dhaz_cfg_wt += level_diff * 0.005 * 256;
        sw_dhaz_cfg_wt             = LIMIT_VALUE(sw_dhaz_cfg_wt, 256, 1);
        ProcRes->ProcResV30.cfg_wt = (int)sw_dhaz_cfg_wt;

        // sw_dhaz_cfg_air
        float sw_dhaz_cfg_air = (float)ProcRes->ProcResV30.cfg_air;
        sw_dhaz_cfg_air += level_diff * 0.5;
        sw_dhaz_cfg_air             = LIMIT_VALUE(sw_dhaz_cfg_air, 255, 1);
        ProcRes->ProcResV30.cfg_air = (int)sw_dhaz_cfg_air;

        // sw_dhaz_cfg_tmax
        float sw_dhaz_cfg_tmax = (float)ProcRes->ProcResV30.cfg_tmax;
        sw_dhaz_cfg_tmax += level_diff * 0.005 * 1024;
        sw_dhaz_cfg_tmax             = LIMIT_VALUE(sw_dhaz_cfg_tmax, 1023, 1);
        ProcRes->ProcResV30.cfg_tmax = (int)sw_dhaz_cfg_tmax;

        LOGD_ADEHAZE(" %s: Adehaze munual level:%f level_diff:%f\n", __func__, DehazeLevel,
                     level_diff);
    }

    if(ProcRes->ProcResV30.dc_en && !(ProcRes->ProcResV30.enhance_en)) {
        if(ProcRes->ProcResV30.cfg_alpha == 255) {
            LOGD_ADEHAZE("%s cfg_alpha:1 CtrlData:%f cfg_air:%f cfg_tmax:%f cfg_wt:%f\n", __func__,
                         CtrlValue, cfg_air, cfg_tmax, cfg_wt);
            LOGD_ADEHAZE("%s cfg_alpha_reg:0x255 cfg_air:0x%x cfg_tmax:0x%x cfg_wt:0x%x\n", __func__,
                         ProcRes->ProcResV30.cfg_air, ProcRes->ProcResV30.cfg_tmax, ProcRes->ProcResV30.cfg_wt);
        }
        else if(ProcRes->ProcResV30.cfg_alpha == 0) {
            LOGD_ADEHAZE(
                "%s cfg_alpha:0 CtrlData:%f air_max:%f air_min:%f tmax_base:%f wt_max:%f\n",
                __func__, CtrlValue, air_max, air_min, tmax_base, wt_max);
            LOGD_ADEHAZE("%s cfg_alpha_reg:0x0 air_max:0x%x air_min:0x%x tmax_base:0x%x wt_max:0x%x\n", __func__, ProcRes->ProcResV30.air_max, ProcRes->ProcResV30.air_min,
                         ProcRes->ProcResV30.tmax_base, ProcRes->ProcResV30.wt_max);
        }
    }

    LOG1_ADEHAZE("EIXT: %s \n", __func__);
}

void GetEnhanceParamsV21(aDehazeAttrData_t* pCalibV21, int EnhanceLevel,
                         RkAiqAdehazeProcResult_t* ProcRes, float CtrlValue) {
    LOG1_ADEHAZE("ENTER: %s \n", __func__);

    ProcRes->ProcResV21.enhance_value =
        int(LinearInterp(pCalibV21->enhance_setting.EnhanceData.CtrlData,
                         pCalibV21->enhance_setting.EnhanceData.enhance_value, CtrlValue,
                         DHAZ_CTRL_DATA_STEP_MAX) *
                1024.0f +
            0.5);
    if (EnhanceLevel != DEHAZE_API_ENHANCE_MANUAL_DEFAULT_LEVEL) {
        ProcRes->ProcResV21.enhance_value =
            ProcRes->ProcResV21.enhance_value +
            50 * (EnhanceLevel - DEHAZE_API_ENHANCE_MANUAL_DEFAULT_LEVEL);
    }
    ProcRes->ProcResV21.enhance_value = LIMIT_VALUE(ProcRes->ProcResV21.enhance_value, 0x3fff, 0);
    ProcRes->ProcResV21.enhance_chroma =
        LIMIT_VALUE(int(LinearInterp(pCalibV21->enhance_setting.EnhanceData.CtrlData,
                                     pCalibV21->enhance_setting.EnhanceData.enhance_chroma,
                                     CtrlValue, DHAZ_CTRL_DATA_STEP_MAX) *
                            1024.0f +
                        0.5),
                    0x3fff, 0);

    for(int i = 0; i < ISP21_DHAZ_ENH_CURVE_NUM; i++)
        ProcRes->ProcResV21.enh_curve[i] = (int)(pCalibV21->enhance_setting.enhance_curve[i]);

    if(ProcRes->ProcResV21.dc_en && ProcRes->ProcResV21.enhance_en) {
        LOGD_ADEHAZE("%s CtrlData:%f enhance_value:%f enhance_chroma:%f\n", __func__, CtrlValue,
                     ProcRes->ProcResV21.enhance_value / 1024.0f,
                     ProcRes->ProcResV21.enhance_chroma / 1024.0f);
        LOGD_ADEHAZE("%s enhance_value_reg:0x%x enhance_chroma_reg:0x%x\n", __func__,
                     ProcRes->ProcResV21.enhance_value, ProcRes->ProcResV21.enhance_chroma);
    }

    LOG1_ADEHAZE("EIXT: %s \n", __func__);
}

void GetEnhanceParamsV30(aDehazeAttrData_t* pCalibV21, int EnhanceLevel,
                         RkAiqAdehazeProcResult_t* ProcRes, float CtrlValue) {
    LOG1_ADEHAZE("ENTER: %s \n", __func__);

    ProcRes->ProcResV30.enhance_value =
        int(LinearInterp(pCalibV21->enhance_setting.EnhanceData.CtrlData,
                         pCalibV21->enhance_setting.EnhanceData.enhance_value, CtrlValue,
                         DHAZ_CTRL_DATA_STEP_MAX) *
                1024.0f +
            0.5);
    if (EnhanceLevel != DEHAZE_API_ENHANCE_MANUAL_DEFAULT_LEVEL) {
        ProcRes->ProcResV30.enhance_value =
            ProcRes->ProcResV30.enhance_value +
            50 * (EnhanceLevel - DEHAZE_API_ENHANCE_MANUAL_DEFAULT_LEVEL);
    }
    ProcRes->ProcResV30.enhance_value = LIMIT_VALUE(ProcRes->ProcResV30.enhance_value, 0x3fff, 0);
    ProcRes->ProcResV30.enhance_chroma =
        LIMIT_VALUE(int(LinearInterp(pCalibV21->enhance_setting.EnhanceData.CtrlData,
                                     pCalibV21->enhance_setting.EnhanceData.enhance_chroma,
                                     CtrlValue, DHAZ_CTRL_DATA_STEP_MAX) *
                            1024.0f +
                        0.5),
                    0x3fff, 0);

    for(int i = 0; i < ISP3X_DHAZ_ENH_CURVE_NUM; i++)
        ProcRes->ProcResV30.enh_curve[i] = (int)(pCalibV21->enhance_setting.enhance_curve[i]);

    if(ProcRes->ProcResV30.dc_en && ProcRes->ProcResV30.enhance_en) {
        LOGD_ADEHAZE("%s CtrlData:%f enhance_value:%f enhance_chroma:%f\n", __func__, CtrlValue,
                     ProcRes->ProcResV30.enhance_value / 1024.0f,
                     ProcRes->ProcResV30.enhance_chroma / 1024.0f);
        LOGD_ADEHAZE("%s enhance_value_reg:0x%x enhance_chroma_reg:0x%x\n", __func__,
                     ProcRes->ProcResV30.enhance_value, ProcRes->ProcResV30.enhance_chroma);
    }

    LOG1_ADEHAZE("EIXT: %s \n", __func__);
}

void GetHistParamsV21(aDehazeAttrData_t* pCalibV21, RkAiqAdehazeProcResult_t* ProcRes,
                      float CtrlValue) {
    LOG1_ADEHAZE("ENTER: %s \n", __func__);

    float hist_gratio = LinearInterp(pCalibV21->hist_setting.HistData.CtrlData,
                                     pCalibV21->hist_setting.HistData.hist_gratio, CtrlValue,
                                     DHAZ_CTRL_DATA_STEP_MAX);
    float hist_th_off = LinearInterp(pCalibV21->hist_setting.HistData.CtrlData,
                                     pCalibV21->hist_setting.HistData.hist_th_off, CtrlValue,
                                     DHAZ_CTRL_DATA_STEP_MAX);
    float hist_k =
        LinearInterp(pCalibV21->hist_setting.HistData.CtrlData,
                     pCalibV21->hist_setting.HistData.hist_k, CtrlValue, DHAZ_CTRL_DATA_STEP_MAX);
    float hist_min =
        LinearInterp(pCalibV21->hist_setting.HistData.CtrlData,
                     pCalibV21->hist_setting.HistData.hist_min, CtrlValue, DHAZ_CTRL_DATA_STEP_MAX);
    float hist_scale = LinearInterp(pCalibV21->hist_setting.HistData.CtrlData,
                                    pCalibV21->hist_setting.HistData.hist_scale, CtrlValue,
                                    DHAZ_CTRL_DATA_STEP_MAX);
    float cfg_gratio = LinearInterp(pCalibV21->hist_setting.HistData.CtrlData,
                                    pCalibV21->hist_setting.HistData.cfg_gratio, CtrlValue,
                                    DHAZ_CTRL_DATA_STEP_MAX);

    ProcRes->ProcResV21.hpara_en =
        pCalibV21->hist_setting.hist_para_en ? FUNCTION_ENABLE : FUNCTION_DISABLE;  //  hist_para_en
    // clip hpara_en
    ProcRes->ProcResV21.hpara_en = ProcRes->ProcResV21.dc_en ? ProcRes->ProcResV21.hpara_en : FUNCTION_ENABLE; //  dc en 关闭，hpara必需开

    ProcRes->ProcResV21.hist_gratio   = int(hist_gratio * 8); //       (8bit) hist_gratio
    ProcRes->ProcResV21.hist_th_off   = int(hist_th_off);  //       (8bit) hist_th_off
    ProcRes->ProcResV21.hist_k   = int(hist_k * 4 + 0.5); //0~7    (5bit),3bit+2bit, hist_k
    ProcRes->ProcResV21.hist_min   = int(hist_min * 256); //       (9bit) hist_min
    ProcRes->ProcResV21.cfg_gratio = int(cfg_gratio * 256); //       (13bit),5bit+8bit, cfg_gratio
    ProcRes->ProcResV21.hist_scale       = int(hist_scale *  256 + 0.5 );  //       (13bit),5bit + 8bit, sw_hist_scale

    if(ProcRes->ProcResV21.hist_en) {
        LOGD_ADEHAZE(
            "%s cfg_alpha:%f CtrlData:%f hist_para_en:%d hist_gratio:%f hist_th_off:%f hist_k:%f "
            "hist_min:%f hist_scale:%f cfg_gratio:%f\n",
            __func__, ProcRes->ProcResV21.cfg_alpha / 255.0, CtrlValue,
            ProcRes->ProcResV21.hpara_en, hist_gratio, hist_th_off, hist_k, hist_min, hist_scale,
            cfg_gratio);
        LOGD_ADEHAZE("%s cfg_alpha_reg:0x%x hist_gratio_reg:0x%x hist_th_off_reg:0x%x hist_k_reg:0x%x hist_min_reg:0x%x hist_scale_reg:0x%x cfg_gratio_reg:0x%x\n", __func__,
                     ProcRes->ProcResV21.cfg_alpha, ProcRes->ProcResV21.hist_gratio, ProcRes->ProcResV21.hist_th_off, ProcRes->ProcResV21.hist_k,
                     ProcRes->ProcResV21.hist_min, ProcRes->ProcResV21.hist_scale, ProcRes->ProcResV21.cfg_gratio);
    }

    LOG1_ADEHAZE("EIXT: %s \n", __func__);
}

void GetHistParamsV30(aDehazeAttrData_t* pCalibV21, RkAiqAdehazeProcResult_t* ProcRes,
                      float CtrlValue) {
    LOG1_ADEHAZE("ENTER: %s \n", __func__);

    float hist_gratio = LinearInterp(pCalibV21->hist_setting.HistData.CtrlData,
                                     pCalibV21->hist_setting.HistData.hist_gratio, CtrlValue,
                                     DHAZ_CTRL_DATA_STEP_MAX);
    float hist_th_off = LinearInterp(pCalibV21->hist_setting.HistData.CtrlData,
                                     pCalibV21->hist_setting.HistData.hist_th_off, CtrlValue,
                                     DHAZ_CTRL_DATA_STEP_MAX);
    float hist_k =
        LinearInterp(pCalibV21->hist_setting.HistData.CtrlData,
                     pCalibV21->hist_setting.HistData.hist_k, CtrlValue, DHAZ_CTRL_DATA_STEP_MAX);
    float hist_min =
        LinearInterp(pCalibV21->hist_setting.HistData.CtrlData,
                     pCalibV21->hist_setting.HistData.hist_min, CtrlValue, DHAZ_CTRL_DATA_STEP_MAX);
    float hist_scale = LinearInterp(pCalibV21->hist_setting.HistData.CtrlData,
                                    pCalibV21->hist_setting.HistData.hist_scale, CtrlValue,
                                    DHAZ_CTRL_DATA_STEP_MAX);
    float cfg_gratio = LinearInterp(pCalibV21->hist_setting.HistData.CtrlData,
                                    pCalibV21->hist_setting.HistData.cfg_gratio, CtrlValue,
                                    DHAZ_CTRL_DATA_STEP_MAX);

    ProcRes->ProcResV30.hpara_en =
        pCalibV21->hist_setting.hist_para_en ? FUNCTION_ENABLE : FUNCTION_DISABLE;  //  hist_para_en
    // clip hpara_en
    ProcRes->ProcResV30.hpara_en = ProcRes->ProcResV30.dc_en ? ProcRes->ProcResV30.hpara_en : FUNCTION_ENABLE; //  dc en 关闭，hpara必需开

    ProcRes->ProcResV30.hist_gratio   = int(hist_gratio * 8); //       (8bit) hist_gratio
    ProcRes->ProcResV30.hist_th_off   = int(hist_th_off);  //       (8bit) hist_th_off
    ProcRes->ProcResV30.hist_k   = int(hist_k * 4 + 0.5); //0~7    (5bit),3bit+2bit, hist_k
    ProcRes->ProcResV30.hist_min   = int(hist_min * 256); //       (9bit) hist_min
    ProcRes->ProcResV30.cfg_gratio = int(cfg_gratio * 256); //       (13bit),5bit+8bit, cfg_gratio
    ProcRes->ProcResV30.hist_scale       = int(hist_scale *  256 + 0.5 );  //       (13bit),5bit + 8bit, sw_hist_scale

    if(ProcRes->ProcResV30.hist_en) {
        LOGD_ADEHAZE(
            "%s cfg_alpha:%f CtrlData:%f hist_para_en:%d hist_gratio:%f hist_th_off:%f hist_k:%f "
            "hist_min:%f hist_scale:%f cfg_gratio:%f\n",
            __func__, ProcRes->ProcResV30.cfg_alpha / 255.0, CtrlValue,
            ProcRes->ProcResV30.hpara_en, hist_gratio, hist_th_off, hist_k, hist_min, hist_scale,
            cfg_gratio);
        LOGD_ADEHAZE("%s cfg_alpha_reg:0x%x hist_gratio_reg:0x%x hist_th_off_reg:0x%x hist_k_reg:0x%x hist_min_reg:0x%x hist_scale_reg:0x%x cfg_gratio_reg:0x%x\n", __func__,
                     ProcRes->ProcResV30.cfg_alpha, ProcRes->ProcResV30.hist_gratio, ProcRes->ProcResV30.hist_th_off, ProcRes->ProcResV30.hist_k,
                     ProcRes->ProcResV30.hist_min, ProcRes->ProcResV30.hist_scale, ProcRes->ProcResV30.cfg_gratio);
    }

    LOG1_ADEHAZE("EIXT: %s \n", __func__);
}

void GetDehazeHistDuoISPSettingV30(RkAiqAdehazeProcResult_t* ProcRes, rkisp_adehaze_stats_t* pStats, bool DuoCamera, int FrameID)
{
    LOG1_ADEHAZE("ENTER: %s \n", __func__);

    //round_en
    ProcRes->ProcResV30.round_en = FUNCTION_ENABLE;

    //deahze duo setting
    if(DuoCamera) {
        ProcRes->ProcResV30.soft_wr_en = FUNCTION_ENABLE;
#if 1
        //support default value for kernel calc
        for(int i = 0; i < ISP3X_DHAZ_HIST_WR_NUM; i++) {
            ProcRes->ProcResV30.hist_wr[i] = 16 * (i + 1);
            ProcRes->ProcResV30.hist_wr[i] = ProcRes->ProcResV30.hist_wr[i] > 1023 ? 1023 : ProcRes->ProcResV30.hist_wr[i];
        }
#else
        ProcRes->ProcResV30.adp_air_wr = pStats->dehaze_stats_v30.dhaz_adp_air_base;
        ProcRes->ProcResV30.adp_gratio_wr = pStats->dehaze_stats_v30.dhaz_adp_gratio;
        ProcRes->ProcResV30.adp_tmax_wr = pStats->dehaze_stats_v30.dhaz_adp_tmax;
        ProcRes->ProcResV30.adp_wt_wr = pStats->dehaze_stats_v30.dhaz_adp_wt;

        static int hist_wr[64];
        if(!FrameID)
            for(int i = 0; i < 64; i++) {
                hist_wr[i] = 16 * (i + 1);
                hist_wr[i] = hist_wr[i] > 1023 ? 1023 : hist_wr[i];
                ProcRes->ProcResV30.hist_wr[i] = hist_wr[i];
            }
        else {
            int num = MIN(FrameID + 1, ProcRes->ProcResV30.stab_fnum);
            int tmp = 0;
            for(int i = 0; i < 64; i++) {
                tmp = (hist_wr[i] * (num - 1) + pStats->dehaze_stats_v30.h_rgb_iir[i]) / num;
                ProcRes->ProcResV30.hist_wr[i] = tmp;
                hist_wr[i] = tmp;
            }
        }

        LOGD_ADEHAZE("%s adp_air_wr:0x%x adp_gratio_wr:0x%x adp_tmax_wr:0x%x adp_wt_wr:0x%x\n", __func__, ProcRes->ProcResV30.adp_air_wr, ProcRes->ProcResV30.adp_gratio_wr,
                     ProcRes->ProcResV30.adp_tmax_wr, ProcRes->ProcResV30.adp_wt_wr);

        LOGV_ADEHAZE("%s hist_wr:0x%x", __func__, ProcRes->ProcResV30.hist_wr[0]);
        for(int i = 1; i < 63; i++)
            LOGV_ADEHAZE(" 0x%x", ProcRes->ProcResV30.hist_wr[i]);
        LOGV_ADEHAZE(" 0x%x\n", ProcRes->ProcResV30.hist_wr[63]);
#endif
        LOGD_ADEHAZE("%s DuoCamera:%d soft_wr_en:%d\n", __func__, DuoCamera, ProcRes->ProcResV30.soft_wr_en);
    }
    else
        ProcRes->ProcResV30.soft_wr_en = FUNCTION_DISABLE;

    LOG1_ADEHAZE("EIXT: %s \n", __func__);
}

void GetDehazeLocalGainSettingV30(RkAiqAdehazeProcResult_t* pProcRes, RkAiqYnrV3Res* pYnrRes) {
    LOG1_ADEHAZE("ENTER: %s \n", __func__);

    // get sigma_idx
    for (int i = 0; i < ISP3X_DHAZ_SIGMA_IDX_NUM; i++)
        pProcRes->ProcResV30.sigma_idx[i] = (i + 1) * YNR_CURVE_STEP;

    // get sigma_lut
    int tmp = 0;
    for (int i = 0; i < ISP3X_DHAZ_SIGMA_LUT_NUM; i++) {
        tmp = LIMIT_VALUE(8.0f * pYnrRes->sigma[i], ADHZ10BITMAX, ADHZ10BITMIN);
        pProcRes->ProcResV30.sigma_lut[i] = tmp;
    }

#if 0
            LOGE_ADEHAZE("%s(%d) dehaze local gain IDX(0~5): 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", __func__, __LINE__, pProcRes->ProcResV30.sigma_idx[0], pProcRes->ProcResV30.sigma_idx[1],
                         pProcRes->ProcResV30.sigma_idx[2], pProcRes->ProcResV30.sigma_idx[3], pProcRes->ProcResV30.sigma_idx[4], pProcRes->ProcResV30.sigma_idx[5]);
            LOGE_ADEHAZE("%s(%d) dehaze local gain LUT(0~5): 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", __func__, __LINE__, pProcRes->ProcResV30.sigma_lut[0], pProcRes->ProcResV30.sigma_lut[1],
                         pProcRes->ProcResV30.sigma_lut[2], pProcRes->ProcResV30.sigma_lut[3], pProcRes->ProcResV30.sigma_lut[4], pProcRes->ProcResV30.sigma_lut[5]);
#endif

    LOG1_ADEHAZE("EIXT: %s \n", __func__);
}

void AdehazeGetStats
(
    AdehazeHandle_t*           pAdehazeCtx,
    rkisp_adehaze_stats_t*         ROData
) {
    LOG1_ADEHAZE( "%s:enter!\n", __FUNCTION__);
    LOGV_ADEHAZE("%s: Adehaze RO data from register:\n", __FUNCTION__);

    if(CHECK_ISP_HW_V20()) {
        pAdehazeCtx->stats.dehaze_stats_v20.dhaz_adp_air_base = ROData->dehaze_stats_v20.dhaz_adp_air_base;
        pAdehazeCtx->stats.dehaze_stats_v20.dhaz_adp_wt = ROData->dehaze_stats_v20.dhaz_adp_wt;
        pAdehazeCtx->stats.dehaze_stats_v20.dhaz_adp_gratio = ROData->dehaze_stats_v20.dhaz_adp_gratio;
        pAdehazeCtx->stats.dehaze_stats_v20.dhaz_adp_tmax = ROData->dehaze_stats_v20.dhaz_adp_tmax;
        for(int i = 0; i < ISP2X_DHAZ_HIST_IIR_NUM; i++) {
            pAdehazeCtx->stats.dehaze_stats_v20.h_b_iir[i] = ROData->dehaze_stats_v20.h_b_iir[i];
            pAdehazeCtx->stats.dehaze_stats_v20.h_g_iir[i] = ROData->dehaze_stats_v20.h_g_iir[i];
            pAdehazeCtx->stats.dehaze_stats_v20.h_r_iir[i] = ROData->dehaze_stats_v20.h_r_iir[i];
        }

        LOGV_ADEHAZE("%s:  dhaz_adp_air_base:%d dhaz_adp_wt:%d dhaz_adp_gratio:%d dhaz_adp_tmax:%d\n", __FUNCTION__,
                     pAdehazeCtx->stats.dehaze_stats_v20.dhaz_adp_air_base, pAdehazeCtx->stats.dehaze_stats_v20.dhaz_adp_wt,
                     pAdehazeCtx->stats.dehaze_stats_v20.dhaz_adp_gratio, pAdehazeCtx->stats.dehaze_stats_v20.dhaz_adp_tmax);
        for(int i = 0; i < ISP2X_DHAZ_HIST_IIR_NUM; i++) {
            LOGV_ADEHAZE("%s:  h_b_iir[%d]:%d:\n", __FUNCTION__, i, pAdehazeCtx->stats.dehaze_stats_v20.h_b_iir[i]);
            LOGV_ADEHAZE("%s:  h_g_iir[%d]:%d:\n", __FUNCTION__, i, pAdehazeCtx->stats.dehaze_stats_v20.h_g_iir[i]);
            LOGV_ADEHAZE("%s:  h_r_iir[%d]:%d:\n", __FUNCTION__, i, pAdehazeCtx->stats.dehaze_stats_v20.h_r_iir[i]);
        }
    }
    else if(CHECK_ISP_HW_V21()) {
        pAdehazeCtx->stats.dehaze_stats_v21.dhaz_adp_air_base = ROData->dehaze_stats_v21.dhaz_adp_air_base;
        pAdehazeCtx->stats.dehaze_stats_v21.dhaz_adp_wt = ROData->dehaze_stats_v21.dhaz_adp_wt;
        pAdehazeCtx->stats.dehaze_stats_v21.dhaz_adp_gratio = ROData->dehaze_stats_v21.dhaz_adp_gratio;
        pAdehazeCtx->stats.dehaze_stats_v21.dhaz_adp_tmax = ROData->dehaze_stats_v21.dhaz_adp_tmax;
        for(int i = 0; i < ISP21_DHAZ_HIST_IIR_NUM; i++)
            pAdehazeCtx->stats.dehaze_stats_v21.h_rgb_iir[i] = ROData->dehaze_stats_v21.h_rgb_iir[i];

        LOGV_ADEHAZE("%s:  dhaz_adp_air_base:%d dhaz_adp_wt:%d dhaz_adp_gratio:%d dhaz_adp_tmax:%d\n", __FUNCTION__,
                     pAdehazeCtx->stats.dehaze_stats_v21.dhaz_adp_air_base, pAdehazeCtx->stats.dehaze_stats_v21.dhaz_adp_wt,
                     pAdehazeCtx->stats.dehaze_stats_v21.dhaz_adp_gratio, pAdehazeCtx->stats.dehaze_stats_v21.dhaz_adp_tmax);
        for(int i = 0; i < ISP21_DHAZ_HIST_IIR_NUM; i++)
            LOGV_ADEHAZE("%s:  h_rgb_iir[%d]:%d:\n", __FUNCTION__, i, pAdehazeCtx->stats.dehaze_stats_v21.h_rgb_iir[i]);
    }
    else if(CHECK_ISP_HW_V30()) {
        pAdehazeCtx->stats.dehaze_stats_v30.dhaz_adp_air_base = ROData->dehaze_stats_v30.dhaz_adp_air_base;
        pAdehazeCtx->stats.dehaze_stats_v30.dhaz_adp_wt = ROData->dehaze_stats_v30.dhaz_adp_wt;
        pAdehazeCtx->stats.dehaze_stats_v30.dhaz_adp_gratio = ROData->dehaze_stats_v30.dhaz_adp_gratio;
        pAdehazeCtx->stats.dehaze_stats_v30.dhaz_adp_tmax = ROData->dehaze_stats_v30.dhaz_adp_tmax;
        pAdehazeCtx->stats.dehaze_stats_v30.dhaz_pic_sumh_left = ROData->dehaze_stats_v30.dhaz_pic_sumh_left;
        pAdehazeCtx->stats.dehaze_stats_v30.dhaz_pic_sumh_right = ROData->dehaze_stats_v30.dhaz_pic_sumh_right;
        for(int i = 0; i < ISP3X_DHAZ_HIST_IIR_NUM; i++)
            pAdehazeCtx->stats.dehaze_stats_v30.h_rgb_iir[i] = ROData->dehaze_stats_v30.h_rgb_iir[i];

        LOGV_ADEHAZE("%s:  dhaz_adp_air_base:%d dhaz_adp_wt:%d dhaz_adp_gratio:%d dhaz_adp_tmax:%d dhaz_pic_sumh_left:%d dhaz_pic_sumh_right:%d\n", __FUNCTION__,
                     pAdehazeCtx->stats.dehaze_stats_v30.dhaz_adp_air_base, pAdehazeCtx->stats.dehaze_stats_v30.dhaz_adp_wt,
                     pAdehazeCtx->stats.dehaze_stats_v30.dhaz_adp_gratio, pAdehazeCtx->stats.dehaze_stats_v30.dhaz_adp_tmax,
                     pAdehazeCtx->stats.dehaze_stats_v30.dhaz_pic_sumh_left, pAdehazeCtx->stats.dehaze_stats_v30.dhaz_pic_sumh_right);
        for(int i = 0; i < ISP3X_DHAZ_HIST_IIR_NUM; i++)
            LOGV_ADEHAZE("%s:  h_rgb_iir[%d]:%d:\n", __FUNCTION__, i, pAdehazeCtx->stats.dehaze_stats_v30.h_rgb_iir[i]);
    }

    //get other stats from stats
    for(int i = 0; i < 225; i++)
    {
        pAdehazeCtx->stats.other_stats.short_luma[i] = ROData->other_stats.short_luma[i];
        pAdehazeCtx->stats.other_stats.long_luma[i] = ROData->other_stats.long_luma[i];
        pAdehazeCtx->stats.other_stats.tmo_luma[i] = ROData->other_stats.tmo_luma[i];
    }

    if(pAdehazeCtx->FrameNumber == HDR_3X_NUM)
    {
        for(int i = 0; i < 25; i++)
            pAdehazeCtx->stats.other_stats.middle_luma[i] = ROData->other_stats.middle_luma[i];
    }

    LOG1_ADEHAZE( "%s:exit!\n", __FUNCTION__);
}

void AdehazeGetEnvLvISO
(
    AdehazeHandle_t*           pAdehazeCtx,
    RkAiqAlgoPreResAe*         pAePreRes
) {
    LOG1_ADEHAZE( "%s:enter!\n", __FUNCTION__);

    if(pAePreRes == NULL) {
        LOGE_ADEHAZE( "%s:Ae Pre Res is NULL!\n", __FUNCTION__);
        pAdehazeCtx->CurrData.V21.EnvLv = 0.0;
        return;
    }

    if(CHECK_ISP_HW_V21()) {
        pAdehazeCtx->CurrData.V21.EnvLv = pAePreRes->ae_pre_res_rk.GlobalEnvLv[pAePreRes->ae_pre_res_rk.NormalIndex];

        //Normalize the current envLv for AEC
        pAdehazeCtx->CurrData.V21.EnvLv = (pAdehazeCtx->CurrData.V21.EnvLv  - MIN_ENV_LV) / (MAX_ENV_LV - MIN_ENV_LV);
        pAdehazeCtx->CurrData.V21.EnvLv = LIMIT_VALUE(pAdehazeCtx->CurrData.V21.EnvLv, ENVLVMAX, ENVLVMIN);

        //get iso
        if(pAdehazeCtx->FrameNumber == LINEAR_NUM)
            pAdehazeCtx->CurrData.V21.ISO = pAePreRes->ae_pre_res_rk.LinearExp.exp_real_params.analog_gain *
                                            pAePreRes->ae_pre_res_rk.LinearExp.exp_real_params.isp_dgain *
                                            pAePreRes->ae_pre_res_rk.LinearExp.exp_real_params.digital_gain * 50.0;
        else if(pAdehazeCtx->FrameNumber == HDR_2X_NUM || pAdehazeCtx->FrameNumber == HDR_3X_NUM)
            pAdehazeCtx->CurrData.V21.ISO = pAePreRes->ae_pre_res_rk.HdrExp[1].exp_real_params.analog_gain *
                                            pAePreRes->ae_pre_res_rk.HdrExp[1].exp_real_params.digital_gain * 50.0;
        pAdehazeCtx->CurrData.V21.ISO = LIMIT_VALUE(pAdehazeCtx->CurrData.V21.ISO, ISOMAX, ISOMIN);
    }
    else if(CHECK_ISP_HW_V30()) {
        pAdehazeCtx->CurrData.V30.EnvLv = pAePreRes->ae_pre_res_rk.GlobalEnvLv[pAePreRes->ae_pre_res_rk.NormalIndex];

        //Normalize the current envLv for AEC
        pAdehazeCtx->CurrData.V30.EnvLv = (pAdehazeCtx->CurrData.V30.EnvLv  - MIN_ENV_LV) / (MAX_ENV_LV - MIN_ENV_LV);
        pAdehazeCtx->CurrData.V30.EnvLv = LIMIT_VALUE(pAdehazeCtx->CurrData.V30.EnvLv, ENVLVMAX, ENVLVMIN);

        //get iso
        if(pAdehazeCtx->FrameNumber == LINEAR_NUM)
            pAdehazeCtx->CurrData.V30.ISO = pAePreRes->ae_pre_res_rk.LinearExp.exp_real_params.analog_gain *
                                            pAePreRes->ae_pre_res_rk.LinearExp.exp_real_params.isp_dgain *
                                            pAePreRes->ae_pre_res_rk.LinearExp.exp_real_params.digital_gain * 50.0;
        else if(pAdehazeCtx->FrameNumber == HDR_2X_NUM || pAdehazeCtx->FrameNumber == HDR_3X_NUM)
            pAdehazeCtx->CurrData.V30.ISO =
                pAePreRes->ae_pre_res_rk.HdrExp[1].exp_real_params.analog_gain *
                pAePreRes->ae_pre_res_rk.HdrExp[1].exp_real_params.isp_dgain *
                pAePreRes->ae_pre_res_rk.HdrExp[1].exp_real_params.digital_gain * 50.0;
        pAdehazeCtx->CurrData.V30.ISO = LIMIT_VALUE(pAdehazeCtx->CurrData.V30.ISO, ISOMAX, ISOMIN);
    }

    LOG1_ADEHAZE( "%s:exit!\n", __FUNCTION__);
}

void AdehazeGetCurrDataGroup(AdehazeHandle_t* pAdehazeCtx, RKAiqAecExpInfo_t* pAeEffExpo,
                             XCamVideoBuffer* pAePreRes, float* simga) {
    LOG1_ADEHAZE( "%s:enter!\n", __FUNCTION__);

    if(CHECK_ISP_HW_V20()) {
        int iso = 50;
        AdehazeExpInfo_t stExpInfo;
        memset(&stExpInfo, 0x00, sizeof(AdehazeExpInfo_t));

        stExpInfo.hdr_mode = 0;
        for(int i = 0; i < 3; i++) {
            stExpInfo.arIso[i] = 50;
            stExpInfo.arAGain[i] = 1.0;
            stExpInfo.arDGain[i] = 1.0;
            stExpInfo.arTime[i] = 0.01;
        }

        if(pAdehazeCtx->working_mode == RK_AIQ_WORKING_MODE_NORMAL) {
            stExpInfo.hdr_mode = 0;
        } else if(RK_AIQ_HDR_GET_WORKING_MODE(pAdehazeCtx->working_mode) == RK_AIQ_WORKING_MODE_ISP_HDR2) {
            stExpInfo.hdr_mode = 1;
        } else if(RK_AIQ_HDR_GET_WORKING_MODE(pAdehazeCtx->working_mode) == RK_AIQ_WORKING_MODE_ISP_HDR3) {
            stExpInfo.hdr_mode = 2;
        }

        if(pAeEffExpo != NULL) {
            if(pAdehazeCtx->working_mode == RK_AIQ_WORKING_MODE_NORMAL) {
                stExpInfo.arAGain[0] = pAeEffExpo->LinearExp.exp_real_params.analog_gain;
                stExpInfo.arDGain[0] = pAeEffExpo->LinearExp.exp_real_params.digital_gain;
                stExpInfo.arTime[0] = pAeEffExpo->LinearExp.exp_real_params.integration_time;
                stExpInfo.arIso[0] = stExpInfo.arAGain[0] * stExpInfo.arDGain[0] * 50;
            } else {
                for(int i = 0; i < 3; i++) {
                    stExpInfo.arAGain[i] = pAeEffExpo->HdrExp[i].exp_real_params.analog_gain;
                    stExpInfo.arDGain[i] = pAeEffExpo->HdrExp[i].exp_real_params.digital_gain;
                    stExpInfo.arTime[i] = pAeEffExpo->HdrExp[i].exp_real_params.integration_time;
                    stExpInfo.arIso[i] = stExpInfo.arAGain[i] * stExpInfo.arDGain[i] * 50;

                    LOGD_ADEHAZE("index:%d again:%f dgain:%f time:%f iso:%d hdr_mode:%d\n",
                                 i,
                                 stExpInfo.arAGain[i],
                                 stExpInfo.arDGain[i],
                                 stExpInfo.arTime[i],
                                 stExpInfo.arIso[i],
                                 stExpInfo.hdr_mode);
                }
            }
        } else {
            LOGE_ADEHAZE("%s:%d pAEPreRes is NULL, so use default instead \n", __FUNCTION__, __LINE__);
        }

        iso = stExpInfo.arIso[stExpInfo.hdr_mode];
        pAdehazeCtx->CurrData.V20.ISO = (float)iso;
    }
    else if(CHECK_ISP_HW_V21()) {
        RkAiqAlgoPreResAe* pAEPreRes = NULL;
        if (pAePreRes) {
            pAEPreRes = (RkAiqAlgoPreResAe*)pAePreRes->map(pAePreRes);
            AdehazeGetEnvLvISO(pAdehazeCtx, pAEPreRes);
        }
        else {
            pAdehazeCtx->CurrData.V21.EnvLv = 0.0;
            LOGW_ADEHAZE( "%s:PreResBuf is NULL!\n", __FUNCTION__);
        }
    }
    else if(CHECK_ISP_HW_V30()) {
        // get ynr sigma
        for (int i = 0; i < YNR_V3_ISO_CURVE_POINT_NUM; i++)
            pAdehazeCtx->YnrPorcRes.sigma[i] = simga[i];

        // get ae data
        RkAiqAlgoPreResAe* pAEPreRes = NULL;
        if (pAePreRes) {
            pAEPreRes = (RkAiqAlgoPreResAe*)pAePreRes->map(pAePreRes);
            AdehazeGetEnvLvISO(pAdehazeCtx, pAEPreRes);
        }
        else {
            pAdehazeCtx->CurrData.V30.EnvLv = 0.0;
            LOGE_ADEHAZE( "%s:PreResBuf is NULL!\n", __FUNCTION__);
        }
    }

    LOG1_ADEHAZE( "%s:exit!\n", __FUNCTION__);
}

void AdehazeGetCurrData
(
    AdehazeHandle_t*           pAdehazeCtx,
    RkAiqAlgoProcAdhaz*         pProcPara
) {
    LOG1_ADEHAZE( "%s:enter!\n", __FUNCTION__);

    if(CHECK_ISP_HW_V20()) {
        int iso = 50;
        AdehazeExpInfo_t stExpInfo;
        memset(&stExpInfo, 0x00, sizeof(AdehazeExpInfo_t));

        stExpInfo.hdr_mode = 0;
        for(int i = 0; i < 3; i++) {
            stExpInfo.arIso[i] = 50;
            stExpInfo.arAGain[i] = 1.0;
            stExpInfo.arDGain[i] = 1.0;
            stExpInfo.arTime[i] = 0.01;
        }

        if(pAdehazeCtx->working_mode == RK_AIQ_WORKING_MODE_NORMAL) {
            stExpInfo.hdr_mode = 0;
        } else if(RK_AIQ_HDR_GET_WORKING_MODE(pAdehazeCtx->working_mode) == RK_AIQ_WORKING_MODE_ISP_HDR2) {
            stExpInfo.hdr_mode = 1;
        } else if(RK_AIQ_HDR_GET_WORKING_MODE(pAdehazeCtx->working_mode) == RK_AIQ_WORKING_MODE_ISP_HDR3) {
            stExpInfo.hdr_mode = 2;
        }

        XCamVideoBuffer* xCamAePreRes = pProcPara->com.u.proc.res_comb->ae_pre_res;
        RkAiqAlgoPreResAe* pAEPreRes = NULL;
        if (xCamAePreRes) {
            pAEPreRes = (RkAiqAlgoPreResAe*)xCamAePreRes->map(xCamAePreRes);
            if(pAdehazeCtx->working_mode == RK_AIQ_WORKING_MODE_NORMAL) {
                stExpInfo.arAGain[0] = pAEPreRes->ae_pre_res_rk.LinearExp.exp_real_params.analog_gain;
                stExpInfo.arDGain[0] = pAEPreRes->ae_pre_res_rk.LinearExp.exp_real_params.digital_gain;
                stExpInfo.arTime[0] = pAEPreRes->ae_pre_res_rk.LinearExp.exp_real_params.integration_time;
                stExpInfo.arIso[0] = stExpInfo.arAGain[0] * stExpInfo.arDGain[0] * 50;
            } else {
                for(int i = 0; i < 3; i++) {
                    stExpInfo.arAGain[i] = pAEPreRes->ae_pre_res_rk.HdrExp[i].exp_real_params.analog_gain;
                    stExpInfo.arDGain[i] = pAEPreRes->ae_pre_res_rk.HdrExp[i].exp_real_params.digital_gain;
                    stExpInfo.arTime[i] = pAEPreRes->ae_pre_res_rk.HdrExp[i].exp_real_params.integration_time;
                    stExpInfo.arIso[i] = stExpInfo.arAGain[i] * stExpInfo.arDGain[i] * 50;

                    LOGD_ADEHAZE("index:%d again:%f dgain:%f time:%f iso:%d hdr_mode:%d\n",
                                 i,
                                 stExpInfo.arAGain[i],
                                 stExpInfo.arDGain[i],
                                 stExpInfo.arTime[i],
                                 stExpInfo.arIso[i],
                                 stExpInfo.hdr_mode);
                }
            }
        } else {
            LOGE_ADEHAZE("%s:%d pAEPreRes is NULL, so use default instead \n", __FUNCTION__, __LINE__);
        }

        iso = stExpInfo.arIso[stExpInfo.hdr_mode];
        pAdehazeCtx->CurrData.V20.ISO = (float)iso;
    }
    else if(CHECK_ISP_HW_V21()) {
        XCamVideoBuffer* xCamAePreRes = pProcPara->com.u.proc.res_comb->ae_pre_res;
        RkAiqAlgoPreResAe* pAEPreRes = NULL;
        if (xCamAePreRes) {
            pAEPreRes = (RkAiqAlgoPreResAe*)xCamAePreRes->map(xCamAePreRes);
            AdehazeGetEnvLvISO(pAdehazeCtx, pAEPreRes);
        }
        else {
            pAdehazeCtx->CurrData.V21.EnvLv = 0.0;
            pAdehazeCtx->CurrData.V30.ISO = 50.0;
            LOGW_ADEHAZE( "%s:PreResBuf is NULL!\n", __FUNCTION__);
        }
    }
    else if(CHECK_ISP_HW_V30()) {
        for (int i = 0; i < YNR_V3_ISO_CURVE_POINT_NUM; i++)
            pAdehazeCtx->YnrPorcRes.sigma[i] = pProcPara->ynrV3_proc_res.sigma[i];

        // get ae data
        XCamVideoBuffer* xCamAePreRes = pProcPara->com.u.proc.res_comb->ae_pre_res;
        RkAiqAlgoPreResAe* pAEPreRes = NULL;
        if (xCamAePreRes) {
            pAEPreRes = (RkAiqAlgoPreResAe*)xCamAePreRes->map(xCamAePreRes);
            AdehazeGetEnvLvISO(pAdehazeCtx, pAEPreRes);
        }
        else {
            pAdehazeCtx->CurrData.V30.EnvLv = 0.0;
            pAdehazeCtx->CurrData.V30.ISO = 50.0;
            LOGE_ADEHAZE( "%s:PreResBuf is NULL!\n", __FUNCTION__);
        }
    }

    LOG1_ADEHAZE( "%s:exit!\n", __FUNCTION__);
}

void stAuto2Calib(AdehazeHandle_t* pAdehazeCtx) {
    LOG1_ADEHAZE("%s:enter!\n", __FUNCTION__);

    pAdehazeCtx->pCalib->DehazeTuningPara.Enable =
        pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.Enable;
    pAdehazeCtx->pCalib->DehazeTuningPara.CtrlDataType =
        pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.CtrlDataType;
    pAdehazeCtx->pCalib->DehazeTuningPara.cfg_alpha =
        pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.cfg_alpha;
    pAdehazeCtx->pCalib->DehazeTuningPara.ByPassThr =
        pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.ByPassThr;
    pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.en =
        pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.en;
    pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.air_lc_en =
        pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.air_lc_en;
    pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.stab_fnum =
        pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.stab_fnum;
    pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.sigma =
        pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.sigma;
    pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.wt_sigma =
        pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.wt_sigma;
    pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.air_sigma =
        pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.air_sigma;
    pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.tmax_sigma =
        pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.tmax_sigma;
    pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.pre_wet =
        pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.pre_wet;
    pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.en =
        pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.en;
    pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.en =
        pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.en;
    pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.hist_para_en =
        pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.hist_para_en;
    for (int i = 0; i < CALIBDB_ADEHAZE_ENHANCE_CURVE_KNOTS_NUM; i++)
        pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.enhance_curve[i] =
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.enhance_curve[i];
    if (pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len <=
        DHAZ_CTRL_DATA_STEP_MAX) {
        for (int i = 0;
             i < pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len;
             i++) {
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .CtrlData[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dc_min_th[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .dc_min_th[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dc_max_th[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .dc_max_th[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.yhist_th[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .yhist_th[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.yblk_th[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .yblk_th[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dark_th[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .dark_th[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.bright_min[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .bright_min[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.bright_max[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .bright_max[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.wt_max[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .wt_max[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.air_min[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .air_min[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.air_max[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .air_max[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.tmax_base[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .tmax_base[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.tmax_off[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .tmax_off[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.tmax_max[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .tmax_max[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.cfg_wt[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .cfg_wt[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.cfg_air[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .cfg_air[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.cfg_tmax[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .cfg_tmax[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dc_weitcur[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .dc_weitcur[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.bf_weight[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .bf_weight[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.range_sigma[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .range_sigma[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.space_sigma_cur[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .space_sigma_cur[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.space_sigma_pre[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .space_sigma_pre[i];
        }
    } else {
        for (int i = 0; i < DHAZ_CTRL_DATA_STEP_MAX; i++) {
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .CtrlData[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dc_min_th[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .dc_min_th[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dc_max_th[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .dc_max_th[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.yhist_th[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .yhist_th[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.yblk_th[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .yblk_th[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dark_th[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .dark_th[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.bright_min[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .bright_min[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.bright_max[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .bright_max[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.wt_max[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .wt_max[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.air_min[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .air_min[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.air_max[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .air_max[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.tmax_base[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .tmax_base[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.tmax_off[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .tmax_off[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.tmax_max[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .tmax_max[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.cfg_wt[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .cfg_wt[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.cfg_air[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .cfg_air[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.cfg_tmax[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .cfg_tmax[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dc_weitcur[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .dc_weitcur[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.bf_weight[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .bf_weight[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.range_sigma[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .range_sigma[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.space_sigma_cur[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .space_sigma_cur[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.space_sigma_pre[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .space_sigma_pre[i];
        }
        for (int i = DHAZ_CTRL_DATA_STEP_MAX;
             i < pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len;
             i++) {
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .CtrlData[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dc_min_th[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .dc_min_th[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dc_max_th[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .dc_max_th[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.yhist_th[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .yhist_th[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.yblk_th[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .yblk_th[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dark_th[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .dark_th[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.bright_min[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .bright_min[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.bright_max[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .bright_max[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.wt_max[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .wt_max[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.air_min[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .air_min[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.air_max[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .air_max[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.tmax_base[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .tmax_base[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.tmax_off[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .tmax_off[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.tmax_max[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .tmax_max[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.cfg_wt[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .cfg_wt[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.cfg_air[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .cfg_air[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.cfg_tmax[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .cfg_tmax[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dc_weitcur[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .dc_weitcur[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.bf_weight[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .bf_weight[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.range_sigma[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .range_sigma[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.space_sigma_cur[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .space_sigma_cur[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.space_sigma_pre[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                    .space_sigma_pre[DHAZ_CTRL_DATA_STEP_MAX - 1];
        }
    }
    if (pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.CtrlData_len <=
        DHAZ_CTRL_DATA_STEP_MAX) {
        for (int i = 0;
             i < pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.CtrlData_len;
             i++) {
            pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.CtrlData[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.EnhanceData
                    .CtrlData[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.enhance_chroma[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.EnhanceData
                    .enhance_chroma[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.enhance_value[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.EnhanceData
                    .enhance_value[i];
        }
    } else {
        for (int i = 0; i < DHAZ_CTRL_DATA_STEP_MAX; i++) {
            pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.CtrlData[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.EnhanceData
                    .CtrlData[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.enhance_chroma[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.EnhanceData
                    .enhance_chroma[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.enhance_value[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.EnhanceData
                    .enhance_value[i];
        }
        for (int i = DHAZ_CTRL_DATA_STEP_MAX;
             i < pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.CtrlData_len;
             i++) {
            pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.CtrlData[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.EnhanceData
                    .CtrlData[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.enhance_chroma[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.EnhanceData
                    .enhance_chroma[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.enhance_value[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.EnhanceData
                    .enhance_value[DHAZ_CTRL_DATA_STEP_MAX - 1];
        }
    }
    if (pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.CtrlData_len <=
        DHAZ_CTRL_DATA_STEP_MAX) {
        for (int i = 0;
             i < pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.CtrlData_len; i++) {
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.CtrlData[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.CtrlData[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_gratio[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData
                    .hist_gratio[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_th_off[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData
                    .hist_th_off[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_k[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.hist_k[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_min[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.hist_min[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_scale[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData
                    .hist_scale[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.cfg_gratio[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData
                    .cfg_gratio[i];
        }
    } else {
        for (int i = 0; i < DHAZ_CTRL_DATA_STEP_MAX; i++) {
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.CtrlData[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.CtrlData[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_gratio[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData
                    .hist_gratio[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_th_off[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData
                    .hist_th_off[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_k[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.hist_k[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_min[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.hist_min[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_scale[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData
                    .hist_scale[i];
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.cfg_gratio[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData
                    .cfg_gratio[i];
        }
        for (int i = DHAZ_CTRL_DATA_STEP_MAX;
             i < pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.CtrlData_len; i++) {
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.CtrlData[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData
                    .CtrlData[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_gratio[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData
                    .hist_gratio[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_th_off[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData
                    .hist_th_off[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_k[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData
                    .hist_k[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_min[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData
                    .hist_min[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_scale[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData
                    .hist_scale[DHAZ_CTRL_DATA_STEP_MAX - 1];
            pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.cfg_gratio[i] =
                pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData
                    .cfg_gratio[DHAZ_CTRL_DATA_STEP_MAX - 1];
        }
    }

    LOG1_ADEHAZE("%s:exit!\n", __FUNCTION__);
}

void Calib2stAuto(AdehazeHandle_t* pAdehazeCtx) {
    LOG1_ADEHAZE("%s:enter!\n", __FUNCTION__);

    pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.Enable =
        pAdehazeCtx->pCalib->DehazeTuningPara.Enable;
    pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.CtrlDataType =
        pAdehazeCtx->pCalib->DehazeTuningPara.CtrlDataType;
    pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.cfg_alpha =
        pAdehazeCtx->pCalib->DehazeTuningPara.cfg_alpha;
    pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.ByPassThr =
        pAdehazeCtx->pCalib->DehazeTuningPara.ByPassThr;
    pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.en =
        pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.en;
    pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.air_lc_en =
        pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.air_lc_en;
    pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.stab_fnum =
        pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.stab_fnum;
    pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.sigma =
        pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.sigma;
    pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.wt_sigma =
        pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.wt_sigma;
    pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.air_sigma =
        pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.air_sigma;
    pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.tmax_sigma =
        pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.tmax_sigma;
    pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.pre_wet =
        pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.pre_wet;
    pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.en =
        pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.en;
    pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.en =
        pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.en;
    pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.hist_para_en =
        pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.hist_para_en;
    for (int i = 0; i < CALIBDB_ADEHAZE_ENHANCE_CURVE_KNOTS_NUM; i++)
        pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.enhance_curve[i] =
            pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.enhance_curve[i];
    if (pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len >=
        DHAZ_CTRL_DATA_STEP_MAX) {
        for (int i = 0; i < DHAZ_CTRL_DATA_STEP_MAX; i++) {
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.CtrlData[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .dc_min_th[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dc_min_th[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .dc_max_th[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dc_max_th[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.yhist_th[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.yhist_th[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.yblk_th[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.yblk_th[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.dark_th[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dark_th[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .bright_min[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.bright_min[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .bright_max[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.bright_max[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.wt_max[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.wt_max[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.air_min[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.air_min[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.air_max[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.air_max[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .tmax_base[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.tmax_base[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.tmax_off[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.tmax_off[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.tmax_max[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.tmax_max[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.cfg_wt[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.cfg_wt[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.cfg_air[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.cfg_air[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.cfg_tmax[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.cfg_tmax[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .dc_weitcur[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dc_weitcur[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .bf_weight[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.bf_weight[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .range_sigma[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.range_sigma[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .space_sigma_cur[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.space_sigma_cur[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .space_sigma_pre[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.space_sigma_pre[i];
        }
    } else {
        for (int i = 0;
             i < pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len;
             i++) {
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.CtrlData[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .dc_min_th[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dc_min_th[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .dc_max_th[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dc_max_th[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.yhist_th[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.yhist_th[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.yblk_th[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.yblk_th[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.dark_th[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dark_th[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .bright_min[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.bright_min[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .bright_max[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.bright_max[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.wt_max[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.wt_max[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.air_min[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.air_min[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.air_max[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.air_max[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .tmax_base[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.tmax_base[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.tmax_off[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.tmax_off[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.tmax_max[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.tmax_max[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.cfg_wt[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.cfg_wt[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.cfg_air[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.cfg_air[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.cfg_tmax[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.cfg_tmax[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .dc_weitcur[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dc_weitcur[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .bf_weight[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.bf_weight[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .range_sigma[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.range_sigma[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .space_sigma_cur[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.space_sigma_cur[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .space_sigma_pre[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.space_sigma_pre[i];
        }
        for (int i = pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len;
             i < DHAZ_CTRL_DATA_STEP_MAX; i++) {
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.CtrlData[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData
                    [pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len -
                     1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .dc_min_th[i] = pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData
                                    .dc_min_th[pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting
                                                   .DehazeData.CtrlData_len -
                                               1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .dc_max_th[i] = pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData
                                    .dc_max_th[pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting
                                                   .DehazeData.CtrlData_len -
                                               1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.yhist_th[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.yhist_th
                    [pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len -
                     1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.yblk_th[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.yblk_th
                    [pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len -
                     1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.dark_th[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.dark_th
                    [pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len -
                     1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .bright_min[i] = pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData
                                     .bright_min[pAdehazeCtx->pCalib->DehazeTuningPara
                                                     .dehaze_setting.DehazeData.CtrlData_len -
                                                 1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .bright_max[i] = pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData
                                     .bright_max[pAdehazeCtx->pCalib->DehazeTuningPara
                                                     .dehaze_setting.DehazeData.CtrlData_len -
                                                 1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.wt_max[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.wt_max
                    [pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len -
                     1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.air_min[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.air_min
                    [pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len -
                     1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.air_max[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.air_max
                    [pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len -
                     1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .tmax_base[i] = pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData
                                    .tmax_base[pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting
                                                   .DehazeData.CtrlData_len -
                                               1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.tmax_off[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.tmax_off
                    [pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len -
                     1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.tmax_max[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.tmax_max
                    [pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len -
                     1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.cfg_wt[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.cfg_wt
                    [pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len -
                     1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.cfg_air[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.cfg_air
                    [pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len -
                     1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData.cfg_tmax[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.cfg_tmax
                    [pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len -
                     1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .dc_weitcur[i] = pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData
                                     .dc_weitcur[pAdehazeCtx->pCalib->DehazeTuningPara
                                                     .dehaze_setting.DehazeData.CtrlData_len -
                                                 1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .bf_weight[i] = pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData
                                    .bf_weight[pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting
                                                   .DehazeData.CtrlData_len -
                                               1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .range_sigma[i] = pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData
                                      .range_sigma[pAdehazeCtx->pCalib->DehazeTuningPara
                                                       .dehaze_setting.DehazeData.CtrlData_len -
                                                   1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .space_sigma_cur[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.space_sigma_cur
                    [pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len -
                     1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.dehaze_setting.DehazeData
                .space_sigma_pre[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.space_sigma_pre
                    [pAdehazeCtx->pCalib->DehazeTuningPara.dehaze_setting.DehazeData.CtrlData_len -
                     1];
        }
    }

    if (pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.CtrlData_len >=
        DHAZ_CTRL_DATA_STEP_MAX) {
        for (int i = 0; i < DHAZ_CTRL_DATA_STEP_MAX; i++) {
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.EnhanceData
                .CtrlData[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.CtrlData[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.EnhanceData
                .enhance_chroma[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.enhance_chroma[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.EnhanceData
                .enhance_value[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.enhance_value[i];
        }
    } else {
        for (int i = 0;
             i < pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.CtrlData_len;
             i++) {
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.EnhanceData
                .CtrlData[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.CtrlData[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.EnhanceData
                .enhance_chroma[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.enhance_chroma[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.EnhanceData
                .enhance_value[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.enhance_value[i];
        }
        for (int i = pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData.CtrlData_len;
             i < DHAZ_CTRL_DATA_STEP_MAX; i++) {
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.EnhanceData
                .CtrlData[i] = pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData
                                   .CtrlData[pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting
                                                 .EnhanceData.CtrlData_len -
                                             1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.EnhanceData
                .enhance_chroma[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData
                    .enhance_chroma[pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting
                                        .EnhanceData.CtrlData_len -
                                    1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.enhance_setting.EnhanceData
                .enhance_value[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData
                    .enhance_value[pAdehazeCtx->pCalib->DehazeTuningPara.enhance_setting.EnhanceData
                                       .CtrlData_len -
                                   1];
        }
    }
    if (pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.CtrlData_len >=
        DHAZ_CTRL_DATA_STEP_MAX) {
        for (int i = 0; i < DHAZ_CTRL_DATA_STEP_MAX; i++) {
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.CtrlData[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.CtrlData[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.hist_gratio[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_gratio[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.hist_th_off[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_th_off[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.hist_k[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_k[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.hist_min[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_min[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.hist_scale[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_scale[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.cfg_gratio[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.cfg_gratio[i];
        }
    } else {
        for (int i = 0;
             i < pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.CtrlData_len; i++) {
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.CtrlData[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.CtrlData[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.hist_gratio[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_gratio[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.hist_th_off[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_th_off[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.hist_k[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_k[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.hist_min[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_min[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.hist_scale[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_scale[i];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.cfg_gratio[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.cfg_gratio[i];
        }
        for (int i = pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.CtrlData_len;
             i < DHAZ_CTRL_DATA_STEP_MAX; i++) {
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.CtrlData[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.CtrlData
                    [pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.CtrlData_len - 1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.hist_gratio[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_gratio
                    [pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.CtrlData_len - 1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.hist_th_off[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_th_off
                    [pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.CtrlData_len - 1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.hist_k[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_k
                    [pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.CtrlData_len - 1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.hist_min[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_min
                    [pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.CtrlData_len - 1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.hist_scale[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.hist_scale
                    [pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.CtrlData_len - 1];
            pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.hist_setting.HistData.cfg_gratio[i] =
                pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.cfg_gratio
                    [pAdehazeCtx->pCalib->DehazeTuningPara.hist_setting.HistData.CtrlData_len - 1];
        }
    }

    LOG1_ADEHAZE("%s:exit!\n", __FUNCTION__);
}

XCamReturn AdehazeInit(AdehazeHandle_t** pAdehazeCtx, CamCalibDbV2Context_t* pCalib)
{
    LOG1_ADEHAZE("ENTER: %s \n", __func__);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    AdehazeHandle_t *handle = (AdehazeHandle_t*)malloc(sizeof(AdehazeHandle_t));
    if (NULL == handle)
        return XCAM_RETURN_ERROR_MEM;
    memset(handle, 0, sizeof(AdehazeHandle_t));

    if(CHECK_ISP_HW_V20())
        handle->HWversion = ADEHAZE_ISP20;
    else if(CHECK_ISP_HW_V21())
        handle->HWversion = ADEHAZE_ISP21;
    else if(CHECK_ISP_HW_V30())
        handle->HWversion = ADEHAZE_ISP30;

    if(handle->HWversion == ADEHAZE_ISP20) {
        // isp20 todo

    } else if(handle->HWversion == ADEHAZE_ISP21) {
        //isp21
        CalibDbV2_dehaze_V21_t* calibv2_adehaze_calib_V21 =
            (CalibDbV2_dehaze_V21_t*)(CALIBDBV2_GET_MODULE_PTR(pCalib, adehaze_calib_v21));
        handle->pCalib = calibv2_adehaze_calib_V21;

        handle->PreData.V21.EnvLv = 0.0;
        handle->PreData.V21.ApiMode = DEHAZE_API_AUTO;
    } else if(handle->HWversion == ADEHAZE_ISP30) {
        //isp30
        CalibDbV2_dehaze_V21_t* calibv2_adehaze_calib_V30 =
            (CalibDbV2_dehaze_V21_t*)(CALIBDBV2_GET_MODULE_PTR(pCalib, adehaze_calib_v30));
        handle->pCalib = calibv2_adehaze_calib_V30;

        handle->PreData.V30.EnvLv = 0.0;
        handle->PreData.V30.ApiMode = DEHAZE_API_AUTO;
    }
    Calib2stAuto(handle);

    //set api default
    handle->AdehazeAtrr.mode                                               = DEHAZE_API_AUTO;
    handle->AdehazeAtrr.stManual.Enable = true;
    handle->AdehazeAtrr.stManual.cfg_alpha = 1.0;
    handle->AdehazeAtrr.stManual.dehaze_setting.en = false;
    handle->AdehazeAtrr.stManual.dehaze_setting.air_lc_en = true;
    handle->AdehazeAtrr.stManual.dehaze_setting.stab_fnum = 8;
    handle->AdehazeAtrr.stManual.dehaze_setting.sigma = 6;
    handle->AdehazeAtrr.stManual.dehaze_setting.wt_sigma = 8;
    handle->AdehazeAtrr.stManual.dehaze_setting.air_sigma = 120;
    handle->AdehazeAtrr.stManual.dehaze_setting.tmax_sigma = 0.01;
    handle->AdehazeAtrr.stManual.dehaze_setting.pre_wet = 0.01;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.dc_min_th = 64;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.dc_max_th = 192;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.yhist_th = 249;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.yblk_th = 0.002;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.dark_th = 250;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.bright_min = 180;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.bright_max = 240;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.wt_max = 0.9;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.air_min = 200;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.air_max = 250;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.tmax_base = 125;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.tmax_off = 0.1;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.tmax_max = 0.8;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.cfg_wt = 0.8;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.cfg_air = 210;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.cfg_tmax = 0.2;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.dc_weitcur = 1;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.bf_weight = 0.5;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.range_sigma = 0.14;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.space_sigma_pre = 0.14;
    handle->AdehazeAtrr.stManual.dehaze_setting.DehazeData.space_sigma_cur = 0.14;

    handle->AdehazeAtrr.stManual.enhance_setting.en = true;
    handle->AdehazeAtrr.stManual.enhance_setting.EnhanceData.enhance_value = 1.0;
    handle->AdehazeAtrr.stManual.enhance_setting.EnhanceData.enhance_chroma = 1.0;
    handle->AdehazeAtrr.stManual.enhance_setting.enhance_curve[0] = 0;
    handle->AdehazeAtrr.stManual.enhance_setting.enhance_curve[1] = 64;
    handle->AdehazeAtrr.stManual.enhance_setting.enhance_curve[2] = 128;
    handle->AdehazeAtrr.stManual.enhance_setting.enhance_curve[3] = 192;
    handle->AdehazeAtrr.stManual.enhance_setting.enhance_curve[4] = 256;
    handle->AdehazeAtrr.stManual.enhance_setting.enhance_curve[5] = 320;
    handle->AdehazeAtrr.stManual.enhance_setting.enhance_curve[6] = 384;
    handle->AdehazeAtrr.stManual.enhance_setting.enhance_curve[7] = 448;
    handle->AdehazeAtrr.stManual.enhance_setting.enhance_curve[8] = 512;
    handle->AdehazeAtrr.stManual.enhance_setting.enhance_curve[9] = 576;
    handle->AdehazeAtrr.stManual.enhance_setting.enhance_curve[10] = 640;
    handle->AdehazeAtrr.stManual.enhance_setting.enhance_curve[11] = 704;
    handle->AdehazeAtrr.stManual.enhance_setting.enhance_curve[12] = 768;
    handle->AdehazeAtrr.stManual.enhance_setting.enhance_curve[13] = 832;
    handle->AdehazeAtrr.stManual.enhance_setting.enhance_curve[14] = 896;
    handle->AdehazeAtrr.stManual.enhance_setting.enhance_curve[15] = 960;
    handle->AdehazeAtrr.stManual.enhance_setting.enhance_curve[16] = 1023;

    handle->AdehazeAtrr.stManual.hist_setting.en = false;
    handle->AdehazeAtrr.stManual.hist_setting.hist_para_en = true;
    handle->AdehazeAtrr.stManual.hist_setting.HistData.hist_gratio = 2;
    handle->AdehazeAtrr.stManual.hist_setting.HistData.hist_th_off = 64;
    handle->AdehazeAtrr.stManual.hist_setting.HistData.hist_k = 2;
    handle->AdehazeAtrr.stManual.hist_setting.HistData.hist_min = 0.015;
    handle->AdehazeAtrr.stManual.hist_setting.HistData.hist_scale = 0.09;
    handle->AdehazeAtrr.stManual.hist_setting.HistData.cfg_gratio = 2;

    handle->AdehazeAtrr.stDehazeManu.level  = DEHAZE_API_MANUAL_DEFAULT_LEVEL;
    handle->AdehazeAtrr.stEnhanceManu.level = DEHAZE_API_ENHANCE_MANUAL_DEFAULT_LEVEL;

    *pAdehazeCtx = handle;
    LOG1_ADEHAZE("EXIT: %s \n", __func__);
    return(ret);
}

XCamReturn AdehazeRelease(AdehazeHandle_t* pAdehazeCtx)
{
    LOG1_ADEHAZE("ENTER: %s \n", __func__);
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (pAdehazeCtx)
        free(pAdehazeCtx);
    LOG1_ADEHAZE("EXIT: %s \n", __func__);
    return(ret);
}

XCamReturn AdehazeProcess(AdehazeHandle_t* pAdehazeCtx, AdehazeVersion_t version)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    LOG1_ADEHAZE("ENTER: %s \n", __func__);
    float CtrlValue = 0.0f;

    if(version == ADEHAZE_ISP20) {
        //todo
    }
    else if(version == ADEHAZE_ISP21) {
        if (pAdehazeCtx->AdehazeAtrr.mode == DEHAZE_API_AUTO) {
            LOGD_ADEHAZE(" %s: Adehaze Api off!!!\n", __func__);
            CtrlValue = pAdehazeCtx->CurrData.V21.EnvLv;

            // cfg setting
            pAdehazeCtx->ProcRes.ProcResV21.cfg_alpha = LIMIT_VALUE(
                (int)(pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.cfg_alpha * 256.0), 255, 0);

            // enable setting
            EnableSettingV21(&pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara,
                             &pAdehazeCtx->ProcRes);

            // dehaze setting
            GetDehazeParamsV21(&pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara,
                               pAdehazeCtx->AdehazeAtrr.stDehazeManu.level, &pAdehazeCtx->ProcRes,
                               CtrlValue);

            // enhance setting
            GetEnhanceParamsV21(&pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara,
                                pAdehazeCtx->AdehazeAtrr.stEnhanceManu.level, &pAdehazeCtx->ProcRes,
                                CtrlValue);

            // hist setting
            GetHistParamsV21(&pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara,
                             &pAdehazeCtx->ProcRes, CtrlValue);
        } else if (pAdehazeCtx->AdehazeAtrr.mode == DEHAZE_API_MANUAL) {
            LOGD_ADEHAZE(" %s: Adehaze Api stManual!!!\n", __func__);

            // cfg setting
            pAdehazeCtx->ProcRes.ProcResV21.cfg_alpha =
                LIMIT_VALUE((int)(pAdehazeCtx->AdehazeAtrr.stManual.cfg_alpha * 256.0), 255, 0);

            // enable setting
            stManuEnableSettingV21(&pAdehazeCtx->AdehazeAtrr.stManual, &pAdehazeCtx->ProcRes);

            // dehaze setting
            stManuGetDehazeParamsV21(&pAdehazeCtx->AdehazeAtrr.stManual,
                                     pAdehazeCtx->AdehazeAtrr.stDehazeManu.level,
                                     &pAdehazeCtx->ProcRes);

            // enhance setting
            stManuGetEnhanceParamsV21(&pAdehazeCtx->AdehazeAtrr.stManual,
                                      pAdehazeCtx->AdehazeAtrr.stEnhanceManu.level,
                                      &pAdehazeCtx->ProcRes);

            // hist setting
            stManuGetHistParamsV21(&pAdehazeCtx->AdehazeAtrr.stManual, &pAdehazeCtx->ProcRes);
        } else
            LOGE_ADEHAZE("%s:Wrong Adehaze API mode!!! \n", __func__);
    }
    else if(version == ADEHAZE_ISP30) {
        if (pAdehazeCtx->AdehazeAtrr.mode == DEHAZE_API_AUTO) {
            LOGD_ADEHAZE(" %s: Adehaze Api off!!!\n", __func__);
            CtrlValue = pAdehazeCtx->CurrData.V30.EnvLv;
            if (pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.CtrlDataType == CTRLDATATYPE_ISO)
                CtrlValue = pAdehazeCtx->CurrData.V30.ISO;

            // cfg setting
            pAdehazeCtx->ProcRes.ProcResV30.cfg_alpha = LIMIT_VALUE(
                (int)(pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.cfg_alpha * 256.0), 255, 0);

            // enable setting
            EnableSettingV30(&pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara,
                             &pAdehazeCtx->ProcRes);

            // dehaze setting
            GetDehazeParamsV30(&pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara,
                               pAdehazeCtx->AdehazeAtrr.stDehazeManu.level, &pAdehazeCtx->ProcRes,
                               CtrlValue);

            // enhance setting
            GetEnhanceParamsV30(&pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara,
                                pAdehazeCtx->AdehazeAtrr.stEnhanceManu.level, &pAdehazeCtx->ProcRes,
                                CtrlValue);

            // hist setting
            GetHistParamsV30(&pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara,
                             &pAdehazeCtx->ProcRes, CtrlValue);
        } else if (pAdehazeCtx->AdehazeAtrr.mode == DEHAZE_API_MANUAL) {
            LOGD_ADEHAZE(" %s: Adehaze Api stManual!!!\n", __func__);

            // cfg setting
            pAdehazeCtx->ProcRes.ProcResV30.cfg_alpha =
                LIMIT_VALUE((int)(pAdehazeCtx->AdehazeAtrr.stManual.cfg_alpha * 256.0), 255, 0);

            // enable setting
            stManuEnableSettingV30(&pAdehazeCtx->AdehazeAtrr.stManual, &pAdehazeCtx->ProcRes);

            // dehaze setting
            stManuGetDehazeParamsV30(&pAdehazeCtx->AdehazeAtrr.stManual,
                                     pAdehazeCtx->AdehazeAtrr.stDehazeManu.level,
                                     &pAdehazeCtx->ProcRes);

            // enhance setting
            stManuGetEnhanceParamsV30(&pAdehazeCtx->AdehazeAtrr.stManual,
                                      pAdehazeCtx->AdehazeAtrr.stEnhanceManu.level,
                                      &pAdehazeCtx->ProcRes);

            // hist setting
            stManuGetHistParamsV30(&pAdehazeCtx->AdehazeAtrr.stManual, &pAdehazeCtx->ProcRes);
        } else
            LOGE_ADEHAZE("%s:Wrong Adehaze API mode!!! \n", __func__);

        // get local gain setting
        GetDehazeLocalGainSettingV30(&pAdehazeCtx->ProcRes, &pAdehazeCtx->YnrPorcRes);

        // get Duo cam setting
        GetDehazeHistDuoISPSettingV30(&pAdehazeCtx->ProcRes, &pAdehazeCtx->stats,
                                      pAdehazeCtx->is_multi_isp_mode, pAdehazeCtx->FrameID);
    }
    else
        LOGE_ADEHAZE(" %s:Wrong hardware version!! \n", __func__);

    stAuto2Calib(pAdehazeCtx);

    //store pre data
    if(version == ADEHAZE_ISP20)
        pAdehazeCtx->PreData.V20.ISO = pAdehazeCtx->CurrData.V20.ISO;
    else if (version == ADEHAZE_ISP21) {
        pAdehazeCtx->PreData.V21.EnvLv = pAdehazeCtx->CurrData.V21.EnvLv;
        pAdehazeCtx->PreData.V21.ISO   = pAdehazeCtx->CurrData.V21.ISO;
    } else if (version == ADEHAZE_ISP30) {
        pAdehazeCtx->PreData.V30.EnvLv = pAdehazeCtx->CurrData.V30.EnvLv;
        pAdehazeCtx->PreData.V30.ISO   = pAdehazeCtx->CurrData.V30.ISO;
    }

    LOG1_ADEHAZE("EXIT: %s \n", __func__);
    return ret;
}

bool AdehazeByPassProcessing(AdehazeHandle_t* pAdehazeCtx)
{
    LOG1_ADEHAZE("ENTER: %s \n", __func__);
    bool ret = false;
    float diff = 0.0;

    if(pAdehazeCtx->FrameID <= 2)
        pAdehazeCtx->byPassProc = false;
    else if(CHECK_ISP_HW_V20()) {
        // todo
    }
    else if(CHECK_ISP_HW_V21()) {
        if (pAdehazeCtx->AdehazeAtrr.mode > DEHAZE_API_AUTO)
            pAdehazeCtx->byPassProc = false;
        else if(pAdehazeCtx->AdehazeAtrr.mode != pAdehazeCtx->PreData.V21.ApiMode)
            pAdehazeCtx->byPassProc = false;
        else {
            if (pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.CtrlDataType ==
                CTRLDATATYPE_ENVLV) {
                diff = pAdehazeCtx->PreData.V21.EnvLv - pAdehazeCtx->CurrData.V21.EnvLv;
                if (pAdehazeCtx->PreData.V21.EnvLv == 0.0) {
                    diff = pAdehazeCtx->CurrData.V21.EnvLv;
                    if (diff == 0.0)
                        pAdehazeCtx->byPassProc = true;
                    else
                        pAdehazeCtx->byPassProc = false;
                } else {
                    diff /= pAdehazeCtx->PreData.V21.EnvLv;
                    if (diff >= pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.ByPassThr ||
                        diff <= (0 - pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.ByPassThr))
                        pAdehazeCtx->byPassProc = false;
                    else
                        pAdehazeCtx->byPassProc = true;
                }
            } else if (pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.CtrlDataType ==
                       CTRLDATATYPE_ISO) {
                diff = pAdehazeCtx->PreData.V21.ISO - pAdehazeCtx->CurrData.V21.ISO;
                diff /= pAdehazeCtx->PreData.V21.ISO;
                if (diff >= pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.ByPassThr ||
                    diff <= (0 - pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.ByPassThr))
                    pAdehazeCtx->byPassProc = false;
                else
                    pAdehazeCtx->byPassProc = true;
            }
        }
    }
    else if(CHECK_ISP_HW_V30()) {
        if (pAdehazeCtx->AdehazeAtrr.mode > DEHAZE_API_AUTO)
            pAdehazeCtx->byPassProc = false;
        else if(pAdehazeCtx->AdehazeAtrr.mode != pAdehazeCtx->PreData.V30.ApiMode)
            pAdehazeCtx->byPassProc = false;
        else {
            if (pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.CtrlDataType ==
                CTRLDATATYPE_ENVLV) {
                diff = pAdehazeCtx->PreData.V30.EnvLv - pAdehazeCtx->CurrData.V30.EnvLv;
                if (pAdehazeCtx->PreData.V30.EnvLv == 0.0) {
                    diff = pAdehazeCtx->CurrData.V30.EnvLv;
                    if (diff == 0.0)
                        pAdehazeCtx->byPassProc = true;
                    else
                        pAdehazeCtx->byPassProc = false;
                } else {
                    diff /= pAdehazeCtx->PreData.V30.EnvLv;
                    if (diff >= pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.ByPassThr ||
                        diff <= (0 - pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.ByPassThr))
                        pAdehazeCtx->byPassProc = false;
                    else
                        pAdehazeCtx->byPassProc = true;
                }
            } else if (pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.CtrlDataType ==
                       CTRLDATATYPE_ISO) {
                diff = pAdehazeCtx->PreData.V30.ISO - pAdehazeCtx->CurrData.V30.ISO;
                diff /= pAdehazeCtx->PreData.V30.ISO;
                if (diff >= pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.ByPassThr ||
                    diff <= (0 - pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.ByPassThr))
                    pAdehazeCtx->byPassProc = false;
                else
                    pAdehazeCtx->byPassProc = true;
            }
        }
    }
    else
        LOGE_ADEHAZE(" %s:Wrong hardware version!! \n", __func__);

    ret = pAdehazeCtx->byPassProc;

    LOGD_ADEHAZE("%s:FrameID:%d CtrlDataType:%d EnvLv:%f ISO:%f diff:%f byPassProc:%d \n", __func__,
                 pAdehazeCtx->FrameID,
                 pAdehazeCtx->AdehazeAtrr.stAuto.DehazeTuningPara.CtrlDataType,
                 pAdehazeCtx->CurrData.V30.EnvLv, pAdehazeCtx->CurrData.V30.ISO, diff,
                 pAdehazeCtx->byPassProc);

    LOG1_ADEHAZE("EXIT: %s \n", __func__);
    return ret;
}

