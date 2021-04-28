//  *************************************************************************************************
//
//   File:    ssfseasons.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef SSFSEASONS_H
#define SSFSEASONS_H

#include "./playerinstance.h"
#include "./player.h"
#include "./reference.h"
#include "./ssfseasonsgamesession.h"
#include "./gameplayuser.h"
#include "component/messaging/tdf/messagingtypes.h"

namespace Blaze {
	namespace Stress {

		using namespace eastl;

		class SsfSeasons : public GamePlayUser, public SsfSeasonsGameSession, ReportTelemtryFiber {
			NON_COPYABLE(SsfSeasons);

		public:
			SsfSeasons(StressPlayerInfo* playerData);
			virtual ~SsfSeasons();
			virtual BlazeRpcError   simulateLoad();
			void                    addPlayerToList();
			void                    removePlayerFromList();
			//BlazeRpcError	    	seasonLeaderboardCalls();
			virtual PlayerType      getPlayerType() {
				return SSFSEASONS;
			}
			virtual void            deregisterHandlers();
			virtual void            postGameCalls();
			virtual void            onAsync(uint16_t component, uint16_t type, RawBuffer* payload) {
				GameSession::onAsync(component, type, payload);
			}
			BlazeRpcError			startMatchmakingScenario(ScenarioType scenarioName);
			BlazeRpcError			startGroupMatchmakingScenario(ScenarioType scenarioName);
			BlazeRpcError			createOrJoinGame();
			//BlazeRpcError			initializeCupAndTournments();
			BlazeRpcError			lobbyRPCcalls();
			BlazeRpcError			joinClub(ClubId clubID);
			ShieldExtension*        shieldExtension;
			//int32_t					getOSEventId();
			void					checkIfFriendExist();

		protected:
			virtual bool            executeReportTelemetry();
			virtual void            simulateInGame();
			BlazeRpcError           submitGameReport(const char8_t* gametype);

		private:
			bool                    mIsMarkedForLeaving;
		};


	}  // namespace Stress
}  // namespace Blaze

#endif  //   SSFSEASONS_H