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
#ifndef _RK_AIQ_HANDLE_INT_H_
#define _RK_AIQ_HANDLE_INT_H_

#include "RkAiqHandle.h"
#include "ae/rk_aiq_uapi_ae_int.h"
#include "awb/rk_aiq_uapi_awb_int.h"
#include "awb/rk_aiq_uapiv2_awb_int.h"
#include "adebayer/rk_aiq_uapi_adebayer_int.h"
#include "amerge/rk_aiq_uapi_amerge_int.h"
#include "atmo/rk_aiq_uapi_atmo_int.h"
#include "adrc/rk_aiq_uapi_adrc_int.h"
#include "alsc/rk_aiq_uapi_alsc_int.h"
#include "accm/rk_aiq_uapi_accm_int.h"
#include "a3dlut/rk_aiq_uapi_a3dlut_int.h"
#include "xcam_mutex.h"
#include "adehaze/rk_aiq_uapi_adehaze_int.h"
#include "agamma/rk_aiq_uapi_agamma_int.h"
#include "adegamma/rk_aiq_uapi_adegamma_int.h"
#include "ablc/rk_aiq_uapi_ablc_int.h"
#include "adpcc/rk_aiq_uapi_adpcc_int.h"
#include "anr/rk_aiq_uapi_anr_int.h"
#include "asharp/rk_aiq_uapi_asharp_int.h"
#include "agic/rk_aiq_uapi_agic_int.h"
#include "afec/rk_aiq_uapi_afec_int.h"
#include "af/rk_aiq_uapi_af_int.h"
#include "asd/rk_aiq_uapi_asd_int.h"
#include "aldch/rk_aiq_uapi_aldch_int.h"
#include "acp/rk_aiq_uapi_acp_int.h"
#include "aie/rk_aiq_uapi_aie_int.h"
#include "aeis/rk_aiq_uapi_aeis_int.h"
#include "arawnr/rk_aiq_uapi_abayernr_int_v1.h"
#include "aynr/rk_aiq_uapi_aynr_int_v1.h"
#include "auvnr/rk_aiq_uapi_auvnr_int_v1.h"
#include "amfnr/rk_aiq_uapi_amfnr_int_v1.h"
#include "again/rk_aiq_uapi_again_int.h"
#include "acac/rk_aiq_uapi_acac_int.h"
#include "acsm/rk_aiq_uapi_acsm.h"

#include "rk_aiq_pool.h"
#include "rk_aiq_api_private.h"

