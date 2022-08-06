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
#ifndef _RK_AIQ_HANDLE_INT_V3_H_
#define _RK_AIQ_HANDLE_INT_V3_H_

#include "RkAiqHandleInt.h"

#include "aynr3/rk_aiq_uapi_aynr_int_v3.h"
#include "acnr2/rk_aiq_uapi_acnr_int_v2.h"
#include "asharp4/rk_aiq_uapi_asharp_int_v4.h"
#include "abayer2dnr2/rk_aiq_uapi_abayer2dnr_int_v2.h"
#include "abayertnr2/rk_aiq_uapi_abayertnr_int_v2.h"
#include "again2/rk_aiq_uapi_again_int_v2.h"

namespace RkCam {

// aynr v2
class RkAiqAynrV3HandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAynrV3HandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        updateAtt = false;
        updateStrength = false;
        memset(&mCurStrength, 0x00, sizeof(mCurStrength));
        mCurStrength.percent = 1.0;
        memset(&mNewStrength, 0x00, sizeof(mNewStrength));
        mNewStrength.percent = 1.0;
        memset(&mCurAtt, 0x00, sizeof(mCurAtt));
        memset(&mNewAtt, 0x00, sizeof(mNewAtt));
    };
    virtual ~RkAiqAynrV3HandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_ynr_attrib_v3_t *att);
    XCamReturn getAttrib(rk_aiq_ynr_attrib_v3_t *att);
    XCamReturn setStrength(rk_aiq_ynr_strength_v3_t *pStrength);
    XCamReturn getStrength(rk_aiq_ynr_strength_v3_t *pStrength);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    // TODO
    rk_aiq_ynr_attrib_v3_t mCurAtt;
    rk_aiq_ynr_attrib_v3_t mNewAtt;
    rk_aiq_ynr_strength_v3_t mCurStrength;
    rk_aiq_ynr_strength_v3_t mNewStrength;
    mutable std::atomic<bool> updateStrength;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAynrV3HandleInt);
};


// acnr v2
class RkAiqAcnrV2HandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAcnrV2HandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        updateStrength = false;
        updateAtt = false;
        memset(&mCurStrength, 0x00, sizeof(mCurStrength));
        mCurStrength.percent = 1.0;
        memset(&mNewStrength, 0x00, sizeof(mNewStrength));
        mNewStrength.percent = 1.0;
        memset(&mCurAtt, 0x00, sizeof(mCurAtt));
        memset(&mNewAtt, 0x00, sizeof(mNewAtt));
    };
    virtual ~RkAiqAcnrV2HandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_cnr_attrib_v2_t *att);
    XCamReturn getAttrib(rk_aiq_cnr_attrib_v2_t *att);
    XCamReturn setStrength(rk_aiq_cnr_strength_v2_t *pStrength);
    XCamReturn getStrength(rk_aiq_cnr_strength_v2_t *pStrength);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    // TODO
    rk_aiq_cnr_attrib_v2_t mCurAtt;
    rk_aiq_cnr_attrib_v2_t mNewAtt;
    rk_aiq_cnr_strength_v2_t mCurStrength;
    rk_aiq_cnr_strength_v2_t mNewStrength;
    mutable std::atomic<bool> updateStrength;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAcnrV2HandleInt);
};

// asharp v3
class RkAiqAsharpV4HandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAsharpV4HandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        updateStrength = false;
        updateAtt = false;
        memset(&mCurStrength, 0x00, sizeof(mCurStrength));
        memset(&mNewStrength, 0x00, sizeof(mNewStrength));
        mCurStrength.percent = 1.0;
        mNewStrength.percent = 1.0;
        memset(&mCurAtt, 0x00, sizeof(mCurAtt));
        memset(&mNewAtt, 0x00, sizeof(mNewAtt));
    };
    virtual ~RkAiqAsharpV4HandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_sharp_attrib_v4_t *att);
    XCamReturn getAttrib(rk_aiq_sharp_attrib_v4_t *att);
    XCamReturn setStrength(rk_aiq_sharp_strength_v4_t *pStrength);
    XCamReturn getStrength(rk_aiq_sharp_strength_v4_t *pStrength);

protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    // TODO
    rk_aiq_sharp_attrib_v4_t mCurAtt;
    rk_aiq_sharp_attrib_v4_t mNewAtt;
    rk_aiq_sharp_strength_v4_t mCurStrength;
    rk_aiq_sharp_strength_v4_t mNewStrength;
    mutable std::atomic<bool> updateStrength;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAsharpV4HandleInt);
};

// aynr v2
class RkAiqAbayer2dnrV2HandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAbayer2dnrV2HandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        updateStrength = false;
        updateAtt = false;
        memset(&mCurStrength, 0x00, sizeof(mCurStrength));
        memset(&mNewStrength, 0x00, sizeof(mNewStrength));
        mCurStrength.percent = 1.0;
        mNewStrength.percent = 1.0;
        memset(&mCurAtt, 0x00, sizeof(mCurAtt));
        memset(&mNewAtt, 0x00, sizeof(mNewAtt));

    };
    virtual ~RkAiqAbayer2dnrV2HandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_bayer2dnr_attrib_v2_t *att);
    XCamReturn getAttrib(rk_aiq_bayer2dnr_attrib_v2_t *att);
    XCamReturn setStrength(rk_aiq_bayer2dnr_strength_v2_t *pStrength);
    XCamReturn getStrength(rk_aiq_bayer2dnr_strength_v2_t *pStrength);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    // TODO
    rk_aiq_bayer2dnr_attrib_v2_t mCurAtt;
    rk_aiq_bayer2dnr_attrib_v2_t mNewAtt;
    rk_aiq_bayer2dnr_strength_v2_t mCurStrength;
    rk_aiq_bayer2dnr_strength_v2_t mNewStrength;
    mutable std::atomic<bool>  updateStrength;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAbayer2dnrV2HandleInt);
};

// aynr v2
class RkAiqAbayertnrV2HandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAbayertnrV2HandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {
        updateStrength = false;
        updateAtt = false;
        memset(&mCurStrength, 0x00, sizeof(mCurStrength));
        memset(&mNewStrength, 0x00, sizeof(mNewStrength));
        mCurStrength.percent = 1.0;
        mNewStrength.percent = 1.0;
        memset(&mCurAtt, 0x00, sizeof(mCurAtt));
        memset(&mNewAtt, 0x00, sizeof(mNewAtt));
    };
    virtual ~RkAiqAbayertnrV2HandleInt() {
        RkAiqHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    virtual XCamReturn prepare();
    virtual XCamReturn preProcess();
    virtual XCamReturn processing();
    virtual XCamReturn postProcess();
    virtual XCamReturn genIspResult(RkAiqFullParams* params, RkAiqFullParams* cur_params);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_bayertnr_attrib_v2_t *att);
    XCamReturn getAttrib(rk_aiq_bayertnr_attrib_v2_t *att);
    XCamReturn setStrength(rk_aiq_bayertnr_strength_v2_t *pStrength);
    XCamReturn getStrength(rk_aiq_bayertnr_strength_v2_t *pStrength);
protected:
    virtual void init();
    virtual void deInit() {
        RkAiqHandle::deInit();
    };
private:
    // TODO
    rk_aiq_bayertnr_attrib_v2_t mCurAtt;
    rk_aiq_bayertnr_attrib_v2_t mNewAtt;
    rk_aiq_bayertnr_strength_v2_t mCurStrength;
    rk_aiq_bayertnr_strength_v2_t mNewStrength;
    mutable std::atomic<bool> updateStrength;
private:
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAbayertnrV2HandleInt);
};

// again v1
class RkAiqAgainV2HandleInt:
    virtual public RkAiqHandle {
public:
    explicit RkAiqAgainV2HandleInt(RkAiqAlgoDesComm* des, RkAiqCore* aiqCore)
        : RkAiqHandle(des, aiqCore) {}
    virtual ~RkAiqAgainV2HandleInt() {
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
    DECLARE_HANDLE_REGISTER_TYPE(RkAiqAgainV2HandleInt);
};

}

#endif

