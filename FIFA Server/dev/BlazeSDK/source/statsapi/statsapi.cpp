/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
**************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/statsapi/statsapi.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/usermanager/usermanager.h"
#include "BlazeSDK/usermanager/user.h"
#include "BlazeSDK/component/statscomponent.h"

#include "BlazeSDK/job.h"

#include "EASTL/algorithm.h"
#include "EASTL/functional.h"


namespace Blaze
{
    namespace Stats
    {

//    TODO: move utility functional predicate to a class def instead of belonging to Blaze namespace
struct MatchStatsGroupAndName: eastl::binary_function<Stats::StatGroupSummary*, const char *, bool>
{
    bool operator()(const Stats::StatGroupSummary* summary, const char* name) const {
        return strcmp(summary->getName(), name) == 0;
    }
};


///////////////////////////////////////////////////////////////////////////////
//
//    class StatsAPI
//
///////////////////////////////////////////////////////////////////////////////

//    BlazeHub hook method
void StatsAPI::createAPI(BlazeHub &hub, EA::Allocator::ICoreAllocator* allocator)
{
    if (hub.getStatsAPI() != nullptr) 
    {
        BLAZE_SDK_DEBUGF("[StatsAPI] Warning: Ignoring attempt to recreate API\n");
        return;
    }
    if (Blaze::Allocator::getAllocator(MEM_GROUP_STATS) == nullptr)
        Blaze::Allocator::setAllocator(MEM_GROUP_STATS, allocator!=nullptr? allocator : Blaze::Allocator::getAllocator());
    Stats::StatsComponent::createComponent(&hub);            // dependency
    hub.createAPI(STATS_API, BLAZE_NEW(MEM_GROUP_STATS, "APIInstance") StatsAPI(hub, MEM_GROUP_STATS));
}

StatsAPI::StatsAPI(BlazeHub& hub, MemoryGroupId memGroupId) : SingletonAPI(hub),
    mStatGroupList(nullptr),
    mKeyScopes(nullptr),
    mStatsGroupTable(memGroupId, MEM_NAME(memGroupId, "StatsAPI::mStatsGroupTable")),
    mViewId(0),
    mMemGroup(memGroupId)
{
    // Add the callback to all users (since this is a SingletonAPI)
    for (uint32_t userIndex = 0; userIndex < getBlazeHub()->getNumUsers(); ++userIndex)
    {
        Stats::StatsComponent *component = getBlazeHub()->getComponentManager(userIndex)->getStatsComponent(); 
        component->setGetStatsAsyncNotificationHandler(StatsComponent::GetStatsAsyncNotificationCb(this, &StatsAPI::onGetStatsAsyncNotification));
    }
}

StatsAPI::~StatsAPI()
{
    // Add the callback to all users (since this is a SingletonAPI)
    for (uint32_t userIndex = 0; userIndex < getBlazeHub()->getNumUsers(); ++userIndex)
    {
        Stats::StatsComponent *component = getBlazeHub()->getComponentManager(userIndex)->getStatsComponent();
        if (component)
            component->clearGetStatsAsyncNotificationHandler();
    }

    releaseStatsGroupList();
    releaseKeyScope();
}


void StatsAPI::logStartupParameters() const
{
}


void StatsAPI::onDeAuthenticated(uint32_t userIndex)
{
    // only clean up if last local user
    if (userIndex == getBlazeHub()->getPrimaryLocalUserIndex())
    {
        releaseStatsGroupList();
    }
}

JobId StatsAPI::requestKeyScopes(GetKeyScopeCb& getMapCb)
{
    JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
    JobId jobId = jobScheduler->reserveJobId();

    if (mKeyScopes != nullptr)
    {
        //    group key scope resident.
        jobId = jobScheduler->scheduleFunctor("requestKeyScopesCb", getMapCb, ERR_OK, jobId, const_cast<const KeyScopes*>(mKeyScopes.get()),
                this, 0, jobId); // assocObj, delayMs, reservedJobId
        Job::addTitleCbAssociatedObject(jobScheduler, jobId, getMapCb);
        return jobId;
    }

    mKeyScopes = BLAZE_NEW(mMemGroup, "KeyScopes") KeyScopes(mMemGroup);
    Stats::StatsComponent *component = getBlazeHub()->getComponentManager()->getStatsComponent();
    JobId rpcJobId = component->getKeyScopesMap(MakeFunctor(this, &StatsAPI::getStatKeyScopeCb), getMapCb, jobId, USE_DEFAULT_TIMEOUT, mKeyScopes);
    Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), rpcJobId, getMapCb);
    return rpcJobId;
}

