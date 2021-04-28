/*************************************************************************************************/
/*!
    \file   fifastatsslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FifaStatsSlaveImpl

    FifaStats Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifastatsslaveimpl.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/database/dbscheduler.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

// fifastats includes
#include "fifastats/rpc/fifastatsmaster.h"
#include "fifastats/tdf/fifastatstypes.h"
#include "fifastats/tdf/fifastatstypes_server.h"
namespace Blaze
{
namespace FifaStats
{
// static
FifaStatsSlave* FifaStatsSlave::createImpl()
{
    return BLAZE_NEW_NAMED("FifaStatsSlaveImpl") FifaStatsSlaveImpl();
}

/*** Public Methods ******************************************************************************/
FifaStatsSlaveImpl::FifaStatsSlaveImpl()
{
}

FifaStatsSlaveImpl::~FifaStatsSlaveImpl()
{
}

bool FifaStatsSlaveImpl::onConfigure()
{
	bool success = configureHelper();
	TRACE_LOG("[FifaStatsSlaveImpl:" << this << "].onConfigure: configuration " << (success ? "successful." : "failed."));
	return true;
}

bool FifaStatsSlaveImpl::onReconfigure()
{
	bool success = configureHelper();
	TRACE_LOG("[FifaStatsSlaveImpl:" << this << "].onReconfigure: configuration " << (success ? "successful." : "failed."));
	return true;
}

void FifaStatsSlaveImpl::onShutdown()
{
	TRACE_LOG("[FifaStatsSlaveImpl].onShutdown");
}

bool FifaStatsSlaveImpl::configureHelper()
{
	TRACE_LOG("[FifaStatsSlaveImpl].configureHelper");
	return true;
}

} // FifaStats
} // Blaze
