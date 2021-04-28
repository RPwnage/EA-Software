/*************************************************************************************************/
/*!
    \file   gamereportcollator.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_GAMEREPORTCOLLATOR_H
#define BLAZE_GAMEREPORTCOLLATOR_H

/*** Include files *******************************************************************************/
#include "gamereporting/tdf/gamereporting.h"
#include "gamereporting/tdf/gamereporting_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

class GameReportingSlaveImpl;

/*!
    class GameReportCollator

    Teams that wish to modify report collation can implement their own GameReportCollator based class.
    It's suggested to implement a reporting pipeline per GameType.  Often processing is customizable by
    GameType, so separate pipeline classes will help to prevent switch/case statements based on GameReportName
    within a pipeline's implementation.

    The class factory method create takes a string identifier.  When Game Reporting creates
    a pipeline object, it passes along the GameInfo from StartedGameInfo and a string identifier defined in
    gamereporting.cfg. 

    gameTypeA = {
        reportProcessorClass = "basic",
        reportCollatorClass = "basic"
    }

    Teams must add construction code to GameReportingSlaveImpl::registerCustomGameReportCollators so
    that Game Reporting can create the pipeline object associated with the class name specified in
    gamereporting.cfg for the game report type.

    Since collation for an individual game report can be done by any GR slave, collators will be
    created/destroyed on demand as different slaves receive game reports to collate.  Therefore,
    any persistent custom collation data should be setup as a custom TDF and added to the external
    ReportCollateData object used to create the collator (see ReportCollateData::mCustomData).
*/
class GameReportCollator
{
    NON_COPYABLE(GameReportCollator);

public:
    enum ReportResult
    {
        RESULT_COLLATE_NO_REPORTS,          //<! No reports submitted for collation.
        RESULT_COLLATE_CONTINUE,            //<! Continue collation of GameReport.
        RESULT_COLLATE_REJECTED,            //<! Rejected report but continue collation phase.
        RESULT_COLLATE_FAILED,              //<! Rejected report, stop collation and throw out the game report.
        RESULT_COLLATE_REJECT_DUPLICATE,    //<! Rejected report due to duplicate but continue collation.
        RESULT_COLLATE_REJECT_INVALID_PLAYER,   //<! Rejected report due to player not being in game but continue collation.
        RESULT_COLLATE_COMPLETE             //<! Marks the end of the game report - triggers call to finalizeCollatedGameReport.
    };

public:
    virtual ~GameReportCollator() { }

    /*! ****************************************************************************/
    /*! \brief Triggered when GameReporting is ready to process the final collated report.

        \param collatedReportType The report type for the final collated report submitted to
            the slave.
        \return The collated game report to be sent for processing on slave.
    ********************************************************************************/
    virtual CollatedGameReport& finalizeCollatedGameReport(ReportType collatedReportType) = 0;

    /*! ****************************************************************************/
    /*! \brief Triggered when a report is submitted by a slave for collection.

        This override allows customization of how reports are collected before collating the report.

        The implementation may simply add the report to a report collection via the Collector
        utility.   Or it may attempt to collate that report into an existing GameReport
        to reduce the number of GameReports collated in the finalizeCollatedGameReport override.

        \param playerId Submitter of game report
        \param report A client GameReport submitted by a game
        \param privateReport Private data submitted by this player
        \param finishedStatus How to handle finished game status
        \param reportType Report type

        \return See ReportResult for details.
    ********************************************************************************/
    virtual ReportResult reportSubmitted(BlazeId playerId, const GameReport& report, const EA::TDF::Tdf* privateReport, GameReportPlayerFinishedStatus finishedStatus, ReportType reportType) = 0;
 
    /*! ****************************************************************************/
    /*! \brief Triggered when the game has finished, supplying the final game state.

        \param gameInfo GameInfo sent by GameManager on game finish.
        \return ERR_OK on success.  On error throws out the game report
    ********************************************************************************/
    virtual ReportResult gameFinished(const GameInfo& gameInfo) = 0;

    /*! ****************************************************************************/
    /*! \brief Report timeout triggered, decide whether to process or throw out the report.

        \return ERR_OK to process current report.  On error, throws out the report.
    ********************************************************************************/
    virtual ReportResult timeout() const = 0;

    /*! ****************************************************************************/
    /*! \brief Performs any cleanup needed after game reporting processes a report.
    ********************************************************************************/
    virtual void postProcessCollatedReport() = 0;

    /*! ****************************************************************************/
    /*! \brief Triggered when the persistent data's game type changes.

        It's possible that a report has been submitted of a different game type for the current game;
        if that is the case then simply keep going using the new one -- and any already collected
        reports are normally discarded (because they would have the old game type).
    ********************************************************************************/
    virtual void onGameTypeChanged();

public:
    GameManager::GameReportingId getGameReportingId() const {
        return mGameReport.getStartedGameInfo().getGameReportingId();
    }
    const char8_t* getGameReportName() const {
        return mGameReport.getStartedGameInfo().getGameReportName();
    }
    GameNetworkTopology getGameNetworkTopology() const {
        return mGameReport.getStartedGameInfo().getTopology();
    }
    bool usesTrustedGameReporting() const {
        return mGameReport.getStartedGameInfo().getTrustedGameReporting();
    }
    GameReportsMap& getCollectedGameReportsMap() {
        return mGameReport.getCollectedGameReportsMap();
    }

    GameManager::GameId getGameId() const {
        return mGameReport.getStartedGameInfo().getGameId();
    }

    static const char8_t* getReportResultName(ReportResult reportResult);

protected:
    GameReportCollator(ReportCollateData& gameReport, GameReportingSlaveImpl& component);

    GameReportingSlaveImpl& getComponent() {
        return mComponent;
    }
    GameReportingSlaveImpl& getComponent() const {
        return mComponent;
    }

    EA::TDF::Tdf* getCustomData()
    {
        return mGameReport.getCustomData();
    }

    void clearCollectedGameReportMap();

private:
    GameReportingSlaveImpl& mComponent;
    ReportCollateData& mGameReport;
    uint32_t mRefCount;

    friend void intrusive_ptr_add_ref(GameReportCollator* ptr);
    friend void intrusive_ptr_release(GameReportCollator* ptr);
};

typedef eastl::intrusive_ptr<GameReportCollator> GameReportCollatorPtr;
inline void intrusive_ptr_add_ref(GameReportCollator* ptr)
{
    ptr->mRefCount++;
}

inline void intrusive_ptr_release(GameReportCollator* ptr)
{
    if (--ptr->mRefCount == 0)
        delete ptr;
}

}   // namespace GameReporting
}   // namespace Blaze

#endif // BLAZE_GAMEREPORTCOLLATOR_H
