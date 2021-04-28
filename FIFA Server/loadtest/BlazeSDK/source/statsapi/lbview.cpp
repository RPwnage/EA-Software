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

///////////////////////////////////////////////////////////////////////////////
//
//    class StatsView
//
///////////////////////////////////////////////////////////////////////////////

LeaderboardView::LeaderboardView(LeaderboardAPI &api, Leaderboard &lb, MemoryGroupId memGroupId)
: mStatValues(memGroupId), mScopeNameValueMap(memGroupId, MEM_NAME(memGroupId, "LeaderboardView::mScopeNameValueMap")), mMemGroup(memGroupId)
{  
    mLBAPI = &api;
    mLeaderboard = &lb;
    mData = nullptr;
    mUserSetId = EA::TDF::OBJECT_ID_INVALID;
    mUseRawStats = false;
}


LeaderboardView::~LeaderboardView()
{
    mLeaderboard->unregisterView(this);
    // Remove any outstanding txns.  Canceling here can be dangerous here as it will still attempt
    // to call the callback.  Since the object is being deleted, we go with the remove.
    mLBAPI->getBlazeHub()->getScheduler()->removeByAssociatedObject(reinterpret_cast<void*>(this));   
}

void LeaderboardView::release()
{
    BLAZE_DELETE_PRIVATE(mMemGroup, LeaderboardView, this);
}


void LeaderboardView::setStatValues(const Stats::LeaderboardStatValues* stats)
{
    if (stats == nullptr)
    {
        // Remove old entries
        mStatValues.getRows().release();
    }
    else
    {
        stats->copyInto(mStatValues);
    }
}

void LeaderboardView::copyScopeMap(Stats::ScopeNameValueMap *srcMap, Stats::ScopeNameValueMap *destMap) const
{
    if ((srcMap == nullptr) || (destMap == nullptr)) 
        return;
        
    Stats::ScopeNameValueMap::const_iterator scopeIt = srcMap->begin();
    Stats::ScopeNameValueMap::const_iterator scopeEnd = srcMap->end();
    for (; scopeIt != scopeEnd; ++scopeIt)
    {
        destMap->insert(eastl::make_pair(scopeIt->first, scopeIt->second));
    }
}

void LeaderboardView::copyScopeMap(Stats::ScopeNameValueMap *destMap)
{
    copyScopeMap(&mScopeNameValueMap, destMap);
}

void LeaderboardView::setScopeNameValueMap(Stats::ScopeNameValueMap *map)
{
    copyScopeMap(map, &mScopeNameValueMap);
}

///////////////////////////////////////////////////////////////////////////////
//
//    class GlobalLeaderboardView
//
///////////////////////////////////////////////////////////////////////////////

GlobalLeaderboardView::GlobalLeaderboardView(LeaderboardAPI &api, Leaderboard& lb, MemoryGroupId memGroupId)
    : LeaderboardView(api, lb, memGroupId)
{
    mStartRank = 0;
    mEndRank = 0;
    mCenteredEntityId = 0;
    mRowCount = 0;
    mIsCentered = false;
    mShowAtBottomIfNotFound = false;
}


GlobalLeaderboardView::~GlobalLeaderboardView()
{
}


JobId GlobalLeaderboardView::refresh(LeaderboardView::GetStatsResultCb& callback, int32_t startRank, int32_t endRank, 
    int32_t periodOffset, int32_t time, EA::TDF::ObjectId userSetId)
{
    mStartRank = startRank;
    mEndRank = endRank;
    mCenteredEntityId = 0;
    mRowCount = static_cast<size_t>(endRank - startRank)+1;
    setPeriodOffset(periodOffset);
    mIsCentered = false;
    mShowAtBottomIfNotFound = false;
    mTime = time;
    mUserSetId = userSetId;

    return refresh(callback);
}  /*lint !e1746 Avoid lint to check whether parameter 'userSetId' could be made const reference, because it has a default value*/


