/*************************************************************************************************/
/*!
    \file   arsonbasicgamereportprocessor.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_ARSONBASICGAMEREPORTPROCESSOR_H
#define BLAZE_CUSTOM_ARSONBASICGAMEREPORTPROCESSOR_H

/*** Include files *******************************************************************************/
#include "gamereporting/gamereportprocessor.h"
#ifdef TARGET_arson
#include "arson/tdf/arsongamehistoryreport.h"
#include "arson/tdf/arsongamehistoryreportbasic.h"
#include "arson/tdf/arsonctfcustom.h"
#endif

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class ArsonBasicGameReportProcessor

*/
class ArsonBasicGameReportProcessor: public GameReportProcessor
{
public:
    ArsonBasicGameReportProcessor(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    ~ArsonBasicGameReportProcessor() override;

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
    int32_t getScore(const GameHistoryBasic::PlayerReport& playerReport, bool didPlayerFinish);
    int32_t getScore(const ArsonCTF_Custom::PlayerReport& playerReport, bool didPlayerFinish);
    int32_t getWLT(const GameHistoryBasic::PlayerReport& playerReport);
    uint32_t calcPlayerDampingPercent(GameManager::PlayerId playerId, bool isWinner, bool didAllPlayersFinish,
    const GameHistoryBasic::Report::PlayerReportsMap &playerReportMap);
#endif
};

}   // namespace GameReporting
}   // namespace Blaze

#endif // BLAZE_CUSTOM_ARSONBASICGAMEREPORTPROCESSOR_H
