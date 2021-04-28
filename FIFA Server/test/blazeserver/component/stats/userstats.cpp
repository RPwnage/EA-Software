/*************************************************************************************************/
/*!
    \file   userstats.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
/*************************************************************************************************/
/*!
    \class UserStats
    Manages user stats available through the user session
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/query.h"
#include "framework/tdf/userextendeddatatypes_server.h"

#include "stats/tdf/stats_server.h"
#include "stats/rpc/statsslave.h"
#include "statsconfig.h"
#include "statscommontypes.h"
#include "userstats.h"
#include "statsslaveimpl.h"

#include "EAStdC/EAString.h"

namespace Blaze
{
namespace Stats
{

UserStats::~UserStats()
{
    UserCategoryMap::const_iterator catItr = mUserCategoryMap.begin();
    for (; catItr != mUserCategoryMap.end(); ++catItr)
    {
        UserCategory* tmp = catItr->second;
        delete tmp;
    }

    //UserStat map is just a reference, no need to clean up here.
}

void UserStats::updateUserStats(UpdateExtendedDataRequest& request, EntityId entityId, uint32_t categoryId, int32_t periodType, const ScopeNameValueMap& scopeMap, const StatValMap& statValMap) const
{
    UpdateExtendedDataRequest::UserOrSessionToUpdateMap& userMap = request.getUserOrSessionToUpdateMap();

    // Only need to prepare UED updates for users with primary sessions
    UserSessionId primarySessionId = gUserSessionManager->getPrimarySessionId(entityId);
    if (primarySessionId == INVALID_USER_SESSION_ID)
        return;

    UserCategoryMap::const_iterator ic = mUserCategoryMap.find(categoryId);
    if (ic == mUserCategoryMap.end()) 
        return;

    const UserCategory* userCategory = ic->second;
    const UserPeriodMap* periodMap = userCategory->getUserPeriodMap();
    UserPeriodMap::const_iterator ip = periodMap->find(periodType);
    if (ip == periodMap->end()) 
        return;

    const ScopeStatsMap* scopeStatsMap = ip->second;
    ScopeStatsMap::const_iterator scIter = scopeStatsMap->find(&scopeMap);
    if (scIter == scopeStatsMap->end())
        return;

    const UserStatMap* statMap = scIter->second;
    UpdateExtendedDataRequest::ExtendedDataUpdate *update = nullptr;

    StatValMap::const_iterator is = statValMap.begin();
    StatValMap::const_iterator ise = statValMap.end();
    for ( ;is != ise; ++is)
    {
        UserStatMap::const_iterator isu = statMap->find(is->first);
        if (isu != statMap->end())
        {
            if (update == nullptr)
            {
                UpdateExtendedDataRequest::UserOrSessionToUpdateMap::iterator mapIt = userMap.find(entityId);
                if (mapIt == userMap.end())
                {
                    update = userMap.allocate_element();
                    userMap.insert(eastl::make_pair(entityId, update));
                }
                else
                {
                    update = mapIt->second;
                }
            }
            UserExtendedDataMap &map = update->getDataMap();
            const UserStat* userStat = isu->second;
            int32_t statId = UED_KEY_FROM_IDS(StatsSlave::COMPONENT_ID, userStat->getId());
            if (userStat->getStat()->getDbType().getType() != STAT_TYPE_FLOAT)
                map[statId] = is->second->intVal;
            else
            {
                if (is->second->floatVal > INT64_MAX)
                {
                    WARN_LOG("[UserStats].updateUserStats: float value " << is->second->floatVal << " for stat "
                        << userStat->getId() << " will be clamped to max user extended data value " << INT64_MAX << ", for user " << entityId << ".");
                    map[statId] = INT64_MAX;
                }
                else
                    map[statId] = static_cast<UserExtendedDataValue>(is->second->floatVal);
            }
        }
    }
}               

void UserStats::deleteUserStats(UpdateExtendedDataRequest& request, EntityId entityId) const
{
    UserSessionId primarySessionId = gUserSessionManager->getPrimarySessionId(entityId);
    if (primarySessionId == INVALID_USER_SESSION_ID)
        return;

    for (UserCategoryMap::const_iterator ic = mUserCategoryMap.begin(); ic != mUserCategoryMap.end(); ++ic)
    {
        const UserCategory* userCategory = ic->second;
        const UserPeriodMap* periodMap = userCategory->getUserPeriodMap();
        for (UserPeriodMap::const_iterator ip = periodMap->begin(); ip != periodMap->end(); ++ip)
        {
            const ScopeStatsMap* scopeStatsMap = ip->second;
            for (ScopeStatsMap::const_iterator scIter = scopeStatsMap->begin(); scIter != scopeStatsMap->end(); ++scIter)
            {
                const UserStatMap* statMap = scIter->second;
                deleteUserStats(request, entityId, *statMap);
            }
        }
    }
}

void UserStats::deleteUserStats(UpdateExtendedDataRequest& request, EntityId entityId, uint32_t categoryId, int32_t periodType) const
{
    UserSessionId primarySessionId = gUserSessionManager->getPrimarySessionId(entityId);
    if (primarySessionId == INVALID_USER_SESSION_ID)
        return;

    UserCategoryMap::const_iterator ic = mUserCategoryMap.find(categoryId);
    if (ic == mUserCategoryMap.end()) 
        return;

    const UserCategory* userCategory = ic->second;
    const UserPeriodMap* periodMap = userCategory->getUserPeriodMap();
    UserPeriodMap::const_iterator ip = periodMap->find(periodType);
    if (ip == periodMap->end()) 
        return;

    const ScopeStatsMap* scopeStatsMap = ip->second;
    for (ScopeStatsMap::const_iterator scIter = scopeStatsMap->begin(); scIter != scopeStatsMap->end(); ++scIter)
    {
        const UserStatMap* statMap = scIter->second;
        deleteUserStats(request, entityId, *statMap);
    }
}

void UserStats::deleteUserStats(UpdateExtendedDataRequest& request, EntityId entityId, uint32_t categoryId, int32_t periodType, const ScopeNameValueMap& scopeMap) const
{
    UserSessionId primarySessionId = gUserSessionManager->getPrimarySessionId(entityId);
    if (primarySessionId == INVALID_USER_SESSION_ID)
        return;

    UserCategoryMap::const_iterator ic = mUserCategoryMap.find(categoryId);
    if (ic == mUserCategoryMap.end()) 
        return;

    const UserCategory* userCategory = ic->second;
    const UserPeriodMap* periodMap = userCategory->getUserPeriodMap();
    UserPeriodMap::const_iterator ip = periodMap->find(periodType);
    if (ip == periodMap->end()) 
        return;

    const ScopeStatsMap* scopeStatsMap = ip->second;
    ScopeStatsMap::const_iterator scIter = scopeStatsMap->find(&scopeMap);
    if (scIter == scopeStatsMap->end())
        return;

    const UserStatMap* statMap = scIter->second;
    deleteUserStats(request, entityId, *statMap);
}

void UserStats::deleteUserStats(UpdateExtendedDataRequest& request, EntityId entityId, const UserStatMap& statMap) const
{
    UpdateExtendedDataRequest::UserOrSessionToUpdateMap& userMap = request.getUserOrSessionToUpdateMap();
    UpdateExtendedDataRequest::ExtendedDataUpdate *update = nullptr;

    for (UserStatMap::const_iterator isu = statMap.begin(); isu != statMap.end(); ++isu)
    {
        if (update == nullptr)
        {
            UpdateExtendedDataRequest::UserOrSessionToUpdateMap::iterator mapIt = userMap.find(entityId);
            if (mapIt == userMap.end())
            {
                update = userMap.allocate_element();
                userMap.insert(eastl::make_pair(entityId, update));
            }
            else
            {
                update = mapIt->second;
            }
        }
        UserExtendedDataMap &map = update->getDataMap();
        const UserStat* userStat = isu->second;
        int32_t statId = UED_KEY_FROM_IDS(StatsSlave::COMPONENT_ID, userStat->getId());
        if (userStat->getStat()->getDbType().getType() != STAT_TYPE_FLOAT)
            map[statId] = userStat->getStat()->getDefaultIntVal();
        else
        {
            map[statId] = userStat->getStat()->getDefaultFloatVal() > INT64_MAX ? INT64_MAX : static_cast<UserExtendedDataValue>(userStat->getStat()->getDefaultFloatVal());
        }
    }
}               

/*************************************************************************************************/
/*!
    \brief onLoadExtendedData

    Updates user stats in the user extended data map

    Loads data for user predefined stats from db into user session extended data map. If data is
    not found in db the default values defined in the configuration file are used.

    \param[in]  data
    \param[in]  extendedData
    \return - none
*/
/*************************************************************************************************/
BlazeRpcError UserStats::onLoadExtendedData(const UserInfoData &data, UserSessionExtendedData &extendedData)
{
    BlazeId blazeId = data.getId();
    TRACE_LOG("[UserStats].onLoadExtendedData: user ID: " << blazeId);
    bool useDb = (blazeId != INVALID_BLAZE_ID); //if false we'll skip db and use configured defaults
    bool returnErrorOnDbIssue = (gUserSessionManager->getConfig().getAllowLoginOnUEDLookupError() == false);

    QueryPtr query;
    DbConnPtr conn;
    if (useDb)
    {
        uint32_t dbId;
        BlazeRpcError err;
        err = mComponent.getConfigData()->getShardConfiguration().lookupDbId(ENTITY_TYPE_USER, blazeId, dbId);
        if (err != ERR_OK)
        {
            if (returnErrorOnDbIssue)
                return err;
            else 
                useDb = false;
        }
        else
        {
            conn = gDbScheduler->getReadConnPtr(dbId);
            if (conn == nullptr)
            {
                if (returnErrorOnDbIssue)
                {
                    ERR_LOG("[UserStats].onLoadExtendedData: unable to obtain DB connection");
                    return ERR_SYSTEM;
                }
                else 
                    useDb = false;
            }
            else
            {
                query = DB_NEW_QUERY_PTR(conn);
            }
        }
    }
    int32_t periodIds[STAT_NUM_PERIODS];
    mComponent.getPeriodIds(periodIds);
    UserExtendedDataMap& userMap = extendedData.getDataMap();

    // Fill in query using data parsed from "userSessionStats" section
    // in the stats.cfg
    UserCategoryMap::const_iterator it = mUserCategoryMap.begin();
    UserCategoryMap::const_iterator catEnd = mUserCategoryMap.end();
    for (; (useDb && (it != catEnd)); ++it)
    {
        UserCategory* userCategory = it->second;

        const UserPeriodMap* periodMap = userCategory->getUserPeriodMap();
        UserPeriodMap::const_iterator ip = periodMap->begin();
        UserPeriodMap::const_iterator ipe = periodMap->end();
        for (;ip != ipe; ++ip)
        {
            const char8_t* tableName = userCategory->getDbTable(ip->first);
            ScopeStatsMap* scopeStatsMap = ip->second;
            ScopeStatsMap::const_iterator itsc = scopeStatsMap->begin();
            ScopeStatsMap::const_iterator itsce = scopeStatsMap->end();
            for (; itsc != itsce; ++itsc)
            {
                const ScopeNameValueMap* scopeNameValueMap = itsc->first;
                const UserStatMap* statMap = itsc->second;
                
                query->append("SELECT ");   
                
                UserStatMap::const_iterator itStat = statMap->begin();
                UserStatMap::const_iterator itStatEnd = statMap->end();
                for (; itStat != itStatEnd; ++itStat)
                {
                    query->append(" `$s`,", itStat->first);
                }
                query->trim(1);

                query->append(" FROM `$s` WHERE `entity_id` = $D", tableName, blazeId);

                if (ip->first != STAT_PERIOD_ALL_TIME)
                    query->append(" AND `period_id` = $d", periodIds[ip->first]);

                // if it is empty we just don't have scopes
                if (!scopeNameValueMap->empty())
                {
                    ScopeNameValueMap::const_iterator itScValue = scopeNameValueMap->begin();
                    ScopeNameValueMap::const_iterator itScValueEnd = scopeNameValueMap->end();
                    for (;itScValue != itScValueEnd; ++itScValue)
                    {
                        ScopeValue scopeValue = itScValue->second;
                        if (scopeValue == KEY_SCOPE_VALUE_AGGREGATE)
                        {
                            scopeValue = mComponent.getConfigData()->getAggregateKeyValue(itScValue->first.c_str());
                        }
                        query->append(" AND `$s` = $D", itScValue->first.c_str(), scopeValue);
                    }
                }
                
                query->append(";\n");
            }
        }
    }
    
    // If user has no db records, load UED from configured defaults. Or else load it via the db query set above.
    if (!useDb || !query->isEmpty())
    {
        DbResultPtrs results;
        if (useDb)
        {
            BlazeRpcError dbErr = conn->executeMultiQuery(query, results);
            if (dbErr != DB_ERR_OK)
            {
                ERR_LOG("[UserStats].onLoadExtendedData: failed to obtain data: " << ErrorHelp::getErrorName(dbErr) << ".");
                return ERR_SYSTEM;
            }
        }
        
        DbResultPtrs::const_iterator resultItr = results.begin();
        DbResultPtrs::const_iterator resultEnd = results.end();
        UserCategoryMap::const_iterator itr = mUserCategoryMap.begin();
        
        for (; itr != catEnd; ++itr)
        {
            UserCategory* userCategory = itr->second;

            const UserPeriodMap* periodMap = userCategory->getUserPeriodMap();
            UserPeriodMap::const_iterator ip = periodMap->begin();
            UserPeriodMap::const_iterator ipe = periodMap->end();
            for (;ip != ipe; ++ip)
            {
                ScopeStatsMap* scopeStatsMap = ip->second;
                ScopeStatsMap::const_iterator itsc = scopeStatsMap->begin();
                ScopeStatsMap::const_iterator itsce = scopeStatsMap->end();
                for (; itsc != itsce; ++itsc)
                {
                    const UserStatMap* statMap = itsc->second;
                    DbRow* row = nullptr;
                    if (resultItr != resultEnd)
                    {
                        DbResultPtr dbResult = *resultItr;
                        ++resultItr;
                        // we expect only 1 row
                        if (!dbResult->empty())
                            row = *(dbResult->begin());
                    }
                    
                    UserStatMap::const_iterator itStat = statMap->begin();
                    UserStatMap::const_iterator itStatEnd = statMap->end();
                    for (; itStat != itStatEnd; ++itStat)
                    {
                        const UserStat* userStat = itStat->second;
                        int32_t statId = UED_KEY_FROM_IDS(StatsSlave::COMPONENT_ID, userStat->getId());
                
                        if (userStat->getStat()->getDbType().getType() != STAT_TYPE_FLOAT)
                        {
                            // fill default data if skipped db or db returned no result
                            (userMap)[statId] = ((row == nullptr) ? userStat->getStat()->getDefaultIntVal() : row->getInt64(userStat->getStatName()));
                        }
                        else
                        {
                            float floatVal = ((row == nullptr) ? userStat->getStat()->getDefaultFloatVal() : row->getFloat(userStat->getStatName()));
                            if (floatVal > INT64_MAX)
                            {
                                WARN_LOG("[UserStats].onLoadExtendedData: float value " << floatVal << " for stat " << statId << " will be clamped to max user extended data value " << INT64_MAX << ", for user " << blazeId << ".");
                                floatVal = static_cast<float>(INT64_MAX);
                            }
                            (userMap)[statId] = static_cast<UserExtendedDataValue>(floatVal);
                        }
                    }
                }
            }
        }
    } // if (!query->isEmpty())

    return ERR_OK;
}    

