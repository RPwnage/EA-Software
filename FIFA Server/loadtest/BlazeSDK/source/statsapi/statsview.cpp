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

StatsView::StatsView(StatsAPI *api, StatsGroup *group, MemoryGroupId memGroupId) :
    mKeyScopeStatsValueMap(memGroupId, MEM_NAME(memGroupId, "StatsView::mKeyScopeStatsValueMap")),
    mEntityIDList(memGroupId, MEM_NAME(memGroupId, "StatsView::mEntityIDList")),
    mIsCreating(true),
    mDoneFlag(false),
    mViewId(0),
    mMemGroup(memGroupId)
{
    mAPI = api;
    mGroup = group;
    mDataPtr = nullptr;
    
    mPeriodType = STAT_PERIOD_ALL_TIME;
    mPeriodOffset = 0;
    mTime = 0;
}


StatsView::~StatsView()
{
    mGroup->unregisterView(this);

    // clear memory allocated in th keyscopemap
    releaseKeyScopeMap();

    // Remove any outstanding txns.  Canceling here can be dangerous here as it will still attempt
    // to call the callback.  Since the object is being deleted, we go with the remove.
    mAPI->getBlazeHub()->getScheduler()->removeByAssociatedObject(reinterpret_cast<void*>(this));
}

void StatsView::release()
{
    BLAZE_DELETE_PRIVATE(mMemGroup, StatsView, this);
}


const EntityId* StatsView::getEntityIDs(size_t *outCount) const
{
    *outCount = mEntityIDList.size();
    return mEntityIDList.data();
}

const Stats::StatValues* StatsView::getStatValues() const
{
    ScopeString mapKey(SCOPE_NAME_NONE);
    KeyScopeStatsValueMap::const_iterator it = mKeyScopeStatsValueMap.find(mapKey);
    if (it != mKeyScopeStatsValueMap.end())
    {
        return it->second;
    }

    return nullptr;
}

const Stats::StatValues* StatsView::getStatValues(ScopeValue scopeValue) const
{
    // validate the scopeName and value
    char8_t mapKey[STATS_SCOPESTRING_LENGTH] = {0};
    const char8_t* scopeName(nullptr);
    const ScopeNameValueMap* scopeNameValueMap = mGroup->getScopeNameValueMap();
    if (scopeNameValueMap)
    {
        ScopeNameValueMap::const_iterator scopeIt = scopeNameValueMap->begin();
        BlazeAssertMsg((scopeNameValueMap->size() == 1), "Used only when 1 scope setting for the group, should be only one entry in the scope unit map.");
        scopeName = scopeIt->first.c_str();
        if (genStatValueMapKeyForUnit(scopeName, scopeValue, mapKey, sizeof(mapKey)))
        {
            KeyScopeStatsValueMap::const_iterator it = mKeyScopeStatsValueMap.find(mapKey);
            if (it != mKeyScopeStatsValueMap.end())
            {
                return it->second;
            }
        }
    }
    else
    {
        BlazeAssertMsg(false, "Should be Used only when 1 scope setting for the group, however there is no scope setting found for the group.");
        return nullptr;
    }

    BLAZE_SDK_DEBUGF("[StatsAPI] Warning: No stats retrieved for keyscope (%s) value (%" PRId64 ")\n",
        scopeName == nullptr ? "" : scopeName, scopeValue);
    // client can verify that the keyscope value is valid by checking the value against keyscope map (getKeyScopesMap)
    return nullptr;
}

