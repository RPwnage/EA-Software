/*************************************************************************************************/
/*!
    \file   getstatsbygroup_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetStatsByGroupCommand

    Retrieves statistics for pre-defined stat columns.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "statsslaveimpl.h"
#include "stats/rpc/statsslave/getstatsbygroup_stub.h"
#include "getstatsbygroupbase.h"

namespace Blaze
{
namespace Stats
{
class GetStatsByGroupCommand : public GetStatsByGroupCommandStub, GetStatsByGroupBase
{
public:

    GetStatsByGroupCommand(Message* message, GetStatsByGroupRequest* request, StatsSlaveImpl* componentImpl);
    ~GetStatsByGroupCommand() override {}
    
private:

    // States
    GetStatsByGroupCommandStub::Errors execute() override;
};

DEFINE_GETSTATSBYGROUP_CREATE()

GetStatsByGroupCommand::GetStatsByGroupCommand(
    Message* message, GetStatsByGroupRequest* request, StatsSlaveImpl* componentImpl)
    : GetStatsByGroupCommandStub(message, request),
    GetStatsByGroupBase(componentImpl)
{
}

/* Private methods *******************************************************************************/    
GetStatsByGroupCommandStub::Errors GetStatsByGroupCommand::execute()
{
    TRACE_LOG("[GetStatsByGroupCommand].execute() - retrieving stats.");

    Errors err = commandErrorFromBlazeError(executeBase(mRequest, mResponse));

    return err;
}

} // Stats
} // Blaze
