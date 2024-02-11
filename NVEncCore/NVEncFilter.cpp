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

#include "NVEncFilter.h"

const TCHAR *NVEncFilter::INFO_INDENT = _T("               ");

NVEncFilter::NVEncFilter() :
    m_sFilterName(), m_sFilterInfo(), m_pPrintMes(), m_pFrameBuf(), m_nFrameIdx(0),
    m_pFieldPairIn(), m_pFieldPairOut(),
    m_pParam(),
    m_nPathThrough(FILTER_PATHTHROUGH_ALL), m_bCheckPerformance(false),
    m_peFilterStart(), m_peFilterFin(), m_dFilterTimeMs(0.0), m_nFilterRunCount(0) {

}

NVEncFilter::~NVEncFilter() {
    m_pFrameBuf.clear();
    m_pFieldPairIn.reset();
    m_pFieldPairOut.reset();
    m_peFilterStart.reset();
    m_peFilterFin.reset();
    m_pParam.reset();
}

RGY_ERR NVEncFilter::AllocFrameBuf(const RGYFrameInfo& frame, int frames) {
    if ((int)m_pFrameBuf.size() == frames
        && !cmpFrameInfoCspResolution(&m_pFrameBuf[0]->frame, &frame)) {
        //すべて確保されているか確認
        bool allocated = true;
        for (size_t i = 0; i < m_pFrameBuf.size(); i++) {
            if (m_pFrameBuf[i]->frame.ptrArray[0] == nullptr) {
                allocated = false;
                break;
            }
        }
        if (allocated) {
            return RGY_ERR_NONE;
        }
    }
    m_pFrameBuf.clear();

    for (int i = 0; i < frames; i++) {
        auto uptr = std::make_unique<CUFrameBuf>(frame);
        uptr->releasePtr();
        auto ret = uptr->alloc();
        if (ret != RGY_ERR_NONE) {
            m_pFrameBuf.clear();
            return ret;
        }
        m_pFrameBuf.push_back(std::move(uptr));
    }
    m_nFrameIdx = 0;
    return RGY_ERR_NONE;
}

