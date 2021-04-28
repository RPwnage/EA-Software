/*************************************************************************************************/
/*!
    \file   updatestatshelper.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateStatsHelper

    Update statistics.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/util/expression.h"
#include "framework/controller/controller.h"

#include "EASTL/functional.h"
#include "EASTL/sort.h"
#include "EASTL/scoped_ptr.h"

#include "dbstatsprovider.h"
#include "globalstatsprovider.h"
#include "updatestatshelper.h"
#include "stats/statsslaveimpl.h"

namespace Blaze
{
namespace Stats
{
/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief UpdateStatsHelper

    Constructor for the UpdateStatsHelper wrapper that holds the data structures and acts as a 
    state machine required for performing database actions including fetch, apply derived changes 
    and commit.
*/
/*************************************************************************************************/
UpdateStatsHelper::UpdateStatsHelper() 
    : mUpdateState(UNINITIALIZED),
    mStatsComp(nullptr),
    mDirty(false),
    mTransactionContextId(INVALID_TRANSACTION_ID),
    mRemoteSlaveId(INVALID_INSTANCE_ID)
{
    BlazeRpcError err = ERR_SYSTEM;
    mStatsComp = (Blaze::Stats::StatsSlave*) gController->getComponent(Stats::StatsSlave::COMPONENT_ID, false/*reqLocal*/, true, &err);
    if (mStatsComp != nullptr)
    {
        mRemoteSlaveId = mStatsComp->selectInstanceId();
    }
}

/*************************************************************************************************/
/*!
    \brief ~UpdateStatsHelper

    Destroys the UpdateStatsHelper object.
*/
/*************************************************************************************************/
UpdateStatsHelper::~UpdateStatsHelper()
{
    reset();
}

/*************************************************************************************************/
/*!
    \brief initializeStatUpdate

    Initialize the data structures and the state to be ready for making database calls and 
    storing stats results.

    \param[in]  request - collection of stat update requests to perform
    \param[in]  statsComp - stats slave component
    \param[in]  strict - enforce well-formed stat row update requests (i.e. invalid requests are ignored 
            instead of throwing out the request wholesale.
    \param[in]  processGlobalStats - set to true by broadacsting notification handler to enable processing of 
            broadcasted global stat update
 
    \return - corresponding error code
*/
/*************************************************************************************************/
BlazeRpcError UpdateStatsHelper::initializeStatUpdate(const UpdateStatsRequest &request,
                                                      bool strict,
                                                      bool processGlobalStats,
                                                      uint64_t timeout)
{
    reset();
    request.copyInto(mRequest);

    BlazeRpcError rc = ERR_SYSTEM;
    if (mStatsComp != nullptr)     
    {         
        Blaze::Stats::InitializeStatsTransactionRequest req; 
        Blaze::Stats::InitializeStatsTransactionResponse res;
        request.getStatUpdates().copyInto(req.getStatUpdates());
        req.setStrict(strict);
        req.setGlobalStats(processGlobalStats);
        req.setTimeout(timeout);
        
        RpcCallOptions opts;
        opts.routeTo.setInstanceId(mRemoteSlaveId);
        rc = mStatsComp->initializeStatsTransaction(req, res, opts);          
        if (rc == ERR_OK)
        {
            mTransactionContextId = res.getTransactionContextId();
            mUpdateState |= INITIALIZED;
        }
    } 

    return rc;
}

/*************************************************************************************************/
/*!
    \brief fetchStats

    Build the SELECT FOR statements and execute the queries to retrieve the requested stats and
    to lock rows for future update.
 
    \return - corresponding database error code
*/
/*************************************************************************************************/

BlazeRpcError UpdateStatsHelper::fetchStats()
{
    if ((mUpdateState & ABORTED) != 0 || (mUpdateState & INITIALIZED)==0)
        return ERR_SYSTEM;

    BlazeRpcError rc = ERR_SYSTEM;
    if (mStatsComp != nullptr)     
    {         
        Blaze::Stats::RetrieveValuesStatsRequest req; 
        Blaze::Stats::RetrieveValuesStatsResponse res;
        req.setTransactionContextId(mTransactionContextId);
        
        RpcCallOptions opts;
        opts.routeTo.setInstanceId(mRemoteSlaveId);
        rc = mStatsComp->retrieveValuesStats(req, res, opts);   
        if (rc == ERR_OK)
        {
            if (mOriginalCacheRowList.empty())
            {
                res.getCacheRowList().copyInto(mOriginalCacheRowList);
                res.getUpdatedCacheRowList().copyInto(mCacheRowList);   // The updated cache row list is only used on the initial update. 
            }           
            else
            {
                res.getCacheRowList().copyInto(mCacheRowList);
            }
            mUpdateState |= FETCHED;
        }
    } 

    return rc;
}


