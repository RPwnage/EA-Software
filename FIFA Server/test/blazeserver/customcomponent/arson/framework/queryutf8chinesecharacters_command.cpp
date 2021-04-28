
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
#include "arson/rpc/arsonslave/queryutf8chinesecharacters_stub.h"

namespace Blaze
{
namespace Arson
{
class QueryUtf8ChineseCharactersCommand : public QueryUtf8ChineseCharactersCommandStub
{
public:
    QueryUtf8ChineseCharactersCommand(Message* message, ArsonSlaveImpl* componentImpl)
        : QueryUtf8ChineseCharactersCommandStub(message),
        mComponent(componentImpl)
    {
    }

    // Helpers
    BlazeRpcError fetch_utf8_chinese_characters_query();

    ~QueryUtf8ChineseCharactersCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    static const int64_t max64BitInt = 9223372036854775807;

    QueryUtf8ChineseCharactersCommandStub::Errors execute() override
    {
        // Query the integer using getInt() instead of getInt64()
        BlazeRpcError fetchErr = fetch_utf8_chinese_characters_query();

        if(ERR_OK == fetchErr)
            return ERR_OK;
        else
            return ERR_SYSTEM;
    }
};

DEFINE_QUERYUTF8CHINESECHARACTERS_CREATE()


BlazeRpcError QueryUtf8ChineseCharactersCommand::fetch_utf8_chinese_characters_query()
{
    BlazeRpcError rc = Blaze::ERR_SYSTEM;

    // Query the DB
    DbConnPtr conn = gDbScheduler->getReadConnPtr(mComponent->getDbId());
    if (conn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        if (query != nullptr)
        {
            // Manually inserted row with id=0.  See blazeserver\customcomponent\arson\db\mysql\arson.sql
            query->append("SELECT * FROM arson_test_data WHERE id=0");
            DbResultPtr dbResult;
            BlazeRpcError dbError = conn->executeQuery(query, dbResult);

            if (dbError == DB_ERR_OK)
            {
                rc = Blaze::ERR_DB_NOT_SUPPORTED;
                
                DbResult::const_iterator itr = dbResult->begin();
                DbResult::const_iterator end = dbResult->end();
                for (; itr != end; ++itr)
                {
                    const DbRow* row = *itr;
                    if (blaze_strcmp(row->getString("data1"), "退出遊戲") == 0)
                    {
                        rc = Blaze::ERR_OK;
                        break;
                    }
                }
            }
            else
            {
                // query failed
                ERR_LOG("[QueryUtf8ChineseCharactersCommand].fetch_utf8_chinese_characters_query: Query failed");
            }
        }
        else
        {
            // failed to create query
            ERR_LOG("[QueryUtf8ChineseCharactersCommand].fetch_utf8_chinese_characters_query: Failed to create query");
        }
    }
    else
    {
        // failed to obtain connection
        ERR_LOG("[QueryUtf8ChineseCharactersCommand].fetch_utf8_chinese_characters_query: Failed to obtain connection");
    }

    return rc;
}

} //Arson
} //Blaze
