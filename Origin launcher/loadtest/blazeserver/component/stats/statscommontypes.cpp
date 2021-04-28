/*************************************************************************************************/
/*!
    \file   statscommontypes.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "statscommontypes.h"
#include "statsconfig.h"
#include "framework/util/shared/blazestring.h"

namespace Blaze
{
namespace Stats
{
    
/*************************************************************************************************/
/*!
    \brief init

    Initialize a StatCategory.

    \param[in] id - an internal ID for the category
    \param[in] entityType - type of entities in this category (users, clubs, etc.)
    \param[in] statMap - a map of Stat objects keyed by the name of the stat
    \param[in] scopeNameSet - collection of scope names for this category
*/
/*************************************************************************************************/
void StatCategory::init(uint32_t id, EA::TDF::ObjectType entityType, StatMap* statMap, ScopeNameSet* scopeNameSet)
{
    clean(); // clean fields

    mCategoryId = id;
    mCategoryEntityType = entityType;
    mStatMap = statMap;
    mScopeNameSet = scopeNameSet;
    
    for (int32_t i = 0; i < STAT_NUM_PERIODS; ++i)
    {
        if ((1 << i) & mPeriodTypes)
        {
            blaze_snzprintf(mTableNames[i], STATS_MAX_TABLE_NAME_LENGTH, "stats_%s_%s", STAT_PERIOD_NAMES[i], mName.c_str());
            blaze_strlwr(mTableNames[i]);
        }
        else
        {
            mTableNames[i][0] = '\0';
        }
    }
}

void StatCategory::clean()
{
    if (mStatMap != nullptr)
    {
        mStatMap->clear();
        delete mStatMap;
        mStatMap = nullptr;
    }

    // clean scope information it the category has scope settings
    if (mScopeNameSet != nullptr)
    {
        mScopeNameSet->clear();
        delete mScopeNameSet;
        mScopeNameSet = nullptr;
    }

    for (int32_t i = 0; i < STAT_NUM_PERIODS; ++i)
        mTableNames[i][0] = '\0';

    mCategoryDependency = nullptr;
    mCategoryEntityType = EA::TDF::ObjectType();
}

/*************************************************************************************************/
/*!
    \brief ~StatCategory

    Destroy and cleanup the memory owned by a stat category.
*/
/*************************************************************************************************/
StatCategory::~StatCategory()
{
    clean();
}

/*! ************************************************************************************************/
/*! \brief validate the ScopeNameValueMap setting in the request is right for the category it belongs to
    
    \param[in]config - config obj to retrieve category keyscope info
    \param[in]scopeNameValueMap - list of scope name/value pairs to validate

    \return true if the ScopeNameValueMap is valid, otherwise false
***************************************************************************************************/
bool StatCategory::validateScopeForCategory(const StatsConfigData& config, const ScopeNameValueMap& scopeNameValueMap) const
{
    if (hasScope())
    {
        if (mScopeNameSet->size() != scopeNameValueMap.size())
        {
            WARN_LOG("[StatCategory].validateScopeForCategory(): "
                "Number of keyscopes in category and request do not match");
            return false;
        }

        ScopeNameValueMap::const_iterator it = scopeNameValueMap.begin();
        ScopeNameValueMap::const_iterator itend = scopeNameValueMap.end();
        for (; it != itend; ++it)
        {
            // if the scope name is defined for the category
            if (isValidScopeName(it->first) == false)
            {
                WARN_LOG("[StatCategory].validateScopeForCategory(): key scope " << it->first.c_str() 
                         << " in the request is not defined for the category " << mName << ".");
                return false;
            }

            if (it->second == KEY_SCOPE_VALUE_AGGREGATE)
            {
                if (config.getAggregateKeyValue(it->first) >= 0)
                {
                    // aggregate key supported
                    continue;
                }
                // otherwise...
                WARN_LOG("[StatCategory].validateScopeForCategory(): key scope " << it->first.c_str() 
                         << " category does not define an aggregate key.");
                return false;
            }

            if (it->second == KEY_SCOPE_VALUE_ALL)
            {
                continue;
            }

            if (!config.isValidScopeValue(it->first, it->second))
            {
                WARN_LOG("[StatCategory].validateScopeForCategory(): key scope " << it->first.c_str() 
                         << " has invalid value " << it->second << ".");
                return false;
            }
        }
    }
    else if (!scopeNameValueMap.empty())
    {
        WARN_LOG("[StatCategory].validateScopeForCategory(): "
            "Request specifies keyscopes while category " << mName << " has none.");
        return false;
    }

    return true;
}

