/*************************************************************************************************/
/*!
    \file   logevent_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class LogEventCommand

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/controller/controller.h"

// clubs includes
#include "clubs/rpc/clubsslave/logevent_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

// messaging includes
#include "messaging/messagingslaveimpl.h"
#include "messaging/tdf/messagingtypes.h"

namespace Blaze
{
namespace Clubs
{

class LogEventCommand : public LogEventCommandStub, private ClubsCommandBase
{
public:
    LogEventCommand(Message* message, LogEventRequest* request, ClubsSlaveImpl* componentImpl)
        : LogEventCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~LogEventCommand() override
    {
    }

        /* Private methods *******************************************************************************/
private:

    LogEventCommandStub::Errors execute() override
    {
        BlazeId blazeId = INVALID_BLAZE_ID; // default super user
        if (gCurrentUserSession != nullptr)
        {
            blazeId = gCurrentUserSession->getBlazeId();
        }

        // check permission
        if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_LOG_EVENT))
        {          
            WARN_LOG("[LogEventCommand].execute: User [" << blazeId << "] attempted to log event, no permission!");
            return ERR_AUTHORIZATION_REQUIRED;
        }

        ClubsDbConnector dbc(mComponent); 
        if (dbc.acquireDbConnection(false) == nullptr)
            return ERR_SYSTEM;


        // Firstly, get the event string.
        ClubLogEventKeyType eventType = mRequest.getEventKeyType();
        const char8_t* eventString = nullptr;

        switch (eventType)
        {
        case CLUB_LOG_EVENT_CREATE_CLUB: 
            eventString = mComponent->getEventString(LOG_EVENT_CREATE_CLUB); 
            break;
        case CLUB_LOG_EVENT_JOIN_CLUB: 
            eventString = mComponent->getEventString(LOG_EVENT_JOIN_CLUB); 
            break;
        case CLUB_LOG_EVENT_LEAVE_CLUB: 
            eventString = mComponent->getEventString(LOG_EVENT_LEAVE_CLUB); 
            break;
        case CLUB_LOG_EVENT_REMOVED_FROM_CLUB: 
            eventString = mComponent->getEventString(LOG_EVENT_REMOVED_FROM_CLUB); 
            break;
        case CLUB_LOG_EVENT_AWARD_TROPHY: 
            eventString = mComponent->getEventString(LOG_EVENT_AWARD_TROPHY); 
            break;
        case CLUB_LOG_EVENT_GM_PROMOTED: 
            eventString = mComponent->getEventString(LOG_EVENT_GM_PROMOTED); 
            break;
        case CLUB_LOG_EVENT_GM_DEMOTED: 
            eventString = mComponent->getEventString(LOG_EVENT_GM_DEMOTED); 
            break;
        case CLUB_LOG_EVENT_OWNERSHIP_TRANSFERRED: 
            eventString = mComponent->getEventString(LOG_EVENT_OWNERSHIP_TRANSFERRED); 
            break;
        case CLUB_LOG_EVENT_FORCED_CLUB_NAME_CHANGE:
            eventString = mComponent->getEventString(LOG_EVENT_FORCED_CLUB_NAME_CHANGE);
            break;
		case CLUB_LOG_EVENT_MAX_CLUB_NAME_CHANGES:
			eventString = mComponent->getEventString(LOG_EVENT_MAX_CLUB_NAME_CHANGES);
			break;
        default: break;
        }
        
        // Secondly, parsing the parameters for the event.
        NewsParamType arg0type = NEWS_PARAM_NONE; 
        NewsParamType arg1type = NEWS_PARAM_NONE;
        NewsParamType arg2type = NEWS_PARAM_NONE;
        NewsParamType arg3type = NEWS_PARAM_NONE;
        const char8_t* arg0 = nullptr;
        const char8_t* arg1 = nullptr;
        const char8_t* arg2 = nullptr;
        const char8_t* arg3 = nullptr;

        ClubLogEventParams::iterator itBegin = mRequest.getEventParams().begin();
        ClubLogEventParams::iterator itEnd = mRequest.getEventParams().end();

        // if we have one param...
        if (itBegin != itEnd)
        {
            arg0type = itBegin->first;
            arg0 = itBegin->second.c_str();

            ++itBegin;
            // if we have two params...
            if (itBegin != itEnd)
            {
                arg1type = itBegin->first;
                arg1 = itBegin->second.c_str();

                ++itBegin;
                // if we have three params...
                if (itBegin != itEnd)
                {
                    arg2type = itBegin->first;
                    arg2 = itBegin->second.c_str();

                    ++itBegin;
                    // if we have four params...
                    if (itBegin != itEnd)
                    {
                        arg3type = itBegin->first;
                        arg3 = itBegin->second.c_str();
                    }
                }
            }
        }

        BlazeRpcError result = mComponent->logEvent(&(dbc.getDb()), 
                                                    mRequest.getClubId(), 
                                                    eventString,
                                                    arg0type, arg0,
                                                    arg1type, arg1,
                                                    arg2type, arg2,
                                                    arg3type, arg3);

        return commandErrorFromBlazeError(result);
    }
};

// static factory method impl
DEFINE_LOGEVENT_CREATE()

} // Clubs
} // Blaze
