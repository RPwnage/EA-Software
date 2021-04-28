/*************************************************************************************************/
/*!
    \file   perfmasterimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class PerfMasterImpl

    Perf Master implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "perfmasterimpl.h"

// global includes
#include "framework/controller/controller.h"

#include "framework/replication/replicatedmap.h"
#include "framework/replication/replicator.h"

// Perf includes
#include "perf/tdf/perftypes.h"

namespace Blaze
{
namespace Perf
{

// static
PerfMaster* PerfMaster::createImpl()
{
    return BLAZE_NEW_NAMED("PerfMasterImpl") PerfMasterImpl();
}

/*** Public Methods ******************************************************************************/
PerfMasterImpl::PerfMasterImpl()
{
}

PerfMasterImpl::~PerfMasterImpl()
{
}


bool PerfMasterImpl::onConfigure()
{
    TRACE_LOG("[PerfMasterImpl:" << this << "].configure: configuring component.");

    return true;
}

} // Perf
} // Blaze

