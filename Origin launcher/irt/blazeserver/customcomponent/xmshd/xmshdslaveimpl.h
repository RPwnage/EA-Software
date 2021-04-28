/*************************************************************************************************/
/*!
    \file   xmshdslaveimpl.h

    $Header: //gosdev/games/FIFA/2014/Gen3/DEV/blazeserver/13.x/customcomponent/xmshd/xmshdslaveimpl.h#1 $

    \attention
    (c) Electronic Arts 2013. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_XMSHD_SLAVEIMPL_H
#define BLAZE_XMSHD_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "framework/replication/replicatedmap.h"
#include "framework/replication/replicationcallback.h"
#include "framework/usersessions/usersessionmanager.h"

// for stats rollover event
#include "stats/statscommontypes.h"

#include "xmshd/rpc/xmshdslave_stub.h"
#include "xmshd/rpc/xmshdmaster.h"
#include "xmshd/tdf/xmshdtypes.h"
#include "xmshd/tdf/xmshdtypes_server.h"
#include "xmshd/slavemetrics.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace XmsHd
{

class XmsHdSlaveImpl : 
    public XmsHdSlaveStub,
    private UserSessionSubscriber
{
public:
    XmsHdSlaveImpl();
    ~XmsHdSlaveImpl();

    // publish an assert to XMS HD
    BlazeRpcError publishData(PublishDataRequest request);

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
    //! Health monitoring statistics.
    SlaveMetrics mMetrics;

};

} // XmsHd
} // Blaze

#endif // BLAZE_XMSHD_SLAVEIMPL_H

