/*************************************************************************************************/
/*!
    \file   basicgamereportcollator.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_BASICGAMEREPORTCOLLATOR_H
#define BLAZE_BASICGAMEREPORTCOLLATOR_H

/*** Include files *******************************************************************************/
#include "gamereporting/tdf/gamereporting.h"
#include "gamereporting/tdf/gamereporting_server.h"
#include "gamereportcollator.h"
#include "util/collectorutil.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{
/*!
    class BasicGameReportCollator

    A simple GameReportCollator implementation.  This is the default report collator for a stock
    blazeserver, and this also provides an example for teams to customize their own collation.
*/
class BasicGameReportCollator: public GameReportCollator
{
    NON_COPYABLE(BasicGameReportCollator);

public:
    BasicGameReportCollator(ReportCollateData& gameReport, GameReportingSlaveImpl& component);
    static GameReportCollator* create(ReportCollateData& gameReport, GameReportingSlaveImpl& component);

    ~BasicGameReportCollator() override;

    /*! ****************************************************************************/
    /*! \brief Triggered when GameReporting is ready to process the final collated report.

        \param collatedReportType The report type for the final collated report

        \return The collated game report to be sent for processing
    ********************************************************************************/
    CollatedGameReport& finalizeCollatedGameReport(ReportType collatedReportType) override;

    /*! ****************************************************************************/
    /*! \brief Check for whether or not to force the dnf collation and
               merging of the staging reports into a final collated report

        \return default is to not force, this is to be overridden by custom collator's

    ********************************************************************************/
    virtual bool forceDnfCollation() { return false; }

    /*! ****************************************************************************/
    /*! \brief Triggered when a report is submitted for collection.

        This override allows customization of how reports are collected before collating the report.

        The implementation may simply add the report to a report collection via the Collector
        utility.   Or it may attempt to collate that report into an existing GameReport
        to reduce the number of GameReports collated in the finalizeCollatedGameReport override.

        \param playerId Submitter of game report
        \param report A client GameReport submitted by a game
        \param privateReport PrivateReport as submitted by player
        \param finishedStatus How to handle finished game status
        \param reportType Report type

        \return See ReportResult for details.
    ********************************************************************************/
    ReportResult reportSubmitted(BlazeId playerId, const GameReport& report,  const EA::TDF::Tdf* privateReport, GameReportPlayerFinishedStatus finishedStatus, ReportType reportType) override;
 
    /*! ****************************************************************************/
    /*! \brief Triggered when the game has finished, supplying the final game state.

        \param gameInfo GameInfo sent by GameManager on game finish

        \return ERR_OK on success.  On error throws out the game report
    ********************************************************************************/
    ReportResult gameFinished(const GameInfo& gameInfo) override;

    /*! ****************************************************************************/
    /*! \brief Report timeout triggered, decide whether to process or throw out the report.

        \return ERR_OK to process current report.  On error, throws out the report.
    ********************************************************************************/   
    ReportResult timeout() const override;

    /*! ****************************************************************************/
    /*! \brief Performs any cleanup needed after game reporting processes a report.
    ********************************************************************************/  
    void postProcessCollatedReport() override;

    /*! ****************************************************************************/
    /*! \brief Triggered when the persistent data's game type changes.

        It's possible that a report has been submitted of a different game type for the current game;
        if that is the case then simply keep going using the new one -- and any already collected
        reports are normally discarded (because they would have the old game type).
    ********************************************************************************/
    void onGameTypeChanged() override;

protected:
    const Utilities::Collector& getCollector() const { return mCollectorUtil; }
    const GameInfo& getGameInfo() const { return mFinishedGameInfo; }

    // IMPORTANT:  If you implement a collator derived from this BasicGameReportCollator
    // class *AND* use a different custom TDF, then you must ensure that mCollatedReport
    // points to an appropriate CollatedGameReport object in your custom TDF.
    CollatedGameReport* mCollatedReport;

protected:
	Utilities::Collector mCollectorUtil;
	GameInfo& mFinishedGameInfo;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif // BLAZE_BASICGAMEREPORTCOLLATOR_H
