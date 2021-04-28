/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ClubsDbConnector

    This class consolidates database connecting operations, such as acquiring connection,
    starting/ending transaction and releasing database connection.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"

// clubs includes
#include "clubs/clubsdbconnector.h"
#include "clubs/clubsslaveimpl.h"

namespace Blaze
{
namespace Clubs
{
/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/
ClubsDbConnector::ClubsDbConnector(ClubsSlaveImpl* component)
    : mComponent(component)
{
    TRACE_LOG("[ClubsDbConnector].ClubsDbConnector() - constructing clubs db connector");

    mTransaction = false;
    mReadOnly = false;
}

ClubsDbConnector::~ClubsDbConnector()
{
    TRACE_LOG("[ClubsDbConnector].~ClubsDbConnector() - destroying clubs db connector");
    // if there is an open transaction, close it here
    completeTransaction(false);
}

/*** Public Methods ***************************************************************************/
DbConnPtr& ClubsDbConnector::acquireDbConnection(bool readOnly)
{
    releaseConnection();
    
    if (readOnly)
    {
        TRACE_LOG("[ClubsDbConnector].acquireDbConnection() - Acquring a new read-only connection");
        mInternalDbConn = gDbScheduler->getReadConnPtr(mComponent->getDbId());
    }
    else
    {
        TRACE_LOG("[ClubsDbConnector].acquireDbConnection() - Acquring a new read/write connection");
        mInternalDbConn = gDbScheduler->getConnPtr(mComponent->getDbId());
    }
    
    mReadOnly = readOnly;
    mTransaction = false;

    mClubsDb.setDbConn(mInternalDbConn);

    return mInternalDbConn;
}

bool ClubsDbConnector::startTransaction()
{
    TRACE_LOG("[ClubsDbConnector].startTransaction() - Marking transaction as started");
    
    if (mReadOnly)
    {
        ERR_LOG("[ClubsDbConnector].startTransaction() - Cannot start transaction on read-only connection");
        return false;
    }
    
    if (mInternalDbConn == nullptr && acquireDbConnection(false) == nullptr)
        return false;
        
    BlazeRpcError dbError = mInternalDbConn->startTxn();
    
    if (dbError != DB_ERR_OK)
    {
        ERR_LOG("[ClubsDbConnector].startTransaction() - Could not start transaction!");
        return false;
    }
    
    mTransaction = true;
    
    return true;
}

bool ClubsDbConnector::completeTransaction(bool success)
{
    TRACE_LOG("[ClubsDbConnector].completeTransaction() - completing transaction on connection " 
              << (mInternalDbConn != nullptr ? mInternalDbConn->getConnNum() : 0));

    bool result = false;
    
    // Only take action if we actually own the connection
    if (mInternalDbConn != nullptr)
    {
        if (mTransaction)
        {
            // If we opened a transaction, we need to commit or rollback now depending on the success status
            TRACE_LOG("[ClubsDbConnector].completeTransaction() - Completing transaction on connection " 
                << mInternalDbConn->getConnNum() << " with " << (success? "COMMIT" : "ROLLBACK"));

            BlazeRpcError dbError;
            
            if (success) 
                dbError = mInternalDbConn->commit();
            else
                dbError = mInternalDbConn->rollback();
                
            if (dbError != DB_ERR_OK)
            {
                ERR_LOG("[ClubsDbConnector].completeTransaction() - database transaction was not successful");
            }
            else
                result = true;
        }

        releaseConnection();
    }
    
    return result;
}

void ClubsDbConnector::releaseConnection()
{
    if (mInternalDbConn != nullptr)
    {
        TRACE_LOG("[ClubsDbConnector].releaseConnection() - Releasing connection " << mInternalDbConn->getConnNum());
            
        mTransaction = false;
        mReadOnly = false;
    }
}

} // Clubs
} // Blaze

