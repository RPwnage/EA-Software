/*************************************************************************************************/
/*!
    \file   basicgamereportprocessor.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_BASICGAMEREPORTPROCESSOR_H
#define BLAZE_BASICGAMEREPORTPROCESSOR_H

/*** Include files *******************************************************************************/
#include "gamereportprocessor.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class BasicGameReportProcessor

    A simple GameReportProcessor implementation.  This is the default report processor for a stock
    blazeserver, and this also provides an example for teams to customize their own processing.
*/
class BasicGameReportProcessor: public GameReportProcessor
{
public:
    BasicGameReportProcessor(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    ~BasicGameReportProcessor() override;

    /*! ****************************************************************************/
    /*! \brief Implementation performs simple validation and if necessary modifies 
            the game report.
 
        On success report may be submitted for collation or direct to processing
        for offline or trusted reports.   Behavior depends on the calling RPC.

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
};

}   // namespace GameReporting
}   // namespace Blaze

#endif // BLAZE_BASICGAMEREPORTPROCESSOR_H
