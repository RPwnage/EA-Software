//  *************************************************************************************************
//
//   File:    gameplayuser.h
//
//   Author:  systest
//
//   (c) Electronic Arts. All Rights Reserved.
//
//  *************************************************************************************************

#ifndef GamePLAYUSER_H
#define GamePLAYUSER_H

#include "playerinstance.h"
#include "stressinstance.h"
#include "reference.h"

#include "bytevault/tdf/bytevault.h"
#include "stats/tdf/stats.h"
#include "component/messaging/tdf/messagingtypes.h"
#include "utility.h"
#include "gamesession.h"
#include "statsproxy.h"

using namespace eastl;
using namespace Blaze::OSDKTournaments;

namespace Blaze {
namespace Stress {

class GamePlayUser: public Player {
        NON_COPYABLE(GamePlayUser);
    public:
        GamePlayUser(StressPlayerInfo* playerData);
        virtual                         ~GamePlayUser();
        virtual void                    onAsync(uint16_t component, uint16_t type, RawBuffer* payload);
		virtual void					SetSubscriptionSatatus() {    mCensusdataSubscriptionStatus = true; }
		virtual void					UnsetSubscriptionSatatus() {  mCensusdataSubscriptionStatus = false; }
		
	protected:
        virtual BlazeRpcError           simulateLoad() = 0;
        virtual BlazeRpcError           simulateMenuFlow();
		bool							mCensusdataSubscriptionStatus;

	private:
		StatsProxyHandler*			getStatsProxyHandler() {
			return mStatsProxyHandler;
		}

		StatsProxyHandler*			mStatsProxyHandler;

};

}  // namespace Stress
}  // namespace Blaze

#endif  //   GamePLAYUSER_H
