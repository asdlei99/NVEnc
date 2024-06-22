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

#include <array>
#include <numeric>
#include "convert_csp.h"
#include "NVEncFilter.h"
#include "NVEncFilterParam.h"
#include "NVEncFilterNGX.h"
#include "rgy_device.h"
#include <cuda_d3d11_interop.h>

CUDADX11Texture::CUDADX11Texture() :
    pTexture(nullptr),
    pSRView(nullptr),
    cudaResource(nullptr),
    cuArray(nullptr),
    width(0),
    height(0),
    offsetInShader(0) {
}

RGY_ERR CUDADX11Texture::create(ID3D11Device* pD3DDevice, ID3D11DeviceContext* pD3DDeviceCtx, const int w, const int h, const DXGI_FORMAT dxgiformat) {
    this->width = w;
    this->height = h;

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = dxgiformat;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    if (FAILED(pD3DDevice->CreateTexture2D(&desc, NULL, &pTexture))) {
        return RGY_ERR_NULL_PTR;
    }

    if (FAILED(pD3DDevice->CreateShaderResourceView(pTexture, NULL, &pSRView))) {
        return RGY_ERR_NULL_PTR;
    }

    offsetInShader = 0;  // to be clean we should look for the offset from the shader code
    pD3DDeviceCtx->PSSetShaderResources(offsetInShader, 1, &pSRView);
    return RGY_ERR_NONE;
}

RGY_ERR CUDADX11Texture::registerTexture() {
    auto sts = err_to_rgy(cudaGraphicsD3D11RegisterResource(&cudaResource, pTexture, cudaGraphicsRegisterFlagsNone));
    if (sts != RGY_ERR_NONE) {
        return sts;
    }
    return RGY_ERR_NONE;
}

RGY_ERR CUDADX11Texture::map() {
    auto sts = err_to_rgy(cudaGraphicsMapResources(1, &cudaResource, 0));
    if (sts != RGY_ERR_NONE) {
        return sts;
    }
    return RGY_ERR_NONE;
}

RGY_ERR CUDADX11Texture::unmap() {
    auto sts = RGY_ERR_NONE;
    if (cuArray) {
        sts = err_to_rgy(cudaGraphicsUnmapResources(1, &cudaResource, 0));
        cuArray = nullptr;
    }
    return sts;
}

cudaArray *CUDADX11Texture::getMappedArray() {
    if (cuArray == nullptr) {
        cudaGraphicsSubResourceGetMappedArray(&cuArray, cudaResource, 0, 0);
    }
    return cuArray;
}

RGY_ERR CUDADX11Texture::unregisterTexture() {
    auto sts = unmap();
    if (sts != RGY_ERR_NONE) {
        return sts;
    }
    if (cudaResource) {
        sts = err_to_rgy(cudaGraphicsUnregisterResource(cudaResource));
        cudaResource = nullptr;
    }
    return sts;
}

RGY_ERR CUDADX11Texture::release() {
    unregisterTexture();
    if (pSRView) {
        pSRView->Release();
        pSRView = nullptr;
    }
    if (pTexture) {
        pTexture->Release();
        pTexture = nullptr;
    }
    return RGY_ERR_NONE;
}

NVEncNVSDKNGXFuncs::NVEncNVSDKNGXFuncs() :
    hModule(),
    fcreate(nullptr),
    finit(nullptr),
    fdelete(nullptr),
    fprocFrame(nullptr) {
}
NVEncNVSDKNGXFuncs::~NVEncNVSDKNGXFuncs() {
    close();
}

