/*************************************************************************************************/
/*!
    \file   getstatsbygroupasync_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetStatsByGroupAsyncCommand

    Retrieves statistics for pre-defined stat columns. Data is sent to client via notifications.
    Code retreiving data from db is separated to the base class to be reused by both
    GetStatsByGroupCommand and GetStatsByGroupAsyncCommand
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "framework/usersessions/usersessionmanager.h"
#include "stats/statsslaveimpl.h"
#include "stats/rpc/statsslave/getstatsbygroupasync_stub.h"
#include "stats/rpc/statsslave/getstatsbygroupasync2_stub.h"
#include "stats/getstatsbygroupbase.h"

namespace Blaze
{
namespace Stats
{

class GetStatsByGroupAsyncCommonBase : public GetStatsByGroupBase
{
public:
    GetStatsByGroupAsyncCommonBase(StatsSlaveImpl* componentImpl)
        : GetStatsByGroupBase(componentImpl)
    {

    }

    BlazeRpcError executeCommon(const GetStatsByGroupRequest& request, uint32_t msgNum)
    {
        TRACE_LOG("[GetStatsByGroupAsyncCommand].execute()");

        GetStatsResponse response;
        BlazeRpcError error = executeBase(request, response);

        if (error != ERR_OK)
            return error;

        // Send stats to the client one kescope at time 
        // Package is accompanied with a client view ID and scope key string to allow assembling 
        // it together on client side
        // End of transmission defined by the mLast value. 
        KeyScopeStatsValueMap::const_iterator it = response.getKeyScopeStatsValueMap().begin();
        KeyScopeStatsValueMap::const_iterator ite = response.getKeyScopeStatsValueMap().end();

        do
        {
            KeyScopedStatValues keyScopedStatValues;
            keyScopedStatValues.setViewId(request.getViewId());
            keyScopedStatValues.setGroupName(request.getGroupName());

            if (it != ite)
            {
                const StatValues* statValues = it->second;
                keyScopedStatValues.setKeyString(it->first);
                keyScopedStatValues.setLast(false);
                statValues->copyInto(keyScopedStatValues.getStatValues());
                ++it;
            }

            if (it == ite)
                keyScopedStatValues.setLast(true);

            mComponent->sendGetStatsAsyncNotificationToUserSession(gCurrentLocalUserSession, &keyScopedStatValues, false, msgNum);
        } while (it != ite);

        return error;
    }
};

class GetStatsByGroupAsyncCommand : public GetStatsByGroupAsyncCommandStub, GetStatsByGroupAsyncCommonBase
{
public:

    GetStatsByGroupAsyncCommand(Message* message, GetStatsByGroupRequest* request, StatsSlaveImpl* componentImpl);
    ~GetStatsByGroupAsyncCommand() override {}
    
private:
    // States
    GetStatsByGroupAsyncCommandStub::Errors execute() override;
};

DEFINE_GETSTATSBYGROUPASYNC_CREATE()

GetStatsByGroupAsyncCommand::GetStatsByGroupAsyncCommand(
    Message* message, GetStatsByGroupRequest* request, StatsSlaveImpl* componentImpl)
    : GetStatsByGroupAsyncCommandStub(message, request),
    GetStatsByGroupAsyncCommonBase(componentImpl)
{
}

/* Private methods *******************************************************************************/    
GetStatsByGroupAsyncCommandStub::Errors GetStatsByGroupAsyncCommand::execute()
{
    return commandErrorFromBlazeError(executeCommon(mRequest, getMsgNum()));
}

class GetStatsByGroupAsync2Command : public GetStatsByGroupAsync2CommandStub, GetStatsByGroupAsyncCommonBase
{
public:

    GetStatsByGroupAsync2Command(Message* message, GetStatsByGroupRequest* request, StatsSlaveImpl* componentImpl);
    virtual ~GetStatsByGroupAsync2Command() {}

private:
    // States
    GetStatsByGroupAsync2CommandStub::Errors execute();
};

DEFINE_GETSTATSBYGROUPASYNC2_CREATE()

GetStatsByGroupAsync2Command::GetStatsByGroupAsync2Command(
    Message* message, GetStatsByGroupRequest* request, StatsSlaveImpl* componentImpl)
    : GetStatsByGroupAsync2CommandStub(message, request),
    GetStatsByGroupAsyncCommonBase(componentImpl)
{
}

/* Private methods *******************************************************************************/
GetStatsByGroupAsync2CommandStub::Errors GetStatsByGroupAsync2Command::execute()
{
    return commandErrorFromBlazeError(executeCommon(mRequest, getMsgNum()));
}


} // Stats
} // Blaze
