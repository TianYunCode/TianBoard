#include "recording/WindowsMediaFile.h"

#include <QtGui>

WindowsMediaFile::WindowsMediaFile(QObject *pParent)
    : QObject(pParent), mWMhDC(0), mWMWriter(0), mPushSink(0), mVideoInputIndex(0), mWMProfile(0), mWMInputVideoProps(0), mWMInputAudioProps(0),
      mWMProfileManager(0), mFramesPerSecond(10) // 与 AbstractVideoEncoder 的默认帧率一致；它是 DWORD，写 -1 会变成 0xFFFFFFFF 还被当除数用
{
    // 刻意留空
}

bool WindowsMediaFile::init(
    const QString &videoFileName, const QString &profileData, int pFramesPerSecond, int pixelWidth, int pixelHeight, int bitsPerPixel)
{
    mFramesPerSecond = pFramesPerSecond;
    mVideoFileName   = videoFileName;

    // 每个写入器自己初始化 COM，close() 里配对反初始化；调用方不需要额外准备 COM。
    CoInitialize(0);

    if (FAILED(WMCreateProfileManager(&mWMProfileManager)))
    {
        setLastErrorMessage("Unable to create a WMProfileManager");
        close();
        return false;
    }

    // 自定义 .prx 是 WM 9 格式，必须先让 profile manager 按 9.0 版本解析。
    IWMProfileManager2 *wmProfileManager = 0;

    if (FAILED(mWMProfileManager->QueryInterface(IID_IWMProfileManager2, (void **)&wmProfileManager)))
    {
        setLastErrorMessage("Unable to query the WMProfileManager for interface WMProfileManager2");
        close();
        return false;
    }

    HRESULT hr = wmProfileManager->SetSystemProfileVersion(WMT_VER_9_0);
    wmProfileManager->Release();

    if (FAILED(hr))
    {
        setLastErrorMessage("Unable to set WMProfileManager SystemProfileVersion");
        close();
        return false;
    }

    if (FAILED(mWMProfileManager->LoadProfileByData((LPCTSTR)profileData.utf16(), &mWMProfile)))
    {
        setLastErrorMessage("Unable to load WMProfileManager custom profile");
        close();
        return false;
    }

    DWORD streamCount = 0; // 纯输出参数，GetStreamCount 会覆盖它；-1 是无符号下的巨大值，没有意义

    if (FAILED(mWMProfile->GetStreamCount(&streamCount)))
    {
        setLastErrorMessage("Unable to read mWMProfile stream count");
        close();
        return false;
    }

    if (FAILED(WMCreateWriter(NULL, &mWMWriter)))
    {
        setLastErrorMessage("Unable to create WMMediaWriter Object");
        close();
        return false;
    }

    if (FAILED(mWMWriter->SetProfile(mWMProfile)))
    {
        setLastErrorMessage("Unable to set WMWriter system profile");
        close();
        return false;
    }

    DWORD mediaInputCount = 0;

    if (FAILED(mWMWriter->GetInputCount(&mediaInputCount)))
    {
        setLastErrorMessage("Unable to get input count for profile");
        close();
        return false;
    }

    // profile 里可能同时声明音视频输入；记下各自索引，写入样本时按索引区分轨道。
    for (DWORD i = 0; i < mediaInputCount; i++)
    {
        IWMInputMediaProps *wmInoutMediaProps = 0;

        if (FAILED(mWMWriter->GetInputProps(i, &wmInoutMediaProps)))
        {
            setLastErrorMessage("Unable to get WMWriter input properties");
            close();
            return false;
        }

        GUID guidInputType;

        if (FAILED(wmInoutMediaProps->GetType(&guidInputType)))
        {
            setLastErrorMessage("Unable to get WMWriter input property type");
            close();
            return false;
        }

        if (guidInputType == WMMEDIATYPE_Video)
        {
            mWMInputVideoProps = wmInoutMediaProps;
            mVideoInputIndex   = i;
        }
        else if (guidInputType == WMMEDIATYPE_Audio)
        {
            mWMInputAudioProps = wmInoutMediaProps;
            mAudioInputIndex   = i;
        }
        else
        {
            wmInoutMediaProps->Release();
            wmInoutMediaProps = 0;
        }
    }

    if (mWMInputVideoProps == 0)
    {
        setLastErrorMessage("Profile does not accept video input");
        close();
        return false;
    }

    if (mWMInputAudioProps == 0)
    {
        setLastErrorMessage("Profile does not accept audio input");
        close();
        return false;
    }

    if (FAILED(mWMWriter->SetOutputFilename((LPCTSTR)videoFileName.utf16())))
    {
        setLastErrorMessage("Unable to set the output filename");
        close();
        return false;
    }

    if (!initVideoStream(pixelWidth, pixelHeight, bitsPerPixel))
    {
        close();
        return false;
    }

    if (FAILED(mWMWriter->BeginWriting()))
    {
        setLastErrorMessage("Unable to initialize video frame writing");
        return false;
    }

    return true;
}

