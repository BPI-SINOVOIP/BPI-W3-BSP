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

#include "sample_comm.h"

#define UAPI_INTERP_DEBAYER(x0, x1, ratio)    ((ratio) * ((x1) - (x0)) + x0)

static void sample_adebayer_usage()
{
    printf("Usage : \n");
    printf("  Module API: \n");
    printf("\t 0) ADEBAYER:         enable/disable with AUTO mode in sync.\n");
    printf("\t 1) ADEBAYER:         set sharp strength of 250 with AUTO modein sync.\n");
    printf("\t 2) ADEBAYER:         set sharp strength of 0 with AUTO modein sync.\n");
    printf("\t 3) ADEBAYER:         set high freq thresh of 250 with AUTO modein sync.\n");
    printf("\t 4) ADEBAYER:         set high freq thresh of 0 with AUTO mode in sync.\n");
    printf("\t 5) ADEBAYER:         set low freq thresh of 250 with AUTO mode in sync.\n");
    printf("\t 6) ADEBAYER:         set low freq thresh of 0 with AUTO mode in sync.\n");
    printf("\n");

    printf("\t 7) ADEBAYER:         enable/disable with MANUAL mode in sync.\n");
    printf("\t 8) ADEBAYER:         set manual params from json with MANUAL mode in sync, iso is 50.\n");
    printf("\t 9) ADEBAYER:         set manual params from json with MANUAL mode in sync, iso is 6400.\n");
    printf("\n");

    printf("\t h) ADEBAYER: help.\n");
    printf("\t q/Q) ADEBAYER:       return to main sample screen.\n");
    printf("\n");
    printf("\t please press the key: \n\n");

    return;
}

void sample_print_adebayer_info(const void *arg)
{
    printf ("enter ADEBAYER test!\n");
}

XCamReturn sample_adebayer_en(const rk_aiq_sys_ctx_t* ctx, rk_aiq_debayer_op_mode_t mode, bool en)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (ctx == NULL) {
        ret = XCAM_RETURN_ERROR_PARAM;
        RKAIQ_SAMPLE_CHECK_RET(ret, "param error!");
    }

    adebayer_attrib_t attr;
    ret = rk_aiq_user_api2_adebayer_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get debayer attrib failed!");
    attr.enable = en;
    attr.mode = mode;
    rk_aiq_user_api2_adebayer_SetAttrib(ctx, attr);

    ret = rk_aiq_user_api2_adebayer_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get ldch attrib failed!");

    printf ("sync_mode: %d, done: %d\n", attr.sync.sync_mode, attr.sync.done);

    return ret;
}

XCamReturn sample_adebayer_setSharpStrength(const rk_aiq_sys_ctx_t* ctx, unsigned char *strength)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (ctx == NULL) {
        ret = XCAM_RETURN_ERROR_PARAM;
        RKAIQ_SAMPLE_CHECK_RET(ret, "param error!");
    }

    adebayer_attrib_t attr;
    ret = rk_aiq_user_api2_adebayer_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get debayer attrib failed!");
    attr.mode = RK_AIQ_DEBAYER_MODE_AUTO;
    memcpy(attr.stAuto.sharp_strength, strength, sizeof(attr.stAuto.sharp_strength));
    rk_aiq_user_api2_adebayer_SetAttrib(ctx, attr);

    ret = rk_aiq_user_api2_adebayer_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get ldch attrib failed!");

    printf ("mode: %d, sync_mode: %d, done: %d\n", attr.mode, attr.sync.sync_mode, attr.sync.done);

    return ret;
}

XCamReturn sample_adebayer_setLowFreqThresh(const rk_aiq_sys_ctx_t* ctx, __u8 thresh)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (ctx == NULL) {
        ret = XCAM_RETURN_ERROR_PARAM;
        RKAIQ_SAMPLE_CHECK_RET(ret, "param error!");
    }

    adebayer_attrib_t attr;
    ret = rk_aiq_user_api2_adebayer_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get debayer attrib failed!");
    attr.mode = RK_AIQ_DEBAYER_MODE_AUTO;
    memset(attr.stAuto.debayer_thed1, thresh, sizeof(attr.stAuto.debayer_thed1));
    rk_aiq_user_api2_adebayer_SetAttrib(ctx, attr);

    ret = rk_aiq_user_api2_adebayer_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get ldch attrib failed!");

    printf ("mode: %d, sync_mode: %d, done: %d\n", attr.mode, attr.sync.sync_mode, attr.sync.done);

    return ret;
}

