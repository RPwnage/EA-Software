/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
**************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/statsapi/lbapi.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/usermanager/usermanager.h"


namespace Blaze
{
    namespace Stats
    {

///////////////////////////////////////////////////////////////////////////////
//
//    class LeaderboardAPI
//
///////////////////////////////////////////////////////////////////////////////

void LeaderboardAPI::createAPI(BlazeHub &hub, EA::Allocator::ICoreAllocator* allocator)
{
    if (hub.getLeaderboardAPI() != nullptr)
    {
        BLAZE_SDK_DEBUGF("[LeaderboardAPI] Warning: Ignoring attempt to recreate API\n");
        return;
    }
    if (Blaze::Allocator::getAllocator(MEM_GROUP_LEADERBOARD) == nullptr)
        Blaze::Allocator::setAllocator(MEM_GROUP_LEADERBOARD, allocator!=nullptr? allocator : Blaze::Allocator::getAllocator());
    Stats::StatsComponent::createComponent(&hub);            // dependency  
    hub.createAPI(LEADERBOARD_API, BLAZE_NEW(MEM_GROUP_LEADERBOARD, "createAPI") LeaderboardAPI(hub, MEM_GROUP_LEADERBOARD));
}


LeaderboardAPI::LeaderboardAPI(BlazeHub &hub, MemoryGroupId memGroupId) 
    : SingletonAPI(hub),
    mLBTable(memGroupId, MEM_NAME(memGroupId, "LeaderboardAPI::mLBFolderTable")),
    mLBFolderTable(memGroupId, MEM_NAME(memGroupId, "LeaderboardAPI::mLBFolderTable")),
    mLBFolderNameTable(memGroupId, MEM_NAME(memGroupId, "LeaderboardAPI::mLBFolderNameTable")),
    mLeaderboardTreeMap(memGroupId, MEM_NAME(memGroupId, "LeaderboardAPI::mLeaderboardTreeMap")),
    mMemGroup(memGroupId)
{
    // Add the callback to all users (since this is a SingletonAPI)
    for (uint32_t userIndex = 0; userIndex < getBlazeHub()->getNumUsers(); userIndex++)
    {
        Stats::StatsComponent *component = getBlazeHub()->getComponentManager(userIndex)->getStatsComponent(); 
        component->setGetLeaderboardTreeNotificationHandler(StatsComponent::GetLeaderboardTreeNotificationCb(this, &LeaderboardAPI::onGetLeaderboardTreeNotification));
    }
}


LeaderboardAPI::~LeaderboardAPI()
{
    for (uint32_t userIndex = 0; userIndex < getBlazeHub()->getNumUsers(); userIndex++)
    {
        Stats::StatsComponent *component = getBlazeHub()->getComponentManager(userIndex)->getStatsComponent();
        if (component)
            component->clearGetLeaderboardTreeNotificationHandler();
    }

    releaseLeaderboardHierarchy();
    
    LeaderboardTreeMap::iterator it = mLeaderboardTreeMap.begin();
    LeaderboardTreeMap::iterator ite = mLeaderboardTreeMap.end();
    for (; it != ite; ++it)
    {
        BLAZE_DELETE(mMemGroup, it->second);
    }
    
    mLeaderboardTreeMap.clear();
}


void LeaderboardAPI::logStartupParameters() const
{
}



void LeaderboardAPI::onDeAuthenticated(uint32_t userIndex)
{
    // only clean up if last local user
    if (userIndex == getBlazeHub()->getPrimaryLocalUserIndex())
    {
        releaseLeaderboardHierarchy();
    }
}


void LeaderboardAPI::deactivate()
{
    releaseLeaderboardHierarchy();
    API::deactivate();
}
        
JobId LeaderboardAPI::requestTopLeaderboardFolder(GetLeaderboardFolderCb& cb)
{
    const uint32_t topFolderId = Stats::LeaderboardFolderGroup::TOP_FOLDER;
    LeaderboardFolder *topFolder = findLeaderboardFolder(topFolderId);
    if (topFolder != nullptr)
    {
        JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
        JobId jobId = jobScheduler->reserveJobId();
        jobId = jobScheduler->scheduleFunctor("requestTopLeaderboardFolderCb", cb, ERR_OK, jobId, topFolder, this, 0, jobId);
        Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, cb);
        return jobId;
    }
    
    return retrieveLeaderboardFolder(cb, topFolderId, nullptr);
}


JobId LeaderboardAPI::requestLeaderboardFolder(GetLeaderboardFolderCb& cb, const char *name)
{
    LeaderboardFolder *folder = findLeaderboardFolderByName(name);
    if (folder != nullptr)
    {
        JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
        JobId jobId = jobScheduler->reserveJobId();
        jobId = jobScheduler->scheduleFunctor("requestLeaderboardFolderCb", cb, ERR_OK, jobId, folder, this, 0, jobId);
        Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, cb);
        return jobId;
    }
    
