
/*** Include Files *******************************************************************************/

// framework includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/database/dbrow.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/query.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/query64bitint_stub.h"

namespace Blaze
{
namespace Arson
{
class Query64BitIntCommand : public Query64BitIntCommandStub
{
public:
    Query64BitIntCommand(Message* message, Blaze::Arson::Query64BitIntRequest* request, ArsonSlaveImpl* componentImpl)
        : Query64BitIntCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    // Helpers
    BlazeRpcError insert_id_query();
    BlazeRpcError fetch_id_query();

    ~Query64BitIntCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    static const int64_t max64BitInt = 9223372036854775807;

    Query64BitIntCommandStub::Errors execute() override
    {
        if (mRequest.getExecuteOnArsonMaster())
        {
            return commandErrorFromBlazeError(mComponent->getMaster()->query64BitIntMaster());
        }

        // Insert a 64bit integer into arson_test_data
        BlazeRpcError insertErr = insert_id_query();
        // Query the integer using getInt() instead of getInt64()
        BlazeRpcError fetchErr = fetch_id_query();

        if(ERR_OK == insertErr && ERR_OK == fetchErr)
            return ERR_OK;
        else
            return ERR_SYSTEM;
    }
};

DEFINE_QUERY64BITINT_CREATE()


BlazeRpcError Query64BitIntCommand::insert_id_query()
{
    BlazeRpcError rc = Blaze::ERR_SYSTEM;

    // Query the DB
    DbConnPtr conn = gDbScheduler->getConnPtr(mComponent->getDbId());
    if (conn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        if (query != nullptr)
        {
            query->append("INSERT INTO arson_test_data VALUES ('9223372036854775807','datafield 1','datafield 2','datafield 3')");
            BlazeRpcError dbError = conn->executeQuery(query);

            if (dbError == DB_ERR_OK)
                rc = Blaze::ERR_OK;
            else
            {
                // query failed
                ERR_LOG("[Query64BitIntCommand].insert_id_query: Query failed");
            }
        }
        else
        {
            // failed to create query
            ERR_LOG("[Query64BitIntCommand].insert_id_query: Failed to create query");
        }
    }
    else
    {
        // failed to obtain connection
        ERR_LOG("[Query64BitIntCommand].insert_id_query: Failed to obtain connection");
    }

    return rc;
}

BlazeRpcError Query64BitIntCommand::fetch_id_query()
{
    BlazeRpcError rc = Blaze::ERR_SYSTEM;

    // Query the DB
    DbConnPtr conn = gDbScheduler->getReadConnPtr(mComponent->getDbId());
    if (conn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        if (query != nullptr)
        {
            query->append("SELECT * FROM arson_test_data");
            DbResultPtr dbResult;
            BlazeRpcError dbError = conn->executeQuery(query, dbResult);

            if (dbError == DB_ERR_OK)
            {
                rc = Blaze::ERR_OK;
                
                DbResult::const_iterator itr = dbResult->begin();
                DbResult::const_iterator end = dbResult->end();
                for (; itr != end; ++itr)
                {
                    const DbRow* row = *itr;
                    row->getInt("id");                  
                }
            }
            else
            {
                // query failed
                ERR_LOG("[Query64BitIntCommand].insert_id_query: Query failed");
            }
        }
        else
        {
            // failed to create query
            ERR_LOG("[Query64BitIntCommand].insert_id_query: Failed to create query");
        }
    }
    else
    {
        // failed to obtain connection
        ERR_LOG("[Query64BitIntCommand].insert_id_query: Failed to obtain connection");
    }

    return rc;
}

} //Arson
} //Blaze
