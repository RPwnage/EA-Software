/*************************************************************************************************/
/*!
    \file   getpetitions_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetPetitionsCommand

    Get petitions sent to or from a particular user

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/getpetitions_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

    class GetPetitionsCommand : public GetPetitionsCommandStub, private ClubsCommandBase
    {
    public:
        GetPetitionsCommand(Message* message, GetPetitionsRequest* request, ClubsSlaveImpl* componentImpl)
            : GetPetitionsCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~GetPetitionsCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        GetPetitionsCommandStub::Errors execute() override
        {
            TRACE_LOG("[GetPetitionsCommand] start");

            ClubMessageListMap clubMessageListMap;

            ClubIdList clubIdList;
            clubIdList.push_back(mRequest.getClubId());

            clubMessageListMap[mRequest.getClubId()] = &(mResponse.getClubPetitionsList());

            Blaze::BlazeRpcError result = mComponent->getPetitionsForClubs( clubIdList,
                                                                            mRequest.getPetitionsType(),
                                                                            mRequest.getSortType(),
                                                                            clubMessageListMap);

            clubMessageListMap.clear();
            return (Errors)result;
        }
    };

    // static factory method impl
    DEFINE_GETPETITIONS_CREATE()

} // Clubs
} // Blaze
