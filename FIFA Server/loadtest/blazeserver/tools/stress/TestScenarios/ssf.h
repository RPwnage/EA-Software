//  *************************************************************************************************
//
//   File:    ssf.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef SSF_H
#define SSF_H

#include "playerinstance.h"
#include "gameplayuser.h"

namespace Blaze {
namespace Stress {
class SSFPlayer : public GamePlayUser {
		NON_COPYABLE(SSFPlayer);
	public:
		SSFPlayer(StressPlayerInfo *playerData);
		virtual                     ~SSFPlayer();
		virtual BlazeRpcError       simulateLoad();
		virtual PlayerType          getPlayerType() {
			return SSFPLAYER;
		}

	protected:
		SSFPlayerGamePlayConfig*	mConfig;
		uint32_t                    mViewId;
		BlazeRpcError               submitOfflineGameReport(const char8_t* gametype);
		ScenarioType				mScenarioType;
		StressPlayerInfo*			mPlayerData;
		
	private:
		bool                        mIsInSSFPlayerGame;
		Blaze::TimeValue            mSSFPlayerGameStartTime;
	};
}
}
#endif