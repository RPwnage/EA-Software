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


namespace Blaze
{
    namespace Stats
    {

///////////////////////////////////////////////////////////////////////////////
//
//    class Leaderboard
//
///////////////////////////////////////////////////////////////////////////////

Leaderboard::Leaderboard(LeaderboardAPI *api, const Stats::LeaderboardGroupResponse *groupData, MemoryGroupId memGroupId)    :
    mGroupData(memGroupId), mViewList(memGroupId, MEM_NAME(memGroupId, "Leaderboard:mViewList"))
{
    mLBAPI = api;
    groupData->copyInto(mGroupData);
}


Leaderboard::~Leaderboard()
{
    releaseViews();
}


JobId Leaderboard::createGlobalLeaderboardView(CreateGlobalViewCb& callback, int32_t startRank, int32_t endRank,
            Stats::ScopeNameValueMap *scopeNameValueMap, int32_t periodOffset, void *data, int32_t time,
            EA::TDF::ObjectId userSetId, bool useRawStats)
{
    GlobalLeaderboardView *view = BLAZE_NEW(MEM_GROUP_LEADERBOARD, "GlobalView") GlobalLeaderboardView(*mLBAPI, *this, MEM_GROUP_LEADERBOARD);
    GroupViewNode node;
    node.globalCb = callback;
    node.view = view;
    mViewList.push_back(node);
    view->setScopeNameValueMap(scopeNameValueMap);

    view->setDataPtr(data);
    view->setUseRawStats( useRawStats );
    LeaderboardView::GetStatsResultCb refreshCb(this, &Leaderboard::initialViewRefreshCb);
    return view->refresh(refreshCb, startRank, endRank, periodOffset, time, userSetId);
}  /*lint !e1746 Avoid lint to check whether parameter 'userSetId' could be made const reference, because it has a default value*/


JobId Leaderboard::createGlobalLeaderboardViewCentered(CreateGlobalViewCb& callback, EntityId centeredEntityId, size_t count,
            Stats::ScopeNameValueMap *scopeNameValueMap, int32_t periodOffset, void *data, int32_t time,
            EA::TDF::ObjectId userSetId, bool showAtBottomIfNotFound, bool useRawStats)
{
    GlobalLeaderboardView *view = BLAZE_NEW(MEM_GROUP_LEADERBOARD, "GlobalView") GlobalLeaderboardView(*mLBAPI, *this, MEM_GROUP_LEADERBOARD);
    GroupViewNode node;
    node.globalCb = callback;
    node.view = view;
    mViewList.push_back(node);
    view->setScopeNameValueMap(scopeNameValueMap);

    view->setDataPtr(data);
    view->setUseRawStats( useRawStats );
    LeaderboardView::GetStatsResultCb refreshCb(this, &Leaderboard::initialViewRefreshCb);
    return view->refresh(refreshCb, centeredEntityId, count, periodOffset, time, userSetId, showAtBottomIfNotFound);
} /*lint !e1746 Avoid lint to check whether parameter 'userSetId' could be made const reference, because it has a default value*/


JobId Leaderboard::createFilteredLeaderboardView(CreateFilteredViewCb& callback, const EntityId *entityIDs, size_t entityIDCount,
                                                 Stats::ScopeNameValueMap *scopeNameValueMap, int32_t periodOffset, void *data, 
                                                 int32_t time, EA::TDF::ObjectId userSetId , bool includeStatlessEntities /*= true */,
                                                 uint32_t limit /*=UINT32_MAX*/, bool useRawStats/* = false */, bool enforceCutoffStatValue/* = false */)
{
    FilteredLeaderboardView *view = BLAZE_NEW(MEM_GROUP_LEADERBOARD, "FilteredView") FilteredLeaderboardView(*mLBAPI, *this, MEM_GROUP_LEADERBOARD);
    GroupViewNode node;
    node.filteredCb = callback;
    node.view = view;
    mViewList.push_back(node);
    view->setScopeNameValueMap(scopeNameValueMap);

    view->setDataPtr(data);
    view->setUseRawStats(useRawStats);
    LeaderboardView::GetStatsResultCb refreshCb(this, &Leaderboard::initialViewRefreshCb);
    return view->refresh(refreshCb, entityIDs, entityIDCount, periodOffset, time, userSetId, includeStatlessEntities, limit, enforceCutoffStatValue);
} /*lint !e1746 Avoid lint to check whether parameter 'userSetId' could be made const reference, because it has a default value*/


//    view creation callback - initial stats refresh of the LeaderboardView
void Leaderboard::initialViewRefreshCb(BlazeError error, JobId id, LeaderboardView *view)
{
    MatchViewWithNode pred;
    ViewList::iterator viewItEnd = mViewList.end();
    ViewList::iterator viewIt = eastl::find_if(mViewList.begin(), viewItEnd, eastl::bind2nd(pred, view));
    if (viewIt == viewItEnd)
    {
        //    View initial refresh callback not found.   If the app hits this, it's a critical error that we can't report.
        BlazeAssert(viewIt != viewItEnd);
        return;
    }

    GroupViewNode vn = *viewIt;    

    if (vn.globalCb.isValid())
    {
        if (error == ERR_OK)
        {
            vn.globalCb(ERR_OK, id, static_cast<GlobalLeaderboardView*>(vn.view));
        }
        else
        {
            vn.globalCb(error, id, nullptr);
            vn.view->release();
        }        
    }
    else if (vn.filteredCb.isValid())
    {
        if (error == ERR_OK)
        {
            vn.filteredCb(ERR_OK, id, static_cast<FilteredLeaderboardView*>(vn.view));
        }
        else
        {
            vn.filteredCb(error, id, nullptr);
            vn.view->release();
        }
    }
    else
    {
        //    never should get here - no valid response callback for the caller.
        BlazeAssert(false);
    }
} /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

// Retrive number of enties in the leaderboard
JobId Leaderboard::getEntityCount(GetEntityCountCb& callback, 
        Stats::ScopeNameValueMap* scopeNameValueMap, int32_t periodOffset)
{      
     Stats::StatsComponent *statsComp = 
        mLBAPI->getBlazeHub()->getComponentManager()->getStatsComponent();

    Stats::LeaderboardEntityCountRequest req;
    req.setBoardName(mGroupData.getBoardName());
    req.setBoardId(0);
    req.setPeriodOffset(periodOffset);
    
    if (scopeNameValueMap != nullptr)
    {
        ScopeNameValueMap::const_iterator it = scopeNameValueMap->begin();
        ScopeNameValueMap::const_iterator ite = scopeNameValueMap->end();
        for (; it != ite; ++it)
        {
            req.getKeyScopeNameValueMap().insert(eastl::make_pair(it->first, it->second));
        }
    }
        
    JobId jobId = statsComp->getLeaderboardEntityCount(req, MakeFunctor(this, &Leaderboard::getEntityCountResultCb), callback);
    Job::addTitleCbAssociatedObject(mLBAPI->getBlazeHub()->getScheduler(), jobId, callback);
    return jobId;
}

void Leaderboard::getEntityCountResultCb(const Blaze::Stats::EntityCount* entityCount, BlazeError err, JobId id, GetEntityCountCb cb)
{
    if (err == ERR_OK)
    {
        cb(err, id, entityCount->getCount());
    }
    else
    {
        cb(err, id, 0);
    }
} /*lint !e1746 !e1762 - Avoid lint to check whether parameter or function could be made const reference or const*/

void Leaderboard::unregisterView(LeaderboardView *view)
{
    MatchViewWithNode pred;
    ViewList::iterator viewItEnd = mViewList.end();
    ViewList::iterator viewIt = eastl::find_if(mViewList.begin(), viewItEnd, eastl::bind2nd(pred, view));
    if (viewIt == viewItEnd)
    {
        //    Application attempted to release a view not managed by this group.
        BlazeAssert(viewIt != viewItEnd);
        return;
    }

    mViewList.erase(viewIt);
}


void Leaderboard::releaseViews()
{
    while (!mViewList.empty())
    {
        mViewList.front().view->release();
    }
}

    }   // namespace Stats
}
// namespace Blaze

