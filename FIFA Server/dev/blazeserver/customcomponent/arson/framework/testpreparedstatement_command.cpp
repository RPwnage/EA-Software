/*************************************************************************************************/
/*!
    \file   testpreparedstatement_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbscheduler.h"

// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/testpreparedstatement_stub.h"
#include "arson/rpc/arsonslave/testadddbentry_stub.h"
#include "arson/rpc/arsonslave/testgetdbentry_stub.h"
#include "arson/rpc/arsonslave/getuserdata_stub.h"
#include "arson/rpc/arsonslave/deactivateuserdata_stub.h"

namespace Blaze
{
namespace Arson
{
class TestPreparedStatementCommand : public TestPreparedStatementCommandStub
{
public:
    TestPreparedStatementCommand(
        Message* message, ArsonSlaveImpl* componentImpl)
        : TestPreparedStatementCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~TestPreparedStatementCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    Blaze::DbConnPtr mDbConn;
    
    TestPreparedStatementCommandStub::Errors saveTestData(int64_t id, const char8_t* data1, const char8_t* data2, const char8_t* data3)
    {
        // Build the query to save test data
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mComponent->getQueryId(ArsonSlaveImpl::TEST_DATA_SAVE));
        stmt->setInt64(0, id);
        stmt->setString(1, data1);
        stmt->setString(2, data2);
        stmt->setString(3, data3);
        stmt->setString(4, data1);
        stmt->setString(5, data2);
        stmt->setString(6, data3);

        eastl::string strQuery;
        TRACE_LOG("[TestPreparedStatementCommand].saveTestData: Query -- " << stmt->toString(strQuery));

        DbResultPtr dbResult;
        BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbResult);
        if (error != DB_ERR_OK)
        {
            ERR_LOG("[TestPreparedStatementCommand].saveTestData:"
                " Failed to write record id(" << id << "). (quit)");
            return ERR_SYSTEM;
        }
        else
        {
            TRACE_LOG("[TestPreparedStatementCommand].saveTestData: Success! (quit)");
        } // if

        return ERR_OK;
    }

    TestPreparedStatementCommandStub::Errors loadTestDataSingleRow(int64_t id)
    {
        // Build the query to load test data in single row
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mComponent->getQueryId(ArsonSlaveImpl::TEST_DATA_LOAD_SINGLE_ROW));
        stmt->setInt64(0, id);

        eastl::string strQuery;
        TRACE_LOG("[TestPreparedStatementCommand].loadTestDataSingleRow: Query -- " << stmt->toString(strQuery));

        DbResultPtr dbResult;
        BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbResult);
        if (error != DB_ERR_OK)
        {
            ERR_LOG("[TestPreparedStatementCommand].loadTestDataSingleRow:"
                " Failed to load record id(" << id << "). (quit)");
            return ERR_SYSTEM;
        }
        else
        {
            TRACE_LOG("[TestPreparedStatementCommand].loadTestDataSingleRow: Success! (quit)");
        } // if

        return ERR_OK;
    }

    TestPreparedStatementCommandStub::Errors loadTestDataMultiRow(int64_t id1, int64_t id2)
    {
        // Build the query to load test data in multiple rows between id1 and id2
        PreparedStatement* stmt = mDbConn->getPreparedStatement(mComponent->getQueryId(ArsonSlaveImpl::TEST_DATA_LOAD_MULTI_ROW));
        stmt->setInt64(0, id1);
        stmt->setInt64(1, id2);

        eastl::string strQuery;
        TRACE_LOG("[TestPreparedStatementCommand].loadTestDataMultiRow: Query -- " << stmt->toString(strQuery));

        DbResultPtr dbResult;
        BlazeRpcError error = mDbConn->executePreparedStatement(stmt, dbResult);
        if (error != DB_ERR_OK)
        {
            ERR_LOG("[TestPreparedStatementCommand].loadTestDataMultiRow:"
                " Failed to load records between id1(" << id1 << ") and id2(" << id2 << "). (quit)");
            return ERR_SYSTEM;
        }
        else
        {
            TRACE_LOG("[TestPreparedStatementCommand].loadTestDataMultiRow: Success! (quit)");
        } // if

        return ERR_OK;
    }

    TestPreparedStatementCommandStub::Errors execute() override
    {
        // Test data strings table
        // 100      1024    1500
        // 1024     1025    1501
        // 1200     1500    1800
        // 1500     1501    2000

        const char8_t* str100 = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
        const char8_t* str1024 = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234";
        const char8_t* str1025 = "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345";
        const char8_t* str1200 = "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
        const char8_t* str1500 = "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
        const char8_t* str1501 = "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901";
        const char8_t* str1800 = "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
        const char8_t* str2000 = "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
        
        // Get a database connection.
        mDbConn = gDbScheduler->getConnPtr(mComponent->getDbId());
        if (mDbConn == nullptr)
        {
            ERR_LOG("[TestPreparedStatementCommand].execute: Failed to obtain a connection. (quit)");
            return ERR_SYSTEM;
        } // if

        Errors result = ERR_OK;

        // save test data row 1 with three strings with 100, 1024, and 1500 characters
        int64_t id1 = 1;
        result = saveTestData(id1, str100, str1024, str1500);
        if(result != ERR_OK)
            return result;

        // load test data row 1
        result = loadTestDataSingleRow(id1);
        if(result != ERR_OK)
            return result;

        // save test data row 2 with three strings with 1024, 1025, and 1501 characters
        int64_t id2 = 2;
        result = saveTestData(id2, str1024, str1025, str1501);
        if(result != ERR_OK)
            return result;

        // save test data row 3 with three strings with 1200, 1500, and 1800 characters
        int64_t id3 = 3;
        result = saveTestData(id3, str1200, str1500, str1800);
        if(result != ERR_OK)
            return result;

        // load test data row 2 and 3
        result = loadTestDataMultiRow(id2, id3);
        if(result != ERR_OK)
            return result;

        // save test data row 4 with three strings with 1500, 1501, and 2000 characters
        int64_t id4 = 4;
        result = saveTestData(id4, str1500, str1501, str2000);
        if(result != ERR_OK)
            return result;

        // load test data row 1, 2, 3 and 4
        result = loadTestDataMultiRow(id1, id4);
        if(result != ERR_OK)
            return result;

        return ERR_OK;
    }
};

DEFINE_TESTPREPAREDSTATEMENT_CREATE();

class TestAddDbEntryCommand : public TestAddDbEntryCommandStub
{
public:
    TestAddDbEntryCommand(
        Message* message, TestDbEntry* request,  ArsonSlaveImpl* componentImpl)
        : TestAddDbEntryCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~TestAddDbEntryCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    TestAddDbEntryCommandStub::Errors execute() override
    {
        // Get a database connection.
        Blaze::DbConnPtr dbConn = gDbScheduler->getConnPtr(mComponent->getDbId());
        if (dbConn == nullptr)
        {
            ERR_LOG("[TestAddDbEntryCommand].execute: Failed to obtain a connection. (quit)");
            return ERR_SYSTEM;
        }

        // Build the query to save test data
        PreparedStatement* stmt = dbConn->getPreparedStatement(mComponent->getQueryId(ArsonSlaveImpl::TEST_DATA_SAVE));
        stmt->setInt64(0, mRequest.getId());
        stmt->setString(1, mRequest.getData1());
        stmt->setString(2, mRequest.getData2());
        stmt->setString(3, mRequest.getData3());
        stmt->setString(4, mRequest.getData1());
        stmt->setString(5, mRequest.getData2());
        stmt->setString(6, mRequest.getData3());

        eastl::string strQuery;
        TRACE_LOG("[TestAddDbEntryCommand].saveTestData: Query -- " << stmt->toString(strQuery));

        DbResultPtr dbResult;
        BlazeRpcError error = dbConn->executePreparedStatement(stmt, dbResult);
        if (error != DB_ERR_OK)
        {
            ERR_LOG("[TestAddDbEntryCommand].saveTestData:"
                " Failed to write record id(" << mRequest.getId() << "). (quit)");
            return ERR_SYSTEM;
        }
        else
        {
            TRACE_LOG("[TestAddDbEntryCommand].saveTestData: Success! (quit)");
        } // if

        return ERR_OK;
    }
};

DEFINE_TESTADDDBENTRY_CREATE();

class TestGetDbEntryCommand : public TestGetDbEntryCommandStub
{
public:
    TestGetDbEntryCommand(
        Message* message, GetDbEntryRequest* request, ArsonSlaveImpl* componentImpl)
        : TestGetDbEntryCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~TestGetDbEntryCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    TestGetDbEntryCommandStub::Errors execute() override
    {
        // Get a database connection.
        Blaze::DbConnPtr readDbConn = gDbScheduler->getLagFreeReadConnPtr(mComponent->getDbId());
        if (readDbConn == nullptr)
        {
            ERR_LOG("[TestGetDbEntryCommandStub].execute: Failed to obtain a connection. (quit)");
            return ERR_SYSTEM;
        }

            // Build the query to load test data in single row
        PreparedStatement* stmt = readDbConn->getPreparedStatement(mComponent->getQueryId(ArsonSlaveImpl::TEST_DATA_LOAD_SINGLE_ROW));
        stmt->setInt64(0, mRequest.getId());

        eastl::string strQuery;
        TRACE_LOG("[TestGetDbEntryCommandStub].loadTestDataSingleRow: Query -- " << stmt->toString(strQuery));

        DbResultPtr dbResult;
        BlazeRpcError error = readDbConn->executePreparedStatement(stmt, dbResult);
        if (error != DB_ERR_OK)
        {
            ERR_LOG("[TestGetDbEntryCommandStub].loadTestDataSingleRow:"
                " Failed to load record id(" << mRequest.getId() << "). (quit)");
            return ARSON_ERR_DATA_NOT_FOUND;
        }
        else
        {
            if (dbResult->empty())
                return ARSON_ERR_DATA_NOT_FOUND;


            DbResult::const_iterator it = dbResult->begin();
            DbRow *dbRow = *it;
            uint32_t col = 0;

            mResponse.setId(dbRow->getInt64(col++));
            mResponse.setData1(dbRow->getString(col++));
            mResponse.setData2(dbRow->getString(col++));
            mResponse.setData3(dbRow->getString(col++));
        } 

        return ERR_OK;
    }
};

DEFINE_TESTGETDBENTRY_CREATE();

class GetUserDataCommand : public GetUserDataCommandStub
{
public:
    GetUserDataCommand(
        Message* message, Blaze::GdprCompliance::CustomComponentGetDataRequest* request, ArsonSlaveImpl* componentImpl)
        : GetUserDataCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~GetUserDataCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    DbResultPtr getDbEntry(ClientPlatformType platform, int64_t id, bool& fatalError)
    {
        DbResultPtr dbResult;

        // Get a database connection.
        Blaze::DbConnPtr readDbConn = gDbScheduler->getLagFreeReadConnPtr(mComponent->getDbId()); // platform
        if (readDbConn == nullptr)
        {
            ERR_LOG("[GetUserDataCommandStub].execute: Failed to obtain a connection. (quit)");
            fatalError = true;
            return dbResult;
        }

        // Build the query to load test data in single row
        PreparedStatement* stmt = readDbConn->getPreparedStatement(mComponent->getQueryId(ArsonSlaveImpl::TEST_DATA_LOAD_SINGLE_ROW));
        stmt->setInt64(0, id);

        eastl::string strQuery;
        TRACE_LOG("[GetUserDataCommandStub].loadTestDataSingleRow: Query -- " << stmt->toString(strQuery));
        
        BlazeRpcError error = readDbConn->executePreparedStatement(stmt, dbResult);
        if (error != DB_ERR_OK)
        {
            ERR_LOG("[GetUserDataCommandStub].loadTestDataSingleRow:"
                " Failed to load record id(" << id << "). (quit)");
        }
        return dbResult;
    }

    GetUserDataCommandStub::Errors execute() override
    {
        bool fatalError = false;
        DbResultPtr dbResults = getDbEntry(common, (int64_t)mRequest.getAccountId(), fatalError);  
        if (fatalError) 
            return ERR_SYSTEM;

        if (!dbResults->empty())
        {
            Blaze::GdprCompliance::TableDataList* tableDataList = mResponse.getTablesByPlatform().allocate_element();
            mResponse.getTablesByPlatform()[common] = tableDataList;

            Blaze::GdprCompliance::TableData* tableData = tableDataList->pull_back();
            tableData->setTableDesc("Arson Custom Table");
            Blaze::GdprCompliance::RowPtr descRow = tableData->getRows().pull_back();     // First RowList must be the descriptions:
            descRow->getRowData().push_back("PII Data One");
            descRow->getRowData().push_back("Non-PII Data Two");
            descRow->getRowData().push_back("PII Data Three");

            DbResult::const_iterator it = dbResults->begin();
            DbRow *dbRow = *it;

            Blaze::GdprCompliance::RowPtr dataRow = tableData->getRows().pull_back();     // Second and later rows are the values:
            dataRow->getRowData().push_back(dbRow->getString("data1"));
            dataRow->getRowData().push_back(dbRow->getString("data2"));
            dataRow->getRowData().push_back(dbRow->getString("data3"));
        }

        for (auto& platformIds : mRequest.getPlatformMap())
        {
            dbResults = getDbEntry(platformIds.second, (int64_t)platformIds.first, fatalError);
            if (fatalError) 
                return ERR_SYSTEM;

            if (dbResults->empty())
                continue;

            // There may already be an entry, don't override it:
            if (mResponse.getTablesByPlatform().find(platformIds.second) == mResponse.getTablesByPlatform().end())
                mResponse.getTablesByPlatform()[platformIds.second] = mResponse.getTablesByPlatform().allocate_element();

            Blaze::GdprCompliance::TableData* tableData = mResponse.getTablesByPlatform()[platformIds.second]->pull_back();
            tableData->setTableDesc("Arson Custom Table");
            Blaze::GdprCompliance::RowPtr descRow = tableData->getRows().pull_back();     // First RowList must be the descriptions:
            descRow->getRowData().push_back("PII Data One");
            descRow->getRowData().push_back("Non-PII Data Two");
            descRow->getRowData().push_back("PII Data Three");

            DbResult::const_iterator it = dbResults->begin();
            DbRow *dbRow = *it;

            Blaze::GdprCompliance::RowPtr dataRow = tableData->getRows().pull_back();     // Second and later rows are the values:
            dataRow->getRowData().push_back(dbRow->getString("data1"));
            dataRow->getRowData().push_back(dbRow->getString("data2"));
            dataRow->getRowData().push_back(dbRow->getString("data3"));
        }

        return ERR_OK;
    }
};

DEFINE_GETUSERDATA_CREATE();

class DeactivateUserDataCommand : public DeactivateUserDataCommandStub
{
public:
    DeactivateUserDataCommand(
        Message* message, Blaze::GdprCompliance::CustomComponentGetDataRequest* request, ArsonSlaveImpl* componentImpl)
        : DeactivateUserDataCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~DeactivateUserDataCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    BlazeRpcError deactivateDbEntry(ClientPlatformType platform, int64_t id)
    {
        // Get a database connection.
        Blaze::DbConnPtr dbConn = gDbScheduler->getConnPtr(mComponent->getDbId());  // platform
        if (dbConn == nullptr)
        {
            ERR_LOG("[DeactivateUserDataCommand].execute: Failed to obtain a connection. (quit)");
            return ERR_SYSTEM;
        }

        // Build the query to save test data
        QueryPtr queryBuf = DB_NEW_QUERY_PTR(dbConn);
        queryBuf->append("UPDATE `arson_test_data` SET `data1`='', `data3`=''");
        queryBuf->append("WHERE `id`=$D", id );

        TRACE_LOG("[DeactivateUserDataCommand].deactivateDbEntry: Query -- " << queryBuf->c_str());
        return dbConn->executeQuery(queryBuf);
    }

    DeactivateUserDataCommandStub::Errors execute() override
    {
        BlazeRpcError error = deactivateDbEntry(common, (int64_t)mRequest.getAccountId());
        if (error != ERR_OK)
            return DeactivateUserDataCommandStub::commandErrorFromBlazeError(error);

        for (auto& platformIds : mRequest.getPlatformMap())
        {
            error = deactivateDbEntry(platformIds.second, (int64_t)platformIds.first);
            if (error != ERR_OK)
                return DeactivateUserDataCommandStub::commandErrorFromBlazeError(error);
        }

        return ERR_OK;
    }

};

DEFINE_DEACTIVATEUSERDATA_CREATE();

} //Arson
} //Blaze
