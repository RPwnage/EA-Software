/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_DBADMIN_H
#define BLAZE_DBADMIN_H

/*** Include files *******************************************************************************/

#include "framework/database/dberrors.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class DbConnBase;
class DbConnInterface;
class DbConfig;

class DbAdmin
{
public:
    virtual ~DbAdmin() { }

    virtual EA::Thread::ThreadId start() = 0;
    virtual void stop() = 0;

    virtual BlazeRpcError checkDbStatus() = 0;

    virtual DbConnInterface* createDbConn(const DbConfig& config, DbConnBase& owner) = 0;

    virtual BlazeRpcError registerDbThread() = 0;
    virtual BlazeRpcError unregisterDbThread() = 0;
};

} // Blaze

#endif // BLAZE_DBADMIN_H