XCamReturn sample_adebayer_setHighFreqThresh(const rk_aiq_sys_ctx_t* ctx, __u8 thresh)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (ctx == NULL) {
        ret = XCAM_RETURN_ERROR_PARAM;
        RKAIQ_SAMPLE_CHECK_RET(ret, "param error!");
    }

    adebayer_attrib_t attr;
    ret = rk_aiq_user_api2_adebayer_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get debayer attrib failed!");
    attr.mode = RK_AIQ_DEBAYER_MODE_AUTO;
    memset(attr.stAuto.debayer_thed0, thresh, sizeof(attr.stAuto.debayer_thed0));
    rk_aiq_user_api2_adebayer_SetAttrib(ctx, attr);

    printf ("mode: %d, sync_mode: %d, done: %d\n", attr.mode, attr.sync.sync_mode, attr.sync.done);

    return ret;
}

XCamReturn
sample_adebayer_translate_params(adebayer_attrib_auto_t& stAuto, adebayer_attrib_manual_t& stManual, int32_t ISO)
{
    int iso_low = 0, iso_high = 0, iso_low_index = 0, iso_high_index = 0;
    float ratio = 0.0f;
    int i = 0;

    for(int j = 0; j < 5; j++) {

        stManual.filter1[i] = stAuto.debayer_filter1[i];
        stManual.filter2[i] = stAuto.debayer_filter2[i];

    }

    for(i = 0; i < UAPI_DEBAYER_ISO_LEN; i++) {
        if (ISO < stAuto.ISO[i])
        {

            iso_low = stAuto.ISO[MAX(0, i - 1)];
            iso_high = stAuto.ISO[i];
            iso_low_index = MAX(0, i - 1);
            iso_high_index = i;

            if(i == 0)
                ratio = 0.0f;
            else
                ratio = (float)(ISO - iso_low) / (iso_high - iso_low);

            break;
        }
    }

    if(i == UAPI_DEBAYER_ISO_LEN) {
        iso_low = stAuto.ISO[i - 1];
        iso_high = stAuto.ISO[i - 1];
        iso_low_index = i - 1;
        iso_high_index = i - 1;
        ratio = 1;
    }

    stManual.offset = ROUND_F(UAPI_INTERP_DEBAYER(stAuto.debayer_offset[iso_low_index], stAuto.debayer_offset[iso_high_index], ratio));
    stManual.gain_offset = ROUND_F(UAPI_INTERP_DEBAYER(stAuto.debayer_gain_offset[iso_low_index], stAuto.debayer_gain_offset[iso_high_index], ratio));
    stManual.clip_en = ROUND_F(UAPI_INTERP_DEBAYER(stAuto.debayer_clip_en[iso_low_index], stAuto.debayer_clip_en[iso_high_index], ratio));
    stManual.filter_g_en = ROUND_F(UAPI_INTERP_DEBAYER(stAuto.debayer_filter_g_en[iso_low_index], stAuto.debayer_filter_g_en[iso_high_index], ratio));
    stManual.filter_c_en = ROUND_F(UAPI_INTERP_DEBAYER(stAuto.debayer_filter_c_en[iso_low_index], stAuto.debayer_filter_c_en[iso_high_index], ratio));
    stManual.thed0 = ROUND_F(UAPI_INTERP_DEBAYER(stAuto.debayer_thed0[iso_low_index], stAuto.debayer_thed0[iso_high_index], ratio));
    stManual.thed1 = ROUND_F(UAPI_INTERP_DEBAYER(stAuto.debayer_thed1[iso_low_index], stAuto.debayer_thed1[iso_high_index], ratio));
    stManual.dist_scale = ROUND_F(UAPI_INTERP_DEBAYER(stAuto.debayer_dist_scale[iso_low_index], stAuto.debayer_dist_scale[iso_high_index], ratio));
    stManual.shift_num = ROUND_F(UAPI_INTERP_DEBAYER(stAuto.debayer_shift_num[iso_low_index], stAuto.debayer_shift_num[iso_high_index], ratio));
    stManual.sharp_strength = ROUND_F(UAPI_INTERP_DEBAYER(stAuto.sharp_strength[iso_low_index], stAuto.sharp_strength[iso_high_index], ratio));
    stManual.hf_offset = ROUND_F(UAPI_INTERP_DEBAYER(stAuto.debayer_hf_offset[iso_low_index], stAuto.debayer_hf_offset[iso_high_index], ratio));
    stManual.cnr_strength = ROUND_F(UAPI_INTERP_DEBAYER(stAuto.debayer_cnr_strength[iso_low_index], stAuto.debayer_cnr_strength[iso_high_index], ratio));

    printf ("sharp_strength: %d, hf_offset: %d\n", stManual.sharp_strength, stManual.hf_offset);

    return XCAM_RETURN_NO_ERROR;
}

