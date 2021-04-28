/*************************************************************************************************/
/*!
    \file   submitgamereport_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "gamereporting/rpc/gamereportingslave/submitgamereport_stub.h"
#include "gamereporting/tdf/gamereportingevents_server.h"
#include "gamereportingslaveimpl.h"

namespace Blaze
{
namespace GameReporting
{

//
//  SubmitGameReportCommand
//      For Non-dedicated-server (i.e. untrusted) game reports.
//      In this model, each player submits a game report to be collated
//
class SubmitGameReportCommand : public SubmitGameReportCommandStub
{
public:
    SubmitGameReportCommand(Message * message, SubmitGameReportRequest *request, GameReportingSlaveImpl *componentImpl)
        :  SubmitGameReportCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~SubmitGameReportCommand() override
    {
    }

private:
    SubmitGameReportCommandStub::Errors execute() override;
    GameReportingSlaveImpl *mComponent;

};
DEFINE_SUBMITGAMEREPORT_CREATE()


//  SubmitGameReportCommand 
//      Non-trusted reports sent by peers in a game.
//          Player submits a report to be collated with other player reports.
//          Custom validation run against report before collation.
//          Once submitted for collation, the command completes.
SubmitGameReportCommandStub::Errors SubmitGameReportCommand::execute()
{
    //Check if we trust this user
    if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_SUBMIT_ONLINE_GAMEREPORT))
    {
        WARN_LOG("[SubmitGameReportCommandStub].execute: User [" << gCurrentUserSession->getBlazeId() << "] attempted to submit an online game report ["<< mRequest.getGameReport().getGameReportingId() << "] name ["<< mRequest.getGameReport().getGameReportName() << "], no permission!");
        return ERR_AUTHORIZATION_REQUIRED;
    }

    GameReport& gameReport = mRequest.getGameReport();
    GameManager::GameReportName gameReportName = gameReport.getGameReportName();
    const GameType *gameType = mComponent->getGameTypeCollection().getGameType(gameReportName);
    if (gameType == nullptr)
    {
        return SubmitGameReportCommandStub::GAMEREPORTING_ERR_INVALID_GAME_TYPE;
    }

    if (gameReport.getReport() == nullptr)
    {
        ERR_LOG("[SubmitGameReportCommand].execute() : nullptr report passed by submitted.");
        return SubmitGameReportCommandStub::ERR_SYSTEM;
    }
    
    GameReportProcessorPtr reportProcessor = GameReportProcessorPtr(mComponent->createGameReportProcessor(gameType->getReportProcessorClass()));
    if (reportProcessor == nullptr)
    {
        // Likely this warning is a result of a mismatch between the GameReportProcessor class factory and the game reporting configuration
        // (Undefined class name, or customer needs to update class factory)
        ERR_LOG("[SubmitGameReportCommand].execute() : Unable to generate a validator for submitted game report id=" 
                << mRequest.getGameReport().getGameReportingId() << ".");
        return commandErrorFromBlazeError(Blaze::ERR_SYSTEM);
    }

    //  run custom validation on report before sending for collation and processing.
    BlazeRpcError reportError = reportProcessor->validate(gameReport);
    if (reportError == Blaze::ERR_OK)
    {
        mComponent->getMetrics().mValidReports.increment(1, REPORT_TYPE_STANDARD, gameReportName.c_str());
        reportError = mComponent->collectGameReport(gCurrentUserSession->getBlazeId(), REPORT_TYPE_STANDARD, mRequest);
    }
    else
    {
        mComponent->getMetrics().mInvalidReports.increment(1, REPORT_TYPE_STANDARD, gameReportName.c_str());
    }

    return commandErrorFromBlazeError(reportError);
}

}   // namespace GameReporting
}   // namespace Blaze
