#include "rk_aiq_uapi_adehaze_int.h"
#include "rk_aiq_types_adehaze_algo_prvt.h"
#include "xcam_log.h"

XCamReturn
rk_aiq_uapi_adehaze_SetAttrib(RkAiqAlgoContext *ctx,
                              adehaze_sw_V2_t attr,
                              bool need_sync)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    AdehazeHandle_t * AdehazeHandle = (AdehazeHandle_t *)ctx;

    AdehazeHandle->AdehazeAtrr.mode = attr.mode;
    if (attr.mode == DEHAZE_API_AUTO)
        memcpy(&AdehazeHandle->AdehazeAtrr.stAuto, &attr.stAuto, sizeof(aDehazeAttr_t));
    else if (attr.mode == DEHAZE_API_MANUAL)
        memcpy(&AdehazeHandle->AdehazeAtrr.stManual, &attr.stManual, sizeof(mDehazeAttr_t));

    if (attr.stDehazeManu.update) {
        AdehazeHandle->AdehazeAtrr.stDehazeManu.level = attr.stDehazeManu.level;
        attr.stDehazeManu.update                      = false;
    }
    if (attr.stEnhanceManu.update) {
        AdehazeHandle->AdehazeAtrr.stEnhanceManu.level = attr.stEnhanceManu.level;
        attr.stEnhanceManu.update                      = false;
    }

    return ret;
}

XCamReturn
rk_aiq_uapi_adehaze_GetAttrib(RkAiqAlgoContext *ctx, adehaze_sw_V2_t *attr)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    AdehazeHandle_t * AdehazeHandle = (AdehazeHandle_t *)ctx;

    attr->mode = AdehazeHandle->AdehazeAtrr.mode;
    memcpy(&attr->stAuto, &AdehazeHandle->AdehazeAtrr.stAuto, sizeof(aDehazeAttr_t));
    memcpy(&attr->stManual, &AdehazeHandle->AdehazeAtrr.stManual, sizeof(mDehazeAttr_t));
    memcpy(&attr->stDehazeManu, &AdehazeHandle->AdehazeAtrr.stDehazeManu, sizeof(DehazeManuAttr_t));
    memcpy(&attr->stEnhanceManu, &AdehazeHandle->AdehazeAtrr.stEnhanceManu, sizeof(EnhanceManuAttr_t));

    return ret;
}

