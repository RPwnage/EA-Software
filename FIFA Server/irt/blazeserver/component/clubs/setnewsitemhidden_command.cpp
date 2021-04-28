/*************************************************************************************************/
/*!
    \file   setnewsitemhidden_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class SetNewsItemHidden

    Hides or unhides news item published by club members

    \note
    
    Restricted to GMs only
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbutil.h"
#include "framework/database/query.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
#include "framework/util/localization.h"

// clubs includes
#include "clubs/rpc/clubsslave/setnewsitemhidden_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"

namespace Blaze
{
namespace Clubs
{

    class SetNewsItemHiddenCommand : public SetNewsItemHiddenCommandStub, private ClubsCommandBase
    {
    public:
        SetNewsItemHiddenCommand(Message* message, SetNewsItemHiddenRequest* request, ClubsSlaveImpl* componentImpl)
            : SetNewsItemHiddenCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~SetNewsItemHiddenCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        SetNewsItemHiddenCommandStub::Errors execute() override
        {
            TRACE_LOG("[SetNewsItemHiddenCommand] start");
            BlazeRpcError result = Blaze::ERR_OK;
            
            // only GM or club_admin can invoke this function
            BlazeId requestorId = gCurrentUserSession->getBlazeId();
            ClubId clubId= mRequest.getClubId();

            ClubsDbConnector dbc(mComponent);
            dbc.startTransaction();

            bool isAdmin = false;
            bool isGM = false;

            MembershipStatus mshipStatus;
            MemberOnlineStatus onlSatus;
            TimeValue memberSince = 0;

            result = mComponent->getMemberStatusInClub(
                clubId, 
                requestorId, 
                dbc.getDb(), 
                mshipStatus, 
                onlSatus,
                memberSince);

            if (result == Blaze::ERR_OK)
            {
                isGM = (mshipStatus >= CLUBS_GM);
            }

            if (!isGM)
            {
                isAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR, true);

                if (!isAdmin)
                {
                    TRACE_LOG("[SetNewsItemHiddenCommand] user [" << requestorId
                        << "] is not admin for club [" << clubId << "] and is not allowed this action");
                    dbc.completeTransaction(false);
                    if (result == Blaze::ERR_OK)
                    {
                        return CLUBS_ERR_NO_PRIVILEGE;
                    }
                    return commandErrorFromBlazeError(result);
                }
            }
            
            ClubId newsClubId;
            ClubServerNews serverNews;
            result = dbc.getDb().getServerNews(
                static_cast<uint32_t>(mRequest.getNewsId().id),
                newsClubId,
                serverNews);
            
            if (result != Blaze::ERR_OK)
            {
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(result);
            }
            
            if (clubId != newsClubId || (serverNews.getType() != CLUBS_MEMBER_POSTED_NEWS && serverNews.getType() != CLUBS_ASSOCIATE_NEWS))
            {
                dbc.completeTransaction(false);
                return CLUBS_ERR_NO_PRIVILEGE;
            }
            
            ClubNewsFlags mask;
            ClubNewsFlags flags;
            mask.setCLUBS_NEWS_HIDDEN_BY_GM();
            
            if (mRequest.getIsHidden())
                flags.setCLUBS_NEWS_HIDDEN_BY_GM();
            
            result = dbc.getDb().updateNewsItemFlags(
                static_cast<uint32_t>(mRequest.getNewsId().id), 
                mask, 
                flags);
            
            if (result != (BlazeRpcError)ERR_OK)
            {
                if (result != (BlazeRpcError)CLUBS_ERR_NEWS_ITEM_NOT_FOUND)
                {
                    // log unexpected database error
                    ERR_LOG("[SetNewsItemHiddenCommand] Database call failed with error " << SbFormats::HexLower(result) );
                }
                else
                    result = Blaze::ERR_OK; // news already exists and it has same flags as we're
                                           // trying to set
                dbc.completeTransaction(false); 
            }
            else
                dbc.completeTransaction(true);
            
            TRACE_LOG("[SetNewsItemHiddenCommand] end");

            return (Errors)result;
        }
    };

    // static factory method impl
    DEFINE_SETNEWSITEMHIDDEN_CREATE()

} // Clubs
} // Blaze
