#pragma once
#include <SonarAudio/Audio.h>
#include <SonarCommon/Common.h>

namespace sonar
{
	class DSPlaybackDeviceEnum
	{
	public:
		DSPlaybackDeviceEnum();
		~DSPlaybackDeviceEnum();

		const AudioDeviceList &getDevices();

	private:

		AudioDeviceList m_list;
	};
}