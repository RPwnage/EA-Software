#pragma once

#include <SonarCommon/Common.h>

namespace sonar
{
	struct AudioDeviceId
	{
		String id;
		String name;
	};

	typedef SonarList<AudioDeviceId> AudioDeviceList;

	enum CaptureResult
	{
		CR_Success,
		CR_DeviceLost,
		CR_NoAudio,
	};

	enum PlaybackResult
	{
		PR_Success,
		PR_DeviceLost,
		PR_Underrun,
		PR_Overflow

	};

	void PCMShortToFloat (short *_pInBuffer, size_t _cSamples, float *_pOutBuffer);

}