XCamReturn sample_adebayer_setManualAtrrib(const rk_aiq_sys_ctx_t* ctx, int32_t ISO)
{
    XCamReturn ret = XCAM_RETURN_NO_ERROR;
    if (ctx == NULL) {
        ret = XCAM_RETURN_ERROR_PARAM;
        RKAIQ_SAMPLE_CHECK_RET(ret, "param error!");
    }

    adebayer_attrib_t attr;
    ret = rk_aiq_user_api2_adebayer_GetAttrib(ctx, &attr);
    RKAIQ_SAMPLE_CHECK_RET(ret, "get debayer attrib failed!");
    attr.mode = RK_AIQ_DEBAYER_MODE_MANUAL;
    sample_adebayer_translate_params(attr.stAuto, attr.stManual, ISO);
    rk_aiq_user_api2_adebayer_SetAttrib(ctx, attr);

    printf ("mode: %d, sync_mode: %d, done: %d\n", attr.mode, attr.sync.sync_mode, attr.sync.done);

    return ret;
}

XCamReturn sample_adebayer_module (const void *arg)
{
    int key = -1;
    CLEAR();

    const demo_context_t *demo_ctx = (demo_context_t *)arg;
    const rk_aiq_sys_ctx_t* ctx;
    if (demo_ctx->camGroup) {
        ctx = (rk_aiq_sys_ctx_t*)(demo_ctx->camgroup_ctx);
    } else {
        ctx = (rk_aiq_sys_ctx_t*)(demo_ctx->aiq_ctx);
    }

    if (ctx == nullptr) {
        ERR ("%s, ctx is nullptr\n", __FUNCTION__);
        return XCAM_RETURN_ERROR_PARAM;
    }

    sample_adebayer_usage ();
    do {

        key = getchar ();
        while (key == '\n' || key == '\r')
            key = getchar ();
        printf ("\n");

        switch (key)
        {
        case 'h':
            CLEAR();
            sample_adebayer_usage ();
            break;
        case '0': {
            static bool on = false;
            on = !on;
            sample_adebayer_en(ctx, RK_AIQ_DEBAYER_MODE_AUTO, on);
            printf("%s adebayer\n\n", on ? "enable" : "disable");
            break;
        }
        case '1': {
            unsigned char sharp_strength[UAPI_DEBAYER_ISO_LEN] = {250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250, 250};
            sample_adebayer_setSharpStrength(ctx, sharp_strength);
            printf("test the sharp_strength of 255 in sync mode...\n");
            break;
        }
        case '2': {
            unsigned char sharp_strength[UAPI_DEBAYER_ISO_LEN] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            sample_adebayer_setSharpStrength(ctx, sharp_strength);
            printf("test the sharp_strength of 0 in sync mode...\n");
            break;
        }
        case '3':
            sample_adebayer_setHighFreqThresh(ctx, 250);
            printf("test the high freq thresh of 250 in sync mode...\n");
            break;
        case '4':
            sample_adebayer_setHighFreqThresh(ctx, 0);
            printf("test the high freq thresh of 0 in sync mode...\n");
            break;
        case '5':
            sample_adebayer_setLowFreqThresh(ctx, 250);
            printf("test the low freq thresh of 250 in sync mode...\n");
            break;
        case '6':
            sample_adebayer_setLowFreqThresh(ctx, 0);
            printf("test the low freq thresh of 0 in sync mode...\n");
            break;
        case '7':
            static bool on = false;
            on = !on;
            sample_adebayer_en(ctx, RK_AIQ_DEBAYER_MODE_MANUAL, on);
            printf("%s adebayer\n\n", on ? "enable" : "disable");
            break;
        case '8':
            sample_adebayer_setManualAtrrib(ctx, 50);
            printf("set manual params from json with MANUAL mode in sync, ISO: 50\n");
            break;
        case '9':
            sample_adebayer_setManualAtrrib(ctx, 6400);
            printf("set manual params from json with MANUAL mode in sync, ISO: 6400\n");
            break;
        default:
            break;
        }
    } while (key != 'q' && key != 'Q');

    return XCAM_RETURN_NO_ERROR;
}