/*! ************************************************************************************************/
/*! \brief check if the passed in scope name list matches settings in category 

    \param[in] ScopeNameValueMap - a list of scope name/value pairs

    Note: this function does not check the scope value, we assume the value is already validated at 
        this point

    \return true if the all scope names in category are found in the passed-in list
        and all scope names in the passed-in list are found in the category scope settings.
***************************************************************************************************/
bool StatCategory::isValidScopeNameSet(const ScopeNameValueMap* scopeNameValueMap) const
{
    if (hasScope())
    {
        if (scopeNameValueMap == nullptr || scopeNameValueMap->empty())
        {
            ERR_LOG("[StatCategory].isValidScopeNameSet(): stat scope for category [" << mName << "] does not define any required scope name and value.");
            return false;
        }

        if (mScopeNameSet->size() != scopeNameValueMap->size())
        {
            ERR_LOG("[StatCategory].isValidScopeNameSet(): Number of stat scope does not match config for category [" << mName << "]");
            return false;
        }

        ScopeNameSet::const_iterator catScopeIt = mScopeNameSet->begin();
        ScopeNameSet::const_iterator catScopeItend = mScopeNameSet->end();
        for (; catScopeIt != catScopeItend; ++catScopeIt)
        {
            if (scopeNameValueMap->find(*catScopeIt) == scopeNameValueMap->end())
            {
                ERR_LOG("[StatCategory].isValidScopeNameSet(): category [" << mName << "] is missing value for scope [" << (*catScopeIt).c_str() << "]");
                return false;
            }
        }
    }
    else if (scopeNameValueMap != nullptr && !scopeNameValueMap->empty())
    {
        ERR_LOG("[StatCategory].isValidScopeNameSet(): scopes provided for category (" << mName << ") that does not have scope settings");
        return false;
    }

    return true;
}

/*! ************************************************************************************************/
/*! \brief check if the passed in scope name list matches settings in category

    This method is basically an exact copy of the previous one, with just a different arg type.
    @todo consolidate the two via templates or some other means

    \param[in] scopeNameValueListMap - a list of scope name/valuelist pairs

    Note: this function does not check the scope value, we assume the value is already validated at
        this point

    \return true if the all scope names in category are found in the passed-in list
        and all scope names in the passed-in list are found in the category scope settings.
***************************************************************************************************/
bool StatCategory::isValidScopeNameSet(const ScopeNameValueListMap* scopeNameValueListMap) const
{
    if (hasScope())
    {
        if (scopeNameValueListMap == nullptr || scopeNameValueListMap->empty())
        {
            ERR_LOG("[StatCategory].isValidScopeNameSet(): stat scope for category [" << mName << "] does not define any required scope name and value.");
            return false;
        }

        if (mScopeNameSet->size() != scopeNameValueListMap->size())
        {
            ERR_LOG("[StatCategory].isValidScopeNameSet(): Number of stat scope does not match config for category [" << mName << "]");
            return false;
        }

        ScopeNameSet::const_iterator catScopeIt = mScopeNameSet->begin();
        ScopeNameSet::const_iterator catScopeItend = mScopeNameSet->end();
        for (; catScopeIt != catScopeItend; ++catScopeIt)
        {
            if (scopeNameValueListMap->find(*catScopeIt) == scopeNameValueListMap->end())
            {
                ERR_LOG("[StatCategory].isValidScopeNameSet(): category [" << mName << "] is missing value for scope [" << (*catScopeIt).c_str() << "]");
                return false;
            }
        }
    }
    else if (scopeNameValueListMap != nullptr && !scopeNameValueListMap->empty())
    {
        ERR_LOG("[StatCategory].isValidScopeNameSet(): scopes provided for category (" << mName << ") that does not have scope settings");
        return false;
    }

    return true;
}

