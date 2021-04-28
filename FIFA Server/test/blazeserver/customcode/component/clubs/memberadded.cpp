/*************************************************************************************************/
/*!
    \file   memberadded.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
// FIFA SPECIFIC CODE START
#include "framework/controller/controller.h"

#ifdef TARGET_gamereporting
#include "stats/updatestatsrequestbuilder.h"
#include "stats/updatestatshelper.h"
#endif

// clubs includes
#include "clubs/clubsslaveimpl.h"
#include "customcode/component/clubs/tdf/osdk_clubs_server.h"
// FIFA SPECIFIC CODE END
#include "rivals.h"

namespace Blaze
{
namespace Clubs
{

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief onMemberAdded

    Custom processing upon member added.

*/
/*************************************************************************************************/
BlazeRpcError ClubsSlaveImpl::onMemberAdded(ClubsDatabase& clubsDb, 
                                            const Club& club, const ClubMember& member)
{
    BlazeRpcError result = ERR_OK;
    // FIFA SPECIFIC CODE START
    Stats::StatsSlaveImpl* statsComponent = static_cast<Stats::StatsSlaveImpl*>(gController->getComponent(Stats::StatsSlaveImpl::COMPONENT_ID, false));

    /***************************************************************
         Check timestamps of leaving club
     ****************************************************************/
    if (nullptr != statsComponent)
    {
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
                getStatsRequest.getEntityIds().push_back(member.getUser().getBlazeId());
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
                                                TRACE_LOG("[onMemberAdded]  stat Name=" << (*statDescItr)->getName() << ", Value=" << statsValue);
                                                nLastLeavingTime = atoi(statsValue);
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            else
                            {
                                 WARN_LOG("[onMemberAdded] error - there's no club jumping data in stats DB");
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
                     WARN_LOG("[onMemberAdded] error - load ClubJumping category failed (" << error << ")");
                }
            }
            else
            {
                 WARN_LOG("[onMemberAdded] error - load ClubJumpingStats group failed(" << error << ")");
            }
        }
        else if (!bResult)
        {
            WARN_LOG("[onMemberAdded] error - cannot find tunable timeout value of club jumping in configuration.");
        }
    }

    /**************************************
         update users Club Stats
     **************************************/
    {
        if (statsComponent != nullptr)
        {

            // Update the clubsjoined stats in users club career stats
            const char8_t *userStatsCategory = "ClubUserCareerStats";
            const char8_t *userStatsName = "clubsjoined";

#ifdef TARGET_gamereporting
            Stats::UpdateStatsRequestBuilder updateStatsUtil;
            updateStatsUtil.startStatRow(userStatsCategory, member.getUser().getBlazeId(), 0);
            updateStatsUtil.incrementStat(userStatsName, 1);
            updateStatsUtil.completeStatRow();
            Blaze::Clubs::ClubsUtil clubUtilhelper;
            clubUtilhelper.SetClubMemberAsFreeAgent(&updateStatsUtil, member.getUser().getBlazeId(), false);

            // Update stats
            Stats::UpdateStatsHelper updateStatsHelper;
            BlazeRpcError error = ERR_OK;
            error = updateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)updateStatsUtil);
            if (ERR_OK != error)
            {
                ERR_LOG("[ClubsSlaveImpl].onMemberAdded() - initializeStatUpdate expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
            }
            else
            {
                error = updateStatsHelper.commitStats();
                if (ERR_OK != error)
                {
                    ERR_LOG("[ClubsSlaveImpl].onMemberAdded() - commitStats expecting error code: ERR_OK, but got: " << ErrorHelp::getErrorName(error));
                }
            }
#endif
        }
        else
        {
            ERR_LOG("[ClubsSlaveImpl].onMemberAdded() - stats component is not available");
            result = Blaze::ERR_SYSTEM;
        }
    }
    // end of update users Club Career Stats

    /*************************
        CLUB RIVAL MODE
    *************************/
    {
        if (club.getClubSettings().getAcceptanceFlags().getCLUBS_ACCEPT_RIVALS() &&
            club.getClubSettings().getCustClubSettings().getCustOpt5() 
                == static_cast<uint32_t>(CLUBS_RIVAL_NOT_ELIGIBLE))
        {
            // check club eligibility for rivals
            if (club.getClubInfo().getMemberCount() >= minMemberCountReqForRivals)
            {
                result = assignRival(&clubsDb, &club, false);
                if(result != Blaze::ERR_OK)
                {
                    WARN_LOG("[Clubs::onMemberAdded] - Failed to Assign Rival to club [" << club.getClubId() << "].");
                }
                result = setIsRankedOnRivalLeaderboard(club.getClubId(), true);
                if(result != Blaze::ERR_OK)
                {
                    WARN_LOG("[Clubs::onMemberOnline] error listing club [" << club.getClubId() << "] on rival leaderboard.");
                }
            }
        }
    }
    // end of CLUB RIVAL MODE

    /**************************************
        update Club News to log Join Event
     **************************************/
    {
        UserInfoPtr userInfo;
        result = gUserSessionManager->lookupUserInfoByBlazeId(member.getUser().getBlazeId(), userInfo);
        if (result != Blaze::ERR_OK)
        {
            WARN_LOG("[onMemberAdded] error (" << result << ") looking up user "
                     << member.getUser().getBlazeId() << " for club " << club.getClubId());
        }
        else
        {
            char blazeId[32];
            blaze_snzprintf(blazeId, sizeof(blazeId), "%" PRId64, userInfo->getId());
            result = logEvent(&clubsDb, club.getClubId(), getEventString(LOG_EVENT_JOIN_CLUB),
                              NEWS_PARAM_BLAZE_ID, blazeId);

            if (result != Blaze::ERR_OK)
            {
                WARN_LOG("[onMemberAdded] unable to log event for user "
                         << member.getUser().getBlazeId() << " in club " << club.getClubId());
            }
        }
    }
    // end of CLUB NEWS
    // FIFA SPECIFIC CODE END

    return result;
}

} // Clubs
} // Blaze