/*************************************************************************************************/
/*!
    \brief calcDerivedStats

    Calculate the derived stats and write the new values to the stat value map inside UpdateRowMap.
 
    \return - corresponding error code
*/
/*************************************************************************************************/
BlazeRpcError UpdateStatsHelper::calcDerivedStats()
{
    if ((mUpdateState & ABORTED) != 0 || (mUpdateState & INITIALIZED)==0)
        return ERR_SYSTEM;

    BlazeRpcError rc = ERR_SYSTEM;
    if (mStatsComp != nullptr)     
    {         
        Blaze::Stats::CalcDerivedStatsRequest req; 
        if (mDirty)
        {
            mCacheRowList.copyInto(req.getCacheRowList());
        }
        req.setTransactionContextId(mTransactionContextId);

        RpcCallOptions opts;
        opts.routeTo.setInstanceId(mRemoteSlaveId);
        rc = mStatsComp->calcDerivedStats(req, opts);     
        if (rc == ERR_OK)
        {
            mCacheRowList.clear();
            mDirty = false;
            rc = fetchStats();
        }
    } 

    return rc;
}

/*************************************************************************************************/
/*!
    \brief commitStats

    Commit the stats updates previously applied by making a stats component call.

    \return - corresponding error code
*/
/*************************************************************************************************/

BlazeRpcError UpdateStatsHelper::commitStats()
{
    BlazeRpcError rc = ERR_SYSTEM;
    if (mStatsComp != nullptr)     
    {         
        Blaze::Stats::CommitTransactionRequest req; 
        if (mDirty)
        {
            mCacheRowList.copyInto(req.getCacheRowList());
        }
        req.setTransactionContextId(mTransactionContextId);

        RpcCallOptions opts;
        opts.routeTo.setInstanceId(mRemoteSlaveId);
        rc = mStatsComp->commitStatsTransaction(req, opts);    
        if (rc == ERR_OK)
        {
            mUpdateState = COMMITED;
            mDirty = false;
        }
    } 

    return rc;

}

/*************************************************************************************************/
/*!
    \brief abortTransaction

    Rollback database transaction if the transaction has started earlier and release 
    database connection if the connection is still active.
*/
/*************************************************************************************************/
void UpdateStatsHelper::abortTransaction()
{
    // prevent aborting more than once or abort after commit
    if ((mUpdateState & ABORTED) != 0 || (mUpdateState & INITIALIZED)==0)
        return;

    BlazeRpcError rc = ERR_SYSTEM;
    if (mStatsComp != nullptr)     
    {         
        Blaze::Stats::AbortTransactionRequest req; 
        req.setTransactionContextId(mTransactionContextId);
        
        RpcCallOptions opts;
        opts.routeTo.setInstanceId(mRemoteSlaveId);
        rc = mStatsComp->abortStatsTransaction(req, opts);  
        if (rc == ERR_OK)
        {
            mUpdateState = ABORTED;
        }
    }   
}

/*************************************************************************************************/
/*!
    \brief reset

    Reset the update stats to the uninitialized state and clean up the data structures.
*/
/*************************************************************************************************/
void UpdateStatsHelper::reset()
{
    abortTransaction();
    mCacheRowList.clear();
    mOriginalCacheRowList.clear();
    mUpdateState = UNINITIALIZED;
}