/*! ************************************************************************************************/
/*! \brief check if the passed in scope name lists matches settings in category

    \param[in] scopeNameValueListMap - a list of scope name/valuelist pairs
    \param[in] inheritedScopeNameValueListMap - a list of scope name/valuelist pairs

    Note: this function does not check the scope value, we assume the value is already validated at
        this point

    \return true if the all scope names in category are found in EITHER of the passed-in lists
        and all scope names in the FIRST passed-in list are found in the category scope settings.
***************************************************************************************************/
bool StatCategory::isValidScopeNameSet(const ScopeNameValueListMap* scopeNameValueListMap, const ScopeNameValueListMap* inheritedScopeNameValueListMap) const
{
    if (hasScope())
    {
        // check the category scopes
        ScopeNameSet::const_iterator catScopeIt = mScopeNameSet->begin();
        ScopeNameSet::const_iterator catScopeItend = mScopeNameSet->end();
        for (; catScopeIt != catScopeItend; ++catScopeIt)
        {
            if (scopeNameValueListMap == nullptr || scopeNameValueListMap->find(*catScopeIt) == scopeNameValueListMap->end())
            {
                if (inheritedScopeNameValueListMap == nullptr || inheritedScopeNameValueListMap->find(*catScopeIt) == inheritedScopeNameValueListMap->end())
                {
                    ERR_LOG("[StatCategory].isValidScopeNameSet(): category [" << mName << "] is missing value for scope [" << (*catScopeIt).c_str() << "]");
                    return false;
                }
            }
        }

        // check the scopes in the first (non-inherited) list
        if (scopeNameValueListMap != nullptr)
        {
            ScopeNameValueListMap::const_iterator nvItr = scopeNameValueListMap->begin();
            ScopeNameValueListMap::const_iterator nvEnd = scopeNameValueListMap->end();
            for (; nvItr != nvEnd; ++nvItr)
            {
                if (mScopeNameSet->find(nvItr->first) == mScopeNameSet->end())
                {
                    ERR_LOG("[StatCategory].isValidScopeNameSet(): category [" << mName << "] does not define value for scope [" << nvItr->first.c_str() << "]");
                    return false;
                }
            }
        }
    }
    else if (scopeNameValueListMap != nullptr && !scopeNameValueListMap->empty())
    {
        ERR_LOG("[StatCategory].isValidScopeNameSet(): scopes provided for category (" << mName << ") that does not have scope settings");
        return false;
    }

    return true;
}

/*! ************************************************************************************************/
/*! \brief check if the passed in scope name is supported by the category

    \param[in] scopeName - scopeName to check

    \return true if the scope name is supported by the category
***************************************************************************************************/
bool StatCategory::isValidScopeName(const char8_t* scopeName)const
{
    if (mScopeNameSet == nullptr)
        return false;

    return (mScopeNameSet->find(scopeName) != mScopeNameSet->end());
}


/*************************************************************************************************/
/*!
    \brief init

    Initialize a StatGroup.

    \param[in] category - primary category this group is associated with
    \param[in] scopeNameValueMap - list of scope name value pairs
*/
/*************************************************************************************************/
void StatGroup::init(const StatCategory* category, ScopeNameValueMap* scopeNameValueMap)
{
    clean(); // clean fields

    mStatCategory = category;
    mScopeNameValueMap = scopeNameValueMap;
}

void StatGroup::buildSelectableCategories()
{
    // reserve max possible space needed for selects
    //(can't be greater than the number of stat descs)
    mSelectList.reserve(getStats().size());
    
    // always put primary category into first position
    // (safe to use a reference because push_back will 
    // never reallocate, as we reserve the mem upfront)
    Select& primary = mSelectList.push_back();
    primary.mCategory = getStatCategory();
    primary.mScopes = getScopeNameValueMap();
    // primary category often contains the majority of the group stats
    primary.mDescs.reserve(getStats().size());

    // walk all stats in the group and build a unique list of
    // categories with their corresponding stats attached
    for (StatDescList::const_iterator di = getStats().begin(), de = getStats().end(); di != de; ++di)
    {
        Select* sel = nullptr;
        const StatDesc& desc = **di;
        for (SelectList::iterator i = mSelectList.begin(), e = mSelectList.end(); i != e; ++i)
        {
            if (i->isCompatibleWith(desc))
            {
                sel = &(*i);
                break;
            }
        }
        if (sel == nullptr)
        {
            sel = &mSelectList.push_back();
            sel->mCategory = &desc.getStat()->getCategory();
            sel->mScopes = desc.getScopeNameValueMap();
        }
        sel->mDescs.push_back(&desc);
    }
}

