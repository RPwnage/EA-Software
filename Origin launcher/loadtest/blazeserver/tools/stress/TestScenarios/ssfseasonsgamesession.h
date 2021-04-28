//  *************************************************************************************************
//
//   File:    ssfseasonsgamesession.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************


#ifndef SSFSEASONSGAMESESSION_H
#define SSFSEASONSGAMESESSION_H

#include "./playerinstance.h"
#include "./player.h"
#include "./playermodule.h"
#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/tdf/matchmaker.h"
#include "gamemanager/rpc/gamemanagerslave.h"
#include "./reference.h"
#include "./ssfseasons.h"


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

class SsfSeasonsGameSession : public GameSession {
	NON_COPYABLE(SsfSeasonsGameSession);

public:
	SsfSeasonsGameSession(StressPlayerInfo* playerData, uint32_t GameSize, GameNetworkTopology Topology, GameProtocolVersionString GameProtocolVersionString);
	virtual ~SsfSeasonsGameSession();

	virtual void                asyncHandlerFunction(uint16_t type, EA::TDF::Tdf* notification);
	StressPlayerInfo&			getOpponentPlayerInfo() {
		return mOpponentPlayerInfo;
	}

	BlazeIdList					mPlayerIds;
	BlazeRpcError				submitGameReport(ScenarioType scenarioName);
	GameReportingId				mGameReportingId;
	uint32_t                    geteventId() {
		return mEventId;
	}
	void                        seteventId(uint32_t eventId)
	{
		mEventId = eventId;
	}

	BlazeId						getFriendInMap(BlazeId playerID);
	void						removeFriendFromMap();
	void						removeFriendFromList();
	void						checkIfFriendExist();
	BlazeId						getARandomFriend();
	void						addVoltaFriendsToMap(BlazeId hostId, BlazeId friendID);
	void						addFriendToList(BlazeId playerId);

protected:
	virtual uint32_t            getInGameDurationFromConfig() {
		return SSFSEASONS_CONFIG.roundInGameDuration;
	}
	virtual uint32_t            getInGameDurationDeviationFromConfig() {
		return SSFSEASONS_CONFIG.inGameDeviation;
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
	void						handleNotifyPingResponse(Blaze::Perf::NotifyPingResponse* notify, uint16_t type = Blaze::Perf::PerfSlave::NOTIFY_NOTIFYPINGRESPONSE);
	void						handleNotifyGameSetUpGroup(NotifyGameSetup* notify, uint16_t type = GameManagerSlave::NOTIFY_NOTIFYGAMESETUP);

	ScenarioType				mScenarioType;
	StressPlayerInfo            mOpponentPlayerInfo;
	uint32_t                    mEventId;
	mutable EA::Thread::Mutex   mMutex;
	BlazeId						mFriendBlazeId;
	bool						isGrupPlatformHost;
	PlayerInGameMap				mPlayerInGameMap;
	PlayerInGameMap				mPlayerInGameGroupMap;
	GameId						mGameGroupId;
	ConnectionGroupId			mTopologyHostConnectionGroupId;
	GameManager::ScenarioVariantName			mSceanrioVariant;

private:

};

	}  // namespace Stress
}  // namespace Blaze
/*{
public:
	ssfseasonsgamesession();
	~ssfseasonsgamesession();
};*/

#endif  //   SSFSEASONSGAMESESSION_H