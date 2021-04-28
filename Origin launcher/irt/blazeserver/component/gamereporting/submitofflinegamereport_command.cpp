/*************************************************************************************************/
/*!
    \file   submitofflinegamereport_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "gamereporting/rpc/gamereportingslave/submitofflinegamereport_stub.h"
#include "gamereporting/tdf/gamereportingevents_server.h"
#include "framework/event/eventmanager.h"
#include "stats/rpc/statsslave.h"

#include "gamereportingslaveimpl.h"

#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"

namespace Blaze
{
namespace GameReporting
{

class SubmitOfflineGameReportCommand : public SubmitOfflineGameReportCommandStub
{
public:
    SubmitOfflineGameReportCommand(Message * message, SubmitGameReportRequest *request, GameReportingSlaveImpl *componentImpl)
        :  SubmitOfflineGameReportCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~SubmitOfflineGameReportCommand() override
    {
    }

private:
    SubmitOfflineGameReportCommandStub::Errors execute() override;
    GameReportingSlaveImpl *mComponent;

};
DEFINE_SUBMITOFFLINEGAMEREPORT_CREATE()

//  SubmitOfflineGameReportCommand 
//      Reports submitted that are not associated with a GameManager game.
//          Custom validation run against report before processing.
//          Publish game history - TODO incorporate offline games into same table as
//              "online" games for that type.
SubmitOfflineGameReportCommandStub::Errors SubmitOfflineGameReportCommand::execute()
{
    //Check if we trust this user
    if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_SUBMIT_OFFLINE_GAMEREPORT))
    {
        WARN_LOG("[SubmitOfflineGameReportCommandStub].execute: User [" << gCurrentUserSession->getBlazeId() << "] attempted to submit an offline game report ["<< mRequest.getGameReport().getGameReportingId() << "] name ["<< mRequest.getGameReport().getGameReportName() << "], no permission!");
        return ERR_AUTHORIZATION_REQUIRED;
    }

    mComponent->getMetrics().mOfflineGames.increment();

    GameReport& gameReport = mRequest.getGameReport();
    GameManager::GameReportName gameReportName = gameReport.getGameReportName();
    const GameType *gameType = mComponent->getGameTypeCollection().getGameType(gameReportName);
    if (gameType == nullptr)
    {
        return GAMEREPORTING_OFFLINE_ERR_INVALID_GAME_TYPE;
    }

    if (gameReport.getReport() == nullptr)
    {
        WARN_LOG("[SubmitOfflineGameReportCommand].execute() : nullptr report passed by submitted.");
        return ERR_SYSTEM;
    }

    GameReportProcessorPtr reportProcessor = GameReportProcessorPtr(mComponent->createGameReportProcessor(gameType->getReportProcessorClass()));
    if (reportProcessor == nullptr)
    {
        // Likely this warning is a result of a mismatch between the GameReportProcessor class factory and the game reporting configuration
        // (Undefined class name, or customer needs to update class factory)
        WARN_LOG("[SubmitOfflineGameReportCommand].execute() : Unable to generate a validator for submitted game report id=" 
                 << gameReport.getGameReportingId() << ".");
        return ERR_SYSTEM;
    }

    GameManager::GameReportingId id;
    if (mComponent->getNextGameReportingId(id) != Blaze::ERR_OK)
    {
        ERR_LOG("[SubmitOfflineGameReportCommand].execute() : Unable to fetch the next game reporting id.");
        return ERR_SYSTEM;
    }

    gameReport.setGameReportingId(id);

    //  since no players are involved, simply dispatch result notification back to current user.
    ResultNotification result;
    result.setFinalResult(true); // default for offline game reports

    //  run custom validation on report before processing.
    BlazeRpcError reportError = reportProcessor->validate(gameReport);
    if (reportError == Blaze::ERR_OK)
    {
        mComponent->getMetrics().mValidReports.increment(1, REPORT_TYPE_OFFLINE, gameReport.getGameReportName());

        // mComponent->mMetricsCollection->updateGameMetrics(collatedReport);
        // initialize the participant list provided by the report
        GameManager::PlayerIdList& participantIds = gameReport.getOfflineParticipantList();
        if (participantIds.empty())
        {
            participantIds.push_back(gCurrentUserSession->getBlazeId());
        }

        CollatedGameReport::PrivateReportsMap privateReportsMap;
        if (mRequest.getPrivateReport() != nullptr)
        {
            privateReportsMap[0] = privateReportsMap.allocate_element();
            privateReportsMap[0]->setData(*mRequest.getPrivateReport());
        }
        CollatedGameReport::DnfStatusMap dnfStatusMap;
        GameInfo* nullGameInfo = nullptr;
        GameManager::PlayerIdList submitterIds;
        submitterIds.push_back(gCurrentUserSession->getBlazeId());

        ProcessedGameReport processedReport(REPORT_TYPE_OFFLINE, *gameType, gameReport, privateReportsMap, nullGameInfo, dnfStatusMap, submitterIds, mComponent->getCustomMetrics(), GameManager::INVALID_GAME_ID);
        reportError = reportProcessor->process(processedReport, participantIds);

        if (reportError == Blaze::ERR_OK)
        {
            // save the report to the history tables if required
            mComponent->getMetrics().mGamesProcessedSuccessfully.increment(1, REPORT_TYPE_OFFLINE, gameReportName.c_str());

            if (processedReport.needsHistorySave())
            {
                BlazeRpcError saveErr = mComponent->saveHistoryReport(&processedReport, *gameType, participantIds, submitterIds, false);
                if (saveErr!= Blaze::ERR_OK)
                {
                    WARN_LOG("[SubmitOfflineGameReportCommand].execute() : Unable to saveHistoryReport err=" << (ErrorHelp::getErrorName(saveErr)));
                }
            }
            
            //  submit success report event
            mComponent->submitSucceededEvent(processedReport, nullptr, true);
        }
        else
        {
            mComponent->getMetrics().mGamesWithProcessingErrors.increment(1, REPORT_TYPE_OFFLINE, gameReportName.c_str());
            if (BLAZE_COMPONENT_FROM_ERROR(reportError)==Blaze::Stats::StatsSlave::COMPONENT_ID)
            {
                mComponent->getMetrics().mGamesWithStatUpdateFailure.increment(1, REPORT_TYPE_OFFLINE, gameReportName.c_str());
            }

            //  submit failure report event
            mComponent->submitGameReportProcessFailedEvent(processedReport, nullptr);
        }

        //Copy the custom data tdf, if it exists. 
        if (processedReport.getCustomData() != nullptr) 
        { 
            result.setCustomData(*(processedReport.getCustomData()->clone())); 
        } 

        result.setFinalResult(processedReport.isFinalResult()); // value after report processing
    }
    else
    {
        mComponent->getMetrics().mInvalidReports.increment(1, REPORT_TYPE_OFFLINE, gameReportName.c_str());

        reportError = Blaze::GAMEREPORTING_OFFLINE_ERR_REPORT_INVALID;

        // submit invalid game reporting event
        GameReportInvalid gameReportInvalid;
        gameReportInvalid.setPlayerId(UserSession::getCurrentUserBlazeId());
        gameReport.copyInto(gameReportInvalid.getGameReport());
        gEventManager->submitEvent(GameReportingSlave::EVENT_GAMEREPORTINVALIDEVENT, gameReportInvalid, true);
    }

    result.setGameReportingId(gameReport.getGameReportingId());
    result.setGameHistoryId(gameReport.getGameReportingId());
    result.setBlazeError(reportError);

    mComponent->sendResultNotificationToUserSessionById(UserSession::getCurrentUserSessionId(), &result);

    return ERR_OK;
}

}   // namespace GameReporting
}   // namespace Blaze
