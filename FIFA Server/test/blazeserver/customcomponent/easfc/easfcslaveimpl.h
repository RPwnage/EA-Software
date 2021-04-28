/*************************************************************************************************/
/*!
    \file   easfcslaveimpl.h

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/easfc/easfcslaveimpl.h#1 $

    \attention
    (c) Electronic Arts 2012. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_EASFC_SLAVEIMPL_H
#define BLAZE_EASFC_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "framework/replication/replicatedmap.h"
#include "framework/replication/replicationcallback.h"
#include "framework/usersessions/usersessionmanager.h"

// for stats rollover event
#include "stats/statscommontypes.h"

#include "easfc/rpc/easfcslave_stub.h"
#include "easfc/rpc/easfcmaster.h"
#include "easfc/tdf/easfctypes.h"
#include "easfc/tdf/easfctypes_server.h"
#include "easfc/slavemetrics.h"

#include "framework/database/dbconn.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace EASFC
{

class EasfcSlaveImpl : 
    public EasfcSlaveStub,
    private UserSessionSubscriber
{
public:
    EasfcSlaveImpl();
    ~EasfcSlaveImpl();


    // purchase a game win
    BlazeRpcError purchaseGameWin(PurchaseGameRequest request);

	// purchase a game draw
	BlazeRpcError purchaseGameDraw(PurchaseGameRequest request);

	// purchase a game loss
	BlazeRpcError purchaseGameLoss(PurchaseGameRequest request);

	// purchase a game match
	BlazeRpcError purchaseGameMatch(PurchaseGameRequest request);

	// fill normal game stats table
	BlazeRpcError updateNormalGameStats(const char* userStatsCategory, EntityId id, const NormalGameStatsList& normalGameStatsList);

	// fill seasonal play div and overall stats tables
	BlazeRpcError updateSPStats(const char* userStatsCategory, EntityId entId, const StatNameValueMapInt* pStatMap);

	// sets vpro stats for a given user
	BlazeRpcError updateVproStats(const char* userStatsCategory, EntityId entId, const StatNameValueMapStr* vproStatMap);

	// fill seasonal play div and overall stats tables
	BlazeRpcError applySeasonsRewards(EntityId entId, const StatNameValueMapInt* pSPOValues);

	// fill previous seasonal play overall stats table
	BlazeRpcError writeToSPPrevOverallStats(EntityId entId, const StatNameValueMapInt* pSPOValues, NormalGameStatsList& normalGameStatsList);

	// sets vpro stats for a given user
	BlazeRpcError updateVproIntStats(const char* userStatsCategory, EntityId entId, const StatNameValueMapInt* vproStatMap);

	// helper to verify if hdkc upgrades were already applied 
	bool isUpgradeTitle(EntityId id, const char* statCategoryName);

	enum SeasonsItemState
	{
		ITEM_NOT_REDEEMED,
		ITEM_REDEEMED
	};

private:

    // component life cycle events
    virtual bool onConfigureReplication();
    virtual bool onConfigure();
    virtual bool onReconfigure();
    virtual bool onResolve();
    virtual void onShutdown();

    //! Called periodically to collect status and statistics about this component that can be used to determine what future improvements should be made.
    virtual void getStatusInfo(ComponentStatus& status) const;

    // event handlers for User Session Manager events
    virtual void onUserSessionExistence(const UserSession& userSession);

	// helper to update the upgradeTitle flag for hdkc
	void setUpgradeTitleColumn(Stats::StatRowUpdate* rowUpdate, Stats::StatUpdate* statUpdate);

	// helper to write column to a statRow
	void writeToColumn(const char8_t* name, int value, Stats::StatRowUpdate* statRow);

    //! Health monitoring statistics.
    SlaveMetrics mMetrics;

};

} // EASFC
} // Blaze

#endif // BLAZE_EASFC_SLAVEIMPL_H

