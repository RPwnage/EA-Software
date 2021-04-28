/*************************************************************************************************/
/*!
    \file   eaaccessslaveimpl.h

    $Header: //gosdev/games/FIFA/2015/Gen4/DEV/blazeserver/13.x/customcomponent/eaaccess/eaaccessslaveimpl.h#1 $

    \attention
    (c) Electronic Arts 2013. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_EAACCESS_SLAVEIMPL_H
#define BLAZE_EAACCESS_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "framework/replication/replicatedmap.h"
#include "framework/replication/replicationcallback.h"
#include "framework/usersessions/usersessionmanager.h"

// for stats rollover event
#include "stats/statscommontypes.h"

#include "eaaccess/rpc/eaaccessslave_stub.h"
#include "eaaccess/rpc/eaaccessmaster.h"
#include "eaaccess/tdf/eaaccesstypes.h"
#include "eaaccess/tdf/eaaccesstypes_server.h"
#include "eaaccess/slavemetrics.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace EaAccess
{

class EaAccessSlaveImpl : 
    public EaAccessSlaveStub,
    private UserSessionSubscriber
{
public:
    EaAccessSlaveImpl();
    ~EaAccessSlaveImpl();

    BlazeRpcError getEaAccessSubInfo(EaAccessSubInfoRequest request);

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
    virtual void onUserSessionExistence(const Blaze::UserSession& userSession);

    //! Health monitoring statistics.
    SlaveMetrics mMetrics;

};

} // EaAccess
} // Blaze

#endif // BLAZE_EAACCESS_SLAVEIMPL_H

