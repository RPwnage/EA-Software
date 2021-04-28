/*************************************************************************************************/
/*!
    \file   getclubs_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetClubCommand

    Get a specific club by its Id

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
#include "clubs/rpc/clubsslave/getclubs_stub.h"
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

    class GetClubsCommand : public GetClubsCommandStub, private ClubsCommandBase
    {
    public:
        GetClubsCommand(Message* message, GetClubsRequest* request, ClubsSlaveImpl* componentImpl)
            : GetClubsCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~GetClubsCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:
        GetClubsCommandStub::Errors execute() override
        {
            TRACE_LOG("[GetClubCommand] start");
            
            if (mRequest.getClubIdList().size() > OC_MAX_ITEMS_PER_FETCH)
                return CLUBS_ERR_TOO_MANY_ITEMS_PER_FETCH_REQUESTED;

            ClubsDbConnector dbc(mComponent);

            if (dbc.acquireDbConnection(true) == nullptr)
                return ERR_SYSTEM;
            
            if (mRequest.getClubIdList().size() == 0)
                return CLUBS_ERR_INVALID_ARGUMENT;

            uint32_t totalCount = 0;
            uint64_t* versions = nullptr;
            BlazeRpcError result = dbc.getDb().getClubs(mRequest.getClubIdList(),
                mResponse.getClubList(),
                versions,
                mRequest.getMaxResultCount(),
                mRequest.getOffset(),
                mRequest.getSkipCalcDbRows() ? nullptr : &totalCount,
                mRequest.getClubsOrder(),
                mRequest.getOrderMode());

            if (versions != nullptr)
                delete [] versions;

            if (result == Blaze::ERR_OK && mRequest.getIncludeClubTags())
            {
                result = dbc.getDb().getTagsForClubs(mResponse.getClubList());
            }

            dbc.releaseConnection();
            mResponse.setTotalCount(totalCount);

            if (result == Blaze::ERR_OK)
            {
                result = mComponent->filterClubPasswords(mResponse.getClubList());
            }

            if (result == Blaze::ERR_OK)
            {
                ClubList &list = mResponse.getClubList();
                ClubList::iterator it = list.begin();
                EntityIdList opponentIdList;
                IdentityInfoByEntityIdMap opponentIdToNameMap;
                for (; it != list.end(); it++)
                {
                    mComponent->getClubOnlineStatusCounts((*it)->getClubId(), (*it)->getClubInfo().getMemberOnlineStatusCounts());
                    opponentIdList.push_back((*it)->getClubInfo().getLastOppo());
                }
                
                // ignore error here as the club's last opponent club may have been disbanded
                // even if we ran into an error resolving the opponent club name it should not
                // fail this RPC
                result = gIdentityManager->getIdentities(ENTITY_TYPE_CLUB, 
                    opponentIdList, opponentIdToNameMap);

                if (result == Blaze::ERR_OK)
                {
                    for (it = list.begin(); it != list.end(); it++)
                    {
                        IdentityInfoByEntityIdMap::const_iterator identItr;
                        identItr = opponentIdToNameMap.find((*it)->getClubInfo().getLastOppo());
                        if (identItr != opponentIdToNameMap.end())
                        {
                            (*it)->getClubInfo().setLastOppoName(identItr->second->getIdentityName());
                        }
                    }
                }
                
                // it is ok if the opponent club is not found because it may have been removed
                if (result == Blaze::CLUBS_ERR_INVALID_CLUB_ID)
                    result = Blaze::ERR_OK;
                
            }
            else if (result != Blaze::CLUBS_ERR_INVALID_CLUB_ID)
            {
                ERR_LOG("[GetClubsCommand] Database error (" << result << ")");
            }
            
            return commandErrorFromBlazeError(result);            
        }

    };
    // static factory method impl
    DEFINE_GETCLUBS_CREATE()

} // Clubs
} // Blaze
