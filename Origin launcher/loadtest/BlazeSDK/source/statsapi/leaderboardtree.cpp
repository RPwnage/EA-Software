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
#include "BlazeSDK/job.h"

namespace Blaze
{
namespace Stats
{

LeaderboardTree::LeaderboardTree(const char8_t* name, LeaderboardAPI* lbapi, MemoryGroupId memGroupId)
: mLeaderboardTreeNodeMap(memGroupId, MEM_NAME(memGroupId, "LeaderboardTree::mLeaderboardTreeNodeMap")),
mLeaderboardTreeIdMap(memGroupId, MEM_NAME(memGroupId, "LeaderboardTree::mLeaderboardTreeIdMap")),
mJobId(INVALID_JOB_ID),
mLbAPI(lbapi),
mLoadingDone(false),
mInProgress(false),
mMemGroup(memGroupId)
{
    mName = blaze_strdup(name, mMemGroup);
}

LeaderboardTree::~LeaderboardTree()
{
    BLAZE_FREE(mMemGroup, mName);
    
    LeaderboardTreeNodeMap::iterator it = mLeaderboardTreeNodeMap.begin();
    LeaderboardTreeNodeMap::iterator ite = mLeaderboardTreeNodeMap.end();
    for (;it != ite; ++it)
    {
        BLAZE_DELETE(mMemGroup, it->second);
    }
    
    mLeaderboardTreeNodeMap.clear();
}

// addNode is called when notification with node data for tree arrives. 
// Last notification in set has a flag mLastNode set to true
// which triggers restructuring data to the more accessable form
void LeaderboardTree::addNode(const Blaze::Stats::LeaderboardTreeNode* node)
{
    
    LeaderboardTreeNodeBase* treeNode; 
    
    if ( node->getFirstChild() == 0) // leaderboard
    {
        treeNode = BLAZE_NEW(mMemGroup, "LeaderboardTreeLeaderboard") LeaderboardTreeLeaderboard(node->getNodeName(), node->getShortDesc(),
            node->getFirstChild(), node->getChildCount(), node->getNodeId());
    }
    else
    {
        treeNode = BLAZE_NEW(mMemGroup, "LeaderboardTreeFolder") LeaderboardTreeFolder(node->getNodeName(), node->getShortDesc(),
            node->getFirstChild(), node->getChildCount(), node->getNodeId(), mMemGroup);
    }

    mLeaderboardTreeIdMap.insert(eastl::make_pair(treeNode->getId(), treeNode));
    mLeaderboardTreeNodeMap.insert(eastl::make_pair(treeNode->getName(), treeNode));

    if (node->getLastNode())
    {
        LeaderboardTreeNodeMap::iterator it = mLeaderboardTreeNodeMap.begin();
        LeaderboardTreeNodeMap::iterator ite = mLeaderboardTreeNodeMap.end();
        for (; it != ite; ++it)
        {
            uint32_t first = it->second->getFirstChild();
            if (first == 0) // leaderboard
            {
                continue;
            }

            uint32_t childCount = it->second->getChildCount();
            LeaderboardTreeFolder* treeFolder= static_cast<LeaderboardTreeFolder*>(it->second);
        
            for (uint32_t j = first; j < first + childCount; ++j)
            {
                LeaderboardTreeIdMap::iterator in = mLeaderboardTreeIdMap.find(j);
                if (in == mLeaderboardTreeIdMap.end()) 
                {
                    continue; // never expected to happen
                }
                LeaderboardTreeNodeBase* treeNodeTemp = in->second;
            
                if (treeNodeTemp->getFirstChild() == 0) //leaderboard
                {
                    treeFolder->addLeaderboard(treeNodeTemp);
                }
                else
                {
                    treeFolder->addFolder(treeNodeTemp);
                }
            }
        }
        
        mLeaderboardTreeIdMap.clear();
        
        // Title cb should be fired when both getLeaderboardTreeAsyncCb() and all LB tree node notifications are received.
        // mLoadingDone is set if getLeaderboardTreeAsyncCb() was already received, which means that it's our responsibility to trigger the title cb
        if (mLoadingDone)
        {
            LeaderboardAPI::LeaderboardTreeJob* lbTreeJob = (LeaderboardAPI::LeaderboardTreeJob*)mLbAPI->getBlazeHub()->getScheduler()->getJob(mJobId);
            if (lbTreeJob != nullptr)
            {
                lbTreeJob->execute();
                mLbAPI->getBlazeHub()->getScheduler()->removeJob(lbTreeJob);
            }
            
            mInProgress = false;
        }
        else  // otherwise set mLoadingDone to delegate getLeaderboardTreeAsyncCb() to trigger the title cb
        {
            mLoadingDone = true;
        }
    }
}

const LeaderboardTreeFolder* LeaderboardTree::getLeaderboardTreeFolder(const char8_t* name)
{
    LeaderboardTree::LeaderboardTreeNodeMap::const_iterator it = mLeaderboardTreeNodeMap.find(name);
    if (it == mLeaderboardTreeNodeMap.end())
    {
        return nullptr;
    }
    
    return static_cast<LeaderboardTreeFolder*>(it->second);
}

const LeaderboardTreeFolder* LeaderboardTree::getLeaderboardTreeTopFolder()
{
    return getLeaderboardTreeFolder(mName);
}

void LeaderboardTreeFolder::addLeaderboard(LeaderboardTreeNodeBase* treeNode)
{
    mLeaderboardList.push_back( static_cast<LeaderboardTreeLeaderboard*>(treeNode));
} 
 
void LeaderboardTreeFolder::addFolder(LeaderboardTreeNodeBase* treeNode)
{
    mFolderList.push_back( static_cast<LeaderboardTreeFolder*>(treeNode));
} 

LeaderboardTreeNodeBase::LeaderboardTreeNodeBase(const char8_t* name, const char8_t* shortDesc, 
        uint32_t firstChild, uint32_t childCount, uint32_t id, MemoryGroupId memGroupId)
    : mFirstChild(firstChild), 
    mChildCount(childCount), 
    mId(id),
    mMemGroup(memGroupId)
{
    mName = blaze_strdup(name, mMemGroup);
    mShortDesc = blaze_strdup(shortDesc, mMemGroup);
}

LeaderboardTreeNodeBase::~LeaderboardTreeNodeBase()
{
    BLAZE_FREE(mMemGroup, mName);
    BLAZE_FREE(mMemGroup, mShortDesc);
}

LeaderboardTreeFolder::~LeaderboardTreeFolder()
{
mLeaderboardList.clear();
mFolderList.clear();
}

} // Stats
} // Blaze