RGY_ERR NVEncFilter::filter(RGYFrameInfo *pInputFrame, RGYFrameInfo **ppOutputFrames, int *pOutputFrameNum, cudaStream_t stream) {
    if (m_bCheckPerformance) {
        auto cudaerr = cudaEventRecord(*m_peFilterStart.get());
        if (cudaerr != cudaSuccess) {
            AddMessage(RGY_LOG_ERROR, _T("failed cudaEventRecord(m_peFilterStart): %s.\n"), get_err_mes(err_to_rgy(cudaerr)));
        }
    }

    if (pInputFrame == nullptr) {
        *pOutputFrameNum = 0;
        ppOutputFrames[0] = nullptr;
    }
    if (m_pParam
        && m_pParam->bOutOverwrite //上書きか?
        && pInputFrame != nullptr && pInputFrame->ptrArray[0] != nullptr //入力が存在するか?
        && ppOutputFrames != nullptr && ppOutputFrames[0] == nullptr) { //出力先がセット可能か?
        ppOutputFrames[0] = pInputFrame;
        *pOutputFrameNum = 1;
    }
    const auto ret = run_filter(pInputFrame, ppOutputFrames, pOutputFrameNum, stream);
    const int nOutFrame = *pOutputFrameNum;
    if (!m_pParam->bOutOverwrite && nOutFrame > 0) {
        if (m_nPathThrough & FILTER_PATHTHROUGH_TIMESTAMP) {
            if (nOutFrame != 1) {
                AddMessage(RGY_LOG_ERROR, _T("timestamp path through can only be applied to 1-in/1-out filter.\n"));
                return RGY_ERR_INVALID_CALL;
            } else {
                ppOutputFrames[0]->timestamp = pInputFrame->timestamp;
                ppOutputFrames[0]->duration  = pInputFrame->duration;
                ppOutputFrames[0]->inputFrameId = pInputFrame->inputFrameId;
            }
        }
        for (int i = 0; i < nOutFrame; i++) {
            if (m_nPathThrough & FILTER_PATHTHROUGH_FLAGS)     ppOutputFrames[i]->flags     = pInputFrame->flags;
            if (m_nPathThrough & FILTER_PATHTHROUGH_PICSTRUCT) ppOutputFrames[i]->picstruct = pInputFrame->picstruct;
            if (m_nPathThrough & FILTER_PATHTHROUGH_DATA)      ppOutputFrames[i]->dataList  = pInputFrame->dataList;
        }
    }
    if (m_bCheckPerformance) {
        auto cudaerr = cudaEventRecord(*m_peFilterFin.get());
        if (cudaerr != cudaSuccess) {
            AddMessage(RGY_LOG_ERROR, _T("failed cudaEventRecord(m_peFilterFin): %s.\n"), get_err_mes(err_to_rgy(cudaerr)));
        }
        cudaerr = cudaEventSynchronize(*m_peFilterFin.get());
        if (cudaerr != cudaSuccess) {
            AddMessage(RGY_LOG_ERROR, _T("failed cudaEventSynchronize(m_peFilterFin): %s.\n"), get_err_mes(err_to_rgy(cudaerr)));
        }
        float time_ms = 0.0f;
        cudaerr = cudaEventElapsedTime(&time_ms, *m_peFilterStart.get(), *m_peFilterFin.get());
        if (cudaerr != cudaSuccess) {
            AddMessage(RGY_LOG_ERROR, _T("failed cudaEventElapsedTime(m_peFilterStart - m_peFilterFin): %s.\n"), get_err_mes(err_to_rgy(cudaerr)));
        }
        m_dFilterTimeMs += time_ms;
        m_nFilterRunCount++;
    }
    return ret;
}

RGY_ERR NVEncFilter::filter_as_interlaced_pair(const RGYFrameInfo *pInputFrame, RGYFrameInfo *pOutputFrame, cudaStream_t stream) {
    if (!m_pFieldPairIn) {
        auto uptr = std::make_unique<CUFrameBuf>(*pInputFrame);
        uptr->releasePtr();
        uptr->frame.height >>= 1;
        uptr->frame.picstruct = RGY_PICSTRUCT_FRAME;
        uptr->frame.flags &= ~(RGY_FRAME_FLAG_RFF | RGY_FRAME_FLAG_RFF_COPY | RGY_FRAME_FLAG_RFF_TFF | RGY_FRAME_FLAG_RFF_BFF);
        auto ret = uptr->alloc();
        if (ret != RGY_ERR_NONE) {
            m_pFrameBuf.clear();
            return ret;
        }
        m_pFieldPairIn = std::move(uptr);
    }
    if (!m_pFieldPairOut) {
        auto uptr = std::make_unique<CUFrameBuf>(*pOutputFrame);
        uptr->releasePtr();
        uptr->frame.height >>= 1;
        uptr->frame.picstruct = RGY_PICSTRUCT_FRAME;
        uptr->frame.flags &= ~(RGY_FRAME_FLAG_RFF | RGY_FRAME_FLAG_RFF_COPY | RGY_FRAME_FLAG_RFF_TFF | RGY_FRAME_FLAG_RFF_BFF);
        auto ret = uptr->alloc();
        if (ret != RGY_ERR_NONE) {
            m_pFrameBuf.clear();
            return ret;
        }
        m_pFieldPairOut = std::move(uptr);
    }

    for (int i = 0; i < 2; i++) {
        auto err = copyFrameFieldAsync(&m_pFieldPairIn->frame, pInputFrame, i == 0, i == 0, stream);
        if (err != RGY_ERR_NONE) {
            AddMessage(RGY_LOG_ERROR, _T("failed to seprate field(0): %s.\n"), get_err_mes(err));
            return RGY_ERR_CUDA;
        }
        int nFieldOut = 0;
        auto pFieldOut = &m_pFieldPairOut->frame;
        err = run_filter(&m_pFieldPairIn->frame, &pFieldOut, &nFieldOut, stream);
        if (err != RGY_ERR_NONE) {
            return err;
        }
        err = copyFrameFieldAsync(pOutputFrame, pFieldOut, i == 0, i == 0, stream);
        if (err != RGY_ERR_NONE) {
            AddMessage(RGY_LOG_ERROR, _T("failed to merge field(1): %s.\n"), get_err_mes(err));
            return err;
        }
    }
    return RGY_ERR_NONE;
}

