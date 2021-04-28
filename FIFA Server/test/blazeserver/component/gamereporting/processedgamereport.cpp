/*************************************************************************************************/
/*!
    \file   processedgamereport.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "processedgamereport.h"
#include "gametype.h"

namespace Blaze
{

namespace GameReporting
{

    ProcessedGameReport::ProcessedGameReport(ReportType reportType, const GameType& gameType, CollatedGameReport& report, GameReportingMetricsCollection& metricsCollection)
    :mGameType(gameType),
    mReportType(reportType),
    mGameInfo(&report.getGameInfo()),
    mGameReport(report.getGameReport()),
    mPrivateReportsMap(report.getPrivateReports()),
    mDnfStatusMap(report.getDnfStatus()),
    mSubmitterIds(report.getSubmitterPlayerIds()),
    mMetricsCollection(metricsCollection),
    mSaveHistory(!gameType.getHistoryTableDefinitions().empty() && report.getReportType() != REPORT_TYPE_TRUSTED_MID_GAME),
    mFinalResult(report.getReportType() != REPORT_TYPE_TRUSTED_MID_GAME),
    mGameId(report.getGameId())
{
    //create the customdata if that is defined in the config
    if (mGameType.getCustomDataTdf()!=nullptr && mGameType.getCustomDataTdf()[0]!='\0')
    {
        mCustomData = GameType::createReportTdf(mGameType.getCustomDataTdf());
        if (mCustomData == nullptr)
        {
            ERR_LOG("[ProcessedGameReport].ctor: failed to create custom data tdf for gametype(" << mGameType.getGameReportName().c_str() << ")");
        }
    }

    mReportFlag.getFlag().clearAbuse();

    updatePINReports();
}

ProcessedGameReport::ProcessedGameReport(ReportType reportType, const GameType& gameType, GameReport& gameReport, CollatedGameReport::PrivateReportsMap& privateReportsMap, const GameInfo* gameInfo,
                                         const CollatedGameReport::DnfStatusMap& dnfStatusMap, const GameManager::PlayerIdList& submitterIds, GameReportingMetricsCollection& metricsCollection, GameManager::GameId gameId)
    :mGameType(gameType),
    mReportType(reportType),
    mGameInfo(gameInfo),
    mGameReport(gameReport),
    mPrivateReportsMap(privateReportsMap),
    mDnfStatusMap(dnfStatusMap),
    mSubmitterIds(submitterIds),
    mMetricsCollection(metricsCollection),
    mSaveHistory(!gameType.getHistoryTableDefinitions().empty()),
    mFinalResult(true),
    mCustomData(nullptr),
    mGameId(gameId)
{
    //create the customdata if that is defined in the config
    if (mGameType.getCustomDataTdf()!=nullptr && mGameType.getCustomDataTdf()[0]!='\0')
    {
        mCustomData = GameType::createReportTdf(mGameType.getCustomDataTdf());
        if (mCustomData == nullptr)
        {
            ERR_LOG("[ProcessedGameReport].ctor: failed to create custom data tdf for gametype(" << mGameType.getGameReportName().c_str() << ")");
        }
    }   

    mReportFlag.getFlag().clearAbuse();

    updatePINReports();
}

ProcessedGameReport::~ProcessedGameReport()
{
}

bool ProcessedGameReport::didAllPlayersFinish() const
{
    CollatedGameReport::DnfStatusMap::const_iterator iter = mDnfStatusMap.begin();
    CollatedGameReport::DnfStatusMap::const_iterator end = mDnfStatusMap.end();
    for (; iter != end; ++iter)
    {
        //  player did not finish? 
        if (iter->second != 0)
            return false;
    }

    return true;
}

void ProcessedGameReport::updatePINReports()
{
    for (auto playerId : mSubmitterIds)
    {
        mGameReport.copyInto(mPINGameReportMap[playerId]);
    }
}

}   // namespace GameReporting
}   // namespace Blaze
