/*************************************************************************************************/
/*!
    \file   psnmatchutil.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/outboundhttpservice.h" // for gOutboundHttpService
#include "psnmatches/rpc/psnmatchesslave.h"
#include "gamemanager/externalsessions/externalsessionutilps5.h" // for getNewWebApiConnMgr
#include "gamemanager/externalsessions/externalsessionutilps5matches.h" // for callPSN
#include "gamereportingslaveimpl.h"
#include "psnmatchutil.h"

namespace Blaze
{
namespace GameReporting
{
namespace Utilities
{

/*************************************************************************************************/
/*!
    \brief constructor
*/
/*************************************************************************************************/
PsnMatchUtil::PsnMatchUtil(GameReportingSlaveImpl& component, ReportType reportType, const char8_t* reportName, const GameInfo& gameInfo)
    : mComponent(component)
    , mReportType(reportType)
    , mReportName(reportName)
    , mGameInfo(gameInfo)
    , mCooperativeResult(PSNServices::Matches::PsnCooperativeResultEnum::INVALID_COOPRESULT_TYPE)
    , mRefCount(0)
{
    GameManager::ExternalSessionUtilPs5::getNewWebApiConnMgr(mPsnConnMgrPtr, PSNServices::Matches::PSNMatchesSlave::COMPONENT_INFO.name);

    mPlayerList.reserve(mGameInfo.getPlayerInfoMap().size());
}

/*************************************************************************************************/
/*!
    \brief destructor

    Should not be called until after all the RPCs maintained by this util have been sent
    and their responses handled.
*/
/*************************************************************************************************/
PsnMatchUtil::~PsnMatchUtil()
{
    //mComponent.getMetrics().mActivePsnMatchUpdates.decrement();

    if (mPsnConnMgrPtr != nullptr)
    {
        mPsnConnMgrPtr.reset();
    }

    for (auto& team : mTeamList)
    {
        delete team;
    }
    for (auto& player : mPlayerList)
    {
        delete player;
    }
}