void NVEncFilter::CheckPerformance(bool flag) {
    if (flag == m_bCheckPerformance) {
        return;
    }
    m_bCheckPerformance = flag;
    if (!m_bCheckPerformance) {
        m_peFilterStart.reset();
        m_peFilterFin.reset();
    } else {
        auto deleter = [](cudaEvent_t *pEvent) {
            cudaEventDestroy(*pEvent);
            delete pEvent;
        };
        m_peFilterStart = std::unique_ptr<cudaEvent_t, cudaevent_deleter>(new cudaEvent_t(), cudaevent_deleter());
        m_peFilterFin = std::unique_ptr<cudaEvent_t, cudaevent_deleter>(new cudaEvent_t(), cudaevent_deleter());
        auto cudaerr = cudaEventCreate(m_peFilterStart.get());
        if (cudaerr != cudaSuccess) {
            AddMessage(RGY_LOG_ERROR, _T("failed cudaEventCreate(m_peFilterStart): %s.\n"), get_err_mes(err_to_rgy(cudaerr)));
        }
        AddMessage(RGY_LOG_DEBUG, _T("cudaEventCreate(m_peFilterStart)\n"));

        cudaerr = cudaEventCreate(m_peFilterFin.get());
        if (cudaerr != cudaSuccess) {
            AddMessage(RGY_LOG_ERROR, _T("failed cudaEventCreate(m_peFilterFin): %s.\n"), get_err_mes(err_to_rgy(cudaerr)));
        }
        AddMessage(RGY_LOG_DEBUG, _T("cudaEventCreate(m_peFilterFin)\n"));
        m_dFilterTimeMs = 0.0;
        m_nFilterRunCount = 0;
    }
}

double NVEncFilter::GetAvgTimeElapsed() {
    if (!m_bCheckPerformance) {
        return 0.0;
    }
    return m_dFilterTimeMs / (double)m_nFilterRunCount;
}

NVEncFilterParamCrop::NVEncFilterParamCrop() : NVEncFilterParam(), crop(initCrop()), matrix(RGY_MATRIX_ST170_M) {};
NVEncFilterParamCrop::~NVEncFilterParamCrop() {};

bool check_if_nppi_dll_available() {
    HMODULE hModule = RGY_LOAD_LIBRARY(NPPI_DLL_NAME_TSTR);
    if (hModule == NULL)
        return false;
    RGY_FREE_LIBRARY(hModule);
    return true;
}

#if ENABLE_NVRTC
bool check_if_nvrtc_dll_available() {
    HMODULE hModule = RGY_LOAD_LIBRARY(NVRTC_DLL_NAME_TSTR);
    if (hModule == NULL)
        return false;
    RGY_FREE_LIBRARY(hModule);
    return true;
}

bool check_if_nvrtc_builtin_dll_available() {
#if defined(_WIN32) || defined(_WIN64)
    HMODULE hModule = RGY_LOAD_LIBRARY(NVRTC_BUILTIN_DLL_NAME_TSTR);
    if (hModule == NULL)
        return false;
    RGY_FREE_LIBRARY(hModule);
#endif //#if defined(_WIN32) || defined(_WIN64)
    return true;
}
#endif
