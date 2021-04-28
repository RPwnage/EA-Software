/*************************************************************************************************/
/*!
    \file   statsprovider.h

    Stat provider is a proxy between Stats component engine and 
    storage used to persist stats. Examples of storage include 
    database, cache or simple file.

    This file declares bare interfaces that stats provider should implement.

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_STATSPROVIDER_H
#define BLAZE_STATS_STATSPROVIDER_H

/*** Include files *******************************************************************************/
#include "EASTL/vector_map.h"
#include "EASTL/intrusive_ptr.h"

#include "framework/identity/identity.h"

#include "stats/statscommontypes.h"
#include "stats/tdf/stats_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace Stats
{
// forward definitions
class StatsProvider;
class StatsProviderResultSet;
class StatsSlaveImpl;
struct UpdateAggregateRowKey;

/********************************************************************/
/*
    StatsProviderRow

    This class represents single row in result set returned by
    stats provider. A row consists of cells which contain values.
    Cells are addressed by name.

    Row is always part of result set (see StatsProviderResultSet 
    below).

    Implementation of this class should be just facade to the real
    storage row.
*/
/********************************************************************/
class StatsProviderRow
{
private:
    NON_COPYABLE(StatsProviderRow);

public:
    virtual ~StatsProviderRow() { }

    virtual int64_t getValueInt64(const char8_t* column) const = 0;
    virtual int32_t getValueInt(const char8_t* column) const = 0;
    virtual float_t getValueFloat(const char8_t* column) const = 0;
    virtual const char8_t* getValueString(const char8_t* column) const = 0;
    virtual EntityId getEntityId() const = 0;
    virtual int32_t getPeriodId() const = 0;

    const StatsProviderResultSet& getProviderResultSet() const { return mProviderResultSet; }

protected:
    friend class StatsProviderResultSet;
    StatsProviderRow(const StatsProviderResultSet &resultSet)
        : mProviderResultSet(resultSet), mRefCount(0) {}

    const StatsProviderResultSet& mProviderResultSet;

private:
    uint32_t mRefCount;

    friend void intrusive_ptr_add_ref(StatsProviderRow* ptr);
    friend void intrusive_ptr_release(StatsProviderRow* ptr);
};

typedef eastl::intrusive_ptr<StatsProviderRow> StatsProviderRowPtr;
inline void intrusive_ptr_add_ref(StatsProviderRow* ptr)
{
    ptr->mRefCount++;
}

inline void intrusive_ptr_release(StatsProviderRow* ptr)
{
    if (--ptr->mRefCount == 0)
        delete ptr;
}

/********************************************************************/
/*
    StatsProviderResultSet
    
    Contains list of rows that belong to particular result set. Result sets 
    are typically produced during fetch or fetch for update operations.
    Each row in result set is addressed by its Stats composite key consisting of
    Entity Id, period Id and scope vector.
*/
/********************************************************************/
class StatsProviderResultSet
{
private:
    NON_COPYABLE(StatsProviderResultSet);

public:
    virtual ~StatsProviderResultSet() { };

    // gets row defined by complete stats composite key
    virtual StatsProviderRowPtr getProviderRow(const EntityId entityId, const int32_t periodId, const RowScopesVector* rowScopesVector) const = 0;
    // gets first row that matchis any key created as combination of provided entity id, period id and all variations of scope value set
    virtual StatsProviderRowPtr getProviderRow(const EntityId entityId, const int32_t periodId, const ScopeNameValueMap* rowScopesMap) const = 0;
    // gets first row that matches scope part of the key (any entity Id or period Id)
    virtual StatsProviderRowPtr getProviderRow(const ScopeNameValueMap* rowScopesMap) const = 0;

    // count of rows in this result set
    virtual const size_t size() const = 0;

    StatsProvider& getStatsProvider() const { return mStatsProvider; }

    // iteration functions
    virtual bool hasNextRow() const = 0;
    virtual StatsProviderRowPtr getNextRow() = 0;
    virtual void reset() = 0;

protected:
    friend class StatsProvider;
    StatsProviderResultSet(
        const char8_t* category,
        StatsProvider &statsProvider);

    char8_t mCategory[STATS_CATEGORY_NAME_LENGTH];
    StatsProvider& mStatsProvider;
};


/********************************************************************/
/*
    StatsProvider
    
    Main class representing all provider functionality. It is used for
    *) Updating stats in four steps:
        1) create queries for rows that match keys in update request
        2) fetch rows
        3) create queries based on update requests
        4) write the result to storage
    *) Fetching stats in two steps:
        1) create queries for rows that match keys in fetch request
        2) fetch data from storage 
     
     Result for fetch operations is provided as collection of result sets.
     Each fetch operation produces one result set.
*/
/********************************************************************/
class StatsProvider
{
private:
    NON_COPYABLE(StatsProvider);

public:
    StatsProvider(StatsSlaveImpl& statsSlave)
        : mComponent(statsSlave) , mRefCount(0){}
    virtual ~StatsProvider();

    // creates query to fetch stats to be updated for row in specific category and period
    // with full Stats composite key
    virtual BlazeRpcError addStatsFetchForUpdateRequest(
        const StatCategory& category,
        EntityId entityId,
        int32_t periodType,
        int32_t periodId,
        const ScopeNameValueSetMap &scopeNameValueSetMap) = 0;

    // creates query to insert default stats for row in specific category and period
    // with full Stats composite key
    virtual BlazeRpcError addDefaultStatsInsertForUpdateRequest(
        const StatCategory& category,
        EntityId entityId,
        int32_t periodType,
        int32_t periodId,
        const RowScopesVector* rowScopes) = 0;

    // creates query to update stats for row in specific category and period
    // with full Stats composite key specified by aggKey which also be key to aggregate rows
    virtual BlazeRpcError addStatsUpdateRequest(
        int32_t periodId,
        const UpdateAggregateRowKey &aggKey,
        const StatValMap &statsValMap) = 0;

    // creates query to update stats for row in specific category and period
    // with full Stats composite key specified by parameters
    virtual BlazeRpcError addStatsUpdateRequest(
        const StatCategory& category,
        EntityId entityId,
        int32_t periodType,
        int32_t periodId,
        const RowScopesVector* rowScopes,
        const StatPtrValMap &valMap,
        bool newRow) = 0;

    // creates and immediately executes query for to fetch stats for row in 
    // specific category and period with full Stats composite key
    virtual BlazeRpcError executeFetchRequest(
        const StatCategory& category,
        const EntityIdList &entityIds,
        int32_t periodType,
        int32_t periodId,
        int32_t periodCount,
        const ScopeNameValueMap &scopeNameValueMap) = 0;

    // creates and immediately executes query for to fetch stats for row in 
    // specific category and period with full Stats composite key
    virtual BlazeRpcError executeFetchRequest(
        const StatCategory& category,
        const EntityIdList &entityIds,
        int32_t periodType,
        int32_t periodId,
        int32_t periodCount,
        const ScopeNameValueListMap &scopeNameValueListMap) = 0;

    // functions called to complete step 2 of update operation (see above)
    virtual BlazeRpcError executeFetchForUpdateRequest() = 0;
    virtual BlazeRpcError executeUpdateRequest() = 0;

    // transaction functions
    virtual BlazeRpcError commit() = 0;
    virtual BlazeRpcError rollback() = 0;

    // iteration functions
    void reset() { mResultSetIterator = mResultSetVector.begin(); }
    size_t getResultsetCount() const;
    bool hasNextResultSet() const;
    StatsProviderResultSet* getNextResultSet();

protected:
    StatsSlaveImpl& mComponent;

    typedef eastl::vector<StatsProviderResultSet*> ResultSetVector;
    ResultSetVector mResultSetVector;
    ResultSetVector::const_iterator mResultSetIterator;

private:
    uint32_t mRefCount;

    friend void intrusive_ptr_add_ref(StatsProvider* ptr);
    friend void intrusive_ptr_release(StatsProvider* ptr);
};

typedef eastl::intrusive_ptr<StatsProvider> StatsProviderPtr;
inline void intrusive_ptr_add_ref(StatsProvider* ptr)
{
    ptr->mRefCount++;
}

inline void intrusive_ptr_release(StatsProvider* ptr)
{
    if (--ptr->mRefCount == 0)
        delete ptr;
}

} // Stats
} // Blaze
#endif // BLAZE_STATS_STATSPROVIDER_H
