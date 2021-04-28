/*************************************************************************************************/
/*!
    \file   resetclubrecords_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ResetClubRecordsCommand

    reset club records for given list of record ids

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
#include "clubs/rpc/clubsslave/resetclubrecords_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

    class ResetClubRecordsCommand : public ResetClubRecordsCommandStub, private ClubsCommandBase
    {
    public:
        ResetClubRecordsCommand(Message* message, ResetClubRecordsRequest* request, ClubsSlaveImpl* componentImpl)
            : ResetClubRecordsCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~ResetClubRecordsCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        ResetClubRecordsCommandStub::Errors execute() override
        {
            TRACE_LOG("[ResetClubRecordsCommand] start");

            BlazeId requestorId = gCurrentUserSession->getBlazeId();
            ClubId clubId = mRequest.getClubId();

            ClubsDbConnector dbc(mComponent); 
            if (dbc.acquireDbConnection(false) == nullptr)
                return ERR_SYSTEM;
            
            TRACE_LOG("[ResetClubRecordsCommand] received DB connection.");
            BlazeRpcError error = Blaze::ERR_OK;

            // club admin and GM can reset club records
            bool isAdmin = false;
            bool isGM = false;
            
            MembershipStatus rtorMshipStatus;
            MemberOnlineStatus rtorOnlSatus;
            TimeValue memberSince;

            error = mComponent->getMemberStatusInClub(
                clubId, 
                requestorId, 
                dbc.getDb(), 
                rtorMshipStatus, 
                rtorOnlSatus,
                memberSince);

            if (error == Blaze::ERR_OK)
            {
                isGM = (rtorMshipStatus >= CLUBS_GM);
            }

            if (!isGM)
            {
                isAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR, true);

                if (!isAdmin)
                {
                    TRACE_LOG("[ResetClubRecordsCommand] user [" << requestorId
                        << "] is not admin for club [" << clubId << "] and is not allowed this action");
                    dbc.releaseConnection();
                    if (error == Blaze::ERR_OK)
                    {
                        return CLUBS_ERR_NO_PRIVILEGE;
                    }
                    return commandErrorFromBlazeError(error);
                }
            }
            
            if (!dbc.startTransaction())
                return ERR_SYSTEM;
            
            error = mComponent->resetClubRecords(
                dbc.getDbConn(), 
                clubId, 
                mRequest.getRecordIdList());

            if (error != Blaze::ERR_OK)
            {
                dbc.completeTransaction(false);

                ERR_LOG("[ResetClubRecordsCommand] Unable to reset records for club id " << clubId
                    << " because resetClubRecords returned error code " << SbFormats::HexLower(error));

                return commandErrorFromBlazeError(error);
            }
                
            if (!dbc.completeTransaction(true))
            {
                ERR_LOG("[ResetClubRecordsCommand] could not commit transaction");
                return ERR_SYSTEM;
            }

            return commandErrorFromBlazeError(error);
        }
    };

    // static factory method impl
    DEFINE_RESETCLUBRECORDS_CREATE()

} // Clubs
} // Blaze