/*************************************************************************************************/
/*!
    \brief getValueInt

    Retrieve the value for a stat from the in-progress stats update.  Caller has the option to
    fetch either the current value after any processing has already been applied, or the
    originally fetched value straight from the provider.

    \param[in]  key - key of the row to fetch the stat from
    \param[in]  statName - name of stat within the row
    \param[in]  periodType - period type to fetch the stat for
    \param[in]  fromProvider - true to get original value from provider, false for current updated value

    \return - the stat value if found, 0 otherwise
*/
/*************************************************************************************************/
int64_t UpdateStatsHelper::getValueInt(const UpdateRowKey* key, const char8_t* statName, 
                                       StatPeriodType periodType, bool fromProvider)
{
    EA_ASSERT_MSG((mUpdateState & FETCHED) != 0, "The data you tried to fetch is not cached");

    FetchedRowUpdate* update = nullptr;
    if (fromProvider)
    {
        update = findCacheRow(key, periodType, mOriginalCacheRowList);
    }
    else
    {
        update = findCacheRow(key, periodType, mCacheRowList);
    }

    if (update != nullptr)
    {
        uint32_t size = update->getStatUpdates().size();
        for (uint32_t i=0; i<size; i++)
        {
            if (blaze_stricmp(update->getStatUpdates()[i]->getName(), statName) == 0)
            {
                return update->getStatUpdates()[i]->getValueInt();
            }
        }
    }

    return 0;
}

/*************************************************************************************************/
/*!
    \brief getValueFloat

    Retrieve the value for a stat from the in-progress stats update.  Caller has the option to
    fetch either the current value after any processing has already been applied, or the
    originally fetched value straight from the provider.

    \param[in]  key - key of the row to fetch the stat from
    \param[in]  statName - name of stat within the row
    \param[in]  periodType - period type to fetch the stat for
    \param[in]  fromProvider - true to get original value from provider, false for current updated value

    \return - the stat value if found, 0 otherwise
*/
/*************************************************************************************************/
float_t UpdateStatsHelper::getValueFloat(const UpdateRowKey* key, const char8_t* statName, 
                                         StatPeriodType periodType, bool fromProvider)
{
    EA_ASSERT_MSG((mUpdateState & FETCHED) != 0, "The data you tried to fetch is not cached");

    FetchedRowUpdate* update = nullptr;
    if (fromProvider)
    {
        update = findCacheRow(key, periodType, mOriginalCacheRowList);
    }
    else
    {
        update = findCacheRow(key, periodType, mCacheRowList);
    }

    if (update != nullptr)
    {
        uint32_t size = update->getStatUpdates().size();
        for (uint32_t i=0; i<size; i++)
        {
            if (blaze_stricmp(update->getStatUpdates()[i]->getName(), statName) == 0)
                return update->getStatUpdates()[i]->getValueFloat();
        }
    }

    return 0;
}

/*************************************************************************************************/
/*!
    \brief getValueString

    Retrieve the value for a stat from the in-progress stats update.  Caller has the option to
    fetch either the current value after any processing has already been applied, or the
    originally fetched value straight from the provider.

    \param[in]  key - key of the row to fetch the stat from
    \param[in]  statName - name of stat within the row
    \param[in]  periodType - period type to fetch the stat for
    \param[in]  fromProvider - true to get original value from provider, false for current updated value

    \return - the stat value if found, nullptr otherwise
*/
/*************************************************************************************************/
const char8_t* UpdateStatsHelper::getValueString(const UpdateRowKey* key, const char8_t* statName, 
                                                 StatPeriodType periodType, bool fromProvider)
{
    EA_ASSERT_MSG((mUpdateState & FETCHED) != 0, "The data you tried to fetch is not cached");

    FetchedRowUpdate* update = nullptr;
    if (fromProvider)
    {
        update = findCacheRow(key, periodType, mOriginalCacheRowList);
    }
    else
    {
        update = findCacheRow(key, periodType, mCacheRowList);
    }

    if (update != nullptr)
    {
        uint32_t size = update->getStatUpdates().size();
        for (uint32_t i=0; i<size; i++)
        {
            if (blaze_stricmp(update->getStatUpdates()[i]->getName(), statName) == 0)
                return update->getStatUpdates()[i]->getValueString();
        }
    }

    return nullptr;
}

/*************************************************************************************************/
/*!
    \brief setValueInt

    Modify the value for a stat.

    \param[in]  key - key of the row to update the stat in 
    \param[in]  statName - name of stat within the row
    \param[in]  periodType - period type to update the stat for
    \param[in]  newValue - new value to assign to the stat
*/
/*************************************************************************************************/
void UpdateStatsHelper::setValueInt(const UpdateRowKey* key, const char8_t* statName,
                              StatPeriodType periodType, int64_t newValue)
{
    EA_ASSERT_MSG((mUpdateState & FETCHED) != 0, "The data you tried to fetch is not cached");

    FetchedRowUpdate* update = findCacheRow(key, periodType, mCacheRowList);
    if (update != nullptr)
    {
        uint32_t size = update->getStatUpdates().size();
        for (uint32_t i=0; i<size; i++)
        {
            if (blaze_stricmp(update->getStatUpdates()[i]->getName(), statName) == 0)
            {
                update->getStatUpdates()[i]->setValueInt(newValue);
                update->getStatUpdates()[i]->setChanged(true);
                mDirty = true;
            }
        }
    }
}