namespace RkCam {

// ae
class RkAiqCustomAeHandle;
class RkAiqAeHandleInt:
    public RkAiqHandle {
    friend class RkAiqCustomAeHandle;
public:
    explicit RkAiqAeHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , mPreResShared(nullptr)
        , mProcResShared(nullptr) {

        updateExpSwAttrV2 = false;
        updateLinExpAttrV2 = false;
        updateHdrExpAttrV2 = false;

        updateLinAeRouteAttr = false;
        updateHdrAeRouteAttr = false;
        updateIrisAttr = false;
        updateSyncTestAttr = false;
        updateExpWinAttr = false;

        memset(&mCurExpSwAttrV2, 0, sizeof(Uapi_ExpSwAttrV2_t));
        memset(&mNewExpSwAttrV2, 0, sizeof(Uapi_ExpSwAttrV2_t));
        memset(&mCurLinExpAttrV2, 0, sizeof(Uapi_LinExpAttrV2_t));
        memset(&mNewLinExpAttrV2, 0, sizeof(Uapi_LinExpAttrV2_t));
        memset(&mCurHdrExpAttrV2, 0, sizeof(Uapi_HdrExpAttrV2_t));
        memset(&mNewHdrExpAttrV2, 0, sizeof(Uapi_HdrExpAttrV2_t));

        memset(&mCurLinAeRouteAttr, 0, sizeof(Uapi_LinAeRouteAttr_t));
        memset(&mNewLinAeRouteAttr, 0, sizeof(Uapi_LinAeRouteAttr_t));
        memset(&mCurHdrAeRouteAttr, 0, sizeof(Uapi_HdrAeRouteAttr_t));
        memset(&mNewHdrAeRouteAttr, 0, sizeof(Uapi_HdrAeRouteAttr_t));
        memset(&mCurIrisAttr, 0, sizeof(Uapi_IrisAttrV2_t));
        memset(&mNewIrisAttr, 0, sizeof(Uapi_IrisAttrV2_t));
        memset(&mCurAecSyncTestAttr, 0, sizeof(Uapi_AecSyncTest_t));
        memset(&mNewAecSyncTestAttr, 0, sizeof(Uapi_AecSyncTest_t));
        memset(&mCurExpWinAttr, 0, sizeof(Uapi_ExpWin_t));
        memset(&mNewExpWinAttr, 0, sizeof(Uapi_ExpWin_t));

    };
    virtual ~RkAiqAeHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();

    // TODO: calibv1
    virtual XCamReturn setExpSwAttr(Uapi_ExpSwAttr_t ExpSwAttr);
    virtual XCamReturn getExpSwAttr(Uapi_ExpSwAttr_t* pExpSwAttr);
    virtual XCamReturn setLinExpAttr(Uapi_LinExpAttr_t LinExpAttr);
    virtual XCamReturn getLinExpAttr(Uapi_LinExpAttr_t* pLinExpAttr);
    virtual XCamReturn setHdrExpAttr(Uapi_HdrExpAttr_t HdrExpAttr);
    virtual XCamReturn getHdrExpAttr (Uapi_HdrExpAttr_t* pHdrExpAttr);

    // TODO: calibv2
    virtual XCamReturn setExpSwAttr(Uapi_ExpSwAttrV2_t ExpSwAttr);
    virtual XCamReturn getExpSwAttr(Uapi_ExpSwAttrV2_t* pExpSwAttr);
    virtual XCamReturn setLinExpAttr(Uapi_LinExpAttrV2_t LinExpAttr);
    virtual XCamReturn getLinExpAttr(Uapi_LinExpAttrV2_t* pLinExpAttr);
    virtual XCamReturn setHdrExpAttr(Uapi_HdrExpAttrV2_t HdrExpAttr);
    virtual XCamReturn getHdrExpAttr (Uapi_HdrExpAttrV2_t* pHdrExpAttr);

    virtual XCamReturn setLinAeRouteAttr(Uapi_LinAeRouteAttr_t LinAeRouteAttr);
    virtual XCamReturn getLinAeRouteAttr(Uapi_LinAeRouteAttr_t* pLinAeRouteAttr);
    virtual XCamReturn setHdrAeRouteAttr(Uapi_HdrAeRouteAttr_t HdrAeRouteAttr);
    virtual XCamReturn getHdrAeRouteAttr(Uapi_HdrAeRouteAttr_t* pHdrAeRouteAttr);

    virtual XCamReturn setIrisAttr(Uapi_IrisAttrV2_t IrisAttr);
    virtual XCamReturn getIrisAttr (Uapi_IrisAttrV2_t* pIrisAttr);
    virtual XCamReturn setSyncTestAttr(Uapi_AecSyncTest_t SyncTestAttr);
    virtual XCamReturn getSyncTestAttr (Uapi_AecSyncTest_t* pSyncTestAttr);
    virtual XCamReturn queryExpInfo(Uapi_ExpQueryInfo_t* pExpQueryInfo);
    virtual XCamReturn setLockAeForAf(bool lock_ae);
    virtual XCamReturn setExpWinAttr(Uapi_ExpWin_t ExpWinAttr);
    virtual XCamReturn getExpWinAttr(Uapi_ExpWin_t* pExpWinAttr);
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
    SmartPtr<RkAiqAlgoPreResAeIntShared> mPreResShared;
    SmartPtr<RkAiqAlgoProcResAeIntShared> mProcResShared;
private:

    // TODO: calibv1
    Uapi_ExpSwAttr_t  mCurExpSwAttr;
    Uapi_ExpSwAttr_t  mNewExpSwAttr;
    Uapi_LinExpAttr_t mCurLinExpAttr;
    Uapi_LinExpAttr_t mNewLinExpAttr;
    Uapi_HdrExpAttr_t mCurHdrExpAttr;
    Uapi_HdrExpAttr_t mNewHdrExpAttr;

    // TODO: calibv2
    Uapi_ExpSwAttrV2_t  mCurExpSwAttrV2;
    Uapi_ExpSwAttrV2_t  mNewExpSwAttrV2;
    Uapi_LinExpAttrV2_t mCurLinExpAttrV2;
    Uapi_LinExpAttrV2_t mNewLinExpAttrV2;
    Uapi_HdrExpAttrV2_t mCurHdrExpAttrV2;
    Uapi_HdrExpAttrV2_t mNewHdrExpAttrV2;

    Uapi_LinAeRouteAttr_t mCurLinAeRouteAttr;
    Uapi_LinAeRouteAttr_t mNewLinAeRouteAttr;
    Uapi_HdrAeRouteAttr_t mCurHdrAeRouteAttr;
    Uapi_HdrAeRouteAttr_t mNewHdrAeRouteAttr;
    Uapi_IrisAttrV2_t     mCurIrisAttr;
    Uapi_IrisAttrV2_t     mNewIrisAttr;
    Uapi_AecSyncTest_t    mCurAecSyncTestAttr;
    Uapi_AecSyncTest_t    mNewAecSyncTestAttr;
    Uapi_ExpWin_t         mCurExpWinAttr;
    Uapi_ExpWin_t         mNewExpWinAttr;

    bool updateExpSwAttr = false;
    bool updateLinExpAttr = false;
    bool updateHdrExpAttr = false;

    mutable std::atomic<bool> updateExpSwAttrV2;
    mutable std::atomic<bool> updateLinExpAttrV2;
    mutable std::atomic<bool> updateHdrExpAttrV2;

    mutable std::atomic<bool> updateLinAeRouteAttr;
    mutable std::atomic<bool> updateHdrAeRouteAttr;
    mutable std::atomic<bool> updateIrisAttr;
    mutable std::atomic<bool> updateSyncTestAttr;
    mutable std::atomic<bool> updateExpWinAttr;

    uint16_t updateAttr = 0;

    XCam::Mutex mLockAebyAfMutex;
    bool lockaebyaf = false;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAeHandleInt);
};

