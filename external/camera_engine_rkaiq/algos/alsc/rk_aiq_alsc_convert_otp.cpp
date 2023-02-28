#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcam_std.h>
#include "rk_aiq_alsc_convert_otp.h"

// #define WRITE_OTP_TABLE 1
#define LOGI printf

void convertLscTableParameter(AlscOtpInfo_t *otpInfo, int32_t bayer_pattern, int32_t dstWidth, int32_t dstHeight)
{
    XCAM_STATIC_PROFILING_START(convertLscTable);

    int32_t bayer = bayer_pattern;
    uint32_t srcLscWidth    = otpInfo->width;
    uint32_t srcLscHeight   = otpInfo->height;

    uint32_t maxSize        = srcLscWidth > srcLscHeight ? srcLscWidth : srcLscHeight;
    uint32_t ratio          = maxSize > 3200 ? 8 : (maxSize > 1600 ? 4 : (maxSize > 800 ? 2 : 1));
    srcLscWidth             = 2 * (int32_t)(srcLscWidth / (ratio * 2));
    srcLscHeight            = 2 * (int32_t)(srcLscHeight / (ratio * 2));
    dstWidth                = 2 * (int32_t)(dstWidth / (ratio * 2));
    dstHeight               = 2 * (int32_t)(dstHeight / (ratio * 2));

    uint16_t *rTable = new uint16_t[srcLscWidth * srcLscHeight];
    uint16_t *grTable = new uint16_t[srcLscWidth * srcLscHeight];
    uint16_t *gbTable = new uint16_t[srcLscWidth * srcLscHeight];
    uint16_t *bTable = new uint16_t[srcLscWidth * srcLscHeight];
    uint16_t *corTable = new uint16_t[srcLscWidth * srcLscHeight];
    uint16_t *corTable_crop = new uint16_t[dstWidth * dstHeight];

#if WRITE_OTP_TABLE
    char fileName[32] = {0};
    snprintf(fileName, sizeof(fileName) - 1, "/tmp/otp_lsc_r.bin");
    writeFile(fileName, otpInfo->lsc_r);
    snprintf(fileName, sizeof(fileName) - 1, "/tmp/otp_lsc_b.bin");
    writeFile(fileName, otpInfo->lsc_b);
    snprintf(fileName, sizeof(fileName) - 1, "/tmp/otp_lsc_gr.bin");
    writeFile(fileName, otpInfo->lsc_gr);
    snprintf(fileName, sizeof(fileName) - 1, "/tmp/otp_lsc_gb.bin");
    writeFile(fileName, otpInfo->lsc_gb);
#endif

    // Interpolate gain table back to full size
    int sizeX[16];
    int sizeY[16];
    computeSamplingInterval(srcLscWidth, srcLscHeight, sizeX, sizeY);
    calculateCorrectFactor(otpInfo->lsc_r, rTable, srcLscWidth, srcLscHeight, sizeX, sizeY);
    calculateCorrectFactor(otpInfo->lsc_gr, grTable, srcLscWidth, srcLscHeight, sizeX, sizeY);
    calculateCorrectFactor(otpInfo->lsc_gb, gbTable, srcLscWidth, srcLscHeight, sizeX, sizeY);
    calculateCorrectFactor(otpInfo->lsc_b, bTable, srcLscWidth, srcLscHeight, sizeX, sizeY);
    getLscParameter(rTable, grTable, gbTable, bTable, corTable, srcLscWidth, srcLscHeight, bayer);

    // Clipping a Gain table is centered by default
    int cropoffsetx = srcLscWidth / 2 - dstWidth / 2;
    int cropoffsety = srcLscHeight / 2 - dstHeight / 2;

    if (cropoffsetx % 2 != 0)
        cropoffsetx = cropoffsetx - 1;
    if (cropoffsety % 2 != 0)
        cropoffsety = cropoffsety -1;

    for (int i = 0; i < dstHeight; i++)
    {

        memcpy(corTable_crop + i * dstWidth, corTable + (cropoffsety + i)*srcLscWidth + cropoffsetx, dstWidth * 2);
        //*(corTable_crop + i*dstWidth + j) = *(corTable + (cropoffsety + i)*srcLscWidth + j + cropoffsetx);
    }

    delete[] rTable;
    delete[] grTable;
    delete[] gbTable;
    delete[] bTable;
    delete[] corTable;


    // down-sampling again according to crop size
    uint16_t *plscdataR = new uint16_t[dstWidth / 2 * dstHeight / 2];
    uint16_t *plscdataGr = new uint16_t[dstWidth / 2 * dstHeight / 2];
    uint16_t *plscdataGb = new uint16_t[dstWidth / 2 * dstHeight / 2];
    uint16_t *plscdataB = new uint16_t[dstWidth / 2 * dstHeight / 2];

    memset(plscdataR, 0, sizeof(uint16_t)*dstWidth / 2 * dstHeight / 2);
    memset(plscdataGr, 0, sizeof(uint16_t)*dstWidth / 2 * dstHeight / 2);
    memset(plscdataGb, 0, sizeof(uint16_t)*dstWidth / 2 * dstHeight / 2);
    memset(plscdataB, 0, sizeof(uint16_t)*dstWidth / 2 * dstHeight / 2);

    separateBayerChannel(corTable_crop, plscdataR, plscdataGr, plscdataGb, plscdataB, dstWidth, dstHeight, bayer);


    int cPos[17];       // col sampling
    int rPos[17];       // row sampling
    computeSamplingPoint(dstWidth / 2, cPos);
    computeSamplingPoint(dstHeight / 2, rPos);
    int r, c;
    for (int i = 0; i < 17; i++)
    {
        for (int j = 0; j < 17; j++)
        {
            r = rPos[i];
            c = cPos[j];

            *(otpInfo->lsc_r + i * 17 + j) = plscdataR[r * dstWidth / 2 + c];
            *(otpInfo->lsc_gr + i * 17 + j) = plscdataGr[r * dstWidth / 2 + c];
            *(otpInfo->lsc_gb + i * 17 + j) = plscdataGb[r * dstWidth / 2 + c];
            *(otpInfo->lsc_b + i * 17 + j) = plscdataB[r * dstWidth / 2 + c];
        }
    }

#if WRITE_OTP_TABLE
    snprintf(fileName, sizeof(fileName) - 1, "/tmp/converted_otp_lsc_r.bin");
    writeFile(fileName, otpInfo->lsc_r);
    snprintf(fileName, sizeof(fileName) - 1, "/tmp/converted_otp_lsc_b.bin");
    writeFile(fileName, otpInfo->lsc_b);
    snprintf(fileName, sizeof(fileName) - 1, "/tmp/converted_otp_lsc_gr.bin");
    writeFile(fileName, otpInfo->lsc_gr);
    snprintf(fileName, sizeof(fileName) - 1, "/tmp/converted_otp_lsc_gb.bin");
    writeFile(fileName, otpInfo->lsc_gb);
#endif

    delete[] plscdataR;
    delete[] plscdataGr;
    delete[] plscdataGb;
    delete[] plscdataB;
    delete[] corTable_crop;

    XCAM_STATIC_PROFILING_END(convertLscTable, 0);
}

