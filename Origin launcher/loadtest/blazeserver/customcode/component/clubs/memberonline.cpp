/*************************************************************************************************/
/*!
    \file   memberonline.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

// clubs includes
#include "clubs/clubsslaveimpl.h" // FIFA SPECIFIC CODE
#include "rivals.h"

namespace Blaze
{
namespace Clubs
{

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief onMemberOnline

    Custom processing upon first club member comes online.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::onMemberOnline(ClubsDatabase * clubsDb,  
        const ClubIdList &clubIdList, const BlazeId blazeId)
{
    BlazeRpcError result = ERR_OK;
    // FIFA SPECIFIC CODE START
    uint64_t version = 0;

    for (ClubIdList::const_iterator it = clubIdList.begin(); 
         it != clubIdList.end() && result == Blaze::ERR_OK; 
         it++)
    {
        ClubId clubId = *it;

    Club club;
    result = clubsDb->getClub(clubId, &club, version);
    if (result != Blaze::ERR_OK)
    {
        WARN_LOG("[Clubs::onMemberOnline] - Failed to obtain club " << clubId << ".");
            return Blaze::ERR_SYSTEM;
        }

    /************************
        CLUB RIVAL MODE
    ************************/
    {
        uint32_t rivalOpt = club.getClubSettings().getCustClubSettings().getCustOpt5();
        if (rivalOpt == CLUBS_RIVAL_ASSIGNED)
        {
        // if club has rival assigned check if there was season rollover
        
            ClubRivalList rivalList;
            result = clubsDb->listClubRivals(clubId, &rivalList);
            
            if (result != Blaze::ERR_OK || rivalList.size() == 0)
            {
                WARN_LOG("[Clubs::onMemberOnline] - Failed to obtain rivals for club " << clubId << ".");
            }
            else
            {            
                const ClubRival *rival = *rivalList.begin();
                if (static_cast<int64_t>(rival->getCreationTime()) < mSeasonRollover.getSeasonStart())
                {
                    // we need new rival if we are eligible
                    if (club.getClubSettings().getAcceptanceFlags().getCLUBS_ACCEPT_RIVALS()
                        && club.getClubInfo().getMemberCount() >= minMemberCountReqForRivals)
                    {
                        result = assignRival(clubsDb, &club, true);

                        if(result != Blaze::ERR_OK)
                        {
                            WARN_LOG("[Clubs::onMemberOnline] - Failed to Assign Rival to club [" << clubId << "].");
                        }
                        result = setIsRankedOnRivalLeaderboard(clubId, true);
                        if(result != Blaze::ERR_OK)
                        {
                            WARN_LOG("[Clubs::onMemberOnline] error listing club [" << clubId << "] on rival leaderboard.");
                        }
                    }
                    else 
                    {  
                        // we no longer accept rivals so make us non-eligible
                        ClubSettings& settings = club.getClubSettings();
                        settings.getCustClubSettings().setCustOpt5(CLUBS_RIVAL_NOT_ELIGIBLE);
                        result = clubsDb->updateClubSettings(clubId, settings, version);
                        if (result != Blaze::ERR_OK)
                        {
                            WARN_LOG("[Clubs::onMemberOnline] - Failed to update club settings for club " << clubId << ".");
                        }
                        else
                        {
                            result = setIsRankedOnRivalLeaderboard(clubId, false);
                            if (result != Blaze::ERR_OK)
                            {
                                WARN_LOG("[Clubs::onMemberOnline] error unlisting club [" << clubId << "] on rival leaderboard.");
                            }
                        }
                        updateCachedClubInfo(version, clubId, club);

                    } 
                } 
            }
        }
        else if (rivalOpt == CLUBS_RIVAL_PENDING_ASSIGNMENT)
        {
            result = assignRival(clubsDb, &club, false);
            if(result != Blaze::ERR_OK)
            {
                WARN_LOG("[Clubs::onMemberOnline] - Failed to Assign Rival to club [" << clubId << "].");
            }

            result = setIsRankedOnRivalLeaderboard(clubId, true);
            if(result != Blaze::ERR_OK)
            {
                WARN_LOG("[Clubs::onMemberOnline] error listing club [" << clubId << "] on rival leaderboard.");
            }
        }
    }
    // end of CLUB RIVAL MODE

	}// for each Club Id in clubIdList
    // FIFA SPECIFIC CODE END
    return result;
}   
} // Clubs
} // Blaze
