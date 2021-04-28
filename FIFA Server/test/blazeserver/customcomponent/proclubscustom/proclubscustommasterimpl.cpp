/*************************************************************************************************/
/*!
    \file   proclubscustommasterimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "proclubscustommasterimpl.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/database/dbscheduler.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

// proclubscustom includes
#include "proclubscustom/tdf/proclubscustomtypes.h"

namespace Blaze
{
namespace proclubscustom
{

// static
	proclubscustomMaster* proclubscustomMaster::createImpl()
{
    return BLAZE_NEW_NAMED("ProclubsCustomMasterImpl") ProclubsCustomMasterImpl();
}

/*** Public Methods ******************************************************************************/
ProclubsCustomMasterImpl::ProclubsCustomMasterImpl():
	mDbId(DbScheduler::INVALID_DB_ID)
{
}

ProclubsCustomMasterImpl::~ProclubsCustomMasterImpl()
{
}

bool ProclubsCustomMasterImpl::onConfigure()
{
    TRACE_LOG("[ProclubsCustomMasterImpl:" << this << "].configure: configuring component.");

    return true;
}

} // 
} // Blaze
