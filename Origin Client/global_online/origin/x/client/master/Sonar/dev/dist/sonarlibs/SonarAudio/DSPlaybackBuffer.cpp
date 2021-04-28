#include <SonarAudio/DSPlaybackBuffer.h>
#include <SonarAudio/DSPlaybackDevice.h>

#include <SonarCommon/Common.h>
#include <SonarAudio/dscommon.h>

#include <assert.h>
#include <iostream>
#include <fstream>

std::ofstream dsoundLog;

namespace {
    const size_t numBuckets = 100;
    static unsigned int interPlaybackLatency[numBuckets];
    static unsigned int receptionToPlaybackLatency[numBuckets];

    inline void clearStats() {
        std::memset(interPlaybackLatency, 0, sizeof(interPlaybackLatency));
        std::memset(receptionToPlaybackLatency, 0, sizeof(receptionToPlaybackLatency));
    }
}

namespace sonar
{

DSPlaybackBuffer::DSPlaybackBuffer (DSPlaybackDevice &_device, int _numSamples, int _samplesPerFrame)
    : m_pDSBuffer(NULL)
    , m_lastEnqueue(0)
    , m_shortBuffer(NULL)
{
    clearStats();

    if (common::ENABLE_PLAYBACK_DSOUND_LOGGING) {
        static int index = 0;
        std::ostringstream filename;
        filename << "sonar_playback_dsound_" << index++ << ".log";
        dsoundLog.open(filename.str(), std::ios::out | std::ios::trunc | std::ios::binary);
    }

    m_samplesPerFrame = _samplesPerFrame;
    m_cOverflow = 0;
    WAVEFORMATEX formatEx;
    memset (&formatEx, 0, sizeof (formatEx));
    formatEx.wFormatTag = WAVE_FORMAT_PCM;
    formatEx.nChannels = 1;
    formatEx.nSamplesPerSec = _device.getSamplesPerSec ();
    formatEx.wBitsPerSample = (WORD) _device.getBitsPerSample ();
    formatEx.nBlockAlign = formatEx.wBitsPerSample / 8;
    formatEx.nAvgBytesPerSec = formatEx.nSamplesPerSec * formatEx.nBlockAlign;
    formatEx.cbSize = 0;

    m_totalBufferSize = (formatEx.wBitsPerSample * _numSamples) / 8;
    m_bytesPerFrame = (formatEx.wBitsPerSample * _samplesPerFrame) / 8;
    m_frameCount = _numSamples / _samplesPerFrame;

    DSBUFFERDESC desc;
    memset (&desc, 0, sizeof (desc));

    desc.dwSize = sizeof (DSBUFFERDESC);
    desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
    desc.dwBufferBytes = m_totalBufferSize;
    desc.dwReserved = 0;
    desc.guid3DAlgorithm = DS3DALG_DEFAULT;
    desc.lpwfxFormat = &formatEx;

    if (common::ENABLE_CLIENT_DSOUND_TRACING)
        common::Log("sonar::DSPlaybackBuffer::DSPlaybackBuffer creating sound buffer");

    if (FAILED (internal::lastPlaybackDeviceError = _device.getDS8 ()->CreateSoundBuffer (&desc, &m_pDSBuffer, NULL)))
    {
        if( !errorSent("SPlaybackBuffer", "create", internal::lastPlaybackDeviceError) )
        {
            common::Log("sonar::DSPlaybackBuffer::DSPlaybackBuffer CreateSoundBuffer err=%d", internal::lastPlaybackDeviceError);
            common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SPlaybackBuffer", "create", internal::lastPlaybackDeviceError);
        }
        return;
    }

    void *pBuffer;
    DWORD cBufferBytes;

    if (FAILED (internal::lastPlaybackDeviceError = m_pDSBuffer->Lock (0, desc.dwBufferBytes, &pBuffer, &cBufferBytes, NULL, NULL, 0)))
    {
        if( !errorSent("SPlaybackBuffer", "lock", internal::lastPlaybackDeviceError) )
        {
            common::Log("sonar::DSPlaybackBuffer::DSPlaybackBuffer Lock err=%d", internal::lastPlaybackDeviceError);
            common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SPlaybackBuffer", "lock", internal::lastPlaybackDeviceError);
        }
        return;
    }

    memset (pBuffer, 0, cBufferBytes);

    m_pDSBuffer->Unlock (pBuffer, cBufferBytes, NULL, 0);
    m_nextWriteOffset = 0;

    m_shortBuffer = new short[m_samplesPerFrame];
    m_pDSBuffer->Play (0, 0, DSBPLAY_LOOPING);
}

DSPlaybackBuffer::~DSPlaybackBuffer (void)
{
    if (common::ENABLE_PLAYBACK_DSOUND_LOGGING)
        dsoundLog.close();

    if (m_pDSBuffer) m_pDSBuffer->Release ();
    if (m_shortBuffer) delete[] m_shortBuffer;

    if (common::ENABLE_CLIENT_DSOUND_TRACING)
        common::Log("sonar::DSPlaybackBuffer::~DSPlaybackBuffer releasing playback buffer");
}

void DSPlaybackBuffer::stop(void)
{
    if (m_pDSBuffer)
        m_pDSBuffer->Stop();
}

bool DSPlaybackBuffer::isCreated(void)
{
    return m_pDSBuffer != NULL;
}

int DSPlaybackBuffer::getDiff(void)
{
    DWORD dwWriteCursor;
    DWORD dwPlayCursor;

    if (FAILED (internal::lastPlaybackDeviceError = m_pDSBuffer->GetCurrentPosition (&dwPlayCursor, &dwWriteCursor)))
    {
        if( !errorSent("SPlaybackBuffer", "diff_pos", internal::lastPlaybackDeviceError) )
        {
            common::Log("sonar::DSPlaybackBuffer::getDiff GetCurrentPosition err=%d", internal::lastPlaybackDeviceError);
            common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SPlaybackBuffer", "diff_pos", internal::lastPlaybackDeviceError);
        }
        return -1;
    }

     int diff = 0;
     
     if (m_nextWriteOffset < dwWriteCursor)
     {
         diff = m_totalBufferSize - dwWriteCursor + m_nextWriteOffset;
     }
     else
     {
         diff = (int) m_nextWriteOffset - (int) dwWriteCursor;
     }
 
     if (diff < 0) diff = 0;
 
     return diff;
}

PlaybackResult DSPlaybackBuffer::query(void)
{
    int diff = getDiff();

    if (diff == -1)
    {
        return PR_DeviceLost;
    }

    if (diff < (int)  m_bytesPerFrame * 1)
    { 
        return PR_Underrun;
    }
    else
    if (diff > (int)  m_bytesPerFrame * 2)
    {
        return PR_Overflow;
    }
    
    return PR_Success;
}


void DSPlaybackBuffer::enqueue (const AudioFrame &_frame)
{
    common::FloatToPCMShort (&_frame[0], m_samplesPerFrame, m_shortBuffer);

    // calculate how long it's been since the last enqueue
    UINT64 currtime = common::getTimeAsMSEC();
    if (m_lastEnqueue != 0)
    {
        static unsigned int frameCounter = 0;

        UINT64 diff = (currtime - m_lastEnqueue);
        if (diff >= numBuckets)
            diff = numBuckets - 1;
        ++interPlaybackLatency[diff];

        assert(_frame.timestamp());
        diff = (currtime - _frame.timestamp());
        if (diff >= numBuckets)
            diff = numBuckets - 1;
        ++receptionToPlaybackLatency[diff];

        ++frameCounter;
    }
    m_lastEnqueue = currtime;

    void *pBuffer;
    DWORD cBufferBytes;

    if (FAILED (internal::lastPlaybackDeviceError = m_pDSBuffer->Lock (
        m_nextWriteOffset, 
        m_bytesPerFrame, 
        &pBuffer, 
        &cBufferBytes, 
        NULL, 
        NULL, 
        0)))
    {
        if( !errorSent("SPlaybackBuffer", "enqueue_lock", internal::lastPlaybackDeviceError) )
        {
            common::Log("sonar::DSPlaybackBuffer::enqueue Lock err=%d", internal::lastPlaybackDeviceError);
            common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SPlaybackBuffer", "enqueue_lock", internal::lastPlaybackDeviceError);
        }
        return;
    }
    
    memcpy (pBuffer, m_shortBuffer, cBufferBytes);
    m_pDSBuffer->Unlock (pBuffer, cBufferBytes, NULL, 0);

    if (common::ENABLE_PLAYBACK_DSOUND_LOGGING)
        dsoundLog.write(reinterpret_cast<const char*>(pBuffer), cBufferBytes);

    m_nextWriteOffset += m_bytesPerFrame;
    m_nextWriteOffset %= m_totalBufferSize;
}

void DSPlaybackBuffer::calculateStats(float& deltaPlaybackMean, float& deltaPlaybackMax, float& receiveToPlaybackMean, float& receiveToPlaybackMax) {

    if (common::ENABLE_SONAR_TIMING_LOGGING)
    {
        // dump tab-delimited stats
        std::stringstream stat1;
        std::stringstream stat2;
        std::stringstream stat3;

        stat1 << "ms:";
        stat2 << "playback_to_playback:";
        stat3 << "reception_to_playback:";

        for (auto i = 0; i < numBuckets; ++i)
        {
            stat1 << '\t' << i;
            stat2 << '\t' << interPlaybackLatency[i];
            stat3 << '\t' << receptionToPlaybackLatency[i];
        }

        common::Log("Latency distribution stats");
        common::Log("\n%s\n%s\n%s", stat1.str().c_str(), stat2.str().c_str(), stat3.str().c_str());
    }

    // calculate stats from the inter audio playback latencies
    float count = 0.0;
    float total = 0.0;
    auto max = 0;
    for (auto i(0); i < numBuckets; ++i) {
        count += interPlaybackLatency[i];
        total += interPlaybackLatency[i] * i;
        if (interPlaybackLatency[i])
            max = i;
    }
    deltaPlaybackMean = total / count;
    deltaPlaybackMax = max;

    // calculate stats from the reception to playback latencies
    count = 0.0;
    total = 0.0;
    max = 0;
    for (auto i(0); i < numBuckets; ++i) {
        count += receptionToPlaybackLatency[i];
        total += receptionToPlaybackLatency[i] * i;
        if (receptionToPlaybackLatency[i])
            max = i;
    }
    receiveToPlaybackMean = total / count;
    receiveToPlaybackMax = max;

    clearStats();
}


}