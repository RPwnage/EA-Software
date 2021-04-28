/*************************************************************************************************/
/*!
    \file   customhooks.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// osdktournaments includes
#include "osdktournaments/tdf/osdktournamentstypes_custom.h"
#include "osdktournaments/osdktournamentsslaveimpl.h"
#include "osdktournaments/osdktournamentsdatabase.h"

// database includes
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"

// other includes
#include "framework/component/blazerpc.h"

// seasonal play includes
#include "osdkseasonalplay/rpc/osdkseasonalplayslave.h"
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"

// fifacups includes
#include "fifacups/tdf/fifacupstypes.h"
#include "fifacups/fifacupsslaveimpl.h"

// clubs includes
#include "clubs/rpc/clubsslave.h"
#include "clubs/tdf/clubs.h"

namespace Blaze
{
namespace OSDKTournaments
{

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief getCurrentUserMemberIdForMode

    Custom processing to determine a TournamentMemberId for the user from a mode.
*/
/*************************************************************************************************/
TournamentMemberId OSDKTournamentsSlaveImpl::getCurrentUserMemberIdForMode(TournamentMode tournamentMode)
{
    TournamentMemberId tempMemberId = INVALID_TOURNAMENT_MEMBER_ID;
    if (tournamentMode == MODE_USER || tournamentMode == MODE_SEASONAL_USER || tournamentMode == MODE_FIFA_CUPS_USER)
    {
        // use the user id
        tempMemberId = static_cast<TournamentMemberId>( gCurrentUserSession->getUserId() );
    }
    else if (tournamentMode == MODE_SEASONAL_CLUB || tournamentMode == MODE_FIFA_CUPS_CLUB)
    {
        // search the extended data for the club id and use it
        UserSessionExtendedData userExtendedData;
        
        UserSessionId sessionId = gCurrentUserSession->getSessionId();
        BlazeRpcError error = gUserSessionManager->getRemoteUserExtendedData(sessionId, userExtendedData);

        if (ERR_OK != error)
        {
            ERR_LOG("[OSDKTournamentsSlaveImpl:" << this << "].getCurrentUserMemberIdForMode: unable to get UserExtendedData for session " << sessionId);
            return tempMemberId; // Early return
        }

        const BlazeObjectIdList& objectIds = userExtendedData.getBlazeObjectIdList();

        for (BlazeObjectIdList::const_iterator i = objectIds.begin(), e = objectIds.end(); i != e; ++i)
        {
            const BlazeObjectId objId = *i;
            if (Clubs::ENTITY_TYPE_CLUB == objId.type)
            {
                tempMemberId = static_cast<TournamentMemberId>(objId.id);
                break;
            }
        }
    }

    return tempMemberId;
}