const Stats::StatValues* StatsView::getStatValues(const char8_t* scopeName1, ScopeValue scopeValue1,
                                                  const char8_t* scopeName2, ScopeValue scopeValue2) const
{
    // validate the scopeName and value
    char8_t mapKey[STATS_SCOPESTRING_LENGTH] = {0};

    const ScopeNameValueMap* scopeNameValueMap = mGroup->getScopeNameValueMap();
    if (scopeNameValueMap)
    {
        BlazeAssertMsg((scopeNameValueMap->size() == 2), "Used only when 2 scope setting for the group.");

        ScopeNameValueMap nameValueMap(MEM_GROUP_FRAMEWORK_TEMP, "StatsView::nameValueMap");
        nameValueMap.insert(eastl::make_pair(scopeName1, scopeValue1));
        nameValueMap.insert(eastl::make_pair(scopeName2, scopeValue2));

        if (genStatValueMapKeyForUnitMap(nameValueMap, mapKey, sizeof(mapKey)))
        {
            KeyScopeStatsValueMap::const_iterator it = mKeyScopeStatsValueMap.find(mapKey);
            if (it != mKeyScopeStatsValueMap.end())
            {
                return it->second;
            }
        }
    }
    else
    {
        BlazeAssertMsg(false, "Should be Used only when 2 scope setting for the group, however there is no scope setting found for the group.");
        return nullptr;
    }

    BLAZE_SDK_DEBUGF("[StatsAPI] Warning: No stats retrieved for keyscope (%s,%s) values (%" PRId64 ", %" PRId64 ")\n",
        scopeName1 == nullptr ? "" : scopeName1, scopeName2 == nullptr ? "" : scopeName2,
        scopeValue1, scopeValue2);
    // client can verify that the keyscope values are valid by checking the values against keyscope map (getKeyScopesMap)
    return nullptr;
}

const Stats::StatValues* StatsView::getStatValues(const ScopeNameValueMap* scopeNameValueMap) const 
{
    char8_t mapKey[STATS_SCOPESTRING_LENGTH] = {0};
    if (scopeNameValueMap != nullptr && genStatValueMapKeyForUnitMap(*scopeNameValueMap, mapKey, sizeof(mapKey)))
    {
        KeyScopeStatsValueMap::const_iterator it = mKeyScopeStatsValueMap.find(mapKey);
        if (it != mKeyScopeStatsValueMap.end())
        {
            return it->second;
        }
    }

    BLAZE_SDK_DEBUGF("[StatsAPI] Warning: No stats retrieved for keyscope values\n");
    // client can verify that the keyscope values are valid by checking the values against keyscope map (getKeyScopesMap)
    return nullptr;
}

JobId StatsView::refresh(GetStatsResultCb& callback,
                         const EntityId *entityIDs, 
                         size_t entityIDCount, 
                         StatPeriodType periodType,
                         int32_t periodOffset, 
                         const ScopeNameValueMap* scopeNameValueMap/*= nullptr*/,
                         int32_t time /* = 0 */,
                         int32_t periodCtr /* = 1 */
                        )
{
    //    EASTL vector implementation - more optimal way to resize (if necessary) and 
    //    copy the new entity array into the view's entity container.
    if (entityIDs == nullptr || entityIDCount == 0)
        mEntityIDList.resize(0);
    else
    {
        mEntityIDList.resize((EntityIDList::size_type)entityIDCount);
        EntityIDList::pointer data = mEntityIDList.data();
        memcpy(data, entityIDs, sizeof(EntityId) * mEntityIDList.size());
    }

    mPeriodType = periodType;
    mPeriodOffset = periodOffset;
    mTime = time;
    mPeriodCtr = periodCtr;

    return refresh(callback, scopeNameValueMap);
}


void StatsView::copyStatValues(const KeyScopedStatValues* keyScopedStatValues)
{
    Stats::StatValues* statValues = nullptr;
    KeyScopeStatsValueMap::iterator it = mKeyScopeStatsValueMap.find(keyScopedStatValues->getKeyString());
    // if it is already there some error has happened just ignore
    if (it == mKeyScopeStatsValueMap.end())
    {
        statValues = mKeyScopeStatsValueMap.allocate_element();
        keyScopedStatValues->getStatValues().copyInto(*statValues);
        mKeyScopeStatsValueMap.insert(eastl::make_pair(keyScopedStatValues->getKeyString(), statValues));
    }
}        

