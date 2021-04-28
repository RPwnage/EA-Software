//  *************************************************************************************************
//
//   File:    clubs.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef CLUBS_H
#define CLUBS_H

#include "playerinstance.h"
#include "player.h"
#include "reference.h"
#include "clubsgamesession.h"
#include "gameplayuser.h"
#include "component/messaging/tdf/messagingtypes.h"

namespace Blaze {
namespace Stress {

using namespace eastl;

class ClubsPlayer : public GamePlayUser, public ClubsPlayerGameSession, ReportTelemtryFiber {
        NON_COPYABLE(ClubsPlayer);

    public:
        ClubsPlayer(StressPlayerInfo* playerData);
        virtual ~ClubsPlayer();
        virtual BlazeRpcError   simulateLoad();
		virtual BlazeRpcError   createOrJoinClubs();
        void                    addPlayerToList();
        void                    removePlayerFromList();
        virtual PlayerType      getPlayerType() {
			return CLUBSPLAYER;
        }
        virtual void            deregisterHandlers();
        virtual void            postGameCalls();
        virtual void            onAsync(uint16_t component, uint16_t type, RawBuffer* payload) {
            GameSession::onAsync(component, type, payload);
        }
		BlazeRpcError			startMatchmakingScenario(ScenarioType scenarioName);
		BlazeRpcError			createOrJoinGame(GameGroupType gametype);
		void					lobbyRpcCalls();
		BlazeRpcError			clubsLeaderboards();
		BlazeRpcError			tournamentRpcCalls();		
		BlazeRpcError			tournamentCalls();
		BlazeRpcError			createClub(CreateClubResponse& response);
		BlazeRpcError			createPracticeClub(CreateClubResponse& response);
		BlazeRpcError			joinClub(ClubId clubID);
		bool					isTargetTeamSizeReached();		
		BlazeRpcError			sendOrReceiveInvitation();
		uint32_t				postNewsCount;
		
    protected:
        virtual bool            executeReportTelemetry();
        virtual void            simulateInGame();

    private:
        bool                    mIsMarkedForLeaving;
		uint8_t					mTargetTeamSize;
};


}  // namespace Stress
}  // namespace Blaze

#endif  //   CLUBS_H