/*************************************************************************************************/
/*!
    \brief parseUserStats
    Parses user stats. Establishes links to the corresponding Category and Stat objects and 
    enforces extended data key uniqueness. Data is kept in tree structure defined as following:
    UserCategoryMap->UserPeriodMap->ScopeStatsMap->UserStatsMap->[stats]
    \param[in]  config - stats config file input to be parsed
    \param[in\out] keyMap - mapping of extended data keys used in stats component.
    \param[in]  statsConfig - the translated stats config data 
    \param[out] uedIdMap - mapping of ued names to ids to be filled in
    \param[out] validationErrors - error messages for UserStats config validation
    \return - true if parsing successful, false otherwise
*/
/*************************************************************************************************/
bool UserStats::parseUserStats(const StatsConfig& config, ExtendedDataKeyMap &keyMap,
    StatsConfigData* statsConfig, UserExtendedDataIdMap& uedIdMap, ConfigureValidationErrors& validationErrors)
{
    TRACE_LOG("[UserStats].parseUserStats(): Processing user stats");
    const UserSessionStatsList& userstatsList = config.getUserSessionStats();

    UserSessionStatsList::const_iterator iter = userstatsList.begin();
    UserSessionStatsList::const_iterator end = userstatsList.end();

    for (; iter != end; ++iter)
    {
        const UserSessionStatsData* userstatsData = *iter;
        
        UserStatId statId = userstatsData->getStatid();
        if (statId <= 0 || statId > 0xffff) 
        {
            eastl::string msg;
            msg.sprintf("[UserStats].parseUserStats(): stat: statid  must be > 0 and < 65536");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return false;
        }

        const char8_t* name = userstatsData->getName();
        if (name[0] == '\0') 
        {
            eastl::string msg;
            msg.sprintf("[UserStats].parseUserStats(): stat: keyword <name> not found.");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return false;
        }

        const char8_t* statName = userstatsData->getStatName();
        if (statName[0] == '\0') 
        {
            eastl::string msg;
            msg.sprintf("[UserStats].parseUserStats(): stat: keyword <statName> not found.");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return false;
        }

        if(keyMap.find(statId) != keyMap.end())
        {
            eastl::string msg;
            msg.sprintf("[UserStats].parseUserStats(): stat: %s has duplicate statId %u", statName, statId);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return false;
        }

        int32_t periodType = userstatsData->getPeriodType();
        if (periodType == -1) 
        {
            eastl::string msg;
            msg.sprintf("[UserStats].parseUserStats(): stat: %s: period type not found or invalid.", statName);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return false;
        } 
        
        const char8_t* category = userstatsData->getCategory();
        if (category[0] == '\0') 
        {
            eastl::string msg;
            msg.sprintf("[UserStats].parseUserStats(): stat: %s: keyword <category> not found.", statName);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return false;
        }

        TRACE_LOG("[UserStats].parseUserStats() category: " << category << "  stat: " << statName << " period: " << SbFormats::Raw("0x%02x", periodType));

        const CategoryMap* statsCategoryMap = statsConfig->getCategoryMap();
        CategoryMap::const_iterator catIter = statsCategoryMap->find(category);
        if (catIter == statsCategoryMap->end())
        {
            eastl::string msg;
            msg.sprintf("[UserStats].parseUserStats(): stat: %s: category <%s> is not defined.", statName, category);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return false;
        }
        
        const StatCategory* statCategory = catIter->second;
        const StatMap* statmap = statCategory->getStatsMap();
        StatMap::const_iterator it = statmap->find(statName);
        if (it == statmap->end())
        {
            eastl::string msg;
            msg.sprintf("[UserStats].parseUserStats(): stat: %s is not defined in category: %s.", statName, category);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return false;
        }

        switch (it->second->getDbType().getType())
        {
            case STAT_TYPE_INT:
                break;
            case STAT_TYPE_FLOAT:
                WARN_LOG("[UserStats].parseUserStats(): stat: " << statName << " has db type float. Float values may be truncated/rounded when set as user extended data values.");
                break;
            case STAT_TYPE_STRING:
                eastl::string msg;
                msg.sprintf("[UserStats].parseUserStats(): stat: %s's db type STAT_TYPE_STRING, is not supported for user stats.", statName);
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
                return false;
        };

        // if the stat category has scope settings, we need to make sure the stat here also define
        // a valid scope
        EA::TDF::tdf_ptr<ScopeNameValueMap> scopeNameValueMap;
        if (statCategory->hasScope())
        {
            const ScopeMap& scopeMap = userstatsData->getScope();
            if (scopeMap.size() != 0)
            {
                // note, the scope index here has to be specific, can't be a reserved index
                // like "*" or "?"
                scopeNameValueMap = statsConfig->parseUserStatsScope(scopeMap, validationErrors);

                // make sure the setting matches category scope name settings
                if (!statCategory->isValidScopeNameSet(scopeNameValueMap))
                {
                    eastl::string msg;
                    msg.sprintf("[UserStats].parseUserStats(): stat: %s: category: %s : invalid scope set.", statName, category);
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                    return false;
                }
            }
            else
            {
                eastl::string msg;
                msg.sprintf("[UserStats].parseUserStats(): stat: %s: category: %s has scope setting, however, this user stat does not specifiy any scope.", statName, category);
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
                return false;
            }
        }

        int32_t period = -1;
        switch (periodType)
        {
            case 0x1:
                period = STAT_PERIOD_ALL_TIME;
                break;
            case 0x2:
                period = STAT_PERIOD_MONTHLY;
                break;
            case 0x4:
                period = STAT_PERIOD_WEEKLY;
                break;
            case 0x8:
                period = STAT_PERIOD_DAILY;
                break;
            default:
                break;
        }

        if (period == -1 || !statCategory->isValidPeriod(period))
        {
            eastl::string msg;
            msg.sprintf("[UserStats].parseUserStats(): stat: %s: category: %s: invalid period type: 0x%x", statName, category, periodType);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return false;
        }

        /// @todo cleanup: add nullptr-ptr checks instead of alloc here?
        if (scopeNameValueMap == nullptr)
        {
            scopeNameValueMap = BLAZE_NEW ScopeNameValueMap();
        }

        if (!addStat(statId, name, statCategory, it->second, period, scopeNameValueMap))
        {
            eastl::string msg;
            msg.sprintf("[UserStats].parseUserStats(): UserStat: %s has duplicate in category: %s and period: 0x%x with statName: %s.", 
                name, category, periodType, statName);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return false;
        }
        //insert new key into map.
        keyMap.insert(eastl::make_pair(statId, USER_STAT));

        // Add the name/(component|statId) pair to the map to allow other components to reference the UED by name.
        UserSessionManager::addUserExtendedDataId(uedIdMap, name, StatsSlave::COMPONENT_ID, static_cast<uint16_t>(statId));
    }

    return true;
}