    return retrieveLeaderboardFolder(cb, -1, name);   
}



JobId LeaderboardAPI::requestLeaderboard(GetLeaderboardCb& cb, const char *name)
{
    Leaderboard *lb = findLeaderboard(name);
    if (lb != nullptr)
    {
        JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
        JobId jobId = jobScheduler->reserveJobId();
        jobId = jobScheduler->scheduleFunctor("requestLeaderboardCb", cb, ERR_OK, jobId, lb, this, 0, jobId);
        Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, cb);
        return jobId;
    }

    return retrieveLeaderboard(cb, -1, name);
}


///////////////////////////////////////////////////////////////////////////////

//    lookup of  (useful for tree traversal)
Leaderboard* LeaderboardAPI::findLeaderboard(const char *name)
{
    LeaderboardTable::iterator it = mLBTable.find(name);
    return (it != mLBTable.end()) ? (*it).second : nullptr;
}


LeaderboardFolder* LeaderboardAPI::findLeaderboardFolder(int32_t id)
{
    LeaderboardFolderTable::iterator it = mLBFolderTable.find(id);
    return (it != mLBFolderTable.end()) ? (*it).second : nullptr;
}


LeaderboardFolder* LeaderboardAPI::findLeaderboardFolderByName(const char *name)
{
    LeaderboardFolderNameTable::iterator it = mLBFolderNameTable.find(name);
    return (it != mLBFolderNameTable.end()) ? (*it).second : nullptr;
}


//    factory for Leaderboard and LeaderboardFolder objects - retrieves data from
//    server and adds the result to the appropriate table
//    note: for now, calls must be chained - callback must finish before starting a new request.
JobId LeaderboardAPI::retrieveLeaderboard(GetLeaderboardCb& cb, int32_t id, const char *name)
{
    Stats::StatsComponent *component = getBlazeHub()->getComponentManager()->getStatsComponent();
    Stats::LeaderboardGroupRequest req;
    req.setBoardId(id);
    if (name != nullptr)
    {
        req.setBoardName(name);
    }

    JobId jobId = component->getLeaderboardGroup(req, 
        MakeFunctor(this, &LeaderboardAPI::getLeaderboardGroupCb), cb);
    Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, cb);
    return jobId;
}


void LeaderboardAPI::getLeaderboardGroupCb(const Stats::LeaderboardGroupResponse *resp, BlazeError err, JobId id, GetLeaderboardCb cb)
{
    Leaderboard *leaderboard = nullptr;

    if (err == ERR_OK)
    {
        //    Verify that the retrieved leaderboard object isn't already resident in the lookup table.
        //    The check here is redundant since the caller is internal and should've checked the table 
        //    prior to issuing the RPC.
        
        
        LeaderboardTable::iterator itf = mLBTable.find(resp->getBoardName());
        if (itf == mLBTable.end())
        {
            leaderboard = BLAZE_NEW(mMemGroup, "Leaderboard") Leaderboard(this, resp, mMemGroup);
            mLBTable.insert(eastl::make_pair(leaderboard->getName(), leaderboard));
        }
        else
        {
            //    request slipped through the issuer's pre-RPC check.  return the found leaderboard.
            BlazeVerify(false);
            leaderboard = itf->second;
            BlazeAssert(leaderboard != nullptr);
        }
    }

    cb(err, id, static_cast<Leaderboard*>(leaderboard));
}  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/


JobId LeaderboardAPI::retrieveLeaderboardFolder(GetLeaderboardFolderCb& cb, int32_t id, const char *name)
{
    Stats::StatsComponent *component = getBlazeHub()->getComponentManager()->getStatsComponent();
    Stats::LeaderboardFolderGroupRequest req;
    if (id > 0)
    {
        req.setFolderId(static_cast<uint32_t>(id));
    }
    else if (name != nullptr)
    {
        req.setFolderName(name);
    }

    JobId jobId = component->getLeaderboardFolderGroup(req,            
        MakeFunctor(this, &LeaderboardAPI::getLeaderboardFolderGroupCb), cb);
    Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, cb);
    return jobId;

}


