/*************************************************************************************************/
/*!
    \file   xmshdslaveimpl.cpp

    $Header: //gosdev/games/FIFA/2014/Gen3/DEV/blazeserver/13.x/customcomponent/xmshd/xmshdslaveimpl.cpp#2 $

    \attention
    (c) Electronic Arts 2013. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class XmsHdSlaveImpl

    XmsHd Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "framework/connection/selector.h"
#include "xmshdslaveimpl.h"

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
#include "xmshd/rpc/xmshdmaster.h"


namespace Blaze
{
namespace XmsHd
{


// static
XmsHdSlave* XmsHdSlave::createImpl()
{
    return BLAZE_NEW XmsHdSlaveImpl();
}


/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief  constructor

    

*/
/*************************************************************************************************/
XmsHdSlaveImpl::XmsHdSlaveImpl() :
    mMetrics()
{
}


/*************************************************************************************************/
/*!
    \brief  destructor

    

*/
/*************************************************************************************************/
XmsHdSlaveImpl::~XmsHdSlaveImpl()
{
}

/*************************************************************************************************/
/*!
    \brief  publishData

    publish an assert to XMS HD

*/
/*************************************************************************************************/
BlazeRpcError XmsHdSlaveImpl::publishData(PublishDataRequest request)
{
	TRACE_LOG("[XmsHdSlaveImpl].publishData. nucleusId = ");

	BlazeRpcError error = Blaze::ERR_OK;

	// record metrics
	mMetrics.mPublishData.add(1);

	return error;
}

/*************************************************************************************************/
/*!
    \brief  onConfigureReplication

    lifecycle method: called when the component is ready to listen to replicated events.
    Register as a ReplicatedSeasonConfigurationMap subscriber.

*/
/*************************************************************************************************/
bool XmsHdSlaveImpl::onConfigureReplication()
{
    TRACE_LOG("[XmsHdSlaveImpl].onConfigureReplication");

    return true;
}


/*************************************************************************************************/
/*!
    \brief  onConfigure

    lifecycle method: called when the slave is to configure itself. registers for stats
    and usersession events. 

*/
/*************************************************************************************************/
bool XmsHdSlaveImpl::onConfigure()
{
    TRACE_LOG("[XmsHdSlaveImpl].onConfigure");

    return true;
}


/*************************************************************************************************/
/*!
    \brief  onReconfigure

    lifecycle method: called when the slave is to reconfigure itself. unregisters from stats
    and usersession events before calling onConfigure

*/
/*************************************************************************************************/
bool XmsHdSlaveImpl::onReconfigure()
{
    TRACE_LOG("[XmsHdSlaveImpl].onReconfigure");

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
bool XmsHdSlaveImpl::onResolve()
{
    TRACE_LOG("[XmsHdSlaveImpl].onResolve");

    return true;
}


/*************************************************************************************************/
/*!
    \brief  onShutdown

    lifecycle method: called when the slave is shutting down. need to unhook our event listeners

*/
/*************************************************************************************************/
void XmsHdSlaveImpl::onShutdown()
{
    TRACE_LOG("[XmsHdSlaveImpl].onShutdown");
}

/*************************************************************************************************/
/*!
    \brief  getStatusINFO_LOG(

    Called periodically to collect status and statistics about this component that can be used 
    to determine what future improvements should be made.

*/
/*************************************************************************************************/
void XmsHdSlaveImpl::getStatusInfo(ComponentStatus& status) const
{
    TRACE_LOG("[XmsHdSlaveImpl].getStatusInfo");

    mMetrics.report(&status);
}


/*************************************************************************************************/
/*!
    \brief  onUserSessionExistence

    called when a user logs into the slave. 

*/
/*************************************************************************************************/
void XmsHdSlaveImpl::onUserSessionExistence(const UserSession& userSession)
{
	TRACE_LOG("[XmsHdSlaveImpl].onUserSessionExistence.");

    // only care about this login event if the user is logged into this slave from a console
    if (true == userSession.isLocal() && 
        CLIENT_TYPE_GAMEPLAY_USER == userSession.getClientType())
    {
        TRACE_LOG("[XmsHdSlaveImpl].verifyUserRegistration. User logged in. User id = "<<userSession.getUserId());
    }
}
} // XmsHd
} // Blaze
