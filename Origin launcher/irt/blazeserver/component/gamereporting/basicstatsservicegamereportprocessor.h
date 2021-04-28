/*************************************************************************************************/
/*!
    \file   basicstatsservicegamereportprocessor.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_BASICSTATSSERVICEGAMEREPORTPROCESSOR_H
#define BLAZE_BASICSTATSSERVICEGAMEREPORTPROCESSOR_H

/*** Include files *******************************************************************************/
#include "gamereportprocessor.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class BasicStatsServiceGameReportProcessor

    A simple GameReportProcessor implementation.  This is the default report processor for a stock
    blazeserver to make Stats Service updates *without* Blaze Stats updates,
    and this also provides an example for teams to customize their own processing.
*/
class BasicStatsServiceGameReportProcessor: public GameReportProcessor
{
public:
    BasicStatsServiceGameReportProcessor(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    ~BasicStatsServiceGameReportProcessor() override;

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

#endif // BLAZE_BASICSTATSSERVICEGAMEREPORTPROCESSOR_H