JobId GlobalLeaderboardView::refresh(LeaderboardView::GetStatsResultCb& callback, EntityId centeredEntityId, 
    size_t count, int32_t periodOffset, int32_t time, EA::TDF::ObjectId userSetId, bool showAtBottomIfNotFound)
{
    mStartRank = 0;
    mEndRank = 0;
    mCenteredEntityId = centeredEntityId;
    mRowCount = count;
    setPeriodOffset(periodOffset);
    mIsCentered = true;
    mShowAtBottomIfNotFound = showAtBottomIfNotFound;
    mTime = time;
    mUserSetId = userSetId;

    return refresh(callback);
} /*lint !e1746 Avoid lint to check whether parameter 'userSetId' could be made const reference, because it has a default value*/


JobId GlobalLeaderboardView::refresh(GetStatsResultCb& callback)
{
    Stats::StatsComponent *statsComp = 
        mLBAPI->getBlazeHub()->getComponentManager()->getStatsComponent();

    JobId result;

    // Clear mStatValues
    mStatValues.getRows().release();

    if (mIsCentered)
    {
        Stats::CenteredLeaderboardStatsRequest req;
        req.setBoardName(getName());
        req.setPeriodOffset(getPeriodOffset());
        req.setTime(mTime);
        req.setUserSetId(mUserSetId);
        req.setCenter(mCenteredEntityId);
        req.setShowAtBottomIfNotFound(mShowAtBottomIfNotFound);
        req.setCount(static_cast<int32_t>(mRowCount));
        copyScopeMap(&req.getKeyScopeNameValueMap());
        
        // Pass address of member TDF (mStatValues) in which we want the response to be decoded
        if (!mUseRawStats)
        {
            result = statsComp->getCenteredLeaderboard(req, 
                MakeFunctor(this, &GlobalLeaderboardView::getStatsByGroupCb), callback, INVALID_JOB_ID, USE_DEFAULT_TIMEOUT, &mStatValues);
        }
        else
        {
            result = statsComp->getCenteredLeaderboardRaw(req, 
                MakeFunctor(this, &GlobalLeaderboardView::getStatsByGroupCb), callback, INVALID_JOB_ID, USE_DEFAULT_TIMEOUT, &mStatValues);
        }
        Job::addTitleCbAssociatedObject(mLBAPI->getBlazeHub()->getScheduler(), result, callback);
    }
    else
    {
        Stats::LeaderboardStatsRequest req;
        req.setBoardName(getName());
        req.setPeriodOffset(getPeriodOffset());
        req.setTime(mTime);
        req.setUserSetId(mUserSetId);
        req.setRankStart(getStartRank());
        req.setCount(static_cast<int32_t>(mRowCount));
        copyScopeMap(&req.getKeyScopeNameValueMap());
        
        // Pass address of member TDF (mStatValues) in which we want the response to be decoded
        if (!mUseRawStats)
        {
            result = statsComp->getLeaderboard(req, 
                MakeFunctor(this, &GlobalLeaderboardView::getStatsByGroupCb), callback, INVALID_JOB_ID, USE_DEFAULT_TIMEOUT, &mStatValues);
        }
        else
        {
            result = statsComp->getLeaderboardRaw(req, 
                MakeFunctor(this, &GlobalLeaderboardView::getStatsByGroupCb), callback, INVALID_JOB_ID, USE_DEFAULT_TIMEOUT, &mStatValues);
        }
        Job::addTitleCbAssociatedObject(mLBAPI->getBlazeHub()->getScheduler(), result, callback);
    }

    return result;
}

void GlobalLeaderboardView::getStatsByGroupCb(const Stats::LeaderboardStatValues* stats, BlazeError err, JobId id, GetStatsResultCb cb)
{
    // assumption: mStatValues was passed to the job by the requester, and filled in by the decoder.
    
    if (err != ERR_OK)
    {
        setStatValues(nullptr);
    }

    cb(err, id, this);
} /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/


///////////////////////////////////////////////////////////////////////////////
//
//    class FilteredLeaderboardView
//
///////////////////////////////////////////////////////////////////////////////

FilteredLeaderboardView::FilteredLeaderboardView(LeaderboardAPI &api, Leaderboard& lb, MemoryGroupId memGroupId)
    : LeaderboardView(api, lb, memGroupId),
    mEntityIDList(memGroupId, MEM_NAME(memGroupId, "FilteredLeaderboardView::mEntityIdList")),
    mIncludeStatlessEntities(true),
    mEnforceCutoffStatValue(false),
    mLimit(UINT32_MAX)
{
    
}


