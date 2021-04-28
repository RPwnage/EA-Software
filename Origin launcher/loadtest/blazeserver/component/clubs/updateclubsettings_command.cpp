/*************************************************************************************************/
/*!
    \file   updateclubsettings_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateClubSettingsCommand

    Set the password and other parameters of the club to new values.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/event/eventmanager.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
#include "framework/util/profanityfilter.h"
#include "framework/controller/controller.h"

// clubs includes
#include "clubs/rpc/clubsslave/updateclubsettings_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

// stats includes
#include "stats/statsslaveimpl.h"

namespace Blaze
{
namespace Clubs
{

    class UpdateClubSettingsCommand : public UpdateClubSettingsCommandStub, private ClubsCommandBase
    {
    public:
        UpdateClubSettingsCommand(Message* message, UpdateClubSettingsRequest* request, ClubsSlaveImpl* componentImpl)
            : UpdateClubSettingsCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~UpdateClubSettingsCommand() override
        {
        }

    /* Private methods *******************************************************************************/
    private:

        BlazeRpcError checkRequestParameters()
        {
            BlazeRpcError error = Blaze::ERR_OK;

            // check tags
            error = mComponent->checkClubTags(mRequest.getTagList());
            if (error != Blaze::ERR_OK)
                return error;

            // Check club non-unique name and description for profanity
            error = mComponent->checkClubStrings(nullptr, 
                mRequest.getClubSettings().getNonUniqueName(),
                mRequest.getClubSettings().getDescription(),
                mRequest.getClubSettings().getPassword());
            if ( error != Blaze::ERR_OK)
                return error;

            return Blaze::ERR_OK;
        }

        UpdateClubSettingsCommandStub::Errors execute() override
        {
            TRACE_LOG("[UpdateClubSettingsCommand] start");
            
            BlazeRpcError error = Blaze::ERR_OK;
            
            // check parameters of request (without database connection)
            error = checkRequestParameters();
            if (error != Blaze::ERR_OK)
                return commandErrorFromBlazeError(error);

            ClubId clubId = mRequest.getClubId();

            ClubsDbConnector dbc(mComponent); 
            if (dbc.acquireDbConnection(false) == nullptr)
                return ERR_SYSTEM;

			BlazeId blazeId = 0;
			if (nullptr != gCurrentUserSession)
			{
				blazeId = gCurrentUserSession->getBlazeId();

				// only GMs or users authorized as Clubs Administrator can access this RPC
				bool isAdmin = false;
				bool isGMOrOwner = false;

				MembershipStatus rtorMshipStatus;
				MemberOnlineStatus rtorOnlSatus;
				TimeValue memberSince;

				error = mComponent->getMemberStatusInClub(
					clubId, 
					blazeId, 
					dbc.getDb(), 
					rtorMshipStatus, 
					rtorOnlSatus,
					memberSince);

				if (error == (BlazeRpcError)ERR_OK)
				{
					isGMOrOwner = (rtorMshipStatus == CLUBS_GM || rtorMshipStatus == CLUBS_OWNER);
				}

				if (!isGMOrOwner)
				{
					isAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR, true);

					if (!isAdmin)
					{
						TRACE_LOG("[UpdateClubSettingsCommand] user [" << blazeId
							<< "] is not admin for club [" << clubId << "] and is not allowed this action");
						return CLUBS_ERR_NO_PRIVILEGE;
					}
				}
			}
			else if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR, true))
			{
				return CLUBS_ERR_NO_PRIVILEGE;
			}


            if (!dbc.startTransaction())
                return ERR_SYSTEM;

            error = dbc.getDb().updateClubLastActive(clubId);
            if (error != Blaze::ERR_OK)
            {
                ERR_LOG("[UpdateClubSettingsCommand] Could not update last active, database error " << ErrorHelp::getErrorName(error));
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }
 
            ClubSettings cachedSettings;

            if (mComponent->getClubSettings(dbc.getDbConn(), clubId, cachedSettings) != Blaze::ERR_OK)
            {
                ERR_LOG("[UpdateClubSettingsCommand] Could not get cached settings.");
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }

			char8_t tempCachedMetaData[MAX_METADATASTRING_LEN];
			blaze_strnzcpy(tempCachedMetaData, cachedSettings.getMetaDataUnion().getMetadataString(), MAX_METADATASTRING_LEN);
			char8_t tempNewMetaData[MAX_METADATASTRING_LEN];
			blaze_strnzcpy(tempNewMetaData, mRequest.getClubSettings().getMetaDataUnion().getMetadataString(), MAX_METADATASTRING_LEN);

			char8_t* pCachedContext = nullptr;
			char8_t* pCachedStadiumName = blaze_strtok(tempCachedMetaData, ">", &pCachedContext);
			while (pCachedStadiumName != nullptr && blaze_strstr(pCachedStadiumName, "StadName") == nullptr)
			{
				pCachedStadiumName = blaze_strtok(nullptr, ">", &pCachedContext);
			}

			char8_t* pNewContext = nullptr;
			char8_t* pNewStadiumName = blaze_strtok(tempNewMetaData, ">", &pNewContext);
			while (pNewStadiumName != nullptr && blaze_strstr(tempNewMetaData, "StadName") == nullptr)
			{
				pNewStadiumName = blaze_strtok(nullptr, ">", &pNewContext);
			}
			
			if (pCachedStadiumName != nullptr && pNewStadiumName != nullptr && blaze_strcmp(pCachedStadiumName, pNewStadiumName) == 0)
			{
				mRequest.getClubSettings().setLastUpdatedBy(cachedSettings.getLastUpdatedBy());
			}
			else
			{
				mRequest.getClubSettings().setLastUpdatedBy(blazeId);
			}

            uint64_t version = 0;
            error = dbc.getDb().updateClubSettings(
                    clubId, 
                    mRequest.getClubSettings(),
                    version);
            if (error != Blaze::ERR_OK)
            {
                ERR_LOG("[UpdateClubSettingsCommand] Could not update settings, database error " << ErrorHelp::getErrorName(error));
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }

            error = mComponent->onSettingsUpdated(&dbc.getDb(), clubId,
                &cachedSettings, &mRequest.getClubSettings());

            if (error != Blaze::ERR_OK)
            {
                ERR_LOG("[UpdateClubSettingsCommand].onSettingsUpdated failed with error=" << ErrorHelp::getErrorName(error));
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }

            // onSettingsUpdated can potentially update the cache with new settings, so we need to fetch the club
            // and latest version in order to finish updating the cache
            Club club;
            error = dbc.getDb().getClub(clubId, &club, version);
            if (error != Blaze::ERR_OK)
            {
                ERR_LOG("[UpdateClubSettingsCommand].getClub failed with error =" << ErrorHelp::getErrorName(error));
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }

            // try and update cached data
            const ClubDomain *domain;
            if (mComponent->getClubDomainForClub(clubId, dbc.getDb(), domain) == Blaze::ERR_OK)
            {
                mComponent->updateCachedClubInfo(version, clubId, club);
            }
            
            // update tags
            error = dbc.getDb().updateClubTags(clubId, mRequest.getTagList());
            if (error != Blaze::ERR_OK)
            {
                ERR_LOG("[UpdateClubSettingsCommand] Could not update club tags, database error " << ErrorHelp::getErrorName(error));
                dbc.completeTransaction(false);
                return commandErrorFromBlazeError(error);
            }

            if (!dbc.completeTransaction(true))
            {
                ERR_LOG("[UpdateClubSettingsCommand] could not commit transaction");
                return ERR_SYSTEM;
            }

			if (blazeId != 0)
			{
	            ClubSettingsUpdated clubSettingsUpdatedEvent;
		        clubSettingsUpdatedEvent.setUpdaterPersonaId(blazeId);
			    clubSettingsUpdatedEvent.setClubId(clubId);
				gEventManager->submitEvent(static_cast<uint32_t>(ClubsSlave::EVENT_CLUBSETTINGSUPDATEDEVENT), clubSettingsUpdatedEvent, true /*logBinaryEvent*/);
			}

            TRACE_LOG("[UpdateClubSettingsCommand].execute: User [" << blazeId << "] updated settings in the club [" 
                << clubId << "], had permission.");

            return ERR_OK;

        }
    };

    // static factory method impl
    DEFINE_UPDATECLUBSETTINGS_CREATE()

} // Clubs
} // Blaze