BlazeRpcError PsnMatchUtil::addPlayerResult(BlazeId playerId, float score, int64_t coopValue, const MatchStatMap& matchStatMap)
{
    PSNServices::Matches::GetMatchDetailResponsePtr matchDetail = getMatchDetail();
    if (matchDetail == nullptr)
    {
        ERR_LOG(logPrefix() << ".addPlayerResult: no match detail");
        return ERR_SYSTEM;
    }

    auto competitionType = PSNServices::Matches::PsnCompetitionTypeEnum::INVALID_COMPETITION_TYPE;
    PSNServices::Matches::ParsePsnCompetitionTypeEnum(matchDetail->getCompetitionType(), competitionType);
    if (competitionType == PSNServices::Matches::PsnCompetitionTypeEnum::COOPERATIVE)
    {
        // all players should report the SAME cooperative result
        PSNServices::Matches::PsnCooperativeResultEnum newCoopResult = (PSNServices::Matches::PsnCooperativeResultEnum) coopValue;

        if (mCooperativeResult == PSNServices::Matches::PsnCooperativeResultEnum::INVALID_COOPRESULT_TYPE)
        {
            // not set yet -- this *should* be the first player
            if (newCoopResult != mCooperativeResult)
            {
                PSNServices::Matches::MatchResults& matchResults = mReportRequest.getBody().getMatchResults();
                matchResults.setCooperativeResult(PSNServices::Matches::PsnCooperativeResultEnumToString((PSNServices::Matches::PsnCooperativeResultEnum) coopValue));
                mCooperativeResult = newCoopResult;
            }
            else
            {
                WARN_LOG(logPrefix() << ".addPlayerResult: playerId(" << playerId << ") does not have valid coopValue(" << coopValue << ")");
            }
        }
        else
        {
            if (newCoopResult != mCooperativeResult)
            {
                WARN_LOG(logPrefix() << ".addPlayerResult: ignoring playerId(" << playerId << ")'s coopValue(" << coopValue << ") -- should be (" << mCooperativeResult << ")");
            }
        }
    }

    // regardless, store all the player results in this util's cache

    if (mPlayerMap.find(playerId) != mPlayerMap.end())
    {
        WARN_LOG(logPrefix() << ".addPlayerResult: ignoring duplicate player result for playerId(" << playerId << "), score(" << score << "), coopValue(" << coopValue << ")");
        return ERR_OK;
    }

    // find the corresponding external player ID
    auto pInfItr = mGameInfo.getPlayerInfoMap().find(playerId);
    if ((EA_UNLIKELY(pInfItr == mGameInfo.getPlayerInfoMap().end())) || (pInfItr->second->getUserIdentification().getPlatformInfo().getClientPlatform() != ps5))
    {
        TRACE_LOG(logPrefix() << ".addPlayerResult: BlazeId(" << playerId << ") is non-PSN user");
        return ERR_OK;
    }
    ExternalPsnAccountId externalPlayerId = pInfItr->second->getUserIdentification().getPlatformInfo().getExternalIds().getPsnAccountId();

    eastl::string teamId;

    // if TEAM_MATCH, find the teamId from game.   (Can't just get it from match details
    // as side-changing players can show as 'inactive'/joinFlag false in multiple match teams, with no way to tell which was latest)
    if (competitionType == PSNServices::Matches::PsnCompetitionTypeEnum::COMPETITIVE)
    {
        auto groupingType = PSNServices::Matches::PsnGroupingTypeEnum::INVALID_GROUPING_TYPE;
        PSNServices::Matches::ParsePsnGroupingTypeEnum(matchDetail->getGroupingType(), groupingType);
        if (groupingType == PSNServices::Matches::PsnGroupingTypeEnum::TEAM_MATCH)
        {
            GameManager::ExternalSessionUtilPs5Matches::toPsnReqTeamId(teamId, pInfItr->second->getTeamIndex());
            
            // Just in case, validate player was at some pt in the team in the match below
            bool wasInExpectedMatchTeam = false;
            eastl::string playerIdStr;
            GameManager::ExternalSessionUtilPs5Matches::toPsnReqPlayerId(playerIdStr, pInfItr->second->getUserIdentification());
            for (auto& matchTeam : matchDetail->getInGameRoster().getTeams())
            {
                if (blaze_strcmp(matchTeam->getTeamId(), teamId.c_str()) != 0)
                    continue;
                for (auto& matchTeamMember : matchTeam->getMembers())
                {
                    if (blaze_strcmp(matchTeamMember->getPlayerId(), playerIdStr.c_str()) == 0)
                    {
                        wasInExpectedMatchTeam = true;
                        break;
                    }
                }
                if (wasInExpectedMatchTeam)
                    break;
            }

            if (teamId.empty() || !wasInExpectedMatchTeam)
            {
                WARN_LOG(logPrefix() << ".addPlayerResult: unable to find playerId(" << playerId << ") within the Match TeamId(" << teamId << "), externalPlayerId(" << externalPlayerId << "), but continuing to process");
            }
            else
            {
                // make sure there's a team map entry cache for now
                mTeamMap[teamId] = nullptr;
            }
        }
    }

    PlayerResult*& playerResult = mPlayerList.push_back();

    playerResult = BLAZE_NEW PlayerResult(playerId, externalPlayerId, score, teamId.c_str());

    for (const auto& stat : matchStatMap)
    {
        playerResult->statMap[stat.first] = stat.second;
    }

    mPlayerMap[playerId] = playerResult;

    return ERR_OK;
}

