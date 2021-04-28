/*************************************************************************************************/
/*!
    \file   getmemberonlinestatus_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetMemberOnlineStatusCommand

    get member online status in the given club for users

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/controller/controller.h"
#include "framework/usersessions/usersessionmanager.h"

// clubs includes
#include "clubs/rpc/clubsslave/getmemberonlinestatus_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

// messaging includes
#include "messaging/messagingslaveimpl.h"
#include "messaging/tdf/messagingtypes.h"

namespace Blaze
{
namespace Clubs
{

class GetMemberOnlineStatusCommand : public GetMemberOnlineStatusCommandStub, private ClubsCommandBase
{
public:
    GetMemberOnlineStatusCommand(Message* message, GetMemberOnlineStatusRequest* request, ClubsSlaveImpl* componentImpl)
        : GetMemberOnlineStatusCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~GetMemberOnlineStatusCommand() override
    {
    }

        /* Private methods *******************************************************************************/
private:

    GetMemberOnlineStatusCommandStub::Errors execute() override
    {
        mComponent->getMemberOnlineStatus(mRequest.getBlazeIds(),
                                            mRequest.getClubId(), 
                                            mResponse.getStatus()); 

        if(mComponent->allowOnlineStatusOverride())
        {
            // Override the online status based on the privacy settings. We have to do a bit of trickery because of the id incompatibilities and 
            // going back and forth between them.
            MemberOnlineStatusMapForClub& statusMapByBlazeId = mResponse.getStatus();

            Blaze::BlazeIdVector blazeIds;
            blazeIds.reserve(statusMapByBlazeId.size());
            for (MemberOnlineStatusMapForClub::const_iterator it = statusMapByBlazeId.begin(), itEnd = statusMapByBlazeId.end(); it != itEnd; ++it)
            {
                blazeIds.push_back(it->first);
            }

            Blaze::BlazeIdToUserInfoMap infoMap;
            BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeIds(blazeIds, infoMap);
            if (err != Blaze::ERR_OK)
            {
                ERR_LOG("[GetMemberOnlineStatusCommand] User identity provider error (" << ErrorHelp::getErrorName(err) << ").");
                return ERR_SYSTEM;
            }

            Blaze::PlatformInfoVector platformInfos;
            platformInfos.reserve(statusMapByBlazeId.size());

            MemberOnlineStatusMapByExternalId statusMapByExtId;
            for (Blaze::BlazeIdToUserInfoMap::const_iterator it = infoMap.begin(), itEnd = infoMap.end(); it != itEnd; ++it)
            {
                ExternalId extId = getExternalIdFromPlatformInfo(it->second->getPlatformInfo());
                statusMapByExtId[extId] = statusMapByBlazeId[it->first];
                platformInfos.push_back(it->second->getPlatformInfo());
            }

            mComponent->overrideOnlineStatus(statusMapByExtId);

            Blaze::UserInfoPtrList userInfoList;
            err = gUserSessionManager->lookupUserInfoByPlatformInfoVector(platformInfos, userInfoList);
            if (err != Blaze::ERR_OK)
            {
                ERR_LOG("[GetMemberOnlineStatusCommand] User identity provider error (" << ErrorHelp::getErrorName(err) << ").");
                return ERR_SYSTEM;
            }

            for (auto userInfoPtr : userInfoList)
            {
                ExternalId extId = getExternalIdFromPlatformInfo(userInfoPtr->getPlatformInfo());
                statusMapByBlazeId[userInfoPtr->getId()] = statusMapByExtId[extId];
            }
        }
        

        return ERR_OK;
    }
};

// static factory method impl
DEFINE_GETMEMBERONLINESTATUS_CREATE()

} // Clubs
} // Blaze
