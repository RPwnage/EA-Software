#include "framework/blaze.h"
#include "arson/rpc/arsonslave/querydb_stub.h"
#include "arson/tdf/arson.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/query.h"
#include "framework/database/dbconn.h"
#include "framework/database/mysql/mysqlrow.h"
#include "framework/controller/controller.h"
#include "framework/connection/selector.h"
#include "framework/usersessions/usersession.h"

#include "arsonslaveimpl.h"

namespace Blaze
{

namespace Arson
{

class QueryDbCommand : public QueryDbCommandStub
{
public:
    QueryDbCommand(Message* message, QueryRequest *req, ArsonSlaveImpl* componentImp)
        : QueryDbCommandStub(message, req)
    {
    }

    ~QueryDbCommand() override { }

    QueryDbCommandStub::Errors execute() override
    {
        Errors rc = ERR_OK;

        TRACE_LOG("[QueryDbCommand].start: starting query on db '" << mRequest.getDbName() << "': " << mRequest.getQuery());
        uint32_t dbId = gDbScheduler->getDbId(mRequest.getDbName());
        DbConnPtr conn = mRequest.getReadOnly() ? gDbScheduler->getReadConnPtr(dbId) : gDbScheduler->getConnPtr(dbId);
        if (conn != nullptr)
        {
            BlazeRpcError error = DB_ERR_OK;
            if (mRequest.getTransaction())
            {
                error = conn->startTxn();
                if (error != DB_ERR_OK)
                {
                    ERR_LOG("[QueryDbCommand].start: startTxn failed: " << getDbErrorString(error));
                    rc = ARSON_ERR_DB;
                }
            }

            if (error == DB_ERR_OK)
            {
                rc = ERR_OK;
                for(int32_t idx = 0; (idx < mRequest.getCount()) && (rc == ERR_OK); ++idx)
                {
                    if (mRequest.getIsStreaming())
                    {
                        Fiber::CreateParams params(Fiber::STACK_MEDIUM);
                        gSelector->scheduleFiberCall(this, &QueryDbCommand::executeStreamingQuery, dbId, mRequest.getQuery(), "QueryDbCommand::executeStreamingQuery", params);
                    }
                    else
                    {
                        QueryPtr query = DB_NEW_QUERY_PTR(conn);

                        // Perhaps a verbatim string isn't needed here...
                        // perhaps string checks and destructive keywords wouldn't be a problem...
                        // Since this is just for Arson, we'll let anything go here. 
                        query->appendVerbatimString(mRequest.getQuery(), true /*skipStringChecks*/, true /*allowDestructiveKeywords*/);

                        DbResultPtrs result;
                        BlazeRpcError err = conn->executeMultiQuery(query, result);
                        if (err != DB_ERR_OK)
                        {
                            ERR_LOG("[QueryDbCommand].start: query failed: " << getDbErrorString(err));
                            rc = ARSON_ERR_DB;
                        }
                        else
                        {
                            rc = ERR_OK;
                        }
                    }
                }

                if (mRequest.getTransaction())
                {
                    if (error == DB_ERR_OK)
                        error = conn->commit();
                    else
                        error = conn->rollback();
                    if (error != DB_ERR_OK)
                    {
                        ERR_LOG("[QueryDbCommand].start: commit/rollback failed: " << getDbErrorString(error));
                        rc = ARSON_ERR_DB;
                    }
                }

            }
        }
        else
        {
            rc = ARSON_ERR_DB;
        }

        if (mRequest.getAccessUserSession())
        {
            TRACE_LOG("user session: " << gCurrentLocalUserSession->getUserInfo());
        }

        TRACE_LOG("[QueryDbCommand].start: completed request.");

        return rc;
    }

    void executeStreamingQuery(uint32_t dbId, const char8_t* queryText)
    {
        DbConnPtr conn = gDbScheduler->getReadConnPtr(dbId);
        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        query->append(queryText);
        StreamingDbResultPtr result;
        BlazeRpcError error = conn->executeStreamingQuery(query, result);
        if (error != DB_ERR_OK)
        {
            ERR_LOG("[QueryDbStreamingCommand].executeStreamingQuery: query failed: " << getDbErrorString(error));
        }
        else
        {
            DbRow* row = result->next();
            eastl::string logBuf;
            while (row != nullptr)
            {
                logBuf.clear();
                row->toString(logBuf);
                TRACE_LOG("[QueryDbStreamingCommand].executeStreamingQuery: result: " << logBuf.c_str());
                delete row;
                row = result->next();
            }
        }
    }
};


DEFINE_QUERYDB_CREATE()

} // namespace Arson
} // namespace Blaze

