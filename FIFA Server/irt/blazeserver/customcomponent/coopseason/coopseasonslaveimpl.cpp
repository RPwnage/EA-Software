/*************************************************************************************************/
/*!
    \file   coopseasonslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class CoopSeasonSlaveImpl

    Coop Season Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "coopseasonslaveimpl.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/database/dbscheduler.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

// coopseason includes
#include "coopseason/rpc/coopseasonmaster.h"
#include "coopseason/tdf/coopseasontypes.h"
#include "coopseason/tdf/coopseasontypes_server.h"
#include "coopseason/coopseasonidentityprovider.h"


namespace Blaze
{
namespace CoopSeason
{
// static
CoopSeasonSlave* CoopSeasonSlave::createImpl()
{
    return BLAZE_NEW_NAMED("CoopSeasonSlaveImpl") CoopSeasonSlaveImpl();
}


/*** Public Methods ******************************************************************************/
CoopSeasonSlaveImpl::CoopSeasonSlaveImpl():
    mDbId(DbScheduler::INVALID_DB_ID),
	mbUseCache(false)
{
	mIdentityProvider = BLAZE_NEW CoopSeasonIdentityProvider(this);
	gIdentityManager->registerProvider(COMPONENT_ID, *mIdentityProvider);
}

CoopSeasonSlaveImpl::~CoopSeasonSlaveImpl()
{
	delete mIdentityProvider;
}

void CoopSeasonSlaveImpl::getAllCoopIds(BlazeId targetId, Blaze::CoopSeason::CoopPairDataList &outCoopPairDataList, uint32_t &outCount, BlazeRpcError &outError)
{
	CoopSeasonDb dbHelper = CoopSeasonDb(this);

	DbConnPtr dbConn = gDbScheduler->getReadConnPtr(getDbId());
	outError = dbHelper.setDbConnection(dbConn);

	if (outError == Blaze::ERR_OK)
	{
		outError = dbHelper.fetchAllCoopIds(targetId, outCount, outCoopPairDataList);
	}
}

void CoopSeasonSlaveImpl::getCoopIdDataByCoopId(uint64_t coopId, Blaze::CoopSeason::CoopPairData &outCoopPairData, BlazeRpcError &outError)
{
	CoopSeasonDb dbHelper = CoopSeasonDb(this);

	DbConnPtr dbConn = gDbScheduler->getReadConnPtr(getDbId());
	outError = dbHelper.setDbConnection(dbConn);

	if (outError == Blaze::ERR_OK)
	{
		outError = dbHelper.fetchCoopIdDataByCoopId(coopId, outCoopPairData);
	}
}

void CoopSeasonSlaveImpl::getCoopIdDataByBlazeIds(BlazeId memberOneId, BlazeId memberTwoId, Blaze::CoopSeason::CoopPairData &outCoopPairData, BlazeRpcError &outError)
{
	CoopSeasonDb dbHelper = CoopSeasonDb(this);

	DbConnPtr dbConn = gDbScheduler->getReadConnPtr(getDbId());
	outError = dbHelper.setDbConnection(dbConn);

	if (outError == Blaze::ERR_OK)
	{
		outError = dbHelper.fetchCoopIdDataByBlazeIds(memberOneId, memberTwoId, outCoopPairData);
	}
}

void CoopSeasonSlaveImpl::removeCoopIdDataByCoopId(uint64_t coopId, BlazeRpcError &outError)
{
	CoopSeasonDb dbHelper = CoopSeasonDb(this);

	DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
	outError = dbHelper.setDbConnection(dbConn);

	if (outError == Blaze::ERR_OK)
	{
		outError = dbHelper.removeCoopIdDataByCoopId(coopId);
	}
}

void CoopSeasonSlaveImpl::removeCoopIdDataByBlazeIds(BlazeId memberOneId, BlazeId memberTwoId, BlazeRpcError &outError)
{
	CoopSeasonDb dbHelper = CoopSeasonDb(this);

	DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
	outError = dbHelper.setDbConnection(dbConn);

	if (outError == Blaze::ERR_OK)
	{
		outError = dbHelper.removeCoopIdDataByBlazeIds(memberOneId, memberTwoId);
	}
}

void CoopSeasonSlaveImpl::setCoopIdData(BlazeId memberOneId, BlazeId memberTwoId, const char8_t* metaData, Blaze::CoopSeason::CoopPairData &outCoopPairData, BlazeRpcError &outError)
{
	CoopSeasonDb dbHelper = CoopSeasonDb(this);

	DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
	outError = dbHelper.setDbConnection(dbConn);

	if (outError == Blaze::ERR_OK)
	{
		Blaze::CoopSeason::CoopPairData coopPairDataObj;
		outError = dbHelper.fetchCoopIdDataByBlazeIds(memberOneId, memberTwoId, coopPairDataObj);

		if (coopPairDataObj.getCoopId() != 0)
		{
			outError = dbHelper.updateCoopIdData(memberOneId, memberTwoId, metaData);
		}
		else
		{
			outError = dbHelper.insertCoopIdData(memberOneId, memberTwoId, metaData, outCoopPairData);
		}
	}
}

bool CoopSeasonSlaveImpl::onConfigure()
{
	bool success = configureHelper();
	TRACE_LOG("[CoopSeasonSlaveImpl:" << this << "].onConfigure: configuration " << (success ? "successful." : "failed."));
    return true;
}

void CoopSeasonSlaveImpl::onShutdown()
{
    gIdentityManager->deregisterProvider(COMPONENT_ID);
}

bool CoopSeasonSlaveImpl::configureHelper()
{
	TRACE_LOG("[CoopSeasonSlaveImpl].configureHelper");

	const char8_t* dbName = getConfig().getDbName();
	mDbId = gDbScheduler->getDbId(dbName);
	if(mDbId == DbScheduler::INVALID_DB_ID)
	{
		ERR_LOG("[CoopSeasonSlaveImpl].configureHelper(): Failed to initialize db");
		return false;
	}

	CoopSeasonDb::initialize(mDbId);

	return true;
}

void CoopSeasonSlaveImpl::onVoidMasterNotification(UserSession *)
{
	TRACE_LOG("[CoopSeasonSlaveImpl:" << this << "].onVoidMasterNotification");
}


} // CoopSeason
} // Blaze
