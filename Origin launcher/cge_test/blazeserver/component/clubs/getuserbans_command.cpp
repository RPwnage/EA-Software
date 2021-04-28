/*************************************************************************************************/
/*!
    \file   getuserbans_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetUserBansCommand

        Gets all club bans for a specified user.

    \note
        Gets a list of clubs from which the user is banned, together with ban status (banning 
        authority type) for each club.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"

// clubs includes
#include "clubs/rpc/clubsslave/getuserbans_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

    class GetUserBansCommand : public GetUserBansCommandStub, private ClubsCommandBase
    {
    public:
        GetUserBansCommand(Message* message, GetUserBansRequest* request, ClubsSlaveImpl* componentImpl)
        :   GetUserBansCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~GetUserBansCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        GetUserBansCommandStub::Errors execute() override
        {
            TRACE_LOG("[GetUserBansCommand] start");

            const BlazeId banBlazeId = mRequest.getBlazeId();
            const BlazeId rtorBlazeId = gCurrentUserSession->getBlazeId();

            if (banBlazeId == 0)
            {
                return CLUBS_ERR_INVALID_USER_ID;
            }

            ClubsDbConnector dbc(mComponent); 

            if (dbc.acquireDbConnection(true) == nullptr)
            {
                return ERR_SYSTEM;
            }
            
            BlazeRpcError error = Blaze::ERR_SYSTEM;
            
            // Only GMs or Clubs Admins can access this RPC.
            bool rtorIsAdmin = false;
            bool rtorIsGmOfSharedClub = false;

            // Check if he is a GM of the user in some club(s) 
            // they both share.
            ClubIdList  clubsInWhichGm;
            error = dbc.getDb().requestorIsGmOfUser(rtorBlazeId, banBlazeId, &clubsInWhichGm);
    
            if (error == Blaze::ERR_OK)
            {
                rtorIsGmOfSharedClub = !clubsInWhichGm.empty();
            }

            if (!rtorIsGmOfSharedClub)
            {
                rtorIsAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR_VIEW, true);

                if (!rtorIsAdmin)
                {
                    TRACE_LOG("[GetUserBansCommand] user [" << rtorBlazeId << "] does not have admin view for user ["
                        << banBlazeId << "] and is not allowed this action");
                    if (error == Blaze::ERR_OK)
                    {
                        return CLUBS_ERR_NO_PRIVILEGE;
                    }
                    return commandErrorFromBlazeError(error);
                }
            }

            error = dbc.getDb().getUserBans(banBlazeId, &mResponse.getClubIdToBanStatusMap());

            if (error != Blaze::ERR_OK)           
            {
                return commandErrorFromBlazeError(error);
            }

            return ERR_OK;           
        }
    };

    // static factory method impl
    DEFINE_GETUSERBANS_CREATE()

} // Clubs
} // Blaze
