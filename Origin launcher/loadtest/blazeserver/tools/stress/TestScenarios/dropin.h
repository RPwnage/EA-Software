//  *************************************************************************************************
//
//   File:    clubs.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef DROPIN_H
#define DROPIN_H

#include "playerinstance.h"
#include "player.h"
#include "reference.h"
#include "dropingamesession.h"
#include "gameplayuser.h"
#include "component/messaging/tdf/messagingtypes.h"

namespace Blaze {
namespace Stress {

using namespace eastl;

class DropinPlayer : public GamePlayUser, public DropinPlayerGameSession, ReportTelemtryFiber {
        NON_COPYABLE(DropinPlayer);

    public:
		DropinPlayer(StressPlayerInfo* playerData);
        virtual ~DropinPlayer();
        virtual BlazeRpcError   simulateLoad();
		virtual void            deregisterHandlers();
        void                    addPlayerToList();
        void                    removePlayerFromList();
		void					lobbyRpcCalls();
		BlazeRpcError			tournamentRpcCalls();
        virtual PlayerType      getPlayerType() {
			return DROPINPLAYER;
        }
        virtual void            postGameCalls();
        virtual void            onAsync(uint16_t component, uint16_t type, RawBuffer* payload) {
            GameSession::onAsync(component, type, payload);
        }
		BlazeRpcError			joinClub(ClubId clubID);
		BlazeRpcError			sendOrReceiveInvitation();
		BlazeRpcError			startMatchmakingScenario(ScenarioType scenarioName);		
		
    protected:
        virtual bool            executeReportTelemetry();
        virtual void            simulateInGame();

    private:
        bool                    mIsMarkedForLeaving;
		uint32_t				postNewsCount;
};


}  // namespace Stress
}  // namespace Blaze

#endif  //   CLUBS_H

