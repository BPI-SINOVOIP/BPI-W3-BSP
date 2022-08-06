#include "acnr2/rk_aiq_uapi_acnr_int_v2.h"
#include "acnr2/rk_aiq_types_acnr_algo_prvt_v2.h"

#if 1
#define ACNRV2_CHROMA_SF_STRENGTH_MAX_PERCENT (100.0)


XCamReturn
rk_aiq_uapi_acnrV2_SetAttrib(RkAiqAlgoContext *ctx,
                             rk_aiq_cnr_attrib_v2_t *attr,
                             bool need_sync)
{

    Acnr_Context_V2_t* pCtx = (Acnr_Context_V2_t*)ctx;

    pCtx->eMode = attr->eMode;
    if(pCtx->eMode == ACNRV2_OP_MODE_AUTO) {
        pCtx->stAuto = attr->stAuto;
    } else if(pCtx->eMode == ACNRV2_OP_MODE_MANUAL) {
        pCtx->stManual.stSelect = attr->stManual.stSelect;
    } else if(pCtx->eMode == ACNRV2_OP_MODE_REG_MANUAL) {
        pCtx->stManual.stFix = attr->stManual.stFix;
    }
    pCtx->isReCalculate |= 1;

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn
rk_aiq_uapi_acnrV2_GetAttrib(const RkAiqAlgoContext *ctx,
                             rk_aiq_cnr_attrib_v2_t *attr)
{

    Acnr_Context_V2_t* pCtx = (Acnr_Context_V2_t*)ctx;

    attr->eMode = pCtx->eMode;
    memcpy(&attr->stAuto, &pCtx->stAuto, sizeof(attr->stAuto));
    memcpy(&attr->stManual, &pCtx->stManual, sizeof(attr->stManual));

    return XCAM_RETURN_NO_ERROR;
}



XCamReturn
rk_aiq_uapi_acnrV2_SetChromaSFStrength(const RkAiqAlgoContext *ctx,
                                       float fPercent)
{
    Acnr_Context_V2_t* pCtx = (Acnr_Context_V2_t*)ctx;

    float fStrength = 1.0f;
    float fMax = ACNRV2_CHROMA_SF_STRENGTH_MAX_PERCENT;

    if(fPercent <= 0.5) {
        fStrength =  fPercent / 0.5;
    } else {
        if(fPercent >= 0.999999)
            fPercent = 0.999999;
        fStrength = 0.5 / (1.0 - fPercent);
    }

    pCtx->fCnr_SF_Strength = fStrength;
    pCtx->isReCalculate |= 1;

    return XCAM_RETURN_NO_ERROR;
}


XCamReturn
rk_aiq_uapi_acnrV2_GetChromaSFStrength(const RkAiqAlgoContext *ctx,
                                       float *pPercent)
{
    Acnr_Context_V2_t* pCtx = (Acnr_Context_V2_t*)ctx;

    float fStrength = 1.0f;
    float fMax = ACNRV2_CHROMA_SF_STRENGTH_MAX_PERCENT;

    fStrength = pCtx->fCnr_SF_Strength;


    if(fStrength <= 1) {
        *pPercent = fStrength * 0.5;
    } else {
        float tmp = 1.0;
        tmp = 1 - 0.5 / fStrength;
        if(abs(tmp - 0.999999) < 0.000001) {
            tmp = 1.0;
        }
        *pPercent = tmp;
    }


    return XCAM_RETURN_NO_ERROR;
}


#endif

