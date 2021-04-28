/*************************************************************************************************/
/*!
    \file   updatememberonlinestatus_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateMemberOnlineStatusCommand

    Updates member online status for a list of users which are members of the same club

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
#include "clubs/rpc/clubsslave/updatememberonlinestatus_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

    class UpdateMemberOnlineStatusCommand : public UpdateMemberOnlineStatusCommandStub, private ClubsCommandBase
    {
    public:
        UpdateMemberOnlineStatusCommand(Message* message, UpdateMemberOnlineStatusRequest* request, ClubsSlaveImpl* componentImpl)
            : UpdateMemberOnlineStatusCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~UpdateMemberOnlineStatusCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        UpdateMemberOnlineStatusCommandStub::Errors execute() override
        {
            TRACE_LOG("[UpdateMemberOnlineStatusCommand] start");

            BlazeId blazeId = gCurrentUserSession->getBlazeId();
            BlazeRpcError error = Blaze::ERR_OK;

            // We need to iterate through the BlazeId online status map to see if there is another club with custom status, and if so, set it to be ONLINE
            ClubsSlaveImpl::OnlineStatusForUserPerClubCache::const_iterator userItr = mComponent->getOnlineStatusForUserPerClubCache().find(blazeId);
            if (userItr != mComponent->getOnlineStatusForUserPerClubCache().end())
            {
                for (ClubsSlaveImpl::OnlineStatusInClub::const_iterator statusItr = userItr->second->begin();
                     statusItr != userItr->second->end();
                     ++statusItr)
                {
                    if (statusItr->first == mRequest.getClubId())
                        continue;

                    if (statusItr->second > CLUBS_MEMBER_ONLINE_NON_INTERACTIVE)
                    {
                        error = mComponent->updateMemberUserExtendedData(blazeId, statusItr->first, UPDATE_REASON_USER_ONLINE_STATUS_UPDATED, mComponent->getOnlineStatus(blazeId));
                        break;
                    }
                }
            }

            if (error == Blaze::ERR_OK)
                error = mComponent->updateMemberUserExtendedData(blazeId, mRequest.getClubId(), UPDATE_REASON_USER_ONLINE_STATUS_UPDATED, mRequest.getNewStatus());

            TRACE_LOG("[UpdateMemberOnlineStatusCommand] end");

            return commandErrorFromBlazeError(error);
        }
    };

    // static factory method impl
    DEFINE_UPDATEMEMBERONLINESTATUS_CREATE()

} // Clubs
} // Blaze
