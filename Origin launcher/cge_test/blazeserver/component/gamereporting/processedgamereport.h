/*************************************************************************************************/
/*!
    \file   processedgamereport.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_GAMEREPORTING_PROCESSEDGAMEREPORT_H
#define BLAZE_GAMEREPORTING_PROCESSEDGAMEREPORT_H

#include "gamereporting/tdf/gamereporting.h"
#include "gamereporting/tdf/gamehistory.h"
#include "gamereporting/tdf/gamereporting_server.h"
#include "metricsconfig.h"

namespace Blaze
{
namespace GameReporting
{

class GameType;
class UpdateStatsUtil;

typedef eastl::hash_map<GameManager::PlayerId, GameReport> GameReportPINMap;

/*!
    class ProcessedGameReport
*/
class ProcessedGameReport
{
    NON_COPYABLE(ProcessedGameReport);
public:
    ProcessedGameReport(ReportType reportType, const GameType& gameType, CollatedGameReport& report, GameReportingMetricsCollection& metricsCollection);
    ProcessedGameReport(ReportType reportType, const GameType& gameType, GameReport& gameReport, CollatedGameReport::PrivateReportsMap& privateReportsMap,
        const GameInfo* gameInfo, const CollatedGameReport::DnfStatusMap& dnfStatusMap, const GameManager::PlayerIdList& submitterIds, GameReportingMetricsCollection& metricsCollection, GameManager::GameId gameId);
    ~ProcessedGameReport();

    //  returns the game type associated with the report.
    const GameType& getGameType() const 
    {
        return mGameType;
    }

    //  returns the type of report being processed.
    ReportType getReportType() const 
    {
        return mReportType;
    }

    //  returns the finished game info
    const GameInfo* getGameInfo() const
    {
        return mGameInfo;
    }

    //  returns the collated game report
    const GameReport& getGameReport() const
    {
        return mGameReport;
    }

    //  returns the collated game report
    GameReport& getGameReport()
    {
        return mGameReport;
    }

    //  returns the PIN version collated game report
    GameReportPINMap& getPINGameReportMap()
    {
        return mPINGameReportMap;
    }

    //  returns the Dnf status map for the report - if this is an offline or trusted report, this will be empty.
    const CollatedGameReport::DnfStatusMap& getDnfStatusMap() const 
    {
        return mDnfStatusMap;
    }

    //  checks if all players finished
    bool didAllPlayersFinish() const;

    //  returns the private reports map.  if this an offline or trusted report, this will return a map with a single entry containing the report
    //  submitted in the SubmitGameReport request.
    CollatedGameReport::PrivateReportsMap& getPrivateReportsMap() 
    {
        return mPrivateReportsMap;
    }

    //  determines whether game history should save a history report.
    bool needsHistorySave() const 
    {
        return mSaveHistory;
    }

    //  set by GameReportProcessor::process(), determines whether game history should save a history report.
    void enableHistorySave(bool saveHistory) 
    {
        mSaveHistory = saveHistory;
    }

    //  if set, this report is the final game report for the game session (normally true - for mid-game trusted reports, then it'll be false by default.)
    bool isFinalResult() const {
        return mFinalResult;
    }

    //  an enabler is supplied to allow customization.
    void setFinalResult(bool isFinal) {
        mFinalResult = isFinal;
    }

    //  returns the attribute map used by game reporting to populate the history tables.
    //  callers should map player reports to the equivalent attribute map.  game attributes and other report classes to their respective
    //  maps as well.
    GameHistoryReport& getGameHistoryReport() 
    {
        return mHistoryReport;
    }

    // returns the title specific custom data, it can be any tdf type and can be customized as part of the gametype definition.
    EA::TDF::Tdf* getCustomData() 
    {
        return mCustomData;
    }

    // const version of the "EA::TDF::Tdf* getCustomData()"
    const EA::TDF::Tdf* getCustomData() const
    {
        return mCustomData;
    }

    //  returns the list of players who have submitted game reports.
    const GameManager::PlayerIdList& getSubmitterIds() const
    {
        return mSubmitterIds;
    }

    //  returns the custom metrics object used to modify custom configured metrics.
    GameReportingMetricsCollection& getMetricsCollection()
    {
        return mMetricsCollection;
    }

    //  returns the game report flags and the reason of the flags.
    const ReportFlagInfo* getReportFlagInfo() const
    {
        return &mReportFlag;
    }

    //  returns the game report flags and the reason of the flags.
    ReportFlagInfo* getReportFlagInfo()
    {
        return &mReportFlag;
    }

    GameManager::GameId getGameId() const
    {
        return mGameId;
    }

    void updatePINReports();

private:
    const GameType& mGameType;
    ReportType mReportType;
    const GameInfo* mGameInfo;
    GameReport& mGameReport;
    GameReportPINMap mPINGameReportMap;
    CollatedGameReport::PrivateReportsMap& mPrivateReportsMap;
    const CollatedGameReport::DnfStatusMap& mDnfStatusMap;
    const GameManager::PlayerIdList& mSubmitterIds;
    GameReportingMetricsCollection& mMetricsCollection;
    GameHistoryReport mHistoryReport;
    bool mSaveHistory;
    bool mFinalResult;

    EA::TDF::TdfPtr mCustomData;
    GameManager::GameId mGameId;

    ReportFlagInfo mReportFlag;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif
