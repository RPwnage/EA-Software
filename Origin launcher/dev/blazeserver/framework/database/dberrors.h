/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_DBERRORS_H
#define BLAZE_DBERRORS_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

typedef BlazeRpcError DbError;

//  map old DbError values to BlazeRpcError equivalents
const BlazeRpcError DB_ERR_OK = ERR_OK;                                 // Success
const BlazeRpcError DB_ERR_SYSTEM = ERR_DB_SYSTEM;                      // General DB error
const BlazeRpcError DB_ERR_NOT_CONNECTED = ERR_DB_NOT_CONNECTED;        // Not connected to the DB
const BlazeRpcError DB_ERR_NOT_SUPPORTED = ERR_DB_NOT_SUPPORTED;        // Operation not supported
const BlazeRpcError DB_ERR_NO_CONNECTION_AVAILABLE = ERR_DB_NO_CONNECTION_AVAILABLE;     // No connection could be obtained
const BlazeRpcError DB_ERR_DUP_ENTRY = ERR_DB_DUP_ENTRY;                // A duplicate entry already exists in the DB
const BlazeRpcError DB_ERR_NO_SUCH_TABLE = ERR_DB_NO_SUCH_TABLE;        // Table does not exist
const BlazeRpcError DB_ERR_DISCONNECTED = ERR_DB_DISCONNECTED;          // Lost connection to DB
const BlazeRpcError DB_ERR_TIMEOUT = ERR_DB_TIMEOUT;                    // Request timed out
const BlazeRpcError DB_ERR_INIT_FAILED = ERR_DB_INIT_FAILED;            // Driver initialization failed
const BlazeRpcError DB_ERR_TRANSACTION_NOT_COMPLETE = ERR_DB_TRANSACTION_NOT_COMPLETE;    // Connection was released while a transaction was pending
const BlazeRpcError DB_ERR_LOCK_DEADLOCK = ERR_DB_LOCK_DEADLOCK;        // Deadlock
const BlazeRpcError DB_ERR_DROP_PARTITION_NON_EXISTENT = ERR_DB_DROP_PARTITION_NON_EXISTENT; // Partition does not exist
const BlazeRpcError DB_ERR_SAME_NAME_PARTITION = ERR_DB_SAME_NAME_PARTITION; // Partition name already exists
const BlazeRpcError DB_ERR_NO_ROWS_AFFECTED = ERR_DB_NO_ROWS_AFFECTED; // No rows were affected by the query
const BlazeRpcError DB_ERR_NO_SUCH_THREAD = ERR_DB_NO_SUCH_THREAD;      // No such database thread

const char8_t* getDbErrorString(BlazeRpcError error);

} // Blaze

#endif // BLAZE_DBERRORS_H