void computeSamplingPoint(int size, int *pos)
{
    float sampPos[17] = { 0.0, 6.25, 12.5, 18.75, 25.0, 31.25, 37.5, 43.75, 50.0,
                          56.25, 62.5, 68.75, 75.0, 81.25, 87.5, 93.75, 100.0
                        };
    float tmpVal;

    pos[0] = 0;
    for (int i = 1; i < 17; i++)
    {
        tmpVal = sampPos[i] / 100;
        pos[i] = (int)(tmpVal * size) - 1;
    }

}

void computeSamplingInterval(int width, int height, int *xInterval, int *yInterval)
{
    int xpos[17];
    int ypos[17];

    computeSamplingPoint(width, xpos);
    computeSamplingPoint(height, ypos);

    for (int i = 0; i < 16; i++)
    {
        xInterval[i] = xpos[i + 1] - xpos[i];
        yInterval[i] = ypos[i + 1] - ypos[i];
    }

    xInterval[0] = xInterval[0] + 1;
    yInterval[0] = yInterval[0] + 1;
}

void calculateCorrectFactor(uint16_t *table, uint16_t *correctTable, int width, int height, int *xInterval, int *yInterval)
{
    int xGrad[16], yGrad[16];
    int xblk, yblk;
    uint16_t xbase, ybase, xoffset, yoffset;
    uint16_t curSizeX, curSizeY, curGradX, curGradY;
    uint16_t luCoeff, ldCoeff, ruCoeff, rdCoeff;
    uint32_t lCoeff, rCoeff, coeff;
    uint32_t tmp, tmp2;

    computeGradient(xInterval, yInterval, xGrad, yGrad);

    for (ybase = 0, yblk = 0; yblk < 16; yblk++)
    {
        curSizeY = yInterval[yblk];
        curGradY = yGrad[yblk];
        for (xbase = 0, xblk = 0; xblk < 16; xblk++)
        {
            curSizeX = xInterval[xblk];
            curGradX = xGrad[xblk];
            luCoeff = table[yblk * 17 + xblk];
            ldCoeff = table[(yblk + 1) * 17 + xblk];
            ruCoeff = table[yblk * 17 + xblk + 1];
            rdCoeff = table[(yblk + 1) * 17 + xblk + 1];
            for (yoffset = 0; yoffset < curSizeY; yoffset++)
            {
                tmp = abs(luCoeff - ldCoeff);
                tmp = tmp * curGradY;
                tmp = (tmp + c_dy_round) >> c_dy_shift;
                tmp = tmp * yoffset;
                tmp = (tmp + c_extend_round) >> c_lsc_corr_extend;
                tmp = (tmp << (32 - c_lsc_corr_bw)) >> (32 - c_lsc_corr_bw);
                lCoeff = luCoeff << c_corr_diff;
                lCoeff = (luCoeff > ldCoeff) ? (lCoeff - tmp) : (lCoeff + tmp);

                tmp = abs(ruCoeff - rdCoeff);
                tmp = tmp * curGradY;
                tmp = (tmp + c_dy_round) >> c_dy_shift;
                tmp = tmp * yoffset;
                tmp = (tmp + c_extend_round) >> c_lsc_corr_extend;
                tmp = (tmp << (32 - c_lsc_corr_bw)) >> (32 - c_lsc_corr_bw);
                rCoeff = ruCoeff << c_corr_diff;
                rCoeff = (ruCoeff > rdCoeff) ? (rCoeff - tmp) : (rCoeff + tmp);

                coeff = lCoeff << c_lsc_corr_extend;
                tmp = abs((int)(rCoeff - lCoeff));
                tmp = tmp * curGradX;
                tmp = (tmp + c_dx_round) >> c_dx_shift;
                for (xoffset = 0; xoffset < curSizeX; xoffset++)
                {
                    tmp2 = (coeff + c_extend_round) >> c_lsc_corr_extend;
                    tmp2 = (tmp2 > ((1 << c_lsc_corr_bw) - 1)) ? ((1 << c_lsc_corr_bw) - 1) : tmp2;
                    *(correctTable + (ybase + yoffset) * width + (xbase + xoffset)) = (uint16_t)tmp2 >> c_corr_diff;
                    coeff = (lCoeff > rCoeff) ? (coeff - tmp) : (coeff + tmp);
                }
            }
            xbase += curSizeX;
        }
        ybase += curSizeY;
    }
}

