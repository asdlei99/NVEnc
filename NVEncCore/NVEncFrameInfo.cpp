﻿// -----------------------------------------------------------------------------------------
// NVEnc by rigaya
// -----------------------------------------------------------------------------------------
//
// The MIT License
//
// Copyright (c) 2014-2016 rigaya
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// ------------------------------------------------------------------------------------------

#include "NVEncFrameInfo.h"

#if 0
FrameInfoExtra getFrameInfoExtra(const RGYFrameInfo *pFrameInfo) {
    FrameInfoExtra exinfo = { 0 };
    switch (pFrameInfo->csp) {
    case RGY_CSP_NV12:
        exinfo.width_byte = pFrameInfo->width;
        exinfo.height_total = pFrameInfo->height * 3 / 2;
        break;
    case RGY_CSP_YV12:
        exinfo.width_byte = pFrameInfo->width;
        exinfo.height_total = pFrameInfo->height * 2;
        break;
    case RGY_CSP_YUY2:
        exinfo.width_byte = pFrameInfo->width * 2;
        exinfo.height_total = pFrameInfo->height;
        break;
    case RGY_CSP_YUV422:
        exinfo.width_byte = pFrameInfo->width;
        exinfo.height_total = pFrameInfo->height * 2;
        break;
    case RGY_CSP_NV16:
        exinfo.width_byte = pFrameInfo->width;
        exinfo.height_total = pFrameInfo->height * 2;
        break;
    case RGY_CSP_YUV444:
        exinfo.width_byte = pFrameInfo->width;
        exinfo.height_total = pFrameInfo->height * 3;
        break;
    case RGY_CSP_YV12_09:
    case RGY_CSP_YV12_10:
    case RGY_CSP_YV12_12:
    case RGY_CSP_YV12_14:
    case RGY_CSP_YV12_16:
        exinfo.width_byte = pFrameInfo->width * 2;
        exinfo.height_total = pFrameInfo->height * 2;
        break;
    case RGY_CSP_P010:
        exinfo.width_byte = pFrameInfo->width * 2;
        exinfo.height_total = pFrameInfo->height * 3 / 2;
        break;
    case RGY_CSP_YUV422_09:
    case RGY_CSP_YUV422_10:
    case RGY_CSP_YUV422_12:
    case RGY_CSP_YUV422_14:
    case RGY_CSP_YUV422_16:
        exinfo.width_byte = pFrameInfo->width * 2;
        exinfo.height_total = pFrameInfo->height * 3;
        break;
    case RGY_CSP_P210:
        exinfo.width_byte = pFrameInfo->width * 2;
        exinfo.height_total = pFrameInfo->height * 2;
        break;
    case RGY_CSP_YUV444_09:
    case RGY_CSP_YUV444_10:
    case RGY_CSP_YUV444_12:
    case RGY_CSP_YUV444_14:
    case RGY_CSP_YUV444_16:
        exinfo.width_byte = pFrameInfo->width * 2;
        exinfo.height_total = pFrameInfo->height * 3;
        break;
    case RGY_CSP_YUV444_32:
        exinfo.width_byte = pFrameInfo->width * 4;
        exinfo.height_total = pFrameInfo->height * 3;
        break;
    case RGY_CSP_YUVA444:
        exinfo.width_byte = pFrameInfo->width;
        exinfo.height_total = pFrameInfo->height * 4;
        break;
    case RGY_CSP_YUVA444_16:
        exinfo.width_byte = pFrameInfo->width * 2;
        exinfo.height_total = pFrameInfo->height * 4;
        break;
    case RGY_CSP_RGB24:
    case RGY_CSP_RGB24R:
    case RGY_CSP_BGR24:
        exinfo.width_byte = (pFrameInfo->width * 3 + 3) & (~3);
        exinfo.height_total = pFrameInfo->height;
        break;
    case RGY_CSP_RGB32:
    case RGY_CSP_RGB32R:
    case RGY_CSP_BGR32:
        exinfo.width_byte = pFrameInfo->width * 4;
        exinfo.height_total = pFrameInfo->height;
        break;
    case RGY_CSP_RGB:
        exinfo.width_byte = pFrameInfo->width;
        exinfo.height_total = pFrameInfo->height * 3;
        break;
    case RGY_CSP_RGBA:
        exinfo.width_byte = pFrameInfo->width;
        exinfo.height_total = pFrameInfo->height * 4;
        break;
    case RGY_CSP_RGB_16:
    case RGY_CSP_BGR_16:
        exinfo.width_byte = pFrameInfo->width * 2;
        exinfo.height_total = pFrameInfo->height * 3;
        break;
    case RGY_CSP_RGBA_16:
    case RGY_CSP_BGRA_16:
        exinfo.width_byte = pFrameInfo->width * 2;
        exinfo.height_total = pFrameInfo->height * 4;
        break;
    case RGY_CSP_RGB_F32:
    case RGY_CSP_BGR_F32:
        exinfo.width_byte = pFrameInfo->width * 4;
        exinfo.height_total = pFrameInfo->height * 3;
        break;
    case RGY_CSP_RGBA_F32:
    case RGY_CSP_BGRA_F32:
        exinfo.width_byte = pFrameInfo->width * 4;
        exinfo.height_total = pFrameInfo->height * 4;
        break;
    case RGY_CSP_YC48:
        exinfo.width_byte = pFrameInfo->width * 6;
        exinfo.height_total = pFrameInfo->height;
        break;
    case RGY_CSP_Y8:
        exinfo.width_byte = pFrameInfo->width;
        exinfo.height_total = pFrameInfo->height;
        break;
    case RGY_CSP_Y16:
        exinfo.width_byte = pFrameInfo->width * 2;
        exinfo.height_total = pFrameInfo->height;
        break;
    default:
        break;
    }
    exinfo.frame_size = pFrameInfo->pitch[0] * exinfo.height_total;
    return exinfo;
}
#endif

