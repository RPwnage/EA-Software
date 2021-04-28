/*************************************************************************************************/
/*!
    \file   setclubtickermessagessubscription_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class SetClubTickerMessagesSubscription

    Subscribe or unsubscribe client to club's ticker messages

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"

// clubs includes
#include "clubs/rpc/clubsslave/setclubtickermessagessubscription_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

// stats includes
#include "stats/rpc/statsslave.h"
#include "stats/tdf/stats.h"
#include "stats/statscommontypes.h"
#include "stats/statsslaveimpl.h"

namespace Blaze
{
namespace Clubs
{

    class SetClubTickerMessagesSubscriptionCommand : public SetClubTickerMessagesSubscriptionCommandStub, private ClubsCommandBase
    {
    public:
        SetClubTickerMessagesSubscriptionCommand(Message* message, SetClubTickerMessagesSubscriptionRequest *request, ClubsSlaveImpl* componentImpl)
            : SetClubTickerMessagesSubscriptionCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~SetClubTickerMessagesSubscriptionCommand() override
        {
        }

    /* Private methods *******************************************************************************/
    private:

        SetClubTickerMessagesSubscriptionCommandStub::Errors execute() override
        {
            TRACE_LOG("[SetClubTickerMessagesSubscription] start");
            
            // check if valid clubid
            if (mRequest.getClubId() == INVALID_CLUB_ID)
            {
                return CLUBS_ERR_INVALID_CLUB_ID;
            }
            
            BlazeId blazeId = gCurrentUserSession->getBlazeId();
            ClubId clubId = mRequest.getClubId();

            ClubsDbConnector dbc(mComponent); 
            if (dbc.acquireDbConnection(true) == nullptr)
                return ERR_SYSTEM;
            
            // check if user is member of club
            if (!mComponent->checkMembership(dbc.getDbConn(), clubId, blazeId))
            {
                dbc.releaseConnection();
                return CLUBS_ERR_NO_PRIVILEGE;
            }

            BlazeRpcError result =    
                mComponent->setClubTickerSubscription(
                    clubId,
                    blazeId,
                    mRequest.getIsSubscribed());
                
            dbc.releaseConnection();

            return commandErrorFromBlazeError(result);
        }
    };

    // static factory method impl
    DEFINE_SETCLUBTICKERMESSAGESSUBSCRIPTION_CREATE()

} // Clubs
} // Blaze
