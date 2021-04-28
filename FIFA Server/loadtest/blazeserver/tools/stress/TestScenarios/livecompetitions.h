//  *************************************************************************************************
//
//   File:    livecompetitions.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef LIVECOMPETITIONS_H
#define LIVECOMPETITIONS_H

#include "./playerinstance.h"
#include "./player.h"
#include "./reference.h"
#include "./livecompetitionsgamesession.h"
#include "./gameplayuser.h"
#include "component/messaging/tdf/messagingtypes.h"

namespace Blaze {
namespace Stress {

using namespace eastl;

class LiveCompetitions : public GamePlayUser, public LiveCompetitionsGameSession, ReportTelemtryFiber {
        NON_COPYABLE(LiveCompetitions);

    public:
        LiveCompetitions(StressPlayerInfo* playerData);
        virtual ~LiveCompetitions();
        virtual BlazeRpcError   simulateLoad();
        void                    addPlayerToList();
        void                    removePlayerFromList();
		BlazeRpcError			liveCompetitionsLeaderboards();
        virtual PlayerType      getPlayerType() {
            return LIVECOMPETITIONS;
        }
        virtual void            deregisterHandlers();
        virtual void            preLogoutAction();
        virtual void            postGameCalls();
        virtual void            onAsync(uint16_t component, uint16_t type, RawBuffer* payload) {
            GameSession::onAsync(component, type, payload);
        }
		BlazeRpcError			startMatchmaking();
		BlazeRpcError playerEventRegistrationflow();
		BlazeRpcError					lobbyRPCs();
		int32_t getLCEventId();

    protected:
        virtual bool            executeReportTelemetry();
        virtual void            simulateInGame();

    private:
        bool                    mIsMarkedForLeaving;
        char mCompetitionName[64];
};


}  // namespace Stress
}  // namespace Blaze

#endif  //   LIVECOMPETITIONS_H

