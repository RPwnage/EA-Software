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
#include "BlazeSDK/job.h"


namespace Blaze
{
    namespace Stats
    {

///////////////////////////////////////////////////////////////////////////////
//
//    class LeaderboardFolder
//
///////////////////////////////////////////////////////////////////////////////

LeaderboardFolder::LeaderboardFolder(LeaderboardAPI *api, const Stats::LeaderboardFolderGroup *folderData, MemoryGroupId memGroupId) :
    mFolderData(memGroupId),
    mFolderList(memGroupId, MEM_NAME(memGroupId, "LeaderBoardFolder::mFolderList")),
    mLeaderboardList(memGroupId, MEM_NAME(memGroupId, "LeaderBoardFolder::mLeaderboardList")),
    mJobId(INVALID_JOB_ID)
{
    mLBAPI = api;
    folderData->copyInto(mFolderData);

    mHasChildFolders = false;
    mHasChildLeaderboards = false;

    //    baed on the child folder descriptors, determine child node content
    Stats::LeaderboardFolderGroup::FolderDescriptorList::iterator it = mFolderData.getFolderDescriptors().begin();
    Stats::LeaderboardFolderGroup::FolderDescriptorList::iterator itEnd = mFolderData.getFolderDescriptors().end();
    while ((it != itEnd) && (!mHasChildFolders || !mHasChildLeaderboards))
    {
        const Stats::FolderDescriptor *folderDesc = *it;
        if ((folderDesc->getFolderId() & Stats::FolderDescriptor::IS_LEADERBOARD) != 0)
            mHasChildLeaderboards = true;
        else
            mHasChildFolders = true;
        ++it;
    }

}


LeaderboardFolder::~LeaderboardFolder()
{
    // Remove any outstanding txns.  Canceling here can be dangerous here as it will still attempt
    // to call the callback.  Since the object is being deleted, we go with the remove.
    mLBAPI->getBlazeHub()->getScheduler()->removeByAssociatedObject(reinterpret_cast<void*>(this));
}


JobId LeaderboardFolder::requestChildLeaderboardFolderList(GetLeaderboardFolderListCb& cb)
{
    if (mFolderListCb.isValid())
    {
        JobScheduler *jobScheduler = mLBAPI->getBlazeHub()->getScheduler();
        JobId jobId = jobScheduler->reserveJobId();
        jobId = jobScheduler->scheduleFunctor("requestChildLeaderboardFolderListCb", cb, SDK_ERR_IN_PROGRESS, jobId, (const LeaderboardFolderList*) nullptr,
            this, 0, jobId); // assocObj, delayMs, reservedJobId
        Job::addTitleCbAssociatedObject(mLBAPI->getBlazeHub()->getScheduler(), jobId, cb);
        return jobId;
    }

    //    trap request if there are no children of this type.
    if (!hasChildFolders())
    {
        BlazeVerify(mFolderList.empty());

        JobScheduler *jobScheduler = mLBAPI->getBlazeHub()->getScheduler();
        JobId jobId = jobScheduler->reserveJobId();
        jobId = jobScheduler->scheduleFunctor("requestChildLeaderboardFolderListCb", cb, ERR_OK, jobId, const_cast<const LeaderboardFolderList *>(&mFolderList),
                this, 0, jobId); // assocObj, delayMs, reservedJobId
        Job::addTitleCbAssociatedObject(mLBAPI->getBlazeHub()->getScheduler(), jobId, cb);
        return jobId;
    }

    //    assume current list represents whether we've loaded folder data or not (since no callback is active.)
    if (!mFolderList.empty())
    {
        JobScheduler *jobScheduler = mLBAPI->getBlazeHub()->getScheduler();
        JobId jobId = jobScheduler->reserveJobId();
        jobId = jobScheduler->scheduleFunctor("requestChildLeaderboardFolderListCb", cb, ERR_OK, jobId, const_cast<const LeaderboardFolderList *>(&mFolderList),
            this, 0, jobId); // assocObj, delayMs, reservedJobId
        Job::addTitleCbAssociatedObject(mLBAPI->getBlazeHub()->getScheduler(), jobId, cb);
        return jobId;
    }

    //    start folder list retrieval.
    mFolderListCb = cb;

    //TODO Bug #522 - This result id is actually incorrect -if we need to chain things together
    //we should really reserve and id and return it.  However, rather than fix this tangle
    //now we'll defer until we have a bulk leaderboard fetch transaction.
    //    build list from existing objects.  if there's no folder loaded in, retrieve it and have the callback perform
    //    this same algorithm using
    mBuildFolderListIt  = mFolderData.getFolderDescriptors().begin();
    JobId jobId = fillFolderListAtCurrentIt();
    if (jobId != INVALID_JOB_ID)
    {
        return jobId;
    }
    else
    {
    //    Compiled a folder list with objects already loaded in memory.
        JobScheduler *jobScheduler = mLBAPI->getBlazeHub()->getScheduler();
        jobId = jobScheduler->reserveJobId();
        jobId = jobScheduler->scheduleFunctor("requestChildLeaderboardFolderListCb", cb, ERR_OK, jobId, const_cast<const LeaderboardFolderList *>(&mFolderList),
            this, 0, jobId);
        Job::addTitleCbAssociatedObject(mLBAPI->getBlazeHub()->getScheduler(), jobId, cb);
        return jobId;
    }
}


JobId LeaderboardFolder::fillFolderListAtCurrentIt()
{
    Stats::LeaderboardFolderGroup::FolderDescriptorList::iterator itEnd = mFolderData.getFolderDescriptors().end();

    for (; mBuildFolderListIt != itEnd; ++mBuildFolderListIt)
    {
        uint32_t folderId = (*mBuildFolderListIt)->getFolderId();
        if ((folderId & Stats::FolderDescriptor::IS_LEADERBOARD) == 0)
        {
            LeaderboardFolder *folder = mLBAPI->findLeaderboardFolder(static_cast<int32_t>(folderId));
            if (folder == nullptr)
            {
                ++mBuildFolderListIt;
                GetLeaderboardFolderCb cb(this, &LeaderboardFolder::getLeaderboardFolderCb);
                return mLBAPI->retrieveLeaderboardFolder(cb, static_cast<int32_t>(folderId), nullptr);
            }
            else
            {
                mFolderList.push_back(folder);
            }
        }
    }

    return INVALID_JOB_ID;
}


//    adds a folder object returned from server and continues populating the folder list if necessary.
void LeaderboardFolder::getLeaderboardFolderCb(BlazeError err, JobId id, LeaderboardFolder* folder)
{
    if (err != ERR_OK)
    {
        //    TODO: Don't fail retrieval of folder list if one folder errors (while other rpcs succeed?)
        mFolderListCb(err, id, nullptr);
        mFolderListCb.clear();
        return;
    }

    mFolderList.push_back(folder);

    //    If done, issue user supplied callback.  Otherwise issues a request on the unloaded folder.
    if (!fillFolderListAtCurrentIt().isWaitSet())
    {
        GetLeaderboardFolderListCb cb = mFolderListCb;
        mFolderListCb.clear();
        cb(ERR_OK, id, const_cast<LeaderboardFolderList*>(&mFolderList));
    }
} /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/


JobId LeaderboardFolder::requestChildLeaderboardList(GetLeaderboardListCb& cb)
{
    if (mLeaderboardListCb.isValid())
    {
        JobScheduler *jobScheduler = mLBAPI->getBlazeHub()->getScheduler();
        JobId jobId = jobScheduler->reserveJobId();
        jobId = jobScheduler->scheduleFunctor("requestChildLeaderboardListCb", cb, SDK_ERR_IN_PROGRESS, jobId, (const LeaderboardList *) nullptr,
                this, 0, jobId); // assocObj, delayMs, reservedJobId
        Job::addTitleCbAssociatedObject(mLBAPI->getBlazeHub()->getScheduler(), jobId, cb);
        return jobId;
    }

    //    trap request if there are no children of this type.
    if (!hasChildLeaderboards())
    {
        BlazeVerify(mLeaderboardList.empty());
        JobScheduler *jobScheduler = mLBAPI->getBlazeHub()->getScheduler();
        JobId jobId = jobScheduler->reserveJobId();
        jobId = jobScheduler->scheduleFunctor("requestChildLeaderboardListCb", cb, ERR_OK, jobId, const_cast<const LeaderboardList *>(&mLeaderboardList),
                this, 0, jobId); // assocObj, delayMs, reservedJobId
        Job::addTitleCbAssociatedObject(mLBAPI->getBlazeHub()->getScheduler(), jobId, cb);
        return jobId;

    }

    //    assume current list represents whether we've loaded folder data or not (since no callback is active.)
    if (!mLeaderboardList.empty())
    {
        JobScheduler *jobScheduler = mLBAPI->getBlazeHub()->getScheduler();
        JobId jobId = jobScheduler->reserveJobId();
        jobId = jobScheduler->scheduleFunctor("requestChildLeaderboardListCb", cb, ERR_OK, jobId, const_cast<const LeaderboardList *>(&mLeaderboardList),
                this, 0, jobId); // assocObj, delayMs, reservedJobId
        Job::addTitleCbAssociatedObject(mLBAPI->getBlazeHub()->getScheduler(), jobId, cb);
        return jobId;
    }

    //    start leaderboard list retrieval.
    mLeaderboardListCb = cb;

    //    build list from existing objects.  if there's no leaderboard loaded in, retrieve it and have the callback perform
    //    this same algorithm using
    mBuildLeaderboardListIt = mFolderData.getFolderDescriptors().begin();
    mJobId = fillLeaderboardListAtCurrentIt();
    if (mJobId != INVALID_JOB_ID)
    {
        return mJobId;
    }
    else
    {
    //    Compiled a leaderboard list with objects already loaded in memory.
        JobScheduler *jobScheduler = mLBAPI->getBlazeHub()->getScheduler();
        JobId jobId = jobScheduler->reserveJobId();
        jobId = jobScheduler->scheduleFunctor("requestChildLeaderboardListCb", cb, ERR_OK, jobId, const_cast<const LeaderboardList *>(&mLeaderboardList),
            this, 0, jobId); // assocObj, delayMs, reservedJobId
        Job::addTitleCbAssociatedObject(mLBAPI->getBlazeHub()->getScheduler(), jobId, cb);
        return jobId;
    }
}


JobId LeaderboardFolder::fillLeaderboardListAtCurrentIt()
{
    Stats::LeaderboardFolderGroup::FolderDescriptorList::iterator itEnd = mFolderData.getFolderDescriptors().end();

    for (; mBuildLeaderboardListIt != itEnd; ++mBuildLeaderboardListIt)
    {
        uint32_t lbId = (*mBuildLeaderboardListIt)->getFolderId();
        if ((lbId & Stats::FolderDescriptor::IS_LEADERBOARD) != 0)
        {
            Leaderboard *lb = mLBAPI->findLeaderboard((*mBuildLeaderboardListIt)->getName());
            if (lb == nullptr)
            {
                ++mBuildLeaderboardListIt;
                GetLeaderboardCb cb(this, &LeaderboardFolder::getLeaderboardCb);
                return mLBAPI->retrieveLeaderboard(cb, static_cast<int32_t>(lbId), nullptr);
            }
            else
            {
                mLeaderboardList.push_back(lb);
            }
        }
    }

    return INVALID_JOB_ID;
}


//    adds a leaderboard object returned from server and continues populating the folder list if necessary.
void LeaderboardFolder::getLeaderboardCb(BlazeError err, JobId id, Leaderboard* lb)
{
    LeaderboardList *list = nullptr;

    if (err == ERR_OK)
    {
        mLeaderboardList.push_back(lb);

        //    If not done this issues a request on the unloaded folder.
        if (fillLeaderboardListAtCurrentIt().isWaitSet())
        {
            return;
        }

        list = &mLeaderboardList;
    }

    GetLeaderboardListCb cb = mLeaderboardListCb;
    mLeaderboardListCb.clear();
    cb(err, mJobId, const_cast<LeaderboardList*>(list));
} /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/


JobId LeaderboardFolder::requestParentLeaderboardFolder(GetLeaderboardFolderCb& cb)
{
    LeaderboardFolder *folder = mLBAPI->findLeaderboardFolder(static_cast<int32_t>(mFolderData.getParentId()));
    if (folder != nullptr)
    {
        JobScheduler *jobScheduler = mLBAPI->getBlazeHub()->getScheduler();
        JobId jobId = jobScheduler->reserveJobId();
        jobId = jobScheduler->scheduleFunctor("requestParentLeaderboardFolderCb", cb, ERR_OK, jobId, folder, this, 0, jobId);
        Job::addTitleCbAssociatedObject(mLBAPI->getBlazeHub()->getScheduler(), jobId, cb);
        return jobId;
    }

    return mLBAPI->retrieveLeaderboardFolder(cb, static_cast<int32_t>(mFolderData.getParentId()), nullptr);
}

    }   // namespace Stats
}
// namespace Blaze