/*************************************************************************************************/
/*!
    \brief getMemberName

    Custom processing to return the name of a member based on tournament mode.
*/
/*************************************************************************************************/
bool OSDKTournamentsSlaveImpl::getMemberName(TournamentMemberId memberId, TournamentMode tournamentMode, eastl::string &memberName)
{
    bool bResult = false;
    memberName.clear();

    if (tournamentMode == MODE_USER || tournamentMode == MODE_SEASONAL_USER || tournamentMode == MODE_FIFA_CUPS_USER)
    {
        UserInfoPtr memberUserInfo;
        BlazeRpcError memberLookupError = gUserSessionManager->lookupUserInfoByBlazeId(static_cast<BlazeId>(memberId), memberUserInfo);
        if ((ERR_OK == memberLookupError) && (NULL != memberUserInfo))
        {
            memberName = memberUserInfo->getPersonaName();
            bResult = true;
        }
        else
        {
            ERR_LOG("[OSDKTournamentsSlaveImpl:" << this << "].getMemberName: unable to look up user " << static_cast<BlazeId>(memberId));
        }
    }
    else if (tournamentMode == MODE_SEASONAL_CLUB || tournamentMode == MODE_FIFA_CUPS_CLUB)
    {
        Blaze::BlazeObjectType objectType = BlazeRpcComponentDb::getBlazeObjectTypeByName("clubs.club");
        if (true == BlazeRpcComponentDb::hasIdentity(objectType))
        {
            EntityId memberEntityId = static_cast<EntityId>(memberId);
            EntityIdList entityIdList;
            entityIdList.push_back(memberEntityId);

            IdentityInfoByEntityIdMap identityMap;
            BlazeRpcError identityError = gIdentityManager->getIdentities(objectType, entityIdList, identityMap);

            IdentityInfoByEntityIdMap::const_iterator identEnd = identityMap.end();
            if (Blaze::ERR_OK == identityError)
            {
                // once we have the map, search for the entity we need and get the name
                IdentityInfoByEntityIdMap::const_iterator identItr = identityMap.find(memberEntityId);
                if (identItr != identEnd)
                {
                    memberName = identItr->second->getIdentityName();
                    bResult = true;
                }
                else
                {
                    ERR_LOG("[OSDKTournamentsSlaveImpl:" << this << "].getMemberName: unable to determine identity of entity " << memberEntityId);
                }
            }
            else if (Blaze::CLUBS_ERR_INVALID_CLUB_ID != identityError) // The club can be destroyed by the club manager at any time, so suppress this particular error.
            {
                ERR_LOG("[OSDKTournamentsSlaveImpl:" << this << "].getMemberName: error " << identityError << " occured when determining the identity of entity " << memberEntityId);
            } 
        }
    }

    TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].getMemberName: returning member name: " << memberName.c_str());
    return bResult;
}

/*** Private Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief getMemberMetaData

    Custom processing to return/set the meta data of a member based on tournament mode.
*/
/*************************************************************************************************/
void OSDKTournamentsSlaveImpl::getMemberMetaData(
    TournamentMemberId memberId, TournamentMode tournamentMode, char8_t* memberMetaData, size_t memberMetaDataBufferSize)
{
    // currently only supporting meta-data for seasonal clubs (club abbreviation)
    if (tournamentMode == MODE_SEASONAL_CLUB || tournamentMode == MODE_FIFA_CUPS_CLUB)
    {
        // this now introduces an official dependency on clubs...
        Blaze::Clubs::ClubsSlave *clubsComponent = static_cast<Clubs::ClubsSlave*>(gController->getComponent(Clubs::ClubsSlave::COMPONENT_ID,false));
        if (NULL != clubsComponent)
        {
            Blaze::Clubs::Club memberClub;
            memberClub.setClubId(static_cast<Blaze::Clubs::ClubId>(memberId));
            
            Blaze::Clubs::GetClubsRequest request;
            Blaze::Clubs::GetClubsResponse response;
            Blaze::Clubs::ClubIdList &idList = request.getClubIdList();
            idList.push_back(static_cast<Blaze::Clubs::ClubId>(memberId));

            BlazeRpcError error = clubsComponent->getClubs(request, response);
            if (ERR_OK == error)
            {
                Blaze::Clubs::ClubList& clubList = response.getClubList();
                if(1 != clubList.size())
                {
                    ERR_LOG("[OSDKTournamentsSlaveImpl:" << this << "].getMemberMetaData: getClubs returned " << clubList.size() << " clubs for club id" << memberId);
                    blaze_strnzcpy(memberMetaData,"", memberMetaDataBufferSize);
                }
                else
                {
                    blaze_strnzcpy(memberMetaData, clubList[0]->getClubSettings().getNonUniqueName(), memberMetaDataBufferSize);
                    TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].getMemberMetaData: returning club abbreviation: " << memberMetaData << " for member: " << memberId);
                }
            }
            else
            {
                ERR_LOG("[OSDKTournamentsSlaveImpl:" << this << "].getMemberMetaData: getClubs returned error " << error);
                blaze_strnzcpy(memberMetaData,"", memberMetaDataBufferSize);
            }
        }
        else
        {
            ERR_LOG("[OSDKTournamentsSlaveImpl:" << this << "].getMemberMetaData: unable to find Clubs component");
            blaze_strnzcpy(memberMetaData,"", memberMetaDataBufferSize);
        }
    }
    else
    {
        // other modes
        strcpy(memberMetaData, "");
    }

    return;
}

