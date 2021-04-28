/*************************************************************************************************/
/*!
    \file   userranks.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
/*************************************************************************************************/
/*!
    \class UserRanks
    Manages user ranking available through the user session
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/query.h"
#include "framework/usersessions/usersessionmanager.h"
#include "stats/tdf/stats_server.h"
#include "stats/tdf/stats.h"
#include "statsslaveimpl.h"
#include "statsconfig.h"
#include "statscommontypes.h"
#include "userranks.h"


namespace Blaze
{
namespace Stats
{


UserRanks::~UserRanks()
{
    UserRankMap::const_iterator itr = mUserRankMap.begin();
    for (; itr != mUserRankMap.end(); ++itr)
    {
        UserRank* tmp = itr->second;
        delete tmp;
    }
}

/*************************************************************************************************/
/*!
    \brief updateUserRanks
    Scans through the user ranks and updates them if match is found
    \param[in]  data - pointer to the stats update object
    \param[in\out] extendedData- the extended data object to update
    \return - none
*/
/*************************************************************************************************/
BlazeRpcError UserRanks::updateUserRanks(const UserInfoData& data, UserSessionExtendedData &extendedData) const
{
    BlazeRpcError err = ERR_OK;
    BlazeId blazeId = data.getId();
    UserExtendedDataMap& userMap = extendedData.getDataMap();
    bool dbIssuesUseDefaultUED = gUserSessionManager->getConfig().getAllowLoginOnUEDLookupError();

    UserRankMap::const_iterator rankIterator = mUserRankMap.begin();
    UserRankMap::const_iterator rankEnd = mUserRankMap.end();
    for(; rankIterator != rankEnd; ++rankIterator)
    {    
        UserRank* userRank = rankIterator->second;
        const StatLeaderboard* leaderboard = userRank->getLeaderboard();
        
        //getLeaderboardRanking will give leaderboard size as rank if user not in LB.
        int32_t ranking = 0;

        //do leaderboard lookup to get user ranking
        int32_t periodType = leaderboard->getPeriodType();
        int32_t currentPeriodId = mComponent.getPeriodId(periodType);
        err = mComponent.getLeaderboardRanking(leaderboard, userRank->getScopeNameValueListMap(), currentPeriodId,
                        blazeId, ranking);

        if(err != ERR_OK)
        {
            //unexpected error
            ERR_LOG("[UserRanks].updateUserRanks(): problem updating user rank " << userRank->getName() << " for user " << data.getId() 
                    << ", getLeaderBoardRanking() returned error " << (ErrorHelp::getErrorName(err)) << ".");
            // set rank to 0 meaning unranked
            ranking = 0;

            if (dbIssuesUseDefaultUED)
            {
                err = ERR_OK;
            }
        }

        int32_t rankId = UED_KEY_FROM_IDS(StatsSlave::COMPONENT_ID, userRank->getId());
        (userMap)[rankId] = ranking;
    }

    return err;
}               

