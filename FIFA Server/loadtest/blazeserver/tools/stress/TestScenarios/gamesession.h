//  *************************************************************************************************
//
//   File:    gamesession.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef GAMESESSION_H
#define GAMESESSION_H

#include "framework/connection/selector.h"
#include "playerinstance.h"
#include "player.h"
#include "playermodule.h"
#include "gamemanager/tdf/gamemanager.h"
#include "gamereporting/tdf/gamereporting.h"
#include "gamereporting/rpc/gamereportingslave.h"
#include "gamemanager/tdf/matchmaker.h"
#include "gamemanager/rpc/gamemanagerslave.h"
#include "customcomponent/perf/rpc/perfslave.h"
//#include "reference.h"
#include "osdktournaments/tdf/osdktournamentstypes.h"
#include "clubs/tdf/clubs_base.h"

namespace Blaze {
namespace Stress {
using namespace Blaze::GameManager;
using namespace Blaze::OSDKTournaments;
using namespace eastl;
//class Blaze::Stress::StressPlayerInfo;
class StressPlayerInfo;

class GameSession {
        NON_COPYABLE(GameSession);

    public:
		GameSession();
        virtual ~GameSession();
        bool                                 isTopologyHost;
        bool                                 isPlatformHost;
		bool                                 isPlatformGameGroupHost;
        eastl::string                        mGameNpSessionId;

        inline uint32_t                      getGameSize()                                      {
            return mGameSize;
        }
        inline GameNetworkTopology           getGameNetworkTopology()                           {
            return mTopology;
        }
        inline GameProtocolVersionString     getGameProtocolVersionString()                     {
            return mGameProtocolVersionString;
        }
        inline GameState                     getPlayerState()                                   {
            return mState;
        }
        inline void                          setGameId(GameId id)                               {
            myGameId = id;
        }
        inline GameId                        getGameId()                                        {
            return myGameId;
        }
        inline void                          setPlayerState(GameState state)                    {
            mPrevState = mState;
            mState = state;
        }
        inline StressPlayerInfo*             getStressPlayerInfo()                              {
            return mPlayerData;
        }
        inline void                          initializePlayerData(StressPlayerInfo* playerData) {
            mPlayerData = playerData;
        }
        //           bool                          getDoPing()                                        { return doPing; }

        virtual void						onAsync(uint16_t component, uint16_t type, RawBuffer* payload);
        virtual void                        asyncHandlerFunction(uint16_t type, EA::TDF::Tdf* notification);
        virtual BlazeRpcError               cancelMatchMakingScenario(MatchmakingSessionId sessionId);
        virtual BlazeRpcError               replayGame();
        inline void                         resetInGameStartTime()                              {
            mInGameStartTime = 0;
        }
        inline void                         setInGameStartTime()                                {
            mInGameStartTime = TimeValue::getTimeOfDay().getMillis();
        }
        bool								IsInGameTimedOut();
		
		inline uint32_t					getCupId(){
			return mCupId;
		}
		inline void						setCupId(uint32_t id){
            mCupId = id;
        }
		inline Blaze::Clubs::ClubId					getMyClubId(){
			return mClubID;
		}
		inline void						setMyClubId(Blaze::Clubs::ClubId id){
            mClubID = id;
        }
		inline OSDKTournaments::TournamentId getTournamentId(){
			return mTournamentId;
		}
		inline void						setTournamentId(OSDKTournaments::TournamentId id){
            mTournamentId = id;
        }

    protected:
        GameSession(StressPlayerInfo* PlayerData, uint32_t GameSize, GameNetworkTopology Topology, GameProtocolVersionString GameProtocolVersionString);
		void setGameName(eastl::string name);

		//  Define the below functions in the derived classes.
        virtual BlazeRpcError	advanceGameState(GameState state, GameId gameId = 0);
        void                finalizeGameCreation(GameId gameId = 0);
		BlazeRpcError		setPlayerAttributes(const char8_t* key, const char8_t*  value, GameId gameId = 0);
		BlazeRpcError		setPlayerAttributes(GameId  gameId, Collections::AttributeMap &attributes);
		void				setGameSettings(uint32_t bits);
		BlazeRpcError		setGameAttributes(GameId  gameId, Collections::AttributeMap &gameAttributes);
        void                updatePrimaryExternalSessionForUserStub(GameId gameId);
        BlazeRpcError       updatePrimaryExternalSessionForUser(GameActivityList* gameActivityList);

		virtual void		meshEndpointsConnected(BlazeObjectId& targetGroupId, uint32_t latency = 0, uint32_t connFlags = 0, GameId gameId = 0);
		virtual void		meshEndpointsDisconnected(BlazeObjectId& targetGroupId, PlayerNetConnectionStatus disconnectStat = DISCONNECTED, uint32_t connFlags = 4,  GameId gameId = 0);
		virtual void		meshEndpointsConnectionLost( BlazeObjectId& targetGroupId, uint32_t connFlags = 0,GameId gameId = 0);

        virtual BlazeRpcError               leaveGameByGroup(PlayerId playerId, PlayerRemovedReason reason, GameId gameId = 0);
		virtual BlazeRpcError				returnDedicatedServerToPool();
        virtual BlazeRpcError               preferredJoinOptOut(GameId gameId);
        virtual BlazeRpcError               removePlayer(PlayerRemovedReason reason);
        virtual BlazeRpcError               updateGameSession(GameId gameId = 0);
        virtual uint32_t                    getInGameDurationFromConfig() = 0;
        virtual uint32_t                    getInGameDurationDeviationFromConfig() = 0;
        virtual void                        deregisterHandlers();

        uint32_t                            mGameSize;
        GameNetworkTopology                 mTopology;
        GameProtocolVersionString           mGameProtocolVersionString;
        GameState                           mState;
        GameState                           mPrevState;
        GameId                              myGameId;
        MatchmakingSessionId                mMatchmakingSessionId;
		PlayerInGameMap						mPlayerInGameGroupMap;
        bool                                doPing;

        //  Don't delete this pointer in the destructor;
        //  Instead of below pointer use playerdata struture.
        StressPlayerInfo*           mPlayerData;
        int64_t                     mInGameStartTime;
		char						mGameName[MAX_GAMENAME_LEN];

        GameActivity                mGameActivity;

	private:
		uint32_t						mCupId;
		Blaze::Clubs::ClubId			mClubID;
		OSDKTournaments::TournamentId	mTournamentId;
};

}  // namespace Stress
}  // namespace Blaze

#endif  //   GAMESESSION_H
