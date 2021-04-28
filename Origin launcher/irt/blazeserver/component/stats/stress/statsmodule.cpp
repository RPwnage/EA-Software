/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class StatsModule

    Stress tests stats component.
    ---------------------------------------
    Issues RPC calls to the Stats component according hit rates and timing defined in stats_stress.cfg 
    Collected test data is logged to the 'stats_stressXXX.log'. XXX is a sequential number, 
    which automatically incremented to preserve results of previous tests
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"

#include "statsmodule.h"
#include "loginmanager.h"
#include "framework/connection/selector.h"
#include "framework/config/config_file.h"
#include "framework/config/config_boot_util.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/config/configdecoder.h"
#include "framework/rpc/usersessions_defines.h"
#include "framework/util/random.h"
#include "blazerpcerrors.h"
#include "clubs/rpc/clubs_defines.h"
#include "stats/tdf/stats.h"
#include "stats/tdf/stats_server.h"



namespace Blaze
{
namespace Stress
{

    typedef BlazeRpcError (StatsInstance::*ActionFun)(int32_t);

/*
 * List of available RPCs that can be called by the module.
 */

typedef struct {
    const char8_t* rpcName;
    ActionFun action;
    double hitRate;
    int32_t wMin;
    int32_t wMax;
    int32_t rowCtr; // number of rows defined in config
    uint32_t resRowCtr; // total number of rows involved in operation
    uint32_t perRowCtr;  // the same per period
    uint32_t ctr; // total counter
    uint32_t errCtr; //error counter;
    uint32_t perCtr; // counter per measurement period
    uint32_t perErrCtr; // error counter per period
    int64_t durTotal;
    int64_t durPeriod;
    uint32_t users;
    uint32_t usersPer;
    TimeValue periodStartTime;
} RpcDescriptor;

RpcDescriptor rpcList[] = {
    {"updateStats", &StatsInstance::updateStats},
    {"getLeaderboard", &StatsInstance::getLeaderboard},
    {"getCenteredLeaderboard", &StatsInstance::getCenteredLeaderboard},
    {"getFilteredLeaderboard", &StatsInstance::getFilteredLeaderboard},
    {"getUserSetLeaderboardCached", &StatsInstance::getUserSetLeaderboardCached},
    {"getUserSetLeaderboardUncached", &StatsInstance::getUserSetLeaderboardUncached},
    {"getStatsByGroup", &StatsInstance::getStatsByGroup},
    {"getStatsByGroupAsync", &StatsInstance::getStatsByGroupAsync},
    {"getLeaderboardTree", &StatsInstance::getLeaderboardTree},
    {"getStats", &StatsInstance::getStats},
    {"getLeaderboardFolder", &StatsInstance::getLeaderboardFolder},
    {"getStatGroup", &StatsInstance::getStatGroup},
    {"getLeaderboardGroup", &StatsInstance::getLeaderboardGroup},
    {"deleteStats", &StatsInstance::deleteStats},
    {"getEntityCount", &StatsInstance::getEntityCount},
    {"getLeaderboardEntityCount", &StatsInstance::getLeaderboardEntityCount},
    {"updateNormalGameStats", &StatsInstance::updateNormalGameStats},
    {"updateClubGameStats", &StatsInstance::updateClubGameStats},
    {"updateSoloGameStats", &StatsInstance::updateSoloGameStats},
    {"updateSponsorEventGameStats", &StatsInstance::updateSponsorEventGameStats},
    {"updateOtpGameStats", &StatsInstance::updateOtpGameStats},
    {"getKeyScopesMap", &StatsInstance::getKeyScopesMap},
    {"getStatGroupList", &StatsInstance::getStatGroupList},
    {"getStatDescs", &StatsInstance::getStatDescs},
    {"updateGlobalStats", &StatsInstance::updateGlobalStats},
    {"getGlobalStats", &StatsInstance::getGlobalStats}
};

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

BlazeRpcError StatsInstance::execute()
{
    BlazeRpcError result = ERR_OK;
    
    int32_t rnd = Random::getRandomNumber(Random::MAX);
    
    for (int j = 0; j < mOwner->getRpcCount(); j++)
    {
        if (rpcList[j].hitRate == 0) continue;
        
        if (rnd < rpcList[j].wMin || rnd > rpcList[j].wMax) continue;
        
        mRpcIndex = j;
        TimeValue startTime = TimeValue::getTimeOfDay();
        Fiber::setCurrentContext(rpcList[j].rpcName);
        result = (this->*(rpcList[j].action))(j);
        TimeValue endTime = TimeValue::getTimeOfDay();
        TimeValue durTime = endTime - startTime;
        
        int64_t dur = durTime.getMicroSeconds();
        rpcList[j].durTotal += dur;
        rpcList[j].durPeriod += dur;
        rpcList[j].ctr++;
        rpcList[j].perCtr++;
        if (result != ERR_OK)
        {
            rpcList[j].errCtr++;
            rpcList[j].perErrCtr++;
        }
        
        break;
    }
    
    return result;
}
// ============= RPCs ================
BlazeRpcError StatsInstance::getKeyScopesMap(int32_t index)
{        
    BlazeRpcError result = ERR_OK;
    KeyScopes resp;
    
    result = mProxy->getKeyScopesMap(resp);

    return result;
}

// ===================================
BlazeRpcError StatsInstance::getLeaderboard(int32_t index)
{        
    BlazeRpcError result = ERR_OK;
    const Stats::StatLeaderboard* lb = mOwner->nextLeaderboard();
    LeaderboardStatsRequest req;
    LeaderboardStatValues resp;

    mOwner->fillLeaderboardRequestScopes(req.getKeyScopeNameValueMap(), lb);

    req.setBoardName(lb->getBoardName()); 
    req.setRankStart(mOwner->getLBStartRank()); 
    req.setCount(rpcList[index].rowCtr);
    req.setPeriodOffset(mOwner->getRandomPeriodOffset());
    result = mProxy->getLeaderboard(req, resp);

    if (result == ERR_OK)
    {
        rpcList[index].resRowCtr += rpcList[index].rowCtr;
        rpcList[index].perRowCtr += rpcList[index].rowCtr;
    }
    return result;
}

// ===================================
BlazeRpcError StatsInstance::getFilteredLeaderboard(int32_t index)
{
    BlazeRpcError result = ERR_OK;
    const Stats::StatLeaderboard* lb = mOwner->nextLeaderboard();
    FilteredLeaderboardStatsRequest req;
    LeaderboardStatValues resp;

    mOwner->fillLeaderboardRequestScopes(req.getKeyScopeNameValueMap(), lb);

    req.setBoardName(lb->getBoardName());

    for (int32_t j = 0; j < rpcList[index].rowCtr; ++j)
    {
        EntityId entityId = mOwner->getRandomEntityId();
        req.getListOfIds().push_back(entityId);
    }
    
    req.setPeriodOffset(mOwner->getRandomPeriodOffset());
    
    result = mProxy->getFilteredLeaderboard(req, resp);

    if (result == ERR_OK)
    {
        rpcList[index].resRowCtr += rpcList[index].rowCtr;
        rpcList[index].perRowCtr += rpcList[index].rowCtr;
    }

    return result;
}
// ===================================
BlazeRpcError StatsInstance::getCenteredLeaderboard(int32_t index)
{
    BlazeRpcError result = ERR_OK;
    const Stats::StatLeaderboard* lb = mOwner->nextLeaderboard();
    CenteredLeaderboardStatsRequest req;
    LeaderboardStatValues resp;

    mOwner->fillLeaderboardRequestScopes(req.getKeyScopeNameValueMap(), lb);

    req.setBoardName(lb->getBoardName());
    req.setCount(rpcList[index].rowCtr);
    req.setCenter(mOwner->getRandomEntityId());
    req.setPeriodOffset(mOwner->getRandomPeriodOffset());
    
    result = mProxy->getCenteredLeaderboard(req, resp);
    
    if (result == ERR_OK)
    {
        rpcList[index].resRowCtr += rpcList[index].rowCtr;
        rpcList[index].perRowCtr += rpcList[index].rowCtr;
    }
    
    return result;
}

// ===================================
BlazeRpcError StatsInstance::getUserSetLeaderboardBase(int32_t index, bool isInMemory)
{
#ifdef TARGET_clubs
    BlazeRpcError result = ERR_OK;

    const Stats::StatLeaderboard* lb = mOwner->nextLeaderboard();
    const Stats::StatLeaderboard* lbStart = lb;

    while (true)
    {
        const Stats::StatCategory* cat = lb->getStatCategory();
        if (cat->getCategoryEntityType() == ENTITY_TYPE_USER && lb->isInMemory() == isInMemory)
        {
            break;
        }
        // try next lb
        lb = mOwner->nextLeaderboard();
        if (lb == lbStart)
        {
            // no suitable category...
            BLAZE_ERR_LOG(BlazeRpcLog::stats,"[StatsInstance].getUserSetLeaderboardBase: " << (isInMemory ? "isInMemory" : "not InMemory") 
                          << " user categories");
            return STATS_ERR_UNKNOWN_CATEGORY;
        }
    }

    LeaderboardStatsRequest req;
    LeaderboardStatValues resp;

    mOwner->fillLeaderboardRequestScopes(req.getKeyScopeNameValueMap(), lb);

    req.setBoardName(lb->getBoardName());
    req.setCount(rpcList[index].rowCtr);
    req.setPeriodOffset(mOwner->getRandomPeriodOffset());

    // using club, but other entities will do as well
    BlazeId clubId = static_cast<BlazeId>(Random::getRandomNumber(mOwner->getMaxClubCount()) + 1);
    req.setUserSetId(EA::TDF::ObjectId(Clubs::ENTITY_TYPE_CLUB, clubId));

    result = mProxy->getLeaderboard(req, resp);

    if (result == ERR_OK)
    {
        rpcList[index].resRowCtr += rpcList[index].rowCtr;
        rpcList[index].perRowCtr += rpcList[index].rowCtr;
    }

    return result;
#else
    return ERR_NOT_SUPPORTED;
#endif
}

BlazeRpcError StatsInstance::getUserSetLeaderboardCached(int32_t index)
{
    return getUserSetLeaderboardBase(index, true);
}

BlazeRpcError StatsInstance::getUserSetLeaderboardUncached(int32_t index)
{
    return getUserSetLeaderboardBase(index, false);
}

// ===================================
BlazeRpcError StatsInstance::getLeaderboardGroup(int32_t index)
{
    // Go through all leaderboard groups available 
    BlazeRpcError result = ERR_OK;
    const Stats::StatLeaderboard* lb = mOwner->nextLeaderboard();
    LeaderboardGroupRequest req;
    LeaderboardGroupResponse resp;
    req.setBoardName(lb->getBoardName());
    result = mProxy->getLeaderboardGroup(req, resp);
    return result;
}
// ===================================
BlazeRpcError StatsInstance::getStatGroup(int32_t index)
{
    // Go through all Stats groups available 
    BlazeRpcError result = ERR_OK;
    const Stats::StatGroup* sg = mOwner->nextStatGroup();
    GetStatGroupRequest req;
    StatGroupResponse resp;
    req.setName(sg->getName());
    result = mProxy->getStatGroup(req, resp);
    return result;
}

// ===================================
BlazeRpcError StatsInstance::getStatGroupList(int32_t index)
{
    BlazeRpcError result = ERR_OK;
    StatGroupList resp;
    result = mProxy->getStatGroupList(resp);

    return result;
}

// ===================================
BlazeRpcError StatsInstance::getStatDescs(int32_t index)
{
    BlazeRpcError result = ERR_OK;
    GetStatDescsRequest req;
    StatDescs resp;
    const StatCategory* cat = mOwner->nextCategory();
    const Stat* stat = cat->getStatsMap()->begin()->second;
    
    req.setCategory(cat->getName());
    req.getStatNames().push_back(stat->getName());

    result = mProxy->getStatDescs(req, resp);

    return result;
}

// ===================================
BlazeRpcError StatsInstance::getLeaderboardFolder(int32_t index)
{
    BlazeRpcError result = ERR_OK;
    LeaderboardFolderGroupRequest req;
    LeaderboardFolderGroup resp;
    req.setFolderId(0);
    req.setFolderName(mOwner->nextLeaderboardFolderName());
    result = mProxy->getLeaderboardFolderGroup(req, resp);
    return result;
}

BlazeRpcError StatsInstance::getLeaderboardTree(int32_t index)
{
    BlazeRpcError result = ERR_OK;
    GetLeaderboardTreeRequest req;
    req.setFolderName(mOwner->nextLeaderboardFolderName());
    result = mProxy->getLeaderboardTreeAsync(req);
    return result;
}

BlazeRpcError StatsInstance::getStats(int32_t index)
{
    BlazeRpcError result = ERR_OK;
    GetStatsRequest req;
    const StatCategory* cat = mOwner->nextCategory();
    req.setCategory(cat->getName());

    mOwner->nextScopeNameValueMap(req.getKeyScopeNameValueMap(), cat);

    for (int32_t j = 0; j < rpcList[index].rowCtr; ++j)
    {
        req.getEntityIds().push_back(mOwner->getRandomEntityId());
    }

    const StatMap* statMap = cat->getStatsMap();
    StatMap::const_iterator statIter = statMap->begin();
    //StatMap::const_iterator statEnd = statMap->end();
    int32_t size = (int32_t) statMap->size();
    int32_t n = Blaze::Random::getRandomNumber(size+1);
    for (int32_t j=0; j<n; ++j)
    {
        req.getStatNames().push_back(statIter->first);
        ++statIter;
    }

    uint32_t periodType;
    do
    {
        periodType = Blaze::Random::getRandomNumber(STAT_NUM_PERIODS);
    }    
    while(!cat->isValidPeriod(periodType));
    
    req.setPeriodType(periodType);
    
    int32_t periodOff = mOwner->getRandomPeriodOffset();      
    req.setPeriodOffset(periodOff);

    GetStatsResponse resp;
    result = mProxy->getStats(req, resp);
    
    if (result == ERR_OK)
    {
        rpcList[index].resRowCtr += rpcList[index].rowCtr;
        rpcList[index].perRowCtr += rpcList[index].rowCtr;
    }
    
    return result;
}

BlazeRpcError StatsInstance::getStatsByGroupAsync(int32_t index)
{
    return getStatsByGroupBase(index, true);
}

BlazeRpcError StatsInstance::getStatsByGroup(int32_t index)
{
    return getStatsByGroupBase(index, false);
}

BlazeRpcError StatsInstance::getStatsByGroupBase(int32_t index, bool async)
{
    BlazeRpcError result = ERR_OK;
    GetStatsByGroupRequest req;
    const Stats::StatGroup* sg = mOwner->nextStatGroup();

    mOwner->fillStatGroupRequestScopes(req.getKeyScopeNameValueMap(), sg);

    req.setGroupName(sg->getName());

    for (int32_t j = 0; j < rpcList[index].rowCtr; ++j)
    {
        req.getEntityIds().push_back(mOwner->getRandomEntityId());
    }

    uint32_t periodType;
    do
    {
        periodType = Blaze::Random::getRandomNumber(STAT_NUM_PERIODS);
    }    
    while(!sg->getStatCategory()->isValidPeriod(periodType));
    
    req.setPeriodType(periodType);
    
    int32_t periodOff = mOwner->getRandomPeriodOffset();      
    req.setPeriodOffset(periodOff);
    
    if (async)
    {
        result = mProxy->getStatsByGroupAsync(req);
    }
    else
    {
        GetStatsResponse resp;
        result = mProxy->getStatsByGroup(req, resp);
    }

    if (result == ERR_OK)
    {
        rpcList[index].resRowCtr += rpcList[index].rowCtr;
        rpcList[index].perRowCtr += rpcList[index].rowCtr;
    }
    
    return result;
}
// ===================================
BlazeRpcError StatsInstance::deleteStats(int32_t index)
{
    BlazeRpcError result = ERR_OK;
    WipeStatsRequest req;
  
    const StatCategory* cat = mOwner->nextCategory();
    req.setCategoryName(cat->getName());
    req.setEntityId(mOwner->getRandomEntityId());

    mOwner->getScopeNameValueMapByEntity(req.getKeyScopeNameValueMap(), cat, req.getEntityId());

    req.setOperation(WipeStatsRequest::DELETE_BY_CATEGORY_KEYSCOPE_ENTITYID);

    result = mProxy->wipeStats(req);
    return result;
}

// ===================================
BlazeRpcError StatsInstance::updateStats(int32_t index)
{
    BlazeRpcError result = ERR_OK;
    UpdateStatsRequest req;
    char8_t buf[256];

    int32_t rowNum = rpcList[index].rowCtr;
    EntityId* entities = BLAZE_NEW_ARRAY(EntityId, rowNum);
    for (int32_t j=0; j<rowNum; ++j)
    {
        const Stats::StatCategory* category = mOwner->nextCategory();
        EntityId entity;
        int32_t m; 
        StatRowUpdate* row = req.getStatUpdates().pull_back();
        row->setCategory(category->getName());
        // get unique entity id within this call
        do
        {
            entity = mOwner->getRandomEntityId(); 
            for (m=0; m<j; m++)
            {
                if (entities[m] == entity) break ;
            }
        }
        while (m != j);
        entities[j] = entity;
        
        row->setEntityId(entity); // if new create otherwise update
        //row->getPeriodTypes()->push_back(0); // all time for now

        // add keyscopes
        mOwner->nextScopeNameValueMap(row->getKeyScopeNameValueMap(), category, true);

        const Stats::StatMap* statMap =  category->getStatsMap();
        Stats::StatMap::const_iterator sIter = statMap->begin();
        Stats::StatMap::const_iterator sIterE = statMap->end();
        
        for (; sIter != sIterE; ++sIter)
        {
            const Stats::Stat* cStat = sIter->second;
            if (cStat->isDerived()) continue;
            
            StatUpdate* stat = row->getUpdates().pull_back();
            stat->setName(cStat->getName());
            
            switch (cStat->getDbType().getType())
            {
                case Stats::STAT_TYPE_INT:
                {
                    if (cStat->getTypeSize() < 2)
                    {
                        blaze_snzprintf(buf, sizeof(buf), "%d", 1 + Blaze::Random::getRandomNumber(100));
                    }
                    else
                    {
                        blaze_snzprintf(buf, sizeof(buf), "%d", 1 + Blaze::Random::getRandomNumber(16000));
                    }
                }
                break;
                case Stats::STAT_TYPE_STRING:
                {
                    int32_t len = 8;
                    if (cStat->getTypeSize() > 9)
                    {
                        len += Blaze::Random::getRandomNumber(cStat->getTypeSize() - 8);
                        if (len > 72)
                        {
                            len = 72;
                        }
                    }
                    for (int32_t i =0; i <len; i++)
                    {
                        buf[i] = 'a' + (char8_t)Blaze::Random::getRandomNumber('z' - 'a' + 1);
                    }
                    buf[len] = 0;
                }
                break;
                case Stats::STAT_TYPE_FLOAT:
                double d = ((double)entity)/(double) (1 + Blaze::Random::getRandomNumber(100)) ;
                blaze_snzprintf(buf, 256, "%.2f", d);
                break;
            }
            stat->setValue(buf);

            stat->setUpdateType(Stats::STAT_UPDATE_TYPE_ASSIGN); // TODO may be something better
        }        
    }

    delete[] entities;

    result = mProxy->updateStats(req);
    
    if (result == ERR_OK) 
    {
        rpcList[index].resRowCtr += rowNum;
        rpcList[index].perRowCtr += rowNum;
    }
    
    return result;
}


// ================= Global stats ==================
BlazeRpcError StatsInstance::getGlobalStats(int32_t index)
{
    // update one row only but all the stats found in it

    BlazeRpcError result = ERR_OK;
    GetStatsRequest req;
    const Stats::StatCategory* category =
        mOwner->getStatsConfig()->getCategoryMap()->find(
            mOwner->getGlobalCategoryName())->second;

    req.setCategory(category->getName());

    for (int32_t i = 1; i < rpcList[index].rowCtr + 1; i++)
        req.getEntityIds().push_back(i);

    const StatMap* statMap = category->getStatsMap();
    StatMap::const_iterator statIter = statMap->begin();
    int32_t size = (int32_t) statMap->size();
    for (int32_t j=0; j<size; ++j)
    {
        req.getStatNames().push_back(statIter->first);
        ++statIter;
    }
    
    req.setPeriodType(STAT_PERIOD_ALL_TIME);
    req.setPeriodOffset(0);

    GetStatsResponse resp;
    result = mProxy->getStats(req, resp);

    if (result == ERR_OK)
    {
        KeyScopeStatsValueMap &ksmap = resp.getKeyScopeStatsValueMap();
        if (ksmap.size() != 1)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::stats,"[StatsInstance].getGlobalStats: " 
                          << " expected 1 result but got " << ksmap.size() << " in keyScopeValueMap.");
            result = ERR_SYSTEM;
        }
        else if (ksmap.begin()->second->getEntityStatsList().size() != (size_t)rpcList[index].rowCtr)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::stats,"[StatsInstance].getGlobalStats: " 
                          << " expected " << rpcList[index].rowCtr << " results but got " 
                          << ksmap.begin()->second->getEntityStatsList().size() 
                          << " in entityStatList.");
            result = ERR_SYSTEM;
        }
    }

    if (result == ERR_OK)
    {
        rpcList[index].resRowCtr += rpcList[index].rowCtr;
        rpcList[index].perRowCtr += rpcList[index].rowCtr;
    }
    
    return result;
}

