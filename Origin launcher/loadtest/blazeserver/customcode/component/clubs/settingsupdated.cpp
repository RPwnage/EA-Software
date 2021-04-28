/*************************************************************************************************/
/*!
    \file   settingsupdated.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// clubs includes
#include "clubs/clubsslaveimpl.h"
#include "clubs/rivals.h"
#include "stats/statsslaveimpl.h"
// FIFA SPECIFIC CODE START
#include "gamereporting/osdk/gameclub.h"    // for CATEGORYNAME_CLUB_SEASON

// seasonal play includes
#include "osdkseasonalplay/rpc/osdkseasonalplayslave.h"

#include "framework/util/profanityfilter.h"
// FIFA SPECIFIC CODE END

namespace Blaze
{
namespace Clubs
{

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief onSettingsUpdated

    Custom processing upon member metadata updated.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::onSettingsUpdated(ClubsDatabase * clubsDb, const ClubId clubId,
                                                const ClubSettings* originalSettings,
                                                const ClubSettings* newSettings)
{
    // FIFA SPECIFIC CODE START
    BlazeRpcError result = Blaze::ERR_OK;
    Stats::StatsSlaveImpl* stats = static_cast<Stats::StatsSlaveImpl*>(gController->getComponent(Stats::StatsSlaveImpl::COMPONENT_ID, false));
    uint64_t version = 0;
    
    // Check club metadata for profanity
    result = checkClubMetadata(*newSettings);
    if (result != Blaze::ERR_OK)
    {
        return result;
    }

    /******************************
    CLUB REGION CHANGE
    ******************************/
    {
        // if a club changes region, update the keyscope used for ClubRankStats stats category so
        // the club stats will be moved to the new region keyscope value
        if (originalSettings->getRegion() != newSettings->getRegion())
        {
            const char8_t *strKeyscope = "clubregion";
            Stats::KeyScopeChangeRequest keyscopeChangeRequest;
            keyscopeChangeRequest.setEntityType(ENTITY_TYPE_CLUB);
            keyscopeChangeRequest.setEntityId(static_cast<EntityId>(clubId));
            keyscopeChangeRequest.setKeyScopeName(strKeyscope);
            keyscopeChangeRequest.setOldKeyScopeValue(static_cast<Stats::ScopeValue>(originalSettings->getRegion()));
            keyscopeChangeRequest.setNewKeyScopeValue(static_cast<Stats::ScopeValue>(newSettings->getRegion()));

            stats->changeKeyscopeValue(keyscopeChangeRequest);
        }
    }

    /******************************
    CLUB NAME CHANGE
    ******************************/
    {
        // check if the club's name has changed
        if (originalSettings->getClubName() != newSettings->getClubName())
        {
            result = clubsDb->changeIsNameProfaneFlag(clubId, 0);
            if (result != Blaze::ERR_OK)
            {
                WARN_LOG("[Clubs::UpdateClubSettings] error updating IsNameProfane flag for clubId " << clubId << ".");
            }
        }
    }

    /******************************
        OSDK SEASONAL PLAY (the seasonal play)
    ******************************/
    {
        // check if the club's league has changed
        if (originalSettings->getSeasonLevel() != newSettings->getSeasonLevel())
        {
            // update the club's seasonal play registration to put it in the appropriate season for the new league
            OSDKSeasonalPlay::OSDKSeasonalPlaySlave *seasonalPlayComponent =
                static_cast<OSDKSeasonalPlay::OSDKSeasonalPlaySlave*>(gController->getComponent(OSDKSeasonalPlay::OSDKSeasonalPlaySlave::COMPONENT_ID, false));
            if (seasonalPlayComponent != nullptr)
            {
                OSDKSeasonalPlay::UpdateClubRegistrationRequest req;
                req.setClubId(static_cast<OSDKSeasonalPlay::MemberId>(clubId));
                req.setLeagueId(static_cast<OSDKSeasonalPlay::LeagueId>(newSettings->getSeasonLevel()));
                result = seasonalPlayComponent->updateClubRegistration(req);
                if(ERR_OK != result)
                {
                    ERR_LOG("[ClubsSlaveImpl:" << this << "].onSettingsUpdated(). Failed updateClubRegistration rpc on seasonalplay component.");
                }
                else
                {
                    Stats::StatsSlaveImpl* pStatsComponent = static_cast<Stats::StatsSlaveImpl*>(gController->getComponent(Stats::StatsSlaveImpl::COMPONENT_ID, false));
                    // delete club seasonal play stats for the old league
                    const Stats::CategoryMap* categoryMap = pStatsComponent->getConfigData()->getCategoryMap();
                    Stats::CategoryMap::const_iterator catIter = categoryMap->find(CATEGORYNAME_CLUB_SEASON);
                    if (catIter != categoryMap->end())
                    {
                        Stats::StatDelete statDelete;
                        statDelete.setCategory(CATEGORYNAME_CLUB_SEASON);
                        statDelete.setEntityId(clubId);
                        Stats::DeleteStatsRequest deleteStatsRequest;
                        deleteStatsRequest.getStatDeletes().push_back(&statDelete);

                        const Stats::StatCategory* cat = catIter->second;

                        // exclude STAT_PERIOD_ALL_TIME according to producer.
                        for (int32_t period = Stats::STAT_PERIOD_ALL_TIME + 1; period < Stats::STAT_NUM_PERIODS; ++period)
                        {
                            if (cat->isValidPeriod(period))
                            {
                                statDelete.getPeriodTypes().push_back(period);
                            }
                        }

                        pStatsComponent->deleteStats(deleteStatsRequest);
                        deleteStatsRequest.getStatDeletes().pop_back();// by default, TdfStructVector (returned by getStatDeletes) owns memory of element which is not the case here. pop the element, to prevent the vector deleting it.
                    }
                }
            }
            else
            {
                TRACE_LOG("[ClubsSlaveImpl].onSettingsUpdated() - osdkseasonalplay component is not available");
                result = Blaze::ERR_OK;
            }
  
        }
    }
    // end of OSDK Seasonal Play


    /************************
        CLUB RIVAL MODE
    ************************/
    {
        // check if club rival acceptance flag has been modified
        if (originalSettings->getAcceptanceFlags().getCLUBS_ACCEPT_RIVALS() !=
            newSettings->getAcceptanceFlags().getCLUBS_ACCEPT_RIVALS())
        {
            Club club;
            result = clubsDb->getClub(clubId, &club, version);
            if (result != Blaze::ERR_OK)
            {
                WARN_LOG("[Clubs::UpdateClubSettings] error fetching club " << clubId << ".");
            }
            else if (newSettings->getAcceptanceFlags().getCLUBS_ACCEPT_RIVALS())
            {
                // this club just turned on the Rival mode
                if ((club.getClubInfo().getMemberCount() >= minMemberCountReqForRivals) &&
                    (club.getClubSettings().getCustClubSettings().getCustOpt5() == CLUBS_RIVAL_NOT_ELIGIBLE))
                {
                    // this club has enough user to get the Rival pair up now
                    result = assignRival(clubsDb, &club, false);
                    if (result != Blaze::ERR_OK)
                    {
                        WARN_LOG("[Clubs::UpdateClubSettings] error assigning rivial to club " << clubId << ".");
                    }

                    result = setIsRankedOnRivalLeaderboard(clubId, true);
                    if (result != Blaze::ERR_OK)
                    {
                        WARN_LOG("[Clubs::UpdateClubSettings] error enlisting club " << clubId << " on rival leaderboard.");
                    }
                }
            }
            else if (club.getClubSettings().getCustClubSettings().getCustOpt5() == CLUBS_RIVAL_PENDING_ASSIGNMENT)
            {
                // we no longer accept rivals so make us non-eligible
                ClubSettings& settings = club.getClubSettings();
                settings.getCustClubSettings().setCustOpt5(CLUBS_RIVAL_NOT_ELIGIBLE);
                result = clubsDb->updateClubSettings(clubId, settings, version);
                if (result != Blaze::ERR_OK)
                {
                    ERR_LOG("[ClubsSlaveImpl].onSettingsUpdated - Failed to update club settings for club " << clubId << ".");
                }
                else
                {
                    result = setIsRankedOnRivalLeaderboard(clubId, false);
                    if (result != Blaze::ERR_OK)
                    {
                        WARN_LOG("[Clubs::UpdateClubSettings] error unlisting club " << clubId << " on rival leaderboard.");
                    }
                }
                
                updateCachedClubInfo(version, club.getClubId(), club);
            }
        }

        if (originalSettings->getRegion() != newSettings->getRegion())
        {
            Stats::KeyScopeChangeRequest request;
            request.setEntityType(ENTITY_TYPE_CLUB);
            request.setEntityId(static_cast<EntityId>(clubId));
            request.setKeyScopeName("clubregion");
            request.setOldKeyScopeValue(static_cast<Stats::ScopeValue>(originalSettings->getRegion()));
            request.setNewKeyScopeValue(static_cast<Stats::ScopeValue>(newSettings->getRegion()));

            stats->changeKeyscopeValue(request);
        }
    }
    // end of CLUB RIVAL MODE
    return result;
    // FIFA SPECIFIC CODE END
}
} // Clubs
} // Blaze
