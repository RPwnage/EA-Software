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
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/simulatestreamingdbquery_stub.h"

namespace Blaze
{
namespace Arson
{
class SimulateStreamingDbQueryCommand : public SimulateStreamingDbQueryCommandStub
{
public:
    SimulateStreamingDbQueryCommand(Message* message, ArsonSlaveImpl* componentImpl)
        : SimulateStreamingDbQueryCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~SimulateStreamingDbQueryCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    SimulateStreamingDbQueryCommand::Errors execute() override
    {
        DbConnPtr dbConn = gDbScheduler->getReadConnPtr(mComponent->getDbId());
        
        QueryPtr dbQuery = DB_NEW_QUERY_PTR(dbConn);
        dbQuery->append("SELECT * FROM blaze_schema_info");
        StreamingDbResultPtr streamingDbResult;
        uint32_t streamingCount = 0;
        
        //Execute streaming query 
        BlazeRpcError error = dbConn->executeStreamingQuery(dbQuery,streamingDbResult);

        if(error == DB_ERR_OK)
        {
            for (DbRow* row = streamingDbResult->next(); row != nullptr; row = streamingDbResult->next())
            {
                ++streamingCount;

                // there is no garbage collection for streaming db results, so ...
                delete row;
            }
        }
        mResponse.setNoRows(streamingCount);
       
        return commandErrorFromBlazeError(error);
    }
};

DEFINE_SIMULATESTREAMINGDBQUERY_CREATE();


} //Arson
} //Blaze