BlazeRpcError StatsInstance::updateGlobalStats(int32_t index)
{
    BlazeRpcError result = ERR_OK;
    UpdateStatsRequest req;

    const Stats::StatCategory* category =
        mOwner->getStatsConfig()->getCategoryMap()->find(
            mOwner->getGlobalCategoryName())->second;

    // each time update rows for each entity in the category table
    for (int32_t j=0; j < rpcList[index].rowCtr; ++j)
    {
        StatRowUpdate* row = req.getStatUpdates().pull_back();
        row->setCategory(category->getName());
        row->setEntityId((EntityId)(j + 1)); 

        const Stats::StatMap* statMap =  category->getStatsMap();
        Stats::StatMap::const_iterator sIter = statMap->begin();
        Stats::StatMap::const_iterator sIterE = statMap->end();

        // increment all stats by 1
        for (; sIter != sIterE; ++sIter)
        {
            const Stats::Stat* cStat = sIter->second;
            // skip derived
            if (cStat->isDerived()) continue;
            
            StatUpdate* stat = row->getUpdates().pull_back();
            stat->setName(cStat->getName());
            stat->setValue("1");

            stat->setUpdateType(Stats::STAT_UPDATE_TYPE_INCREMENT); 
        }
    }

    result = mProxy->updateStats(req);
    
    if (result == ERR_OK) 
    {
        rpcList[index].resRowCtr += rpcList[index].rowCtr;
        rpcList[index].perRowCtr += rpcList[index].rowCtr;
    }
    
    return result;
}


