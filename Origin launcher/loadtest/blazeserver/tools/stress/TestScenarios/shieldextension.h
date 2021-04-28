//  *************************************************************************************************
//
//   File:    shieldextension.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef SHIELDEXTENSION_H
#define SHIELDEXTENSION_H

#include "./reference.h"
//#include "Client.pb.h"
#include "Client.pb.cc"

namespace Blaze {
	namespace Stress {

		using namespace eastl;

		class ShieldExtension {
			NON_COPYABLE(ShieldExtension);
		public:
	//	static	ShieldExtension&	GetInstance(StressPlayerInfo* playerData);
			TimerId mPingTimer;
			bool isFirstPingResponse;
			bool shouldHeartbeat;
			ShieldExtension();
			~ShieldExtension();
			BlazeRpcError perfTune(StressPlayerInfo* playerInfo, bool mEnable);
			void ping(StressPlayerInfo* playerInfo, bool isExtensionHeartbeat, int moduleNum);
			BlazeRpcError startHeartBeat();
			BlazeRpcError stopHeartBeat();
			std::string extensionHeartBeatMessage();
			std::string moduleDetectionHeartBeatMessage(StressPlayerInfo* playerInfo, int moduleNum);
		};


	}  // namespace Stress
}  // namespace Blaze

#endif  //   SHIELDEXTENSION_H