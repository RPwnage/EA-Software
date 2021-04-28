/*************************************************************************************************/
/*!
    \file   getmembersasync_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetMembersAsyncCommand

    Get a list of members of a club and returns result set through notification.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"

// clubs includes
#include "clubs/rpc/clubsslave/getmembersasync_stub.h"
#include "clubs/rpc/clubsslave/getmembersasync2_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

class GetMembersAsyncCommonBase : public ClubsCommandBase
{
public:
    GetMembersAsyncCommonBase(ClubsSlaveImpl* componentImpl)
        : ClubsCommandBase(componentImpl)
    {

    }

    BlazeRpcError executeCommon(const GetMembersRequest& request, GetMembersAsyncResponse& response, uint32_t msgNum)
    {
        TRACE_LOG("[GetMembersAsyncCommand] start");

        ClubsDbConnector dbc(mComponent);
        if (dbc.acquireDbConnection(true) == nullptr)
            return ERR_SYSTEM;

        TRACE_LOG("[GetMembersAsyncCommand] received DB connection.");

        ClubMemberList list;
        uint32_t totalCount = 0;

        const char8_t* personaNamePattern = request.getPersonaNamePattern();
        if (personaNamePattern != nullptr && personaNamePattern[0] != '\0')
        {
            if (!mComponent->getConfig().getEnableSearchByPersonaName())
            {
                return CLUBS_ERR_FEATURE_NOT_ENABLED;
            }
        }
        else
        {
            personaNamePattern = nullptr;
        }

        BlazeRpcError error = dbc.getDb().getClubMembers(
            request.getClubId(),
            request.getMaxResultCount(),
            request.getOffset(),
            list,
            request.getOrderType(),
            request.getOrderMode(),
            request.getMemberType(),
            personaNamePattern,
            &totalCount);
        dbc.releaseConnection();

        if (error == Blaze::ERR_OK)
        {
            response.setTotalCount(totalCount);

            uint32_t thisSequenceID = mComponent->getNextAsyncSequenceId();

            if (list.size() == 0)
            {
                // no results
                response.setCount(0);
                response.setSequenceID(thisSequenceID);
                return error;
            }

            // resolve identities
            Blaze::BlazeIdVector blazeIds;
            Blaze::BlazeIdToUserInfoMap idMap;

            ClubMemberList::iterator it;
            for (it = list.begin(); it != list.end(); it++)
                blazeIds.push_back((*it)->getUser().getBlazeId());

            error = gUserSessionManager->lookupUserInfoByBlazeIds(blazeIds, idMap);

            if (error != Blaze::ERR_OK)
            {
                ERR_LOG("[GetMembersAsyncCommand] User identity provider returned an error (" << ErrorHelp::getErrorName(error) << ")");
                return error;
            }

            for (it = list.begin(); it != list.end(); it++)
            {
                // set online status
                (*it)->setOnlineStatus(mComponent->getMemberOnlineStatus((*it)->getUser().getBlazeId(), request.getClubId()));

                Blaze::BlazeIdToUserInfoMap::const_iterator itm = idMap.find((*it)->getUser().getBlazeId());
                if (itm != idMap.end())
                {
                    UserInfo::filloutUserCoreIdentification(*itm->second, (*it)->getUser());
                }
            }

            mComponent->overrideOnlineStatus(list);

            // fire the results
            for (it = list.begin(); it != list.end(); ++it)
            {
                GetMembersAsyncResult resultNotif;
                resultNotif.setSequenceID(thisSequenceID);
                (*it)->copyInto(resultNotif.getClubMember());
                mComponent->sendGetMembersAsyncNotificationToUserSession(gCurrentLocalUserSession, &resultNotif, false, msgNum);
            }

            // setup response
            response.setCount(static_cast<uint32_t>(list.size()));
            response.setSequenceID(thisSequenceID);
        }
        else if (error != Blaze::CLUBS_ERR_INVALID_CLUB_ID)
        {
            ERR_LOG("[GetMembersAsyncCommand] Database error (" << ErrorHelp::getErrorName(error) << ")");
        }

        return error;
    }
};

class GetMembersAsyncCommand : public GetMembersAsyncCommandStub, private GetMembersAsyncCommonBase
{
public:
    GetMembersAsyncCommand(Message* message, GetMembersRequest* request, ClubsSlaveImpl* componentImpl)
        : GetMembersAsyncCommandStub(message, request),
        GetMembersAsyncCommonBase(componentImpl)
    {
    }

    virtual ~GetMembersAsyncCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    GetMembersAsyncCommandStub::Errors execute() override
    {
        return commandErrorFromBlazeError(executeCommon(mRequest, mResponse, getMsgNum()));
    }

};
// static factory method impl
DEFINE_GETMEMBERSASYNC_CREATE()


class GetMembersAsync2Command : public GetMembersAsync2CommandStub, private GetMembersAsyncCommonBase
{
public:
    GetMembersAsync2Command(Message* message, GetMembersRequest* request, ClubsSlaveImpl* componentImpl)
        : GetMembersAsync2CommandStub(message, request),
        GetMembersAsyncCommonBase(componentImpl)
    {
    }

    virtual ~GetMembersAsync2Command() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    GetMembersAsync2CommandStub::Errors execute() override
    {
        return commandErrorFromBlazeError(executeCommon(mRequest, mResponse, getMsgNum()));
    }

};
// static factory method impl
DEFINE_GETMEMBERSASYNC2_CREATE()
} // Clubs
} // Blaze
