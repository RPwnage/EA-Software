/*************************************************************************************************/
/*!
    \file   postnews_command.cpp

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
#include "arson/rpc/arsonslave/postnews_stub.h"
#include "clubs/rpc/clubsslave.h"

namespace Blaze
{
namespace Arson
{
    class PostNewsCommand : public PostNewsCommandStub
{
public:
    PostNewsCommand(
        Message* message, Blaze::Arson::PostNewsRequest* request, ArsonSlaveImpl* componentImpl)
        : PostNewsCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~PostNewsCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    PostNewsCommandStub::Errors execute() override
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

        UserSession::SuperUserPermissionAutoPtr superPtr(true);
        BlazeRpcError err = component->postNews(mRequest.getPostNewsRequest());

        return arsonErrorFromClubsError(err);
    }

    static Errors arsonErrorFromClubsError(BlazeRpcError error);
};

    DEFINE_POSTNEWS_CREATE()

PostNewsCommandStub::Errors PostNewsCommand::arsonErrorFromClubsError(BlazeRpcError error)
{
    Errors result = ERR_SYSTEM;
    switch (error)
    {
        case ERR_OK: result = ERR_OK; break;
        case ERR_SYSTEM: result = ERR_SYSTEM; break;
        case ERR_TIMEOUT: result = ERR_TIMEOUT; break;
        case CLUBS_ERR_NO_PRIVILEGE: result = ARSON_CLUBS_ERR_NO_PRIVILEGE; break;
        case ARSON_ERR_CLUBS_COMPONENT_NOT_FOUND: result = ARSON_ERR_CLUBS_COMPONENT_NOT_FOUND; break;
        case CLUBS_ERR_INVALID_CLUB_ID: result = ARSON_CLUBS_ERR_INVALID_CLUB_ID; break;
        case CLUBS_ERR_NEWS_TEXT_OR_STRINGID_MUST_BE_EMPTY: result = ARSON_CLUBS_ERR_NEWS_TEXT_OR_STRINGID_MUST_BE_EMPTY; break;
        case CLUBS_ERR_PROFANITY_FILTER: result = ARSON_CLUBS_ERR_PROFANITY_FILTER; break;
        case CLUBS_ERR_ASSOCIATE_CLUB_ID_MUST_BE_ZERO: result = ARSON_CLUBS_ERR_ASSOCIATE_CLUB_ID_MUST_BE_ZERO; break;
        
        default:
        {
            //got an error not defined in rpc definition, log it
            TRACE_LOG("[PostNewsCommand].arsonErrorFromClubsError: unexpected error(" << SbFormats::HexLower(error) << "): return as ERR_SYSTEM");
            result = ERR_SYSTEM;
            break;
        }
    };

    return result;
}

} //Arson
} //Blaze
