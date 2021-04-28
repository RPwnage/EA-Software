/*************************************************************************************************/
/*!
    \file   joinclub_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class JoinClubCommand

    Invite someone else to join a club

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/event/eventmanager.h"

// clubs includes
#include "clubs/rpc/clubsslave/joinclub_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

    class JoinClubCommand : public JoinClubCommandStub, private ClubsCommandBase
    {
    public:
        JoinClubCommand(Message* message, JoinClubRequest* request, ClubsSlaveImpl* componentImpl)
            : JoinClubCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~JoinClubCommand() override
        {
        }

    
        /* Private methods *******************************************************************************/
    private:

        JoinClubCommandStub::Errors execute() override
        {
            TRACE_LOG("[JoinClubCommand] start");

            BlazeRpcError error = Blaze::ERR_SYSTEM;
            const BlazeId rtorBlazeId = gCurrentUserSession->getBlazeId();
            const ClubId joinClubId = mRequest.getClubId();
            const ClubPassword joinPassword = mRequest.getPassword();

            ClubsDbConnector dbc(mComponent);

            if (!dbc.startTransaction())
                return ERR_SYSTEM;

            // lock user messages so that no invitation/petition
            // can be processed while this request is being processed
            error = dbc.getDb().lockUserMessages(rtorBlazeId);
            if (error != Blaze::ERR_OK)
            {
                dbc.completeTransaction(false); 
                return commandErrorFromBlazeError(error);
            }

            uint32_t messageId;
            error = mComponent->joinClub(joinClubId, false/*skipJoinAcceptanceCheck*/, false/*petitionIfJoinFails*/, false/*skipPasswordCheck*/, joinPassword,
                rtorBlazeId, false/*isGMFriend*/, 0/*invitationId*/, dbc, messageId);
            if (error != Blaze::ERR_OK)
            {
                if (error == Blaze::CLUBS_ERR_PASSWORD_REQUIRED)
                    error = Blaze::CLUBS_ERR_WRONG_PASSWORD;
                dbc.completeTransaction(false); 
                return commandErrorFromBlazeError(error);
            }
            else
            {
                if (!dbc.completeTransaction(true))
                {
                    ERR_LOG("[JoinClubCommand] could not commit transaction");
                    mComponent->removeMemberUserExtendedData(rtorBlazeId, joinClubId, UPDATE_REASON_USER_LEFT_CLUB);
                    return ERR_SYSTEM;
                }

                BlazeRpcError result = mComponent->notifyUserJoined(
                    joinClubId, 
                    rtorBlazeId);

                if (result != Blaze::ERR_OK)
                {
                    WARN_LOG("[JoinClubCommand] failed to send notifications after user " << rtorBlazeId << " joined the club " << joinClubId 
                        << " because custom code notifyUserJoined returned error code " << SbFormats::HexLower(result));
                }

                UserJoinedClub userJoinedClubEvent;
                userJoinedClubEvent.setJoiningPersonaId(rtorBlazeId);
                userJoinedClubEvent.setClubId(joinClubId);
                gEventManager->submitEvent(static_cast<uint32_t>(ClubsSlave::EVENT_USERJOINEDCLUBEVENT), userJoinedClubEvent, true /*logBinaryEvent*/);

                mComponent->mPerfJoinsLeaves.increment();
            }
           
            return ERR_OK;
        }
    };

    // static factory method impl

    DEFINE_JOINCLUB_CREATE()

} // Clubs
} // Blaze