FilteredLeaderboardView::~FilteredLeaderboardView()
{
}


const EntityId* FilteredLeaderboardView::getEntityIDs(size_t *outCount) const
{
    *outCount = mEntityIDList.size();
    return mEntityIDList.data();
}


JobId FilteredLeaderboardView::refresh(GetStatsResultCb& callback,
                                       const EntityId *entityIds, size_t entityIDCount, 
                                       int32_t periodOffset, int32_t time, 
                                       EA::TDF::ObjectId userSetId, bool includeStatlessEntities /*= true */,
                                       uint32_t limit /*=UINT32_MAX*/, bool enforceCutoffStatValue/* = false */)
{
    //    EASTL vector implementation - more optimal way to resize (if necessary) and 
    //    copy the new entity array into the view's entity container.
    if (entityIds != nullptr && entityIDCount > 0)
    {
        mEntityIDList.resize((EntityIDList::size_type)entityIDCount);
        EntityIDList::pointer data = mEntityIDList.data();
        memcpy(data, entityIds, sizeof(EntityId) * mEntityIDList.size());
    }
    else
    {
        mEntityIDList.resize(0);
    }

    mIncludeStatlessEntities = includeStatlessEntities;
    mEnforceCutoffStatValue = enforceCutoffStatValue;
    setPeriodOffset(periodOffset);
    mTime = time;
    mUserSetId = userSetId;
    mLimit = limit;

    return refresh(callback);
} /*lint !e1746 Avoid lint to check whether parameter 'userSetId' could be made const reference, because it has a default value*/


JobId FilteredLeaderboardView::refresh(GetStatsResultCb& callback)
{    
    Stats::StatsComponent *statsComp = 
        mLBAPI->getBlazeHub()->getComponentManager()->getStatsComponent();

    Stats::FilteredLeaderboardStatsRequest req;

    //    fill entity id list
    Stats::FilteredLeaderboardStatsRequest::EntityIdList &idList = req.getListOfIds();
    idList.clear();
    size_t numIds = 0;
    const EntityId* entityIds = this->getEntityIDs(&numIds);
    for (size_t idx = 0; idx < numIds; ++idx)
        idList.push_back(entityIds[idx]);

    req.setTime(mTime);
    req.setUserSetId(mUserSetId);
    req.setPeriodOffset(getPeriodOffset());
    req.setBoardName(getLeaderboard()->getName());
    copyScopeMap(&req.getKeyScopeNameValueMap());
    req.setIncludeStatlessEntities(mIncludeStatlessEntities);
    req.setEnforceCutoffStatValue(mEnforceCutoffStatValue);
    req.setLimit(mLimit);

    // Clear mStatValues
    mStatValues.getRows().release();

    // Pass address of member TDF (mStatValues) in which we want the response to be decoded
    JobId jobId;
    if (!mUseRawStats)
    {
        jobId = statsComp->getFilteredLeaderboard(req, MakeFunctor(this, &FilteredLeaderboardView::getStatsByGroupCb), callback, INVALID_JOB_ID, USE_DEFAULT_TIMEOUT, &mStatValues);
    }
    else
    {
        jobId = statsComp->getFilteredLeaderboardRaw(req, MakeFunctor(this, &FilteredLeaderboardView::getStatsByGroupCb), callback, INVALID_JOB_ID, USE_DEFAULT_TIMEOUT, &mStatValues);
    }
    Job::addTitleCbAssociatedObject(mLBAPI->getBlazeHub()->getScheduler(), jobId, callback);
    return jobId;
}


void FilteredLeaderboardView::getStatsByGroupCb(const Stats::LeaderboardStatValues* stats, BlazeError err, JobId id, GetStatsResultCb cb)
{
    // assumption: mStatValues was passed to the job by the requester, and filled in by the decoder.
    
    if (err != ERR_OK)
    {
        setStatValues(nullptr);
    }

    cb(err, id, this);
} /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    }   // namespace Stats
}    
// namespace Blaze