// ================= Normal game stats ==================
BlazeRpcError StatsInstance::updateNormalGameStats(int32_t index)
{
    BlazeRpcError result = ERR_OK;
    UpdateStatsRequest req;
    uint32_t gameUserCtr = 0;
    EntityId entity[2];    

    entity[0] = mOwner->getRandomEntityId();
    do {
        entity[1] = mOwner->getRandomEntityId();
        } while (entity[0] == entity[1]);

    uint32_t numberOfUsers = 2;
    CategoryList::const_iterator catEnd = mOwner->getNormalGames()->end();
    for (uint32_t userNo = 0; userNo < numberOfUsers; ++userNo)
    {
        CategoryList::const_iterator catIter = mOwner->getNormalGames()->begin();
        for (; catIter != catEnd; ++catIter)
        {
            const StatCategory* cat = *catIter;
            fillStats(&req, cat, entity[userNo]);
        }
        ++gameUserCtr;
    }

    result = mProxy->updateStats(req);
    
    if (result == ERR_OK) 
    {
        rpcList[index].users += gameUserCtr;
        rpcList[index].usersPer += gameUserCtr;
    }
    
    return result;
}

// ===================================
BlazeRpcError StatsInstance::updateClubGameStats(int32_t index)
{
#ifdef TARGET_clubs
    const uint32_t CLUB_MEMBER_MAX = 50;
    const uint32_t TEAM_MEMBER_MAX = 11;
    
    BlazeRpcError result = ERR_OK;
    UpdateStatsRequest req;
    BlazeId clubId[2];
    uint32_t gameUserCtr = 0;

    // Pick two random clubs to play
    clubId[0] = static_cast<BlazeId>(Random::getRandomNumber(mOwner->getMaxClubCount()) + 1);
    do {
            clubId[1] = static_cast<BlazeId>(Random::getRandomNumber(mOwner->getMaxClubCount()) + 1);
       } while (clubId[0] == clubId[1]);
    
    // fill club stats
    for (uint32_t clubNo = 0; clubNo < 2; ++clubNo)
    {
        CategoryList::const_iterator catIter = mOwner->getClubGames()->begin();
        CategoryList::const_iterator catEnd = mOwner->getClubGames()->end();
        for (; catIter != catEnd; ++catIter)
        {
            const StatCategory* cat = *catIter;
            if (cat->getCategoryEntityType() != Clubs::ENTITY_TYPE_CLUB) continue;
            fillStats(&req, cat, clubId[clubNo]);
        }
    }
        
    uint32_t teamMemCount = Blaze::Random::getRandomNumber(TEAM_MEMBER_MAX) + 1;
        
    // Let's pick 2 * 11 distinct users for 2 teams
    EntityId entity[TEAM_MEMBER_MAX * 2];

    // pick players and fill their stats
    for (uint32_t clubNo = 0; clubNo < 2; ++clubNo)
    {
        uint32_t pId;
        for (uint32_t j=0; j<teamMemCount; ++j)
        {
            uint32_t playerNo = clubNo*teamMemCount + j;
            bool tryAgain;
            do {
                    tryAgain = false;
                    pId = (uint32_t)(clubId[clubNo] - 1) * CLUB_MEMBER_MAX + Blaze::Random::getRandomNumber(CLUB_MEMBER_MAX) + 1;
                    for (uint32_t m=0; m < playerNo; ++m)
                    {
                        if (entity[m] == pId)
                        {    
                            tryAgain = true;
                            break;
                        }   
                    }
                } while (tryAgain);
                
            entity[playerNo] = pId;
        }

        ScopeNameValueMap clubScopeMap;
        clubScopeMap.insert(eastl::make_pair("club", clubId[clubNo]));

        for (uint32_t playerNo = 0; playerNo < teamMemCount; ++playerNo)
        {
            CategoryList::const_iterator catIter = mOwner->getClubGames()->begin();
            CategoryList::const_iterator catEnd = mOwner->getClubGames()->end();
            for (; catIter != catEnd; ++catIter)
            {
                const StatCategory* cat = *catIter;
                if (cat->getCategoryEntityType() != ENTITY_TYPE_USER) continue;
                fillStats(&req, cat, entity[clubNo * teamMemCount + playerNo], &clubScopeMap);
            }
            ++gameUserCtr; // user finished the game
        }

        clubScopeMap.clear();
    }
    result = mProxy->updateStats(req);
    
    if (result == ERR_OK) 
    {
        // gamecount
        rpcList[index].users += gameUserCtr;
        rpcList[index].usersPer += gameUserCtr;
    }
    
    return result;
#else
    return ERR_NOT_SUPPORTED;
#endif
}

