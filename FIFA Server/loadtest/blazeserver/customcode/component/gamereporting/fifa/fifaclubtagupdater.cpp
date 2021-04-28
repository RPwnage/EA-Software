/*************************************************************************************************/
/*!
    \file   fifaclubtagupdater.cpp
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "gamereporting/fifa/fifaclubtagupdater.h"

namespace Blaze
{
namespace GameReporting
{
namespace FIFA
{

FifaClubTagUpdater::FifaClubTagUpdater() 
	: mComponent(NULL), 
	  mClubsExtension(NULL)
{
}

bool8_t FifaClubTagUpdater::intialize()
{
	if (NULL == gController)
	{
		return false;
	}
	mComponent = static_cast<Clubs::ClubsSlave*>(gController->getComponent(Clubs::ClubsSlave::COMPONENT_ID, false));    
	if (NULL == mComponent)
	{
		return false;
	}
	return true;
}

void FifaClubTagUpdater::setExtension(ClubSeasonsExtension* clubsExtension)
{
	mClubsExtension = clubsExtension;
}


/// 
/// Function used to determine which clubs need tag updates and commit them to the database. 
///
/// @notes: 
///  - Currently only used to update "ClubDiv" tag.
///  - The old and new division lists must be filled prior to this call. 
/// 
bool8_t FifaClubTagUpdater::transformTags()
{
	bool8_t success = true;
	if (NULL == mClubsExtension)
	{
		WARN_LOG("[FifaClubTagUpdater:" << this << "].transformTags() no extension set");
		return false;
	}

	if (false == intialize())
	{
		WARN_LOG("[FifaClubTagUpdater:" << this << "].transformTags() could not initialize");
		return false;
	}

	eastl::vector<int64_t> clubList;
	mClubsExtension->getEntityIds(clubList);

	TRACE_LOG("[FifaClubTagUpdater:" << this << "].transformTags() size: " << clubList.size());

	IFifaSeasonalPlayExtension::EntityList::iterator clubIter = clubList.begin();
	IFifaSeasonalPlayExtension::EntityList::iterator clubEnd = clubList.end();

	Clubs::Club club;

	for(; (clubIter != clubEnd); ++clubIter)
	{
		getClub(*clubIter, club);
		int64_t division = mClubsExtension->getDivision(*clubIter);

		if (true == changeClubDivision(club.getTagList(), division))
		{
			// Division has changed, send the update request
			if (false == sendUpdateRequest(*clubIter, club))
			{
				success = false;
				TRACE_LOG("[FifaClubTagUpdater:" << this << "].transformTags() failedupdate: " << *clubIter);
				// No break - try to update the remaining clubs if any.
			}
		}
	}
	TRACE_LOG("[FifaClubTagUpdater:" << this << "].transformTags() done");
	return success;
}

/// 
/// Helper function, returns the full Club structure for the clubId.
/// Useful for obtaining tags. (no other request retrieves these)
/// 
/// @param[in]  clubId - Id to obtain club info for.
/// @param[out] club   - will be filled with information returned by server.
///
bool FifaClubTagUpdater::getClub(Clubs::ClubId clubId, Clubs::Club& club)
{
	if (mComponent != NULL)
	{
		Clubs::GetClubsRequest getClubsRequest;
		Clubs::GetClubsResponse getClubsResponse;

		getClubsRequest.getClubIdList().push_back(clubId);
		getClubsRequest.setMaxResultCount(1);
		getClubsRequest.setIncludeClubTags(true);

		UserSession::pushSuperUserPrivilege();
		BlazeRpcError error = mComponent->getClubs(getClubsRequest, getClubsResponse);
		UserSession::popSuperUserPrivilege();

		int32_t noOfclubs = 0;
		if (error == Blaze::ERR_OK)
		{
			Clubs::ClubList& responseClubList = getClubsResponse.getClubList();

			Clubs::ClubList::iterator it = responseClubList.begin();
			if (it != responseClubList.end())
			{
				Clubs::Club *cl = *it;
				cl->copyInto(club);
			}

			noOfclubs = static_cast<int32_t>(responseClubList.size());
		}
		
		TRACE_LOG("[FifaClubTagUpdater::getClub:" << this << "] Could not get clubs list for clubId: "<< clubId 
				<< " No. Clubs Returned: " << noOfclubs << " Error:"<< error);
		
		return (error == Blaze::ERR_OK);
	}
	return false;
}

/// 
/// Helper function used to update the list of tags with the new division.
/// 
/// @param[in|out] tagList     - List of tags to search and update
/// @param[in]     newDivision - the new division that this club belongs to
/// 
/// @returns - true if any change done, false otherwise.
bool8_t FifaClubTagUpdater::changeClubDivision(Clubs::ClubTagList& tagList, int64_t newDivision)
{
	bool dbUpdateNeeded = false;

	Clubs::ClubTagList::iterator tagIter = tagList.begin();
	Clubs::ClubTagList::iterator tagEnd = tagList.end();

	bool8_t found = false;
	for(; (tagIter!=tagEnd) && (!found); ++tagIter)
	{
		Clubs::ClubTag& clubTag= *tagIter;
		if (0 == strncmp(clubTag.c_str(), "ClubDiv", strlen("ClubDiv")))
		{
			eastl::string str;
			str.sprintf("ClubDiv%d", newDivision);

			if (0 != strcmp(clubTag.c_str(), str.c_str()))
			{
				clubTag.set( str.c_str() );
				dbUpdateNeeded = true;
			}
			found = true;
		}
	}
	if (!found)
	{
		eastl::string str;
		str.sprintf("ClubDiv%d", newDivision);
		tagList.push_back(str.c_str());
		dbUpdateNeeded = true;
	}

	return dbUpdateNeeded;
}


/*************************************************************************************************/
/*!
    \brief updateClubData

    Updates a club settings and tags with items found in club structure.

    \param[in]     clubId     - ID of the club to set the tag list for
    \param[in]     club       - class with all information that needs to be set on club.

    \return - true if id is valid, false otherwise or in case of system error
*/
/*************************************************************************************************/
bool FifaClubTagUpdater::sendUpdateRequest(Clubs::ClubId clubId, const Clubs::Club &club)
{
    if (mComponent != NULL)
    {
		Clubs::UpdateClubSettingsRequest request;

		request.setClubId(clubId);
		Clubs::ClubTagList& requestTags = request.getTagList();
		club.getTagList().copyInto(requestTags);

		// These are all the settings except club tags. updateClubSettings expects all fields to be set to intended target.
		club.getClubSettings().copyInto(request.getClubSettings());

		UserSession::pushSuperUserPrivilege();
		BlazeRpcError error = mComponent->updateClubSettings(request);
		UserSession::popSuperUserPrivilege();

		return (error == Blaze::ERR_OK);
    }
    return false;
}


}
}
}

