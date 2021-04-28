//  *************************************************************************************************
//
//   File:    kickoff.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef KICKOFF_H
#define KICKOFF_H

#include "eathread/eathread_mutex.h"

#include "playerinstance.h"
#include "gameplayuser.h"
#include "reference.h"
#include "utility.h"
#include "statsproxy.h"
#include "groupsproxy.h"

#include "EASTL/map.h"
#include "EASTL/vector_set.h"
#include "stressmodule.h"
#include "EATDF/time.h"
#include "framework/tdf/attributes.h"
#include "stressinstance.h"
#include "gameplayuser.h"
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

namespace Blaze {
namespace Stress {

using namespace eastl;

typedef enum
{
	TEAM_1_1 = 0, 
	TEAM_1_2,
	TEAM_2_2,
	TEAM_1_CPU,
	TEAM_2_CPU
} KO_Scenario;

typedef enum
{
	TEAM_ALL_REG = 0,
	//TEAM_REG_UNREG,
	TEAM_ALL_UNREG
} KO_PlayersType;

typedef enum 
{
	EASY = 0,
	NORMAL,
	DIFFICULT,
	HARD
} DifficultyLevel;

typedef enum 
{
	EADP_GROUPS_FRIEND_TYPE = 0,
	EADP_GROUPS_OVERALL_TYPE_A,
	EADP_GROUPS_OVERALL_TYPE_B,
	EADP_GROUPS_RIVAL_TYPE
} EADPGroupsType;

class KickOffPlayer : public GamePlayUser {
        NON_COPYABLE(KickOffPlayer);

    public:
		KickOffPlayer(StressPlayerInfo* playerData);
        virtual                     ~KickOffPlayer();
        virtual BlazeRpcError       simulateLoad();
        virtual PlayerType          getPlayerType() {
            return KICKOFFPLAYER;
        }
		void						gameTypeSetter();
        //  { /* Push to single player list declared in player module.h */ }
        virtual void                addPlayerToList();
		virtual void                onLogin(Blaze::BlazeRpcError result);
		BlazeRpcError				lobbyRpcCalls();
		GroupsProxyHandler*			getGroupsProxyHandler() {
			return mGroupsProxyHandler;
		}
		StatsProxyHandler*			getStatsProxyHandler() {
			return mStatsProxyHandler;
		}
		void						setNucleusAccessToken(const eastl::string &accessToken) {
			mNucleusAccessToken = accessToken;
		}
		const eastl::string&		getNucleusAccessToken() {
			return mNucleusAccessToken;
		}		
		DifficultyLevel				getDifficultyLevel() {
			return mLevel;
		}
		KO_Scenario					getScearioType() {
			return mScenarioType;
		}
		KO_PlayersType				getKO_PlayersType() {
			return mPlayersType;
		}
		void						initScenarioType();
		void						initPlayersType();
		void						initDifficultyLevel();
		bool						initFriendPlayerIDs();
		bool						createAndJoinGroups();	
		bool						getMyAccessToken();
		eastl::string				getGroupsType(EADPGroupsType type);
		bool						createGroupNames();
		bool						readGroupIDs();
		bool						searchGroupMembers();
		bool						viewStats();
		BlazeRpcError				createInstance(EADPGroupsType type);
		BlazeRpcError				setMultipleInstanceAttributes(EADPGroupsType type);
		BlazeRpcError				joinGroup(eastl::string guid, PlayerId playerID);
		BlazeRpcError				updateStats_GroupID(EADPGroupsType type);
		
    protected:
		KickoffplayerGamePlayConfig* mConfig;
        uint32_t                    mViewId;	
        void                        removePlayerFromList();
        BlazeRpcError               submitOfflineGameReport();

    private:
        Blaze::TimeValue            mSinglePlayerGameStartTime;
		eastl::string				mNucleusAccessToken;
		BlazeIdList					mRegisteredPlayersList;
		StringList					mUnRegisteredStringPlayersList;

		eastl::string				mFriendsGroup_Name;
		eastl::string				mOverallGroupA_Name;
		eastl::string				mOverallGroupB_Name;
		eastl::string				mRivalGroup_Name;

		eastl::string				mFriendsGroup_ID;
		eastl::string				mOverallGroupA_ID;
		eastl::string				mOverallGroupB_ID;
		eastl::string				mRivalGroup_ID;

		DifficultyLevel				mLevel;
		KO_Scenario					mScenarioType;
		KO_PlayersType				mPlayersType;

		GroupsProxyHandler*			mGroupsProxyHandler;
		StatsProxyHandler*			mStatsProxyHandler;

        static UserIdentMap         kickOffPlayerList;
		static EA::Thread::Mutex	kickOffPlayerListMutex;

		eastl::string				mKickOffScenarioType;

};

}  // namespace Stress
}  // namespace Blaze

#endif  //   KickOffPlayer_H