JobId StatsView::refresh(GetStatsResultCb& callback, const ScopeNameValueMap* scopeNameValueMap)
{
    Stats::StatsComponent *statsComp = 
        mAPI->getBlazeHub()->getComponentManager()->getStatsComponent();

    Stats::GetStatsByGroupRequest req;

    //    fill entity id list
    Stats::GetStatsByGroupRequest::EntityIdList &idList = req.getEntityIds();        
    idList.clear();
    size_t numIds = 0;
    const EntityId* entityIds = this->getEntityIDs(&numIds);
    for (size_t idx = 0; idx < numIds; ++idx)
        idList.push_back(entityIds[idx]);

    req.setGroupName(getStatsGroup()->getName());
    req.setPeriodOffset(getPeriodOffset());
    req.setTime(mTime);
    req.setPeriodCtr(mPeriodCtr);
    req.setPeriodType(getPeriodType());
    req.setViewId(mViewId);

    size_t size(0);
    if (scopeNameValueMap)
    {
        size = scopeNameValueMap->size();
    }

    const Stats::KeyScopesMap* keyScopeMap = mAPI->getKeyScopesMap();
    const ScopeNameValueMap* groupScopeNameValueMap = mGroup->getScopeNameValueMap();

    if (!keyScopeMap)
    {
        if (!groupScopeNameValueMap || !groupScopeNameValueMap->empty())
        {
            // there is scope setting in group but the keyscope map is invalid, which means
            // the keyscope map is not retrieved from server

            BlazeAssertMsg(false,  "Please request the keyscope map for stats first!");
            JobScheduler *jobScheduler = mAPI->getBlazeHub()->getScheduler();
            JobId jobId = jobScheduler->reserveJobId();
            jobId = jobScheduler->scheduleFunctor("StatsViewRefreshCb", callback, Blaze::STATS_ERR_BAD_SCOPE_INFO, jobId, this,
                    this, 0, jobId); // associatedObj, delayMs, reservedJobId
            Job::addTitleCbAssociatedObject(jobScheduler, jobId, callback);
            return jobId;
        }
    }

    bool noScope(true);
    size_t sizeQuestion(0);
    if (keyScopeMap)
    {
        // if there is a keyscope setting in config file
        if (groupScopeNameValueMap)
        {
            // if there is a keyscope setting for the group
            ScopeNameValueMap::const_iterator it = groupScopeNameValueMap->begin();
            ScopeNameValueMap::const_iterator itend = groupScopeNameValueMap->end();
            for (; it != itend; ++it)
            {
                // if it is a question mark defined for the group
                if (it->second == KEY_SCOPE_VALUE_USER)
                {
                    sizeQuestion++;

                    // if yes, check if the scope name is provided in scopeNameValueMap
                    bool validName(false);
                    ScopeValue scopeValue = KEY_SCOPE_VALUE_USER;
                    if (scopeNameValueMap != nullptr)
                    {
                        ScopeNameValueMap::const_iterator userItr = scopeNameValueMap->find(it->first);
                        if (userItr != scopeNameValueMap->end())
                        {
                            scopeValue = userItr->second;
                            validName = true;
                        }
                    }

                    Stats::KeyScopesMap::const_iterator findScope = keyScopeMap->find(it->first);
                    if (findScope == keyScopeMap->end())
                    {
                        BlazeAssertMsg(false,  "keyScope setting or group scope setting is wrong!");
                        JobScheduler *jobScheduler = mAPI->getBlazeHub()->getScheduler();
                        JobId jobId = jobScheduler->reserveJobId();
                        jobId = jobScheduler->scheduleFunctor("StatsViewRefreshCb", callback, Blaze::STATS_ERR_BAD_SCOPE_INFO, jobId, this,
                                this, 0, jobId); // associatedObj, delayMs, reservedJobId
                        Job::addTitleCbAssociatedObject(jobScheduler, jobId, callback);
                        return jobId;
                    }

                    // if the scope name with ? found in scopeNameValueMap
                    bool validValue(false);
                    if (validName)
                    {
                        //first try "total" which is applicable to both string and numeric scopes
                        if (scopeValue == Stats::KEY_SCOPE_VALUE_ALL)
                        {
                            validValue = true;
                        }
                        else
                        {
                            validValue = isValidScopeValue(findScope->second, scopeValue);
                        }
                    }

                    if (validName && validValue)
                    {
                        req.getKeyScopeNameValueMap().insert(eastl::make_pair(it->first, scopeValue));
                    }
                    else
                    {
                        BlazeAssertMsg(false,  "either scope name or scope value for the name passed in is not valid!");

                        JobScheduler *jobScheduler = mAPI->getBlazeHub()->getScheduler();
                        JobId jobId = jobScheduler->reserveJobId();
                        jobId = jobScheduler->scheduleFunctor("StatsViewRefreshCb", callback, Blaze::STATS_ERR_BAD_SCOPE_INFO, jobId, this,
                                this, 0, jobId); // associatedObj, delayMs, reservedJobId
                        Job::addTitleCbAssociatedObject(jobScheduler, jobId, callback);
                        return jobId;
                    }
                }
            }//for (; it != itend; ++it)

            // if scope information client set is not consistent with the settings in config
            if (size != sizeQuestion)
            {
                BlazeAssertMsg(false,  "number of scope pair provide in scopeNameValueMap does not match group requirement!");

                JobScheduler *jobScheduler = mAPI->getBlazeHub()->getScheduler();
                JobId jobId = jobScheduler->reserveJobId();
                jobId = jobScheduler->scheduleFunctor("StatsViewRefreshCb", callback, Blaze::STATS_ERR_BAD_SCOPE_INFO, jobId, this,
                        this, 0, jobId); // associatedObj, delayMs, reservedJobId
                Job::addTitleCbAssociatedObject(jobScheduler, jobId, callback);
                return jobId;
            }

            noScope = false;
        }
    }
    
    if (size > 0 && (noScope))
    {
        // if group does not have scope setting but client set it
        BlazeAssertMsg(false,  "no scope is defined for the group, should not have entry in scopeNameValueMap!");

        JobScheduler *jobScheduler = mAPI->getBlazeHub()->getScheduler();
        JobId jobId = jobScheduler->reserveJobId();
        jobId = jobScheduler->scheduleFunctor("StatsViewRefreshCb", callback, Blaze::STATS_ERR_BAD_SCOPE_INFO, jobId, this,
                this, 0, jobId); // associatedObj, delayMs, reservedJobId
        Job::addTitleCbAssociatedObject(jobScheduler, jobId, callback);
        return jobId;
    }

    releaseKeyScopeMap();
    
    mDoneFlag = false;

    StatsViewJob *statsViewJob = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "StatsViewJob") StatsViewJob(mAPI, callback, this);
    mJobId = mAPI->getBlazeHub()->getScheduler()->scheduleJobNoTimeout("StatsViewJob", statsViewJob, this);

    statsComp->getStatsByGroupAsync(req, MakeFunctor(this, &StatsView::getStatsByGroupAsyncCb), mJobId);

    return mJobId;
}

