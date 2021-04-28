#pragma once

#include <SonarAudio/Audio.h>
#include <SonarCommon/Common.h>

namespace sonar
{
	class DSCaptureDeviceEnum
	{
	public:
		DSCaptureDeviceEnum();
		~DSCaptureDeviceEnum();
				
		const AudioDeviceList &getDevices();


	private:

		AudioDeviceList m_list;
	};
}