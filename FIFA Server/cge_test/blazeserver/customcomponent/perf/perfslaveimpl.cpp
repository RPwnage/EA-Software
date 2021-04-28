// (c) Electronic Arts.  All Rights Reserved.

// main includes
#include "framework/blaze.h"
#include "perfslaveimpl.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/usersessions/usersession.h"

// perf includes
#include "perf/rpc/perfmaster.h"
#include "perf/tdf/perftypes.h"

namespace Blaze
{
namespace Perf
{

// static
PerfSlave* PerfSlave::createImpl()
{
    return BLAZE_NEW_NAMED("PerfSlaveImpl") PerfSlaveImpl();
}

/// Constructor
PerfSlaveImpl::PerfSlaveImpl()
{
}

/// Destructor
PerfSlaveImpl::~PerfSlaveImpl()
{
}

} // Perf
} // Blaze

