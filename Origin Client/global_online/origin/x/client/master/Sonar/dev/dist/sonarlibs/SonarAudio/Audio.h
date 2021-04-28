#pragma once

#ifdef _WIN32

namespace sonar
{
	class DSCaptureDevice;
	class DSCaptureDeviceEnum;
	class DSPlaybackDevice;
	class DSPlaybackDeviceEnum;
	class DSAudioRuntime;
	class DSPlaybackBuffer;

	typedef DSPlaybackBuffer AudioPlaybackBuffer;
	typedef DSCaptureDeviceEnum AudioCaptureEnum;
	typedef DSCaptureDevice AudioCaptureDevice;
	typedef DSPlaybackDeviceEnum AudioPlaybackEnum;
	typedef DSPlaybackDevice AudioPlaybackDevice;
	typedef DSAudioRuntime AudioRuntime;
}

#include "DSCaptureDeviceEnum.h"
#include "DSCaptureDevice.h"
#include "DSPlaybackBuffer.h"
#include "DSPlaybackDevice.h"
#include "DSPlaybackDeviceEnum.h"
#include "DSAudioRuntime.h"

#else
#error "We might have forgot to port this"
#endif