PSNServices::Matches::GetMatchDetailResponsePtr PsnMatchUtil::getMatchDetail()
{
    if (mMatchDetail == nullptr)
    {
        const char8_t* matchId = mGameInfo.getExternalSessionIdentification().getPs5().getMatch().getMatchId();
        if (matchId[0] == '\0')
        {
            ERR_LOG(logPrefix() << ".getMatchDetail: no match id available");
            return PSNServices::Matches::GetMatchDetailResponsePtr();
        }

        // For usability, and in case Activity type here is determined at runtime. Fetch the Match Activity info to confirm type of result to send
        PSNServices::Matches::GetMatchDetailResponsePtr rsp(BLAZE_NEW PSNServices::Matches::GetMatchDetailResponse);
        PSNServices::Matches::GetMatchDetailRequest req;
        req.setNpServiceLabel(getServiceLabel());
        req.setMatchId(matchId);

        const CommandInfo& cmd = PSNServices::Matches::PSNMatchesSlave::CMD_INFO_GETMATCHDETAIL;

        BlazeRpcError err = callPSN(nullptr, cmd, req.getHeader(), req, rsp, nullptr);
        if (err != ERR_OK)
        {
            ERR_LOG(logPrefix() << ".getMatchDetail: failed PSN request for match(" << matchId << "), err(" << ErrorHelp::getErrorName(err) << ").");
            return PSNServices::Matches::GetMatchDetailResponsePtr();
        }
        mMatchDetail = rsp;
    }

    return mMatchDetail;
}

