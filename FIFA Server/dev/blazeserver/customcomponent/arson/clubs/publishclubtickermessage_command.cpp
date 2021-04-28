/*************************************************************************************************/
/*!
    \file   publishclubtickermessage_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class PublishClubTickerMessage

    \brief
    Publish club ticker message

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
#include "arson/rpc/arsonslave/publishclubtickermessage_stub.h"
#include "clubs/clubsslaveimpl.h"
#include "framework/controller/controller.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"

namespace Blaze
{
namespace Arson
{

    class PublishClubTickerMessageCommand : public PublishClubTickerMessageCommandStub
    {
    public:
        PublishClubTickerMessageCommand(Message* message, PublishClubTickerMessageRequest *request, ArsonSlaveImpl* componentImpl)
            : PublishClubTickerMessageCommandStub(message, request)
        {
        }

        ~PublishClubTickerMessageCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        PublishClubTickerMessageCommandStub::Errors execute() override
        {
            TRACE_LOG("[PublishClubTickerMessage].start()");

            Blaze::Clubs::ClubsSlaveImpl *component = (Blaze::Clubs::ClubsSlaveImpl *) gController->getComponent(Blaze::Clubs::ClubsSlave::COMPONENT_ID);           
            DbConnPtr dbConn = gDbScheduler->getConnPtr(component->getDbId());
            if (dbConn == nullptr)
                return ERR_SYSTEM;

            BlazeRpcError dbError = dbConn->startTxn();
            if (dbError != DB_ERR_OK)
            {
                ERR_LOG("[PublishClubTickerMessage] - database transaction was not successful");
                return ERR_SYSTEM;
            }           

            Blaze::Clubs::ClubsDatabase db;
            db.setDbConn(dbConn);
         
            BlazeRpcError err = component->publishClubTickerMessage(
                &db,
                true,
                mRequest.getClubId(),
                mRequest.getIncludeUserId(),
                mRequest.getExcludeUserId(),
                mRequest.getMessage().getMetadata(),
                mRequest.getMessage().getText(),
                mRequest.getParams(),
                nullptr,
                nullptr,
                nullptr,
                nullptr);

            if (err != Blaze::ERR_OK)
            {
                ERR_LOG("[PublishClubTickerMessage] - reportResults attempt not successful");
                dbConn->rollback();
                return static_cast<Errors>(err);
            }

            dbError = dbConn->commit();
            if (dbError != DB_ERR_OK)
            {
                ERR_LOG("[PublishClubTickerMessage] - database transaction was not successful");
                return ERR_SYSTEM;
            }

            return ERR_OK;
        }

    private:

    };

    // static factory method impl
    DEFINE_PUBLISHCLUBTICKERMESSAGE_CREATE()



} // Arson
} // Blaze
