/*************************************************************************************************/
/*!
    \file   voltaslaveimpl.cpp

    \attention
    (c) Electronic Arts 2019. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "framework/connection/selector.h"
#include "voltaslaveimpl.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/database/dbscheduler.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

namespace Blaze
{
namespace Volta
{


// static
VoltaSlave* VoltaSlave::createImpl()
{
    return BLAZE_NEW VoltaSlaveImpl();
}


/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief  constructor

    

*/
/*************************************************************************************************/
VoltaSlaveImpl::VoltaSlaveImpl()
{
}


/*************************************************************************************************/
/*!
    \brief  destructor

    

*/
/*************************************************************************************************/
VoltaSlaveImpl::~VoltaSlaveImpl()
{
}

/*************************************************************************************************/
/*!
    \brief  onConfigureReplication

    lifecycle method: called when the component is ready to listen to replicated events.
    Register as a ReplicatedSeasonConfigurationMap subscriber.

*/
/*************************************************************************************************/
bool VoltaSlaveImpl::onConfigureReplication()
{
    TRACE_LOG("[VoltaSlaveImpl].onConfigureReplication");

    return true;
}


/*************************************************************************************************/
/*!
    \brief  onConfigure

    lifecycle method: called when the slave is to configure itself. registers for stats
    and usersession events. 

*/
/*************************************************************************************************/
bool VoltaSlaveImpl::onConfigure()
{
    TRACE_LOG("[VoltaSlaveImpl].onConfigure");

    return true;
}


/*************************************************************************************************/
/*!
    \brief  onReconfigure

    lifecycle method: called when the slave is to reconfigure itself. unregisters from stats
    and usersession events before calling onConfigure

*/
/*************************************************************************************************/
bool VoltaSlaveImpl::onReconfigure()
{
    TRACE_LOG("[VoltaSlaveImpl].onReconfigure");

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
bool VoltaSlaveImpl::onResolve()
{
    TRACE_LOG("[VoltaSlaveImpl].onResolve");

    return true;
}


/*************************************************************************************************/
/*!
    \brief  onShutdown

    lifecycle method: called when the slave is shutting down. need to unhook our event listeners

*/
/*************************************************************************************************/
void VoltaSlaveImpl::onShutdown()
{
    TRACE_LOG("[VoltaSlaveImpl].onShutdown");
}

} // Volta
} // Blaze