BlazeRpcError PsnMatchUtil::initializeReport()
{
    PSNServices::Matches::GetMatchDetailResponsePtr matchDetail = getMatchDetail();
    if (matchDetail == nullptr)
    {
        ERR_LOG(logPrefix() << ".initializeReport: no match detail");
        return ERR_SYSTEM;
    }

    if (mPlayerList.empty())
    {
        TRACE_LOG(logPrefix() << ".initializeReport: no players");
        return ERR_OK;
    }

    auto competitionType = PSNServices::Matches::PsnCompetitionTypeEnum::INVALID_COMPETITION_TYPE;
    PSNServices::Matches::ParsePsnCompetitionTypeEnum(matchDetail->getCompetitionType(), competitionType);

    auto groupingType = PSNServices::Matches::PsnGroupingTypeEnum::INVALID_GROUPING_TYPE;
    PSNServices::Matches::ParsePsnGroupingTypeEnum(matchDetail->getGroupingType(), groupingType);

    auto resultType = PSNServices::Matches::PsnResultTypeEnum::INVALID_RESULT_TYPE;
    PSNServices::Matches::ParsePsnResultTypeEnum(matchDetail->getResultType(), resultType);

    // add any competitive result (any cooperative result would have been set/added during parsing via addPlayerResult)

    if (competitionType == PSNServices::Matches::PsnCompetitionTypeEnum::COMPETITIVE)
    {
        PSNServices::Matches::MatchResults& matchResults = mReportRequest.getBody().getMatchResults();

        if (groupingType == PSNServices::Matches::PsnGroupingTypeEnum::TEAM_MATCH)
        {
            // build out the team results cache
            if (mTeamMap.size() == 0)
            {
                ERR_LOG(logPrefix() << ".initializeReport: no teams for TEAM_MATCH");
                return ERR_SYSTEM;
            }
            mTeamList.reserve(mTeamMap.size());

            for (auto& teamMapItr : mTeamMap)
            {
                EA_ASSERT(teamMapItr.second == nullptr);
                teamMapItr.second = BLAZE_NEW TeamResult(teamMapItr.first);
                mTeamList.push_back(teamMapItr.second);
            }

            for (auto& player : mPlayerList)
            {
                // each player should belong to a team
                auto teamMapItr = mTeamMap.find(player->teamId);
                if (teamMapItr == mTeamMap.end())
                {
                    ERR_LOG(logPrefix() << ".initializeReport: BlazeId(" << player->playerId << ") does not belong to a team(" << player->teamId << ") for TEAM_MATCH");
                    return ERR_SYSTEM;
                }

                TeamResult& teamResult = *teamMapItr->second;

                // aggregation is only SUM
                /// @todo [ps5-gr] other aggregation methods ???
                teamResult.score += player->score;

                teamResult.memberList.push_back(player);

                // additional MatchStatistics -- aggregate teamStatistics
                for (const auto& stat : player->statMap)
                {
                    teamResult.statMap[stat.first] += stat.second;
                }
            }

            // sort the teams
            eastl::sort(mTeamList.begin(), mTeamList.end(), TeamScoreDescendingCompare());

            // assign rank
            int32_t curRank = 1;
            int32_t prevAssignedRank = curRank;
            float prevScore = (*mTeamList.begin())->score;
            for (auto& team : mTeamList)
            {
                /// @todo [ps5-gr] affected by float precision?
                if (team->score != prevScore)
                {
                    team->rank = curRank;
                    prevAssignedRank = curRank;
                    prevScore = team->score;
                }
                else
                {
                    // handle ties
                    team->rank = prevAssignedRank;
                }
                ++curRank;
            }

            // add match results
            for (auto& team : mTeamList)
            {
                auto teamResult = matchResults.getCompetitiveResult().getTeamResults().pull_back();
                teamResult->setTeamId(team->teamId.c_str());
                teamResult->setRank(team->rank);

                if (resultType == PSNServices::Matches::PsnResultTypeEnum::SCORE)
                {
                    teamResult->setScore(team->score);
                }

                for (auto& teamMember : team->memberList)
                {
                    auto teamMemberResult = teamResult->getTeamMemberResults().pull_back();
                    teamMemberResult->setPlayerId(eastl::string(eastl::string::CtorSprintf(), "%" PRIu64, teamMember->externalPlayerId).c_str());
                    teamMemberResult->setScore(teamMember->score);
                }
            }
        }
        else
        {
            // sort the players
            eastl::sort(mPlayerList.begin(), mPlayerList.end(), PlayerScoreDescendingCompare());

            // assign rank
            int32_t curRank = 1;
            int32_t prevAssignedRank = curRank;
            float prevScore = (*mPlayerList.begin())->score;
            for (auto& player : mPlayerList)
            {
                /// @todo [ps5-gr] affected by float precision?
                if (player->score != prevScore)
                {
                    player->rank = curRank;
                    prevAssignedRank = curRank;
                    prevScore = player->score;
                }
                else
                {
                    // handle ties
                    player->rank = prevAssignedRank;
                }
                ++curRank;
            }

            // add match results
            for (auto& player : mPlayerList)
            {
                auto playerResult = matchResults.getCompetitiveResult().getPlayerResults().pull_back();
                playerResult->setPlayerId(eastl::string(eastl::string::CtorSprintf(), "%" PRIu64, player->externalPlayerId).c_str());
                playerResult->setRank(player->rank);

                if (resultType == PSNServices::Matches::PsnResultTypeEnum::SCORE)
                {
                    playerResult->setScore(player->score);
                }
            }
        }
    }

    // add additional statistics

    if (competitionType == PSNServices::Matches::PsnCompetitionTypeEnum::COMPETITIVE && groupingType == PSNServices::Matches::PsnGroupingTypeEnum::TEAM_MATCH)
    {
        // add team statistics
        // any team cache used here should have been built when adding match results just above
        PSNServices::Matches::MatchStatistics::MatchTeamStatsList& matchTeamStats = mReportRequest.getBody().getMatchStatistics().getTeamStatistics();
        for (const auto& team : mTeamList)
        {
            if (team->statMap.empty())
                continue;

            auto matchTeamStat = matchTeamStats.pull_back();
            matchTeamStat->setTeamId(team->teamId.c_str());

            for (const auto& teamStat : team->statMap)
            {
                auto matchStat = matchTeamStat->getStats().pull_back();
                matchStat->setStatsKey(teamStat.first.c_str());
                matchStat->setStatsValue(eastl::string(eastl::string::CtorSprintf(), "%f", teamStat.second).c_str());
            }

            for (const auto& teamMember : team->memberList)
            {
                if (teamMember->statMap.empty())
                    continue;

                auto matchTeamMemberStats = matchTeamStat->getTeamMemberStatistics().pull_back();
                matchTeamMemberStats->setPlayerId(eastl::string(eastl::string::CtorSprintf(), "%" PRIu64, teamMember->externalPlayerId).c_str());

                for (const auto& teamMemberStat : teamMember->statMap)
                {
                    auto matchStat = matchTeamMemberStats->getStats().pull_back();
                    matchStat->setStatsKey(teamMemberStat.first.c_str());
                    matchStat->setStatsValue(eastl::string(eastl::string::CtorSprintf(), "%f", teamMemberStat.second).c_str());
                }
            }
        }
    }
    else
    {
        // add player statistics
        PSNServices::Matches::MatchStatistics::MatchPlayerStatsList& matchPlayerStats = mReportRequest.getBody().getMatchStatistics().getPlayerStatistics();
        for (const auto& player : mPlayerList)
        {
            if (player->statMap.empty())
                continue;

            auto matchPlayerStat = matchPlayerStats.pull_back();
            matchPlayerStat->setPlayerId(eastl::string(eastl::string::CtorSprintf(), "%" PRIu64, player->externalPlayerId).c_str());

            for (const auto& playerStat : player->statMap)
            {
                auto matchStat = matchPlayerStat->getStats().pull_back();
                matchStat->setStatsKey(playerStat.first.c_str());
                matchStat->setStatsValue(eastl::string(eastl::string::CtorSprintf(), "%f", playerStat.second).c_str());
            }
        }
    }

    return ERR_OK;
}

