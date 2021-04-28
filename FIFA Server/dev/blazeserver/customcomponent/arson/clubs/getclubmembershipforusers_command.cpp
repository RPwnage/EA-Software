/*************************************************************************************************/
/*!
    \file   getclubmembershipforusers_command.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getclubmembershipforusers_stub.h"
#include "clubs/rpc/clubsslave.h"

namespace Blaze
{
namespace Arson
{
    class GetClubMembershipForUsersCommand : public GetClubMembershipForUsersCommandStub
{
public:
    GetClubMembershipForUsersCommand(
        Message* message, Blaze::Arson::GetClubMembershipForUsersRequest* request, ArsonSlaveImpl* componentImpl)
        : GetClubMembershipForUsersCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~GetClubMembershipForUsersCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetClubMembershipForUsersCommandStub::Errors execute() override
    {
      //  BlazeRpcError error;
        Blaze::Clubs::ClubsSlave *component = (Blaze::Clubs::ClubsSlave *) gController->getComponent(Blaze::Clubs::ClubsSlave::COMPONENT_ID);           

        if (component == nullptr)
        {
            return ARSON_ERR_CLUBS_COMPONENT_NOT_FOUND;
        }

        if(mRequest.getNullCurrentUserSession())
        {
            // Set the gCurrentUserSession to nullptr
            UserSession::setCurrentUserSessionId(INVALID_USER_SESSION_ID);
        }
        
        UserSession::SuperUserPermissionAutoPtr superPermission(true);
        BlazeRpcError err = component->getClubMembershipForUsers(mRequest.getGetClubMembershipForUsersRequest(), mResponse.getGetClubMembershipForUsersResponse());

        return arsonErrorFromClubsError(err);
    }

    static Errors arsonErrorFromClubsError(BlazeRpcError error);
};

    DEFINE_GETCLUBMEMBERSHIPFORUSERS_CREATE()

GetClubMembershipForUsersCommandStub::Errors GetClubMembershipForUsersCommand::arsonErrorFromClubsError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case ARSON_ERR_CLUBS_COMPONENT_NOT_FOUND: result = ARSON_ERR_CLUBS_COMPONENT_NOT_FOUND; break;
        case CLUBS_ERR_INVALID_CLUB_ID: result = ARSON_CLUBS_ERR_INVALID_ARGUMENT; break;
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[GetClubMembershipForUsersCommand].arsonErrorFromClubsError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