RGY_ERR NVEncNVSDKNGXFuncs::load() {
    hModule = RGY_LOAD_LIBRARY(NVENC_NVSDKNGX_MODULENAME);
    if (!hModule) {
        return RGY_ERR_NULL_PTR;
    }

#define LOAD_PROC(proc, procName) { \
    proc = (decltype(procName) *)RGY_GET_PROC_ADDRESS(hModule, #procName); \
    if (!proc) { \
        close(); \
        return RGY_ERR_NULL_PTR; \
    } \
}

    LOAD_PROC(fcreate,           NVEncNVSDKNGXCreate);
    LOAD_PROC(finit,             NVEncNVSDKNGXInit);
    LOAD_PROC(fdelete,           NVEncNVSDKNGXDelete);
    LOAD_PROC(fprocFrame,        NVEncNVSDKNGXProcFrame);
#undef LOAD_PROC
    return RGY_ERR_NONE;
}
void NVEncNVSDKNGXFuncs::close() {
    if (hModule) {
        RGY_FREE_LIBRARY(hModule);
        hModule = nullptr;
    }
}

tstring NVEncFilterParamNGXVSR::print() const {
    return ngxvsr.print();
}

NVEncFilterNGX::NVEncFilterNGX() :
    m_func(),
    m_nvsdkNGX(unique_nvsdkngx_handle(nullptr, nullptr)),
    m_ngxCspIn(RGY_CSP_NA),
    m_ngxCspOut(RGY_CSP_NA),
    m_ngxFrameBufOut(),
    m_ngxTextIn(),
    m_ngxTextOut(),
    m_srcCrop(),
    m_dstCrop(),
    m_inputFrames(0),
    m_dx11(nullptr) {
    m_name = _T("nvsdk-ngx");
}
NVEncFilterNGX::~NVEncFilterNGX() {
    close();
}