/*************************************************************************************************/
/*!
    \brief setValueFloat

    Modify the value for a stat.

    \param[in]  key - key of the row to update the stat in 
    \param[in]  statName - name of stat within the row
    \param[in]  periodType - period type to update the stat for
    \param[in]  newValue - new value to assign to the stat
*/
/*************************************************************************************************/
void UpdateStatsHelper::setValueFloat(const UpdateRowKey* key, const char8_t* statName,
                                      StatPeriodType periodType, float_t newValue)
{
    EA_ASSERT_MSG((mUpdateState & FETCHED) != 0, "The data you tried to fetch is not cached");

    FetchedRowUpdate* update = findCacheRow(key, periodType, mCacheRowList);
    if (update != nullptr)
    {
        uint32_t size = update->getStatUpdates().size();
        for (uint32_t i=0; i<size; i++)
        {
            if (blaze_stricmp(update->getStatUpdates()[i]->getName(), statName) == 0)
            {
                update->getStatUpdates()[i]->setValueFloat(newValue);
                update->getStatUpdates()[i]->setChanged(true);
                mDirty = true;
            }
        }
    }
}

/*************************************************************************************************/
/*!
    \brief setValueString

    Modify the value for a stat.

    \param[in]  key - key of the row to update the stat in 
    \param[in]  statName - name of stat within the row
    \param[in]  periodType - period type to update the stat for
    \param[in]  newValue - new value to assign to the stat
*/
/*************************************************************************************************/
void UpdateStatsHelper::setValueString(const UpdateRowKey* key, const char8_t* statName,
                                       StatPeriodType periodType, const char8_t* newValue)
{
    EA_ASSERT_MSG((mUpdateState & FETCHED) != 0, "The data you tried to fetch is not cached");

    FetchedRowUpdate* update = findCacheRow(key, periodType, mCacheRowList);
    if (update != nullptr)
    {
        uint32_t size = update->getStatUpdates().size();
        for (uint32_t i=0; i<size; i++)
        {
            if (blaze_stricmp(update->getStatUpdates()[i]->getName(), statName) == 0)
            {
                update->getStatUpdates()[i]->setValueString(newValue);
                update->getStatUpdates()[i]->setChanged(true);
                mDirty = true;
            }
        }
    }
}

FetchedRowUpdate* UpdateStatsHelper::findCacheRow(const UpdateRowKey* key, StatPeriodType periodType, FetchedRowUpdateList& cacheRowList)
{
    FetchedRowUpdateList::iterator ii = cacheRowList.begin();
    FetchedRowUpdateList::iterator ee = cacheRowList.end();
    while (ii != ee)
    {
        // try to find key
        bool foundKey = false;
        FetchedRowUpdate* update = *ii;

        // key->scopeNameValueMap could be nullptr, instead of size 0.
        if (key->scopeNameValueMap == nullptr && update->getKeyScopeNameValueMap().size() == 0)
        {
            foundKey = true;
        }
        else if (key->scopeNameValueMap != nullptr)
        {
            if (update->getKeyScopeNameValueMap().size() == key->scopeNameValueMap->size())
            {
                foundKey = true;
                ScopeNameValueMap::const_iterator it = update->getKeyScopeNameValueMap().begin();
                ScopeNameValueMap::const_iterator et = update->getKeyScopeNameValueMap().end();
                while(it != et)
                {
                    ScopeNameValueMap::const_iterator ft = key->scopeNameValueMap->find(it->first);
                    if (ft == key->scopeNameValueMap->end() || ft->second != it->second)
                    {
                        foundKey = false;
                        break;
                    }

                    it++;
                }
            }
            else
            {
                foundKey = false;
            }
        }
        
        if ( foundKey
            && blaze_stricmp(key->category, update->getCategory()) == 0
            && periodType == update->getPeriodType() 
            && key->entityId == update->getEntityId())
        {
            return update;      
        }

        ii++;
    }

    return nullptr;
}

}// Stats
} // Blaze