// awb
class RkAiqAwbHandleInt:
    public RkAiqHandle {
    friend class RkAiqAwbV21HandleInt;
public:
    explicit RkAiqAwbHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , mProcResShared(nullptr) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_wb_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_wb_attrib_t));
        memset(&mCurWbV20Attr, 0, sizeof(mCurWbV20Attr));
        memset(&mCurWbOpModeAttr, 0, sizeof(mCurWbOpModeAttr));
        mCurWbOpModeAttr.mode = RK_AIQ_WB_MODE_MAX;
        memset(&mCurWbMwbAttr, 0, sizeof(mCurWbMwbAttr));
        memset(&mCurWbAwbAttr, 0, sizeof(mCurWbAwbAttr));
        memset(&mCurWbAwbWbGainAdjustAttr, 0, sizeof(mCurWbAwbWbGainAdjustAttr));
        memset(&mCurWbAwbWbGainOffsetAttr, 0, sizeof(mCurWbAwbWbGainOffsetAttr));
        memset(&mCurWbAwbMultiWindowAttr, 0, sizeof(mCurWbAwbMultiWindowAttr));
        memset(&mNewWbV20Attr, 0, sizeof(mNewWbV20Attr));
        memset(&mNewWbOpModeAttr, 0, sizeof(mNewWbOpModeAttr));
        mNewWbOpModeAttr.mode = RK_AIQ_WB_MODE_MAX;
        memset(&mNewWbMwbAttr, 0, sizeof(mNewWbMwbAttr));
        memset(&mNewWbAwbAttr, 0, sizeof(mNewWbAwbAttr));
        memset(&mNewWbAwbWbGainAdjustAttr, 0, sizeof(mNewWbAwbWbGainAdjustAttr));
        memset(&mNewWbAwbWbGainOffsetAttr, 0, sizeof(mNewWbAwbWbGainOffsetAttr));
        memset(&mNewWbAwbMultiWindowAttr, 0, sizeof(mNewWbAwbMultiWindowAttr));
        updateWbV20Attr = false;
        updateWbOpModeAttr = false;
        updateWbMwbAttr = false;
        updateWbAwbAttr = false;
        updateWbAwbWbGainAdjustAttr = false;
        updateWbAwbWbGainOffsetAttr = false;
        updateWbAwbMultiWindowAttr = false;
    };
    virtual ~RkAiqAwbHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_wb_attrib_t att);
    XCamReturn getAttrib(rk_aiq_wb_attrib_t *att);
    XCamReturn getCct(rk_aiq_wb_cct_t *cct);
    XCamReturn queryWBInfo(rk_aiq_wb_querry_info_t *wb_querry_info );
    XCamReturn lock();
    XCamReturn unlock();
    XCamReturn setWbV20Attrib(rk_aiq_uapiV2_wbV20_attrib_t att);
    XCamReturn getWbV20Attrib(rk_aiq_uapiV2_wbV20_attrib_t *att);
    XCamReturn setWbOpModeAttrib(rk_aiq_uapiV2_wb_opMode_t att);
    XCamReturn getWbOpModeAttrib(rk_aiq_uapiV2_wb_opMode_t *att);
    XCamReturn setMwbAttrib(rk_aiq_wb_mwb_attrib_t att);
    XCamReturn getMwbAttrib(rk_aiq_wb_mwb_attrib_t *att);
    XCamReturn setAwbV20Attrib(rk_aiq_uapiV2_wbV20_awb_attrib_t att);
    XCamReturn getAwbV20Attrib(rk_aiq_uapiV2_wbV20_awb_attrib_t *att);
    XCamReturn setWbAwbWbGainAdjustAttrib(rk_aiq_uapiV2_wb_awb_wbGainAdjust_t att);
    XCamReturn getWbAwbWbGainAdjustAttrib(rk_aiq_uapiV2_wb_awb_wbGainAdjust_t *att);
    XCamReturn setWbAwbWbGainOffsetAttrib(rk_aiq_uapiV2_wb_awb_wbGainOffset_t att);
    XCamReturn getWbAwbWbGainOffsetAttrib(rk_aiq_uapiV2_wb_awb_wbGainOffset_t *att);
    XCamReturn setWbAwbMultiWindowAttrib(rk_aiq_uapiV2_wb_awb_mulWindow_t att);
    XCamReturn getWbAwbMultiWindowAttrib(rk_aiq_uapiV2_wb_awb_mulWindow_t *att);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
    SmartPtr<RkAiqAlgoProcResAwbIntShared> mProcResShared;