/*************************************************************************************************/
/*!
    \brief addStat
    Adds stat to the userstats object
    \param[in]  id - stat id
    \param[in]  cat - pointer to stat category
    \param[in]  stat - pointer to Stat object
    \param[in]  periodType - period type
    \param[in]  scopeNameValueMap - scope name/value map for the category
    \return - true if stat was added, false if stat is duplicate
*/
/*************************************************************************************************/
bool UserStats::addStat(UserStatId id, const char8_t* name, const StatCategory* cat, const Stat* stat, 
    int32_t periodType, ScopeNameValueMap* scopeNameValueMap)
{
    UserCategory* userCategory = nullptr;
    UserStatMap* statMap = nullptr;
    UserPeriodMap* periodMap = nullptr;
    ScopeStatsMap* scopeStatsMap = nullptr;

    if (mUserStatMap.find(name) != mUserStatMap.end())
    {
        // duplicate UserStat name
        return false;
    }

    UserCategoryMap::const_iterator it = mUserCategoryMap.find(cat->getId());
    if (it == mUserCategoryMap.end())
    {
        userCategory = BLAZE_NEW UserCategory(cat);
        mUserCategoryMap.insert(eastl::make_pair(cat->getId(), userCategory));
    }
    else
    {
        userCategory = it->second;
    }

    periodMap = userCategory->getUserPeriodMap();
    UserPeriodMap::const_iterator itp = periodMap->find(periodType);
    if (itp == periodMap->end()) 
    {
        scopeStatsMap = BLAZE_NEW ScopeStatsMap;
        periodMap->insert(eastl::make_pair(periodType, scopeStatsMap));
    }
    else
    {
        scopeStatsMap = itp->second;
    }    
    
    ScopeStatsMap::iterator itsc = scopeStatsMap->find(scopeNameValueMap);
    if (itsc == scopeStatsMap->end())
    {
        statMap = BLAZE_NEW UserStatMap;
        scopeStatsMap->insert(eastl::make_pair(scopeNameValueMap, statMap));
    }
    else
    {
        statMap = itsc->second;
        UserStatMap::const_iterator itx = statMap->find(stat->getName());
        if (itx != statMap->end()) return false;
    }
    
    const char8_t* nameToKeep = blaze_strdup(name);
    UserStat* userStat = BLAZE_NEW UserStat(id, nameToKeep, stat);
    statMap->insert(eastl::make_pair(stat->getName(), userStat));
    mUserStatMap.insert(eastl::make_pair(nameToKeep, userStat));
    return true;
}