// ===================================
BlazeRpcError StatsInstance::updateSoloGameStats(int32_t index)
{
    BlazeRpcError result = ERR_OK;
    UpdateStatsRequest req;
    uint32_t gameUserCtr = 0;

    EntityId entity  = mOwner->getRandomEntityId();

    CategoryList::const_iterator catIter = mOwner->getSoloGames()->begin();
    CategoryList::const_iterator catEnd = mOwner->getSoloGames()->end();
    for (; catIter != catEnd; ++catIter)
    {
        const StatCategory* cat = *catIter;
        if (cat->getCategoryEntityType() != ENTITY_TYPE_USER) continue;
        fillStats(&req, cat, entity);
    }
    ++gameUserCtr; // user finished the game

    result = mProxy->updateStats(req);
    
    if (result == ERR_OK) 
    {
        // gamecount
        rpcList[index].users += gameUserCtr;
        rpcList[index].usersPer += gameUserCtr;
    }
    
    return result;
}

// ===================================
BlazeRpcError StatsInstance::updateSponsorEventGameStats(int32_t index)
{
    BlazeRpcError result = ERR_OK;
    UpdateStatsRequest req;
    uint32_t gameUserCtr = 0;
    EntityId entity[2];    


    // assume that it could be normal 1:1 or solo 1:0 game
    entity[0] = mOwner->getRandomEntityId();
    do {
        entity[1] = mOwner->getRandomEntityId();
        } while (entity[0] == entity[1]);

    uint32_t numberOfUsers = Blaze::Random::getRandomNumber(2) + 1;
    CategoryList::const_iterator catEnd = mOwner->getSponsorEventGames()->end();
    for (uint32_t userNo = 0; userNo < numberOfUsers; ++userNo)
    {
        CategoryList::const_iterator catIter = mOwner->getSponsorEventGames()->begin();
        for (; catIter != catEnd; ++catIter)
        {
            const StatCategory* cat = *catIter;
            fillStats(&req, cat, entity[userNo]);
        }
        ++gameUserCtr;
    }

    result = mProxy->updateStats(req);
    
    if (result == ERR_OK) 
    {
        rpcList[index].users += gameUserCtr;
        rpcList[index].usersPer += gameUserCtr;
    }
    
    return result;
}

// ===================================
BlazeRpcError StatsInstance::updateOtpGameStats(int32_t index)
{
    const uint32_t TEAM_MEMBER_MAX = 11;
    
    BlazeRpcError result = ERR_OK;
    UpdateStatsRequest req;
    BlazeId clubId[2];
    uint32_t gameUserCtr = 0;

    // Pick two random clubs/groups/teams to play
    clubId[0] = static_cast<BlazeId>(Random::getRandomNumber(mOwner->getMaxClubCount()) + 1);
    do {
            clubId[1] = static_cast<BlazeId>(Random::getRandomNumber(mOwner->getMaxClubCount()) + 1);
       } while (clubId[0] == clubId[1]);

    const uint32_t teamMemCount = Blaze::Random::getRandomNumber(TEAM_MEMBER_MAX) + 1;
    
    // Let's pick 2 * 11 distinct users for 2 teams
    EntityId entity[TEAM_MEMBER_MAX * 2];

    // pick players and fill their stats
    for (uint32_t clubNo = 0; clubNo < 2; ++clubNo)
    {
        uint32_t pId;
        for (uint32_t j=0; j<teamMemCount; ++j)
        {
            uint32_t playerNo = clubNo*teamMemCount + j;
            bool tryAgain;
            do {
                    tryAgain = false;
                    pId = (uint32_t)(clubId[clubNo] - 1) * TEAM_MEMBER_MAX + Blaze::Random::getRandomNumber(TEAM_MEMBER_MAX) + 1;
                    for (uint32_t m=0; m < playerNo; ++m)
                    {
                        if (entity[m] == pId)
                        {    
                            tryAgain = true;
                            break;
                        }   
                    }
                } while (tryAgain);
                
            entity[playerNo] = pId;
        }

        ScopeNameValueMap clubScopeMap;
        clubScopeMap.insert(eastl::make_pair("club", clubId[clubNo]));

        for (uint32_t playerNo = 0; playerNo < teamMemCount; ++playerNo)
        {
            CategoryList::const_iterator catIter = mOwner->getOtpGames()->begin();
            CategoryList::const_iterator catEnd = mOwner->getOtpGames()->end();
            for (; catIter != catEnd; ++catIter)
            {
                const StatCategory* cat = *catIter;
                if (cat->getCategoryEntityType() != ENTITY_TYPE_USER) continue;
                fillStats(&req, cat, entity[clubNo * teamMemCount + playerNo], &clubScopeMap);
            }
            ++gameUserCtr; // user finished the game
        }

        clubScopeMap.clear();
    }
    result = mProxy->updateStats(req);
    
    if (result == ERR_OK) 
    {
        // gamecount
        rpcList[index].users += gameUserCtr;
        rpcList[index].usersPer += gameUserCtr;
    }
    
    return result;
}

