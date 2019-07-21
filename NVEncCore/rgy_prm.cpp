﻿// -----------------------------------------------------------------------------------------
// QSVEnc/NVEnc by rigaya
// -----------------------------------------------------------------------------------------
// The MIT License
//
// Copyright (c) 2011-2016 rigaya
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
// --------------------------------------------------------------------------------------------

#include "rgy_util.h"
#include "rgy_avutil.h"
#include "rgy_prm.h"

RGYParamCommon::RGYParamCommon() :
    inputFilename(),
    outputFilename(),
    sAVMuxOutputFormat(),
    sMaxCll(),
    sMasterDisplay(),
    dynamicHdr10plusJson(),
    videoCodecTag(),
    fSeekSec(0.0f),               //指定された秒数分先頭を飛ばす
    nSubtitleSelectCount(0),
    ppSubtitleSelectList(nullptr),
    nAudioSourceCount(0),
    ppAudioSourceList(nullptr),
    nAudioSelectCount(0), //pAudioSelectの数
    ppAudioSelectList(nullptr),
    nDataSelectCount(0),
    ppDataSelectList(nullptr),
    nAudioResampler(RGY_RESAMPLER_SWR),
    nAVDemuxAnalyzeSec(0),
    nAVMux(RGY_MUX_NONE),                       //RGY_MUX_xxx
    nVideoTrack(0),
    nVideoStreamId(0),
    nTrimCount(0),
    pTrimList(nullptr),
    bCopyChapter(false),
    keyOnChapter(false),
    caption2ass(FORMAT_INVALID),
    nAudioIgnoreDecodeError(DEFAULT_IGNORE_DECODE_ERROR),
    pMuxOpt(nullptr),
    sChapterFile(),
    pAVInputFormat(nullptr),
    nAVSyncMode(RGY_AVSYNC_ASSUME_CFR),     //avsyncの方法 (RGY_AVSYNC_xxx)
    nOutputBufSizeMB(OUTPUT_BUF_SIZE) {

}

RGYParamCommon::~RGYParamCommon() {};

RGYParamControl::RGYParamControl() :
    logfile(),              //ログ出力先
    loglevel(RGY_LOG_INFO),                 //ログ出力レベル
    sFramePosListLog(),     //framePosList出力先
    pMuxVidTsLogFile(nullptr),
    nOutputThread(RGY_OUTPUT_THREAD_AUTO),
    nAudioThread(RGY_INPUT_THREAD_AUTO),
    nInputThread(RGY_AUDIO_THREAD_AUTO),
    nProcSpeedLimit(0),      //処理速度制限 (0で制限なし)
    nPerfMonitorSelect(0),
    nPerfMonitorSelectMatplot(0),
    nPerfMonitorInterval(RGY_DEFAULT_PERF_MONITOR_INTERVAL),
    threadCsp(0),
    simdCsp(-1) {

}
RGYParamControl::~RGYParamControl() {};