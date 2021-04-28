/*************************************************************************************************/
/*!
    \file   perfslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_PERF_SLAVEIMPL_H
#define BLAZE_PERF_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "perf/rpc/perfslave_stub.h"
#include "perf/rpc/perfmaster.h"
#include "framework/replication/replicationcallback.h"
#include "framework/usersessions/usersessionsubscriber.h"
#include <EASTL/shared_ptr.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace Perf
{

class PerfSlaveImpl : public PerfSlaveStub
{
public:
	PerfSlaveImpl();
    virtual ~PerfSlaveImpl() override;	
};

} // Perf
} // Blaze

#endif // BLAZE_PERF_SLAVEIMPL_H