// ============= ******* ================
void StatsInstance::fillStats(UpdateStatsRequest* req, const StatCategory* cat, EntityId entity, ScopeNameValueMap* scopeOverride)
{
    char8_t buf[256];

    StatRowUpdate* row = req->getStatUpdates().pull_back();
    row->setCategory(cat->getName());
    row->setEntityId(entity);

    // update all period types - leave list empty

    // add keyscopes
    mOwner->getScopeNameValueMapByEntity(row->getKeyScopeNameValueMap(), cat, entity);
    if (scopeOverride != nullptr)
    {
        // replace only existing keyscopes in our row update with any found in override
        ScopeNameValueMap::const_iterator scopeItr = scopeOverride->begin();
        ScopeNameValueMap::const_iterator scopeEnd = scopeOverride->end();
        for (; scopeItr != scopeEnd; ++scopeItr)
        {
            ScopeNameValueMap::iterator nvItr = row->getKeyScopeNameValueMap().find(scopeItr->first);
            if (nvItr != row->getKeyScopeNameValueMap().end())
            {
                nvItr->second = scopeItr->second;
            }
        }
    }

    const Stats::StatMap* statMap =  cat->getStatsMap();
    Stats::StatMap::const_iterator sIter = statMap->begin();
    Stats::StatMap::const_iterator sIterE = statMap->end();
    
    for (; sIter != sIterE; ++sIter)
    {
        const Stats::Stat* cStat = sIter->second;
        if (cStat->isDerived()) continue;
        
        StatUpdate* stat = row->getUpdates().pull_back();
        stat->setName(cStat->getName());
        
        switch (cStat->getDbType().getType())
        {
            case Stats::STAT_TYPE_INT:
            {
                if (cStat->getTypeSize() < 2)
                {
                    blaze_snzprintf(buf, sizeof(buf), "%d", 1 + Blaze::Random::getRandomNumber(100));
                }
                else
                {
                    blaze_snzprintf(buf, sizeof(buf), "%d", 1 + Blaze::Random::getRandomNumber(16000));
                }

                // most of int stats are incremented
                int32_t rnd = Blaze::Random::getRandomNumber(7);
                if (rnd == 0)
                {                    
                    stat->setUpdateType(Stats::STAT_UPDATE_TYPE_ASSIGN);
                }
                else
                {
                    stat->setUpdateType(Stats::STAT_UPDATE_TYPE_INCREMENT);
                }
                break;
            }
            case Stats::STAT_TYPE_STRING:
            {
                int32_t len = 8;
                if (cStat->getTypeSize() > 9)
                {
                    len += Blaze::Random::getRandomNumber(cStat->getTypeSize() - 8);
                    if (len > 72)
                    {
                        len = 72;
                    }
                }
                for (int32_t i =0; i <len; i++)
                {
                    char8_t chr = ' ' + (char8_t) Blaze::Random::getRandomNumber('z' - 'a' + 1);
                    if (chr > ' ') chr += 'a' - ' ' - 1 ; 
                    buf[i] = chr;
                }
                buf[len] = 0;
                stat->setUpdateType(Stats::STAT_UPDATE_TYPE_ASSIGN);
                break;
            }
            case Stats::STAT_TYPE_FLOAT:
            {
                double d = ((double)entity)/(double) (1 + Blaze::Random::getRandomNumber(100)) ;
                blaze_snzprintf(buf, 256, "%.2f", d);
                stat->setUpdateType(Stats::STAT_UPDATE_TYPE_ASSIGN);
                break;
            }
        }
        stat->setValue(buf);
    }
}

// ==============================
BlazeRpcError StatsInstance::getEntityCount(int32_t index)
{
    BlazeRpcError result = ERR_OK;
    Stats::GetEntityCountRequest req;
    Stats::EntityCount resp;
    const StatCategory* cat = mOwner->nextCategory();
    req.setCategory(cat->getName());
    req.setPeriodType(mOwner->getRandomPeriodType(cat));
    req.setPeriodOffset(mOwner->getRandomPeriodOffset());

    mOwner->nextScopeNameValueMap(req.getKeyScopeNameValueMap(), cat);

    result = mProxy->getEntityCount(req, resp);
    
    if (result == ERR_OK)
    {
        rpcList[index].resRowCtr +=  resp.getCount();
        rpcList[index].perRowCtr +=  resp.getCount();
    }
    
    return result;
}
// =============================
BlazeRpcError StatsInstance::getLeaderboardEntityCount(int32_t index)
{
    BlazeRpcError result = ERR_OK;
    LeaderboardEntityCountRequest req;
    Stats::EntityCount resp;
    const StatLeaderboard* lb = mOwner->nextLeaderboard();
    req.setBoardName(lb->getBoardName());
    req.setPeriodOffset(mOwner->getRandomPeriodOffset());

    mOwner->fillLeaderboardRequestScopes(req.getKeyScopeNameValueMap(), lb);

    result = mProxy->getLeaderboardEntityCount(req, resp);
    
    if (result == ERR_OK)
    {
        rpcList[index].resRowCtr += resp.getCount();
        rpcList[index].perRowCtr += resp.getCount();
    }

    return result;
}
// =============================
const char8_t* StatsInstance::getName() const
{
    return rpcList[mRpcIndex].rpcName;
}

/*** Public Methods ******************************************************************************/

// static
StressModule* StatsModule::create()
{
    return BLAZE_NEW StatsModule();
}


StatsModule::~StatsModule()
{
    if (mLogFile != nullptr) fclose(mLogFile);

    ScopeVectorMap::iterator itr = mScopeVectorMap.begin();
    ScopeVectorMap::iterator end = mScopeVectorMap.end();
    for (; itr != end; ++itr)
    {
        ScopeVector* sv = itr->second;
        for (ScopeVector::iterator svItr = sv->begin(), svEnd = sv->end();
             svItr != svEnd;
             ++svItr)
        {
            delete *svItr;
        }
        sv->clear();
        delete sv;
    }
    mScopeVectorMap.clear();
}

/*************************************************************************************************/
bool StatsModule::initScopeVectorMaps()
{
    // To iterate thru keyscopes for all categories, set up per category object holding current
    // combination of keyscopes as well as counters and pointers needed to iterate thru keyscopes
    // (similar to LeaderboardCache's mechanism to walk thru the different keyscope combinations)
    const KeyScopesMap& keyScopeMap = mStatsConfig->getKeyScopeMap();
    KeyScopesMap::const_iterator ksEnd = keyScopeMap.end();
    KeyScopesMap::const_iterator ksItr;

    CategoryMap::const_iterator catItr = mStatsConfig->getCategoryMap()->begin();
    CategoryMap::const_iterator catEnd = mStatsConfig->getCategoryMap()->end();
    for (; catItr != catEnd; ++catItr)
    {
        const Stats::StatCategory* cat = catItr->second;

        const ScopeNameSet* scopeNameSet = cat->getScopeNameSet();
        if (scopeNameSet == nullptr || scopeNameSet->empty())
            continue;

        ScopeVector* scopeVector = BLAZE_NEW ScopeVector(Blaze::BlazeStlAllocator("StatsModule::scopeVector", StatsSlave::COMPONENT_MEMORY_GROUP));
        ScopeNameSet::const_iterator scNameItr = scopeNameSet->begin();
        ScopeNameSet::const_iterator scNameEnd = scopeNameSet->end();
        for (; scNameItr != scNameEnd; ++scNameItr)
        {
            const char8_t* scopeName = *scNameItr;

            ksItr = keyScopeMap.find(scopeName);
            if (ksItr == ksEnd)
                return false; // unlikely b/c validation that this keyscope exists already should have been done

            const KeyScopeItem* scopeItem = ksItr->second;

            ScopeCell* scopeCell = BLAZE_NEW ScopeCell;
            scopeCell->name = scopeName;

            if (scopeItem->getKeyScopeValues().empty())
            {
                // no start-end ranges
                // don't need numPossibleValues for this keyscope

                /// @todo probably don't want to cycle thru all non-neg 32-bit int values...
                scopeCell->valuesMap = nullptr;

                if (scopeItem->getEnableAggregation())
                {
                    scopeCell->aggregateValue = scopeItem->getAggregateKeyValue();
                    scopeCell->currentValue = scopeCell->aggregateValue != 0 ? 0 : 1;
                }
                else
                {
                    scopeCell->aggregateValue = KEY_SCOPE_VALUE_AGGREGATE;
                    scopeCell->currentValue = 0;
                }
            }
            else
            {
                scopeCell->valuesMap = &scopeItem->getKeyScopeValues();

                // include any aggregate key value (it shouldn't be in the map already)
                if (scopeItem->getEnableAggregation())
                {
                    scopeCell->aggregateValue = scopeItem->getAggregateKeyValue();
                }
                else
                {
                    scopeCell->aggregateValue = KEY_SCOPE_VALUE_AGGREGATE;
                }

                // count up the number of possible values
                scopeCell->numPossibleValues = 0;
                ScopeStartEndValuesMap::const_iterator valuesItr = scopeCell->valuesMap->begin();
                ScopeStartEndValuesMap::const_iterator valuesEnd = scopeCell->valuesMap->end();
                for (; valuesItr != valuesEnd; ++valuesItr)
                {
                    scopeCell->numPossibleValues += valuesItr->second - valuesItr->first + 1;
                }

                scopeCell->mapItr = scopeCell->valuesMap->begin();

                // with earlier empty-check, itr can't be at end here
                scopeCell->currentValue = scopeCell->mapItr->first;
            }

            scopeVector->push_back(scopeCell);
        }
        mScopeVectorMap.insert(eastl::make_pair(cat, scopeVector));
    }

    return true;
}

