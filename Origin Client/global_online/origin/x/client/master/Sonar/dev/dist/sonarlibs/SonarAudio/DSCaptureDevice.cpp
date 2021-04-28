#include <SonarAudio/DSCaptureDevice.h>
#include <SonarAudio/dscommon.h>
#include <SonarCommon/Common.h>
#include <assert.h>
#include <iostream>
#include <fstream>

#if !defined(ENABLE_DIFF_TRACING)
#define ENABLE_DIFF_TRACING 0
#endif
std::ofstream capturePcmLog;

namespace sonar
{
DSCaptureDevice::DSCaptureDevice (
    AudioRuntime &runtime,
    const AudioDeviceId &_device,
    int _samplesPerSec,
    int _samplesPerFrame,
    int _bitsPerSample,
    int _bufferCount)
    : m_pDSC(NULL)
    , m_pDSCaptureBuffer(NULL)
{
    if (common::ENABLE_CAPTURE_PCM_LOGGING) {
        static int index = 0;
        std::ostringstream filename;
        filename << "sonar_capture_pcm_" << index++ << ".log";
        capturePcmLog.open(filename.str(), std::ios::out | std::ios::trunc | std::ios::binary);
    }

    m_pcmBuffer = new short[_samplesPerFrame];

    GUID guid;
    if (_device.id == "DEFAULT")
    {
        runtime.lookupRealGuid(&DSDEVID_DefaultVoiceCapture, &guid);
    }
    else
    {
        if (!StringToGuid (_device.id, &guid))
        {
            return;
        }
    }
 
  WAVEFORMATEX formatEx;

  memset (&formatEx, 0, sizeof (formatEx));
  
  formatEx.wFormatTag = WAVE_FORMAT_PCM;
  formatEx.nChannels = 1;
  formatEx.nSamplesPerSec = _samplesPerSec;
  formatEx.wBitsPerSample = (WORD) _bitsPerSample;
  formatEx.nBlockAlign = (WORD) _bitsPerSample / 8;
  formatEx.nAvgBytesPerSec = formatEx.nSamplesPerSec * formatEx.nBlockAlign;
  formatEx.cbSize = 0;

    m_samplesPerBuffer = _samplesPerFrame;
    m_bytesPerSample = _bitsPerSample / 8;
    m_bufferCount = _bufferCount;
    m_bytesPerBuffer = m_bytesPerSample * m_samplesPerBuffer;
    m_totalBufferSize = m_bytesPerBuffer * m_bufferCount;
    
    if (common::ENABLE_CLIENT_DSOUND_TRACING)
        common::Log("sonar::DSCaptureDevice::DSCaptureDevice creating capture device");

    // special hook for QA testing to allow them to simulate a DS error
    if (internal::sonarTestingAudioError != 0)
    {
        internal::lastCaptureDeviceError = internal::sonarTestingAudioError;
        common::Log("sonar::DSCaptureDevice::DSCaptureDevice DirectSoundCaptureCreate8 using QA testing error override, err=%d", internal::lastCaptureDeviceError);
        return;
    }

    if (FAILED (internal::lastCaptureDeviceError = DirectSoundCaptureCreate8 (&guid, &m_pDSC, NULL)))
    {
        if (internal::lastCaptureDeviceError == DSERR_NODRIVER) {
            // DSERR_NODRIVER => most likely no microphone, don't bother sending telemetry
            return;
        }

        if( !errorSent("SCaptureDevice", "create1", internal::lastCaptureDeviceError) )
        {
            common::Log("sonar::DSCaptureDevice::DSCaptureDevice DirectSoundCaptureCreate8 err=%d", internal::lastCaptureDeviceError);
            common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SCaptureDevice", "create1", internal::lastCaptureDeviceError);
        }
        return;
    }

    DSCBUFFERDESC desc;
    memset (&desc, 0, sizeof (desc));

    desc.dwSize = sizeof (DSCBUFFERDESC);
    desc.dwFXCount = 0;
    desc.dwFlags = 0;//DSCBCAPS_CTRLFX;
    desc.dwBufferBytes = m_totalBufferSize;
    desc.dwReserved = 0;
    desc.lpwfxFormat = &formatEx;
    //desc.lpDSCFXDesc = effects;
    desc.dwFXCount = 0;//2;


    if (FAILED (internal::lastCaptureDeviceError = m_pDSC->CreateCaptureBuffer (&desc, &m_pDSCaptureBuffer, NULL)))
    {
        if( !errorSent("SCaptureDevice", "create2", internal::lastCaptureDeviceError) )
        {
            common::Log("sonar::DSCaptureDevice::DSCaptureDevice CreateCaptureBuffer err=%d", internal::lastCaptureDeviceError);
            common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SCaptureDevice", "create2", internal::lastCaptureDeviceError);
        }
        m_pDSC->Release();
        m_pDSC = NULL;
        return;
    }

    if (FAILED (internal::lastCaptureDeviceError = m_pDSCaptureBuffer->Start (DSCBSTART_LOOPING)))
    {
        if( !errorSent("SCaptureDevice", "create3", internal::lastCaptureDeviceError) )
        {
            common::Log("sonar::DSCaptureDevice::DSCaptureDevice DSCaptureBuffer Start err=%d", internal::lastCaptureDeviceError);
            common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SCaptureDevice", "create3", internal::lastCaptureDeviceError);
        }

        m_pDSCaptureBuffer->Release();
        m_pDSCaptureBuffer = NULL;

        m_pDSC->Release();
        m_pDSC = NULL;
        return;
    }

    m_nextReadOffset = 0;
}

DSCaptureDevice::~DSCaptureDevice (void)
{
    if (common::ENABLE_CAPTURE_PCM_LOGGING)
        capturePcmLog.close();

    delete[] m_pcmBuffer;

    if (m_pDSCaptureBuffer) m_pDSCaptureBuffer->Release();
    if (m_pDSC) m_pDSC->Release();

    if (common::ENABLE_CLIENT_DSOUND_TRACING)
        common::Log("sonar::DSCaptureDevice::~DSCaptureDevice releasing capture device");
}

bool DSCaptureDevice::isCreated(void)
{
    return (m_pDSCaptureBuffer && m_pDSC);
}

CaptureResult DSCaptureDevice::poll (AudioFrame &_outFrame)
{
    if (m_pDSC == NULL || m_pDSCaptureBuffer == NULL)
    {
        // don't bother sending telemetry here - it's too spammy and we already know that something went wrong
        return CR_DeviceLost;
    }

    DWORD dwCaptureCursor;
    DWORD dwReadCursor;
        
    if (FAILED (internal::lastCaptureDeviceError = m_pDSCaptureBuffer->GetCurrentPosition (&dwCaptureCursor, &dwReadCursor)))
    {
        if( !errorSent("SCaptureDevice", "poll_pos", internal::lastCaptureDeviceError) )
        {
            common::Log("sonar::DSCaptureDevice::poll GetCurrentPosition err=%d", internal::lastCaptureDeviceError);
            common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SCaptureDevice", "poll_pos", internal::lastCaptureDeviceError);
        }
        return CR_DeviceLost;
    }

    int diff;
    diff = abs ( (int) dwReadCursor - (int) m_nextReadOffset);

    if (diff > (int) (m_bytesPerBuffer * (m_bufferCount / 2)))
    {
        diff = m_totalBufferSize - m_nextReadOffset + dwReadCursor;
    }

    int diff2 = dwReadCursor - m_nextReadOffset;
    if (diff2 < 0) {
        diff2 = m_totalBufferSize + diff2;
    }

#if ENABLE_DIFF_TRACING
    std::cout
        << "diff=" << diff
        << ", next= " << m_nextReadOffset
        << ", read= " << dwReadCursor
        << ", capture= " << dwCaptureCursor
        << std::endl;
#endif // ENABLE_DIFF_TRACING

    // wait for at least one frame of data
    if (diff < (int) m_bytesPerBuffer)
    {
        if (common::JITTER_METRICS_LOG_LEVEL > 1)
            printf("capture: diff %lld %d %d\n", common::getTimeAsMSEC(), diff, diff2);
        return CR_NoAudio;
    }

    DWORD bufferASize = 0, bufferBSize = 0;
    void *pBufferA = NULL, *pBufferB = NULL;

    // Lock buffer
    if (FAILED (internal::lastCaptureDeviceError = m_pDSCaptureBuffer->Lock (
        m_nextReadOffset, 
        m_bytesPerBuffer, // lock one frame of data
        &pBufferA, &bufferASize, 
        &pBufferB, &bufferBSize, 0)))
    {
        if( !errorSent("SCaptureDevice", "poll_lock", internal::lastCaptureDeviceError) )
        {
            common::Log("sonar::DSCaptureDevice::poll Lock err=%d", internal::lastCaptureDeviceError);
            common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SCaptureDevice", "poll_lock", internal::lastCaptureDeviceError);
        }
        return CR_DeviceLost;
    }

    BYTE *pOutBuffer = (BYTE *) m_pcmBuffer;

    memcpy (pOutBuffer, pBufferA, bufferASize);

    if (pBufferB)
    {
        memcpy (pOutBuffer + bufferASize, pBufferB, bufferBSize);
    }

    if (FAILED(internal::lastCaptureDeviceError = m_pDSCaptureBuffer->Unlock (pBufferA, bufferASize, pBufferB, bufferBSize)))
    {
        if( !errorSent("SCaptureDevice", "poll_unlock", internal::lastCaptureDeviceError) )
        {
            common::Log("sonar::DSCaptureDevice::poll Unlock err=%d", internal::lastCaptureDeviceError);
            common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SCaptureDevice", "poll_unlock", internal::lastCaptureDeviceError);
        }
        // non-fatal error(?) - keep going
    }

    sonar::INT64 frameCaptureTime = common::getTimeAsMSEC();
    _outFrame.timestamp(frameCaptureTime);

    if (_outFrame.size() != m_samplesPerBuffer)
    {
        _outFrame.resize (m_samplesPerBuffer);
    }

    assert ((bufferASize + bufferBSize) / sizeof(short) == m_samplesPerBuffer);
    common::PCMShortToFloat (m_pcmBuffer, (int) m_samplesPerBuffer, &_outFrame[0]);
    
    if (common::ENABLE_CAPTURE_PCM_LOGGING)
        capturePcmLog.write(reinterpret_cast<const char*>(m_pcmBuffer), m_samplesPerBuffer * sizeof(*m_pcmBuffer));

    m_nextReadOffset += m_bytesPerBuffer;

    if (m_nextReadOffset == m_totalBufferSize)
    {
        m_nextReadOffset = 0;
    }

    if (common::JITTER_METRICS_LOG_LEVEL > 1)
        printf("capture: %lld %d %d\n", frameCaptureTime, bufferASize, bufferBSize);

    return CR_Success;
}

}
