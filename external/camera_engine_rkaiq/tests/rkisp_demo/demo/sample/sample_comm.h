/*
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

#ifndef _SAMPLE_COMM_H_
#define _SAMPLE_COMM_H_

#include <stdio.h>
#include "rkisp_demo.h"
#include "rk_aiq_comm.h"

#define CLEAR() printf("\x1b[2J\x1b[H");
#define RESET_CURSOR() printf("\033[H");

#define RKAIQ_SAMPLE_CHECK_RET(ret, format, ...) \
    if (ret) { \
        printf(format, ##__VA_ARGS__); \
        return ret; \
    }

enum RK_ISP_HW_MODULE_e {
    RK_ISP_AE,
    RK_ISP_AWB,
    RK_ISP_AF,
    RK_ISP_ACCM,
    RK_ISP_A3DLUT,
    RK_ISP_ADRC,
    RK_ISP_AMERGE,
    RK_ISP_AGAMMA,
    RK_ISP_ADEHAZE,
    RK_ISP_AYNR,
    RK_ISP_ACNR,
    RK_ISP_ASHARP,
    RK_ISP_ABAYER2DNR,
    RK_ISP_ABAYERTNR,
    RK_ISP_CSM,
    RK_ISP_ABLC,
    RK_ISP_MAX,
};

#endif

