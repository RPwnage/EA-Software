/*************************************************************************************************/
/*!
    \file   getnewsforclubs_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetNewsForClubsCommand

    Get news for multiple clubs.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/getnewsforclubs_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"

namespace Blaze
{
namespace Clubs
{

    class GetNewsForClubsCommand : public GetNewsForClubsCommandStub, private ClubsCommandBase
    {
    public:
        GetNewsForClubsCommand(Message* message, GetNewsForClubsRequest* request, ClubsSlaveImpl* componentImpl)
            : GetNewsForClubsCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~GetNewsForClubsCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        GetNewsForClubsCommandStub::Errors execute() override
        {
            TRACE_LOG("[GetNewsForClubsCommand] start");

            ClubLocalizedNewsListMap clubLocalizedNewsListMap;
            uint16_t totalPages;

            Blaze::BlazeRpcError result = mComponent->getNewsForClubs(
                                            mRequest.getClubIdList(), 
                                            mRequest.getTypeFilters(),
                                            mRequest.getSortType(),
                                            mRequest.getMaxResultCount(),
                                            mRequest.getOffSet(),
                                            gCurrentUserSession->getSessionLocale(),
                                            clubLocalizedNewsListMap,
                                            totalPages);

            if (result == Blaze::ERR_OK)
            {
                if (clubLocalizedNewsListMap.size() > 0)
                    clubLocalizedNewsListMap.copyInto(mResponse.getLocalizedNewsListMap());

                mResponse.setTotalPages(totalPages);
            }
           
            return (Errors)result;
        }
    };

    // static factory method impl
    DEFINE_GETNEWSFORCLUBS_CREATE()

} // Clubs
} // Blaze
