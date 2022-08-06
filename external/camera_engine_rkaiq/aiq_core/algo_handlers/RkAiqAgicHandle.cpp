/*
 * Copyright (c) 2019-2021 Rockchip Eletronics Co., Ltd.
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
 */
#include "RkAiqCore.h"
#include "RkAiqHandle.h"
#include "RkAiqHandleInt.h"

namespace RkCam {

DEFINE_HANDLE_REGISTER_TYPE(RkAiqAgicHandleInt);

void RkAiqAgicHandleInt::init() {
    ENTER_ANALYZER_FUNCTION();

    RkAiqHandle::deInit();
    mConfig       = (RkAiqAlgoCom*)(new RkAiqAlgoConfigAgic());
    mPreInParam   = (RkAiqAlgoCom*)(new RkAiqAlgoPreAgic());
    mPreOutParam  = (RkAiqAlgoResCom*)(new RkAiqAlgoPreResAgic());
    mProcInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoProcAgic());
    mProcOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoProcResAgic());
    mPostInParam  = (RkAiqAlgoCom*)(new RkAiqAlgoPostAgic());
    mPostOutParam = (RkAiqAlgoResCom*)(new RkAiqAlgoPostResAgic());

    EXIT_ANALYZER_FUNCTION();
}

XCamReturn RkAiqAgicHandleInt::updateConfig(bool needSync) {
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (needSync) mCfgMutex.lock();
    // if something changed
    if (updateAtt) {
        mCurAtt   = mNewAtt;
        updateAtt = false;
        rk_aiq_uapi_agic_SetAttrib(mAlgoCtx, mCurAtt, false);
        sendSignal();
    }
    if (needSync) mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn RkAiqAgicHandleInt::setAttrib(agic_attrib_t att) {
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    mCfgMutex.lock();
    // TODO
    // check if there is different between att & mCurAtt
    // if something changed, set att to mNewAtt, and
    // the new params will be effective later when updateConfig
    // called by RkAiqCore
    if (0 != memcmp(&mCurAtt, &att, sizeof(agic_attrib_t))) {
        mNewAtt   = att;
        updateAtt = true;
        waitSignal();
    }

    mCfgMutex.unlock();

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn RkAiqAgicHandleInt::getAttrib(agic_attrib_t* att) {
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    rk_aiq_uapi_agic_GetAttrib(mAlgoCtx, att);

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn RkAiqAgicHandleInt::prepare() {
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    ret = RkAiqHandle::prepare();
    RKAIQCORE_CHECK_RET(ret, "agic handle prepare failed");

    RkAiqAlgoConfigAgic* agic_config_int = (RkAiqAlgoConfigAgic*)mConfig;
    RkAiqCore::RkAiqAlgosGroupShared_t* shared =
        (RkAiqCore::RkAiqAlgosGroupShared_t*)(getGroupShared());

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret                       = des->prepare(mConfig);
    RKAIQCORE_CHECK_RET(ret, "agic algo prepare failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn RkAiqAgicHandleInt::preProcess() {
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPreAgic* agic_pre_int        = (RkAiqAlgoPreAgic*)mPreInParam;
    RkAiqAlgoPreResAgic* agic_pre_res_int = (RkAiqAlgoPreResAgic*)mPreOutParam;
    RkAiqCore::RkAiqAlgosGroupShared_t* shared =
        (RkAiqCore::RkAiqAlgosGroupShared_t*)(getGroupShared());
    RkAiqCore::RkAiqAlgosComShared_t* sharedCom = &mAiqCore->mAlogsComSharedParams;

    ret = RkAiqHandle::preProcess();
    if (ret) {
        RKAIQCORE_CHECK_RET(ret, "agic handle preProcess failed");
    }

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret                       = des->pre_process(mPreInParam, mPreOutParam);
    RKAIQCORE_CHECK_RET(ret, "agic algo pre_process failed");

    EXIT_ANALYZER_FUNCTION();
    return XCAM_RETURN_NO_ERROR;
}

XCamReturn RkAiqAgicHandleInt::processing() {
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoProcAgic* agic_proc_int        = (RkAiqAlgoProcAgic*)mProcInParam;
    RkAiqAlgoProcResAgic* agic_proc_res_int = (RkAiqAlgoProcResAgic*)mProcOutParam;
    RkAiqCore::RkAiqAlgosGroupShared_t* shared =
        (RkAiqCore::RkAiqAlgosGroupShared_t*)(getGroupShared());
    RkAiqCore::RkAiqAlgosComShared_t* sharedCom = &mAiqCore->mAlogsComSharedParams;

    ret = RkAiqHandle::processing();
    if (ret) {
        RKAIQCORE_CHECK_RET(ret, "agic handle processing failed");
    }

    agic_proc_int->hdr_mode = sharedCom->working_mode;
    switch (sharedCom->snsDes.sensor_pixelformat) {
        case V4L2_PIX_FMT_SBGGR14:
        case V4L2_PIX_FMT_SGBRG14:
        case V4L2_PIX_FMT_SGRBG14:
        case V4L2_PIX_FMT_SRGGB14:
            agic_proc_int->raw_bits = 14;
            break;
        case V4L2_PIX_FMT_SBGGR12:
        case V4L2_PIX_FMT_SGBRG12:
        case V4L2_PIX_FMT_SGRBG12:
        case V4L2_PIX_FMT_SRGGB12:
            agic_proc_int->raw_bits = 12;
            break;
        case V4L2_PIX_FMT_SBGGR10:
        case V4L2_PIX_FMT_SGBRG10:
        case V4L2_PIX_FMT_SGRBG10:
        case V4L2_PIX_FMT_SRGGB10:
            agic_proc_int->raw_bits = 10;
            break;
        default:
            agic_proc_int->raw_bits = 8;
    }

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret                       = des->processing(mProcInParam, mProcOutParam);
    RKAIQCORE_CHECK_RET(ret, "agic algo processing failed");

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn RkAiqAgicHandleInt::postProcess() {
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;

    RkAiqAlgoPostAgic* agic_post_int        = (RkAiqAlgoPostAgic*)mPostInParam;
    RkAiqAlgoPostResAgic* agic_post_res_int = (RkAiqAlgoPostResAgic*)mPostOutParam;
    RkAiqCore::RkAiqAlgosGroupShared_t* shared =
        (RkAiqCore::RkAiqAlgosGroupShared_t*)(getGroupShared());
    RkAiqCore::RkAiqAlgosComShared_t* sharedCom = &mAiqCore->mAlogsComSharedParams;

    ret = RkAiqHandle::postProcess();
    if (ret) {
        RKAIQCORE_CHECK_RET(ret, "agic handle postProcess failed");
        return ret;
    }

    RkAiqAlgoDescription* des = (RkAiqAlgoDescription*)mDes;
    ret                       = des->post_process(mPostInParam, mPostOutParam);
    RKAIQCORE_CHECK_RET(ret, "agic algo post_process failed");

    EXIT_ANALYZER_FUNCTION();
    return ret;
}

XCamReturn RkAiqAgicHandleInt::genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params) {
    ENTER_ANALYZER_FUNCTION();

    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    RkAiqCore::RkAiqAlgosGroupShared_t* shared =
        (RkAiqCore::RkAiqAlgosGroupShared_t*)(getGroupShared());
    RkAiqCore::RkAiqAlgosComShared_t* sharedCom = &mAiqCore->mAlogsComSharedParams;
    RkAiqAlgoProcResAgic* agic_com              = (RkAiqAlgoProcResAgic*)mProcOutParam;
    rk_aiq_isp_gic_params_v20_t* gic_param = params->mGicParams->data().ptr();

    if (!agic_com) {
        LOGD_ANALYZER("no agic result");
        return XCAM_RETURN_NO_ERROR;
    }

    if (!this->getAlgoId()) {
        RkAiqAlgoProcResAgic* agic_rk = (RkAiqAlgoProcResAgic*)agic_com;
        if (sharedCom->init) {
            gic_param->frame_id = 0;
        } else {
            gic_param->frame_id = shared->frameId;
        }
        memcpy(&gic_param->result, &agic_rk->gicRes, sizeof(AgicProcResult_t));
    }

    cur_params->mGicParams = params->mGicParams;

    EXIT_ANALYZER_FUNCTION();

    return ret;
}

};  // namespace RkCam