void LeaderboardAPI::getLeaderboardFolderGroupCb(const Stats::LeaderboardFolderGroup *resp, BlazeError err, JobId id, GetLeaderboardFolderCb cb)
{
    LeaderboardFolder *folder = nullptr;

    if (err == ERR_OK)
    {
        //    Verify that the retrieved leaderboard object isn't already resident in the lookup table.
        //    The check here is redundant since the caller is internal and should've checked the table 
        //    prior to issuing the RPC.
        LeaderboardFolderTable::insert_return_type inserter = mLBFolderTable.insert(static_cast<int32_t>(resp->getFolderId()));
        if (inserter.second)
        {
            folder = BLAZE_NEW(mMemGroup, "Folder") LeaderboardFolder(this, resp, mMemGroup);
            inserter.first->second = folder;            
        }
        else
        {
            //    request slipped through the issuer's pre-RPC check.  return the found leaderboard.
            BlazeVerify(false);
            folder = inserter.first->second;
            BlazeAssert(folder != nullptr);
        }

        LeaderboardFolderNameTable::insert_return_type inserter2 = mLBFolderNameTable.insert(folder->getName());
        inserter2.first->second = folder;
    }

    cb(err, id, static_cast<LeaderboardFolder*>(folder));
}  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/


void LeaderboardAPI::releaseLeaderboardHierarchy()
{
    //  kill any job timers started up
    getBlazeHub()->getScheduler()->cancelByAssociatedObject(reinterpret_cast<void*>(this));

    //    clear all tables
    mLBFolderNameTable.clear();

    LeaderboardFolderTable::iterator itEnd = mLBFolderTable.end();
    for (LeaderboardFolderTable::iterator it = mLBFolderTable.begin(); it != itEnd; ++it)
    {
        BLAZE_DELETE_PRIVATE(mMemGroup, LeaderboardFolder, (*it).second);
    }
    mLBFolderTable.clear();

    LeaderboardTable::iterator itEnd2 = mLBTable.end();
    for (LeaderboardTable::iterator it2 = mLBTable.begin(); it2 != itEnd2; ++it2)
    {
        BLAZE_DELETE_PRIVATE(mMemGroup, Leaderboard, (*it2).second);
    }
    mLBTable.clear();
    
    // Remove all cached leaderboard sub-trees
    LeaderboardTreeMap::iterator it;
    while (!mLeaderboardTreeMap.empty())
    {
        it = mLeaderboardTreeMap.begin();
        LeaderboardTree* tree = it->second;
        mDispatcher.dispatch("onLeaderboardDelete", &LeaderboardAPIListener::onLeaderboardDelete, tree);
        BLAZE_DELETE(mMemGroup, it->second);
        mLeaderboardTreeMap.erase(it);
    }        
}

// -------- leaderboard tree
JobId LeaderboardAPI::requestLeaderboardTree(GetLeaderboardTreeCb& cb, const char* name)
{
    Stats::StatsComponent *component = getBlazeHub()->getComponentManager()->getStatsComponent();
    Stats::GetLeaderboardTreeRequest req;
    req.setFolderName(name);

    LeaderboardTree* leaderboardTree;
    LeaderboardTreeMap::iterator it = mLeaderboardTreeMap.find(name);
    if (it == mLeaderboardTreeMap.end())
    {
        leaderboardTree = BLAZE_NEW(mMemGroup, "LeaderboardTree")  LeaderboardTree(name, this, mMemGroup);
        mLeaderboardTreeMap.insert(eastl::make_pair(leaderboardTree->getName(), leaderboardTree));
        leaderboardTree->setInProgress(true);

        LeaderboardTreeJob* lbTreeJob = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "LeaderboardTreeJob") LeaderboardTreeJob(this, cb, leaderboardTree);
        JobId jobId = getBlazeHub()->getScheduler()->scheduleJobNoTimeout("LeaderboardTreeJob", lbTreeJob, this);
        leaderboardTree->setJobId(jobId);

        component->getLeaderboardTreeAsync(req,            
            MakeFunctor(this, &LeaderboardAPI::getLeaderboardTreeAsyncCb), cb, leaderboardTree, jobId);

        return jobId;
    }
    else
    {
        JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
        JobId jobId = jobScheduler->reserveJobId();

        leaderboardTree = it->second;
        if (leaderboardTree->isInProgress()) // operation in progress
        {
            jobId = jobScheduler->scheduleFunctor("requestLeaderboardTreeCb", cb, STATS_ERR_OPERATION_IN_PROGRESS, jobId, (LeaderboardTree*)nullptr, this, 0, jobId);
            Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, cb);
            return jobId;
        }
    
        // already loaded
        jobId = jobScheduler->scheduleFunctor("requestLeaderboardTreeCb", cb, ERR_OK, jobId, leaderboardTree, this, 0, jobId);
        Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, cb);
        return jobId;
    }
}

