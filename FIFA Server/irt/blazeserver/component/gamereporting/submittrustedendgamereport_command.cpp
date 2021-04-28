/*************************************************************************************************/
/*!
    \file   submittrustedendgamereport_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "gamereporting/rpc/gamereportingslave/submittrustedendgamereport_stub.h"
#include "gamereporting/tdf/gamereportingevents_server.h"
#include "framework/event/eventmanager.h"

#include "framework/usersessions/authorization.h"
#include "framework/controller/controller.h"
#include "framework/usersessions/usersession.h"

#include "gamereportingslaveimpl.h"

#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"

namespace Blaze
{
namespace GameReporting
{

class SubmitTrustedEndGameReportCommand : public SubmitTrustedEndGameReportCommandStub
{
public:
    SubmitTrustedEndGameReportCommand(Message * message, SubmitGameReportRequest *request, GameReportingSlaveImpl *componentImpl)
        :  SubmitTrustedEndGameReportCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~SubmitTrustedEndGameReportCommand() override
    {
    }

private:
    SubmitTrustedEndGameReportCommandStub::Errors execute() override;
    GameReportingSlaveImpl *mComponent;

};
DEFINE_SUBMITTRUSTEDENDGAMEREPORT_CREATE()


SubmitTrustedEndGameReportCommandStub::Errors SubmitTrustedEndGameReportCommand::execute()
{
    GameReport& gameReport = mRequest.getGameReport();

    //Check if we trust this user
    if(!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_SUBMIT_TRUSTED_GAMEREPORT))
    {
        WARN_LOG("[SubmitTrustedEndGameReportCommand].execute: User [" << gCurrentUserSession->getBlazeId() << "] attempted to submit trusted end game report [" << gameReport.getGameReportingId() << "] name [" << gameReport.getGameReportName() << "], no permission!");
        return ERR_AUTHORIZATION_REQUIRED;
    }

    GameManager::GameReportName gameReportName = gameReport.getGameReportName();
    const GameType *gameType = mComponent->getGameTypeCollection().getGameType(gameReportName);
    if (gameType == nullptr)
    {
        return SubmitTrustedEndGameReportCommandStub::GAMEREPORTING_TRUSTED_ERR_INVALID_GAME_TYPE;
    }

    if (gameReport.getReport() == nullptr)
    {
        ERR_LOG("[SubmitTrustedEndGameReportCommandStub].execute() : nullptr report passed by submitted.");
        return ERR_SYSTEM;
    }

    GameReportProcessorPtr reportProcessor = GameReportProcessorPtr(mComponent->createGameReportProcessor(gameType->getReportProcessorClass()));
    if (reportProcessor == nullptr)
    {
        // Likely this warning is a result of a mismatch between the GameReportProcessor class factory and the game reporting configuration
        // (Undefined class name, or customer needs to update class factory)
        ERR_LOG("[SubmitTrustedEndGameReportCommand].execute() : Unable to generate a validator for submitted game report id=" 
                << gameReport.getGameReportingId() << ".");
        return ERR_SYSTEM;
    }

    if (!mComponent-> isGameManagerRunning())
    {
        GameManager::GameReportingId id;
        if (mComponent->getNextGameReportingId(id) != Blaze::ERR_OK)
        {
            ERR_LOG("[SubmitTrustedEndGameReportCommand].execute() : Unable to fetch the next game reporting id.");
            return ERR_SYSTEM;
        }

        gameReport.setGameReportingId(id);
    }

    //  run custom validation on report before processing.
    BlazeRpcError reportError = reportProcessor->validate(gameReport);
    if (reportError == Blaze::ERR_OK)
    {
        mComponent->getMetrics().mValidReports.increment(1, REPORT_TYPE_TRUSTED_END_GAME, gameReportName.c_str());
        if (!mComponent-> isGameManagerRunning())
        {
            // start the game
            StartedGameInfo gameInfo;
            gameInfo.setGameReportingId(gameReport.getGameReportingId());
            gameInfo.setGameReportName(gameReportName);
            gameInfo.setTrustedGameReporting(true);
            reportError = mComponent->gameStarted(gameInfo);
        }
        if (reportError == Blaze::ERR_OK)
        {
            if (mComponent->getConfig().getEnableTrustedReportCollation())
            {
                //  send report off for collation step (usually enabled only if there's a need to customize trusted report collation.)
                reportError = mComponent->collectGameReport(gCurrentUserSession->getBlazeId(), REPORT_TYPE_TRUSTED_END_GAME, mRequest);
            }
            else
            {
                //  send report off for processing immediately, simulating a CollatedGameReport
                reportError = mComponent->scheduleTrustedReportProcess(gCurrentUserSession->getBlazeId(), REPORT_TYPE_TRUSTED_END_GAME, mRequest);
            }
        }
    }
    else
    {
        mComponent->getMetrics().mInvalidReports.increment(1, REPORT_TYPE_TRUSTED_END_GAME, gameReportName.c_str());
        reportError = Blaze::GAMEREPORTING_TRUSTED_ERR_REPORT_INVALID;

        // submit invalid game reporting event
        GameReportInvalid gameReportInvalid;
        gameReportInvalid.setPlayerId(gCurrentUserSession->getBlazeId());
        gameReport.copyInto(gameReportInvalid.getGameReport());
        gEventManager->submitEvent(GameReportingSlave::EVENT_GAMEREPORTINVALIDEVENT, gameReportInvalid, true);
    }

    if (reportError == Blaze::ERR_OK  && gCurrentUserSession->getClientType() != CLIENT_TYPE_DEDICATED_SERVER)
    {
        INFO_LOG("[SubmitTrustedEndGameReportCommand].execute: User [" << gCurrentUserSession->getBlazeId() << "] submitted trusted end game report [" << gameReport.getGameReportingId() << "] name [" << gameReport.getGameReportName() << "], had permission.");
    }
    return commandErrorFromBlazeError(reportError);
}


}   // namespace GameReporting
}   // namespace Blaze
