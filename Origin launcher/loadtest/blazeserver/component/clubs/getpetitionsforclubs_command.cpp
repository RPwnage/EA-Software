/*************************************************************************************************/
/*!
    \file   getpetitionsforclubs_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetPetitionsForClubsCommand

    Get petitions for multiple clubs sent to or from a particular user

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/getpetitionsforclubs_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

    class GetPetitionsForClubsCommand : public GetPetitionsForClubsCommandStub, private ClubsCommandBase
    {
    public:
        GetPetitionsForClubsCommand(Message* message, GetPetitionsForClubsRequest* request, ClubsSlaveImpl* componentImpl)
            : GetPetitionsForClubsCommandStub(message, request),
            ClubsCommandBase(componentImpl)
        {
        }

        ~GetPetitionsForClubsCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        GetPetitionsForClubsCommandStub::Errors execute() override
        {
            TRACE_LOG("[GetPetitionsForClubsCommand] start");

            Blaze::BlazeRpcError result = mComponent->getPetitionsForClubs( mRequest.getClubIdList(),
                                                                                mRequest.getPetitionsType(),
                                                                                mRequest.getSortType(),
                                                                                mResponse.getClubPetitionListMap());

            return (Errors)result;         
        }
    };

    // static factory method impl
    DEFINE_GETPETITIONSFORCLUBS_CREATE()

} // Clubs
} // Blaze