bool StatsView::isValidScopeValue(const KeyScopeItem* scopeItem, ScopeValue scopeValue) const
{
    if (scopeValue < 0)
    {
        // only non-negative values allowed
        return false;
    }

    if (scopeItem->getKeyScopeValues().empty())
    {
        // no explicit keyscope values, so all are valid
        return true;
    }

    if (scopeItem->getEnableAggregation() && (scopeItem->getAggregateKeyValue() == scopeValue))
    {
        return true;
    }

    ScopeStartEndValuesMap::const_iterator valuesItr = scopeItem->getKeyScopeValues().begin();
    ScopeStartEndValuesMap::const_iterator valuesEnd = scopeItem->getKeyScopeValues().end();
    for (; valuesItr != valuesEnd; ++valuesItr)
    {
        if (scopeValue > valuesItr->second)
        {
            // not past any potential match
            continue;
        }
        // else will match...
        if (scopeValue >= valuesItr->first)
        {
            return true;
        }
        // or have gone past...
        break;
    }

    return false;
}

void StatsView::getStatsByGroupAsyncCb(BlazeError err, JobId rpcJobId, JobId jobId)
{
    StatsViewJob* statsViewJob = (StatsViewJob*)mAPI->getBlazeHub()->getScheduler()->getJob(jobId);

    // If the stats view job is already gone, nothing else to do, and if it is currently executing,
    // then presuambly it is in the process of being cancelled in which case we also want to early out,
    // even though the internals of the cancel method itself will prevent against nested execution,
    // it would not prevent the double removal/free of the job itself
    if (statsViewJob == nullptr || statsViewJob->isExecuting())
    {
        mDoneFlag = true;
        return;
    }

    // if error no notifications are expected, call client now
    if (err != ERR_OK)
    {
        mDoneFlag = true;

        // Either cancel itself or the title callback executed from within the cancel may free this view,
        // hence we remove the job first and take ownership of it here, and take care of cleaning up the memory ourselves
        mAPI->getBlazeHub()->getScheduler()->removeJob(statsViewJob, false);
        statsViewJob->cancel(err);
        BLAZE_DELETE(MEM_GROUP_FRAMEWORK_TEMP, statsViewJob);
    }
} /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

void StatsView::releaseKeyScopeMap()
{
    mKeyScopeStatsValueMap.release();
}

} // namespace Stats
} // namespace Blaze
