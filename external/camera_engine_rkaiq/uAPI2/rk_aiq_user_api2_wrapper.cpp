/*
 *  Copyright (c) 2021 Rockchip Corporation
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

#include "uAPI2/rk_aiq_user_api2_wrapper.h"
#include "RkAiqCalibDbV2.h"
#include "RkAiqUapitypes.h"
#include "cJSON.h"
#include "cJSON_Utils.h"
#include "j2s.h"
#include "rk_aiq_user_api_sysctl.h"
#include "stdlib.h"
#include "string.h"
#include "uAPI/include/rk_aiq_user_api_sysctl.h"
#include "uAPI2/rk_aiq_user_api2_ae.h"
#include "uAPI2/rk_aiq_user_api2_acsm.h"
#include "uAPI2/rk_aiq_user_api2_imgproc.h"
#include "uAPI2/rk_aiq_user_api2_sysctl.h"
#include "rk_aiq_api_private.h"

static camgroup_uapi_t last_camindex;

int rk_aiq_uapi_sysctl_swWorkingModeDyn2(const rk_aiq_sys_ctx_t *ctx,
        work_mode_t *mode) {
    return rk_aiq_uapi_sysctl_swWorkingModeDyn(ctx, mode->mode);
}

int rk_aiq_uapi_sysctl_getWorkingModeDyn(const rk_aiq_sys_ctx_t *ctx,
        work_mode_t *mode) {
    return rk_aiq_uapi2_sysctl_getWorkingMode(ctx, &mode->mode);
}

int rk_aiq_uapi2_setWBMode2(rk_aiq_sys_ctx_t *ctx, uapi_wb_mode_t *mode) {
    return rk_aiq_uapi2_setWBMode(ctx, mode->mode);
}

int rk_aiq_uapi2_getWBMode2(rk_aiq_sys_ctx_t *ctx, uapi_wb_mode_t *mode) {
    return rk_aiq_uapi2_getWBMode(ctx, &mode->mode);
}

int rk_aiq_user_api2_amerge_GetCtldata(const rk_aiq_sys_ctx_t *sys_ctx,
                                       uapiMergeCurrCtlData_t *ctldata) {
    amerge_attrib_t setdata;

    memset(&setdata, 0, sizeof(amerge_attrib_t));

    rk_aiq_user_api2_amerge_GetAttrib(sys_ctx, &setdata);

    if (CHECK_ISP_HW_V21())
        memcpy(ctldata, &setdata.attrV21.CtlInfo, sizeof(uapiMergeCurrCtlData_t));
    else if (CHECK_ISP_HW_V30())
        memcpy(ctldata, &setdata.attrV30.CtlInfo, sizeof(uapiMergeCurrCtlData_t));

    return 0;
}

int rk_aiq_user_api2_set_scene(const rk_aiq_sys_ctx_t *sys_ctx,
                               aiq_scene_t *scene) {
    return rk_aiq_uapi_sysctl_switch_scene(sys_ctx, scene->main_scene,
                                           scene->sub_scene);
}

int rk_aiq_user_api2_get_scene(const rk_aiq_sys_ctx_t *sys_ctx,
                               aiq_scene_t *scene) {
    (void)sys_ctx;
    scene->main_scene = strdup("normal");
    scene->sub_scene = strdup("day");

    return 0;
}

int rk_aiq_uapi_get_ae_hwstats(const rk_aiq_sys_ctx_t *sys_ctx,
                               uapi_ae_hwstats_t *ae_hwstats) {
    rk_aiq_isp_stats_t isp_stats;

    if (sys_ctx->cam_type == RK_AIQ_CAM_TYPE_GROUP) {
        LOGE("Can't read 3A stats for group ctx!");
        return XCAM_RETURN_ERROR_PARAM;
    }

    rk_aiq_uapi_sysctl_get3AStats(sys_ctx, &isp_stats);

    memcpy(ae_hwstats, &isp_stats.aec_stats.ae_data, sizeof(uapi_ae_hwstats_t));

    return XCAM_RETURN_NO_ERROR;
}

int rk_aiq_uapi_get_awb_stat(const rk_aiq_sys_ctx_t *sys_ctx,
                             rk_tool_awb_stat_res2_v30_t *awb_stat) {
    rk_aiq_isp_stats_t isp_stats;

    if (sys_ctx->cam_type == RK_AIQ_CAM_TYPE_GROUP) {
        LOGE("Can't read 3A stats for group ctx!");
        return XCAM_RETURN_ERROR_PARAM;
    }

    rk_aiq_uapi_sysctl_get3AStats(sys_ctx, &isp_stats);

    memcpy(awb_stat, &isp_stats.awb_stats_v3x,
           sizeof(rk_tool_awb_stat_res2_v30_t));

    return 0;
}

XCamReturn rk_aiq_get_adpcc_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                        Adpcc_Manual_Attr_t *manual) {
    rk_aiq_dpcc_attrib_V20_t adpcc_attr;

    memset(&adpcc_attr, 0, sizeof(rk_aiq_dpcc_attrib_V20_t));
    rk_aiq_user_api2_adpcc_GetAttrib(sys_ctx, &adpcc_attr);
    memcpy(manual, &adpcc_attr.stManual, sizeof(Adpcc_Manual_Attr_t));

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn rk_aiq_set_adpcc_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                        Adpcc_Manual_Attr_t *manual) {
    rk_aiq_dpcc_attrib_V20_t adpcc_attr;

    memset(&adpcc_attr, 0, sizeof(rk_aiq_dpcc_attrib_V20_t));
    rk_aiq_user_api2_adpcc_GetAttrib(sys_ctx, &adpcc_attr);
    memcpy(&adpcc_attr.stManual, manual, sizeof(Adpcc_Manual_Attr_t));
    rk_aiq_user_api2_adpcc_SetAttrib(sys_ctx, &adpcc_attr);

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn rk_aiq_user_api2_adrc_queryinfo(const rk_aiq_sys_ctx_t *sys_ctx,
        DrcInfo_t *drc_info) {
    drc_attrib_t drc_attr;

    memset(&drc_attr, 0, sizeof(drc_attrib_t));
    rk_aiq_user_api2_adrc_GetAttrib(sys_ctx, &drc_attr);
    memcpy(drc_info, &drc_attr.Info, sizeof(DrcInfo_t));

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn rk_aiq_set_adrc_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                       mdrcAttr_V30_t *manual) {
    drc_attrib_t drc_attr;

    memset(&drc_attr, 0, sizeof(drc_attrib_t));
    rk_aiq_user_api2_adrc_GetAttrib(sys_ctx, &drc_attr);
    memcpy(&drc_attr.stManualV30, manual, sizeof(mdrcAttr_V30_t));
    rk_aiq_user_api2_adrc_SetAttrib(sys_ctx, drc_attr);

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn rk_aiq_get_adrc_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                       mdrcAttr_V30_t *manual) {
    drc_attrib_t drc_attr;

    memset(&drc_attr, 0, sizeof(drc_attrib_t));
    rk_aiq_user_api2_adrc_GetAttrib(sys_ctx, &drc_attr);
    memcpy(manual, &drc_attr.stManualV30, sizeof(mdrcAttr_V30_t));

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn rk_aiq_set_amerge_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
        mMergeAttrV30_t *manual) {
    amerge_attrib_t setdata;

    memset(&setdata, 0, sizeof(amerge_attrib_t));
    rk_aiq_user_api2_amerge_GetAttrib(sys_ctx, &setdata);
    memcpy(&setdata.attrV30.stManual, manual, sizeof(mMergeAttrV30_t));

    return rk_aiq_user_api2_amerge_SetAttrib(sys_ctx, setdata);
}

XCamReturn rk_aiq_get_amerge_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
        mMergeAttrV30_t *manual) {
    amerge_attrib_t setdata;

    memset(&setdata, 0, sizeof(amerge_attrib_t));
    rk_aiq_user_api2_amerge_GetAttrib(sys_ctx, &setdata);
    memcpy(manual, &setdata.attrV30.stManual, sizeof(mMergeAttrV30_t));

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn rk_aiq_set_agamma_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
        Agamma_api_manualV30_t *manual) {
    rk_aiq_gamma_attrib_V2_t agamma_attr;

    memset(&agamma_attr, 0, sizeof(rk_aiq_gamma_attrib_V2_t));
    rk_aiq_user_api2_agamma_GetAttrib(sys_ctx, &agamma_attr);
    memcpy(&agamma_attr.atrrV30.stManual, manual, sizeof(Agamma_api_manualV30_t));

    return rk_aiq_user_api2_agamma_SetAttrib(sys_ctx, agamma_attr);
}

XCamReturn rk_aiq_get_agamma_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
        Agamma_api_manualV30_t *manual) {
    rk_aiq_gamma_attrib_V2_t agamma_attr;

    memset(&agamma_attr, 0, sizeof(rk_aiq_gamma_attrib_V2_t));
    rk_aiq_user_api2_agamma_GetAttrib(sys_ctx, &agamma_attr);
    memcpy(manual, &agamma_attr.atrrV30.stManual, sizeof(Agamma_api_manualV30_t));

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn rk_aiq_set_tool_ccm_mode(rk_aiq_sys_ctx_t *ctx, uapi_wb_mode_t *mode) {
    return rk_aiq_uapi2_setCCMMode(ctx, mode->mode);
}

XCamReturn rk_aiq_get_accm_mode(rk_aiq_sys_ctx_t *ctx, uapi_wb_mode_t *mode) {
    return rk_aiq_uapi2_getCCMMode(ctx, &mode->mode);
}
XCamReturn rk_aiq_set_accm_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                       rk_aiq_ccm_mccm_attrib_t *manual) {
    rk_aiq_ccm_attrib_t accm_attr;

    memset(&accm_attr, 0, sizeof(rk_aiq_ccm_attrib_t));
    rk_aiq_user_api2_accm_GetAttrib(sys_ctx, &accm_attr);
    memcpy(&accm_attr.stManual, manual, sizeof(rk_aiq_ccm_mccm_attrib_t));

    return rk_aiq_user_api2_accm_SetAttrib(sys_ctx, accm_attr);
}

XCamReturn rk_aiq_get_accm_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                       rk_aiq_ccm_mccm_attrib_t *manual) {
    rk_aiq_ccm_attrib_t accm_attr;

    memset(&accm_attr, 0, sizeof(rk_aiq_ccm_attrib_t));
    rk_aiq_user_api2_accm_GetAttrib(sys_ctx, &accm_attr);
    memcpy(manual, &accm_attr.stManual, sizeof(rk_aiq_ccm_mccm_attrib_t));

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn rk_aiq_set_tool_3dlut_mode(rk_aiq_sys_ctx_t *ctx, uapi_wb_mode_t *mode) {
    return rk_aiq_uapi2_setLut3dMode(ctx, mode->mode);
}

XCamReturn rk_aiq_get_a3dlut_mode(rk_aiq_sys_ctx_t *ctx, uapi_wb_mode_t *mode) {
    return rk_aiq_uapi2_getLut3dMode(ctx, &mode->mode);
}

XCamReturn rk_aiq_set_a3dlut_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
        rk_aiq_lut3d_mlut3d_attrib_t *manual) {
    rk_aiq_lut3d_attrib_t a3dlut_attr;

    memset(&a3dlut_attr, 0, sizeof(rk_aiq_lut3d_attrib_t));
    rk_aiq_user_api2_a3dlut_GetAttrib(sys_ctx, &a3dlut_attr);
    memcpy(&a3dlut_attr.stManual, manual, sizeof(rk_aiq_lut3d_mlut3d_attrib_t));

    return rk_aiq_user_api2_a3dlut_SetAttrib(sys_ctx, a3dlut_attr);
}

XCamReturn rk_aiq_get_a3dlut_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
        rk_aiq_lut3d_mlut3d_attrib_t *manual) {
    rk_aiq_lut3d_attrib_t a3dlut_attr;

    memset(&a3dlut_attr, 0, sizeof(rk_aiq_lut3d_attrib_t));
    rk_aiq_user_api2_a3dlut_GetAttrib(sys_ctx, &a3dlut_attr);
    memcpy(manual, &a3dlut_attr.stManual, sizeof(rk_aiq_lut3d_mlut3d_attrib_t));

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn rk_aiq_set_acsm_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                       Csm_Param_t *manual) {
    rk_aiq_uapi_acsm_attrib_t acsm_attr;

    memset(&acsm_attr, 0, sizeof(rk_aiq_uapi_acsm_attrib_t));
    rk_aiq_user_api2_acsm_GetAttrib(sys_ctx, &acsm_attr);
    memcpy(&acsm_attr.param, manual, sizeof(Csm_Param_t));

    return rk_aiq_user_api2_acsm_SetAttrib(sys_ctx, acsm_attr);
}

XCamReturn rk_aiq_get_acsm_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                       Csm_Param_t *manual) {
    rk_aiq_uapi_acsm_attrib_t acsm_attr;

    memset(&acsm_attr, 0, sizeof(rk_aiq_uapi_acsm_attrib_t));
    rk_aiq_user_api2_acsm_GetAttrib(sys_ctx, &acsm_attr);
    memcpy(manual, &acsm_attr.param, sizeof(Csm_Param_t));

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn rk_aiq_set_adehaze_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
        mDehazeAttr_t *manual) {
    adehaze_sw_V2_t adehaze_attr;

    memset(&adehaze_attr, 0, sizeof(adehaze_sw_V2_t));
    rk_aiq_user_api2_adehaze_getSwAttrib(sys_ctx, &adehaze_attr);
    memcpy(&adehaze_attr.stManual, manual, sizeof(mDehazeAttr_t));

    return rk_aiq_user_api2_adehaze_setSwAttrib(sys_ctx, adehaze_attr);
}

XCamReturn rk_aiq_get_adehaze_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
        mDehazeAttr_t *manual) {
    adehaze_sw_V2_t adehaze_attr;

    memset(&adehaze_attr, 0, sizeof(adehaze_sw_V2_t));
    rk_aiq_user_api2_adehaze_getSwAttrib(sys_ctx, &adehaze_attr);
    memcpy(manual, &adehaze_attr.stManual, sizeof(mDehazeAttr_t));

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn rk_aiq_set_adebayer_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                    adebayer_attrib_t *adebayer_attr) {
    adebayer_attrib_t attr;

    memset(&attr, 0, sizeof(attr));
    rk_aiq_user_api2_adebayer_GetAttrib(sys_ctx, &attr);
    memcpy(&attr, adebayer_attr, sizeof(attr));

    return rk_aiq_user_api2_adebayer_SetAttrib(sys_ctx, attr);
}

XCamReturn rk_aiq_get_adebayer_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                    adebayer_attrib_t *adebayer_attr) {
    adebayer_attrib_t attr;

    memset(&attr, 0, sizeof(attr));
    rk_aiq_user_api2_adebayer_GetAttrib(sys_ctx, &attr);
    memcpy(adebayer_attr, &attr, sizeof(attr));

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn rk_aiq_set_alsc_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                       rk_aiq_lsc_table_t *manual) {
    rk_aiq_lsc_attrib_t alsc_attr;

    memset(&alsc_attr, 0, sizeof(rk_aiq_lsc_attrib_t));
    rk_aiq_user_api2_alsc_GetAttrib(sys_ctx, &alsc_attr);
    memcpy(&alsc_attr.stManual, manual, sizeof(rk_aiq_lsc_table_t));

    return rk_aiq_user_api2_alsc_SetAttrib(sys_ctx, alsc_attr);
}

XCamReturn rk_aiq_get_alsc_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                       rk_aiq_lsc_table_t *manual) {
    rk_aiq_lsc_attrib_t alsc_attr;

    memset(&alsc_attr, 0, sizeof(rk_aiq_lsc_attrib_t));
    rk_aiq_user_api2_alsc_GetAttrib(sys_ctx, &alsc_attr);
    memcpy(manual, &alsc_attr.stManual, sizeof(rk_aiq_lsc_table_t));

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn rk_aiq_set_current_camindex(const rk_aiq_sys_ctx_t *sys_ctx,
                                       camgroup_uapi_t *arg) {
    (void)(sys_ctx);
    last_camindex = *arg;

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn rk_aiq_get_current_camindex(const rk_aiq_sys_ctx_t *sys_ctx,
                                       camgroup_uapi_t *arg) {
    (void)(sys_ctx);

    *arg = last_camindex;

    return XCAM_RETURN_NO_ERROR;
}



XCamReturn rk_aiq_set_asharp_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
        asharp_uapi_manual_t *manual) {
    XCamReturn res = XCAM_RETURN_NO_ERROR;

#if ISP_HW_V30
    rk_aiq_sharp_attrib_v4_t sharp_attr_v4;
    memset(&sharp_attr_v4, 0, sizeof(rk_aiq_sharp_attrib_v4_t));
    rk_aiq_user_api2_asharpV4_GetAttrib(sys_ctx, &sharp_attr_v4);
    memcpy(&sharp_attr_v4.stManual.stSelect, &manual->manual_v4, sizeof(RK_SHARP_Params_V4_Select_t));

    if (manual->AsharpOpMode == RK_AIQ_OP_MODE_AUTO)
        sharp_attr_v4.eMode = ASHARP4_OP_MODE_AUTO;
    else if(manual->AsharpOpMode == RK_AIQ_OP_MODE_MANUAL)
        sharp_attr_v4.eMode = ASHARP4_OP_MODE_MANUAL;
    else if(manual->AsharpOpMode == RK_AIQ_OP_MODE_INVALID)
        sharp_attr_v4.eMode = ASHARP4_OP_MODE_INVALID;
    else if(manual->AsharpOpMode == RK_AIQ_OP_MODE_MAX)
        sharp_attr_v4.eMode = ASHARP4_OP_MODE_MAX;
    else
        sharp_attr_v4.eMode = ASHARP4_OP_MODE_AUTO;

    res = rk_aiq_user_api2_asharpV4_SetAttrib(sys_ctx, &sharp_attr_v4);
#endif

    return res;
}

XCamReturn rk_aiq_get_asharp_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
        asharp_uapi_manual_t *manual) {

#if ISP_HW_V30
    rk_aiq_sharp_attrib_v4_t sharp_attr_v4;
    memset(&sharp_attr_v4, 0, sizeof(rk_aiq_sharp_attrib_v4_t));
    rk_aiq_user_api2_asharpV4_GetAttrib(sys_ctx, &sharp_attr_v4);
    memcpy(&manual->manual_v4, &sharp_attr_v4.stManual.stSelect, sizeof(RK_SHARP_Params_V4_Select_t));

    if (sharp_attr_v4.eMode == ASHARP4_OP_MODE_AUTO)
        manual->AsharpOpMode = RK_AIQ_OP_MODE_AUTO;
    else if(sharp_attr_v4.eMode == ASHARP4_OP_MODE_MANUAL)
        manual->AsharpOpMode = RK_AIQ_OP_MODE_MANUAL;
    else if(sharp_attr_v4.eMode == ASHARP4_OP_MODE_INVALID )
        manual->AsharpOpMode = RK_AIQ_OP_MODE_INVALID;
    else if(sharp_attr_v4.eMode == ASHARP4_OP_MODE_MAX)
        manual->AsharpOpMode = RK_AIQ_OP_MODE_MAX;
    else
        manual->AsharpOpMode = RK_AIQ_OP_MODE_AUTO;

#endif
    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_set_abayer2dnr_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                  abayer2dnr_uapi_manual_t *manual) {
    XCamReturn res = XCAM_RETURN_NO_ERROR;

#if ISP_HW_V30
    rk_aiq_bayer2dnr_attrib_v2_t abayer2dnr_attr_v2;

    memset(&abayer2dnr_attr_v2, 0, sizeof(rk_aiq_bayer2dnr_attrib_v2_t));
    rk_aiq_user_api2_abayer2dnrV2_GetAttrib(sys_ctx, &abayer2dnr_attr_v2);
    memcpy(&abayer2dnr_attr_v2.stManual.st2DSelect, &manual->manual_v2,
           sizeof(RK_Bayer2dnr_Params_V2_Select_t));

    if (manual->Abayer2dnrOpMode == RK_AIQ_OP_MODE_AUTO)
        abayer2dnr_attr_v2.eMode = ABAYER2DNR_OP_MODE_AUTO;
    else if(manual->Abayer2dnrOpMode == RK_AIQ_OP_MODE_MANUAL)
        abayer2dnr_attr_v2.eMode = ABAYER2DNR_OP_MODE_MANUAL;
    else if(manual->Abayer2dnrOpMode == RK_AIQ_OP_MODE_INVALID)
        abayer2dnr_attr_v2.eMode = ABAYER2DNR_OP_MODE_INVALID;
    else if(manual->Abayer2dnrOpMode == RK_AIQ_OP_MODE_MAX)
        abayer2dnr_attr_v2.eMode = ABAYER2DNR_OP_MODE_MAX;
    else
        abayer2dnr_attr_v2.eMode = ABAYER2DNR_OP_MODE_AUTO;


    res = rk_aiq_user_api2_abayer2dnrV2_SetAttrib(sys_ctx, &abayer2dnr_attr_v2);
#endif

    return res;
}



XCamReturn
rk_aiq_get_abayer2dnr_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                  abayer2dnr_uapi_manual_t *manual) {
#if ISP_HW_V30

    rk_aiq_bayer2dnr_attrib_v2_t abayer2dnr_attr_v2;
    memset(&abayer2dnr_attr_v2, 0, sizeof(rk_aiq_bayer2dnr_attrib_v2_t));
    rk_aiq_user_api2_abayer2dnrV2_GetAttrib(sys_ctx, &abayer2dnr_attr_v2);
    memcpy(&manual->manual_v2, &abayer2dnr_attr_v2.stManual.st2DSelect,
           sizeof(RK_Bayer2dnr_Params_V2_Select_t));

    if (abayer2dnr_attr_v2.eMode == ABAYER2DNR_OP_MODE_AUTO)
        manual->Abayer2dnrOpMode = RK_AIQ_OP_MODE_AUTO;
    else if(abayer2dnr_attr_v2.eMode == ABAYER2DNR_OP_MODE_MANUAL)
        manual->Abayer2dnrOpMode = RK_AIQ_OP_MODE_MANUAL;
    else if(abayer2dnr_attr_v2.eMode == ABAYER2DNR_OP_MODE_INVALID )
        manual->Abayer2dnrOpMode = RK_AIQ_OP_MODE_INVALID;
    else if(abayer2dnr_attr_v2.eMode == ABAYER2DNR_OP_MODE_MAX)
        manual->Abayer2dnrOpMode = RK_AIQ_OP_MODE_MAX;
    else
        manual->Abayer2dnrOpMode = RK_AIQ_OP_MODE_AUTO;

#endif
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_set_abayertnr_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                 abayertnr_uapi_manual_t *manual) {
    XCamReturn res = XCAM_RETURN_NO_ERROR;

#if ISP_HW_V30
    rk_aiq_bayertnr_attrib_v2_t abayertnr_attr_v2;
    memset(&abayertnr_attr_v2, 0, sizeof(abayertnr_attr_v2));
    rk_aiq_user_api2_abayertnrV2_GetAttrib(sys_ctx, &abayertnr_attr_v2);
    memcpy(&abayertnr_attr_v2.stManual.st3DSelect, &manual->manual_v2,
           sizeof(manual->manual_v2));

    if (manual->AbayertnrOpMode == RK_AIQ_OP_MODE_AUTO)
        abayertnr_attr_v2.eMode = ABAYERTNRV2_OP_MODE_AUTO;
    else if(manual->AbayertnrOpMode == RK_AIQ_OP_MODE_MANUAL)
        abayertnr_attr_v2.eMode = ABAYERTNRV2_OP_MODE_MANUAL;
    else if(manual->AbayertnrOpMode == RK_AIQ_OP_MODE_INVALID)
        abayertnr_attr_v2.eMode = ABAYERTNRV2_OP_MODE_INVALID;
    else if(manual->AbayertnrOpMode == RK_AIQ_OP_MODE_MAX)
        abayertnr_attr_v2.eMode = ABAYERTNRV2_OP_MODE_MAX;
    else
        abayertnr_attr_v2.eMode = ABAYERTNRV2_OP_MODE_AUTO;

    res = rk_aiq_user_api2_abayertnrV2_SetAttrib(sys_ctx, &abayertnr_attr_v2);
#endif

    return res;
}

XCamReturn
rk_aiq_get_abayertnr_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                 abayertnr_uapi_manual_t *manual) {
#if ISP_HW_V30
    rk_aiq_bayertnr_attrib_v2_t abayertnr_attr_v2;
    memset(&abayertnr_attr_v2, 0, sizeof(abayertnr_attr_v2));
    rk_aiq_user_api2_abayertnrV2_GetAttrib(sys_ctx, &abayertnr_attr_v2);
    memcpy(&manual->manual_v2, &abayertnr_attr_v2.stManual.st3DSelect,
           sizeof(manual->manual_v2));

    if (abayertnr_attr_v2.eMode == ABAYERTNRV2_OP_MODE_AUTO)
        manual->AbayertnrOpMode = RK_AIQ_OP_MODE_AUTO;
    else if(abayertnr_attr_v2.eMode == ABAYERTNRV2_OP_MODE_MANUAL)
        manual->AbayertnrOpMode = RK_AIQ_OP_MODE_MANUAL;
    else if(abayertnr_attr_v2.eMode == ABAYERTNRV2_OP_MODE_INVALID )
        manual->AbayertnrOpMode = RK_AIQ_OP_MODE_INVALID;
    else if(abayertnr_attr_v2.eMode == ABAYERTNRV2_OP_MODE_MAX)
        manual->AbayertnrOpMode = RK_AIQ_OP_MODE_MAX;
    else
        manual->AbayertnrOpMode = RK_AIQ_OP_MODE_AUTO;
#endif
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn rk_aiq_set_aynr_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                       aynr_uapi_manual_t *manual) {
    XCamReturn res = XCAM_RETURN_NO_ERROR;

#if ISP_HW_V30

    rk_aiq_ynr_attrib_v3_t aynr_attr_v3;
    memset(&aynr_attr_v3, 0, sizeof(aynr_attr_v3));
    rk_aiq_user_api2_aynrV3_GetAttrib(sys_ctx, &aynr_attr_v3);
    memcpy(&aynr_attr_v3.stManual.stSelect, &manual->manual_v3, sizeof(manual->manual_v3));

    if (manual->AynrOpMode == RK_AIQ_OP_MODE_AUTO)
        aynr_attr_v3.eMode = AYNRV3_OP_MODE_AUTO;
    else if(manual->AynrOpMode == RK_AIQ_OP_MODE_MANUAL)
        aynr_attr_v3.eMode = AYNRV3_OP_MODE_MANUAL;
    else if(manual->AynrOpMode == RK_AIQ_OP_MODE_INVALID)
        aynr_attr_v3.eMode = AYNRV3_OP_MODE_INVALID;
    else if(manual->AynrOpMode == RK_AIQ_OP_MODE_MAX)
        aynr_attr_v3.eMode = AYNRV3_OP_MODE_MAX;

    res = rk_aiq_user_api2_aynrV3_SetAttrib(sys_ctx, &aynr_attr_v3);
#endif

    return res;
}

XCamReturn rk_aiq_get_aynr_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                       aynr_uapi_manual_t *manual) {
#if ISP_HW_V30
    rk_aiq_ynr_attrib_v3_t aynr_attr_v3;

    memset(&aynr_attr_v3, 0, sizeof(aynr_attr_v3));
    rk_aiq_user_api2_aynrV3_GetAttrib(sys_ctx, &aynr_attr_v3);
    memcpy(&manual->manual_v3, &aynr_attr_v3.stManual.stSelect, sizeof(manual->manual_v3));

    if (aynr_attr_v3.eMode == AYNRV3_OP_MODE_AUTO)
        manual->AynrOpMode = RK_AIQ_OP_MODE_AUTO;
    else if(aynr_attr_v3.eMode == AYNRV3_OP_MODE_MANUAL)
        manual->AynrOpMode = RK_AIQ_OP_MODE_MANUAL;
    else if(aynr_attr_v3.eMode == AYNRV3_OP_MODE_INVALID )
        manual->AynrOpMode = RK_AIQ_OP_MODE_INVALID;
    else if(aynr_attr_v3.eMode == AYNRV3_OP_MODE_MAX)
        manual->AynrOpMode = RK_AIQ_OP_MODE_MAX;
    else
        manual->AynrOpMode = RK_AIQ_OP_MODE_AUTO;
#endif

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn rk_aiq_set_acnr_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                       acnr_uapi_manual_t *manual) {
    XCamReturn res = XCAM_RETURN_NO_ERROR;

#if ISP_HW_V30
    rk_aiq_cnr_attrib_v2_t acnr_attr_v2;

    memset(&acnr_attr_v2, 0, sizeof(acnr_attr_v2));
    rk_aiq_user_api2_acnrV2_GetAttrib(sys_ctx, &acnr_attr_v2);
    memcpy(&acnr_attr_v2.stManual.stSelect, &manual->manual_v2, sizeof(manual->manual_v2));

    if (manual->AcnrOpMode == RK_AIQ_OP_MODE_AUTO)
        acnr_attr_v2.eMode = ACNRV2_OP_MODE_AUTO;
    else if(manual->AcnrOpMode == RK_AIQ_OP_MODE_MANUAL)
        acnr_attr_v2.eMode = ACNRV2_OP_MODE_MANUAL;
    else if(manual->AcnrOpMode == RK_AIQ_OP_MODE_INVALID)
        acnr_attr_v2.eMode = ACNRV2_OP_MODE_INVALID;
    else if(manual->AcnrOpMode == RK_AIQ_OP_MODE_MAX)
        acnr_attr_v2.eMode = ACNRV2_OP_MODE_MAX;
    else
        acnr_attr_v2.eMode = ACNRV2_OP_MODE_AUTO;

    res = rk_aiq_user_api2_acnrV2_SetAttrib(sys_ctx, &acnr_attr_v2);
#endif

    return res;
}
XCamReturn rk_aiq_get_acnr_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                       acnr_uapi_manual_t *manual) {
#if ISP_HW_V30
    rk_aiq_cnr_attrib_v2_t acnr_attr_v2;
    memset(&acnr_attr_v2, 0, sizeof(acnr_attr_v2));
    rk_aiq_user_api2_acnrV2_GetAttrib(sys_ctx, &acnr_attr_v2);
    memcpy(&manual->manual_v2, &acnr_attr_v2.stManual.stSelect, sizeof(manual->manual_v2));

    if (acnr_attr_v2.eMode == ACNRV2_OP_MODE_AUTO)
        manual->AcnrOpMode = RK_AIQ_OP_MODE_AUTO;
    else if(acnr_attr_v2.eMode == ACNRV2_OP_MODE_MANUAL)
        manual->AcnrOpMode = RK_AIQ_OP_MODE_MANUAL;
    else if(acnr_attr_v2.eMode == ACNRV2_OP_MODE_INVALID )
        manual->AcnrOpMode = RK_AIQ_OP_MODE_INVALID;
    else if(acnr_attr_v2.eMode == ACNRV2_OP_MODE_MAX)
        manual->AcnrOpMode = RK_AIQ_OP_MODE_MAX;
    else
        manual->AcnrOpMode = RK_AIQ_OP_MODE_AUTO;
#endif

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn rk_aiq_set_again_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                        again_uapi_manual_t *manual) {
    XCamReturn res = XCAM_RETURN_NO_ERROR;

#if ISP_HW_V30
    rk_aiq_gain_attrib_v2_t again_attr_v2;
    memset(&again_attr_v2, 0, sizeof(rk_aiq_gain_attrib_v2_t));
    rk_aiq_user_api2_againV2_GetAttrib(sys_ctx, &again_attr_v2);
    memcpy(&again_attr_v2.stManual.stSelect, &manual->manual_v2, sizeof(RK_GAIN_Select_V2_t));

    if (manual->AgainOpMode == RK_AIQ_OP_MODE_AUTO)
        again_attr_v2.eMode = AGAINV2_OP_MODE_AUTO;
    else if(manual->AgainOpMode == RK_AIQ_OP_MODE_MANUAL)
        again_attr_v2.eMode = AGAINV2_OP_MODE_MANUAL;
    else if(manual->AgainOpMode == RK_AIQ_OP_MODE_INVALID)
        again_attr_v2.eMode = AGAINV2_OP_MODE_INVALID;
    else if(manual->AgainOpMode == RK_AIQ_OP_MODE_MAX)
        again_attr_v2.eMode = AGAINV2_OP_MODE_MAX;
    else
        again_attr_v2.eMode = AGAINV2_OP_MODE_AUTO;

    res = rk_aiq_user_api2_againV2_SetAttrib(sys_ctx, &again_attr_v2);
#endif

    return res;
}

XCamReturn rk_aiq_get_again_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                        again_uapi_manual_t *manual) {
#if ISP_HW_V30
    rk_aiq_gain_attrib_v2_t again_attr_v2;

    memset(&again_attr_v2, 0, sizeof(rk_aiq_gain_attrib_v2_t));
    rk_aiq_user_api2_againV2_GetAttrib(sys_ctx, &again_attr_v2);
    memcpy(&manual->manual_v2, &again_attr_v2.stManual.stSelect, sizeof(RK_GAIN_Select_V2_t));

    if (again_attr_v2.eMode == AGAINV2_OP_MODE_AUTO)
        manual->AgainOpMode = RK_AIQ_OP_MODE_AUTO;
    else if(again_attr_v2.eMode == AGAINV2_OP_MODE_MANUAL)
        manual->AgainOpMode = RK_AIQ_OP_MODE_MANUAL;
    else if(again_attr_v2.eMode == AGAINV2_OP_MODE_INVALID )
        manual->AgainOpMode = RK_AIQ_OP_MODE_INVALID;
    else if(again_attr_v2.eMode == AGAINV2_OP_MODE_MAX)
        manual->AgainOpMode = RK_AIQ_OP_MODE_MAX;
    else
        manual->AgainOpMode = RK_AIQ_OP_MODE_AUTO;
#endif
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn rk_aiq_set_ablc_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                       ablc_uapi_manual_t *manual) {
    XCamReturn res = XCAM_RETURN_NO_ERROR;

#if ISP_HW_V30
    rk_aiq_blc_attrib_t ablc_attr;
    memset(&ablc_attr, 0, sizeof(ablc_attr));
    rk_aiq_user_api2_ablc_GetAttrib(sys_ctx, &ablc_attr);
    memcpy(&ablc_attr.stBlc0Manual, &manual->bls0, sizeof(manual->bls0));
    memcpy(&ablc_attr.stBlc1Manual, &manual->bls1, sizeof(manual->bls1));

    if (manual->AblcOpMode == RK_AIQ_OP_MODE_AUTO)
        ablc_attr.eMode = ABLC_OP_MODE_AUTO;
    else if(manual->AblcOpMode == RK_AIQ_OP_MODE_MANUAL)
        ablc_attr.eMode = ABLC_OP_MODE_MANUAL;
    else if(manual->AblcOpMode == RK_AIQ_OP_MODE_INVALID)
        ablc_attr.eMode = ABLC_OP_MODE_OFF;
    else if(manual->AblcOpMode == RK_AIQ_OP_MODE_MAX)
        ablc_attr.eMode = ABLC_OP_MODE_MAX;
    else
        ablc_attr.eMode = ABLC_OP_MODE_AUTO;

    res = rk_aiq_user_api2_ablc_SetAttrib(sys_ctx, &ablc_attr);
#endif

    return res;
}

XCamReturn rk_aiq_get_ablc_manual_attr(const rk_aiq_sys_ctx_t *sys_ctx,
                                       ablc_uapi_manual_t *manual) {
#if ISP_HW_V30
    rk_aiq_blc_attrib_t ablc_attr;

    memset(&ablc_attr, 0, sizeof(ablc_attr));
    rk_aiq_user_api2_ablc_GetAttrib(sys_ctx, &ablc_attr);
    memcpy(&manual->bls0, &ablc_attr.stBlc0Manual, sizeof(manual->bls0));
    memcpy(&manual->bls1, &ablc_attr.stBlc1Manual, sizeof(manual->bls1));

    if (ablc_attr.eMode == ABLC_OP_MODE_AUTO)
        manual->AblcOpMode = RK_AIQ_OP_MODE_AUTO;
    else if(ablc_attr.eMode == ABLC_OP_MODE_MANUAL)
        manual->AblcOpMode = RK_AIQ_OP_MODE_MANUAL;
    else if(ablc_attr.eMode == ABLC_OP_MODE_OFF )
        manual->AblcOpMode = RK_AIQ_OP_MODE_INVALID;
    else if(ablc_attr.eMode == ABLC_OP_MODE_MAX)
        manual->AblcOpMode = RK_AIQ_OP_MODE_MAX;
    else
        manual->AblcOpMode = RK_AIQ_OP_MODE_AUTO;
#endif
    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_get_abayertnr_info(const rk_aiq_sys_ctx_t *sys_ctx,
                          abayertnr_uapi_info_t  *info) {

#if ISP_HW_V30
    rk_aiq_bayertnr_info_v2_t abayertnr_info_v2;
    rk_aiq_user_api2_abayertnrV2_GetInfo(sys_ctx, &abayertnr_info_v2);
    info->iso = abayertnr_info_v2.iso;
    info->expo_info = abayertnr_info_v2.expo_info;
#endif
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_get_abayer2dnr_info(const rk_aiq_sys_ctx_t *sys_ctx,
                           abayer2dnr_uapi_info_t *info) {
#if ISP_HW_V30
    rk_aiq_bayer2dnr_info_v2_t bayer2dnr_info_v2;
    rk_aiq_user_api2_abayer2dnrV2_GetInfo(sys_ctx, &bayer2dnr_info_v2);
    info->iso = bayer2dnr_info_v2.iso;
    info->expo_info = bayer2dnr_info_v2.expo_info;
#endif
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_get_aynr_info(const rk_aiq_sys_ctx_t *sys_ctx,
                     aynr_uapi_info_t  *info) {
#if ISP_HW_V30
    rk_aiq_ynr_info_v3_t ynr_info_v3;
    rk_aiq_user_api2_aynrV3_GetInfo(sys_ctx, &ynr_info_v3);
    info->iso = ynr_info_v3.iso;
    info->expo_info = ynr_info_v3.expo_info;
#endif
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_get_acnr_info(const rk_aiq_sys_ctx_t *sys_ctx,
                     acnr_uapi_info_t  *info) {
#if ISP_HW_V30
    rk_aiq_cnr_info_v2_t cnr_info_v2;
    rk_aiq_user_api2_acnrV2_GetInfo(sys_ctx, &cnr_info_v2);
    info->iso = cnr_info_v2.iso;
    info->expo_info = cnr_info_v2.expo_info;
#endif

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_get_asharp_info(const rk_aiq_sys_ctx_t *sys_ctx,
                       asharp_uapi_info_t *info) {

#if ISP_HW_V30
    rk_aiq_sharp_info_v4_t sharp_info_v4;
    rk_aiq_user_api2_asharpV4_GetInfo(sys_ctx, &sharp_info_v4);
    info->iso = sharp_info_v4.iso;
    info->expo_info = sharp_info_v4.expo_info;
#endif

    return XCAM_RETURN_NO_ERROR;

}


XCamReturn
rk_aiq_get_again_info(const rk_aiq_sys_ctx_t *sys_ctx,
                      again_uapi_info_t *info) {

#if ISP_HW_V30
    rk_aiq_gain_info_v2_t gain_info_v2;
    rk_aiq_user_api2_againV2_GetInfo(sys_ctx, &gain_info_v2);
    info->iso = gain_info_v2.iso;
    info->expo_info = gain_info_v2.expo_info;
#endif

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_get_ablc_info(const rk_aiq_sys_ctx_t *sys_ctx,
                     ablc_uapi_info_t *info) {

#if ISP_HW_V30
    rk_aiq_ablc_info_t blc_info;
    rk_aiq_user_api2_ablc_GetInfo(sys_ctx, &blc_info);
    info->iso = blc_info.iso;
    info->expo_info = blc_info.expo_info;
#endif

    return XCAM_RETURN_NO_ERROR;
}


rk_aiq_sys_ctx_t* rk_aiq_get_last_sysctx(rk_aiq_sys_ctx_t *sys_ctx) {
    camgroup_uapi_t last_arg;
    rk_aiq_camgroup_ctx_t* grp_ctx = NULL;

    // Single camera instance
    grp_ctx = get_binded_group_ctx(sys_ctx);
    if (!grp_ctx) {
        return sys_ctx->next_ctx ? sys_ctx->next_ctx : sys_ctx;
    }

    // Group camera instance
    // Check if an special index of camera required
    memset(&last_arg, 0, sizeof(camgroup_uapi_t));
    rk_aiq_get_current_camindex(sys_ctx, &last_arg);
    if (last_arg.current_index == 0) {
        return (rk_aiq_sys_ctx_t*)grp_ctx;
    }

    for (auto cam_ctx : grp_ctx->cam_ctxs_array) {
        if (!cam_ctx || cam_ctx->_camPhyId != (last_arg.current_index - 1))
            continue;
        return cam_ctx;
    }

    return NULL;
}
