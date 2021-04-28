/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
**************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/statsapi/statsapi.h"


namespace Blaze
{
    namespace Stats
    {

///////////////////////////////////////////////////////////////////////////////
//
//    class StatsGroup
//
///////////////////////////////////////////////////////////////////////////////

StatsGroup::StatsGroup(StatsAPI *api, const Stats::StatGroupResponse *statGroupData, MemoryGroupId memGroupId) :
    mViewList(memGroupId, MEM_NAME(memGroupId, "StatsGroup::mViewList")), mMemGroup(memGroupId)
{
    mStatGroupData = statGroupData->clone(mMemGroup);
    mStatsAPI = api;
}


StatsGroup::~StatsGroup()
{
    releaseViews();
}

JobId StatsGroup::createStatsView(CreateViewCb& callback,
                                  const EntityId *entityIDs, size_t entityIDCount, 
                                  StatPeriodType periodType,
                                  int32_t periodOffset, 
                                  const ScopeNameValueMap* scopeNameValueMap /* = nullptr*/,
                                  void *data /* = nullptr*/,
                                  int32_t time /* = 0 */,
                                  int32_t periodCtr /* = 1 */
                                  )
{
    StatsView *view = BLAZE_NEW(mMemGroup, "View") StatsView(mStatsAPI, this, mMemGroup);
    GroupViewNode node;
    node.view = view;
    mViewList.push_back(node);

    view->setViewId();
    view->setDataPtr(data);
    return view->refresh(callback, entityIDs, entityIDCount, periodType, 
        periodOffset, scopeNameValueMap, time, periodCtr);
}

void StatsGroup::unregisterView(StatsView *view)
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

void StatsGroup::releaseViews()
{
    while (!mViewList.empty())
    {
        mViewList.front().view->release();
    }
}


StatsView* StatsGroup::getViewById(uint32_t viewId)
{
    ViewList::iterator it = mViewList.begin();
    ViewList::iterator ite = mViewList.end();
    for (;it != ite; ++it)
    {
        if (viewId == (*it).view->getViewId())
        {
            return it->view;
        }
    }
    
    return nullptr;
}

    }   // namespace Stats
}
//    namespace Blaze
