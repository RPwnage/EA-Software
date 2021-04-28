//  *************************************************************************************************
//
//   File:    coopseasons.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef COOPSEASONS_H
#define COOPSEASONS_H

#include "./playerinstance.h"
#include "./player.h"
#include "./reference.h"
#include "./coopseasonsgamesession.h"
#include "./gameplayuser.h"
#include "component/messaging/tdf/messagingtypes.h"

namespace Blaze {
namespace Stress {

using namespace eastl;

class CoopSeasons : public GamePlayUser, public CoopSeasonsGameSession, ReportTelemtryFiber {
        NON_COPYABLE(CoopSeasons);

    public:
        CoopSeasons(StressPlayerInfo* playerData);
        virtual ~CoopSeasons();
        virtual BlazeRpcError   simulateLoad();

        void					deregisterHandlers();
        void                    addPlayerToList();
		void                    removePlayerFromList();
        virtual void            postGameCalls();
		void					lobbyRpcs();
		BlazeRpcError			coopSeasonsLeaderboardCalls();
        void					onAsync(uint16_t component, uint16_t type, RawBuffer* payload) {
            GameSession::onAsync(component, type, payload);
        }

		virtual PlayerType      getPlayerType() {
            return COOPPLAYER;
        }
		BlazeRpcError			coopStartMatchmakingScenario();		
		BlazeRpcError			CreateOrJoinGame();

		 // CoopSeasons
		//BlazeRpcError			getCoopIdData(BlazeId blazeId1, BlazeId blazeId2, BlazeId &coopId);
		BlazeRpcError			setCoopIdData(BlazeId blazeId1, BlazeId blazeId2, BlazeId &coopId);
		BlazeRpcError			coopSeasonGetAllCoopIds(Blaze::Stats::GetStatsByGroupRequest::EntityIdList& coopIds);
		void					checkIfFriendExist();
		
    protected:
        virtual bool            executeReportTelemetry();
        virtual void            simulateInGame();

    private:
        bool                    mIsMarkedForLeaving;
		eastl::string			mGameName;
		eastl::string			mCoopId;
};


}  // namespace Stress
}  // namespace Blaze

#endif  //   COOPSEASONS_H

