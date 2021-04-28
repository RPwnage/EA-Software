#pragma once 
#include <SonarAudio/Audio.h>
#include <SonarCommon/Common.h>

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

namespace sonar
{
	class DSCaptureDevice
	{
	public:
		DSCaptureDevice (AudioRuntime &runtime, const AudioDeviceId &_device, int _samplesPerSec, int _samplesPerFrame, int _bitsPerSample, int _framesPerBuffer);
		~DSCaptureDevice (void);
		CaptureResult poll (AudioFrame &_outFrame);
		bool isCreated(void);

	private:
		LPDIRECTSOUNDCAPTURE8 m_pDSC;
		LPDIRECTSOUNDCAPTUREBUFFER m_pDSCaptureBuffer;
		DWORD m_samplesPerBuffer;
		DWORD m_bytesPerSample;
		DWORD m_bufferCount;
		DWORD m_bytesPerBuffer;
		DWORD m_totalBufferSize;
		DWORD m_nextReadOffset;

		short *m_pcmBuffer;

	};
}