/*************************************************************************************************/
/*!
\file   updatestatsrequestbuilder.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "framework/util/shared/blazestring.h"
#include "framework/controller/controller.h"
#include "stats/statscommontypes.h"
#include "stats/statsconfig.h"
#include "updatestatsrequestbuilder.h"

namespace Blaze
{
namespace Stats
{

UpdateStatsRequestBuilder::UpdateStatsRequestBuilder()
: mOpenRow(nullptr), mOpenNewRow(false)
{
}

UpdateStatsRequestBuilder::~UpdateStatsRequestBuilder()
{
    discardStats();
}

/*************************************************************************************************/
/*!
    \brief startStatRow

    Starts stat row

    \param[in]  category - category name
    \param[in]  entityId - entity ID
    \param[in]  scopeNameValueMap - keyscope name/value map (optional)
*/
/*************************************************************************************************/
void UpdateStatsRequestBuilder::startStatRow(const char8_t* category, EntityId entityId, 
                               const ScopeNameValueMap* scopeNameValueMap)
{
    UpdateRowKey key(category, entityId, 0, scopeNameValueMap); /* key to ensure only unique updates are created */
    StatRowUpdateMap::const_iterator it = mUpdateStatsMap.find(key);
    if (it == mUpdateStatsMap.end())
    {
        // we don't have a cached StatRowUpdate* for this key yet, so create it.
        mOpenRow = getStatUpdates().pull_back(); /* create in-place */
        mOpenRow->setCategory(category);
        mOpenRow->setEntityId(entityId);
        key.category = mOpenRow->getCategory(); /* set to point to memory we own */
        if (scopeNameValueMap != nullptr)
        {
            mOpenRow->getKeyScopeNameValueMap().assign(scopeNameValueMap->begin(), scopeNameValueMap->end()); /* copy the keyscopes */
            key.scopeNameValueMap = &mOpenRow->getKeyScopeNameValueMap(); /* set to point to memory we own */
        }
        mUpdateStatsMap.insert(eastl::make_pair(key, mOpenRow));
        eastl::string statUpdateKey;
        statUpdateKey.sprintf("%s%" PRId64, category, entityId);
        
        // Insert the key if it did not already exist:
        StatUpdateRowKeyMap::iterator rowKeyIter = mStatUpdateRowKeyMap.find(statUpdateKey);
        if (rowKeyIter == mStatUpdateRowKeyMap.end())
            mStatUpdateRowKeyMap.insert(eastl::make_pair(statUpdateKey, key));

        mOpenNewRow = true;
    }
    else
    {
        // already have this row, so make it current
        mOpenRow = it->second;
        mOpenNewRow = false;
    }
}

/*************************************************************************************************/
/*!
    \brief makeStat

    Makes stat for update

    \param[in]  name - stat name
    \param[in]  value - stat value
    \param[in]  updateType - update type

    \return - none
*/
/*************************************************************************************************/
void UpdateStatsRequestBuilder::makeStat(const char8_t* name, const char8_t* value, int32_t updateType)
{
    // FETCH updates don't need to do anything (see selectStat comment).
    if (updateType == STAT_UPDATE_TYPE_FETCH)
        return;

    StatUpdate *statUpdate = mOpenRow->getUpdates().pull_back(); /* create in-place */
    statUpdate->setName(name);
    statUpdate->setUpdateType(updateType);
    statUpdate->setValue(value);
}

/*************************************************************************************************/
/*!
    \brief assignStat

    Assigns stat value, input int64_t

    \param[in]  name - stat name
    \param[in]  value - stat value

    \return - none
*/
/*************************************************************************************************/
void UpdateStatsRequestBuilder::assignStat(const char8_t* name, int64_t value)
{
    char8_t stringValue[32];
    blaze_snzprintf(stringValue, sizeof(stringValue), "%" PRId64, value);
    makeStat(name, stringValue, STAT_UPDATE_TYPE_ASSIGN);
}

/*************************************************************************************************/
/*!
    \brief assignStat

    Assigns stat value, input char8_t

    \param[in]  name - stat name
    \param[in]  value - stat value

    \return - none
*/
/*************************************************************************************************/
void UpdateStatsRequestBuilder::assignStat(const char8_t* name, const char8_t* value)
{
    makeStat(name, value, STAT_UPDATE_TYPE_ASSIGN);
}