BlazeRpcError PsnMatchUtil::submitReport()
{
    const char8_t* matchId = mGameInfo.getExternalSessionIdentification().getPs5().getMatch().getMatchId();

    mReportRequest.setMatchId(matchId);
    mReportRequest.getBody().setNpServiceLabel(getServiceLabel());
    mReportRequest.getBody().getMatchResults().setVersion("1"); // required by Sony spec

    const CommandInfo& cmd = PSNServices::Matches::PSNMatchesSlave::CMD_INFO_REPORTMATCHRESULTS;

    BlazeRpcError err = callPSN(nullptr, cmd, mReportRequest.getHeader(), mReportRequest, nullptr, nullptr);
    if (err != ERR_OK)
    {
        ERR_LOG(logPrefix() << ".submitReport: failed PSN request for match(" << matchId << "), err(" << ErrorHelp::getErrorName(err) << ").");
    }
    return err;
}

BlazeRpcError PsnMatchUtil::cancelMatch()
{
    const char8_t* matchId = mGameInfo.getExternalSessionIdentification().getPs5().getMatch().getMatchId();

    PSNServices::Matches::UpdateMatchStatusRequest req;
    req.setMatchId(matchId);
    req.getBody().setNpServiceLabel(getServiceLabel());
    req.getBody().setStatus(PsnMatchStatusEnumToString(PSNServices::Matches::PsnMatchStatusEnum::CANCELLED));

    const CommandInfo& cmd = PSNServices::Matches::PSNMatchesSlave::CMD_INFO_UPDATEMATCHSTATUS;

    BlazeRpcError err = callPSN(nullptr, cmd, req.getHeader(), req, nullptr, nullptr);
    if (err != ERR_OK)
    {
        ERR_LOG(logPrefix() << ".cancelMatch: failed PSN request for match(" << matchId << "), err(" << ErrorHelp::getErrorName(err) << ").");
    }
    return err;
}

PsnServiceLabel PsnMatchUtil::getServiceLabel() const
{
    return mComponent.getConfig().getExternalSessions().getPsnMatchesServiceLabel();
}

BlazeRpcError PsnMatchUtil::callPSN(const UserIdentification* user, const CommandInfo& cmdInfo, PSNServices::PsnWebApiHeader& reqHeader, const EA::TDF::Tdf& req, EA::TDF::Tdf* rsp, ExternalSessionErrorInfo* errorInfo)
{
    return GameManager::ExternalSessionUtilPs5Matches::callPSN(user, cmdInfo, reqHeader, req, rsp, errorInfo, mPsnConnMgrPtr, mComponent.getConfig().getExternalSessions().getCallOptions());
}

bool PsnMatchUtil::PlayerScoreDescendingCompare::operator()(const PlayerResult* a, const PlayerResult* b)
{
    return a->score > b->score;
}

bool PsnMatchUtil::TeamScoreDescendingCompare::operator()(const TeamResult* a, const TeamResult* b)
{
    return a->score > b->score;
}

} //namespace Utilities
} //namespace GameReporting
} //namespace Blaze
