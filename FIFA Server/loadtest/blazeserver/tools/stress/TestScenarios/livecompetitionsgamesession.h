//  *************************************************************************************************
//
//   File:    livecompetitionsgamesession.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef LIVECOMPETITIONSGAMESESSION_H
#define LIVECOMPETITIONSGAMESESSION_H

#include "./playerinstance.h"
#include "./player.h"
#include "./playermodule.h"
#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/tdf/matchmaker.h"
#include "gamemanager/rpc/gamemanagerslave.h"
#include "./reference.h"
#include "./livecompetitions.h"


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

class LiveCompetitionsGameSession : public GameSession {
        NON_COPYABLE(LiveCompetitionsGameSession);

    public:
        LiveCompetitionsGameSession(StressPlayerInfo* playerData, uint32_t GameSize, GameNetworkTopology Topology, GameProtocolVersionString GameProtocolVersionString);
        virtual ~LiveCompetitionsGameSession();

        virtual void                asyncHandlerFunction(uint16_t type, EA::TDF::Tdf* notification);
		StressPlayerInfo&			getOpponentPlayerInfo() {
			return mOpponentPlayerInfo;
		}
        inline  void                setMyRoleName(RoleName roleName)                    {
            mMyRoleName = roleName;
        }
        inline  RoleName  getMyRoleName()                                     {
            return mMyRoleName;
        }
		BlazeRpcError				submitGameReport();
		uint32_t                    geteventId(){
			return mEventId;
		}
		void                        seteventId(uint32_t eventId)
		{
			mEventId = eventId;
		}

    protected:
        virtual uint32_t            getInGameDurationFromConfig() {
            return LC_CONFIG.roundInGameDuration;
        }
        virtual uint32_t            getInGameDurationDeviationFromConfig() {
            return LC_CONFIG.inGameDeviation;
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
        RoleName                    mMyRoleName;
		ScenarioType				mScenarioType;
        bool                        mPromotedToInGameFromQue;
		StressPlayerInfo            mOpponentPlayerInfo;
		uint32_t                    mEventId;
    private:
        
};

}  // namespace Stress
}  // namespace Blaze

#endif  //   LIVECOMPETITIONSGAMESESSION_H