bool WindowsMediaFile::close()
{
    bool result = true;

    if (mWMWriter)
    {
        if (FAILED(mWMWriter->EndWriting()))
        {
            result = false;
        }
    }

    if (mPushSink)
    {
        mPushSink->Disconnect();
        mPushSink->EndSession();
        mPushSink->Release();
    }

    // EndWriting 之后文件才定型，此时用元数据编辑器重新打开，把缓存的章节标记补进去。
    if (mChapters.length() > 0)
    {
        IWMMetadataEditor *editor     = 0;
        IWMHeaderInfo     *headerInfo = 0;

        WMCreateEditor(&editor);
        editor->Open((LPCTSTR)mVideoFileName.utf16());
        editor->QueryInterface(IID_IWMHeaderInfo, (void **)&headerInfo);

        foreach (MarkerInfo mi, mChapters)
        {
            headerInfo->AddMarker((LPWSTR)mi.label.utf16(), msToSampleTime(mi.timestamp));
        }

        editor->Flush();
        editor->Close();
        headerInfo->Release();
        editor->Release();
    }

    releaseWMObjects();

    CoUninitialize();

    return result;
}

WindowsMediaFile::~WindowsMediaFile()
{
    // 刻意留空
}

void WindowsMediaFile::releaseWMObjects()
{
    if (mWMInputVideoProps)
    {
        mWMInputVideoProps->Release();
        mWMInputVideoProps = 0;
    }

    if (mWMInputAudioProps)
    {
        mWMInputAudioProps->Release();
        mWMInputAudioProps = 0;
    }

    if (mWMWriter)
    {
        mWMWriter->Release();
        mWMWriter = 0;
    }

    if (mWMProfile)
    {
        mWMProfile->Release();
        mWMProfile = 0;
    }

    if (mWMProfileManager)
    {
        mWMProfileManager->Release();
        mWMProfileManager = 0;
    }

    if (mWMhDC)
    {
        DeleteDC(mWMhDC);
        mWMhDC = 0;
    }
}

bool WindowsMediaFile::initVideoStream(int nFrameWidth, int nFrameHeight, int nBitsPerPixel)
{
    BITMAPINFO bmpInfo;

    mWMhDC = CreateCompatibleDC(NULL);

    if (mWMhDC == NULL)
    {
        setLastErrorMessage("Unable to create a device context");
        return false;
    }

    ZeroMemory(&bmpInfo, sizeof(BITMAPINFO));
    bmpInfo.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmpInfo.bmiHeader.biBitCount    = nBitsPerPixel;
    bmpInfo.bmiHeader.biWidth       = nFrameWidth;
    // 高度取负表示 top-down DIB：首行就是画面顶部，与 QImage 的内存行序一致，无需再翻转。
    bmpInfo.bmiHeader.biHeight      = -nFrameHeight;
    bmpInfo.bmiHeader.biPlanes      = 1;
    bmpInfo.bmiHeader.biSizeImage   = nFrameWidth * nFrameHeight * nBitsPerPixel / 8;
    bmpInfo.bmiHeader.biCompression = BI_RGB;

    WMVIDEOINFOHEADER videoInfo;
    videoInfo.rcSource.left   = 0;
    videoInfo.rcSource.top    = 0;
    videoInfo.rcSource.right  = nFrameWidth;
    videoInfo.rcSource.bottom = nFrameHeight;
    videoInfo.rcTarget.left   = 0;
    videoInfo.rcTarget.top    = 0;
    videoInfo.rcTarget.right  = nFrameWidth;
    videoInfo.rcTarget.bottom = nFrameHeight;
    videoInfo.dwBitRate       = (nFrameWidth * nFrameHeight * bmpInfo.bmiHeader.biBitCount * mFramesPerSecond);

    videoInfo.dwBitErrorRate  = 0;
    // WM 的时间单位是 100 ns，每帧时长 = 10^7 / 帧率。
    videoInfo.AvgTimePerFrame = 10000000 / mFramesPerSecond;
    memcpy(&(videoInfo.bmiHeader), &bmpInfo.bmiHeader, sizeof(BITMAPINFOHEADER));

    WM_MEDIA_TYPE mt;
    mt.majortype = WMMEDIATYPE_Video;

    if (bmpInfo.bmiHeader.biCompression == BI_RGB)
    {
        if (bmpInfo.bmiHeader.biBitCount == 32)
        {
            mt.subtype = WMMEDIASUBTYPE_RGB32;
        }
        else if (bmpInfo.bmiHeader.biBitCount == 24)
        {
            mt.subtype = WMMEDIASUBTYPE_RGB24;
        }
        else if (bmpInfo.bmiHeader.biBitCount == 16)
        {
            mt.subtype = WMMEDIASUBTYPE_RGB555;
        }
        else if (bmpInfo.bmiHeader.biBitCount == 8)
        {
            mt.subtype = WMMEDIASUBTYPE_RGB8;
        }
        else
        {
            mt.subtype = GUID_NULL;
        }
    }

    mt.bFixedSizeSamples    = false;
    mt.bTemporalCompression = false;
    mt.lSampleSize          = 0;
    mt.formattype           = WMFORMAT_VideoInfo;
    mt.pUnk                 = NULL;
    mt.cbFormat             = sizeof(WMVIDEOINFOHEADER);
    mt.pbFormat             = (BYTE *)&videoInfo;

    if (FAILED(mWMInputVideoProps->SetMediaType(&mt)))
    {
        setLastErrorMessage("Unable to set the  video media type");
        return false;
    }

    if (FAILED(mWMWriter->SetInputProps(mVideoInputIndex, mWMInputVideoProps)))
    {
        setLastErrorMessage("Unable to set the input properties for WMMediaWriter");
        return false;
    }

    return true;
}