/*************************************************************************************************/
void StatsModule::nextScopeNameValueMap(ScopeNameValueMap& scopeNameValueMap, const StatCategory* category, bool update)
{
    ScopeVectorMap::iterator svItr = mScopeVectorMap.find(category);
    if (svItr == mScopeVectorMap.end())
        return;

    // use the current combination
    ScopeVector* scopeVector = svItr->second;
    ScopeVector::iterator scopeItr = scopeVector->begin();
    ScopeVector::iterator scopeEnd = scopeVector->end();
    for (; scopeItr != scopeEnd; ++scopeItr)
    {
        ScopeCell* scopeCell = *scopeItr;
        scopeNameValueMap.insert(eastl::make_pair(scopeCell->name, scopeCell->currentValue));
    }

    // set next combination
    scopeItr = scopeVector->begin();
    for (; scopeItr != scopeEnd; ++scopeItr)
    {
        ScopeCell* scopeCell = *scopeItr;

        if (scopeCell->valuesMap == nullptr)
        {
            // no start-end ranges

            if (scopeCell->currentValue != scopeCell->aggregateValue)
            {
                // try next value
                ++(scopeCell->currentValue);

                if (scopeCell->currentValue == scopeCell->aggregateValue)
                {
                    // skip it
                    ++(scopeCell->currentValue);
                }

                if (scopeCell->currentValue >= 0)
                {
                    // can use "next" value
                    break;
                }
            }

            // reached the end of our possible values

            if (!update && scopeCell->aggregateValue != KEY_SCOPE_VALUE_AGGREGATE && scopeCell->currentValue != scopeCell->aggregateValue)
            {
                // the aggregate value is our last option (if supported)
                scopeCell->currentValue = scopeCell->aggregateValue;
            }
            else
            {
                // wrap around to first value in this keyscope
                scopeCell->currentValue = scopeCell->aggregateValue != 0 ? 0 : 1;
            }
        }
        else
        {
            if (scopeCell->currentValue != scopeCell->aggregateValue)
            {
                // try next value
                ++(scopeCell->currentValue);

                if (scopeCell->currentValue <= scopeCell->mapItr->second)
                {
                    // can use next value in current start-end range
                    break;
                }

                // try next start-end range
                ++(scopeCell->mapItr);
                if (scopeCell->mapItr != scopeCell->valuesMap->end())
                {
                    // can use start of next start-end range
                    scopeCell->currentValue = scopeCell->mapItr->first;
                    break;
                }
            }

            // reached the end of our possible values

            if (!update && scopeCell->aggregateValue != KEY_SCOPE_VALUE_AGGREGATE && scopeCell->currentValue != scopeCell->aggregateValue)
            {
                // the aggregate value is our last option (if supported)
                scopeCell->currentValue = scopeCell->aggregateValue;
            }
            else
            {
                // wrap around to first value in this keyscope
                scopeCell->mapItr = scopeCell->valuesMap->begin();
                scopeCell->currentValue = scopeCell->mapItr->first;
            }
        }
        // and start running through the next keyscope
    }
}

/*************************************************************************************************/
void StatsModule::getScopeNameValueMapByEntity(ScopeNameValueMap& scopeNameValueMap, const StatCategory* category, EntityId entity)
{
    // turn entity into keyscope sequence for repeatability
    ScopeVectorMap::iterator svItr = mScopeVectorMap.find(category);
    if (svItr == mScopeVectorMap.end())
        return;

    ScopeVector* scopeVector = svItr->second;
    ScopeVector::iterator scopeItr = scopeVector->begin();
    ScopeVector::iterator scopeEnd = scopeVector->end();
    for (; scopeItr != scopeEnd; ++scopeItr)
    {
        ScopeCell* scopeCell = *scopeItr;
        ScopeValue scopeValue;

        if (scopeCell->valuesMap == nullptr)
        {
            // no start-end ranges
            // assuming that entity id won't exceed possible scope values
            scopeValue = static_cast<ScopeValue>(entity);
            if (scopeCell->aggregateValue != KEY_SCOPE_VALUE_AGGREGATE && entity >= scopeCell->aggregateValue)
            {
                if (++scopeValue < 0)
                {
                    scopeValue = 0;
                }
            }
        }
        else
        {
            // convert entity id to an index of possible keyscope values
            scopeValue = static_cast<ScopeValue>(entity % scopeCell->numPossibleValues);

            // walk the ranges to the find the keyscope value
            ScopeStartEndValuesMap::const_iterator valuesItr = scopeCell->valuesMap->begin();
            ScopeStartEndValuesMap::const_iterator valuesEnd = scopeCell->valuesMap->end();
            for (; valuesItr != valuesEnd; ++valuesItr)
            {
                if (valuesItr->first + scopeValue <= valuesItr->second)
                {
                    // convert index to actual keyscope value
                    scopeValue += valuesItr->first;
                    break;
                }
                // adjust the "index"
                scopeValue -= valuesItr->second - valuesItr->first + 1;
                // and try the next start-end range
            }
        }

        scopeNameValueMap.insert(eastl::make_pair(scopeCell->name, scopeValue));
    }
}

/****************************************************************************************************/
bool  StatsModule::parseGameCategories(const ConfigMap* config, CategoryList* catList, const char8_t* name)
{
    const ConfigSequence* gameSequence = config->getSequence(name);
    if (gameSequence == nullptr) return true;
    while (gameSequence->hasNext())
    {
        const char8_t* catName = gameSequence->nextString("");
        CategoryMap::const_iterator catIter = mStatsConfig->getCategoryMap()->find(catName);
        if (catIter == mStatsConfig->getCategoryMap()->end())
        {
            BLAZE_ERR_LOG(BlazeRpcLog::stats,"[StatsModule].parseGameCategories: invalid category: <" << catName << ">");
            return false;
        }
        
        catList->push_back(catIter->second);
    }

    return true;
}

