/*************************************************************************************************/
/*!
    \file   updateclubrecordbook_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateClubRecordbookCommand

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
#include "arson/rpc/arsonslave/updateclubrecordbook_stub.h"

#include "clubs/clubsslaveimpl.h"
#include "framework/controller/controller.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"

namespace Blaze
{
namespace Arson
{

    class UpdateClubRecordbookCommand : public UpdateClubRecordbookCommandStub
    {
    public:
        UpdateClubRecordbookCommand(Message* message, UpdateClubRecordbookRequest *request, ArsonSlaveImpl* componentImpl)
            : UpdateClubRecordbookCommandStub(message, request)
        {
        }

        ~UpdateClubRecordbookCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        UpdateClubRecordbookCommandStub::Errors execute() override
        {
            TRACE_LOG("[UpdateClubRecordbookCommand].start()");

            Blaze::Clubs::ClubsSlaveImpl *component = (Blaze::Clubs::ClubsSlaveImpl *) gController->getComponent(Blaze::Clubs::ClubsSlave::COMPONENT_ID);           
            DbConnPtr dbConn = gDbScheduler->getConnPtr(component->getDbId());
            if (dbConn == nullptr)
                return ERR_SYSTEM;

            BlazeRpcError dbError = dbConn->startTxn();
            if (dbError != DB_ERR_OK)
            {
                ERR_LOG("[UpdateClubRecordbookCommand] - database transaction was not successful");
                return ERR_SYSTEM;
            }           

            Clubs::ClubIdList clubsToLock;
            clubsToLock.push_back(mRequest.getClubId());

            BlazeRpcError err = component->lockClubs(dbConn, &clubsToLock);

            if (err != Blaze::ERR_OK)
            {
                ERR_LOG("[UpdateClubRecordbookCommand] - lockClubs attempt not successful");
                dbConn->rollback();
                return static_cast<Errors>(err);
            }
            
            Clubs::ClubRecordbookRecord record;
            record.blazeId = mRequest.getRecordHolder();
            record.clubId = mRequest.getClubId();
            record.recordId = mRequest.getRecordId();
            record.statType = mRequest.getStatType();
            switch(record.statType)
            {
            case Clubs::CLUBS_RECORD_STAT_INT:
                {
                    sscanf(mRequest.getStatValue(), "%" SCNd64, &record.statValueInt);
                    break;
                }
            case Clubs::CLUBS_RECORD_STAT_FLOAT:
                {
                    record.statValueFloat = static_cast<float_t>(atof(mRequest.getStatValue()));
                    break;
                }
            }
            err = component->updateClubRecord(dbConn, record);         

            if (err != Blaze::ERR_OK)
            {
                ERR_LOG("[UpdateClubRecordbookCommand] - reportResults attempt not successful");
                dbConn->rollback();
                return static_cast<Errors>(err);
            }

            dbError = dbConn->commit();
            if (dbError != DB_ERR_OK)
            {
                ERR_LOG("[UpdateClubRecordbookCommand] - database transaction was not successful");
                return ERR_SYSTEM;
            }

            return ERR_OK;
        }

    private:

    };

    // static factory method impl
    DEFINE_UPDATECLUBRECORDBOOK_CREATE()



} // Arson
} // Blaze