/*************************************************************************************************/
/*!
    \brief onLoadExtendedData
    Updates user ranks in the user extended data map
    \param[in]  data
    \param[in]  extendedData
    \return - none
*/
/*************************************************************************************************/
BlazeRpcError UserRanks::onLoadExtendedData(const UserInfoData &data, UserSessionExtendedData &extendedData)
{
    return updateUserRanks(data, extendedData);
}
/*************************************************************************************************/
/*!
    \brief parseUserRanks
    Parses user ranks. Establishes links to the proper leaderboards and enforces extended data key uniqueness.
    \param[in]  Config - stats config file input to be parsed
    \param[in\out] keyMap - mapping of extended data keys used in stats component.
    \param[in]  statsConfig - the translated stats config data 
    \param[out] uedIdMap - mapping of ued names to ids to be filled in
    \param[out] validationErrors - error messages for UserRanks config validation
    \return - true if parsing successful, false otherwise
*/
/*************************************************************************************************/
bool UserRanks::parseUserRanks(const StatsConfig& config, ExtendedDataKeyMap &keyMap, StatsConfigData* statsConfig,
                               UserExtendedDataIdMap& uedIdMap, ConfigureValidationErrors& validationErrors)
{
    TRACE_LOG("[UserRanks].parseUserRanks(): Processing user ranks");
    const UserSessionRanksList& userranksList = config.getUserSessionRanks();

    UserSessionRanksList::const_iterator iter = userranksList.begin();
    UserSessionRanksList::const_iterator end = userranksList.end();
    for ( ; iter != end; ++iter)
    {
        const UserSessionRanksData* userranksData = *iter;

        UserStatId rankId = userranksData->getRankid();
        if (rankId <= 0 || rankId > 0xffff) 
        {
            eastl::string msg;
            msg.sprintf("[UserRanks].parseUserRanks(): rank: rankid  must be > 0 and < 65536");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return false;
        }

        const char8_t* name = userranksData->getName();
        if (name[0] == '\0') 
        {
            eastl::string msg;
            msg.sprintf("[UserRanks].parseUserRanks(): rank: keyword <name> not found.");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return false;
        }

        if(keyMap.find(rankId) != keyMap.end())
        {
            eastl::string msg;
            msg.sprintf("[UserRanks].parseUserRanks(): rank: %s has duplicate rankId %u", 
                name, rankId);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return false;
        }

        const char8_t* leaderboardName = userranksData->getLeaderboard();
        if (leaderboardName[0] == '\0') 
        {
            eastl::string msg;
            msg.sprintf("[UserRanks].parseUserRanks(): rank: %s: keyword <leaderboard> not found.", name);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return false;
        }

        TRACE_LOG("[UserRanks].parseUserRanks(): leaderboardName: " << leaderboardName << "  name: " << name);

        const StatLeaderboard* statLeaderboard = statsConfig->getStatLeaderboard(0, leaderboardName);
        if(statLeaderboard == nullptr)
        {
            eastl::string msg;
            msg.sprintf("[UserRanks].parseUserRanks(): rank %s: leaderboard: %s not found.", name, leaderboardName);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return false;
        }


        const ScopeMap& scopeMap = userranksData->getScope();
        ScopeNameValueMap userScopeNameValueMap;

        if(scopeMap.size() != 0)
        {
            // parse configured scopes for UserRank
            for (ScopeMap::const_iterator it=scopeMap.begin(),last=scopeMap.end(); it!=last; ++it)
            {
                const char8_t* scopeName      = it->first;
                const char8_t* scopeValueBuff = it->second;
                ScopeValue scopeValue = 0; 

                blaze_str2int(scopeValueBuff, &scopeValue);
                userScopeNameValueMap.insert(eastl::make_pair(scopeName, scopeValue));
            }
        }

        EA::TDF::tdf_ptr<ScopeNameValueListMap> scopeNameValueListMap = BLAZE_NEW ScopeNameValueListMap();
        // Combine leaderboard-defined and user-specified keyscopes in order defined by lb config.
        // Note that we don't care if there are more user-specified keyscopes than needed.
        if (statLeaderboard->getHasScopes())
        {
            const ScopeNameValueListMap* lbScopeNameValueListMap = statLeaderboard->getScopeNameValueListMap();
            ScopeNameValueListMap::const_iterator lbScopeMapItr = lbScopeNameValueListMap->begin();
            ScopeNameValueListMap::const_iterator lbScopeMapEnd = lbScopeNameValueListMap->end();
            for (; lbScopeMapItr != lbScopeMapEnd; ++lbScopeMapItr)
            {
                // any aggregate key indicator has already been replaced with actual aggregate key value

                if (statsConfig->getKeyScopeSingleValue(lbScopeMapItr->second->getKeyScopeValues()) != KEY_SCOPE_VALUE_USER)
                {
                    scopeNameValueListMap->insert(eastl::make_pair(lbScopeMapItr->first, lbScopeMapItr->second->clone()));
                    continue;
                }

                ScopeNameValueMap::const_iterator userScopeMapItr = userScopeNameValueMap.find(lbScopeMapItr->first);
                if (userScopeMapItr == userScopeNameValueMap.end())
                {
                    eastl::string msg;
                    msg.sprintf("[UserRanks].parseUserRanks(): - leaderboard: <%s>: scope value for <%s> is missing for user rank <%s>", 
                        statLeaderboard->getBoardName(), lbScopeMapItr->first.c_str(), name);
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                    return false;
                }

                if (!statsConfig->isValidScopeValue(userScopeMapItr->first, userScopeMapItr->second))
                {
                    eastl::string msg;
                    msg.sprintf("[UserRanks].parseUserRanks(): - leaderboard: <%s>: scope value for <%s> is not defined for user rank <%s>", 
                        statLeaderboard->getBoardName(), lbScopeMapItr->first.c_str(), name);
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                    return false;
                }

                ScopeValues* scopeValues = BLAZE_NEW ScopeValues();
                (scopeValues->getKeyScopeValues())[userScopeMapItr->second] = userScopeMapItr->second;
                scopeNameValueListMap->insert(eastl::make_pair(lbScopeMapItr->first, scopeValues));
            }
        }

        if (!addRank(rankId, name, statLeaderboard, scopeNameValueListMap))
        {
            eastl::string msg;
            msg.sprintf("[UserStats].parseUserRanks(): rank: %s has duplicate in leaderboard: %s.", 
                name, leaderboardName);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return false;
        }
        //insert new key into map.
        keyMap.insert(eastl::make_pair(rankId, USER_RANK));

        // Add the name/(component|rankId) pair to the map to allow other components to reference the UED by name.
        UserSessionManager::addUserExtendedDataId(uedIdMap, name, StatsSlave::COMPONENT_ID, static_cast<uint16_t>(rankId));
    }

    return true;
}

