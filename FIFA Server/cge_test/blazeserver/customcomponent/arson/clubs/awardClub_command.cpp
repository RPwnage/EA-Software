/*************************************************************************************************/
/*!
    \file   awardclub_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class AwardClubCommand

    \brief
    Invokes awardClub on clubs component.

    \note
    Used for testing.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// global includes
#include "framework/blaze.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/awardclub_stub.h"

#include "clubs/clubsslaveimpl.h"
#include "framework/controller/controller.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"

namespace Blaze
{
namespace Arson
{

    class AwardClubCommand : public AwardClubCommandStub
    {
    public:
        AwardClubCommand(Message* message, AwardClubRequest *request, ArsonSlaveImpl* componentImpl)
            : AwardClubCommandStub(message, request)
        {
        }

        ~AwardClubCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        AwardClubCommandStub::Errors execute() override
        {
            TRACE_LOG("[AwardClubCommand].start()");

            Blaze::Clubs::ClubsSlaveImpl *component = (Blaze::Clubs::ClubsSlaveImpl *) gController->getComponent(Blaze::Clubs::ClubsSlave::COMPONENT_ID);           
            DbConnPtr dbConn = gDbScheduler->getConnPtr(component->getDbId());
            if (dbConn == nullptr)
                return ERR_SYSTEM;

            BlazeRpcError dbError = dbConn->startTxn();
            if (dbError != DB_ERR_OK)
            {
                ERR_LOG("[AwardClubCommand] - database transaction was not successful");
                return ERR_SYSTEM;
            }           

            Clubs::ClubIdList clubsToLock;
            clubsToLock.push_back(mRequest.getClubId());

            BlazeRpcError err = component->lockClubs(dbConn, &clubsToLock);

            if (err != Blaze::ERR_OK)
            {
                ERR_LOG("[AwardClubCommand] - lockClubs attempt not successful");
                dbConn->rollback();
                return static_cast<Errors>(err);
            }
            
            err = component->awardClub(dbConn, mRequest.getClubId(), mRequest.getAwardId());         

            if (err != Blaze::ERR_OK)
            {
                ERR_LOG("[AwardClubCommand] - awardClub attempt not successful");
                dbConn->rollback();
                return static_cast<Errors>(err);
            }

            dbError = dbConn->commit();
            if (dbError != DB_ERR_OK)
            {
                ERR_LOG("[AwardClubCommand] - database transaction was not successful");
                return ERR_SYSTEM;
            }

            return ERR_OK;
        }

    private:

    };

    // static factory method impl
    DEFINE_AWARDCLUB_CREATE()


} // Arson
} // Blaze
