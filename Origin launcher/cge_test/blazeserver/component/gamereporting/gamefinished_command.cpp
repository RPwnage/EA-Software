/*************************************************************************************************/
/*!
    \file   gamefinished_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/selector.h"

#include "gamereporting/rpc/gamereportingslave/gamefinished_stub.h"
#include "gamereporting/tdf/gamereportingevents_server.h"
#include "gamereportingslaveimpl.h"

namespace Blaze
{
namespace GameReporting
{

/*************************************************************************************************/
/*!
    \class GameFinishedCommand

    Notify a game report that a game has finished and begin report collation or processing.
*/
/*************************************************************************************************/

class GameFinishedCommand : public GameFinishedCommandStub
{
public:
    GameFinishedCommand(Message *message, Blaze::GameManager::GameInfo *request, GameReportingSlaveImpl *componentImpl)
    :   GameFinishedCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~GameFinishedCommand() override
    {
    }

private:
    GameFinishedCommandStub::Errors execute() override;

private:
    GameReportingSlaveImpl *mComponent;
};
DEFINE_GAMEFINISHED_CREATE()

GameFinishedCommandStub::Errors GameFinishedCommand::execute()
{
    GameReportingSlaveImpl::AutoIncDec autoGauge(mComponent->mMetrics.mGaugeActiveProcessReportCount);
    // ignore trusted games (that do not use collation)
    if (mRequest.getTrustedGameReporting() && !mComponent->getConfig().getEnableTrustedReportCollation())
    {
        return ERR_OK;
    }

    TRACE_LOG("[GameFinishedCommand]: looking for report id " << mRequest.getGameReportingId() << " in the local map.");

    GameReportMasterPtr gameReportMaster = mComponent->getWritableGameReport(mRequest.getGameReportingId());
    if (gameReportMaster == nullptr)
    {
        WARN_LOG("[GameFinishedCommand]: found no game report ("
            << mRequest.getGameReportName() << ") with id " << mRequest.getGameReportingId());

        if (mRequest.getGameSettings().getSendOrphanedGameReportEvent())
        {
            mComponent->submitGameReportOrphanedEvent(mRequest, mRequest.getGameReportingId());
        }
        return ERR_OK;
    }

    // Changes made to gameReport will be committed when gameReportMaster goes out of scope.
    ReportCollateData* gameReport = gameReportMaster->getData();

    gameReport->setIsFinished(true);
    mRequest.copyInto(gameReport->getGameInfo());
    gameReport->setFinishedGameTimeMs(TimeValue::getTimeOfDay().getMillis());
    gameReport->setProcessingStartTime(TimeValue::getTimeOfDay() + (mComponent->getConfig().getCollectionTimeoutSec() * 1000 * 1000));

    // create a game report collator
    GameReportCollatorPtr collator;
    const GameType* gameType = mComponent->getGameTypeCollection().getGameType(gameReport->getStartedGameInfo().getGameReportName());
    if (gameType != nullptr)
    {
        // if game type name provided is valid, create the report collator
        collator = GameReportCollatorPtr(mComponent->createGameReportCollator(gameType->getReportCollatorClass(), *gameReport));
    }
    GameReportCollator::ReportResult reportResult = GameReportCollator::RESULT_COLLATE_CONTINUE;

    if (collator != nullptr)
    {
        // update expected report count
        switch (gameReport->getReportType())
        {
        case REPORT_TYPE_TRUSTED_MID_GAME:
        case REPORT_TYPE_TRUSTED_END_GAME:
            mComponent->getMetrics().mReportsExpected.increment(1, gameReport->getReportType(), mRequest.getGameReportName());
            break;
        case REPORT_TYPE_STANDARD:
            mComponent->getMetrics().mReportsExpected.increment(gameReport->getGameInfo().getPlayerInfoMap().size(), gameReport->getReportType(), mRequest.getGameReportName());
            break;
        case REPORT_TYPE_OFFLINE:
            // no metric
            break;
        }

        reportResult = collator->gameFinished(mRequest);

        if (reportResult == GameReportCollator::RESULT_COLLATE_COMPLETE)
        {
            TRACE_LOG("[GameFinishedCommand]: collate complete for game report ("
                << mRequest.getGameReportName() << ") with id " << mRequest.getGameReportingId() << " ... processing report");
            mComponent->completeCollation(*collator, gameReportMaster);
        }
        else if (reportResult != GameReportCollator::RESULT_COLLATE_CONTINUE)
        {
            TRACE_LOG("[GameFinishedCommand]: collate failed for game report ("
                << mRequest.getGameReportName() << ") with id " << mRequest.getGameReportingId() << " ... throwing out report");
            mComponent->submitGameReportCollationFailedEvent(collator->getCollectedGameReportsMap(), mRequest.getGameReportingId());
            mComponent->eraseGameReport(mRequest.getGameReportingId());
        }
    }

    if (reportResult == GameReportCollator::RESULT_COLLATE_CONTINUE)
    {
        TRACE_LOG("[GameFinishedCommand]: starting collation/processing timer for game report ("
            << mRequest.getGameReportName() << ") with id " << mRequest.getGameReportingId());

        // report collation incomplete - continue to collate reports until timeout reached, or until collation completes.
        TimerId processingStartTimerId = gSelector->scheduleFiberTimerCall(gameReport->getProcessingStartTime(), mComponent, &GameReportingSlaveImpl::collateTimeoutHandler,
            mRequest.getGameReportingId(), "GameReportingSlaveImpl::collateTimeoutHandler");

        gameReportMaster->setProcessingStartTimerId(processingStartTimerId);
    }

    // game report will now wait for incoming reports before processing
    return ERR_OK;
}

} // namespace GameReporting
} // namespace Blaze
