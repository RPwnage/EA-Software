//  *************************************************************************************************
//
//   File:    futplayergamesession.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef FUTPLAYERGAMESESSION_H
#define FUTPLAYERGAMESESSION_H

#include "playerinstance.h"
#include "player.h"
#include "playermodule.h"
#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/tdf/matchmaker.h"
#include "gamemanager/rpc/gamemanagerslave.h"
#include "reference.h"
#include "futplayer.h"


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

class FutPlayerGameSession : public GameSession {
        NON_COPYABLE(FutPlayerGameSession);

    public:
        FutPlayerGameSession(StressPlayerInfo* playerData, uint32_t GameSize, GameNetworkTopology Topology, GameProtocolVersionString GameProtocolVersionString);
        virtual ~FutPlayerGameSession();

        virtual void                asyncHandlerFunction(uint16_t type, EA::TDF::Tdf* notification);
		StressPlayerInfo&			getOpponentPlayerInfo() {
			return mOpponentPlayerInfo;
		}

		mutable EA::Thread::Mutex mMutex;
		BlazeRpcError				submitGameReport(ScenarioType scenarioName = SEASONS);
		BlazeId						getARandomFriend();
		void						addFutFriendsToMap(BlazeId hostId, BlazeId friendID);
		BlazeId						getFriendInMap(BlazeId playerID);
		void						removeFriendFromMap();
		void						addFriendToList(BlazeId playerId);
		BlazeRpcError				futPlayFriendCreateGame(CreateGameResponse& response);
		BlazeRpcError				futPlayFriendJoinGame(GameId gameId);

		BlazeIdList					mPlayerIds;

    protected:
        virtual uint32_t            getInGameDurationFromConfig() {
            return FUT_CONFIG.roundInGameDuration;
        }
        virtual uint32_t            getInGameDurationDeviationFromConfig() {
            return FUT_CONFIG.inGameDeviation;
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
		void						handleNotifyGameSetUpGroup(NotifyGameSetup* notify, uint16_t type = GameManagerSlave::NOTIFY_NOTIFYGAMESETUP);
		void						handleNotifyPingResponse(Blaze::Perf::NotifyPingResponse* notify, uint16_t type = Blaze::Perf::PerfSlave::NOTIFY_NOTIFYPINGRESPONSE);

		void						addFutPlayFriendCreatedGame(GameId gameId);
		GameId						getRandomCreatedGame();
		ScenarioType				mScenarioType;
		BlazeObjectId				mDSGroupId;
		uint16_t					mFutDivision;
		StressPlayerInfo            mOpponentPlayerInfo;
		BlazeId						mFriendBlazeId;

		PlayerInGameMap				mPlayerInGameMap;
		PlayerInGameMap				mPlayerInGameGroupMap;
		GameId						mGameGroupId;
		ConnectionGroupId			mTopologyHostConnectionGroupId;
		ConnectionGroupId			mGroupHostConnectionGroupId;
		bool						isGrupPlatformHost;
    private:
		GameReportingId				mGameReportingId;
};

}  // namespace Stress
}  // namespace Blaze

#endif  //   FUTPLAYERGAMESESSION_H
