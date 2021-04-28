//  *************************************************************************************************
//
//   File:    singleplayer.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef SINGLEPLAYER_H
#define SINGLEPLAYER_H

#include "eathread/eathread_mutex.h"

#include "./playerinstance.h"
#include "./gameplayuser.h"
#include "./reference.h"
#include "./utility.h"


#include "EASTL/map.h"
#include "EASTL/vector_set.h"
#include "stressmodule.h"
#include "EATDF/time.h"
#include "framework/tdf/attributes.h"
#include "stressinstance.h"

#include "stressmodule.h"
#include "stressinstance.h"
#include "gamemanager/tdf/matchmaker.h"
#include "gamemanager/rpc/gamemanagerslave.h"
#include "framework/config/config_map.h"

#include "gamereporting/rpc/gamereportingslave.h"
#include "customcode/component/gamereporting/osdk/tdf/gameosdkreport.h"

#include "customcode/component/gamereporting/fifa/tdf/fifah2hreport.h"
#include "customcode/component/gamereporting/fifa/tdf/fifasoloreport.h"
#include "customcode/component/gamereporting/fifa/tdf/fifaskillgamereport.h"

#include "fifacups/tdf/fifacupstypes.h"
#include "customcomponent/osdktournaments/tdf/osdktournamentstypes.h"
#include "customcomponent/osdktournaments/tdf/osdktournamentstypes_custom.h"

//#include "gamemanager/stress/matchmakerrulesutil.h"  // for defines SizeRuleType, PredefinedRuleConfiguration, GenericRuleDefinitionList etc



namespace Blaze {
namespace Stress {

using namespace eastl;

class SinglePlayer : public GamePlayUser {
        NON_COPYABLE(SinglePlayer);

    public:
        SinglePlayer(StressPlayerInfo* playerData);
        virtual                     ~SinglePlayer();
        virtual BlazeRpcError       simulateLoad();
        virtual PlayerType          getPlayerType() {
            return SINGLEPLAYER;
        }
		bool						isFUTPlayer()		{ return mIsFutPlayer; }
        virtual void                onLogin(Blaze::BlazeRpcError result);
        //  { /* Push to single player list declared in player module.h */ }
        virtual void                addPlayerToList();
		void						gameTypeSetter();
		enum DivisionType {
			SEASONS =0,
			COOPS,
			CLUBS,
			FRIENDLIES,
			NO_DIVISION
		};
		
    protected:
        SingleplayerGamePlayConfig* mConfig;
        uint32_t                    mViewId;
		ClubId						playerClubId;
		
        void                        removePlayerFromList();
        BlazeRpcError               submitOfflineGameReport();
		uint16_t                    viewStats();
		void                        playSkillgame();
		BlazeRpcError               navigateOfflineMode();
		void						statsExecutions();
		BlazeRpcError				viewSeasonLeaderboards();
		BlazeRpcError				loadCOOPScreen();
		BlazeRpcError				updateStats();
		void						loadSeasonsScreens();
		void						viewCOOPLeaderboards();
		void						viewSeasonHistory();
		void						viewGameHistory();
		BlazeRpcError				viewClubLeaderBoards();
		void						viewDivisonStats(DivisionType divisionType);
		BlazeRpcError				submitSkillGameReport(uint16_t skillGameNum);

    private:
        bool                        mIsInSinglePlayerGame;
		eastl::string				mOfflineGameType;
		bool                        mIsFutPlayer;
        Blaze::TimeValue            mSinglePlayerGameStartTime;
		bool                        mPlaySkillGame;
		uint32_t                    mGameCount;
        static UserIdentMap         singlePlayerList;
        static EA::Thread::Mutex    singlePlayerListMutex;
};

}  // namespace Stress
}  // namespace Blaze

#endif  //   SINGLEPLAYER_H
