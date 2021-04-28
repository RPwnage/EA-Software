/*************************************************************************************************/
/*!
    \file   matchcodegenslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class MatchCodeGenSlaveImpl

    MatchCodeGen Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "matchcodegenslaveimpl.h"

// blaze includes
#include "framework/controller/controller.h"

#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

// matchcodegen includes
#include "matchcodegen/rpc/matchcodegenmaster.h"
#include "matchcodegen/tdf/matchcodegentypes.h"

namespace Blaze
{
namespace MatchCodeGen
{


// static
MatchCodeGenSlave* MatchCodeGenSlave::createImpl()
{
    return BLAZE_NEW_NAMED("MatchCodeGenSlaveImpl") MatchCodeGenSlaveImpl();
}


/*** Public Methods ******************************************************************************/
MatchCodeGenSlaveImpl::MatchCodeGenSlaveImpl()
{
}

MatchCodeGenSlaveImpl::~MatchCodeGenSlaveImpl()
{
}

bool MatchCodeGenSlaveImpl::onConfigure()
{
    TRACE_LOG("[MatchCodeGenSlaveImpl].configure: configuring component.");
    return true;
}

} // MatchCodeGen
} // Blaze