static RGYFrameInfo getPlaneSingleAlloc(const RGYFrameInfo *frameInfo, const RGY_PLANE plane) {
    RGYFrameInfo planeInfo = *frameInfo;
    for (int i = 1; i < _countof(planeInfo.ptr); i++) {
        planeInfo.ptr[i] = nullptr;
        planeInfo.pitch[i] = 0;
    }
    if (frameInfo->csp == RGY_CSP_GBR || frameInfo->csp == RGY_CSP_GBRA) {
        switch (plane) {
        case RGY_PLANE_G: break;
        case RGY_PLANE_B: planeInfo.ptr[0] += frameInfo->pitch[0] * frameInfo->height; break;
        case RGY_PLANE_R: planeInfo.ptr[0] += frameInfo->pitch[0] * frameInfo->height * 2; break;
        case RGY_PLANE_A: planeInfo.ptr[0] += frameInfo->pitch[0] * frameInfo->height * 3; break;
        default: break;
        }
    } else if (frameInfo->csp == RGY_CSP_RGB || frameInfo->csp == RGY_CSP_RGBA
            || frameInfo->csp == RGY_CSP_RGB_16 || frameInfo->csp == RGY_CSP_RGBA_16
            || frameInfo->csp == RGY_CSP_RGB_F32 || frameInfo->csp == RGY_CSP_RGBA_F32) {
        switch (plane) {
        case RGY_PLANE_R: break;
        case RGY_PLANE_G: planeInfo.ptr[0] += frameInfo->pitch[0] * frameInfo->height; break;
        case RGY_PLANE_B: planeInfo.ptr[0] += frameInfo->pitch[0] * frameInfo->height * 2; break;
        case RGY_PLANE_A: planeInfo.ptr[0] += frameInfo->pitch[0] * frameInfo->height * 3; break;
        default: break;
        }
    } else if (frameInfo->csp == RGY_CSP_BGR_16 || frameInfo->csp == RGY_CSP_BGRA_16
            || frameInfo->csp == RGY_CSP_BGR_F32 || frameInfo->csp == RGY_CSP_BGRA_F32) {
        switch (plane) {
        case RGY_PLANE_B: break;
        case RGY_PLANE_G: planeInfo.ptr[0] += frameInfo->pitch[0] * frameInfo->height; break;
        case RGY_PLANE_R: planeInfo.ptr[0] += frameInfo->pitch[0] * frameInfo->height * 2; break;
        case RGY_PLANE_A: planeInfo.ptr[0] += frameInfo->pitch[0] * frameInfo->height * 3; break;
        default: break;
        }
    } else {
        switch (plane) {
        case RGY_PLANE_A:
            if (RGY_CSP_CHROMA_FORMAT[frameInfo->csp] != RGY_CHROMAFMT_YUVA444) {
                planeInfo.ptr[0] = nullptr;
                break;
            }
            //フォールスルー
        case RGY_PLANE_U:
        case RGY_PLANE_V:
            //case RGY_PLANE_G:
            //case RGY_PLANE_B:
            if (frameInfo->csp == RGY_CSP_YUY2
                || RGY_CSP_CHROMA_FORMAT[frameInfo->csp] == RGY_CHROMAFMT_RGB_PACKED
                || RGY_CSP_CHROMA_FORMAT[frameInfo->csp] == RGY_CHROMAFMT_MONOCHROME) {
                ; //なにもしない
            } else if (frameInfo->csp == RGY_CSP_NV12 || frameInfo->csp == RGY_CSP_P010) {
                planeInfo.ptr[0] += frameInfo->pitch[0] * frameInfo->height;
                planeInfo.height >>= 1;
            } else if (frameInfo->csp == RGY_CSP_NV16 || frameInfo->csp == RGY_CSP_P210) {
                planeInfo.ptr[0] += frameInfo->pitch[0] * frameInfo->height;
            } else if (RGY_CSP_CHROMA_FORMAT[frameInfo->csp] == RGY_CHROMAFMT_YUV420) {
                planeInfo.ptr[0] += frameInfo->pitch[0] * frameInfo->height;
                planeInfo.width >>= 1;
                planeInfo.height >>= 1;
                if (plane == RGY_PLANE_V) {
                    planeInfo.ptr[0] += planeInfo.pitch[0] * planeInfo.height;
                }
            } else if (RGY_CSP_CHROMA_FORMAT[frameInfo->csp] == RGY_CHROMAFMT_YUV422) {
                planeInfo.ptr[0] += plane * frameInfo->pitch[0] * frameInfo->height;
                planeInfo.width >>= 1;
                if (plane == RGY_PLANE_V) {
                    planeInfo.ptr[0] += planeInfo.pitch[0] * planeInfo.height;
                }
            } else { //RGY_CHROMAFMT_YUV444 & RGY_CHROMAFMT_YUVA444 & RGY_CHROMAFMT_RGB
                planeInfo.ptr[0] += plane * planeInfo.pitch[0] * planeInfo.height;
            }
            break;
        case RGY_PLANE_Y:
            //case RGY_PLANE_R:
        default:
            break;
        }
    }
    return planeInfo;
}

