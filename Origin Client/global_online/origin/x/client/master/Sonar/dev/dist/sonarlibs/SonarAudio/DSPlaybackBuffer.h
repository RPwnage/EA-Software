#pragma once 

#include <SonarAudio/Audio.h>
#include <SonarCommon/Common.h>

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

namespace sonar
{
	class DSPlaybackDevice;

	class DSPlaybackBuffer
	{
	public:
		DSPlaybackBuffer (DSPlaybackDevice &_device, int _numSamples, int _samplesPerFrame);
		~DSPlaybackBuffer (void);
		void enqueue (const AudioFrame &_frame);
		PlaybackResult query(void);
		bool isCreated(void);
        void stop(void);

        void calculateStats(float& deltaPlaybackMean, float& deltaPlaybackMax, float& receiveToPlaybackMean, float& receiveToPlaybackMax);

	private:
		int getDiff();

		LPDIRECTSOUNDBUFFER m_pDSBuffer;
        UINT64 m_lastEnqueue;
		DWORD m_totalBufferSize;
		DWORD m_nextWriteOffset;
		DWORD m_samplesPerFrame;
		DWORD m_bytesPerFrame;
		DWORD m_frameCount;
		short *m_shortBuffer;
		int m_cOverflow;
	};
}