void StatsAPI::deactivate()
{
    releaseStatsGroupList();
    releaseKeyScope();
    API::deactivate();
}

const Stats::KeyScopesMap* StatsAPI::getKeyScopesMap() const
{
    if (mKeyScopes != nullptr)
        return &mKeyScopes->getKeyScopesMap();

    return nullptr;
}

void StatsAPI::addListener(StatsAPIListener* listener)
{
    mDispatcher.addDispatchee(listener);
}

void StatsAPI::removeListener(StatsAPIListener* listener)
{
    mDispatcher.removeDispatchee(listener);
}

JobId StatsAPI::requestStatsGroupList(GetStatsGroupListCb& getListCb)
{
    JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
    JobId jobId = jobScheduler->reserveJobId();
    if (mStatGroupList != nullptr)
    {
        //    group list resident.
        jobId = jobScheduler->scheduleFunctor("requestStatsGroupListCb", getListCb, ERR_OK, jobId, const_cast<const StatGroupList *>(mStatGroupList.get()), this, 0, jobId);
        Job::addTitleCbAssociatedObject(jobScheduler, jobId, getListCb);
        return jobId;
    }

    mStatGroupList = BLAZE_NEW(mMemGroup, "StatGroupList") StatGroupList(mMemGroup);
    Stats::StatsComponent *component = getBlazeHub()->getComponentManager()->getStatsComponent();
    JobId rpcJobId = component->getStatGroupList(MakeFunctor(this, &StatsAPI::getStatGroupListCb), getListCb, jobId, USE_DEFAULT_TIMEOUT, mStatGroupList);
    Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), rpcJobId, getListCb);
    return rpcJobId;
}


void StatsAPI::getStatGroupListCb(const Stats::StatGroupList *list, BlazeError err, JobId id, GetStatsGroupListCb cb)
{
    // assumption: mStatGroupList was passed to the job by the requester, and filled in by the decoder.
    
    if (err != ERR_OK)
    {
        mStatGroupList = nullptr;
    }
    cb(err, id, mStatGroupList);
} /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

void StatsAPI::getStatKeyScopeCb(const Stats::KeyScopes* keyScopes, BlazeError err, JobId id, GetKeyScopeCb cb)
{
    // assumption: mKeyScopes was passed to the job by the requester, and filled in by the decoder.
    
    if (err != ERR_OK)
    {
        mKeyScopes = nullptr;
    }
    cb(err, id, mKeyScopes);
} /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/


void StatsAPI::releaseStatsGroupList()
{    
    //  kill any job timers started up
    getBlazeHub()->getScheduler()->cancelByAssociatedObject(reinterpret_cast<void*>(this));
   
    //    free all StatsGroup managed by the API.
    StatsGroupTable::iterator itEnd = mStatsGroupTable.end();
    for (StatsGroupTable::iterator it = mStatsGroupTable.begin(); it != itEnd; ++it)
    {
        mDispatcher.dispatch("onStatGroupDelete", &StatsAPIListener::onStatGroupDelete, it->second);
        BLAZE_DELETE_PRIVATE(mMemGroup, StatsGroup, it->second);
    }
    mStatsGroupTable.clear();

    //    invalidate the group list so subsequent requests hit the server.
    if (mStatGroupList != nullptr)
    {
        mDispatcher.dispatch("onStatGroupListDelete", &StatsAPIListener::onStatGroupListDelete, mStatGroupList.get());
        mStatGroupList = nullptr;
    }
}

void StatsAPI::releaseKeyScope()
{
    mKeyScopes = nullptr;
}

JobId StatsAPI::requestStatsGroup(GetStatsGroupCb& callback, const char *name)
{
    StatsGroupTable::iterator itGroup = mStatsGroupTable.find(name);
    if (itGroup != mStatsGroupTable.end())
    {

        JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
        JobId jobId = jobScheduler->reserveJobId();
        jobId = jobScheduler->scheduleFunctor("requestStatsGroupCb", callback, ERR_OK, jobId, static_cast<StatsGroup*>(itGroup->second),
                this, 0, jobId); // assocObj, delayMs, reservedJobId
        Job::addTitleCbAssociatedObject(jobScheduler, jobId, callback);
        return jobId;
    }

    Stats::StatsComponent *component = getBlazeHub()->getComponentManager()->getStatsComponent();

    Stats::GetStatGroupRequest req;
    req.setName(name);    
    JobId jobId = component->getStatGroup(req, MakeFunctor(this, &StatsAPI::getStatGroupCb), callback);
    Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, callback);
    return jobId;
}


