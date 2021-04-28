/*************************************************************************************************/
/*!
    \file   findclubsasync_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FindClubsAsyncCommand

    Find a set of clubs using a complex search criteria and returns result set through
    notification.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"


// clubs includes
#include "clubs/rpc/clubsslave/findclubsasync_stub.h"
#include "clubs/rpc/clubsslave/findclubsasync2_stub.h"
#include "clubsslaveimpl.h"
#include "findclubs_commandbase.h"

namespace Blaze
{
namespace Clubs
{
class FindClubsAsyncCommonBase : public FindClubsCommandBase
{
public:
    FindClubsAsyncCommonBase(ClubsSlaveImpl* componentImpl, const char8_t* name)
        : FindClubsCommandBase(componentImpl, name)
    {

    }
    BlazeRpcError executeCommon(const FindClubsRequest& request, FindClubsAsyncResponse& response, uint32_t msgNum)
    {
        ClubList resultList;

        uint32_t totalCount = 0;
        uint32_t thisSequenceID = 0;

        BlazeRpcError result = FindClubsCommandBase::findClubsUtil(
            request,
            nullptr,     // memberOnlineStatusFilter
            0,     // memberOnlineStatusSum
            resultList,
            totalCount,
            &thisSequenceID,
            msgNum);

        if (result == Blaze::ERR_OK)
        {
            // setup response
            response.setTotalCount(totalCount);
            response.setCount(static_cast<uint32_t>(resultList.size()));
            response.setSequenceID(thisSequenceID);
        }

        return result;
    }
};


class FindClubsAsyncCommand : public FindClubsAsyncCommandStub, private FindClubsAsyncCommonBase
{
public:
    FindClubsAsyncCommand(Message* message, FindClubsRequest* request, ClubsSlaveImpl* componentImpl)
        : FindClubsAsyncCommandStub(message, request),
        FindClubsAsyncCommonBase(componentImpl, "FindClubsAsyncCommand")
    {
    }

    virtual ~FindClubsAsyncCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    FindClubsAsyncCommandStub::Errors execute() override
    {
        return commandErrorFromBlazeError(executeCommon(mRequest, mResponse, getMsgNum()));
    }
};
// static factory method impl
DEFINE_FINDCLUBSASYNC_CREATE()


class FindClubsAsync2Command : public FindClubsAsync2CommandStub, private FindClubsAsyncCommonBase
{
public:
    FindClubsAsync2Command(Message* message, FindClubsRequest* request, ClubsSlaveImpl* componentImpl)
        : FindClubsAsync2CommandStub(message, request),
        FindClubsAsyncCommonBase(componentImpl, "FindClubsAsyncCommand2")
    {
    }

    virtual ~FindClubsAsync2Command() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    FindClubsAsync2CommandStub::Errors execute() override
    {
        return commandErrorFromBlazeError(executeCommon(mRequest, mResponse, getMsgNum()));
    }

};
// static factory method impl
DEFINE_FINDCLUBSASYNC2_CREATE()
} // Clubs
} // Blaze
