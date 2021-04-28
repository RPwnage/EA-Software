/*************************************************************************************************/
/*!
    \file   getnews_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetNewsCommand

    Get news associated with a specific club.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/getnews_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"

namespace Blaze
{
namespace Clubs
{

class GetNewsCommand : public GetNewsCommandStub, private ClubsCommandBase
{
public:
    GetNewsCommand(Message* message, GetNewsRequest* request, ClubsSlaveImpl* componentImpl)
        : GetNewsCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~GetNewsCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    GetNewsCommandStub::Errors execute() override
    {
        TRACE_LOG("[GetNewsCommand] start");

        ClubLocalizedNewsListMap clubLocalizedNewsListMap;
        uint16_t totalPages = 0;

        ClubIdList clubIdList;
        clubIdList.push_back(mRequest.getClubId());

        Blaze::BlazeRpcError result = mComponent->getNewsForClubs( clubIdList, 
            mRequest.getTypeFilters(),
            mRequest.getSortType(),
            mRequest.getMaxResultCount(),
            mRequest.getOffSet(),
            gCurrentUserSession->getSessionLocale(),
            clubLocalizedNewsListMap,
            totalPages);

        if (result == Blaze::ERR_OK)
        {
            if (clubLocalizedNewsListMap.size() > 0 
                && (clubLocalizedNewsListMap.find(mRequest.getClubId()) != clubLocalizedNewsListMap.end()))
            {
                clubLocalizedNewsListMap[mRequest.getClubId()]->copyInto(mResponse.getLocalizedNewsList());
            }

            mResponse.setTotalPages(totalPages);
        }

        return (Errors)result;
    }

};
// static factory method impl
DEFINE_GETNEWS_CREATE()

} // Clubs
} // Blaze