/*************************************************************************************************/
/*!
    \brief addRank
    Adds ranking to the userranks object
    \param[in]  id - stat id
    \param[in]  name - name of this rank object
    \param[in]  leaderboard - pointer to stat leaderboard
    \param[in]  scopeIndexMap - scope index for this rank
    \return - true if rank was added, false if rank is duplicate
*/
/*************************************************************************************************/
bool UserRanks::addRank(UserStatId id, const char8_t* name, const StatLeaderboard* leaderboard, EA::TDF::tdf_ptr<ScopeNameValueListMap>& scopeNameValueListMap)
{
    UserRankMap::const_iterator itx = mUserRankMap.find(name);
    if (itx != mUserRankMap.end()) 
    {
        return false;
    }

    if(scopeNameValueListMap == nullptr)
    {
        // need to create an empty scope index map
        scopeNameValueListMap = BLAZE_NEW ScopeNameValueListMap();
    }

    UserRank* userRank = BLAZE_NEW UserRank(id, name, leaderboard, scopeNameValueListMap);
    mUserRankMap.insert(eastl::make_pair(name, userRank));
    return true;
}


/*************************************************************************************************/
/*!
    \brief getUserRankId
    returns stat id for user rank
    \param[in]  name - rank name
    \return - stat ID or INVALID_STAT_ID if not found
*/
/*************************************************************************************************/
uint16_t UserRanks::getUserRankId( const char8_t* name ) const
{
    UserRankMap::const_iterator itx = mUserRankMap.find(name);
    if (itx == mUserRankMap.end()) 
    {
        return INVALID_STAT_ID;
    }
    else
    {
        return static_cast<uint16_t> (itx->second->getId());
    }
}

/*************************************************************************************************/
/*!
    \brief getUserRank
    Scans through the user ranks and return the rank value if match is found
    \param[in]  blazeId   - Blaze user id
    \param[in]  name - user rank name
    \return - rank value of the user
*/
/*************************************************************************************************/
int32_t UserRanks::getUserRank(BlazeId blazeId, const char8_t* name) const
{
    int32_t ranking = 0;

    UserRankMap::const_iterator rankIterator = mUserRankMap.find(name);
    if (rankIterator != mUserRankMap.end())
    {    
        UserRank* userRank = rankIterator->second;
        const StatLeaderboard* leaderboard = userRank->getLeaderboard();
        
        BlazeRpcError err; 
        //do leaderboard lookup to get user ranking
        int32_t periodType = leaderboard->getPeriodType();
        int32_t currentPeriodId = mComponent.getPeriodId(periodType);
        err = mComponent.getLeaderboardRanking(leaderboard, userRank->getScopeNameValueListMap(), currentPeriodId,
                        blazeId, ranking);

        if (err != ERR_OK)
        {          
            //unexpected error
            ERR_LOG("[UserRanks].getUserRank(): problem getting user rank " << userRank->getName() << " for user " << blazeId 
                    << ", getLeaderBoardRanking() returned error " << (ErrorHelp::getErrorName(err)) << ".");
            // set rank to 0 meaning unranked
            ranking = 0;
        }
    }

    return ranking;
}

bool UserRanks::lookupExtendedData(const UserSessionMaster& session, const char8_t* key, UserExtendedDataValue& value) const
{ 
    int16_t mapKey = (int16_t)EA::StdC::AtoI32(key);
    return session.getDataValue(StatsSlave::COMPONENT_ID, mapKey, value);
}

BlazeRpcError UserRanks::onRefreshExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData)
{
    return updateUserRanks(data, extendedData);
}

