/*************************************************************************************************/
/*!
    \file   reportresulttoclubs_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ReportResultToClubsCommand

    \brief
    Invoked report result on clubs command. 

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
#include "arson/rpc/arsonslave/reportresulttoclubs_stub.h"
#include "clubs/clubsslaveimpl.h"
#include "framework/controller/controller.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"

namespace Blaze
{
namespace Arson
{

    class ReportResultToClubsCommand : public ReportResultToClubsCommandStub
    {
    public:
        ReportResultToClubsCommand(Message* message, ReportResultToClubsRequest *request, ArsonSlaveImpl* componentImpl)
            : ReportResultToClubsCommandStub(message, request)
        {
        }

        ~ReportResultToClubsCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        ReportResultToClubsCommandStub::Errors execute() override
        {
            TRACE_LOG("[ReportResultToClubsCommand].start()");

            Blaze::Clubs::ClubsSlaveImpl *component = (Blaze::Clubs::ClubsSlaveImpl *) gController->getComponent(Blaze::Clubs::ClubsSlave::COMPONENT_ID);           
            DbConnPtr dbConn = gDbScheduler->getConnPtr(component->getDbId());
            if (dbConn == nullptr)
                return ERR_SYSTEM;

            BlazeRpcError dbError = dbConn->startTxn();
            if (dbError != DB_ERR_OK)
            {
                ERR_LOG("[ReportResultToClubsCommand] - database transaction was not successful");
                return ERR_SYSTEM;
            }           

            Clubs::ClubIdList clubsToLock;
            clubsToLock.push_back(mRequest.getUpperClubId());
            clubsToLock.push_back(mRequest.getLowerClubId());

            BlazeRpcError err = component->lockClubs(dbConn, &clubsToLock);

            if (err != Blaze::ERR_OK)
            {
                ERR_LOG("[ReportResultToClubsCommand] - lockClubs attempt not successful");
                dbConn->rollback();
                return static_cast<Errors>(err);
            }
            
            err = component->reportResults(
                dbConn,
                mRequest.getUpperClubId(),
                mRequest.getLastResultUpper(),
                mRequest.getLowerClubId(),
                mRequest.getLastResultLower());         

            if (err != Blaze::ERR_OK)
            {
                ERR_LOG("[ReportResultToClubsCommand] - reportResults attempt not successful");
                dbConn->rollback();
                return static_cast<Errors>(err);
            }

            dbError = dbConn->commit();
            if (dbError != DB_ERR_OK)
            {
                ERR_LOG("[ReportResultToClubsCommand] - database transaction was not successful");
                return ERR_SYSTEM;
            }

            return ERR_OK;
        }

    private:

    };

    // static factory method impl
    DEFINE_REPORTRESULTTOCLUBS_CREATE()



} // Arson
} // Blaze
