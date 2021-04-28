/*************************************************************************************************/
/*!
    \file   transformstats.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class UpdateStatsCommand

    Update statistics.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"


#include "EASTL/scoped_array.h"
#include "EASTL/scoped_ptr.h"
#include "transformstats.h"

namespace Blaze
{
namespace Stats
{


// UpdateRowKey code
UpdateRowKey::UpdateRowKey()
{
    // intentional no-op constructor because map stores these objects by value, no point constructing them twice
}

UpdateRowKey::UpdateRowKey(const char8_t* _category, EntityId _entityId, 
        int32_t _period, const Stats::ScopeNameValueMap* map)
    : category(_category), entityId(_entityId), period(_period), scopeNameValueMap(map)
{
}

bool UpdateRowKey::operator < (const UpdateRowKey& other) const
{
    // IMPORTANT: Until we get rid of statRowUpdateLess comparator object,
    // the UpdateRowKey::operator <() comparison algorithm must be equivalent to it,
    // because currently the statRowUpdateLess algorithm is used to sort input
    // to generate queries and the the UpdateRowKey::operator <() is used to walk
    // ther query results in order...
    const int32_t result = blaze_strcmp(category, other.category);
    if (result != 0)                 
        return (result < 0);

    if (entityId != other.entityId)  
        return (entityId < other.entityId);

    // this means both maps are not nullptr
    const size_t sizeA = (scopeNameValueMap != nullptr) ? scopeNameValueMap->size() : 0;
    const size_t sizeB = (other.scopeNameValueMap != nullptr) ? other.scopeNameValueMap->size() : 0;
    if (sizeA != sizeB)            
        return (sizeA < sizeB);

    if (scopeNameValueMap != nullptr && other.scopeNameValueMap != nullptr)
    {
        // this means maps are the same size
        Stats::ScopeNameValueMap::const_iterator itA = scopeNameValueMap->begin();
        Stats::ScopeNameValueMap::const_iterator itAEnd = scopeNameValueMap->end();
        Stats::ScopeNameValueMap::const_iterator itB = other.scopeNameValueMap->begin();
        for(; itA != itAEnd; ++itA, ++itB)
        {
            if (itA->first != itB->first)    
                return (itA->first < itB->first);
            if (itA->second != itB->second)  
                return (itA->second < itB->second);
        }
    }

    if (period != other.period) 
        return (period < other.period);

    // if we get here that essentially means memberwise this == other
    return false;
}

// CategoryUpdateRowKey code
CategoryUpdateRowKey::CategoryUpdateRowKey()
{
    // intentional no-op constructor because map stores these objects by value, no point constructing them twice
}

CategoryUpdateRowKey::CategoryUpdateRowKey(EntityId _entityId, 
        int32_t _period, const Stats::ScopeNameValueMap* map)
    : entityId(_entityId), period(_period), scopeNameValueMap(map)
{
}

bool CategoryUpdateRowKey::operator < (const CategoryUpdateRowKey& other) const
{
    if (entityId != other.entityId) 
        return (entityId < other.entityId);

    const size_t sizeA = (scopeNameValueMap != nullptr) ? scopeNameValueMap->size() : 0;
    const size_t sizeB = (other.scopeNameValueMap != nullptr) ? other.scopeNameValueMap->size() : 0;
    if (sizeA != sizeB)
        return (sizeA < sizeB);

    if (scopeNameValueMap != nullptr && other.scopeNameValueMap != nullptr)
    {
        // this means maps are the same size
        Stats::ScopeNameValueMap::const_iterator itA = scopeNameValueMap->begin();
        Stats::ScopeNameValueMap::const_iterator itAEnd = scopeNameValueMap->end();
        Stats::ScopeNameValueMap::const_iterator itB = other.scopeNameValueMap->begin();
        for(; itA != itAEnd; ++itA, ++itB)
        {
            if (itA->first != itB->first)
                return (itA->first < itB->first);
            if (itA->second != itB->second)
                return (itA->second < itB->second);
        }
    }

    if (period != other.period) 
        return (period < other.period);

    // if we get here that essentially means memberwise this == other
    return false;
}

/*! ************************************************************************************************/
/*! \brief check if the passed in RowScopesVector is scopeNameValueMap hold in UpdateRowKey, 
    \param[in]rowScopes - RowScopesVector to be checked
    \return true if it is the scopeNameValueMap, which means the row is not an aggregate or
        total scope row, otherwise false
***************************************************************************************************/
bool UpdateRowKey::isDefaultScopeIndex(const RowScopesVector& rowScopes) const
{
    if (scopeNameValueMap == nullptr)
        return false;

    size_t size = rowScopes.size();
    for (size_t i = 0; i < size; ++i)
    {
        ScopeNameValueMap::const_iterator it = scopeNameValueMap->find(rowScopes[i].mScopeName);
        if (it != scopeNameValueMap->end())
        {
            if (it->second != (ScopeValue) rowScopes[i].mScopeValue)
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    return true;
}

/// @todo cleanup: deprecate UpdateAggregateRowKey ???  it may be used for both aggregate and "total"
/*! ************************************************************************************************/
/*! \brief * Class used to search for a given StatValMap within UpdateAggregateRowMap
***************************************************************************************************/
UpdateAggregateRowKey::UpdateAggregateRowKey(const char8_t* category, EntityId entityId, 
                                             int32_t period,
                                             const RowScopesVector& rowScopesVector)
                        : mCategory(category), 
                          mEntityId(entityId), 
                          mPeriod(period)
{
    mRowScopesVector = rowScopesVector;
}

bool UpdateAggregateRowKey::operator < (const UpdateAggregateRowKey& other) const
{
    const int32_t result = blaze_strcmp(mCategory, other.mCategory);
    if (result != 0) 
        return (result < 0);

    if (mEntityId != other.mEntityId) 
        return (mEntityId < other.mEntityId);

    // at this point, the vectors should have the same size because we already know
    // that, except for the scope, all other things for these two keys are the same
    const size_t sizeA = mRowScopesVector.size();
    const size_t sizeB = other.mRowScopesVector.size();
    if (sizeA != sizeB)
        return (sizeA < sizeB);

    // below comparison will determine if these two keys are the same
    Stats::RowScopesVector::const_iterator itA = mRowScopesVector.begin();
    Stats::RowScopesVector::const_iterator itAEnd = mRowScopesVector.end();
    Stats::RowScopesVector::const_iterator itB = other.mRowScopesVector.begin();
    for (; itA != itAEnd; ++itA, ++itB)
    {
        // This design makes the values lose [a,b] == [b,a], but avoids issues where the comparisons can fail or produce inconsistent sorting. 
        const int32_t result1 = blaze_strcmp(itA->mScopeName.c_str(), itB->mScopeName.c_str());
        if (result1 != 0)
            return (result1 < 0);

        if (itA->mScopeValue != itB->mScopeValue)
            return (itA->mScopeValue < itB->mScopeValue);
    }

    if (mPeriod != other.mPeriod) 
        return (mPeriod < other.mPeriod);

    return false;
}


CategoryUpdateAggregateRowKey::CategoryUpdateAggregateRowKey
    (EntityId entityId, int32_t period, const RowScopesVector& rowScopesVector)
        : mEntityId(entityId), mPeriod(period)
{
    mRowScopesVector = rowScopesVector;
}

bool CategoryUpdateAggregateRowKey::operator < (const CategoryUpdateAggregateRowKey& other) const
{
    if (mEntityId != other.mEntityId) 
        return (mEntityId < other.mEntityId);

    const size_t sizeA = mRowScopesVector.size();
    const size_t sizeB = other.mRowScopesVector.size();
    if (sizeA != sizeB)
        return (sizeA < sizeB);

    // below comparison will determine if these two keys are the same
    Stats::RowScopesVector::const_iterator itA = mRowScopesVector.begin();
    Stats::RowScopesVector::const_iterator itAEnd = mRowScopesVector.end();
    Stats::RowScopesVector::const_iterator itB = other.mRowScopesVector.begin();
    for (; itA != itAEnd; ++itA, ++itB)
    {
        // This design makes the values lose [a,b] == [b,a], but avoids issues where the comparisons can fail or produce inconsistent sorting. 
        const int32_t result1 = blaze_strcmp(itA->mScopeName.c_str(), itB->mScopeName.c_str());
        if (result1 != 0)
            return (result1 < 0);

        if (itA->mScopeValue != itB->mScopeValue)
            return (itA->mScopeValue < itB->mScopeValue);
    }

    if (mPeriod != other.mPeriod)
        return (mPeriod < other.mPeriod);

    return false;
}

// UpdateRow code
UpdateRow::~UpdateRow()
{
    // clean up the statValMap in aggregate map
    StatValMap::const_iterator valIter = mNewStatValMap.begin();
    StatValMap::const_iterator valIterEnd = mNewStatValMap.end();
    for (; valIter != valIterEnd; ++valIter)
    {
        if (valIter->second != nullptr)
            delete valIter->second;
    }

    mStrings.clear();
}

const StatsProviderRow* UpdateRow::getProviderRow() const
{
    // if it was found once just return it
    if (mProviderRow == nullptr && mProviderResultSet != nullptr)
    {
        mProviderRow = mProviderResultSet->getProviderRow(mScopeNameValueMap);
    }

    return mProviderRow.get();
}

bool UpdateRow::setValueInt(const char8_t* column, int64_t value)
{
    // return update from modified list if any, otherwise default update list
    StatValMap::iterator valIter = mNewStatValMap.find(column);
    if (valIter != mNewStatValMap.end())
    {
        StatVal* statVal = valIter->second;
        if (statVal->intVal != value)
        {
            statVal->changed = true;

            const Stat* stat = getCategory()->getStatsMap()->find(column)->second;
            // This modified list will be sent to the DB:
            getModifiedStatValueMap()[stat] = statVal;
        }
        statVal->intVal = value;
        return true;
    }
    return false;
}

bool UpdateRow::setValueFloat(const char8_t* column, float_t value)
{
    StatValMap::iterator valIter = mNewStatValMap.find(column);
    if (valIter != mNewStatValMap.end())
    {
        StatVal* statVal = valIter->second;
        if (statVal->floatVal != value)
        {
            statVal->changed = true;

            const Stat* stat = getCategory()->getStatsMap()->find(column)->second;
            // This modified list will be sent to the DB:
            getModifiedStatValueMap()[stat] = statVal;
        }
        statVal->floatVal = value;
        return true;
    }
    return false;
}

bool UpdateRow::setValueString(const char8_t* column, const char8_t* value)
{
    StatValMap::iterator valIter = mNewStatValMap.find(column);
    if (valIter != mNewStatValMap.end())
    {
        StatVal* statVal = valIter->second;
        if (statVal->stringcmp(value) != 0)
        {
            statVal->changed = true;

            const Stat* stat = getCategory()->getStatsMap()->find(column)->second;
            // This modified list will be sent to the DB:
            getModifiedStatValueMap()[stat] = statVal;

            if (value != nullptr)
            {
                // maintain a copy in case the original string gets deleted
                StringStatValue& stringStatValue = mStrings.push_back();
                blaze_strnzcpy(stringStatValue.buf, value, sizeof(stringStatValue.buf));
                statVal->stringVal = stringStatValue.buf;
            }
            else
            {
                statVal->stringVal = nullptr;
            }
        }
        return true;
    }
    return false;
}

// UpdateHelper code

void UpdateHelper::clear()
{
    // clean up the statValMap in aggregate map
    UpdateAggregateRowMap::const_iterator it = mUpdateAggregateRowMap.begin();
    UpdateAggregateRowMap::const_iterator itend = mUpdateAggregateRowMap.end();
    for (; it != itend; ++it)
    {
        StatValMap::const_iterator valIter = it->second.begin();
        StatValMap::const_iterator valIterEnd = it->second.end();
        for (; valIter != valIterEnd; ++valIter)
        {
            delete valIter->second;
        }
    }

    mUpdatesTable.clear();
    mUpdateRowMap.clear();
    mCategoryUpdateRowMap.clear();
    mUpdateAggregateRowMap.clear();
    mCategoryUpdateAggregateRowMap.clear();
}

void UpdateHelper::buildRow(
    const StatCategory* category, StatRowUpdate* update, int32_t period, ScopesVectorForRows* scopes)
{
    UpdateRowKey key(update->getCategory(), update->getEntityId(),
        period, &update->getKeyScopeNameValueMap());
    UpdateRow row(this, category, scopes, &update->getUpdates(), &update->getKeyScopeNameValueMap());
    mUpdateRowMap.insert(eastl::make_pair(key, row));

    // At the same time we build the main row, also group together all updates for the same
    // entity, period, and scopes in order to evaluate cross-category derived formulae
    CategoryUpdateRowKey catKey(update->getEntityId(), period, &update->getKeyScopeNameValueMap());
    mCategoryUpdateRowMap[catKey].insert(category->getCategoryDependency());
}

/*! ************************************************************************************************/
/*! \brief retrive a cached statValMap for the given aggregate or total row
    \param[in]updateRowKey - all information for a row except scope info
    \param[in]rowScopesVector - scope information for the row 
    \param[in]cat - pointer to the category for the row
    \return cached (current) StatValMap for the specific row
***************************************************************************************************/
UpdateAggregateRowMap::iterator UpdateHelper::getStatValMapIter(const UpdateRowKey& updateRowKey, const RowScopesVector& rowScopesVector, const StatCategory* category)
{
    UpdateAggregateRowKey aggregateKey(updateRowKey.category, updateRowKey.entityId, updateRowKey.period, rowScopesVector);

    // Let's try to find the aggregateKey in the map, if we can find one, we send it's
    // statValMap out, otherwise, we create a new entry in the map and send it's statValMap
    // out to get update
    UpdateAggregateRowMap::iterator it = mUpdateAggregateRowMap.find(aggregateKey);
    if (it == mUpdateAggregateRowMap.end())
    {
        aggregateKey.mStatus = AGG_NEW;
        StatValMap statValMap;
        mUpdateAggregateRowMap.insert(eastl::make_pair(aggregateKey, statValMap));
        it = mUpdateAggregateRowMap.find(aggregateKey);

        // At the same time we build the main row, also group together all updates for the same
        // entity, period, and scopes in order to evaluate cross-category derived formulae
        CategoryUpdateAggregateRowKey catKey(aggregateKey.mEntityId, aggregateKey.mPeriod, aggregateKey.mRowScopesVector);
        mCategoryUpdateAggregateRowMap[catKey].insert(category->getCategoryDependency());
    }

    return it;
}


}// Stats
} // Blaze
