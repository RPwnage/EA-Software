//  *************************************************************************************************
//
//   File:    player.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef PLAYER_H
#define PLAYER_H

#include "playerinstance.h"
#include "stressinstance.h"
#include "reference.h"
//#include "gamesession.h"

#include "bytevault/tdf/bytevault.h"
#include "stats/tdf/stats.h"
#include "loginmanager.h"

namespace Blaze {
namespace Stress {

using namespace eastl;
//class StressPlayerInfo;
//class PlayerDetails;

//class PlayerInstance;
//class StressPlayerInfo;
//class ComponentProxy;

typedef eastl::vector<eastl::string> StringList;

class Player {
        NON_COPYABLE(Player);

    public:
		Player();
        Player(StressPlayerInfo* playerData, ClientType type);
        virtual ~Player();
        virtual BlazeRpcError       simulateLoad()     = 0;

        void sleep(uint32_t timInMs);
        void sleepRandomTime(uint32_t minMs, uint32_t maxMs);
        void wakeFromSleep();
        inline  ClientType          getClientType()         {
            return mClientType;
        }
        inline  bool                isPlayerLoggedIn()     {
            return mPlayerInfo->getLogin()->isLoggedIn();
        }
		inline  bool                isPlayerLocationSet()      {
            return mLocationSet;
        }
		inline  void                setPlayerLocation(bool value)      {
            mLocationSet = value;
        }

        virtual void                addPlayerToList();
        virtual void                removePlayerFromList();
        virtual BlazeRpcError       loginPlayer();
        virtual BlazeRpcError       logoutPlayer();
        virtual PlayerType          getPlayerType() = 0;
        virtual void                onDisconnected();
        virtual void                deregisterHandlers()    { }
        virtual void                onAsync(uint16_t component, uint16_t type, RawBuffer* payload);
		BlazeRpcError				preAuthRPCcalls();

        bool                        isNewPlayerSession() {
            return mIsNewPlayerSession;
        }
		void                        setNewPlayerSession(bool state) {
			mIsNewPlayerSession = state;
		}
        bool                        isLoggedInForTooLong();
		void						setPlayerCityLocation();

    protected:
       
        StressPlayerInfo*  mPlayerInfo;
        Fiber::EventHandle          mSleepEvent;
        bool                        mProcessInviteFiberExited;
        bool                        mDestructing;
		bool                        mLocationSet;

        //   Authentication
        virtual BlazeRpcError       preAuth();
        virtual BlazeRpcError       postAuth();

        //   Virtual callbacks
        virtual void                onLogin(BlazeRpcError result);
        virtual void                onLogout(BlazeRpcError result);
        virtual void                renewPlayer();
        virtual void                invitesProcessingFiber();
        virtual void                processInvites(Blaze::Stress::PlayerDetails* myDetails);
    private:
        ClientType                  mClientType;
        bool                        mIsNewPlayerSession;
};

}  // namespace Stress
}  // namespace Blaze

#endif  //   PLAYER_H
