/*************************************************************************************************/
/*!
    \file   arsonclubgamereportprocessor.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_ARSONCLUBGAMEREPORTPROCESSOR_H
#define BLAZE_CUSTOM_ARSONCLUBGAMEREPORTPROCESSOR_H

/*** Include files *******************************************************************************/
#include "gamereporting/gamereportprocessor.h"
#ifdef TARGET_arson
#include "arson/tdf/arsonclub.h"
#include "arson/tdf/arsongamehistoryreport.h"
#include "arson/tdf/arsongamehistoryreportbasic.h"
#endif

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Stats
{
    class UpdateStatsRequestBuilder;
}

namespace GameReporting
{

/*!
    class ArsonClubGameReportProcessor

*/
class ArsonClubGameReportProcessor: public GameReportProcessor
{
public:
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    ArsonClubGameReportProcessor(GameReportingSlaveImpl& component);
    ~ArsonClubGameReportProcessor() override;

    /*! ****************************************************************************/
    /*! \brief Implementation performs simple validation and if necessary modifies 
            the game report.

        On success report may be submitted to master for collation or direct to 
        processing for offline or trusted reports.   Behavior depends on the calling RPC.

        \param report Incoming game report from submit request
        \return ERR_OK on success.  GameReporting specific error on failure.
    ********************************************************************************/
    BlazeRpcError validate(GameReport& report) const override;

    /*! ****************************************************************************/
    /*! \brief Called when stats are reported following the process() call.

        \param processedReport Contains the final game report and information used by game history.
        \param playerIds list of players to distribute results to.
        \return ERR_OK on success.  GameReporting specific error on failure.
    ********************************************************************************/
    BlazeRpcError process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds) override;

private:
#ifdef TARGET_arson
    int32_t getClubScore(const ArsonClub::ClubReport& clubReport);

    bool processArsonClub(const GameType& gameType, ProcessedGameReport& processedReport, ArsonClub::Report& report, Stats::UpdateStatsRequestBuilder& builder, GameManager::PlayerIdList& playerIds);
    bool processArsonGameHistoryClub(const GameType& gameType, ProcessedGameReport& processedReport, GameHistoryClubs_NonDerived::Report& report, Stats::UpdateStatsRequestBuilder& builder, GameManager::PlayerIdList& playerIds);
#endif
};

}   // namespace GameReporting
}   // namespace Blaze

#endif // BLAZE_CUSTOM_ARSONCLUBGAMEREPORTPROCESSOR_H
