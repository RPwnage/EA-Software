/*************************************************************************************************/
/*!
    \file   eaaccessslaveimpl.cpp

    $Header: //gosdev/games/FIFA/2015/Gen4/DEV/blazeserver/13.x/customcomponent/eaaccess/eaaccessslaveimpl.cpp#2 $

    \attention
    (c) Electronic Arts 2013. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class EaAccessSlaveImpl

    XmsHd Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "framework/connection/selector.h"
#include "eaaccessslaveimpl.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/database/dbscheduler.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

#include "component/stats/statsconfig.h"
#include "component/stats/statsslaveimpl.h"


// easfc includes
#include "eaaccess/rpc/eaaccessmaster.h"


namespace Blaze
{
namespace EaAccess
{


// static
EaAccessSlave* EaAccessSlave::createImpl()
{
    return BLAZE_NEW EaAccessSlaveImpl();
}


/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief  constructor

    

*/
/*************************************************************************************************/
EaAccessSlaveImpl::EaAccessSlaveImpl() :
    mMetrics()
{
}


/*************************************************************************************************/
/*!
    \brief  destructor

    

*/
/*************************************************************************************************/
EaAccessSlaveImpl::~EaAccessSlaveImpl()
{
}

/*************************************************************************************************/
/*!
    \brief  getEaAccessSubInfo

    publish an assert to XMS HD

*/
/*************************************************************************************************/
BlazeRpcError EaAccessSlaveImpl::getEaAccessSubInfo(EaAccessSubInfoRequest request)
{
	TRACE_LOG("[EaAccessSlaveImpl].getEaAccessSubInfo.");

	BlazeRpcError error = Blaze::ERR_OK;

	// record metrics
	mMetrics.mGetEaAccessSubInfo.add(1);

	return error;
}

/*************************************************************************************************/
/*!
    \brief  onConfigureReplication

    lifecycle method: called when the component is ready to listen to replicated events.
    Register as a ReplicatedSeasonConfigurationMap subscriber.

*/
/*************************************************************************************************/
bool EaAccessSlaveImpl::onConfigureReplication()
{
    TRACE_LOG("[EaAccessSlaveImpl].onConfigureReplication");

    return true;
}


/*************************************************************************************************/
/*!
    \brief  onConfigure

    lifecycle method: called when the slave is to configure itself. registers for stats
    and usersession events. 

*/
/*************************************************************************************************/
bool EaAccessSlaveImpl::onConfigure()
{
    TRACE_LOG("[EaAccessSlaveImpl].onConfigure");

    return true;
}


/*************************************************************************************************/
/*!
    \brief  onReconfigure

    lifecycle method: called when the slave is to reconfigure itself. unregisters from stats
    and usersession events before calling onConfigure

*/
/*************************************************************************************************/
bool EaAccessSlaveImpl::onReconfigure()
{
    TRACE_LOG("[EaAccessSlaveImpl].onReconfigure");

    // stop listening to rollover and user session events as we'll re-subscribe to them in onConfigure

    return onConfigure();
}


/*************************************************************************************************/
/*!
    \brief  onResolve

    lifecycle method: called when the slave is to resolve itself. registers for master 
    notifications

*/
/*************************************************************************************************/
bool EaAccessSlaveImpl::onResolve()
{
    TRACE_LOG("[EaAccessSlaveImpl].onResolve");

    return true;
}


/*************************************************************************************************/
/*!
    \brief  onShutdown

    lifecycle method: called when the slave is shutting down. need to unhook our event listeners

*/
/*************************************************************************************************/
void EaAccessSlaveImpl::onShutdown()
{
    TRACE_LOG("[EaAccessSlaveImpl].onShutdown");
}

/*************************************************************************************************/
/*!
    \brief  getStatusINFO_LOG(

    Called periodically to collect status and statistics about this component that can be used 
    to determine what future improvements should be made.

*/
/*************************************************************************************************/
void EaAccessSlaveImpl::getStatusInfo(ComponentStatus& status) const
{
    TRACE_LOG("[EaAccessSlaveImpl].getStatusInfo");

    mMetrics.report(&status);
}


/*************************************************************************************************/
/*!
    \brief  onUserSessionExistence

    called when a user logs into the slave. 

*/
/*************************************************************************************************/
void EaAccessSlaveImpl::onUserSessionExistence(const Blaze::UserSession& userSession)
{
    TRACE_LOG("[EaAccessSlaveImpl].onUserSessionExistence.");

    // only care about this login event if the user is logged into this slave from a console
    if (true == userSession.isLocal() && 
        CLIENT_TYPE_GAMEPLAY_USER == userSession.getClientType())
    {
        TRACE_LOG("[EaAccessSlaveImpl].onUserSessionExistence. User logged in. User id = " << userSession.getUserId() );
    }
}
} // EaAccess
} // Blaze
