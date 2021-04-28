#include "DownloadPriorityQueue.h"

namespace Origin
{
namespace Downloader
{

DownloadPriorityQueue::DownloadPriorityQueueIterator::DownloadPriorityQueueIterator(DownloadPriorityQueue& queue) :
    mQueue(queue),
    mIsValid(false)
{
    // Grab the first priority group and make sure it's valid
    mPriorityGroupsIter = mQueue.mPriorityGroups.begin();
    while (mPriorityGroupsIter != mQueue.mPriorityGroups.end())
    {
        // Skip non-enabled groups
        if (!(*mPriorityGroupsIter).mEnabled)
        {
            ++mPriorityGroupsIter;
            continue;
        }

        // Grab the first request in the first group
        mSingleGroupIter = (*mPriorityGroupsIter).mRequests.begin();
        if (mSingleGroupIter != (*mPriorityGroupsIter).mRequests.end())
        {
            // Track that we've already iterated over this item
            mAlreadyIterated.insert(*mSingleGroupIter);

            // Mark the iterator valid
            mIsValid = true;

            break;
        }
    }
}

// Bare minimum set of iterator methods
bool DownloadPriorityQueue::DownloadPriorityQueueIterator::valid() const
{
    return mIsValid;
}

RequestId DownloadPriorityQueue::DownloadPriorityQueueIterator::current() const
{
    if (!mIsValid)
        return -1;

    return *mSingleGroupIter;
}

int DownloadPriorityQueue::DownloadPriorityQueueIterator::currentGroupId() const
{
    if (!mIsValid)
        return -1;

    return (*mPriorityGroupsIter).mPriorityGroupId;
}

void DownloadPriorityQueue::DownloadPriorityQueueIterator::next()
{
    // We can only iterate a valid iterator
    if (!mIsValid)
        return;

    // Move the inner iterator.  If it's still valid, we're done
    if (inner_next())
        return;

    // We need to go to the next priority group
    ++mPriorityGroupsIter;

    // Go until we find a valid and enabled group
    while (mPriorityGroupsIter != mQueue.mPriorityGroups.end())
    {
        // Skip non-enabled groups
        if (!(*mPriorityGroupsIter).mEnabled)
        {
            ++mPriorityGroupsIter;
            continue;
        }

        break;
    }

    // If we're not at the end of the priority groups yet
    while (mPriorityGroupsIter != mQueue.mPriorityGroups.end())
    {
        // Grab the first request in the new priority group
        mSingleGroupIter = (*mPriorityGroupsIter).mRequests.begin();

        // If we haven't gone past the end of this group yet, we've found the next item
        if (mSingleGroupIter != (*mPriorityGroupsIter).mRequests.end())
        {
            // Track that we've already iterated over this item
            if (!mAlreadyIterated.contains(*mSingleGroupIter))
            {
                mAlreadyIterated.insert(*mSingleGroupIter);

                return;
            }
            else
            {
                // We'd already seen the last item, so move the inner iterator until we find the next
                if (inner_next())
                    return;
            }
        }

        // We need to go to the next priority group
        ++mPriorityGroupsIter;
    }

    // We didn't find an item
    mIsValid = false;
}

bool DownloadPriorityQueue::DownloadPriorityQueueIterator::inner_next()
{
    // Look for our next item; If we haven't gone past the end of this group yet,
    // and we haven't seen it yet, we've found the next item
    while (++mSingleGroupIter != (*mPriorityGroupsIter).mRequests.end())
    {
        // Track that we've already iterated over this item
        if (!mAlreadyIterated.contains(*mSingleGroupIter))
        {
            mAlreadyIterated.insert(*mSingleGroupIter);

            return true;
        }
    }
    return false;
}

DownloadPriorityQueue::DownloadPriorityQueueIterator DownloadPriorityQueue::constIterator()
{
    return DownloadPriorityQueue::DownloadPriorityQueueIterator(*this);
}

void DownloadPriorityQueue::Insert(QMap<int, QList<RequestId> > requestsByGroup)
{
    // Go through all the groups
    for (QLinkedList<DownloadPriorityGroup>::iterator it = mPriorityGroups.begin(); it != mPriorityGroups.end(); ++it)
    {
        // If we have requests for this group id
        int groupId = (*it).mPriorityGroupId;
        if (requestsByGroup.contains(groupId))
        {
            // Add them to the list
            (*it).mRequests.append(requestsByGroup[groupId]);

            // Remove it from the input queue
            requestsByGroup.remove(groupId);
        }
    }

    // Create priority group items for the remainder
    for (QMap<int, QList<RequestId> >::iterator it = requestsByGroup.begin(); it != requestsByGroup.end(); ++it)
    {
        int groupId = it.key();
        QList<RequestId> requests = it.value();

        // Create a new group
        DownloadPriorityGroup priorityGroup;
        priorityGroup.mPriorityGroupId = groupId;
        priorityGroup.mRequests.append(requests);

        mPriorityGroups.append(priorityGroup);
    }
}

void DownloadPriorityQueue::Insert(RequestId id, int groupId, bool enabled)
{
    // Go through all the groups
    for (QLinkedList<DownloadPriorityGroup>::iterator it = mPriorityGroups.begin(); it != mPriorityGroups.end(); ++it)
    {
        // If we have requests for this group id
        int curGroupId = (*it).mPriorityGroupId;
        if (groupId == curGroupId)
        {
            (*it).mEnabled = enabled;

            // Add it to the list
            (*it).mRequests.append(id);

            return;
        }
    }

    // If we didn't already have a group, create one
    DownloadPriorityGroup priorityGroup;
    priorityGroup.mPriorityGroupId = groupId;
    priorityGroup.mEnabled = enabled;
    priorityGroup.mRequests.append(id);

    mPriorityGroups.append(priorityGroup);
}

void DownloadPriorityQueue::Insert(QVector<RequestId> requests, int groupId, bool enabled)
{
    // Go through all the groups
    for (QLinkedList<DownloadPriorityGroup>::iterator it = mPriorityGroups.begin(); it != mPriorityGroups.end(); ++it)
    {
        // If we have requests for this group id
        int curGroupId = (*it).mPriorityGroupId;
        if (groupId == curGroupId)
        {
            (*it).mEnabled = enabled;

            // Add it to the list
            (*it).mRequests.append(requests.toList());

            return;
        }
    }

    // If we didn't already have a group, create one
    DownloadPriorityGroup priorityGroup;
    priorityGroup.mPriorityGroupId = groupId;
    priorityGroup.mEnabled = enabled;
    priorityGroup.mRequests.append(requests.toList());

    mPriorityGroups.append(priorityGroup);
}

void DownloadPriorityQueue::MoveToTop(int priorityGroupId)
{
    DownloadPriorityGroup groupToMove;

    // Go through all the groups
    bool first = true;
    for (QLinkedList<DownloadPriorityGroup>::iterator it = mPriorityGroups.begin(); it != mPriorityGroups.end(); ++it)
    {
        // If we have requests for this group id
        int groupId = (*it).mPriorityGroupId;

        // If the group is already at the top, make sure it's enabled and do nothing
        if (first && groupId == priorityGroupId)
        {
            (*it).mEnabled = true;
            return;
        }

        first = false;

        if (groupId == priorityGroupId)
        {
            groupToMove = (*it);

            // Mark the new group as enabled
            groupToMove.mEnabled = true;

            // Remove it from the list
            mPriorityGroups.erase(it);

            // Prepend the list
            mPriorityGroups.prepend(groupToMove);
            return;
        }
    }
}

void DownloadPriorityQueue::SetQueueOrder(QVector<int> priorityGroupIds)
{
    // Go through the list in reverse order and successively move-to-top
    for (int idxGroup = priorityGroupIds.count()-1; idxGroup >= 0; --idxGroup)
    {
        int groupId = priorityGroupIds[idxGroup];
        
        MoveToTop(groupId);
    }
}

void DownloadPriorityQueue::SetGroupEnabledStatus(int groupId, bool enabled)
{
    // Go through all the groups
    for (QLinkedList<DownloadPriorityGroup>::iterator it = mPriorityGroups.begin(); it != mPriorityGroups.end(); ++it)
    {
        // Try to match against the group ID
        int curGroupId = (*it).mPriorityGroupId;
        if (groupId == curGroupId)
        {
            // Set the enabled status
            (*it).mEnabled = enabled;

            return;
        }
    }
}

QLinkedList<DownloadPriorityGroup>& DownloadPriorityQueue::GetQueue()
{
    return mPriorityGroups;
}

}// Download
}// Origin