/*************************************************************************************************/
/*!
    \brief selectStat

    Selects stat for transformation

    \param[in]  name - stat name

    \return - none
*/
/*************************************************************************************************/
void UpdateStatsRequestBuilder::selectStat(const char8_t* name)
{
   // We're already getting all of the stats in based on startStatRow, we don't need another OP here. 
}

/*************************************************************************************************/
/*!
    \brief clearStat

    Clears stat
    
    \param[in]  name - stat name

    \return - none
*/
/*************************************************************************************************/
void UpdateStatsRequestBuilder::clearStat(const char8_t* name)
{
    makeStat(name, "", STAT_UPDATE_TYPE_CLEAR);
}

/*************************************************************************************************/
/*!
    \brief decrementStat

    Update the stat, subtracting the (signed) input value to it.
    Side: This is equivalent to calling incrementStat on the negative of the input value.

    \param[in]  name - stat name
    \param[in]  value - (signed) value to subtract

    \return - none
*/
/*************************************************************************************************/
void UpdateStatsRequestBuilder::decrementStat(const char8_t* name, int64_t value)
{
    char8_t stringValue[32];
    blaze_snzprintf(stringValue, sizeof(stringValue), "%" PRId64, value);
    makeStat(name, stringValue, STAT_UPDATE_TYPE_DECREMENT);
}

void UpdateStatsRequestBuilder::decrementStat(const char8_t* name, const char8_t* value)
{
    makeStat(name, value, STAT_UPDATE_TYPE_DECREMENT);
}

/*************************************************************************************************/
/*!
    \brief incrementStat

    Update the stat, adding the (signed) input value to it.
    Side: This is equivalent to calling decrementStat on the negative of the input value.
    
    \param[in]  name - stat name
    \param[in]  value - (signed) value to add

    \return - none
*/
/*************************************************************************************************/
void UpdateStatsRequestBuilder::incrementStat(const char8_t* name, int64_t value)
{
    char8_t stringValue[32];
    blaze_snzprintf(stringValue, sizeof(stringValue), "%" PRId64, value);
    makeStat(name, stringValue, STAT_UPDATE_TYPE_INCREMENT);
}

void UpdateStatsRequestBuilder::incrementStat(const char8_t* name, const char8_t* value)
{
    makeStat(name, value, STAT_UPDATE_TYPE_INCREMENT);
}

/*************************************************************************************************/
/*!
    \brief completeStatRow

    Adds stat row to the update list

    \return - none
*/
/*************************************************************************************************/
void UpdateStatsRequestBuilder::completeStatRow()
{
    mOpenRow = nullptr;
    mOpenNewRow = false;
}

/*************************************************************************************************/
/*!
    \brief discardStats

    Discard pending stat row updates, and prepare to process new startStatRow() calls.
*/
/*************************************************************************************************/
void UpdateStatsRequestBuilder::discardStats()
{
    // reset internal data structures to prepare for collecting more row updates in the future
    mOpenRow = nullptr;
    mUpdateStatsMap.clear();
    mStatUpdateRowKeyMap.clear();

    // to free the objects owned by the container call ::release(), because ::clear() will only discard the pointers
    if (!getStatUpdates().empty())
        getStatUpdates().release();
}

const UpdateRowKey* UpdateStatsRequestBuilder::getUpdateRowKey(const char8_t* category, EntityId entityId,
    const ScopeNameValueMap* scopeNameValueMap)
{
    UpdateRowKey key(category, entityId, 0, scopeNameValueMap); /* key to ensure only unique updates are created */
    StatRowUpdateMap::const_iterator it = mUpdateStatsMap.find(key);
    if (it != mUpdateStatsMap.end())
    {
        return &it->first;
    }

    return nullptr;
}

UpdateRowKey* UpdateStatsRequestBuilder::getUpdateRowKey(const char8_t* category, EntityId entityId)
{
    eastl::string statUpdateKey;
    statUpdateKey.sprintf("%s%" PRId64, category, entityId);
    StatUpdateRowKeyMap::iterator rowKeyIter = mStatUpdateRowKeyMap.find(statUpdateKey);
    if (rowKeyIter != mStatUpdateRowKeyMap.end())
    {
        return &rowKeyIter->second;
    }
    return nullptr;
}

} //namespace GameReporting
} //namespace Blaze