private:
    // TODO
    rk_aiq_wb_attrib_t mCurAtt;
    rk_aiq_wb_attrib_t mNewAtt;
    //v2
    rk_aiq_uapiV2_wbV20_attrib_t mCurWbV20Attr;//v21 todo
    rk_aiq_uapiV2_wbV20_attrib_t mNewWbV20Attr;
    rk_aiq_uapiV2_wb_opMode_t  mCurWbOpModeAttr;
    rk_aiq_uapiV2_wb_opMode_t  mNewWbOpModeAttr;
    rk_aiq_wb_mwb_attrib_t  mCurWbMwbAttr;
    rk_aiq_wb_mwb_attrib_t  mNewWbMwbAttr;
    rk_aiq_uapiV2_wbV20_awb_attrib_t  mCurWbAwbAttr;
    rk_aiq_uapiV2_wbV20_awb_attrib_t  mNewWbAwbAttr;
    rk_aiq_uapiV2_wb_awb_wbGainAdjust_t mCurWbAwbWbGainAdjustAttr;
    rk_aiq_uapiV2_wb_awb_wbGainAdjust_t mNewWbAwbWbGainAdjustAttr;
    rk_aiq_uapiV2_wb_awb_wbGainOffset_t mCurWbAwbWbGainOffsetAttr;
    rk_aiq_uapiV2_wb_awb_wbGainOffset_t mNewWbAwbWbGainOffsetAttr;
    rk_aiq_uapiV2_wb_awb_mulWindow_t mCurWbAwbMultiWindowAttr;
    rk_aiq_uapiV2_wb_awb_mulWindow_t mNewWbAwbMultiWindowAttr;
    mutable std::atomic<bool> updateWbV20Attr;
    mutable std::atomic<bool> updateWbOpModeAttr;
    mutable std::atomic<bool> updateWbMwbAttr;
    mutable std::atomic<bool> updateWbAwbAttr;
    mutable std::atomic<bool> updateWbAwbWbGainAdjustAttr;
    mutable std::atomic<bool> updateWbAwbWbGainOffsetAttr;
    mutable std::atomic<bool> updateWbAwbMultiWindowAttr;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAwbHandleInt);
};

// af
class RkAiqAfHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAfHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , mProcResShared(nullptr) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_af_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_af_attrib_t));
        isUpdateAttDone = false;
        isUpdateZoomPosDone = false;
    };
    virtual ~RkAiqAfHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_af_attrib_t *att);
    XCamReturn getAttrib(rk_aiq_af_attrib_t *att);
    XCamReturn lock();
    XCamReturn unlock();
    XCamReturn Oneshot();
    XCamReturn ManualTriger();
    XCamReturn Tracking();
    XCamReturn setZoomIndex(int index);
    XCamReturn getZoomIndex(int *index);
    XCamReturn endZoomChg();
    XCamReturn startZoomCalib();
    XCamReturn resetZoom();
    XCamReturn GetSearchPath(rk_aiq_af_sec_path_t* path);
    XCamReturn GetSearchResult(rk_aiq_af_result_t* result);
    XCamReturn GetFocusRange(rk_aiq_af_focusrange* range);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    bool getValueFromFile(const char* path, int *pos);

    // TODO
    rk_aiq_af_attrib_t mCurAtt;
    rk_aiq_af_attrib_t mNewAtt;
    mutable std::atomic<bool> isUpdateAttDone;
    mutable std::atomic<bool> isUpdateZoomPosDone;
    int mLastZoomIndex;

    SmartPtr<RkAiqAlgoProcResAfIntShared> mProcResShared;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAfHandleInt);
};

class RkAiqAdebayerHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAdebayerHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {}
    virtual ~RkAiqAdebayerHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(adebayer_attrib_t att);
    XCamReturn getAttrib(adebayer_attrib_t *att);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    adebayer_attrib_t mCurAtt;
    adebayer_attrib_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAdebayerHandleInt);
};

// amerge
class RkAiqAmergeHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAmergeHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {}
    virtual ~RkAiqAmergeHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    XCamReturn setAttrib(amerge_attrib_t att);
    XCamReturn getAttrib(amerge_attrib_t* att);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    amerge_attrib_t mCurAtt;
    amerge_attrib_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAmergeHandleInt);
};

// atmo
class RkAiqAtmoHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAtmoHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {}
    virtual ~RkAiqAtmoHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    XCamReturn setAttrib(atmo_attrib_t att);
    XCamReturn getAttrib(atmo_attrib_t* att);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    atmo_attrib_t mCurAtt;
    atmo_attrib_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAtmoHandleInt);
};

class RkAiqAgicHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAgicHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {}
    virtual ~RkAiqAgicHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    XCamReturn setAttrib(agic_attrib_t att);
    XCamReturn getAttrib(agic_attrib_t *att);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    agic_attrib_t mCurAtt;
    agic_attrib_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAgicHandleInt);
};

// adehaze
class RkAiqAdhazHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAdhazHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {}
    virtual ~RkAiqAdhazHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setSwAttrib(adehaze_sw_V2_t att);
    XCamReturn getSwAttrib(adehaze_sw_V2_t *att);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    // TODO
    adehaze_sw_V2_t mCurAtt;
    adehaze_sw_V2_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAdhazHandleInt);
};

// agamma
class RkAiqAgammaHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAgammaHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_gamma_attrib_V2_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_gamma_attrib_V2_t));
    };
    virtual ~RkAiqAgammaHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_gamma_attrib_V2_t att);
    XCamReturn getAttrib(rk_aiq_gamma_attrib_V2_t *att);
    //XCamReturn queryLscInfo(rk_aiq_lsc_querry_info_t *lsc_querry_info );

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    // TODO
    rk_aiq_gamma_attrib_V2_t mCurAtt;
    rk_aiq_gamma_attrib_V2_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAgammaHandleInt);
};

// adegamma
class RkAiqAdegammaHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAdegammaHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_degamma_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_degamma_attrib_t));
    };
    virtual ~RkAiqAdegammaHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_degamma_attrib_t att);
    XCamReturn getAttrib(rk_aiq_degamma_attrib_t *att);
    //XCamReturn queryLscInfo(rk_aiq_lsc_querry_info_t *lsc_querry_info );

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    // TODO
    rk_aiq_degamma_attrib_t mCurAtt;
    rk_aiq_degamma_attrib_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAdegammaHandleInt);
};

// alsc
class RkAiqAlscHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAlscHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_lsc_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_lsc_attrib_t));
    };
    virtual ~RkAiqAlscHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_lsc_attrib_t att);
    XCamReturn getAttrib(rk_aiq_lsc_attrib_t *att);
    XCamReturn queryLscInfo(rk_aiq_lsc_querry_info_t *lsc_querry_info );

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    // TODO
    rk_aiq_lsc_attrib_t mCurAtt;
    rk_aiq_lsc_attrib_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAlscHandleInt);
};

// accm
class RkAiqAccmHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAccmHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_ccm_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_ccm_attrib_t));
    };
    virtual ~RkAiqAccmHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_ccm_attrib_t att);
    XCamReturn getAttrib(rk_aiq_ccm_attrib_t *att);
    XCamReturn queryCcmInfo(rk_aiq_ccm_querry_info_t *ccm_querry_info );

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    // TODO
    rk_aiq_ccm_attrib_t mCurAtt;
    rk_aiq_ccm_attrib_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAccmHandleInt);
};

// a3dlut
class RkAiqA3dlutHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqA3dlutHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_lut3d_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_lut3d_attrib_t));
    };
    virtual ~RkAiqA3dlutHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_lut3d_attrib_t att);
    XCamReturn getAttrib(rk_aiq_lut3d_attrib_t *att);
    XCamReturn query3dlutInfo(rk_aiq_lut3d_querry_info_t *lut3d_querry_info );

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    // TODO
    rk_aiq_lut3d_attrib_t mCurAtt;
    rk_aiq_lut3d_attrib_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqA3dlutHandleInt);
};

class RkAiqAblcHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAblcHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_blc_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_blc_attrib_t));
        updateAtt = false;
    };
    virtual ~RkAiqAblcHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_blc_attrib_t *att);
    XCamReturn getAttrib(rk_aiq_blc_attrib_t *att);
    XCamReturn getProcRes(AblcProc_t *ProcRes);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    // TODO
    rk_aiq_blc_attrib_t mCurAtt;
    rk_aiq_blc_attrib_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAblcHandleInt);
};

// adpcc
class RkAiqAdpccHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAdpccHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_dpcc_attrib_V20_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_dpcc_attrib_V20_t));
    };
    virtual ~RkAiqAdpccHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_dpcc_attrib_V20_t *att);
    XCamReturn getAttrib(rk_aiq_dpcc_attrib_V20_t *att);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    // TODO
    rk_aiq_dpcc_attrib_V20_t mCurAtt;
    rk_aiq_dpcc_attrib_V20_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAdpccHandleInt);
};

// anr
class RkAiqAnrHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAnrHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_nr_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_nr_attrib_t));
    };
    virtual ~RkAiqAnrHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_nr_attrib_t *att);
    XCamReturn getAttrib(rk_aiq_nr_attrib_t *att);
    XCamReturn setLumaSFStrength(float fPercent);
    XCamReturn setLumaTFStrength(float fPercent);
    XCamReturn getLumaSFStrength(float *pPercent);
    XCamReturn getLumaTFStrength(float *pPercent);
    XCamReturn setChromaSFStrength(float fPercent);
    XCamReturn setChromaTFStrength(float fPercent);
    XCamReturn getChromaSFStrength(float *pPercent);
    XCamReturn getChromaTFStrength(float *pPercent);
    XCamReturn setRawnrSFStrength(float fPercent);
    XCamReturn getRawnrSFStrength(float *pPercent);
    XCamReturn setIQPara(rk_aiq_nr_IQPara_t *pPara);
    XCamReturn getIQPara(rk_aiq_nr_IQPara_t *pPara);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    // TODO
    rk_aiq_nr_attrib_t mCurAtt;
    rk_aiq_nr_attrib_t mNewAtt;
    rk_aiq_nr_IQPara_t mCurIQpara;
    rk_aiq_nr_IQPara_t mNewIQpara;
    bool UpdateIQpara = false;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAnrHandleInt);
};


// anr
class RkAiqAsharpHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAsharpHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_sharp_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_sharp_attrib_t));
    };
    virtual ~RkAiqAsharpHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_sharp_attrib_t *att);
    XCamReturn getAttrib(rk_aiq_sharp_attrib_t *att);
    XCamReturn setStrength(float fPercent);
    XCamReturn getStrength(float *pPercent);
    XCamReturn setIQPara(rk_aiq_sharp_IQpara_t *para);
    XCamReturn getIQPara(rk_aiq_sharp_IQpara_t *para);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    // TODO
    rk_aiq_sharp_attrib_t mCurAtt;
    rk_aiq_sharp_attrib_t mNewAtt;
    rk_aiq_sharp_IQpara_t mCurIQPara;
    rk_aiq_sharp_IQpara_t mNewIQPara;
    bool updateIQpara = false;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAsharpHandleInt);
};

// afec
class RkAiqAfecHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAfecHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_fec_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_fec_attrib_t));
        mCurAtt.en = 0xff;
    };
    virtual ~RkAiqAfecHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);

    XCamReturn setAttrib(rk_aiq_fec_attrib_t att);
    XCamReturn getAttrib(rk_aiq_fec_attrib_t *att);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    rk_aiq_fec_attrib_t mCurAtt;
    rk_aiq_fec_attrib_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAfecHandleInt);
};

class RkAiqAsdHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAsdHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {}
    virtual ~RkAiqAsdHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(asd_attrib_t att);
    XCamReturn getAttrib(asd_attrib_t *att);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    asd_attrib_t mCurAtt;
    asd_attrib_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAsdHandleInt);
};

// aldch
class RkAiqAldchHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAldchHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_ldch_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_ldch_attrib_t));
    };
    virtual ~RkAiqAldchHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);

    XCamReturn setAttrib(rk_aiq_ldch_attrib_t att);
    XCamReturn getAttrib(rk_aiq_ldch_attrib_t *att);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    rk_aiq_ldch_attrib_t mCurAtt;
    rk_aiq_ldch_attrib_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAldchHandleInt);
};

class RkAiqAcpHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAcpHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {}
    virtual ~RkAiqAcpHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    XCamReturn setAttrib(acp_attrib_t att);
    XCamReturn getAttrib(acp_attrib_t *att);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    acp_attrib_t mCurAtt;
    acp_attrib_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAcpHandleInt);
};

class RkAiqAdrcHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAdrcHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {}
    virtual ~RkAiqAdrcHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    XCamReturn setAttrib(drc_attrib_t att);
    XCamReturn getAttrib(drc_attrib_t *att);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    drc_attrib_t mCurAtt;
    drc_attrib_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAdrcHandleInt);
};
class RkAiqAieHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAieHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {}
    virtual ~RkAiqAieHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    XCamReturn setAttrib(aie_attrib_t att);
    XCamReturn getAttrib(aie_attrib_t *att);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    aie_attrib_t mCurAtt;
    aie_attrib_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAieHandleInt);
};

// aeis
class RkAiqAeisHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAeisHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_eis_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_eis_attrib_t));
        mCurAtt.en = 0xff;
    };
    virtual ~RkAiqAeisHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);

    XCamReturn setAttrib(rk_aiq_eis_attrib_t att);
    XCamReturn getAttrib(rk_aiq_eis_attrib_t *att);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    rk_aiq_eis_attrib_t mCurAtt;
    rk_aiq_eis_attrib_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAeisHandleInt);
};

//amd
class RkAiqAmdHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAmdHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore)
        , mProcResShared(nullptr) {};
    virtual ~RkAiqAmdHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    SmartPtr<RkAiqAlgoProcResAmdIntShared> mProcResShared;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAmdHandleInt);
};

// aynr v1
class RkAiqArawnrHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqArawnrHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_bayernr_attrib_v1_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_bayernr_attrib_v1_t));
    };
    virtual ~RkAiqArawnrHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_bayernr_attrib_v1_t *att);
    XCamReturn getAttrib(rk_aiq_bayernr_attrib_v1_t *att);
    XCamReturn setStrength(float fPercent);
    XCamReturn getStrength(float *pPercent);
    XCamReturn setIQPara(rk_aiq_bayernr_IQPara_V1_t *pPara);
    XCamReturn getIQPara(rk_aiq_bayernr_IQPara_V1_t *pPara);
    XCamReturn setJsonPara(rk_aiq_bayernr_JsonPara_V1_t *para);
    XCamReturn getJsonPara(rk_aiq_bayernr_JsonPara_V1_t *para);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    // TODO
    rk_aiq_bayernr_attrib_v1_t mCurAtt;
    rk_aiq_bayernr_attrib_v1_t mNewAtt;
    rk_aiq_bayernr_IQPara_V1_t mCurIQPara;
    rk_aiq_bayernr_IQPara_V1_t mNewIQPara;
    rk_aiq_bayernr_JsonPara_V1_t mCurJsonPara;
    rk_aiq_bayernr_JsonPara_V1_t mNewJsonPara;
    bool updateIQpara = false;
    bool updateJsonpara = false;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqArawnrHandleInt);
};

// aynr v1
class RkAiqAynrHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAynrHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_ynr_attrib_v1_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_ynr_attrib_v1_t));
    };
    virtual ~RkAiqAynrHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_ynr_attrib_v1_t *att);
    XCamReturn getAttrib(rk_aiq_ynr_attrib_v1_t *att);
    XCamReturn setStrength(float fPercent);
    XCamReturn getStrength(float *pPercent);
    XCamReturn setIQPara(rk_aiq_ynr_IQPara_V1_t *pPara);
    XCamReturn getIQPara(rk_aiq_ynr_IQPara_V1_t *pPara);
    XCamReturn setJsonPara(rk_aiq_ynr_JsonPara_V1_t *para);
    XCamReturn getJsonPara(rk_aiq_ynr_JsonPara_V1_t *para);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    // TODO
    rk_aiq_ynr_attrib_v1_t mCurAtt;
    rk_aiq_ynr_attrib_v1_t mNewAtt;
    rk_aiq_ynr_IQPara_V1_t mCurIQPara;
    rk_aiq_ynr_IQPara_V1_t mNewIQPara;
    rk_aiq_ynr_JsonPara_V1_t mCurJsonPara;
    rk_aiq_ynr_JsonPara_V1_t mNewJsonPara;
    bool updateIQpara = false;
    bool updateJsonpara = false;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAynrHandleInt);
};

