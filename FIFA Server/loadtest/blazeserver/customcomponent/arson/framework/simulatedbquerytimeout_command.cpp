/*************************************************************************************************/
/*!
    \file   simulatedbquerytimeout_command.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"

// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/simulatedbquerytimeout_stub.h"

namespace Blaze
{
namespace Arson
{
class SimulateDbQueryTimeoutCommand : public SimulateDbQueryTimeoutCommandStub
{
public:
    SimulateDbQueryTimeoutCommand(Message* message, ArsonSlaveImpl* componentImpl)
        : SimulateDbQueryTimeoutCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~SimulateDbQueryTimeoutCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    SimulateDbQueryTimeoutCommandStub::Errors execute() override
    {
        TimeValue tv(1);
        DbConnPtr dbConn = gDbScheduler->getReadConnPtr(gUserSessionManager->getDbId());

        QueryPtr dbQuery = DB_NEW_QUERY_PTR(dbConn);
        dbQuery->append("SELECT * FROM `accountinfo`");

        DbResultPtr dbResult;
        BlazeRpcError error = dbConn->executeQuery(dbQuery, dbResult, tv);

        if (error == DB_ERR_TIMEOUT)
            return ARSON_ERR_DB_QUERY_TIMEOUT;

        return commandErrorFromBlazeError(error);
    }
};

DEFINE_SIMULATEDBQUERYTIMEOUT_CREATE()


} //Arson
} //Blaze
