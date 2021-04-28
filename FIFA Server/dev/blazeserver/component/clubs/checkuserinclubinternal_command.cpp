/*************************************************************************************************/
/*!
    \file   checkuserinclubinternal_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class CheckUserInClubInternal

    Check if user is in a club.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/checkuserinclubinternal_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"

namespace Blaze
{
namespace Clubs
{

class CheckUserInClubInternalCommand : public CheckUserInClubInternalCommandStub, private ClubsCommandBase
{
public:
    CheckUserInClubInternalCommand(Message* message, CheckUserInClubRequest* request, ClubsSlaveImpl* componentImpl)
        : CheckUserInClubInternalCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~CheckUserInClubInternalCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    CheckUserInClubInternalCommandStub::Errors execute() override
    {
        if (mRequest.getBlazeId() == INVALID_BLAZE_ID)
        {
            return CheckUserInClubInternalCommandStub::CLUBS_ERR_INVALID_USER_ID;
        }

        BlazeRpcError result = Blaze::ERR_OK;

        TransactionContextPtr ptr = mComponent->getTransactionContext(mRequest.getTransactionContextId());
        if (ptr == nullptr)
        {
            TRACE_LOG("[CheckUserInClubInternalCommand] cannot obtain transaction for transaction context id[" << mRequest.getTransactionContextId() <<"].");
            return ERR_SYSTEM;
        }

        DbConnPtr dbPtr = static_cast<ClubsTransactionContext&>(*ptr).getDatabaseConnection(); 

        Clubs::ClubsComponentSettings settings;
        result = mComponent->getClubsComponentSettings(settings);

        if (result == Blaze::ERR_OK)
        {
            result = Blaze::CLUBS_ERR_USER_NOT_MEMBER;

            Clubs::ClubsDatabase clubsDb;
            clubsDb.setDbConn(dbPtr);

            Clubs::ClubDomainList::const_iterator it = settings.getDomainList().begin();                
            for (; it != settings.getDomainList().end(); ++it)
            {
                Clubs::ClubDomainId domainId = (*it)->getClubDomainId();

                Clubs::ClubMembershipList clubMembershipList;
                if (clubsDb.getClubMembershipsInDomain(
                    domainId, 
                    mRequest.getBlazeId(),
                    clubMembershipList) == Blaze::ERR_OK)
                {
                    // user is a club member
                    result = Blaze::ERR_OK;
                    break;
                }
            }
        }

        return commandErrorFromBlazeError(result);
    }

};
// static factory method impl
DEFINE_CHECKUSERINCLUBINTERNAL_CREATE()

} // Clubs
} // Blaze