// auvnr v1
class RkAiqAcnrHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAcnrHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_uvnr_attrib_v1_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_uvnr_attrib_v1_t));
    };
    virtual ~RkAiqAcnrHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_uvnr_attrib_v1_t *att);
    XCamReturn getAttrib(rk_aiq_uvnr_attrib_v1_t *att);
    XCamReturn setStrength(float fPercent);
    XCamReturn getStrength(float *pPercent);
    XCamReturn setIQPara(rk_aiq_uvnr_IQPara_v1_t *pPara);
    XCamReturn getIQPara(rk_aiq_uvnr_IQPara_v1_t *pPara);
    XCamReturn setJsonPara(rk_aiq_uvnr_JsonPara_v1_t *para);
    XCamReturn getJsonPara(rk_aiq_uvnr_JsonPara_v1_t *para);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    // TODO
    rk_aiq_uvnr_attrib_v1_t mCurAtt;
    rk_aiq_uvnr_attrib_v1_t mNewAtt;
    rk_aiq_uvnr_IQPara_v1_t mCurIQPara;
    rk_aiq_uvnr_IQPara_v1_t mNewIQPara;
    rk_aiq_uvnr_JsonPara_v1_t mCurJsonPara;
    rk_aiq_uvnr_JsonPara_v1_t mNewJsonPara;
    bool updateIQpara = false;
    bool updateJsonpara = false;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAcnrHandleInt);
};

class RkAiqAmfnrHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAmfnrHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_mfnr_attrib_v1_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_mfnr_attrib_v1_t));
        memset(&mCurIQPara, 0, sizeof(rk_aiq_mfnr_IQPara_V1_t));
        memset(&mNewIQPara, 0, sizeof(rk_aiq_mfnr_IQPara_V1_t));
        memset(&mCurJsonPara, 0, sizeof(rk_aiq_mfnr_JsonPara_V1_t));
        memset(&mNewJsonPara, 0, sizeof(rk_aiq_mfnr_JsonPara_V1_t));
    };
    virtual ~RkAiqAmfnrHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    XCamReturn setAttrib(rk_aiq_mfnr_attrib_v1_t *att);
    XCamReturn getAttrib(rk_aiq_mfnr_attrib_v1_t *att);
    XCamReturn setLumaStrength(float fPercent);
    XCamReturn getLumaStrength(float *pPercent);
    XCamReturn setChromaStrength(float fPercent);
    XCamReturn getChromaStrength(float *pPercent);
    XCamReturn setIQPara(rk_aiq_mfnr_IQPara_V1_t *pPara);
    XCamReturn getIQPara(rk_aiq_mfnr_IQPara_V1_t *pPara);
    XCamReturn setJsonPara(rk_aiq_mfnr_JsonPara_V1_t *para);
    XCamReturn getJsonPara(rk_aiq_mfnr_JsonPara_V1_t *para);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    rk_aiq_mfnr_attrib_v1_t mCurAtt;
    rk_aiq_mfnr_attrib_v1_t mNewAtt;
    rk_aiq_mfnr_IQPara_V1_t mCurIQPara;
    rk_aiq_mfnr_IQPara_V1_t mNewIQPara;
    rk_aiq_mfnr_JsonPara_V1_t mCurJsonPara;
    rk_aiq_mfnr_JsonPara_V1_t mNewJsonPara;
    bool updateIQpara = false;
    bool updateJsonpara = false;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAmfnrHandleInt);
};


// again v1
class RkAiqAgainHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAgainHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {}
    virtual ~RkAiqAgainHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };

private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAgainHandleInt);
};

// acac
class RkAiqAcacHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAcacHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_cac_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_cac_attrib_t));
        mCurAtt.en = 0xff;
    };
    virtual ~RkAiqAcacHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);

    XCamReturn setAttrib(rk_aiq_cac_attrib_t att);
    XCamReturn getAttrib(rk_aiq_cac_attrib_t *att);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    rk_aiq_cac_attrib_t mCurAtt;
    rk_aiq_cac_attrib_t mNewAtt;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAcacHandleInt);
};

// Aorb
class RkAiqAorbHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAorbHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
    };
    virtual ~RkAiqAorbHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAorbHandleInt);
};

// Awdr
class RkAiqAwdrHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAwdrHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
    };
    virtual ~RkAiqAwdrHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAwdrHandleInt);
};

// Acgc
class RkAiqAcgcHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAcgcHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
    };
    virtual ~RkAiqAcgcHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAcgcHandleInt);
};

// Acsm
class RkAiqAcsmHandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAcsmHandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
    };
    virtual ~RkAiqAcsmHandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn updateConfig(bool needSync);
    XCamReturn setAttrib(rk_aiq_uapi_acsm_attrib_t att);
    XCamReturn getAttrib(rk_aiq_uapi_acsm_attrib_t *att);
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAcsmHandleInt);

    rk_aiq_uapi_acsm_attrib_t mCurAtt;
    rk_aiq_uapi_acsm_attrib_t mNewAtt;
};

}; //namespace RkCam

#endif
