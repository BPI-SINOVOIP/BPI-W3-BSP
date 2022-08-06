/*
 * RkAiqCamGroupHandleInt3x.h
 *
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

#ifndef _RK_AIQ_CAMGROUP_HANDLE_INT_V3_H_
#define _RK_AIQ_CAMGROUP_HANDLE_INT_V3_H_

#include "RkAiqCamgroupHandle.h"

#include "accm/rk_aiq_uapi_accm_int.h"
#include "a3dlut/rk_aiq_uapi_a3dlut_int.h"

namespace RkCam {
// accm
class RkAiqCamGroupAccmHandleInt:
    public RkAiqCamgroupHandle {
public:
    explicit RkAiqCamGroupAccmHandleInt(RkAiqAlgoDesComm* des,
            RkAiqCamGroupManager* camGroupMg)
        : RkAiqCamgroupHandle(des, camGroupMg) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_ccm_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_ccm_attrib_t));
    };
    virtual ~RkAiqCamGroupAccmHandleInt() {
         RkAiqCamgroupHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_ccm_attrib_t att);
    XCamReturn getAttrib(rk_aiq_ccm_attrib_t *att);
    XCamReturn queryCcmInfo(rk_aiq_ccm_querry_info_t* ccm_querry_info);

protected:

private:
    // TODO
    rk_aiq_ccm_attrib_t mCurAtt;
    rk_aiq_ccm_attrib_t mNewAtt;
};

// a3dlut
class RkAiqCamGroupA3dlutHandleInt:
    public RkAiqCamgroupHandle {
public:
    explicit RkAiqCamGroupA3dlutHandleInt(RkAiqAlgoDesComm* des,
            RkAiqCamGroupManager* camGroupMg)
        : RkAiqCamgroupHandle(des, camGroupMg) {
        memset(&mCurAtt, 0, sizeof(rk_aiq_lut3d_attrib_t));
        memset(&mNewAtt, 0, sizeof(rk_aiq_lut3d_attrib_t));
    };
    virtual ~RkAiqCamGroupA3dlutHandleInt() {
         RkAiqCamgroupHandle::deInit();
    };
    virtual XCamReturn updateConfig(bool needSync);
    // TODO add algo specific methords, this is a sample
    XCamReturn setAttrib(rk_aiq_lut3d_attrib_t att);
    XCamReturn getAttrib(rk_aiq_lut3d_attrib_t *att);
    XCamReturn query3dlutInfo(rk_aiq_lut3d_querry_info_t* lut3d_querry_info);

protected:

private:
    // TODO
    rk_aiq_lut3d_attrib_t mCurAtt;
    rk_aiq_lut3d_attrib_t mNewAtt;
};

};

#endif
