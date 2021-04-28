#pragma once

#ifndef SONAR_CLIENT_BUILD_CONFIG
#define SONAR_CLIENT_BUILD_CONFIG <SonarClient/DefaultBuildConfig.h>
#endif
#include SONAR_CLIENT_BUILD_CONFIG

namespace sonar
{
	struct PeerInfo
	{
		int chid;
		String userId;
		String userDesc;
		bool isTalking;
		bool isMuted;
	};

	enum CaptureMode
	{
		PushToTalk,
		VoiceActivation,		
	};

	enum Error
	{
		ClientNotFound,
		InvalidToken,
		ConnectionState,
	};

	typedef SonarVector<PeerInfo> PeerList;
	typedef SonarList<AudioDeviceId> AudioDeviceList;


}