/*************************************************************************************************/
/*!
    \brief scopeNameValueMapsEqual
    Compares two ScopeNameValueMaps
    \param[in]  map1 - pointer to the map 1
    \param[in]  map2 - pointer to the map 2
    \return - if equal true otherwise false
*/
/*************************************************************************************************/
bool UserRanks::scopeNameValueMapsEqual(const ScopeNameValueListMap* map1, const ScopeNameValueMap* map2) const
{
    // the nullptr pointer means the same as size == 0: no scopes
    // if both have no scopes it is also fine
    size_t sz1 = 0;
    size_t sz2 = 0;
    if (map1 != nullptr) 
        sz1 = map1->size();
    if (map2 != nullptr) 
        sz2 = map2->size();
    if (sz1 != sz2)
        return false;
    if (sz1 == 0) 
        return true;

    ScopeNameValueListMap::const_iterator it1 = map1->begin();
    ScopeNameValueListMap::const_iterator it1End = map1->end();
    ScopeNameValueMap::const_iterator it2End = map2->end();
    for (; it1 != it1End; ++it1)
    {
        if (map2->find(it1->first.c_str()) == it2End)
            return false;
    }
    
    return true;
}


/*************************************************************************************************/
/*!
    \brief updateUserRanks
    Scans through the stats updates and if categories and keyscopes match one of leaderboards 
    used by user ranks then it updates corresponding ranks
    \param[in]  statList - update data (tdf)
    \return - none
*/
/*************************************************************************************************/
void UserRanks::updateUserRanks(UpdateExtendedDataRequest& request, const CacheRowUpdateList& statList) const
{
    CacheRowUpdateList::const_iterator it = statList.begin();
    CacheRowUpdateList::const_iterator ite = statList.end();
    for (;it != ite; ++it)
    {
        const CacheRowUpdate* row = *it;
        const uint8_t* bytes = row->getBinaryData().getData();
        updateUserRanks(request, *((const EntityId*)(bytes+4)), *((const uint32_t*)(bytes)), *((const int32_t*)(bytes+12)));
    }                
}

void UserRanks::updateUserRanks(UpdateExtendedDataRequest& request, const CacheRowDeleteList& statList) const
{
    for (CacheRowDeleteList::const_iterator it = statList.begin(); it != statList.end(); ++it)
    {
        const CacheRowDelete* row = *it;
        for (PeriodTypeList::const_iterator pit = row->getPeriodTypes().begin(); pit != row->getPeriodTypes().end(); ++pit)
            updateUserRanks(request, row->getEntityId(), row->getCategoryId(), *pit);
    }
}

void UserRanks::updateUserRanks(UpdateExtendedDataRequest& request, EntityId entityId, uint32_t categoryId, int32_t periodType) const
{
    // Rank updated by only slave user logged in to prevent multiple updates
    UserSessionId primarySessionId = gUserSessionManager->getPrimarySessionId(entityId);
    if (primarySessionId == INVALID_USER_SESSION_ID)
        return;

    UpdateExtendedDataRequest::UserOrSessionToUpdateMap& userMap = request.getUserOrSessionToUpdateMap();
    UpdateExtendedDataRequest::ExtendedDataUpdate *update = nullptr;

    UserRankMap::const_iterator rankIter = mUserRankMap.begin();
    UserRankMap::const_iterator rankEnd = mUserRankMap.end();
    for (; rankIter != rankEnd; ++rankIter)
    {
        // if categories do not match it does not affect the rank
        const UserRank* userRank = rankIter->second;
        const StatLeaderboard* leaderboard = userRank->getLeaderboard();
        if (leaderboard->getStatCategory()->getId() != categoryId)
            continue;

        if (leaderboard->getPeriodType() != periodType)
            continue;

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
            

        // Note, somewhat inefficient to fetch ranking regardless of whether the keyscopes associated with this user rank were in fact
        // part of the stat update itself.
        int32_t ranking = 0;
        int32_t lbPeriodType = leaderboard->getPeriodType();
        int32_t currentPeriodId = mComponent.getPeriodId(lbPeriodType);
        mComponent.getLeaderboardRanking(leaderboard, userRank->getScopeNameValueListMap(), currentPeriodId, entityId, ranking);

        int32_t rankId = UED_KEY_FROM_IDS(StatsSlave::COMPONENT_ID, userRank->getId());
        update->getDataMap()[rankId] = ranking;
    }
}

}
}
