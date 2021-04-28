/*************************************************************************************************/
/*!
    \file   submitgameevents_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "gamereporting/rpc/gamereportingslave/submitgameevents_stub.h"

#include "gameeventsprocessor.h"
#include "gamereportingslaveimpl.h"

namespace Blaze
{
namespace GameReporting
{

class SubmitGameEventsCommand : public SubmitGameEventsCommandStub
{
public:
    SubmitGameEventsCommand(Message * message, GameEvents *request, GameReportingSlaveImpl *componentImpl)
        :  SubmitGameEventsCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~SubmitGameEventsCommand() override
    {
    }

private:
    SubmitGameEventsCommandStub::Errors execute() override;
    GameReportingSlaveImpl *mComponent;

};
DEFINE_SUBMITGAMEEVENTS_CREATE()


SubmitGameEventsCommandStub::Errors SubmitGameEventsCommand::execute()
{
    //Check if we trust this user
    if (!UserSession::isCurrentContextAuthorized(Authorization::PERMISSION_SUBMIT_GAMEEVENTS))
    {
        WARN_LOG("[SubmitGameEventsCommandStub].execute: User [" << gCurrentUserSession->getBlazeId() << "] attempted to submit game events for processor["<< mRequest.getGameEventProcessorName() << "], no permission!");
        return ERR_AUTHORIZATION_REQUIRED;
    }

    Blaze::GameReporting::GameEvents::GameEventList::const_iterator iter = mRequest.getGameEvents().begin();
    Blaze::GameReporting::GameEvents::GameEventList::const_iterator end = mRequest.getGameEvents().end();
    for(;iter != end; ++iter)
    {
        //Pass the event on to the custom code informing it of the currently logged in user.
        GameEventsProcessor *eventProcessor = GameEventsProcessor::createInterface(mRequest.getGameEventProcessorName(), *mComponent);
        eventProcessor->process(**iter, gCurrentUserSession->getBlazeId());
        delete eventProcessor;
    }
    
    return ERR_OK;
}

}   // namespace GameReporting
}   // namespace Blaze
