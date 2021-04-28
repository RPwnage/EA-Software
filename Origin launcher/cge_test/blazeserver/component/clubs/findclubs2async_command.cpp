/*************************************************************************************************/
/*!
    \file   findclubs2async_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FindClubs2AsyncCommand

    Find a set of clubs using a complex search criteria and returns result set through
    notification.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/findclubs2async_stub.h"
#include "clubs/rpc/clubsslave/findclubs2async2_stub.h"
#include "clubsslaveimpl.h"
#include "findclubs_commandbase.h"

namespace Blaze
{
namespace Clubs
{

class FindClubs2AsyncCommonBase : public FindClubsCommandBase
{
public:
    FindClubs2AsyncCommonBase(ClubsSlaveImpl* componentImpl, const char8_t* name)
        : FindClubsCommandBase(componentImpl, name)
    {

    }
    BlazeRpcError executeCommon(const FindClubs2Request& request, FindClubsAsyncResponse& response)
    {
        ClubList resultList;

        uint32_t totalCount = 0;
        uint32_t thisSequenceID = 0;
        BlazeRpcError result = FindClubsCommandBase::findClubsUtil(
            request.getParams(),
            &request.getMemberOnlineStatusFilter(),
            request.getMemberOnlineStatusSum(),
            resultList,
            totalCount,
            &thisSequenceID);

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

class FindClubs2AsyncCommand : public FindClubs2AsyncCommandStub, private FindClubs2AsyncCommonBase
{
public:
    FindClubs2AsyncCommand(Message* message, FindClubs2Request* request, ClubsSlaveImpl* componentImpl)
        : FindClubs2AsyncCommandStub(message, request),
        FindClubs2AsyncCommonBase(componentImpl, "FindClubs2AsyncCommand")
    {
    }

    virtual ~FindClubs2AsyncCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    FindClubs2AsyncCommandStub::Errors execute() override
    {
        return commandErrorFromBlazeError(executeCommon(mRequest, mResponse));
    }

};
// static factory method impl
DEFINE_FINDCLUBS2ASYNC_CREATE()


class FindClubs2Async2Command : public FindClubs2Async2CommandStub, private FindClubs2AsyncCommonBase
{
public:
    FindClubs2Async2Command(Message* message, FindClubs2Request* request, ClubsSlaveImpl* componentImpl)
        : FindClubs2Async2CommandStub(message, request),
        FindClubs2AsyncCommonBase(componentImpl, "FindClubs2Async2Command")
    {
    }

    virtual ~FindClubs2Async2Command() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    FindClubs2Async2CommandStub::Errors execute() override
    {
        return commandErrorFromBlazeError(executeCommon(mRequest, mResponse));
    }

};
// static factory method impl
DEFINE_FINDCLUBS2ASYNC2_CREATE()

} // Clubs
} // Blaze
