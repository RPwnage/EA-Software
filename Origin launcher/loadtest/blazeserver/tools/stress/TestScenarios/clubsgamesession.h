//  *************************************************************************************************
//
//   File:    clubsgamesession.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef CLUBSGAMESESSION_H
#define CLUBSGAMESESSION_H

#include "playerinstance.h"
#include "player.h"
#include "playermodule.h"
#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/tdf/matchmaker.h"
#include "gamemanager/rpc/gamemanagerslave.h"
#include "reference.h"
#include "clubs.h"
#include "gamesession.h"


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

class ClubsPlayerGameSession : public GameSession {
        NON_COPYABLE(ClubsPlayerGameSession);

    public:
        ClubsPlayerGameSession(StressPlayerInfo* playerData, uint32_t GameSize, GameNetworkTopology Topology, GameProtocolVersionString GameProtocolVersionString);
        virtual ~ClubsPlayerGameSession();

        virtual void                asyncHandlerFunction(uint16_t type, EA::TDF::Tdf* notification);
		BlazeRpcError				submitGameReport(ScenarioType scenarioName);
		BlazeRpcError				resetDedicatedServer();
		BlazeRpcError				joinGame(GameId gameId);
		BlazeRpcError				getGameListSubscription();
		bool						isClubGameInProgress();
		void						setClubGameStatus(bool gameProgress);
		GameId						addClubFriendliesCreatedGame();
		bool						isClubFriendliesGameJoiner;
		GameReportingId				mGameReportingId;
    protected:
        virtual uint32_t            getInGameDurationFromConfig() {
            return CLUBS_CONFIG.roundInGameDuration;
        }
        virtual uint32_t            getInGameDurationDeviationFromConfig() {
            return CLUBS_CONFIG.inGameDeviation;
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
		void						handleNotifyGameGroupSetUp(NotifyGameSetup* notify, GameGroupType gameGroupType);
		ScenarioType				mScenarioType;
		eastl::string				mVProName;
		bool						mIsGameGroupOwner;
		bool						mIsHomeGroupOwner;
		PlayerInGameMap				mPlayerInGameMap;
		ClubId						mOpponentClubId;
		mutable EA::Thread::Mutex   mMutex;
		mutable EA::Thread::Mutex   mMutexWrite;
		char8_t						mHomeGameName[MAX_GAMENAME_LEN];
		char8_t						mVoipGameName[MAX_GAMENAME_LEN];
		char8_t						mGameGroupName[MAX_GAMENAME_LEN];
		GameId						mHomeGroupGId;
		GameId						mVoipGroupGId;
		GameId						mGamegroupGId;
		BlazeRpcError				findClubsAsync();
		Blaze::GameManager::GameBrowserListId mGameListId;
		typedef eastl::hash_map<BlazeId, BlazeId> PlayerAccountMap;
		eastl::string				mAvatarName;
    private:
        
};

}  // namespace Stress
}  // namespace Blaze

#endif  //   CLUBSGAMESESSION_H
