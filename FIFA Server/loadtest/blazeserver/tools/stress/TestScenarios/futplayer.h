//  *************************************************************************************************
//
//   File:    futplayer.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef FUTPLAYER_H
#define FUTPLAYER_H

#include "playerinstance.h"
#include "player.h"
#include "reference.h"
#include "futplayergamesession.h"
#include "gameplayuser.h"
#include "component/messaging/tdf/messagingtypes.h"

namespace Blaze {
namespace Stress {

using namespace eastl;

class FutPlayer : public GamePlayUser, public FutPlayerGameSession, ReportTelemtryFiber {
        NON_COPYABLE(FutPlayer);

    public:
        FutPlayer(StressPlayerInfo* playerData);
        virtual ~FutPlayer();
        virtual BlazeRpcError   simulateLoad();
        void                    addPlayerToList();
        void                    removePlayerFromList();
        virtual PlayerType      getPlayerType() {
            return FUTPLAYER;
        }
        virtual void            deregisterHandlers();
        virtual void            postGameCalls();
        virtual void            onAsync(uint16_t component, uint16_t type, RawBuffer* payload) {
            GameSession::onAsync(component, type, payload);
        }
		BlazeRpcError			startMatchmakingScenario(ScenarioType scenarioName);		
		BlazeRpcError			createOrJoinGame();
		void					checkIfFriendExist();
		BlazeRpcError			createGameTemplate();
		ShieldExtension*        shieldExtension;

    protected:
        virtual bool            executeReportTelemetry();
        virtual void            simulateInGame();
		BlazeRpcError			startPlayFriendGame();

    private:
        bool                    mIsMarkedForLeaving;
};


}  // namespace Stress
}  // namespace Blaze

#endif  //   FUTPLAYER_H

