/*************************************************************************************************/
/*!
    \file   clubcreated.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
// FIFA SPECIFIC CODE START
#include "framework/controller/controller.h"

#include "gamereporting/gamereportingslaveimpl.h"
#include "stats/updatestatsrequestbuilder.h"
#include "stats/updatestatshelper.h"

// clubs includes
#include "clubs/clubsslaveimpl.h"
#include "customcode/component/clubs/clubsutil.h"

// seasonal play includes
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"
#include "osdkseasonalplay/rpc/osdkseasonalplayslave.h"

#include "customcode/component/clubs/tdf/osdk_clubs_server.h"
#include "fifacups/fifacupsslaveimpl.h"
#include "framework/util/profanityfilter.h"
// FIFA SPECIFIC CODE END
namespace Blaze
{
namespace Clubs
{

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief onClubCreated

    Custom processing upon club is created.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::onClubCreated(ClubsDatabase * clubsDb, 
        const char8_t* clubName, 
        const ClubSettings& clubSettings, 
        const ClubId clubId, 
        const ClubMember& owner)
{
    // FIFA SPECIFIC CODE START
    BlazeRpcError result = Blaze::ERR_OK;
    
    // Check club metadata for profanity
    result = checkClubMetadata(clubSettings);
    if (result != Blaze::ERR_OK)
    {
        return result;
    }

    Stats::StatsSlaveImpl* statsComponent = static_cast<Stats::StatsSlaveImpl*>(gController->getComponent(Stats::StatsSlaveImpl::COMPONENT_ID, false));
    if (nullptr != statsComponent)
    {
        /***************************************************************
             Check timestamps of leaving club
         ****************************************************************/
        const char8_t *userClubJumpingStatsCategory = "ClubJumping";
        const char8_t *userClubJumpingStatsGroup = "ClubJumpingStats";
        const char8_t *userStatsName = "leaveTime";
        int64_t nLastLeavingTime = 0;

        // Get tunable timeout value from configuration file
        bool bResult = true;
        uint32_t uClubJumpingBlockTime = 0;
        
        const OSDKClubsConfig &config = static_cast<const OSDKClubsConfig&>(*getConfig().getCustomSettings());
        uClubJumpingBlockTime = config.getClubJumpingBlockTime();

        if (true == bResult && 0 < uClubJumpingBlockTime)
        {

            Blaze::BlazeRpcError error = Blaze::ERR_OK;

            // Get group data
            Stats::GetStatGroupRequest statGroupRequest;
            Stats::StatGroupResponse statGroupResponse;
            statGroupRequest.setName(userClubJumpingStatsGroup);
            error = statsComponent->getStatGroup(statGroupRequest, statGroupResponse);

            if (Blaze::ERR_OK == error)
            {
                // Get category data
                Stats::GetStatsRequest getStatsRequest;
                getStatsRequest.setPeriodType(Stats::STAT_PERIOD_ALL_TIME);
                getStatsRequest.getEntityIds().push_back(owner.getUser().getBlazeId());
                getStatsRequest.setCategory(userClubJumpingStatsCategory);

                Stats::GetStatsResponse getStatsResponse;
                error = statsComponent->getStats(getStatsRequest, getStatsResponse);
                
                if (Blaze::ERR_OK == error)
                {
                    // Get Key scope stats data
                    Stats::KeyScopeStatsValueMap::const_iterator it = getStatsResponse.getKeyScopeStatsValueMap().begin();
                    Stats::KeyScopeStatsValueMap::const_iterator itEnd = getStatsResponse.getKeyScopeStatsValueMap().end();
                    
                    for (; it != itEnd; it++)
                    {
                        const Stats::StatValues* statValues = it->second;
                        if (statValues != nullptr)
                        {
                            const Stats::StatValues::EntityStatsList &entityList = statValues->getEntityStatsList();

                            if (entityList.size() > 0)
                            {
                                Stats::StatValues::EntityStatsList::const_iterator entityItr = entityList.begin();
                                Stats::StatValues::EntityStatsList::const_iterator entityItrEnd = entityList.end();
                                
                                const Stats::StatGroupResponse::StatDescSummaryList &statDescList = statGroupResponse.getStatDescs();
                                Stats::StatGroupResponse::StatDescSummaryList::const_iterator statDescItr = statDescList.begin();
                                Stats::StatGroupResponse::StatDescSummaryList::const_iterator statDescItrEnd = statDescList.end();                                
                                
                                // Go through both stats data and group data lists
                                for (; (entityItr != entityItrEnd); ++entityItr)
                                {
                                    if ((*entityItr) != nullptr && (*statDescItr) != nullptr)
                                    {
                                        const Stats::EntityStats::StringStatValueList &statList = (*entityItr)->getStatValues();

                                        Stats::EntityStats::StringStatValueList::const_iterator statValuesItr = statList.begin();
                                        Stats::EntityStats::StringStatValueList::const_iterator statValuesItrEnd = statList.end();

                                        for(; (statValuesItr != statValuesItrEnd) && (statDescItr != statDescItrEnd); ++statValuesItr, ++statDescItr)
                                        {
                                            if (nullptr != (*statValuesItr) &&
                                                (*statDescItr) != nullptr && // EA::TDF::tdf_ptr doesn't support prefixed NULL check
                                                0    == blaze_strcmp((*statDescItr)->getName(), userStatsName))
                                            {
                                                char8_t statsValue[32];
                                                blaze_strnzcpy(statsValue,(*statValuesItr), sizeof(statsValue));
                                                TRACE_LOG("[onClubCreated] stat Name=" << (*statDescItr)->getName() << ", Value=" << statsValue);
                                                nLastLeavingTime = atoi(statsValue);
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                 WARN_LOG("[onClubCreated] error - there's no club jumping data in stats DB");
                            }

                            break;
                        }
                    }

                    // Check if club jumping block already ended
                    if (0 < nLastLeavingTime)
                    {
                        // Get current time
                        int64_t nCurrentTime = TimeValue::getTimeOfDay().getSec();

                        if (nCurrentTime > nLastLeavingTime)
                        {
                            if ((nCurrentTime-nLastLeavingTime) < uClubJumpingBlockTime)
                            {
                                result = Blaze::CLUBS_ERR_JUMP_TOO_FREQUENTLY;
                                return result; // early return
                            }
                        }
                    }
                }
                else
                {
                     WARN_LOG("[onClubCreated] error - load ClubJumping category failed (" << error << ")");
                }
            }
            else
            {
                 WARN_LOG("[onClubCreated] error - load ClubJumpingStats group failed(" << error << ")");
            }
        }
        else if (!bResult)
        {
            WARN_LOG("[onClubCreated] error - cannot find tunable timeout value of club jumping in configuration.");
        }    

        // Update the clubsjoined stats in club user career stats
        Stats::UpdateStatsRequestBuilder updateStatsUtil;
        const char8_t *userCareerStatsCategory = "ClubUserCareerStats";
        const char8_t *userCareerStatsName = "clubsjoined";
        updateStatsUtil.startStatRow(userCareerStatsCategory, owner.getUser().getBlazeId(), 0);
        updateStatsUtil.incrementStat(userCareerStatsName, 1);
        updateStatsUtil.completeStatRow();

        // Update the free agent column
        Blaze::Clubs::ClubsUtil utilHelper;
        utilHelper.SetClubMemberAsFreeAgent(&updateStatsUtil, owner.getUser().getBlazeId(), false);

        // Update stats
        Stats::UpdateStatsHelper updateStatsHelper;
        BlazeRpcError error = ERR_OK;
        error = updateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)updateStatsUtil);
        if (ERR_OK != error)
        {
            ERR_LOG("[onClubCreated] error: initializeStatUpdate expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
        }
        else
        {
            error = updateStatsHelper.commitStats();
            if (ERR_OK != error)
            {
                ERR_LOG("[onClubCreated] error: commitStats expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            }
        }

    }
    else
    {
        WARN_LOG("[ClubsSlaveImpl].onClubCreated() - stats component is not available");
        result = Blaze::ERR_SYSTEM;
    }

    // register the club in seasonal play
    OSDKSeasonalPlay::OSDKSeasonalPlaySlave *seasonalPlayComponent =
        static_cast<OSDKSeasonalPlay::OSDKSeasonalPlaySlave*>(gController->getComponent(OSDKSeasonalPlay::OSDKSeasonalPlaySlave::COMPONENT_ID, false));
    if (seasonalPlayComponent != nullptr)
    {
        // the club's "league id" is stored in "season level"
        OSDKSeasonalPlay::LeagueId leagueId = static_cast<OSDKSeasonalPlay::LeagueId>(clubSettings.getSeasonLevel());

        OSDKSeasonalPlay::RegisterClubRequest req;
        req.setClubId(static_cast<OSDKSeasonalPlay::MemberId>(clubId));
        req.setLeagueId(leagueId);
        result = seasonalPlayComponent->registerClub(req);
        
        if (result != Blaze::ERR_OK)
        {
            ERR_LOG("[onClubCreated] error registering club " << clubId << " in seasonalplay");
            // TODO - do errors need to be reinterpreted?
            return result;
        }
    }
    else
    {
        TRACE_LOG("[ClubsSlaveImpl].onClubCreated() - osdkseasonalplay component is not available");
        result = Blaze::ERR_OK;
    }

    Blaze::FifaCups::FifaCupsSlave *fifaCupsComponent =
        static_cast<FifaCups::FifaCupsSlave*>(gController->getComponent(FifaCups::FifaCupsSlave::COMPONENT_ID,false));

    if (fifaCupsComponent != nullptr)
    {
        Blaze::FifaCups::MemberId memberId = static_cast<Blaze::FifaCups::MemberId>(clubId);
        Blaze::FifaCups::LeagueId leagueId = static_cast<Blaze::FifaCups::LeagueId>(clubSettings.getSeasonLevel());

        Blaze::FifaCups::RegisterClubRequest req;
        req.setMemberId(memberId);
        req.setLeagueId(leagueId);
        result = fifaCupsComponent->registerClub(req);

        if (result != Blaze::ERR_OK)
        {
            ERR_LOG("[onClubCreated] error registering club " << clubId << " in fifacups");
            return result;
        }
    }
    else
    {
        ERR_LOG("[ClubsSlaveImpl].onClubCreated() - fifacups component is not available");
        result = Blaze::ERR_SYSTEM;
        return result;
    }
    // FIFA SPECIFIC CODE END

    // log create event
    BlazeRpcError resultLog = logEvent(clubsDb, clubId, getEventString(LOG_EVENT_CREATE_CLUB),
        NEWS_PARAM_STRING, clubName);
    if (resultLog != Blaze::ERR_OK)
    {
        WARN_LOG("[onClubCreated] unable to log event for club" << clubId);
    }

    // FIFA SPECIFIC CODE START
    return result;
    // FIFA SPECIFIC CODE END
}
} // Clubs
} // Blaze
