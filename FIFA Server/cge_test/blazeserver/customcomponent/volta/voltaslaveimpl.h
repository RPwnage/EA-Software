/*************************************************************************************************/
/*!
    \file   voltaslaveimpl.h

    \attention
    (c) Electronic Arts 2019. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_VOLTA_SLAVEIMPL_H
#define BLAZE_VOLTA_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "framework/replication/replicatedmap.h"
#include "framework/replication/replicationcallback.h"
#include "framework/usersessions/usersessionmanager.h"

#include "volta/rpc/voltaslave_stub.h"
#include "volta/tdf/voltatypes.h"
#include "volta/tdf/voltatypes_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace Volta
{

class VoltaSlaveImpl : 
    public VoltaSlaveStub,
    private UserSessionSubscriber
{
public:
    VoltaSlaveImpl();
    ~VoltaSlaveImpl();

private:

    // component life cycle events
    virtual bool onConfigureReplication();
    virtual bool onConfigure();
    virtual bool onReconfigure();
    virtual bool onResolve();
    virtual void onShutdown();
};

} // Volta
} // Blaze

#endif // BLAZE_VOLTA_SLAVEIMPL_H