void StatsAPI::getStatGroupCb(const Stats::StatGroupResponse *resp, BlazeError err, JobId id, GetStatsGroupCb cb)
{
    StatsGroup *group = nullptr;
    
    if (err == ERR_OK)
    {
        StatsGroupTable::const_iterator foundGroup = mStatsGroupTable.find(resp->getName());
        if (foundGroup != mStatsGroupTable.end())
        {
            // if there's a group already cached, this response should contain the SAME data
            group = foundGroup->second;
        }
        else
        {
            //    if group list in memory then use the StatGroupSummary from the list.
            group = BLAZE_NEW(mMemGroup, "Group") StatsGroup(this, resp, mMemGroup);
            
            StatsGroupTable::insert_return_type insertedGroup = mStatsGroupTable.insert(group->getName());
            insertedGroup.first->second = group;
        }
    }

    cb(err, id, group);
}  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/
// Get stats notifications
void StatsAPI::onGetStatsAsyncNotification(const Blaze::Stats::KeyScopedStatValues* statValues, uint32_t userIndex)
{
    StatsGroupTable::iterator it = mStatsGroupTable.find(statValues->getGroupName());
    // Every view needs to have stat group
    if (it == mStatsGroupTable.end()) return;

    StatsView* view = it->second->getViewById(statValues->getViewId());
    // Should never happen
    if (view == nullptr) return;

    // no data was found
    if (statValues->getKeyString() != nullptr && *statValues->getKeyString() != '\0')
        view->copyStatValues(statValues);

    if (statValues->getLast())
    {
        StatsView::StatsViewJob* statsViewJob = (StatsView::StatsViewJob*)getBlazeHub()->getScheduler()->getJob(view->getJobId());
        if (statsViewJob != nullptr)
        {
            if (view->isDoneFlag())
                return;
            view->setDoneFlag(true);
            view->clearIsCreating();
            statsViewJob->execute();
            getBlazeHub()->getScheduler()->removeJob(statsViewJob);
        }
    }
        
    return;
}

// --- User sesion extended data ---
bool StatsAPI::hasStats(const Blaze::UserManager::User *user)
{
    if (user == nullptr) return false;
    return user->hasExtendedData();
}

bool StatsAPI::getUserStat(const Blaze::UserManager::User *user, uint16_t key, UserExtendedDataValue &value)
{
    if (user == nullptr) return false;
    return user->getDataValue(STATSCOMPONENT_COMPONENT_ID, key, value);
}

bool StatsAPI::hasLocalStats(uint32_t userIndex) const
{
    const Blaze::UserManager::LocalUser* user = getBlazeHub()->getUserManager()->getLocalUser(userIndex);
    if (user == nullptr) return false;
    return user->hasExtendedData();
}

bool StatsAPI::getLocalUserStat(uint16_t key, UserExtendedDataValue &value, uint32_t userIndex) const
{
    const Blaze::UserManager::LocalUser* user = getBlazeHub()->getUserManager()->getLocalUser(userIndex);
    if (user == nullptr) return false;
    return user->getDataValue(STATSCOMPONENT_COMPONENT_ID, key, value);
}

// --- skills ---
bool StatsAPI::hasSkills(const Blaze::UserManager::User *user)
{
    if (user == nullptr) return false;
    return user->hasExtendedData();
}

bool StatsAPI::getUserSkill(const Blaze::UserManager::User *user, uint16_t key, UserExtendedDataValue &value)
{
    if (user == nullptr) return false;
    return user->getDataValue(STATSCOMPONENT_COMPONENT_ID, key, value);
}

bool StatsAPI::hasLocalSkills(uint32_t userIndex) const
{
    return hasLocalStats(userIndex);
}

bool StatsAPI::getLocalUserSkill(uint16_t key, UserExtendedDataValue &value, uint32_t userIndex) const
{
    return getLocalUserStat(key, value, userIndex);
}

// --- dnf ---
bool StatsAPI::hasDnf(const Blaze::UserManager::User *user)
{
    if (user == nullptr) return false;
    return user->hasExtendedData();
}

bool StatsAPI::getUserDnf(const Blaze::UserManager::User *user, uint16_t key, UserExtendedDataValue &value)
{
    if (user == nullptr) return false;
    return user->getDataValue(STATSCOMPONENT_COMPONENT_ID, key, value);
}

bool StatsAPI::hasLocalDnf(uint32_t userIndex) const
{
    return hasLocalStats(userIndex);
}

bool StatsAPI::getLocalUserDnf(uint16_t key, UserExtendedDataValue &value, uint32_t userIndex) const
{
    return getLocalUserStat(key, value, userIndex);
}

}   // namespace Stats
}    // namespace Blaze