void getLscParameter(uint16_t *r, uint16_t *gr, uint16_t *gb, uint16_t *b, uint16_t *table, int width, int height, int bayer)
{
    int bayerIdx;
    int idx;

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            idx = i * width + j;
            bayerIdx = getBayerIndex(bayer, i, j);
            switch (bayerIdx)
            {
            case 0:
                table[idx] = r[idx];
                break;
            case 1:
                table[idx] = gr[idx];
                break;
            case 2:
                table[idx] = gb[idx];
                break;
            case 3:
                table[idx] = b[idx];
                break;
            default:
                break;
            }
        }
    }
}

void separateBayerChannel(uint16_t* src, uint16_t* disR, uint16_t* disGr, uint16_t* disGb, uint16_t* disB, int width, int height, int bayer)
{
    int index = 0;
    int bayerIdx = 0;

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            index = i * width + j;
            bayerIdx = getBayerIndex(bayer, i, j);
            switch (bayerIdx)
            {
            case 0:
                *(disR++) = *(src + index);
                break;
            case 1:
                *(disGr++) = *(src + index);
                break;
            case 2:
                *(disGb++) = *(src + index);
                break;
            case 3:
                *(disB++) = *(src + index);
                break;
            default:
                break;
            }
        }
    }
}
void computeGradient(int *xInterval, int *yInterval, int *xGradient, int *yGradient)
{
    // gradient N = INT(2^15 / SizeN + 0.5)
    for (int i = 0; i < 16; i++)
    {
        xGradient[i] = (int)(32768.0 / (double)xInterval[i] + 0.5);
        if (xGradient[i] > 8191)
        {
            xGradient[i] = 8191;
        }

        yGradient[i] = (int)(32768.0 / (double)yInterval[i] + 0.5);
        if (yGradient[i] > 8191)
        {
            yGradient[i] = 8191;
        }
    }
}
int getBayerIndex(int pattern, int row, int col)
{
    int index = 0;
    int x, y;
    int tmp = 0;

    x = row % 2;
    y = col % 2;

    if (x == 0 && y == 0)
    {
        tmp = 0;
    }
    else if (x == 0 && y == 1)
    {
        tmp = 1;
    }
    else if (x == 1 && y == 0)
    {
        tmp = 2;
    }
    else if (x == 1 && y == 1)
    {
        tmp = 3;
    }

    if (pattern == BAYER_BGGR)
    {
        switch (tmp)
        {
        case 0:
            index = B_INDEX;
            break;
        case 1:
            index = GB_INDEX;
            break;
        case 2:
            index = GR_INDEX;
            break;
        case 3:
            index = R_INDEX;
            break;
        }
    }
    else if (pattern == BAYER_GBRG)
    {
        switch (tmp)
        {
        case 0:
            index = GB_INDEX;
            break;
        case 1:
            index = B_INDEX;
            break;
        case 2:
            index = R_INDEX;
            break;
        case 3:
            index = GR_INDEX;
            break;
        }
    }
    else if (pattern == BAYER_GRBG)
    {
        switch (tmp)
        {
        case 0:
            index = GR_INDEX;
            break;
        case 1:
            index = R_INDEX;
            break;
        case 2:
            index = B_INDEX;
            break;
        case 3:
            index = GB_INDEX;
            break;
        }
    }
    else if (pattern == BAYER_RGGB)
    {
        switch (tmp)
        {
        case 0:
            index = R_INDEX;
            break;
        case 1:
            index = GR_INDEX;
            break;
        case 2:
            index = GB_INDEX;
            break;
        case 3:
            index = B_INDEX;
            break;
        }
    }

    return index;
}

void writeFile(char *fileName, uint16_t *buf)
{
    FILE *fd = fopen(fileName, "wb");
    if (fd == NULL)
        printf("%s: open failed: %s\n", __func__, fileName);
    else {
        fwrite(buf, 1, LSCDATA_LEN * sizeof(uint16_t), fd);
        fclose(fd);
    }
}

