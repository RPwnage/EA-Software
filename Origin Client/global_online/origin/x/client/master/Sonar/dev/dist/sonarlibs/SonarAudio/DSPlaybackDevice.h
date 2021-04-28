#pragma once 
#include <SonarAudio/Audio.h>
#include <SonarCommon/Common.h>

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

namespace sonar
{
	class DSPlaybackBuffer;

	class DSPlaybackDevice
	{
	public:
		DSPlaybackDevice (AudioRuntime &runtime, const AudioDeviceId &_deviceId, int _samplesPerSec, int _samplesPerFrame, int _bitsPerSample);
		~DSPlaybackDevice (void);
		DSPlaybackBuffer *createBuffer (int _numSamples, int _samplesPerFrame);
		int getSamplesPerSec (void);
		int getBitsPerSample (void);
		bool isCreated(void);

		LPDIRECTSOUND8 getDS8 (void);
	private:
		LPDIRECTSOUND8 m_pDS;

		int m_samplesPerSec;
		int m_bitsPerSample;
	};
}