//  *************************************************************************************************
//
//   File:    friendlies.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef FRIENDLIES_H
#define FRIENDLIES_H

#include "./playerinstance.h"
#include "./player.h"
#include "./reference.h"
#include "./gameplayuser.h"
#include "./friendliesgamesession.h"
#include "component/messaging/tdf/messagingtypes.h"

namespace Blaze {
namespace Stress {

using namespace eastl;

class Friendlies : public GamePlayUser, public FriendliesGameSession, public ReportTelemtryFiber {
        NON_COPYABLE(Friendlies);

    public:
        Friendlies(StressPlayerInfo* playerData);
        virtual ~Friendlies();
        virtual BlazeRpcError   simulateLoad();
        void                    addPlayerToList();
        void                    removePlayerFromList();
		void					lobbyCalls();
        virtual PlayerType      getPlayerType() {
            return FRIENDLIES;
        }
        virtual void            deregisterHandlers();
        virtual void            preLogoutAction();
        virtual void            postGameCalls();
        virtual void            onAsync(uint16_t component, uint16_t type, RawBuffer* payload) {
            GameSession::onAsync(component, type, payload);
        }
		BlazeRpcError			joinClub(ClubId clubID);
		
    protected:
        virtual bool            executeReportTelemetry();
        virtual void            simulateInGame();
		BlazeRpcError			startGame();

    private:
        bool                    mIsMarkedForLeaving;
};


}  // namespace Stress
}  // namespace Blaze

#endif  //   FRIENDLIES_H