/*************************************************************************************************/
/*!
    \brief onLoadExtendedDataCustom

    Custom processing to add tournaments to extended data on load.
*/
/*************************************************************************************************/
void OSDKTournamentsSlaveImpl::onLoadExtendedDataCustom(const UserInfoData &data, UserSessionExtendedData &extendedData)
{
    DbConnPtr conn = gDbScheduler->getReadConnPtr(mDbId);
    if (conn == NULL)
    {
        // early return here when the scheduler is unable to find a connection
        ERR_LOG("[OSDKTournamentsSlaveImpl:" << this << "].onLoadExtendedDataCustom: unable to find a database connection");
        return;
    }
    OSDKTournamentsDatabase tournDb(conn);

    // find all tournaments for all modes
    TournamentMode currentMode = MODE_USER;
    while (currentMode < NUM_MODES)
    {
        TournamentMemberId tempMemberId = INVALID_TOURNAMENT_MEMBER_ID;
        if (currentMode == MODE_USER || currentMode == MODE_SEASONAL_USER || currentMode == MODE_FIFA_CUPS_USER)
        {
            // use the user id
            tempMemberId = static_cast<TournamentMemberId>(data.getId());
        }
        else if (currentMode == MODE_SEASONAL_CLUB || currentMode == MODE_FIFA_CUPS_CLUB)
        {
            // search the extended data for the club id and use it
            const BlazeObjectIdList& objectIds = extendedData.getBlazeObjectIdList();
            for (BlazeObjectIdList::const_iterator i = objectIds.begin(), e = objectIds.end(); i != e; ++i)
            {
                const BlazeObjectId objId = *i;
                if (Clubs::ENTITY_TYPE_CLUB == objId.type)
                {
                    tempMemberId = static_cast<TournamentMemberId>( objId.id );
                    break;
                }
            }
        }

        TournamentMemberData memberData;
        if (INVALID_TOURNAMENT_MEMBER_ID != tempMemberId && 
            Blaze::DB_ERR_OK == tournDb.getMemberStatus(tempMemberId, currentMode, memberData))
        {
            TournamentId thisTournamentId = memberData.getTournamentId();
            // make sure the tournament is valid and enabled before adding the entityid
            if (getTournamentsMap().find(thisTournamentId) != getTournamentsMap().end())
            {
                TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].onLoadExtendedDataCustom pushing BlazeObjectId to UED");
                BlazeObjectId objId = BlazeObjectId(ENTITY_TYPE_TOURNAMENT, thisTournamentId);
                extendedData.getBlazeObjectIdList().push_back(objId);
            }
        }

        currentMode++;
    }
}

/*************************************************************************************************/
/*!
    \brief onMemberWonCustom

    Custom processing to handle the event when a member wins a tournament.
*/
/*************************************************************************************************/
void OSDKTournamentsSlaveImpl::onMemberWonCustom(TournamentMemberId winnerId, TournamentMode mode)
{
    // inform seasonal play of the member winning a tournament.
    // requires:
    //  - tournament mode is a seasonal play mode
    //  - member is registered in seasonal play
    //  - season state is in playoff state
    onMemberCustomStatus(winnerId, mode, Blaze::OSDKSeasonalPlay::SEASONALPLAY_TOURNAMENT_STATUS_WON);
}


/*************************************************************************************************/
/*!
    \brief onMemberEliminatedCustom

    Custom processing to handle the event when a member is eliminated from a tournament.
*/
/*************************************************************************************************/
void OSDKTournamentsSlaveImpl::onMemberEliminatedCustom(TournamentMemberId eliminateeId, TournamentMode mode, int32_t eliminatedLevel)
{
    // inform seasonal play of the member being eliminated from a tournament.
    // requires:
    //  - tournament mode is a seasonal play mode
    //  - member is registered in seasonal play
    //  - season state is in playoff state
    onMemberCustomStatus(eliminateeId, mode, Blaze::OSDKSeasonalPlay::SEASONALPLAY_TOURNAMENT_STATUS_ELIMINATED,eliminatedLevel);
}