void LeaderboardAPI::getLeaderboardTreeAsyncCb(BlazeError err, JobId rpcJobId, const GetLeaderboardTreeCb cb, LeaderboardTree* leaderboardTree, JobId jobId)
{
    LeaderboardTreeJob* lbTreeJob = (LeaderboardTreeJob*)getBlazeHub()->getScheduler()->getJob(jobId);
    if (lbTreeJob == nullptr)
    {
        BLAZE_SDK_DEBUGF("[LB] Leader board tree job(%u) was canceled before internal cb.\n", jobId.get());
        LeaderboardTreeMap::iterator it = mLeaderboardTreeMap.find(leaderboardTree->getName());
        if (it != mLeaderboardTreeMap.end())
        {
            BLAZE_DELETE(mMemGroup, leaderboardTree);
            mLeaderboardTreeMap.erase(it);
        }
        return;
    }

    if (err != ERR_OK)
    {
        LeaderboardTreeMap::iterator it = mLeaderboardTreeMap.find(leaderboardTree->getName());
        if (it != mLeaderboardTreeMap.end())
        {
            BLAZE_DELETE(mMemGroup, leaderboardTree);
            mLeaderboardTreeMap.erase(it);
        }
        
        lbTreeJob->setArg1(nullptr);
        lbTreeJob->cancel(err);
        getBlazeHub()->getScheduler()->removeJob(lbTreeJob);
    }
    else
    {
        // For ERR_OK, the server getLeaderboardTreeAsync RPC is guaranteed to send the client at least one tree node notification.
        // Title cb should be fired when both getLeaderboardTreeAsyncCb() and all LB tree node notifications are received.
        // mLoadingDone is set if LeaderboardTree::addNode() got the last tree node, which means that it's our responsibility to trigger the title cb
        if (leaderboardTree->mLoadingDone)
        {
            cb(ERR_OK, leaderboardTree->mJobId, leaderboardTree);
            getBlazeHub()->getScheduler()->removeJob(lbTreeJob);

            leaderboardTree->setInProgress(false);
        }
        else  // else set mLoadingDone so to delegate LeaderboardTree::addNode() to trigger the title cb
        {
            leaderboardTree->mLoadingDone = true;
        }
    }    
    
    return;
} /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

void LeaderboardAPI::onGetLeaderboardTreeNotification(const LeaderboardTreeNodes* treeNodes, uint32_t userIndex)
{
    LeaderboardTreeNodes::LeaderboardTreeNodeList::const_iterator tnIt = treeNodes->getLeaderboardTreeNodes().begin();
    LeaderboardTreeNodes::LeaderboardTreeNodeList::const_iterator tnEnd = treeNodes->getLeaderboardTreeNodes().end();

    for ( ; tnIt != tnEnd; ++tnIt)
    {
        LeaderboardTreeMap::const_iterator it = mLeaderboardTreeMap.find((*tnIt)->getRootName());
        if (it != mLeaderboardTreeMap.end())
        {
            LeaderboardTree* leaderboardTree = it->second;
            leaderboardTree->addNode(*tnIt);
        }
    }

    return;
}

bool LeaderboardAPI::removeLeaderboardTree(const char8_t* name)
{
    LeaderboardTreeMap::iterator it = mLeaderboardTreeMap.find(name);
    if (it != mLeaderboardTreeMap.end())
    {
        BLAZE_DELETE(mMemGroup, it->second);
        mLeaderboardTreeMap.erase(it);
        return true;
    }
    
    return false;
}

// --- User sesion extended data ---
bool LeaderboardAPI::hasRanks(const Blaze::UserManager::User *user)
{
    if (user == nullptr) return false;
    return user->hasExtendedData();
}

bool LeaderboardAPI::getUserRank(const Blaze::UserManager::User *user, uint16_t key, UserExtendedDataValue &value)
{
    if (user == nullptr) return false;
    return user->getDataValue(STATSCOMPONENT_COMPONENT_ID, key, value);
}

bool LeaderboardAPI::hasLocalRanks(uint32_t userIndex) const
{
    const Blaze::UserManager::LocalUser* user = getBlazeHub()->getUserManager()->getLocalUser(userIndex);
    if (user == nullptr) return false;
    return user->hasExtendedData();
}

bool LeaderboardAPI::getLocalUserRank(uint16_t key, UserExtendedDataValue &value, uint32_t userIndex) const
{
    const Blaze::UserManager::LocalUser* user = getBlazeHub()->getUserManager()->getLocalUser(userIndex);
    if (user == nullptr) return false;
    return user->getDataValue(STATSCOMPONENT_COMPONENT_ID, key, value);
}

void LeaderboardAPI::addListener(LeaderboardAPIListener* listener)
{
    mDispatcher.addDispatchee(listener);
}

void LeaderboardAPI::removeListener(LeaderboardAPIListener* listener)
{
    mDispatcher.removeDispatchee(listener);
}

}   // namespace Stats
}
// namespace Blaze