bool UserStats::lookupExtendedData(const UserSessionMaster& session, const char8_t* key, UserExtendedDataValue& value) const
{ 
    //stats will be looked up by id
    int16_t mapKey = (int16_t)EA::StdC::AtoI32(key);
    return session.getDataValue(StatsSlave::COMPONENT_ID, mapKey, value);
}

/*************************************************************************************************/
/*!
    \brief ~UserCategory
    Destructor
*/
/*************************************************************************************************/
UserCategory::~UserCategory()
{
    UserPeriodMap::const_iterator ip = mUserPeriodMap.begin();
    UserPeriodMap::const_iterator ipe = mUserPeriodMap.end();
    for (;ip != ipe; ++ip)
    {
        ScopeStatsMap* scopeStatsMap  = ip->second;
        ScopeStatsMap::const_iterator scIter = scopeStatsMap->begin();
        ScopeStatsMap::const_iterator scEnd = scopeStatsMap->end();
        for (; scIter != scEnd; ++scIter)
        {
            UserStatMap* statMap = scIter->second;
            UserStatMap::const_iterator is = statMap->begin();
            UserStatMap::const_iterator ise = statMap->end();
            for (;is != ise; ++is)
            {
                delete is->second;
            }
        
            delete statMap;            
        }
        delete scopeStatsMap;
    }
}

}
}