/*! ***************************************************************************/
/*! \brief  isCompatibleWith - Stat descriptor compatibility test

            Only stat descriptors that have the
            a) same parent category
            b) same period type
            c) same scope map contents
            are able to be retrieved using a 
            single select statement.
            
            The exception is: desc.empty() == true, then any
            descriptor with matching category is compatible.
            
    \param  const StatDesc & desc 
    \return bool returns true if a stat descriptor can be part of the Select
*******************************************************************************/
bool StatGroup::Select::isCompatibleWith(const StatDesc& desc) const
{
    if (mCategory != &desc.getStat()->getCategory())
        return false;
    if (mDescs.empty())
        return true;
    const StatDesc& first = *mDescs.front();
    if (desc.getStatPeriodType() != first.getStatPeriodType())
        return false;
    const ScopeNameValueMap* hisScopes = desc.getScopeNameValueMap();
    if (mScopes == nullptr && hisScopes == nullptr)
        return true;
    if (mScopes != nullptr && hisScopes != nullptr)
    {
        if (mScopes->size() != hisScopes->size())
            return false;
        ScopeNameValueMap::const_iterator i = mScopes->begin();
        ScopeNameValueMap::const_iterator e = mScopes->end();
        for (; i != e; ++i)
        {
            ScopeNameValueMap::const_iterator j = hisScopes->find(i->first);
            // if keys or values do not match, then not compatible
            if (j == hisScopes->end() || i->second != j->second)
                return false;
        }
        return true;
    }
    return false;
}

int32_t StatGroup::Select::getPeriodType() const
{
    // all descs have the same period type!
    return mDescs.front()->getStatPeriodType();
}

void StatGroup::clean()
{
    // clean the scope name/scope value map if the group has scope settings
    mScopeNameValueMap = nullptr;   
    mStatCategory = nullptr;
}

/*************************************************************************************************/
/*!
    \brief ~StatGroup

    Destroy a StatGroup object and all the StatDesc objects it owns.
*/
/*************************************************************************************************/
StatGroup::~StatGroup()
{
    clean();
}

// ------------------------------ Leaderboards -----------------------------
/*************************************************************************************************/
/*!
    \brief init

    Initialize a LeaderboardGroup.

    \param[in] category - primary category this group is associated with
*/
/*************************************************************************************************/
void LeaderboardGroup::init(const StatCategory* category)
{
    clean(); // clean fields

    mStatCategory = category;
}

void LeaderboardGroup::clean()
{
    mStatCategory = nullptr;
}

/*************************************************************************************************/
/*!
    \brief ~LeaderboardGroup

    Destroy a LeaderboardGroup object.
*/
/*************************************************************************************************/
LeaderboardGroup::~LeaderboardGroup()
{
    clean();
}

/*! ***************************************************************************/
/*! \brief  buildSelectableCategories

    Identify and store unique combinations of categories, periods and
    keyscopes in the leaderboard group.

    \param  periodType - period type of leaderboard

    \return BlazeRpcError result - expected to be ERR_OK given pre-validation
*******************************************************************************/
void LeaderboardGroup::buildSelectableCategories()
{
    // reserve max possible space needed for selects
    //(can't be greater than the number of stat descs)
    mSelectList.reserve(getStats().size());

    // always put primary category into first position
    // (safe to use a reference because push_back will 
    // never reallocate, as we reserve the mem upfront)
    Select& primary = mSelectList.push_back();
    primary.mPrimary = true;
    primary.mCategory = getStatCategory();

    // NOTE we don't set the primary scopes since the helper does not require it

    // primary category often contains the majority of the group stats
    primary.mDescs.reserve(getStats().size());

    // walk all stats in the group and build a unique list of
    // categories with their corresponding stats attached
    for (LbStatList::const_iterator di = getStats().begin(), de = getStats().end(); di != de; ++di)
    {
        Select* sel = nullptr;
        const GroupStat& desc = **di;
        for (SelectList::iterator i = mSelectList.begin(), e = mSelectList.end(); i != e; ++i)
        {
            if (i->isCompatibleWith(desc))
            {
                sel = &(*i);
                break;
            }
        }
        if (sel == nullptr)
        {
            sel = &mSelectList.push_back();
            sel->mCategory = &desc.getStat()->getCategory();
            sel->mScopes = desc.getScopeNameValueListMap();
        }
        sel->mDescs.push_back(&desc);

        mSelectMap[&desc] = sel;
    }
}

