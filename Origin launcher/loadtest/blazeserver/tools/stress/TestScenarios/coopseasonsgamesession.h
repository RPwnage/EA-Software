//  *************************************************************************************************
//
//   File:    coopseasonsgamesession.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef COOPSEASONSGAMESESSION_H
#define COOPSEASONSGAMESESSION_H

/*** Include files *******************************************************************************/

#include "EASTL/map.h"
#include "EASTL/vector_set.h"
#include "stressmodule.h"
#include "EATDF/time.h"
#include "framework/tdf/attributes.h"
#include "./gamesession.h"


#ifdef TARGET_playgroups
    #include "playgroupsutil.h"
#endif

#include "gamemanager/tdf/matchmaker.h"
#include "gamemanager/rpc/gamemanagerslave.h"
#include "framework/config/config_map.h"

#include "gamereporting/rpc/gamereportingslave.h"
#include "customcode/component/gamereporting/osdk/tdf/gameosdkreport.h"
//#include "customcode/component/gamereporting/osdk/tdf/gameosdkreport_server.h"

#include "customcode/component/gamereporting/fifa/tdf/fifaclubreport.h"
#include "customcode/component/gamereporting/osdk/tdf/gameosdkclubreport.h"

#include "customcode/component/gamereporting/fifa/tdf/fifah2hreport.h"  // To set Common Game Report;

#include "customcode/component/gamereporting/fifa/tdf/fifacoopreport.h"  // To include fifacoopseason related structures; To include fifacoopreportbase

#include "fifacups/tdf/fifacupstypes.h"
#include "customcomponent/osdktournaments/tdf/osdktournamentstypes.h"
#include "customcomponent/osdktournaments/tdf/osdktournamentstypes_custom.h"
#include "customcomponent/osdksettings/tdf/osdksettingstypes.h"
#include "customcomponent/osdksettings/rpc/osdksettings_defines.h"

// #include "playgroups/tdf/playgroups.h"

//#include "component/gamemanager/stress/matchmakerrulesutil.h"  // for defines SizeRuleType, PredefinedRuleConfiguration, GenericRuleDefinitionList etc
#include "framework/logger.h"

#include "./playerinstance.h"
#include "./player.h"
#include "./playermodule.h"
#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/tdf/matchmaker.h"
#include "gamemanager/rpc/gamemanagerslave.h"
#include "./reference.h"



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

class CoopSeasonsGameSession : public GameSession {
        NON_COPYABLE(CoopSeasonsGameSession);

    public:
        CoopSeasonsGameSession(StressPlayerInfo* playerData, uint32_t GameSize, GameNetworkTopology Topology, GameProtocolVersionString GameProtocolVersionString);
        virtual ~CoopSeasonsGameSession();

        virtual void                asyncHandlerFunction(uint16_t type, EA::TDF::Tdf* notification);
		//StressPlayerInfo&			getOpponentPlayerInfo() {
		//	return mOpponentPlayerInfo;
		//}
		//
		//StressPlayerInfo&			getFriendPlayerInfo() {
		//	return mOpponentPlayerInfo;
		//}

		BlazeRpcError				submitGameReport();
		BlazeRpcError				setGameAttributes(GameId  gameId, Collections::AttributeMap &gameAttributes);
		BlazeId						getARandomFriend();
		void						addFriendToList(BlazeId playerId);	
		mutable						EA::Thread::Mutex mMutex;
		void						addCoopFriendsToMap(BlazeId hostId, BlazeId friendID);
		BlazeId						getFriendInMap(BlazeId playerID);
		void						removeFriendFromMap();
		//BlazeRpcError			    getCoopIdData(BlazeId blazeId1, BlazeId blazeId2, BlazeId &coopId);
		BlazeRpcError				getCoopIdData(BlazeId BlazeID1, BlazeId BlazeID2, uint64_t &coopId);
		GameReportingId				mGameReportingId;
		BlazeIdList					mPlayerIds;
		GameId						homeGameId;

    protected:
        virtual uint32_t				getInGameDurationFromConfig() {
            return CO_CONFIG.roundInGameDuration;
        }
        virtual uint32_t				getInGameDurationDeviationFromConfig() {
            return CO_CONFIG.inGameDeviation;
        }

        //  Place Holder for Overriding Default async handling, Derived Classes has to implement this so as to override Default Async Notification Handling, return true if handled
        virtual bool					onGameSessionAsyncHandlerOverride(uint16_t type, EA::TDF::Tdf* notification) {
            return false;
        }
        //  Place holder for extending Default Async handling, Derived classes has to implement this so as to extend Default Async Notification Handling
        virtual void					onGameSessionAsyncHandlerExtended(uint16_t type, EA::TDF::Tdf* notification) {
            return;
        }
        //   Default Async Notification Handler
        void							onGameSessionDefaultAsyncHandler(uint16_t type, EA::TDF::Tdf* notification);
        //  This handler will be used for all the notifications with NotifyGameSetup object.
		void							handleNotifyGameSetUp(NotifyGameSetup* notify, uint16_t type = GameManagerSlave::NOTIFY_NOTIFYGAMESETUP);
		void							handleNotifyGameSetUpGroup(NotifyGameSetup* notify, uint16_t type = GameManagerSlave::NOTIFY_NOTIFYGAMESETUP);

		BlazeRpcError					updateExtendedDataAttribute(int64_t value);
		BlazeRpcError					updateGameSession(GameId  gameId);
	
		BlazeId							mFriendBlazeId;
		PlayerInGameMap					mPlayerInGameMap;
		eastl::map<BlazeId, int64_t>	PlayerCoopMap;
		GameId							mGameGroupId;
		int64_t							mCoopId;
		ConnectionGroupId				mTopologyHostConnectionGroupId;
		bool							isGrupPlatformHost;
    private:
		
        
};

}  // namespace Stress
}  // namespace Blaze

#endif  //   ONLINESEASONSGAMESESSION_H