/****************************************************************************************************/
int32_t StatsModule::getInt32(const ConfigMap* config, const char8_t* name, int32_t def, int32_t min, int32_t max)
{
    int32_t val = config->getInt32(name, def);
    if (val < min) val = min;
    if (val > max) val = max ;
    return val;
}
/*************************************************************************************************/
/*!
    \brief parseConfig
    Parse configs
    \param[in] - config map
    \return - true on success
*/
/*************************************************************************************************/
bool StatsModule::parseConfig(const ConfigMap* config, const ConfigBootUtil* bootUtil)
{
    const ConfigMap* rpcListMap = config->getMap("rpcList");
    if (rpcListMap == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::stats,"[StatsModule].parseConfig: 'rpcList' section is not found");
        return false;
    }
        
    double total = 0; 
    mRpcCount = EAArrayCount(rpcList);
    for (int32_t j = 0; j<mRpcCount; ++j)
    {
        rpcList[j].users = 0;
        rpcList[j].usersPer = 0;
        rpcList[j].ctr = 0;
    
        const ConfigMap* desc = rpcListMap->getMap(rpcList[j].rpcName);
        if (desc == nullptr)
        {
            rpcList[j].hitRate = 0;
            continue;
        }
        
        
        rpcList[j].hitRate = desc->getDouble("hitRate", 0);
        total += rpcList[j].hitRate;
        int32_t value = desc->getInt32("rowCtr", 12);
        if (value <= 0 || value >= 200)
        {
            BLAZE_ERR_LOG(BlazeRpcLog::stats,"[StatsModule].parseConfig: 'rowCtr' for '" << rpcList[j].rpcName << "' is out of range");
            return false;
        }
        
        rpcList[j].rowCtr = value; 
    }        

    double kx = Random::MAX/total;
    int32_t range = 0;
        
    TimeValue time = TimeValue::getTimeOfDay();
    for (int32_t j = 0; j<mRpcCount; ++j)
    {
        if (rpcList[j].hitRate == 0) continue;
        
        rpcList[j].wMin = range; 
        range += static_cast<int32_t>(kx*rpcList[j].hitRate);
        rpcList[j].wMax = range; 
        rpcList[j].periodStartTime = time;

        BLAZE_TRACE_LOG(BlazeRpcLog::stats,"[StatsModule].parseConfig: " << rpcList[j].rpcName << ": min: " << rpcList[j].wMin 
                       << "  max " << rpcList[j].wMax << "  total: " << total);
    }

    // Seed the random number generator (required by Blaze::Random)
    mRandSeed = config->getInt32("randSeed", -1);

    (mRandSeed < 0) ? srand((unsigned int)TimeValue::getTimeOfDay().getSec()) : srand(mRandSeed);

    mSampleIntervalCtr = 0;

    mMinEntityId = config->getInt64( "minEntityId", 1000000000000);
    mMaxEntityId = config->getInt64( "maxEntityId",  1000000000000);
    mSeqEntityId = config->getInt32("seqEntityId", 0);
    mEntityId = mMinEntityId;
    mMaxClubCount = config->getInt32("maxClubCount", 460000);
    mMaxUserCount = config->getInt32("maxUserCount", 8000000);
    
    mMaxLbStartRank = getInt32(config, "maxLbStartRank", 1, 1, Random::MAX - 1);
    mMaxPeriodOffset = getInt32(config, "maxPeriodOffset", 0, 0, 16);
    mMaxCategoryCount = getInt32(config, "maxCategoryCount", 3, 1, 24);
    mSampleSize =  getInt32(config, "sampleSize", 1, 1, 100);
    int32_t rpcDelay = config->getInt32("delay", 0); // also read by framework
    
    // non configurable (as per requirements for Global Category)
    mGlobalCategoryName = config->getString("globalCategoryName", "");

    // Read and parse Stats config files
    mStatsConfig = BLAZE_NEW Stats::StatsConfigData();

    if (bootUtil != nullptr)
    {
        mStatsConfigFile = bootUtil->createFrom("stats", ConfigBootUtil::CONFIG_IS_COMPONENT);
    }
    else
    {
        mStatsConfigFile = ConfigFile::createFromFile("component/stats/stats.cfg", true);
    }

    if (mStatsConfigFile == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::stats,"[StatsModule].parseConfig: could not create ConfigFile");
        return false;
    }
    
    mStatsConfigFile = (const ConfigMap*)mStatsConfigFile->getMap("stats");
    if (mStatsConfigFile == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::stats,"[StatsModule].parseConfig: 'stats' section is not found");
        return false;
    }

    ConfigDecoder configDecoder(*mStatsConfigFile);
    configDecoder.decode(&mStatsConfigTdf);
    
    if (!mStatsConfig->parseStatsConfig(mStatsConfigTdf))
    {
        BLAZE_ERR_LOG(BlazeRpcLog::stats,"[StatsSlaveImpl].parseConfig: Failed to parse stats config, will not start");
        return false;
    }

    if (!parseGameCategories(config, &mNormalGames, "normalGames")) return false;
    if (!parseGameCategories(config, &mSoloGames, "soloGames")) return false;
    if (!parseGameCategories(config, &mOtpGames, "otpGames")) return false;
    if (!parseGameCategories(config, &mClubGames, "clubGames")) return false;
    if (!parseGameCategories(config, &mSponsorEventGames, "sponsorEventGames")) return false;

    mCatIterEnd = mStatsConfig->getCategoryMap()->end();
    mCatIter = mCatIterEnd;
    mLbIterEnd = mStatsConfig->getStatLeaderboardsMap()->end();
    mLbIter = mLbIterEnd;
    mStatGroupIterEnd = mStatsConfig->getStatGroupMap()->end();
    mStatGroupIter = mStatGroupIterEnd;
    mLBGroupIterEnd = mStatsConfig->getLeaderboardGroupsMap()->end();
    mLBGroupIter = mLBGroupIterEnd;
    mLBFolderIterEnd = mStatsConfig->getLeaderboardFolderIndexMap()->end();
    mLBFolderIter = mLBFolderIterEnd;

    initScopeVectorMaps();

// ------------------------------------------------------
    uint32_t tyear, tmonth, tday, thour, tmin, tsec;
    TimeValue tnow = TimeValue::getTimeOfDay(); 
    tnow.getGmTimeComponents(tnow,&tyear,&tmonth,&tday,&thour,&tmin,&tsec);
    char8_t fName[256];
    blaze_snzprintf(fName, sizeof(fName), "%s/%s_stress_%02d%02d%02d%02d%02d%02d.log",(Logger::getLogDir()[0] != '\0')?Logger::getLogDir():".",getModuleName(), tyear, tmonth, tday, thour, tmin, tsec);
    
    mLogFile = fopen(fName, "wt");
    if (mLogFile == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::stats,"[StatsModule].parseConfig: could not create log file("<<fName<<") in folder("<<Logger::getLogDir()<<")");
        return false;
    }

    BLAZE_INFO_LOG(BlazeRpcLog::stats,"[StatsModule].parseConfig: maxEntityId: " << mMaxEntityId);    
    BLAZE_INFO_LOG(BlazeRpcLog::stats,"[StatsModule].parseConfig: maxLbStartRank: " << mMaxLbStartRank);    
    BLAZE_INFO_LOG(BlazeRpcLog::stats,"[StatsModule].parseConfig: maxPeriodOffset: " << mMaxPeriodOffset);    
    BLAZE_INFO_LOG(BlazeRpcLog::stats,"[StatsModule].parseConfig: maxCategoryCount: " << mMaxCategoryCount);    
    BLAZE_INFO_LOG(BlazeRpcLog::stats,"[StatsModule].parseConfig: sampleSize: " << mSampleSize);    
    BLAZE_INFO_LOG(BlazeRpcLog::stats,"[StatsModule].parseConfig: rpcDelay: " << rpcDelay);    
    
    return true;
}
// -------------------------------------------------------------------------
bool StatsModule::saveStats()
{
    // accumulation period in number of DUMP_STATS_INTERVAL periods
    ++mSampleIntervalCtr;
    if (mSampleIntervalCtr < mSampleSize)
        return false;
    mSampleIntervalCtr = 0;
    
    int32_t txnTotal = 0;
    double txnPerSecTotal = 0;
    double usersPerSecTotal = 0;
    
    if (mLogFile != nullptr)
    {
        fprintf(mLogFile, "+----------------------------+--------+---------+-----------+-----------+--------+--------+--------+---------+--------+----------+\n");
        fprintf(mLogFile, "|            rpc name        |    txn |     err |     txn/s |     err/s | dur ttl| perDur | result | perRes  | uGames | uGames/s |\n");
        fprintf(mLogFile, "+----------------------------+--------+---------+-----------+-----------+--------+--------+--------+---------+--------+----------+\n");


        TimeValue time = TimeValue::getTimeOfDay();
        double_t periodDur = 0;
        double_t userPerSec = 0;

        for (int32_t j = 0; j < mRpcCount; ++j)
        {
            if (rpcList[j].hitRate == 0) continue;
            
            int32_t dur = 0;
            int32_t perDur = 0;
            int32_t rowCtr = 0;
            int32_t perRowCtr = 0;
            double_t txnPerSec = 0;
            double_t errPerSec = 0;
            
            TimeValue perTime = time - rpcList[j].periodStartTime;
            periodDur = perTime.getMicroSeconds()/(double)1000000.0;
            rpcList[j].periodStartTime = time;
            
            if (rpcList[j].ctr != 0)
            {
                dur = (int32_t)(rpcList[j].durTotal/rpcList[j].ctr);
                rowCtr = rpcList[j].resRowCtr/rpcList[j].ctr;
            }
            
            if (rpcList[j].perCtr != 0)
            { 
                perDur = (int32_t)rpcList[j].durPeriod/rpcList[j].perCtr;
                perRowCtr = rpcList[j].perRowCtr/rpcList[j].perCtr;
            }

            if (periodDur != 0)
            {
                txnPerSec = rpcList[j].perCtr/periodDur;
                errPerSec = rpcList[j].perErrCtr/periodDur;

                userPerSec = rpcList[j].usersPer/periodDur;
                usersPerSecTotal += userPerSec;
            }

            fprintf(mLogFile, "%28s: %8d  %8d  %10.2f  %10.2f %8d %8d %8d %8d %8d %8.2f\n", rpcList[j].rpcName,
            rpcList[j].ctr, rpcList[j].errCtr, txnPerSec, errPerSec, dur, perDur, rowCtr, perRowCtr, rpcList[j].users, userPerSec);
                
            txnTotal += rpcList[j].ctr;
            txnPerSecTotal += txnPerSec;
    
            rpcList[j].durPeriod = 0;
            rpcList[j].perCtr = 0;
            rpcList[j].perErrCtr = 0;
            rpcList[j].perRowCtr = 0;
            rpcList[j].usersPer = 0;
        }
    
        char8_t timeStr[64];
        time.toString(timeStr, sizeof(timeStr));
        fprintf(mLogFile, "%s: print period: %6.2f sec, total transactions: %8d,  tsx/s : %10.2f, total users finished/s : %5.2f\n", 
            timeStr, periodDur, txnTotal, txnPerSecTotal, usersPerSecTotal);
        fflush(mLogFile);
    }

    return false;
}

