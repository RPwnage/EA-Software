/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CLUBS_CLUBSDBCONNECTOR_H
#define BLAZE_CLUBS_CLUBSDBCONNECTOR_H

/*** Include files *******************************************************************************/
#include "clubs/clubsdb.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace Clubs
{
class ClubsSlaveImpl;

class ClubsDbConnector
{
public:
    ClubsDbConnector(ClubsSlaveImpl* component);
    ~ClubsDbConnector();

public:
    DbConnPtr& acquireDbConnection(bool readOnly);
    void releaseConnection();
    
    DbConnPtr& getDbConn() { return mInternalDbConn; }
    ClubsDatabase& getDb() { return mClubsDb; }
    bool startTransaction();
    bool completeTransaction(bool success);

    bool isTransactionStart() const { return mTransaction; }

protected:
    ClubsSlaveImpl* mComponent;
    ClubsDatabase mClubsDb;

private:
    DbConnPtr mInternalDbConn;
    bool mTransaction;
    bool mReadOnly;
};

} // Clubs
} // Blaze

#endif // BLAZE_CLUBS_CLUBSCOMMANDBASE_H

