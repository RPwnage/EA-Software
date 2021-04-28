/*************************************************************************************************/
/*!
    \file   setmetadata_commandbase.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "setmetadata_commandbase.h"

namespace Blaze
{
namespace Clubs
{

    SetMetadataCommandBase::SetMetadataCommandBase(ClubsSlaveImpl* componentImpl, const char8_t* cmdName)
        : ClubsCommandBase(componentImpl),
          mCmdName(cmdName)
    {
    }

    BlazeRpcError SetMetadataCommandBase::setMetadata(const SetMetadataRequest& mRequest, ClubMetadataUpdateType updateType)
    {
        MembershipStatus rtorMshipStatus;
        MemberOnlineStatus rtorOnlSatus;
        TimeValue memberSince;
        
        BlazeId blazeId = gCurrentUserSession->getBlazeId();
        ClubId clubId = mRequest.getClubId();
        
        ClubsDbConnector dbc(mComponent); 
        if (dbc.acquireDbConnection(false) == nullptr)
            return ERR_SYSTEM;

        BlazeRpcError error = Blaze::ERR_OK;

		// club admin and GM can update club metadata; members can update metadata2
        bool isAdmin = false;
        bool isGMOrOwner = false;
		bool isMember = false;

        error = mComponent->getMemberStatusInClub(
            clubId, 
            blazeId, dbc.getDb(),
            rtorMshipStatus, 
            rtorOnlSatus,
            memberSince);

        if (error == Blaze::ERR_OK)
        {
            isGMOrOwner = (rtorMshipStatus == CLUBS_GM || rtorMshipStatus == CLUBS_OWNER);
			isMember = (rtorMshipStatus == CLUBS_MEMBER);
        }

		if ((!isGMOrOwner && updateType == CLUBS_METADATA_UPDATE) ||  
			(!isGMOrOwner && !isMember && updateType == CLUBS_METADATA_UPDATE_2))
        {
            isAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR, true);

            if (!isAdmin)
            {
                TRACE_LOG("[SetMetadataCommandBase] user [" << blazeId
                    << "] is not admin for club [" << clubId << "] and is not allowed this action");
				// insufficient privileges
                return CLUBS_ERR_NO_PRIVILEGE;
            }
        }

        if (!dbc.startTransaction())
            return ERR_SYSTEM;

        if (mRequest.getMetaDataUnion().getActiveMember() == MetadataUnion::MEMBER_UNSET)
        {
            // Backwards compatibility: use deprecated MetaData string member (note that binary metadata was never properly supported before the introduction of MetaDataUnion)
            MetadataUnion metadataUnion;
            metadataUnion.setMetadataString(mRequest.getMetaData());
            error = dbc.getDb().updateMetaData(
                clubId, 
                updateType, 
                metadataUnion,
                true) ;
        }
        else
        {
            error = dbc.getDb().updateMetaData(
                clubId, 
                updateType, 
                mRequest.getMetaDataUnion(),
                true) ;
        }

        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[" << mCmdName << "] Could not update metadata or last active, database error " << ErrorHelp::getErrorName(error));
            dbc.completeTransaction(false);
            return error;
        }
        
        if (!dbc.completeTransaction(true))
        {
            ERR_LOG("[" << mCmdName << "] could not commit transaction");
            return ERR_SYSTEM;
        }

        return ERR_OK;
    }

} // Clubs
} // Blaze