// -------------------------------------------------------------------------
const Stats::StatCategory* StatsModule::nextCategory()
{
    if (mCatIter == mCatIterEnd)
    {
        mCatIter = mStatsConfig->getCategoryMap()->begin();
    }

    const Stats::StatCategory* category = mCatIter->second;
    ++mCatIter;
    return category;
}

// -------------------------------------------------------------------------
const Stats::StatLeaderboard* StatsModule::nextLeaderboard()
{
    if (mLbIter == mLbIterEnd)
    {
        mLbIter = mStatsConfig->getStatLeaderboardsMap()->begin();
    }

    Stats::StatLeaderboard* leaderboard = mLbIter->second;
    ++mLbIter;
    return leaderboard;
}

// -------------------------------------------------------------------------
const Stats::StatGroup* StatsModule::nextStatGroup()
{
    if (mStatGroupIter == mStatGroupIterEnd)
    {
        mStatGroupIter = mStatsConfig->getStatGroupMap()->begin();
    }

    const Stats::StatGroup* statGroup = mStatGroupIter->second;
    ++mStatGroupIter;
    return statGroup;
}

// -------------------------------------------------------------------------
int32_t StatsModule::getLBStartRank()
{
    return 1 + Blaze::Random::getRandomNumber(mMaxLbStartRank);
}

// -------------------------------------------------------------------------
const char8_t* StatsModule::nextLeaderboardFolderName()
{
    if (mLBFolderIter == mLBFolderIterEnd)
    {
        mLBFolderIter = mStatsConfig->getLeaderboardFolderIndexMap()->begin();
    }
    
    const char8_t* name = mLBFolderIter->first;
    ++mLBFolderIter;
    return name;
}

// -------------------------------------------------------------------------
EntityId   StatsModule::getRandomEntityId()
{
    
    if (mSeqEntityId)
    {
        EntityId id = mEntityId;
        if (mEntityId == mMaxEntityId)
            mEntityId = mMinEntityId;
        else
            ++mEntityId;    
        return id;
    }
    // NOTE: For now we only make use of the bottom 32 bits of mMaxEntityId (no need to gen random 64bit number)
    return mMinEntityId + Random::getRandomNumber((int32_t)mMaxEntityId);
}

// -------------------------------------------------------------------------
uint32_t   StatsModule::getRandomPeriodOffset()
{
    if (mMaxPeriodOffset <= 0) return 0;
    return Blaze::Random::getRandomNumber(mMaxPeriodOffset);
}

// -------------------------------------------------------------------------
int32_t StatsModule::getRandomPeriodType(const StatCategory* cat)
{
    int32_t periodType;
    for (int32_t j = 0; j < 1000; ++j)
    {
        periodType = Blaze::Random::getRandomNumber(STAT_NUM_PERIODS);
        if (cat->isValidPeriod(periodType))
            return periodType;
    }
    return 0;
}

// ------------------------------------------------------------------------
void StatsModule::fillLeaderboardRequestScopes(ScopeNameValueMap& scopeNameValueMap, const StatLeaderboard* lb)
{
    if (!lb->getHasScopes())
        return;

    const Stats::ScopeNameValueListMap* lbScopeMap = lb->getScopeNameValueListMap();
    const Stats::StatCategory* cat = lb->getLeaderboardGroup()->getStatCategory();

    ScopeNameValueMap catScopeNameValueMap;
    nextScopeNameValueMap(catScopeNameValueMap, cat);

    Stats::ScopeNameValueListMap::const_iterator lbScopeItr = lbScopeMap->begin();
    Stats::ScopeNameValueListMap::const_iterator lbScopeEnd = lbScopeMap->end();
    for (; lbScopeItr != lbScopeEnd; ++lbScopeItr)
    {
        if (mStatsConfig->getKeyScopeSingleValue(lbScopeItr->second->getKeyScopeValues()) == Stats::KEY_SCOPE_VALUE_USER)
        {
            ScopeNameValueMap::const_iterator nvItr = catScopeNameValueMap.find(lbScopeItr->first);
            if (nvItr == catScopeNameValueMap.end())
            {
                BLAZE_ERR_LOG(BlazeRpcLog::stats,"[StatsModule].fillLeaderboardRequestScopes():"
                    " category (" << cat->getName() << ") does not have keyscope leaderboard (" << lb->getBoardName() << ") wants!");
                return;
            }
            scopeNameValueMap.insert(eastl::make_pair(nvItr->first, nvItr->second));
        }
    }
}

// ------------------------------------------------------------------------
void StatsModule::fillStatGroupRequestScopes(ScopeNameValueMap& scopeNameValueMap, const StatGroup* sg)
{
    const ScopeNameValueMap* sgScopeMap = sg->getScopeNameValueMap();
    if (sgScopeMap == nullptr || sgScopeMap->empty())
        return;

    const Stats::StatCategory* cat = sg->getStatCategory();

    ScopeNameValueMap catScopeNameValueMap;
    nextScopeNameValueMap(catScopeNameValueMap, cat);

    ScopeNameValueMap::const_iterator sgScopeItr = sgScopeMap->begin();
    ScopeNameValueMap::const_iterator sgScopeEnd = sgScopeMap->end();
    for (; sgScopeItr != sgScopeEnd; ++sgScopeItr)
    {
        if (sgScopeItr->second == KEY_SCOPE_VALUE_USER)
        {
            ScopeNameValueMap::const_iterator nvItr = catScopeNameValueMap.find(sgScopeItr->first);
            if (nvItr == catScopeNameValueMap.end())
            {
                BLAZE_ERR_LOG(BlazeRpcLog::stats,"[StatsModule].fillStatGroupRequestScopes():"
                    " category (" << cat->getName() << ") does not have keyscope group (" << sg->getName() << ") wants!");
                return;
            }
            scopeNameValueMap.insert(eastl::make_pair(nvItr->first, nvItr->second));
        }
    }
}

// -------------------------------------------------------------------------
bool StatsModule::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    if (!StressModule::initialize(config, bootUtil)) return false;

    mActionName = config.getString("action", nullptr);
    if (mActionName == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::stats,"[StatsModule].initialize: Action string is missing");
        return false;
    }

    if (!parseConfig(&config, bootUtil)) return false;

    return true;
}

StressInstance* StatsModule::createInstance(StressConnection* connection, Login* login)
{
    return BLAZE_NEW StatsInstance(this, connection, login);
}

/*** Private Methods *****************************************************************************/

StatsModule::StatsModule()
  : mScopeVectorMap(Blaze::BlazeStlAllocator("StatsModuel::mScopeVectorMap", StatsSlave::COMPONENT_MEMORY_GROUP)),
    mRandSeed(-1), 
    mEntityId(1),
    mRpcCount(0),
    mLogFile(nullptr)
{
}

} // Stress
} // Blaze

