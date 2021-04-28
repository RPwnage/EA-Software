/*************************************************************************************************/
/*!
    \file   listrivals_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ListRivalsCommand

    Get rivals for specific club

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
#include "clubs/rpc/clubsslave/listrivals_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

    class ListRivalsCommand : public ListRivalsCommandStub, private ClubsCommandBase
    {
    public:
        ListRivalsCommand(Message* message, ListRivalsRequest* request, ClubsSlaveImpl* componentImpl)
            : ListRivalsCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~ListRivalsCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        ListRivalsCommandStub::Errors execute() override
        {
            TRACE_LOG("[ListRivals Command] start");
            
            ClubsDbConnector dbc(mComponent);

            if (dbc.acquireDbConnection(true) == nullptr)
                return ERR_SYSTEM;
            
            BlazeRpcError result = mComponent->listRivals(
                dbc.getDbConn(), 
                mRequest.getClubId(), 
                mResponse.getClubRivalList());
            
            if (result != Blaze::ERR_OK)
            {
                ERR_LOG("[ListRivalsCommand] Database error (" << result << ")");
            }
            else
            {
                if (mResponse.getClubRivalList().size() > OC_MAX_ITEMS_PER_FETCH)
                    result = Blaze::CLUBS_ERR_TOO_MANY_ITEMS_PER_FETCH_REQUESTED;
            }
            
            TRACE_LOG("[ListRivals Command] end");
            
            return commandErrorFromBlazeError(result);            
        }
    };

    // static factory method impl
    DEFINE_LISTRIVALS_CREATE()

} // Clubs
} // Blaze