bool WindowsMediaFile::appendVideoFrame(const QImage &pImage, long mstimestamp)
{
    Q_ASSERT(pImage.format() == QImage::Format_RGB32);

    INSSBuffer *sampleBuffer = 0;

    if (FAILED(mWMWriter->AllocateSample(static_cast<quint32>(pImage.sizeInBytes()), &sampleBuffer)))
    {
        setLastErrorMessage("Unable to allocate memory for new video frame");
        return false;
    }

    BYTE *rawBuffer       = 0;
    DWORD rawBufferLength = 0;

    if (FAILED(sampleBuffer->GetBufferAndLength(&rawBuffer, &rawBufferLength)))
    {
        setLastErrorMessage("Unable to lock video frame buffer");
        sampleBuffer->Release();
        return false;
    }

    // 输入媒体类型按 RGB32 配置，QImage 也由控制器统一分配成 RGB32，可直接整块拷贝。
    const uchar *imageBuffer = pImage.bits();
    memcpy((void *)rawBuffer, imageBuffer, static_cast<quint32>(pImage.sizeInBytes()));

    HRESULT hrWriteSample = mWMWriter->WriteSample(mVideoInputIndex, msToSampleTime(mstimestamp), 0, sampleBuffer);

    sampleBuffer->Release();

    if (FAILED(hrWriteSample))
    {
        setLastErrorMessage("Unable to write video frame");
        return false;
    }

    mLastErrorMessage = "";

    return true;
}

void WindowsMediaFile::appendAudioBuffer(const QByteArray &audioData, long mstimestamp)
{
    INSSBuffer *sampleBuffer = 0;

    if (FAILED(mWMWriter->AllocateSample(static_cast<DWORD>(audioData.size()), &sampleBuffer)))
    {
        setLastErrorMessage("Unable to allocate memory for new audio frame");
        return;
    }

    BYTE *rawBuffer       = 0;
    DWORD rawBufferLength = 0;

    if (FAILED(sampleBuffer->GetBufferAndLength(&rawBuffer, &rawBufferLength)))
    {
        setLastErrorMessage("Unable to lock audio frame buffer");
        sampleBuffer->Release();
        return;
    }

    // 样本缓冲区由 writer 按 profile 分配，长度可能小于这段 PCM，只能拷贝两者中较小的一段。
    const DWORD copyLength = qMin<DWORD>(rawBufferLength, static_cast<DWORD>(audioData.size()));
    memcpy((void *)rawBuffer, audioData.constData(), copyLength);

    HRESULT hrWriteSample = mWMWriter->WriteSample(mAudioInputIndex, msToSampleTime(mstimestamp), 0, sampleBuffer);

    sampleBuffer->Release();

    if (FAILED(hrWriteSample))
    {
        setLastErrorMessage("Unable to write audio buffer");
        return;
    }

    mLastErrorMessage = "";

    return;
}

void WindowsMediaFile::startNewChapter(const QString &pLabel, long timestamp)
{
    // 录制中途只记录，等 close() 结束写入后再统一落到文件头。
    MarkerInfo mi;
    mi.label     = pLabel;
    mi.timestamp = timestamp;

    mChapters << mi;
}

void WindowsMediaFile::setLastErrorMessage(const QString &error)
{
    mLastErrorMessage = error;
    qWarning() << error;
}