RGY_ERR NVEncFilterNGX::initNGX(shared_ptr<NVEncFilterParam> pParam, const NVEncNVSDKNGXFeature feature, shared_ptr<RGYLog> pPrintMes) {
    RGY_ERR sts = RGY_ERR_NONE;
    m_pLog = pPrintMes;
#if !ENABLE_NVSDKNGX
    AddMessage(RGY_LOG_ERROR, _T("nv optical flow filters are not supported on x86 exec file, please use x64 exec file.\n"));
    return RGY_ERR_UNSUPPORTED;
#else
    auto prm = dynamic_cast<NVEncFilterParamNGX*>(pParam.get());
    if (!prm) {
        AddMessage(RGY_LOG_ERROR, _T("Invalid parameter type.\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (prm->compute_capability.first < 7) {
        AddMessage(RGY_LOG_ERROR, _T("NVSDK NGX filters require Turing GPUs (CC:7.0) or later: current CC %d.%d.\n"), prm->compute_capability.first, prm->compute_capability.second);
        return RGY_ERR_UNSUPPORTED;
    }
    AddMessage(RGY_LOG_DEBUG, _T("GPU CC: %d.%d.\n"),
        prm->compute_capability.first, prm->compute_capability.second);

    m_dx11 = prm->dx11;

    if (!m_func) {
        m_func = std::make_unique<NVEncNVSDKNGXFuncs>();
        if ((sts = m_func->load()) != RGY_ERR_NONE) {
            AddMessage(RGY_LOG_ERROR, _T("Cannot load dll %s: %s.\n"), NVENC_NVSDKNGX_MODULENAME, get_err_mes(sts));
            return RGY_ERR_NULL_PTR;
        }
        AddMessage(RGY_LOG_DEBUG, _T("Loaded dll %s.\n"), NVENC_NVSDKNGX_MODULENAME);

        NVEncNVSDKNGXHandle ngxHandle = nullptr;
        if ((sts = m_func->fcreate(&ngxHandle, feature)) != RGY_ERR_NONE) {
            AddMessage(RGY_LOG_ERROR, _T("Failed to create NVSDK NGX handle: %s.\n"), get_err_mes(sts));
            // RGY_LOAD_LIBRARYを使用して、nvngx_vsr.dll がロードできるかを確認する
            // できなければ、エラーを返す
            auto hModule = RGY_LOAD_LIBRARY(NVENC_NVSDKNGX_DLL_NAME[feature]);
            if (hModule) {
                RGY_FREE_LIBRARY(hModule);
                hModule = nullptr;
            } else {
                AddMessage(RGY_LOG_ERROR, _T("%s is required but not found.\n"), NVENC_NVSDKNGX_DLL_NAME[feature]);
            }
            return sts;
        }
        m_nvsdkNGX = unique_nvsdkngx_handle(ngxHandle, m_func->fdelete);

        if ((sts = m_func->finit(m_nvsdkNGX.get(), m_dx11->GetDevice(), m_dx11->GetDeviceContext())) != RGY_ERR_NONE) {
            AddMessage(RGY_LOG_ERROR, _T("Failed to init NVSDK NGX library: %s.\n"), get_err_mes(sts));
            return sts;
        }
        AddMessage(RGY_LOG_DEBUG, _T("Initialized NVSDK NGX library %s: %s.\n"), get_err_mes(sts));
    }
    return sts;
#endif
}

RGY_ERR NVEncFilterNGXVSR::run_filter(const RGYFrameInfo *pInputFrame, RGYFrameInfo **ppOutputFrames, int *pOutputFrameNum, cudaStream_t stream) {
    RGY_ERR sts = RGY_ERR_NONE;
    if (pInputFrame->ptr[0] == nullptr) {
        return sts;
    }
    auto prm = dynamic_cast<NVEncFilterParamNGXVSR*>(m_param.get());
    if (!prm) {
        AddMessage(RGY_LOG_ERROR, _T("Invalid parameter type.\n"));
        return RGY_ERR_INVALID_PARAM;
    }

    //const auto memcpyKind = getCudaMemcpyKind(pInputFrame->mem_type, ppOutputFrames[0]->mem_type);
    //if (memcpyKind != cudaMemcpyDeviceToDevice) {
    //    AddMessage(RGY_LOG_ERROR, _T("only supported on device memory.\n"));
    //    return RGY_ERR_INVALID_PARAM;
    //}
    if (m_param->frameOut.csp != m_param->frameIn.csp) {
        AddMessage(RGY_LOG_ERROR, _T("csp does not match.\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    
    RGYFrameInfo *ngxFrameBufIn = nullptr;
    { // pInputFrame -> ngxFrameBufIn
        int cropFilterOutputNum = 0;
        RGYFrameInfo *outInfo[1] = { nullptr };
        RGYFrameInfo cropInput = *pInputFrame;
        auto sts_filter = m_srcCrop->filter(&cropInput, (RGYFrameInfo **)&outInfo, &cropFilterOutputNum, stream);
        ngxFrameBufIn = outInfo[0];
        if (ngxFrameBufIn == nullptr || cropFilterOutputNum != 1) {
            AddMessage(RGY_LOG_ERROR, _T("Unknown behavior \"%s\".\n"), m_srcCrop->name().c_str());
            return sts_filter;
        }
        if (sts_filter != RGY_ERR_NONE || cropFilterOutputNum != 1) {
            AddMessage(RGY_LOG_ERROR, _T("Error while running filter \"%s\".\n"), m_srcCrop->name().c_str());
            return sts_filter;
        }
        copyFramePropWithoutRes(ngxFrameBufIn, pInputFrame);
    }
    //mapで同期がかかる
    m_ngxTextIn->map();
    // ngxFrameBufIn -> m_ngxTextIn
    {
        if (RGY_CSP_PLANES[ngxFrameBufIn->csp] != 1) {
            AddMessage(RGY_LOG_ERROR, _T("unsupported csp, ngxFrameBufIn csp must have only 1 plane.\n"));
            return RGY_ERR_UNSUPPORTED;
        }
        sts = err_to_rgy(cudaMemcpy2DToArray(
            m_ngxTextIn->getMappedArray(), 0, 0,
            (uint8_t *)ngxFrameBufIn->ptr[0], ngxFrameBufIn->pitch[0],
            ngxFrameBufIn->width * RGY_CSP_BIT_DEPTH[ngxFrameBufIn->csp] * 4 / 8, ngxFrameBufIn->height,
            cudaMemcpyDeviceToDevice));
        if (sts != RGY_ERR_NONE) {
            AddMessage(RGY_LOG_ERROR, _T("Failed to copy frame to cudaArray: %s.\n"), get_err_mes(sts));
            return sts;
        }
    }
    m_ngxTextIn->unmap();
    // フィルタを適用
    const NVEncNVSDKNGXParamVSR paramVSR = { prm->ngxvsr.quality };
    const NVEncNVSDKNGXRect rectDst = { 0, 0, m_ngxTextOut->width, m_ngxTextOut->height };
    const NVEncNVSDKNGXRect rectSrc = { 0, 0, m_ngxTextIn->width, m_ngxTextIn->height };
    sts = m_func->fprocFrame(m_nvsdkNGX.get(),
        m_ngxTextOut->pTexture, &rectDst,
        m_ngxTextIn->pTexture, &rectSrc,
        (NVEncNVSDKNGXParam *)&paramVSR);

    //mapで同期がかかる
    m_ngxTextOut->map();
    // m_ngxTextOut -> ngxFrameBufOut
    {
        if (RGY_CSP_PLANES[m_ngxFrameBufOut->frame.csp] != 1) {
            AddMessage(RGY_LOG_ERROR, _T("unsupported csp, ngxFrameBufOut csp must have only 1 plane.\n"));
            return RGY_ERR_UNSUPPORTED;
        }
        sts = err_to_rgy(cudaMemcpy2DFromArray(
            (uint8_t *)m_ngxFrameBufOut->frame.ptr[0], m_ngxFrameBufOut->frame.pitch[0],
            m_ngxTextOut->getMappedArray(), 0, 0,
            m_ngxFrameBufOut->frame.width * RGY_CSP_BIT_DEPTH[m_ngxFrameBufOut->frame.csp] * 4 / 8, m_ngxFrameBufOut->frame.height,
            cudaMemcpyDeviceToDevice));
        if (sts != RGY_ERR_NONE) {
            AddMessage(RGY_LOG_ERROR, _T("Failed to copy frame from cudaArray: %s.\n"), get_err_mes(sts));
            return sts;
        }
    }
    m_ngxTextOut->unmap();
    { // m_ngxFrameBufOut -> ppOutputFrames[0]
        *pOutputFrameNum = 1;
        if (ppOutputFrames[0] == nullptr) {
            ppOutputFrames[0] = &m_frameBuf[0]->frame;
        }
        int cropFilterOutputNum = 0;
        RGYFrameInfo *outInfo[1] = { ppOutputFrames[0] };
        auto sts_filter = m_dstCrop->filter(&m_ngxFrameBufOut->frame, (RGYFrameInfo **)&outInfo, &cropFilterOutputNum, stream);
        if (ppOutputFrames[0] == nullptr || cropFilterOutputNum != 1) {
            AddMessage(RGY_LOG_ERROR, _T("Unknown behavior \"%s\".\n"), m_srcCrop->name().c_str());
            return sts_filter;
        }
        if (sts_filter != RGY_ERR_NONE || cropFilterOutputNum != 1) {
            AddMessage(RGY_LOG_ERROR, _T("Error while running filter \"%s\".\n"), m_srcCrop->name().c_str());
            return sts_filter;
        }
        copyFramePropWithoutRes(ppOutputFrames[0], &m_ngxFrameBufOut->frame);
    }
    return RGY_ERR_NONE;
}

void NVEncFilterNGX::close() {
    m_srcCrop.reset();
    m_dstCrop.reset();
    m_ngxFrameBufOut.reset();
    m_nvsdkNGX.reset();
    m_func.reset();
    m_frameBuf.clear();
    m_dx11 = nullptr;
}

NVEncFilterNGXVSR::NVEncFilterNGXVSR() : NVEncFilterNGX() {
    m_name = _T("nvngx-vsr");
}


NVEncFilterNGXVSR::~NVEncFilterNGXVSR() {
}

RGY_ERR NVEncFilterNGXVSR::checkParam(const NVEncFilterParam *param) {
    auto prm = dynamic_cast<NVEncFilterParamNGXVSR*>(m_param.get());
    if (!prm) {
        AddMessage(RGY_LOG_ERROR, _T("Invalid parameter type.\n"));
        return RGY_ERR_INVALID_PARAM;
    }
    if (prm->ngxvsr.quality < 0 || 4 < prm->ngxvsr.quality) {
        AddMessage(RGY_LOG_ERROR, _T("Invalid quality value %d, must be in the range of 0 to 4.\n"), prm->ngxvsr.quality);
        return RGY_ERR_INVALID_PARAM;
    }
    return RGY_ERR_NONE;
}

RGY_ERR NVEncFilterNGXVSR::init(shared_ptr<NVEncFilterParam> pParam, shared_ptr<RGYLog> pPrintMes) {
    RGY_ERR sts = RGY_ERR_NONE;
#if !ENABLE_NVSDKNGX
    AddMessage(RGY_LOG_ERROR, _T("nv optical flow filters are not supported on x86 exec file, please use x64 exec file.\n"));
    return RGY_ERR_UNSUPPORTED;
#else

    sts = checkParam(pParam.get());
    if (sts != RGY_ERR_NONE) {
        return sts;
    }

    sts = initNGX(pParam, NVSDK_NVX_VSR, pPrintMes);
    if (sts != RGY_ERR_NONE) {
        return sts;
    }

    m_ngxCspIn = RGY_CSP_RGB32;
    m_ngxCspOut = RGY_CSP_RGB32;
    if (!m_srcCrop
        || m_srcCrop->GetFilterParam()->frameIn.width  != pParam->frameIn.width
        || m_srcCrop->GetFilterParam()->frameIn.height != pParam->frameIn.height) {
        AddMessage(RGY_LOG_DEBUG, _T("Create input csp conversion filter.\n"));
        unique_ptr<NVEncFilterCspCrop> filter(new NVEncFilterCspCrop());
        shared_ptr<NVEncFilterParamCrop> paramCrop(new NVEncFilterParamCrop());
        paramCrop->frameIn = pParam->frameIn;
        paramCrop->frameOut = paramCrop->frameIn;
        paramCrop->frameOut.csp = m_ngxCspIn;
        paramCrop->baseFps = pParam->baseFps;
        paramCrop->frameIn.mem_type = RGY_MEM_TYPE_GPU;
        paramCrop->frameOut.mem_type = RGY_MEM_TYPE_GPU;
        paramCrop->bOutOverwrite = false;
        sts = filter->init(paramCrop, m_pLog);
        if (sts != RGY_ERR_NONE) {
            return sts;
        }
        m_srcCrop = std::move(filter);
        AddMessage(RGY_LOG_DEBUG, _T("created %s.\n"), m_srcCrop->GetInputMessage().c_str());
    }
    if (!m_ngxTextIn
        || m_ngxTextIn->width != pParam->frameIn.width
        || m_ngxTextIn->height != pParam->frameIn.height) {
        m_ngxTextIn = std::make_unique<CUDADX11Texture>();
        sts = m_ngxTextIn->create(m_dx11->GetDevice(), m_dx11->GetDeviceContext(), pParam->frameIn.width, pParam->frameIn.height, DXGI_FORMAT_R8G8B8A8_UNORM);
        if (sts != RGY_ERR_NONE) {
            AddMessage(RGY_LOG_DEBUG, _T("failed to create input texture: %s.\n"), get_err_mes(sts));
            return sts;
        }
        sts = m_ngxTextIn->registerTexture();
        if (sts != RGY_ERR_NONE) {
            AddMessage(RGY_LOG_DEBUG, _T("failed to register input texture: %s.\n"), get_err_mes(sts));
            return sts;
        }
    }

    if (!m_ngxTextOut
        || m_ngxTextOut->width != pParam->frameOut.width
        || m_ngxTextOut->height != pParam->frameOut.height) {
        m_ngxTextOut = std::make_unique<CUDADX11Texture>();
        sts = m_ngxTextOut->create(m_dx11->GetDevice(), m_dx11->GetDeviceContext(), pParam->frameOut.width, pParam->frameOut.height, DXGI_FORMAT_R8G8B8A8_UNORM);
        if (sts != RGY_ERR_NONE) {
            AddMessage(RGY_LOG_DEBUG, _T("failed to create output texture: %s.\n"), get_err_mes(sts));
            return sts;
        }
        sts = m_ngxTextOut->registerTexture();
        if (sts != RGY_ERR_NONE) {
            AddMessage(RGY_LOG_DEBUG, _T("failed to register output texture: %s.\n"), get_err_mes(sts));
            return sts;
        }
    }

    if (!m_dstCrop
        || m_dstCrop->GetFilterParam()->frameOut.width  != pParam->frameOut.width
        || m_dstCrop->GetFilterParam()->frameOut.height != pParam->frameOut.height) {
        AddMessage(RGY_LOG_DEBUG, _T("Create output csp conversion filter.\n"));
        unique_ptr<NVEncFilterCspCrop> filter(new NVEncFilterCspCrop());
        shared_ptr<NVEncFilterParamCrop> paramCrop(new NVEncFilterParamCrop());
        paramCrop->frameIn = pParam->frameOut;
        paramCrop->frameIn.csp = m_ngxCspOut;
        paramCrop->frameOut = pParam->frameOut;
        paramCrop->baseFps = pParam->baseFps;
        paramCrop->frameIn.mem_type = RGY_MEM_TYPE_GPU;
        paramCrop->frameOut.mem_type = RGY_MEM_TYPE_GPU;
        paramCrop->bOutOverwrite = false;
        sts = filter->init(paramCrop, m_pLog);
        if (sts != RGY_ERR_NONE) {
            return sts;
        }
        m_dstCrop = std::move(filter);
        AddMessage(RGY_LOG_DEBUG, _T("created %s.\n"), m_dstCrop->GetInputMessage().c_str());

        m_ngxFrameBufOut = std::make_unique<CUFrameBuf>();
        sts = m_ngxFrameBufOut->alloc(m_dstCrop->GetFilterParam()->frameOut.width, m_dstCrop->GetFilterParam()->frameOut.height, m_ngxCspOut);
        if (sts != RGY_ERR_NONE) {
            AddMessage(RGY_LOG_DEBUG, _T("failed to allocate memory for ngx output: %s.\n"), get_err_mes(sts));
            return sts;
        }
    }

    if (m_frameBuf.size() == 0
        || !cmpFrameInfoCspResolution(&m_frameBuf[0]->frame, &pParam->frameOut)) {
        sts = AllocFrameBuf(pParam->frameOut, 2);
        if (sts != RGY_ERR_NONE) {
            AddMessage(RGY_LOG_ERROR, _T("failed to allocate memory: %s.\n"), get_err_mes(sts));
            return RGY_ERR_MEMORY_ALLOC;
        }
    }
    for (int i = 0; i < RGY_CSP_PLANES[pParam->frameOut.csp]; i++) {
        pParam->frameOut.pitch[i] = m_frameBuf[0]->frame.pitch[i];
    }
    tstring info = m_name + _T(": ");
    if (m_srcCrop) {
        info += m_srcCrop->GetInputMessage() + _T("\n");
    }
    tstring nameBlank(m_name.length() + _tcslen(_T(": ")), _T(' '));
    info += tstring(INFO_INDENT) + nameBlank + pParam->print();
    if (m_dstCrop) {
        info += tstring(_T("\n")) + tstring(INFO_INDENT) + nameBlank + m_dstCrop->GetInputMessage();
    }
    setFilterInfo(info);
    m_param = pParam;
    return sts;
#endif
}