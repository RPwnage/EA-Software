//  *************************************************************************************************
//
//   File:    friendliesgamesession.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef ESLTOURNAMENTSGAMESESSION_H
#define ESLTOURNAMENTSGAMESESSION_H

#include "./playerinstance.h"
#include "./player.h"
#include "./playermodule.h"
#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/tdf/matchmaker.h"
#include "gamemanager/rpc/gamemanagerslave.h"
#include "./reference.h"
#include "esltournaments.h"


#define SET_SCENARIO_ATTRIBUTE(scenarioReq, attributeName, value) \
    if (scenarioReq.getScenarioAttributes()[attributeName] == NULL) \
    { \
        scenarioReq.getScenarioAttributes()[attributeName] = scenarioReq.getScenarioAttributes().allocate_element(); \
    } \
    scenarioReq.getScenarioAttributes()[attributeName]->set(value);

namespace Blaze {
namespace Stress {
using namespace Blaze::GameManager;
using namespace eastl;

class ESLTournamentsGameSession : public GameSession {
        NON_COPYABLE(ESLTournamentsGameSession);

    public:
		ESLTournamentsGameSession(StressPlayerInfo* playerData, uint32_t GameSize, GameNetworkTopology Topology, GameProtocolVersionString GameProtocolVersionString);
        virtual ~ESLTournamentsGameSession();

        virtual void                asyncHandlerFunction(uint16_t type, EA::TDF::Tdf* notification);
		StressPlayerInfo&			getOpponentPlayerInfo() {
			return mOpponentPlayerInfo;
		}

		BlazeRpcError				submitGameReport();
		mutable EA::Thread::Mutex mMutex;
    protected:
        virtual uint32_t            getInGameDurationFromConfig() {
            return FR_CONFIG.roundInGameDuration;
        }
        virtual uint32_t            getInGameDurationDeviationFromConfig() {
            return FR_CONFIG.inGameDeviation;
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
		BlazeRpcError				createGame();
		void						addFriendliesCreatedGame(GameId gameId);
		GameId 						getRandomCreatedGame();
		BlazeRpcError				joinGame(GameId gameId);
		ScenarioType				mScenarioType;
		StressPlayerInfo            mOpponentPlayerInfo;
		mutable EA::Thread::Mutex   mutex;
		
    private:
        
};

}  // namespace Stress
}  // namespace Blaze

#endif  //   ESLTOURNAMENTSGAMESESSION_H