/*************************************************************************************************/
/*!
    \brief onMemberCustomStatus

    Custom processing to handle the event when a member is eliminated from or win the tournament.
*/
/*************************************************************************************************/
void OSDKTournamentsSlaveImpl::onMemberCustomStatus(TournamentMemberId memberId, TournamentMode mode, Blaze::OSDKSeasonalPlay::TournamentStatus status, int32_t tournamentLevel)
{
    if (MODE_USER == mode)
    {
        // seasonal play doesn't care about this mode so exit
        return; // early return
    }

	if (MODE_SEASONAL_CLUB == mode || MODE_SEASONAL_USER == mode)
	{
		OSDKSeasonalPlay::OSDKSeasonalPlaySlave *seasonalPlayComponent =
			static_cast<OSDKSeasonalPlay::OSDKSeasonalPlaySlave*>(gController->getComponent(OSDKSeasonalPlay::OSDKSeasonalPlaySlave::COMPONENT_ID,false));
		if (seasonalPlayComponent != NULL)
		{
			BlazeRpcError error = Blaze::ERR_OK;

			// determine the member type
			Blaze::OSDKSeasonalPlay::MemberType memberType = Blaze::OSDKSeasonalPlay::SEASONALPLAY_MEMBERTYPE_CLUB;
			if (MODE_SEASONAL_USER == mode)
			{
				memberType = Blaze::OSDKSeasonalPlay::SEASONALPLAY_MEMBERTYPE_USER;
			}

			// is the member registered in seasonal play?
			Blaze::OSDKSeasonalPlay::GetSeasonIdRequest seasonIdReq;
			Blaze::OSDKSeasonalPlay::GetSeasonIdResponse seasonIdResp;
			seasonIdReq.setMemberId( static_cast<Blaze::OSDKSeasonalPlay::MemberId>(memberId));
			seasonIdReq.setMemberType(memberType);
			error = seasonalPlayComponent->getSeasonId(seasonIdReq,seasonIdResp);
			if (Blaze::ERR_OK != error)
			{
				// member not registered in seasonal play, exit
				return; // early return
			}
			Blaze::OSDKSeasonalPlay::SeasonId seasonId = seasonIdResp.getSeasonId();

			Blaze::OSDKSeasonalPlay::GetSeasonDetailsRequest seasonDetailReq;
			Blaze::OSDKSeasonalPlay::SeasonDetails seasonDetails;
			seasonDetailReq.setSeasonId(seasonId);
			error = seasonalPlayComponent->getSeasonDetails(seasonDetailReq,seasonDetails);
			if (Blaze::ERR_OK != error)
			{
				ERR_LOG("[OSDKTournamentsSlaveImpl:" << this << "].onMemberCustomStatus: unable to get season details.");
				return; // early return
			}

			if (Blaze::OSDKSeasonalPlay::SEASONALPLAY_SEASON_STATE_PLAYOFF == seasonDetails.getSeasonState())
			{
				// update the member's seasonal play tournament status
				Blaze::OSDKSeasonalPlay::SetMemberTournamentStatusRequest req;
				req.setMemberId(memberId);
				req.setMemberType(memberType);
				req.setTournamentStatus(status);

				error = seasonalPlayComponent->setMemberTournamentStatus(req);
				if (Blaze::ERR_OK != error)
				{
					ERR_LOG("[OSDKTournamentsSlaveImpl:" << this << "].onMemberCustomStatus: unable to set tournament status for member.");
				}
			}
		}
	}
	else if (MODE_FIFA_CUPS_USER == mode || MODE_FIFA_CUPS_CLUB == mode || MODE_FIFA_CUPS_COOP == mode)
	{
		TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].onMemberCustomStatus");
		FifaCups::FifaCupsSlave *fifaCupsComponent =
			static_cast<FifaCups::FifaCupsSlave*>(gController->getComponent(FifaCups::FifaCupsSlave::COMPONENT_ID,false));
		if (fifaCupsComponent != NULL)
		{
			FifaCups::MemberType memberType = FifaCups::FIFACUPS_MEMBERTYPE_USER;
			switch (mode)
			{
			case MODE_FIFA_CUPS_USER:	memberType = FifaCups::FIFACUPS_MEMBERTYPE_USER; break;
			case MODE_FIFA_CUPS_CLUB:	memberType = FifaCups::FIFACUPS_MEMBERTYPE_CLUB; break;
			case MODE_FIFA_CUPS_COOP:	memberType = FifaCups::FIFACUPS_MEMBERTYPE_COOP; break;
			default:					memberType = FifaCups::FIFACUPS_MEMBERTYPE_USER; break;
			}

			FifaCups::UpdateTournamentResultRequest request;
			request.setMemberId(static_cast<Blaze::FifaCups::MemberId>(memberId));
			request.setMemberType(memberType);

			FifaCups::TournamentResult result = FifaCups::FIFACUPS_TOURNAMENTRESULT_NONE;
			switch (status)
			{
			case Blaze::OSDKSeasonalPlay::SEASONALPLAY_TOURNAMENT_STATUS_WON:
				result = FifaCups::FIFACUPS_TOURNAMENTRESULT_WON;
				break;
			case Blaze::OSDKSeasonalPlay::SEASONALPLAY_TOURNAMENT_STATUS_ELIMINATED:
				result = FifaCups::FIFACUPS_TOURNAMENTRESULT_ELIMINATED;
				break;
			case Blaze::OSDKSeasonalPlay::SEASONALPLAY_TOURNAMENT_STATUS_NONE:
			default:
				result = FifaCups::FIFACUPS_TOURNAMENTRESULT_NONE;
				break;
			}
			request.setTournamentResult(result);
			request.setTournamentLevel(tournamentLevel);

			UserSession::pushSuperUserPrivilege();
			fifaCupsComponent->updateTournamentResult(request);
			UserSession::popSuperUserPrivilege();
		}
	}
}

