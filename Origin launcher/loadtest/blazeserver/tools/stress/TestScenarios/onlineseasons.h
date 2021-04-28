//  *************************************************************************************************
//
//   File:    onlineseasons.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef ONLINESEASONS_H
#define ONLINESEASONS_H

#include "./playerinstance.h"
#include "./player.h"
#include "./reference.h"
#include "./onlineseasonsgamesession.h"
#include "./gameplayuser.h"
#include "component/messaging/tdf/messagingtypes.h"

namespace Blaze {
namespace Stress {

using namespace eastl;

class OnlineSeasons : public GamePlayUser, public OnlineSeasonsGameSession, ReportTelemtryFiber {
        NON_COPYABLE(OnlineSeasons);

    public:
        OnlineSeasons(StressPlayerInfo* playerData);
        virtual ~OnlineSeasons();
        virtual BlazeRpcError   simulateLoad();
        void                    addPlayerToList();
        void                    removePlayerFromList();
		BlazeRpcError	    	seasonLeaderboardCalls();
        virtual PlayerType      getPlayerType() {
            return ONLINESEASONS;
        }
        virtual void            deregisterHandlers();
        virtual void            postGameCalls();
        virtual void            onAsync(uint16_t component, uint16_t type, RawBuffer* payload) {
            GameSession::onAsync(component, type, payload);
        }
		BlazeRpcError			startMatchmakingScenario(ScenarioType scenarioName);
		BlazeRpcError			initializeCupAndTournments();					
		BlazeRpcError			lobbyRPCcalls();
		BlazeRpcError			joinClub(ClubId clubID);
		int32_t					getOSEventId();

    protected:
        virtual bool            executeReportTelemetry();
        virtual void            simulateInGame();

    private:
        bool                    mIsMarkedForLeaving;
};


}  // namespace Stress
}  // namespace Blaze

#endif  //   ONLINESEASONS_H

