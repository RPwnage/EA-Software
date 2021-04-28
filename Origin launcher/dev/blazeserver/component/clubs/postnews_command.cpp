/*************************************************************************************************/
/*!
    \file   postnews_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class PostNewsCommand

    Post news for a specific club.

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
#include "framework/util/profanityfilter.h"

// clubs includes
#include "clubs/rpc/clubsslave/postnews_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

    class PostNewsCommand : public PostNewsCommandStub, private ClubsCommandBase
    {
    public:
        PostNewsCommand(Message* message, PostNewsRequest* request, ClubsSlaveImpl* componentImpl)
            : PostNewsCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~PostNewsCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        PostNewsCommandStub::Errors execute() override
        {
            TRACE_LOG("[PostNewsCommand] start");
            
            BlazeId blazeId = 0;
            ClubId clubId = mRequest.getClubId();
            
            if (gCurrentUserSession != nullptr)
                blazeId = gCurrentUserSession->getBlazeId();
                
            BlazeId contentCreator = blazeId;
                
            if (*mRequest.getClubNews().getText() != '\0' && *mRequest.getClubNews().getStringId() != '\0')
                return CLUBS_ERR_NEWS_TEXT_OR_STRINGID_MUST_BE_EMPTY;
                
            if (mRequest.getClubNews().getType() != CLUBS_ASSOCIATE_NEWS &&
                mRequest.getClubNews().getAssociateClubId() != INVALID_CLUB_ID)
            {
                return CLUBS_ERR_ASSOCIATE_CLUB_ID_MUST_BE_ZERO;              
            }
            
            bool isAdmin = false;

            if (blazeId != 0 && (mRequest.getClubNews().getType() == CLUBS_SERVER_GENERATED_NEWS))
            {
                isAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR, true);

                if (!isAdmin)
                {
                    TRACE_LOG("[PostNewsCommand] user [" << blazeId << "] is not admin of club ["
                        << clubId << "] and is not allowed to publish system-generated news");
                    return CLUBS_ERR_NO_PRIVILEGE; // only system or admin has the right to publish system generated news
                }
            }
            
            ClubsDbConnector dbc(mComponent); 
            if (dbc.acquireDbConnection(false) == nullptr)
                return ERR_SYSTEM;
            
            if (mRequest.getClubNews().getType() == CLUBS_ASSOCIATE_NEWS)
            {
                if (mRequest.getClubNews().getAssociateClubId() == INVALID_CLUB_ID
                    || clubId == mRequest.getClubNews().getAssociateClubId())
                    return CLUBS_ERR_INVALID_CLUB_ID;
                
                Club club;
                uint64_t version = 0;
                if (dbc.getDb().getClub(mRequest.getClubNews().getAssociateClubId(), &club, version) != Blaze::ERR_OK)
                    return CLUBS_ERR_INVALID_CLUB_ID;
                    
            }

            BlazeRpcError result;

            if (blazeId != 0)
            {
                // club admin and club members can post news
                MembershipStatus mshipStatus;
                MemberOnlineStatus onlSatus;
                TimeValue memberSince;
                result = mComponent->getMemberStatusInClub(
                    clubId, 
                    blazeId, 
                    dbc.getDb(), 
                    mshipStatus, 
                    onlSatus,
                    memberSince);
                
                if (result != (BlazeRpcError)ERR_OK)
                {
                    isAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR, true);

                    if (!isAdmin)
                    {
                        TRACE_LOG("[PostNewsCommand] user [" << blazeId << "] is not a member of club ["
                            << clubId << "] and is not allowed this action");
                        // this user is not member of this club
                        return CLUBS_ERR_NO_PRIVILEGE;
                    }
                }

                if (blazeId != mRequest.getClubNews().getUser().getBlazeId() )
                {
                    isAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR, true);

                    if (!isAdmin)
                    {
                        TRACE_LOG("[PostNewsCommand] user [" << blazeId << "] is not news creator or admin of club ["
                            << clubId << "] and is not allowed this action");
                        // this user is not news creator nor club admin
                        return CLUBS_ERR_NO_PRIVILEGE;
                    }

                    contentCreator = mRequest.getClubNews().getUser().getBlazeId();
                }
            }

            // Check news text for profanity
            if (gProfanityFilter->isProfane(mRequest.getClubNews().getText()) != 0)
            {
                TRACE_LOG("[PostNewsCommand] news item contains profanity: " << mRequest.getClubNews().getText());
                return CLUBS_ERR_PROFANITY_FILTER;
            }

            // Check news id text for profanity
            if (gProfanityFilter->isProfane(mRequest.getClubNews().getStringId()) != 0)
            {
                TRACE_LOG("[PostNewsCommand] news string id contains profanity: " << mRequest.getClubNews().getStringId());
                return CLUBS_ERR_PROFANITY_FILTER;
            }
            
            if (!dbc.startTransaction())
                return ERR_SYSTEM;

            // lock this club to prevent racing condition
            uint64_t version = 0;
            result = dbc.getDb().lockClub(clubId, version);
            if (result != Blaze::ERR_OK)
            {
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(result);
            }
            
            // we cannot null fields in db so we'll associate
            // news with the the owning club
            if (mRequest.getClubNews().getAssociateClubId() == INVALID_CLUB_ID)
                mRequest.getClubNews().setAssociateClubId(clubId);

            result = mComponent->insertNewsAndDeleteOldest(
                dbc.getDb(),
                clubId,
                contentCreator, 
                mRequest.getClubNews());
            
            if (result == (BlazeRpcError)ERR_OK)
            {
                if (!dbc.completeTransaction(true))
                {
                    ERR_LOG("[PostNewsCommand] could not commit transaction");
                    return ERR_SYSTEM;
                }
            }
            else
            {
                ERR_LOG("[PostNewsCommand] Database error (" << result << ")");
                if (!dbc.completeTransaction(false))
                    return ERR_SYSTEM;
            }
            
            mComponent->notifyNewsPublished(
                clubId,
                contentCreator, 
                mRequest.getClubNews());

            return (PostNewsCommandStub::Errors)result;
        }
    };

    // static factory method impl
    DEFINE_POSTNEWS_CREATE()

} // Clubs
} // Blaze
