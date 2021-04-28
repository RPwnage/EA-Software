/*************************************************************************************************/
/*!
    \file   updatemembermetadata_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateMemberMetadataCommand

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
#include "clubs/rpc/clubsslave/updatemembermetadata_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

    class UpdateMemberMetadataCommand : public UpdateMemberMetadataCommandStub, private ClubsCommandBase
    {
    public:
        UpdateMemberMetadataCommand(Message* message, UpdateMemberMetadataRequest* request, ClubsSlaveImpl* componentImpl)
            : UpdateMemberMetadataCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~UpdateMemberMetadataCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        UpdateMemberMetadataCommandStub::Errors execute() override
        {
            TRACE_LOG("[UpdateMemberMetadataCommand] start");
            
            BlazeId blazeId = gCurrentUserSession->getBlazeId();
            
            ClubsDbConnector dbc(mComponent); 
            if (dbc.acquireDbConnection(false) == nullptr)
                return ERR_SYSTEM;
            
            BlazeRpcError error = Blaze::ERR_OK;
            ClubId clubId = mRequest.getClubId();
            ClubMember member;
            MembershipStatus memStatus;
            MemberOnlineStatus onlineStatus;
            TimeValue memberSince;
            
            error = mComponent->getMemberStatusInClub(
                clubId, 
                blazeId,
                dbc.getDb(), 
                memStatus, 
                onlineStatus, 
                memberSince);

            if (error != Blaze::ERR_OK)
            {
                return commandErrorFromBlazeError(error);
            }
            
            // starting transaction as this command will call out to
            // custom onMemberMetadataUpdated function
            if (!dbc.startTransaction())
            {
                ERR_LOG("[UpdateMemberMetadataCommand] Could not start transaction.");
                return ERR_SYSTEM;
            }

            member.getUser().setBlazeId(blazeId);
            member.setMembershipStatus(memStatus);
            member.setOnlineStatus(onlineStatus);
            Collections::AttributeMap::const_iterator i = mRequest.getMetaData().begin();
            Collections::AttributeMap::const_iterator end = mRequest.getMetaData().end();
            for(; i != end; i++)
            {
                member.getMetaData().insert(eastl::make_pair(i->first, i->second));
            }

            error = dbc.getDb().updateMemberMetaData(clubId, member);

            if (error != Blaze::ERR_OK)
            {
                ERR_LOG("[UpdateMemberMetadataCommand] Could not update metadata, database error " << ErrorHelp::getErrorName(error));
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }

            error = mComponent->onMemberMetadataUpdated(&dbc.getDb(), clubId, &member);

            if (error != Blaze::ERR_OK)
            {
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }

            if (!dbc.completeTransaction(true))
            {
                ERR_LOG("[UpdateMemberMetadataCommand] could not commit transaction");
                return ERR_SYSTEM;
            }

            TRACE_LOG("[UpdateMemberMetadataCommand] end");
            
            return ERR_OK;
        }
    };

    // static factory method impl
    DEFINE_UPDATEMEMBERMETADATA_CREATE()

} // Clubs
} // Blaze
