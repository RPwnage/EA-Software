#pragma once

#ifndef SONAR_CLIENT_BUILD_CONFIG
#define SONAR_CLIENT_BUILD_CONFIG <SonarClient/DefaultBuildConfig.h>
#endif
#include SONAR_CLIENT_BUILD_CONFIG

#include <SonarCommon/Common.h>

namespace sonar
{
    class Options;

class AudioPreprocess
{
public:

	AudioPreprocess (const Options& opts, int samplesPerFrame, int samplesPerSecond, float destRange);
	~AudioPreprocess ();

	void denoise(AudioFrame &frame);
    void pushEchoFrame(const AudioFrame &frame);
    void setEchoDelayInFrames(int value);
	void autoGainPath(AudioFrame &frame, bool enableVa, bool hotKeyState, bool captureMute, int fadeOff, int vaSensitivity, bool &outXmit, bool &outVaState, bool &outClip, float &avgAmp, float &peakAmp);

    void manualGainPath(
        AudioFrame &frame,
        bool enableVa,
        bool hotKeyState,
        bool captureMute,
        int fadeOff,
        int vaSensitivity,
        bool enableAutoGain,
        float gain,
        bool &outXmit,
        bool &outVaState,
        bool &outClip,
        float &avgAmp,
        float &peakAmp);
	
	static void gain (AudioFrame &frame, float fgain);
	static int clip (AudioFrame &frame, float fDestRange, bool noClip);
	static float calcAvg (const AudioFrame &frame);
	static float calcPeak (const AudioFrame &frame);
	static void mix(AudioFrame &dst, const AudioFrame &src, bool& wasClipped);
	static void maximize (AudioFrame &frame, float fPeak, float fDestRange, int &nOutRangeClip, int &nOutValueClip, float &fOutCoff);

private:
    void dumpStats();
    bool popEchoFrame(short* buffer, size_t bufferSize);

	struct
	{
		float amp;
		unsigned long frames;
	} m_lt;

	struct
	{
		float amp;
		unsigned long frames;
	} m_st;

	int m_framesPerSecond;
	int m_samplesPerFrame;

	float m_destRange;
	
	struct
	{
		float gain;
		float peak;
	} m_auto;
	
	struct
	{
		int ptt;
		int va;
	} m_fadeoff;

    short** m_echoQueue;
    int m_echoQueueDelay;
    int m_echoQueuePopIndex;
    int m_echoQueuePushIndex;

    struct EchoStatsEntry
    {
        LONG timestamp;
        int popIndex;
        int pushIndex;

        enum OpType
        {
            OpType_POP,
            OpType_PUSH
        };
        OpType opType;

        bool delayEntry;
        bool fullQueue;
        bool emptyQueue;
        bool emptySampleFrame;
    };
    int m_echoStatsQueueMaxSize;
    SonarVector<EchoStatsEntry> m_echoStats;

	struct Private;
	struct Private *m_prv;
};

static const float clippingMin = -32768.0;
static const float clippingMax = 32767.0;

// inlines

inline void AudioPreprocess::gain (AudioFrame &frame, float fgain)
{
    size_t samples = frame.size();

    for (size_t index = 0; index < samples; index ++)
    {
        frame[index] *= fgain;
    }

}

inline void AudioPreprocess::mix(AudioFrame &dst, const AudioFrame &src, bool& wasClipped)
{
    size_t samples = dst.size();

    for (size_t index = 0; index < samples; index ++)
    {
        bool wasSampleClipped = dst[index] > clippingMax || dst[index] < clippingMin;
        dst[index] += src[index];
        bool isSampleClipped = dst[index] > clippingMax || dst[index] < clippingMin;
        wasClipped = wasClipped || (!wasSampleClipped && isSampleClipped);
    }
}

}