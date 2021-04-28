//  *************************************************************************************************
//
//   File:    dropingamesession.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef DROPINGAMESESSION_H
#define DROPINGAMESESSION_H

#include "playerinstance.h"
#include "player.h"
#include "playermodule.h"
#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/tdf/matchmaker.h"
#include "gamemanager/rpc/gamemanagerslave.h"
#include "reference.h"
#include "dropin.h"

namespace Blaze {
namespace Stress {
using namespace Blaze::GameManager;
using namespace eastl;

class DropinPlayerGameSession : public GameSession {
        NON_COPYABLE(DropinPlayerGameSession);

    public:
		DropinPlayerGameSession(StressPlayerInfo* playerData, uint32_t GameSize, GameNetworkTopology Topology, GameProtocolVersionString GameProtocolVersionString);
        virtual ~DropinPlayerGameSession();

        virtual void                asyncHandlerFunction(uint16_t type, EA::TDF::Tdf* notification);
		BlazeRpcError				submitGameReport();

    protected:
        virtual uint32_t            getInGameDurationFromConfig() {
            return DROPIN_CONFIG.roundInGameDuration;
        }
        virtual uint32_t            getInGameDurationDeviationFromConfig() {
            return DROPIN_CONFIG.inGameDeviation;
        }

        //  Place Holder for Overriding Default async handling, Derived Classes has to implement this so as to override Default Async Notification Handling, return true if handled
        virtual bool                onGameSessionAsyncHandlerOverride(uint16_t type, EA::TDF::Tdf* notification) {
            return false;
        }
        //  Place holder for extending Default Async handling, Derived classes has to implement this so as to extend Default Async Notification Handling
        virtual void                onGameSessionAsyncHandlerExtended(uint16_t type, EA::TDF::Tdf* notification) {
            return;
        }
        //   Default Async Notification Handler
        void                        onGameSessionDefaultAsyncHandler(uint16_t type, EA::TDF::Tdf* notification);
        //  This handler will be used for all the notifications with NotifyGameSetup object.
		void						handleNotifyGameSetUp(NotifyGameSetup* notify, uint16_t type = GameManagerSlave::NOTIFY_NOTIFYGAMESETUP);
		ScenarioType				mScenarioType;
		PlayerInGameMap				mPlayerInGameMap;
		StressPlayerInfo            mOpponentPlayerInfo;
		mutable EA::Thread::Mutex   mMutex;

    private:
		GameReportingId				mGameReportingId;
};

}  // namespace Stress
}  // namespace Blaze

#endif  
