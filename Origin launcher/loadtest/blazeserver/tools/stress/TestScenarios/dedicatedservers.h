//  *************************************************************************************************
//
//   File:    dedicatedserver.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef DEDICATEDSERVER_H
#define DEDICATEDSERVER_H

#include "./playerinstance.h"
#include "./player.h"
#include "./reference.h"
#include "./dedicatedserversgamesession.h"
#include "./gameplayuser.h"
#include "component/messaging/tdf/messagingtypes.h"

namespace Blaze {
namespace Stress {

using namespace eastl;

class DedicatedServer : public Player, public DedicatedServerGameSession {
        NON_COPYABLE(DedicatedServer);

    public:
        DedicatedServer(StressPlayerInfo* playerData);
        virtual ~DedicatedServer();
        virtual BlazeRpcError   simulateLoad();

        virtual PlayerType      getPlayerType() {
            return DEDICATEDSERVER;
        }
        virtual void            deregisterHandlers();
        virtual void            onAsync(uint16_t component, uint16_t type, RawBuffer* payload) {
            GameSession::onAsync(component, type, payload);
        }
		BlazeRpcError           createGame();
	protected:
        virtual bool            executeReportTelemetry();
        //virtual void            simulateInGame();
	private:
        bool                    mIsMarkedForLeaving;
};


}  // namespace Stress
}  // namespace Blaze

#endif  //   DEDICATEDSERVERS_H

