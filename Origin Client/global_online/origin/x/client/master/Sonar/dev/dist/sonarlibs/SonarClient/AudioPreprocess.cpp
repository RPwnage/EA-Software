#include "AudioPreprocess.h"
#include "SonarClient/Client.h"
#include <assert.h>
#include <speex/speex.h>
#include <speex/speex_echo.h>
#include <speex/speex_preprocess.h>

namespace sonar
{

struct AudioPreprocess::Private
{
	SpeexPreprocessState *m_preState;
    SpeexEchoState *m_echoState;
};

const size_t EchoQueueSize = 16;

bool IsSampleFrameEmpty(const spx_int16_t* buffer, size_t bufferSize)
{
    for (int idx = 0; idx < bufferSize; ++idx)
    {
        if (buffer[idx] != 0)
            return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////

AudioPreprocess::AudioPreprocess (const Options& opts, int samplesPerFrame, int samplesPerSecond, float destRange)
    : m_echoQueue(NULL), m_echoQueueDelay(0), m_echoQueuePopIndex(0), m_echoQueuePushIndex(0), m_echoStatsQueueMaxSize(0)
{
	m_prv = new Private;
	m_destRange = 16000.0;
	m_framesPerSecond = samplesPerSecond / samplesPerFrame;
	m_samplesPerFrame = samplesPerFrame;

	m_lt.frames = 0;
	m_lt.amp = 0.0f;

	m_st.frames = 0;
	m_st.amp = 0.0f;
	m_auto.gain = 5.0f;
	m_auto.peak = 0.0f;

	m_fadeoff.ptt = 0;
	m_fadeoff.va = 0;

	m_prv->m_preState = speex_preprocess_state_init(samplesPerFrame, samplesPerSecond);
	int denoise = 1;
    speex_preprocess_ctl (m_prv->m_preState, SPEEX_PREPROCESS_SET_DENOISE, &denoise); 
    spx_int32_t agcEnable = opts.speexSettingAgcEnable();
    speex_preprocess_ctl (m_prv->m_preState, SPEEX_PREPROCESS_SET_AGC, &agcEnable); 
    float agcLevel = opts.speexSettingAgcLevel();
    speex_preprocess_ctl (m_prv->m_preState, SPEEX_PREPROCESS_SET_AGC_LEVEL, &agcLevel); 

    m_prv->m_echoState = NULL;
    if (common::ENABLE_SPEEX_ECHO_CANCELLATION)
    {
        // samplesPerFrame = 320samples -> 20ms
        // FilterLength = 1/3 of room reverberation time.A small room rev is in order of 300ms
        // -> filterLength = 100ms -> 5 x samplesPerFrame
        int multiplier = common::SPEEX_ECHO_TAIL_MULTIPLIER;

        m_prv->m_echoState = speex_echo_state_init(samplesPerFrame, multiplier * samplesPerFrame);
        speex_echo_ctl(m_prv->m_echoState, SPEEX_ECHO_SET_SAMPLING_RATE, &samplesPerSecond);
        speex_preprocess_ctl(m_prv->m_preState, SPEEX_PREPROCESS_SET_ECHO_STATE, m_prv->m_echoState);

        m_echoQueue = new spx_int16_t*[EchoQueueSize];
        for (int idx = 0; idx < EchoQueueSize; ++idx)
        {
            m_echoQueue[idx] = new spx_int16_t[samplesPerFrame];
            ZeroMemory(m_echoQueue[idx], sizeof(spx_int16_t) * samplesPerFrame);
        }

        // Setup delay we use before using the echo frame
        m_echoQueueDelay = common::SPEEX_ECHO_QUEUE_DELAY;

        // Prepare to store our stats for a min of conversation
        m_echoStatsQueueMaxSize = m_framesPerSecond * 3600;
        m_echoStats.reserve(m_echoStatsQueueMaxSize);
    }
}

AudioPreprocess::~AudioPreprocess ()
{
    if (m_prv->m_echoState)
    {
        //dumpStats();

        speex_echo_state_destroy(m_prv->m_echoState);

        for (int idx = 0; idx < EchoQueueSize; ++idx)
            delete[] m_echoQueue[idx];

        delete[] m_echoQueue;
    }

    speex_preprocess_state_destroy (m_prv->m_preState);

	delete m_prv;
}

void AudioPreprocess::denoise(AudioFrame &frame)
{
	SonarVector<spx_int16_t> inputFrame(frame.size());
    SonarVector<spx_int16_t> noEchoFrame(frame.size());
    spx_int16_t* outputFrame;

	size_t samples = frame.size();

	for(size_t index = 0; index < samples; index ++)
	{
		inputFrame[index] = (spx_int16_t) frame[index];
	}

    // First setup echo cancellation data (if enabled)
    if (m_prv->m_echoState)
    {
        SonarVector<spx_int16_t> echoFrame(frame.size());
        if (!popEchoFrame(&echoFrame[0], frame.size()))
            ZeroMemory(&echoFrame[0], sizeof(spx_int16_t) * frame.size());

        outputFrame = &noEchoFrame[0];
        speex_echo_cancellation(m_prv->m_echoState, &inputFrame[0], &echoFrame[0], outputFrame);
    }

    else
        outputFrame = &inputFrame[0];

    // Then take care of the rest of the pre-processing
	speex_preprocess_run(m_prv->m_preState, outputFrame);

	for(size_t index = 0; index < samples; index ++)
	{
		frame[index] = outputFrame[index];
	}

}

void AudioPreprocess::pushEchoFrame(const AudioFrame &frame)
{
    // TODO: we will need to protect this with a mutex if we ever run in separate threads
    if (!m_prv || !m_prv->m_echoState)
        return;

    assert(frame.size() == m_samplesPerFrame);
    
    // If we are starting a new sequence, add our default delay to compensate for the acoustic delay without
    // having to change our ehco filter size, which should always be around 100-120ms so that it can converge
    // in time to actually do something!
    if (m_echoQueuePushIndex == m_echoQueuePopIndex)
    {
        for (int delayIdx = 0; delayIdx < m_echoQueueDelay; ++delayIdx)
        {
            int prevPushIndex = m_echoQueuePushIndex;
            ZeroMemory(m_echoQueue[m_echoQueuePushIndex], m_samplesPerFrame * sizeof(spx_int16_t));
            ++m_echoQueuePushIndex;
            if (m_echoQueuePushIndex == EchoQueueSize)
                m_echoQueuePushIndex = 0;

            if (m_echoStats.size() < m_echoStatsQueueMaxSize)
            {
                EchoStatsEntry stats;
                stats.timestamp = common::getTimeAsMSEC();
                stats.popIndex = m_echoQueuePopIndex;
                stats.pushIndex = prevPushIndex;
                stats.opType = EchoStatsEntry::OpType_PUSH;
                stats.delayEntry = true;
                stats.fullQueue = m_echoQueuePushIndex == m_echoQueuePopIndex;
                stats.emptyQueue = false;
                stats.emptySampleFrame = true;

                m_echoStats.push_back(stats);
            }
        }
    }

    int prevPushIndex = m_echoQueuePushIndex;
    size_t cnt = min(frame.size(), m_samplesPerFrame);
    common::FloatToPCMShort(&frame[0], cnt, m_echoQueue[m_echoQueuePushIndex]);

    ++m_echoQueuePushIndex;
    if (m_echoQueuePushIndex == EchoQueueSize)
        m_echoQueuePushIndex = 0;

    assert (m_echoQueuePushIndex != m_echoQueuePopIndex);
    if (m_echoStats.size() < m_echoStatsQueueMaxSize)
    {
        EchoStatsEntry stats;
        stats.timestamp = common::getTimeAsMSEC();
        stats.popIndex = m_echoQueuePopIndex;
        stats.pushIndex = prevPushIndex;
        stats.opType = EchoStatsEntry::OpType_PUSH;
        stats.delayEntry = false;
        stats.fullQueue = m_echoQueuePushIndex == m_echoQueuePopIndex;
        stats.emptyQueue = false;
        stats.emptySampleFrame = IsSampleFrameEmpty(m_echoQueue[prevPushIndex], cnt);

        m_echoStats.push_back(stats);
    }
}

bool AudioPreprocess::popEchoFrame(spx_int16_t* buffer, size_t bufferSize)
{
    // TODO: we will need to protect this with a mutex if we ever run in separate threads
    if (!m_prv->m_echoState)
        return false;

    assert(bufferSize == m_samplesPerFrame);

    if (m_echoStats.size() < m_echoStatsQueueMaxSize)
    {
        EchoStatsEntry stats;
        stats.timestamp = common::getTimeAsMSEC();
        stats.popIndex = m_echoQueuePopIndex;
        stats.pushIndex = m_echoQueuePushIndex;
        stats.opType = EchoStatsEntry::OpType_POP;
        stats.delayEntry = 0;
        stats.fullQueue = false;
        stats.emptyQueue = m_echoQueuePushIndex == m_echoQueuePopIndex;

        if (m_echoQueuePopIndex != m_echoQueuePushIndex)
            stats.emptySampleFrame = IsSampleFrameEmpty(m_echoQueue[m_echoQueuePopIndex], m_samplesPerFrame);

        else
            stats.emptySampleFrame = false;

        m_echoStats.push_back(stats);
    }

    if (m_echoQueuePopIndex == m_echoQueuePushIndex)
        return false;

    size_t cnt = min(bufferSize, m_samplesPerFrame);
    memcpy(buffer, m_echoQueue[m_echoQueuePopIndex], cnt * sizeof(spx_int16_t));

    ++m_echoQueuePopIndex;
    if (m_echoQueuePopIndex == EchoQueueSize)
        m_echoQueuePopIndex = 0;

    return true;
}

void AudioPreprocess::setEchoDelayInFrames(int value)
{
    if (value < 0 || value >= EchoQueueSize)
        return;

    m_echoQueueDelay = value;
    m_echoQueuePopIndex = 0;
    m_echoQueuePushIndex = 0;
}

void AudioPreprocess::dumpStats()
{
    if (m_echoStats.size() == 0)
        return;

    static int conversationIdx = -1;
    ++conversationIdx;
    if (conversationIdx >= 10)
        conversationIdx = 0;

    char dumpFileName[MAX_PATH] = {0};
    _snprintf(dumpFileName, sizeof(dumpFileName), "Conv%d.txt", conversationIdx);
    HANDLE fd = CreateFileA(dumpFileName, (GENERIC_READ | GENERIC_WRITE), FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fd != INVALID_HANDLE_VALUE)
    {
        char line[512];
        for (SonarVector<EchoStatsEntry>::const_iterator iter = m_echoStats.begin(); iter != m_echoStats.end(); ++iter)
        {
            int lineSize = 0;
            
            switch (iter->opType)
            {
            case EchoStatsEntry::OpType_POP:
                lineSize = _snprintf(line,sizeof(line), "POP at %d - Idx=%d, emptyQueue=%d, emptySampleFrame=%d\n", iter->timestamp, iter->popIndex, iter->emptyQueue, iter->emptySampleFrame);
                break;

            case EchoStatsEntry::OpType_PUSH:
                lineSize = _snprintf(line,sizeof(line), "PUSH at %d - Idx=%d, delayEntry=%d, fullQueue=%d, emptySampleFrame=%d\n", iter->timestamp, iter->pushIndex, iter->delayEntry, iter->fullQueue, iter->emptySampleFrame);
                break;
            }
            
            if (lineSize > 0)
            {
                DWORD bytesWritten = 0;
                WriteFile(fd, line, lineSize, &bytesWritten, NULL);
            }
        }

        CloseHandle(fd);
    }
}

int AudioPreprocess::clip (AudioFrame &frame, float fDestRange, bool noClip)
{
	int cClips = 0;
	size_t samples = frame.size();

	for (size_t index = 0; index < samples; index ++)
	{
		if (frame[index] > fDestRange)
		{
			cClips ++;
			if (!noClip) frame[index] = fDestRange;
		}
		else
		if (frame[index] < -fDestRange)
		{
			cClips ++;
			if (!noClip) frame[index] = -fDestRange;
		}

	}

	return cClips;
}

float AudioPreprocess::calcAvg (const AudioFrame &frame)
{
	float avgTotal = 0.0f;
	size_t samples = frame.size();

	for (size_t index = 0; index < samples; index ++)
	{
		avgTotal += abs (frame[index]);
	}

	return avgTotal / (float) samples;
}

float AudioPreprocess::calcPeak (const AudioFrame &frame)
{
	float currPeak = 0.0f;
	size_t samples = frame.size();

	for (size_t index = 0; index < samples; index ++)
	{
		float f1 = abs (frame[index]);

		if (f1 > currPeak)
		{
			currPeak = f1;
		}
	}

	return currPeak;
}

inline void calcAvgPeak (const AudioFrame &frame, float& avg, float& peak)
{
    avg = 0.0f;
    peak = 0.0f;
    size_t samples = frame.size();

    for (size_t index = 0; index < samples; index ++)
    {
        float f1 = abs (frame[index]);
        avg += f1;
        if (f1 > peak)
        {
            peak = f1;
        }
    }

    avg = avg / (float) samples;
}

void AudioPreprocess::maximize (AudioFrame &frame, float fPeak, float fDestRange, int &nOutRangeClip, int &nOutValueClip, float &fOutCoff)
{
	nOutRangeClip = 0;
	nOutValueClip = 0;

	if (fPeak == 0.0f)
	{
		return;
	}
	
	if (fPeak < fDestRange)
	{
		return;
	}
		
	float fCoff = fDestRange / fPeak;

	fOutCoff = fCoff;

	size_t samples = frame.size();

	for (size_t index = 0; index < samples; index ++)
	{
		float fSample = frame[index];

		if (fSample > 32767.0f)
		{
			nOutValueClip ++;
		}
		else
		if (fSample < -32768.0f)
		{
			nOutValueClip ++;
		}

		if (fSample < -fDestRange)
		{
			nOutRangeClip ++;
		}
		else
		if (fSample > fDestRange)
		{
			nOutRangeClip ++;
		}

		fSample *= fCoff;

		frame[index] = fSample;
	}
}

void AudioPreprocess::manualGainPath(
    AudioFrame &frame,
    bool enableVa,
    bool hotKeyState,
    bool captureMute,
    int fadeOff,
    int vaSensitivity,
    bool enableAutoGain,
    float _gain,
    bool &outXmit,
    bool &outVaState,
    bool &outClip,
    float &outAvgAmp,
    float &outPeakAmp)
{
	outXmit = false;
	outClip = false;
	outVaState = false;

    if (!enableAutoGain)
    	gain(frame, _gain);
	
    calcAvgPeak(frame, outAvgAmp, outPeakAmp);

	if (hotKeyState)
	{
		m_fadeoff.ptt = fadeOff;
	}

	m_fadeoff.ptt --;
	m_fadeoff.va --;

	if (m_fadeoff.ptt < 0)
		m_fadeoff.ptt = 0;

	if (m_fadeoff.va < 0) 
		m_fadeoff.va = 0;

	if (enableVa)
	{
		if (outPeakAmp > ( (100 - vaSensitivity) * 80))
		{
			m_fadeoff.va = fadeOff;
		}

		outVaState = (m_fadeoff.va > 0);
	}
	else
	{
		m_fadeoff.va = 0;
	}

	outXmit = (m_fadeoff.va > 0) || (m_fadeoff.ptt > 0);

	if (captureMute)
	{
		outXmit = false;
	}

    //common::Log("a:%f, %f, %f, %s", outPeakAmp, (float)vaSensitivity, (float)((100 - vaSensitivity) * 327.67), outXmit ? "xmit" : "clip");
}


void AudioPreprocess::autoGainPath(AudioFrame &frame, bool enableVa, bool hotKeyState, bool captureMute, int fadeOff, int vaSensitivity, bool &outXmit, bool &outVaState, bool &outClip, float &outAvgAmp, float &outPeakAmp)
{
	outXmit = false;
	outClip = false;
	outVaState = false;

	const static float RANGE = 16000.0;

	float avg = calcAvg(frame);
	
	m_lt.amp += avg;
	m_lt.frames ++;

	m_st.amp += avg;
	m_st.frames ++;

	if (m_lt.frames == (unsigned long) m_framesPerSecond * 400)
	{
		m_lt.frames /= 2;
		m_lt.amp /= 2.0f;
	}

	if (m_st.frames == (unsigned long) m_framesPerSecond)
	{
		m_st.frames /= 2;
		m_st.amp /= 2.0f;
	}

	float ltAvg = m_lt.amp / (float) m_lt.frames; 

	AudioFrame finalFrame = frame;
	gain(finalFrame, m_auto.gain);

    calcAvgPeak(finalFrame, outAvgAmp, outPeakAmp);
	
	int nRangeClip;
	int nValueClip;
	float coff;

	maximize(finalFrame, outPeakAmp, m_destRange, nRangeClip, nValueClip, coff);

	if (nValueClip > 0)
	{
		outClip = true;
	}

	if (nRangeClip > m_samplesPerFrame / 8)
	{
		if (m_auto.gain < m_auto.peak || m_auto.peak == 0.0f)
		{
			m_auto.peak = m_auto.gain;
		}

		m_auto.gain -= 0.1f;
	}


	if (hotKeyState)
		m_fadeoff.ptt = fadeOff;

	m_fadeoff.ptt --;
	m_fadeoff.va --;
	
	if (m_fadeoff.ptt < 0)
		m_fadeoff.ptt = 0;
	
	if (m_fadeoff.va < 0)
		m_fadeoff.va = 0;

	if (enableVa)
	{
		if (avg == 0.0f) avg = 0.1f;

		float f1 = avg - ltAvg;
		float f2 = f1 / avg;
		f2 *= 100.0f;

		bool bTrigger = (f2 > 25.0f);

		if (bTrigger)
		{
			if (m_fadeoff.va == 0)
			{
				m_fadeoff.va = fadeOff;
			}

			m_fadeoff.va  ++;

			if (m_fadeoff.va  > 100) m_fadeoff.va = 100;
		}
		else
		{
			m_fadeoff.va  --;
		}

		if (m_fadeoff.va  < 0) m_fadeoff.va  = 0;

		outVaState = (m_fadeoff.va > 0);
	}


	outXmit = (m_fadeoff.va > 0) || (m_fadeoff.ptt);

	if (outXmit)
	{
		if (m_auto.peak == 0.0f)
		{
			m_auto.gain += 0.1f;
		}
		else
		{
			if (m_auto.gain > m_auto.peak)
			{
				m_auto.gain += 0.001f;
			}
			else
			{
				m_auto.gain += 0.01f;
			}
		}
	}

	if (captureMute)
	{
		outXmit = false;
	}


	frame = finalFrame;
}


}