/*************************************************************************************************/
/*!
    \brief onMemberAdvancedCustom

    Custom processing to handle the event when a member advances in a tournament.
*/
/*************************************************************************************************/
void OSDKTournamentsSlaveImpl::onMemberAdvancedCustom(TournamentMemberId memberId, TournamentMode mode, int32_t newLevel)
{
	if (MODE_FIFA_CUPS_USER == mode || MODE_FIFA_CUPS_CLUB == mode || MODE_FIFA_CUPS_COOP == mode)
	{
		TRACE_LOG("[OSDKTournamentsSlaveImpl:" << this << "].onMemberCustomStatus");
		FifaCups::FifaCupsSlave *fifaCupsComponent =
			static_cast<FifaCups::FifaCupsSlave*>(gController->getComponent(FifaCups::FifaCupsSlave::COMPONENT_ID,false));
		if (fifaCupsComponent != NULL)
		{	
			FifaCups::MemberType memberType = FifaCups::FIFACUPS_MEMBERTYPE_USER;
			switch (mode)
			{
			case MODE_FIFA_CUPS_USER:	memberType = FifaCups::FIFACUPS_MEMBERTYPE_USER; break;
			case MODE_FIFA_CUPS_CLUB:	memberType = FifaCups::FIFACUPS_MEMBERTYPE_CLUB; break;
			case MODE_FIFA_CUPS_COOP:	memberType = FifaCups::FIFACUPS_MEMBERTYPE_COOP; break;
			default:					memberType = FifaCups::FIFACUPS_MEMBERTYPE_USER; break;
			}

			FifaCups::UpdateTournamentResultRequest request;
			request.setMemberId(static_cast<Blaze::FifaCups::MemberId>(memberId));
			request.setMemberType(memberType);
			request.setTournamentResult(FifaCups::FIFACUPS_TOURNAMENTRESULT_ADVANCED);
			request.setTournamentLevel(newLevel);

			UserSession::pushSuperUserPrivilege();
			fifaCupsComponent->updateTournamentResult(request);
			UserSession::popSuperUserPrivilege();
		}
	}
}

} // OSDKTournaments
} // Blaze
