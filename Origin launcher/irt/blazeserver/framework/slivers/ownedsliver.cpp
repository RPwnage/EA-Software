/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/slivers/ownedsliver.h"
#include "framework/slivers/sliverowner.h"
#include "framework/slivers/slivermanager.h"
#include "framework/uniqueid/uniqueidmanager.h"

namespace Blaze
{

OwnedSliver::OwnedSliver(SliverOwner& sliverOwner, Sliver& sliver) :
    mSliverOwner(sliverOwner),
    mSliver(sliver),
    mLoadCounter(0),
    mPriorityQueue(nullptr)
{
}

OwnedSliver::~OwnedSliver()
{
}

bool OwnedSliver::getPrioritySliverAccess(Sliver::AccessRef& access)
{
    if (mSliver.accessWouldBlock())
    {
        BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "OwnedSliver.getPrioritySliverAccess: SliverId(" << mSliver.getSliverId() << ") is currently write-locked (migrating), "
            "but the SliverOwner(" << BlazeRpcComponentDb::getComponentNameById(mSliver.getSliverNamespace()) << ") is requesting priority read-access.  Fiber context: " << Fiber::getCurrentContext());
    }

    return mSliver.getPriorityAccess(access);
}

void OwnedSliver::incLoad()
{
    ++mLoadCounter;
    removeFromPriorityQueue();
    addToPriorityQueue();
}

void OwnedSliver::decLoad()
{
    EA_ASSERT(mLoadCounter > 0);
    --mLoadCounter;
    removeFromPriorityQueue();
    addToPriorityQueue();
}

void OwnedSliver::setPriorityQueue(OwnedSliverByPriorityMap* priorityQueue)
{
    removeFromPriorityQueue();
    mPriorityQueue = priorityQueue;
    addToPriorityQueue();
}

void OwnedSliver::addToPriorityQueue()
{
    if (mPriorityQueue != nullptr)
    {
        EA_ASSERT(mPriorityQueueIterator == OwnedSliverByPriorityMap::iterator());
        mPriorityQueueIterator = mPriorityQueue->insert(eastl::make_pair(mLoadCounter, this));
    }
}

void OwnedSliver::removeFromPriorityQueue()
{
    if (mPriorityQueue != nullptr)
    {
        EA_ASSERT(mPriorityQueueIterator != OwnedSliverByPriorityMap::iterator());
        mPriorityQueue->erase(mPriorityQueueIterator);
        mPriorityQueueIterator = OwnedSliverByPriorityMap::iterator();
    }
}

} // namespace Blaze

