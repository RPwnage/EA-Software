//  *************************************************************************************************
//
//   File:    playermodule.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef PLAYERMODULE_H
#define PLAYERMODULE_H

#include "EABase/eabase.h"
#include "tools/stress/stressmodule.h"
#include "tools/stress/stressinstance.h"

#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "EAStdC/EAString.h"

#include "reference.h"

using namespace eastl;

namespace Blaze {
namespace Stress {

// class StressInstance;
//  class CoopPlayerInstance;
//  class MultiPlayerInstance;
//  class OfflinePlayerInstance;
//  class IdlePlayerInstance;

class Login;
typedef eastl::map<BlazeId, BlazeId> CoopFriendsMap;
typedef eastl::map<BlazeId, BlazeId> FutFriendsMap;
typedef eastl::map<BlazeId, BlazeId> VoltaFriendsMap;

class PlayerModule : public StressModule {
        NON_COPYABLE(PlayerModule);

    public:
        virtual                         ~PlayerModule();

        virtual bool                    initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil = NULL);
        virtual StressInstance*         createInstance(StressConnection* connection, Login* login);
        virtual const char8_t*          getModuleName() {
            return "PlayerModule";
        }
        static  StressModule*           create();

        virtual void                    addPlayerToCommonMap(StressPlayerInfo *playerInfo);
        virtual void                    removePlayerFromCommonMap(StressPlayerInfo *playerInfo);
		const PlayerDetailsMap&			getCommonMap() { return mPlayerDetailsMap; }
        virtual PlayerId                getRandomPlayerId();
        virtual PlayerDetails*          getRandomPlayerFromCommonMap();
        virtual PlayerDetails*          findPlayerInCommonMap(BlazeId playerId);
        virtual PlayerId                getMultiplayerPartyInvite();
        virtual void                    addMultiplayerPartyInvite(PlayerId invitesId, int32_t numInvites);
        virtual void                    deleteMultiplayerPartyInvite(PlayerId invitesId);
		bool							readPlayerGeoIPDataInfo();
		void							addPlayerLocationDataToMap(StressPlayerInfo *playerInfo, LocationInfo location);
		bool							getPlayerLocationDataFromMap(EntityId playerId, LocationInfo& location);
		CreatedGamesList*				getFriendliesGameList()		{ return &mFriendliesCreatedGamesList; }
		CreatedGamesList*				getClubsFriendliesGameList() { return &mClubsFriendliesCreatedGamesList; }
		eastl::vector<eastl::pair<BlazeId, ClubId>>*   getActivelyWaitingJoinersList() { return &activelyWaitingJoinersList;   }
		CoopFriendsList*				getCoopFriendsList()		{ return &mCoopFriendsList; }		
		VoltaFriendsList*				getVoltaFriendsList()		{ return &mVoltaFriendsList; }
		FutFriendsList*					getFutFriendsList()			{ return &mFutFriendsList; }
		CoopFriendsMap&					getCoopFriendsMap()			{ return mCoopFriendsMap; }
		VoltaFriendsMap&				getVoltaFriendsMap()		{ return mVoltaFriendsMap; }
		FutFriendsMap&					getFutFriendsMap()			{ return mFutFriendsMap; }
		CreatedGamesList*				getFutPlayFriendGameList()	{ return &mFutPlayFriendCreatedGamesList; }
		GlobalClubsMap*					getCreatedClubsMap()		{ return &mCreatedClubsMap;	}
		ClubsGameStatusMap&				getClubsStatusMap()			{ return mClubsGameStatusMap; }

		StressGameData                  mDedicatedGamesData;
        StressGameData                  mOnlineSeasonsGamesData;
		StressGameData                  mFriendliesGamesData;
		StressGameData                  mLiveCompetitionsGamesData;
		StressGameData                  mDedicatedServerGamesData;
		StressGameData                  mESLServerGamesData;
        StressGameData                  mCoopPlayerGamesData;
		StressGameData					mFutPlayerGamesData;
		StressGameData					mClubsPlayerGamesData;
		StressGameData					mDropinPlayerGamesData;
		StressGameData					mSsfSeasonsGamesData;
		eastl::vector<eastl::pair<BlazeId, ClubId>>   activelyWaitingJoinersList;
		//PLAYERGEOIPDATA
		CityInfoMap						mCityInfoMap; // values read from file
		CityInfoMap::iterator			mItrator;     //Iterator to mCityInfoMap
		PlayerLocationMap				mPlayerLocationMap;

        //  If new list added, update IsPlayerPresentInList, addPlayerToList and removePlayerFromList functions.
        PlayerMap                       onlineSeasonsMap;
     	PlayerMap                       friendliesMap;
		PlayerMap                       liveCompetitionsMap;
		PlayerMap                       futPlayerMap;
		PlayerMap                       clubsPlayerMap;
		PlayerMap                       dropinPlayerMap;
        PlayerIdList                    coopPlayerList;
		PlayerIdList                    eslPlayerList;
		AccountIdList					eslAccountList;
        PlayerIdList                    mMultiplayerInvites;
		PlayerMap                       ssfSeasonsMap;
		PlayerAccountMap				playerAccountMap;
		//Friendlies Scenaro Game ID list
		// Player Health related data
		EadpStatsMap					eadpStatsMap;
		uint32_t						optIn;
        //  a map for users id and name recieved through NOTIFY_USERADDED notification.
        PlayerMap                       allPlayerInfoMap;
        mutable EA::Thread::Mutex     mMutex;		

    protected:


    private:

        //   Methods
		PlayerModule(){
			mItrator = mCityInfoMap.begin();
		};

        //   Member variables
        int32_t                         mSignalUserIndex;
        //  we have a set of player IDs and hash map of playerId to player details. player id set is needed to find a random player id.
        PlayerIdSet                     mAllPlayerIds;
        PlayerDetailsMap                mPlayerDetailsMap;
		CreatedGamesList				mFriendliesCreatedGamesList;
		CreatedGamesList				mClubsFriendliesCreatedGamesList;
		CoopFriendsList					mCoopFriendsList;
		FutFriendsList					mFutFriendsList;
		VoltaFriendsList				mVoltaFriendsList;
		CoopFriendsMap					mCoopFriendsMap;
		FutFriendsMap					mFutFriendsMap;
		VoltaFriendsMap					mVoltaFriendsMap;
		CreatedGamesList				mFutPlayFriendCreatedGamesList;
		GlobalClubsMap					mCreatedClubsMap;
		ClubsGameStatusMap				mClubsGameStatusMap;
};


}  // namespace Stress
}  // namespace Blaze

#endif  //   PLAYERMODULE_H