RGYFrameInfo getPlane(const RGYFrameInfo *frameInfo, RGY_PLANE plane) {
    if (frameInfo->singleAlloc) {
        return getPlaneSingleAlloc(frameInfo, plane);
    }
    RGYFrameInfo planeInfo = *frameInfo;
    if (frameInfo->csp == RGY_CSP_YUY2
        || frameInfo->csp == RGY_CSP_Y210 || frameInfo->csp == RGY_CSP_Y216
        || frameInfo->csp == RGY_CSP_Y410 || frameInfo->csp == RGY_CSP_Y416
        || RGY_CSP_CHROMA_FORMAT[frameInfo->csp] == RGY_CHROMAFMT_RGB_PACKED
        || RGY_CSP_CHROMA_FORMAT[frameInfo->csp] == RGY_CHROMAFMT_MONOCHROME
        || RGY_CSP_PLANES[frameInfo->csp] == 1) {
        return planeInfo; //何もしない
    }
    if (frameInfo->csp == RGY_CSP_GBR || frameInfo->csp == RGY_CSP_GBRA) {
        switch (plane) {
        case RGY_PLANE_R: plane = RGY_PLANE_G; break;
        case RGY_PLANE_G: plane = RGY_PLANE_R; break;
        default:
            break;
        }
    }
    if (frameInfo->csp == RGY_CSP_BGR_16 || frameInfo->csp == RGY_CSP_BGRA_16
        || frameInfo->csp == RGY_CSP_BGR_F32 || frameInfo->csp == RGY_CSP_BGRA_F32) {
        switch (plane) {
        case RGY_PLANE_R: plane = RGY_PLANE_B; break;
        case RGY_PLANE_B: plane = RGY_PLANE_R; break;
        default:
            break;
        }
    }
    if (plane == RGY_PLANE_Y) {
        for (int i = 1; i < RGY_MAX_PLANES; i++) {
            planeInfo.ptr[i] = nullptr;
            planeInfo.pitch[i] = 0;
        }
        return planeInfo;
    }
    auto const ptr = planeInfo.ptr[plane];
    const auto pitch = planeInfo.pitch[plane];
    for (int i = 0; i < RGY_MAX_PLANES; i++) {
        planeInfo.ptr[i] = nullptr;
        planeInfo.pitch[i] = 0;
    }
    planeInfo.ptr[0] = ptr;
    planeInfo.pitch[0] = pitch;

    if (frameInfo->csp == RGY_CSP_NV12 || frameInfo->csp == RGY_CSP_P010) {
        planeInfo.height >>= 1;
    } else if (frameInfo->csp == RGY_CSP_NV16 || frameInfo->csp == RGY_CSP_P210) {
        ;
    } else if (RGY_CSP_CHROMA_FORMAT[frameInfo->csp] == RGY_CHROMAFMT_YUV420) {
        planeInfo.width >>= 1;
        planeInfo.height >>= 1;
    } else if (RGY_CSP_CHROMA_FORMAT[frameInfo->csp] == RGY_CHROMAFMT_YUV422) {
        planeInfo.width >>= 1;
    }
    return planeInfo;
}
