#include "rk_aiq_uapi_adebayer_int.h"
#include "rk_aiq_algo_adebayer.h"

XCamReturn
rk_aiq_uapi_adebayer_SetAttrib
(
    RkAiqAlgoContext* ctx,
    adebayer_attrib_t attr,
    bool need_sync
)
{
    if(ctx == NULL) {
        LOGE("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_PARAM;
    }

    AdebayerContext_t* pAdebayerCtx = (AdebayerContext_t*)&ctx->adebayerCtx;
    pAdebayerCtx->full_param.enable = attr.enable;
    pAdebayerCtx->mode = attr.mode;
    if (attr.mode == RK_AIQ_DEBAYER_MODE_AUTO) {

        for(int i = 0; i < 5; i++) {
            pAdebayerCtx->full_param.filter1[i] = attr.stAuto.debayer_filter1[i];
            pAdebayerCtx->full_param.filter2[i] = attr.stAuto.debayer_filter2[i];
        }

        for(int i = 0; i < DEBAYER_ISO_LEN; i++) {
            pAdebayerCtx->full_param.iso[i] = attr.stAuto.ISO[i];
            pAdebayerCtx->full_param.gain_offset[i] = attr.stAuto.debayer_gain_offset[i];
            pAdebayerCtx->full_param.offset[i] = attr.stAuto.debayer_offset[i];
            pAdebayerCtx->full_param.hf_offset[i] = attr.stAuto.debayer_hf_offset[i];
            pAdebayerCtx->full_param.clip_en[i] = attr.stAuto.debayer_clip_en[i];
            pAdebayerCtx->full_param.filter_c_en[i] = attr.stAuto.debayer_filter_g_en[i];
            pAdebayerCtx->full_param.filter_g_en[i] = attr.stAuto.debayer_filter_c_en[i];

            pAdebayerCtx->full_param.thed0[i] = attr.stAuto.debayer_thed0[i];
            pAdebayerCtx->full_param.thed1[i] = attr.stAuto.debayer_thed1[i];
            pAdebayerCtx->full_param.sharp_strength[i] = attr.stAuto.sharp_strength[i];
            pAdebayerCtx->full_param.cnr_strength[i] = attr.stAuto.debayer_cnr_strength[i];
            pAdebayerCtx->full_param.shift_num[i] = attr.stAuto.debayer_shift_num[i];
            pAdebayerCtx->full_param.dist_scale[i] = attr.stAuto.debayer_dist_scale[i];
        }
    } else if (attr.mode == RK_AIQ_DEBAYER_MODE_MANUAL) {
        memcpy(&pAdebayerCtx->manualAttrib, &attr.stManual, sizeof(attr.stManual));
    } else {
        LOGE("Invalid mode: %s\n", attr.mode == RK_AIQ_DEBAYER_MODE_AUTO ? "auto" : "manual");
        return XCAM_RETURN_ERROR_PARAM;
    }

    pAdebayerCtx->full_param.updated = true;

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_uapi_adebayer_GetAttrib
(
    RkAiqAlgoContext*  ctx,
    adebayer_attrib_t* attr
)
{
    if(ctx == NULL || attr == NULL) {
        LOGE("%s(%d): null pointer\n", __FUNCTION__, __LINE__);
        return XCAM_RETURN_ERROR_PARAM;
    }

    AdebayerContext_t* pAdebayerCtx = (AdebayerContext_t*)&ctx->adebayerCtx;
    attr->enable = pAdebayerCtx->full_param.enable;
    attr->mode = pAdebayerCtx->mode;

    for(int i = 0; i < 5; i++) {
        attr->stAuto.debayer_filter1[i] = pAdebayerCtx->full_param.filter1[i];
        attr->stAuto.debayer_filter2[i] = pAdebayerCtx->full_param.filter2[i];
    }

    for(int i = 0; i < DEBAYER_ISO_LEN; i++) {
        attr->stAuto.ISO[i] = pAdebayerCtx->full_param.iso[i];
        attr->stAuto.debayer_gain_offset[i] = pAdebayerCtx->full_param.gain_offset[i];
        attr->stAuto.debayer_offset[i] = pAdebayerCtx->full_param.offset[i];
        attr->stAuto.debayer_hf_offset[i] = pAdebayerCtx->full_param.hf_offset[i];
        attr->stAuto.debayer_clip_en[i] = pAdebayerCtx->full_param.clip_en[i];

        attr->stAuto.debayer_filter_g_en[i] = pAdebayerCtx->full_param.filter_c_en[i];
        attr->stAuto.debayer_filter_c_en[i] = pAdebayerCtx->full_param.filter_g_en[i];

        attr->stAuto.debayer_thed0[i] = pAdebayerCtx->full_param.thed0[i];
        attr->stAuto.debayer_thed1[i] = pAdebayerCtx->full_param.thed1[i];

        attr->stAuto.sharp_strength[i] = pAdebayerCtx->full_param.sharp_strength[i];
        attr->stAuto.debayer_cnr_strength[i] = pAdebayerCtx->full_param.cnr_strength[i];
        attr->stAuto.debayer_shift_num[i] = pAdebayerCtx->full_param.shift_num[i];
        attr->stAuto.debayer_dist_scale[i] = pAdebayerCtx->full_param.dist_scale[i];
    }

    memcpy(&attr->stManual, &pAdebayerCtx->manualAttrib, sizeof(pAdebayerCtx->manualAttrib));

    return XCAM_RETURN_NO_ERROR;
}