/*! ***************************************************************************/
/*! \brief  isCompatibleWith - Stat descriptor compatibility test

            Only stat descriptors that have the
            a) same parent category
            b) same period type
            c) same scope map contents
            are able to be retrieved using a
            single select statement.
 
            The exception is: desc.empty() == true, then any
            descriptor with matching category is compatible.

    \param  const GroupStat & desc
    \return bool returns true if a stat descriptor can be part of the Select
*******************************************************************************/
bool LeaderboardGroup::Select::isCompatibleWith(const GroupStat& desc) const
{
    if (mCategory != &desc.getStat()->getCategory())
        return false;
    if (mDescs.empty())
        return true;
    const GroupStat& first = *mDescs.front();
    if (desc.getStatPeriodType() != first.getStatPeriodType())
        return false;
    const ScopeNameValueListMap* hisScopes = desc.getScopeNameValueListMap();
    // stats for the primary category wouldn't have their own keyscopes
    if ((mPrimary || mScopes == nullptr) && hisScopes == nullptr)
        return true;
    if (mScopes != nullptr && hisScopes != nullptr)
    {
        if (mScopes->size() != hisScopes->size())
            return false;
        ScopeNameValueListMap::const_iterator i = mScopes->begin();
        ScopeNameValueListMap::const_iterator e = mScopes->end();
        for (; i != e; ++i)
        {
            ScopeNameValueListMap::const_iterator j = hisScopes->find(i->first);
            // if keys do not match, then not compatible
            if (j == hisScopes->end())
                return false;

            // if values do not match, then not compatible
            const ScopeStartEndValuesMap& myValues = i->second->getKeyScopeValues();
            const ScopeStartEndValuesMap& hisValues = j->second->getKeyScopeValues();
            if (myValues.size() != hisValues.size())
                return false;
            ScopeStartEndValuesMap::const_iterator k = myValues.begin();
            ScopeStartEndValuesMap::const_iterator ke = myValues.end();
            ScopeStartEndValuesMap::const_iterator l = hisValues.begin();
            for (; k != ke; ++k, ++l)
            {
                if (k->first != l->first || k->second != l->second)
                    return false;
            }
        }
        return true;
    }
    return false;
}

int32_t LeaderboardGroup::Select::getPeriodType() const
{
    // all descs have the same period type!
    return mDescs.front()->getStatPeriodType();
}

/*************************************************************************************************/
/*!
    \brief getdbtablename

    Provides name of the DB table containing stats for the leaderboard
    
    \return - DB table name
*/
/*************************************************************************************************/
const char8_t* StatLeaderboard::getDBTableName() const
{
    return mLeaderboardGroup->getStatCategory()->getTableName(getPeriodType());
}


/*! ************************************************************************************************/
/*! \brief append scope information to db query when fetching a leaderboard

    Note: caller is responsible for allocate memory for scopeQuery

    \param[in]destLen - length of destination buffer
    \param[out]scopeQuery - string generated for scope setting
    \return true if there is scope setting for this leaderboard and string is not empty, otherwise false
***************************************************************************************************/
bool StatLeaderboard::appendScopeToDBQuery(char8_t* scopeQuery, size_t destLen)
{
    if (scopeQuery == nullptr)
    {
        EA_FAIL_MSG("passed in string pointer is invalid");
        return false;
    }

    // clear the content
    scopeQuery[0] = '\0';
    char8_t scopeUnit[128] = {0};
    if (getScopeNameValueListMap() != nullptr)
    {
        ScopeNameValueListMap::const_iterator scopeIt = getScopeNameValueListMap()->begin();
        ScopeNameValueListMap::const_iterator scopeEnd = getScopeNameValueListMap()->end();
        for (; scopeIt != scopeEnd; ++scopeIt)
        {
            /// @todo investigate DB performance and optimize potentially long query length
            // Could be creating queries with many (hundreds or even thousands of?) consecutive
            // numbers separated by commas.
            const ScopeValues* scopeValues = scopeIt->second;
            bool first = true;

            blaze_strnzcat(scopeQuery, " AND `", destLen);
            blaze_strnzcat(scopeQuery, scopeIt->first.c_str(), destLen);
            blaze_strnzcat(scopeQuery, "` IN (", destLen);

            ScopeStartEndValuesMap::const_iterator valuesItr = scopeValues->getKeyScopeValues().begin();
            ScopeStartEndValuesMap::const_iterator valuesEnd = scopeValues->getKeyScopeValues().end();
            for (; valuesItr != valuesEnd; ++valuesItr)
            {
                ScopeValue scopeValue;
                for (scopeValue = valuesItr->first; scopeValue <= valuesItr->second; ++scopeValue)
                {
                    if (!first)
                    {
                        blaze_strnzcat(scopeQuery, ",", destLen);
                    }

                    blaze_snzprintf(scopeUnit, sizeof(scopeUnit), "%" PRId64, scopeValue);
                    blaze_strnzcat(scopeQuery, scopeUnit, destLen);
                    scopeUnit[0] = '\0';
                    first = false;
                }
            }

            blaze_strnzcat(scopeQuery, ")", destLen);
        }

        return true;
    }

    return false;
}

/*************************************************************************************************/

} // Stats
} // Blaze
