/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_OWNEDSLIVER_H
#define BLAZE_OWNEDSLIVER_H

/*** Include files *******************************************************************************/

#include "framework/system/fiberreadwritemutex.h"
#include "framework/slivers/sliver.h"
#include "framework/tdf/slivermanagertypes_server.h"

#include "EASTL/map.h"
#include "EASTL/intrusive_ptr.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class SliverOwner;

class OwnedSliver;
typedef eastl::intrusive_ptr<OwnedSliver> OwnedSliverRef;
typedef eastl::multimap<uint32_t, OwnedSliver*> OwnedSliverByPriorityMap;

class OwnedSliver
{
    NON_COPYABLE(OwnedSliver);
public:
    SliverId getSliverId() const { return mSliver.getSliverId(); }

    uint32_t getLoad() const { return mLoadCounter; }
    void incLoad();
    void decLoad();

    bool isInPriorityQueue() const { return (mPriorityQueue != nullptr); }

    bool getPrioritySliverAccess(Sliver::AccessRef& access);

private:
    friend class SliverManager;
    friend class SliverOwner;
    friend void intrusive_ptr_add_ref(OwnedSliver* ptr);
    friend void intrusive_ptr_release(OwnedSliver* ptr);

    OwnedSliver(SliverOwner& sliverOwner, Sliver& sliver);
    virtual ~OwnedSliver();

    void setPriorityQueue(OwnedSliverByPriorityMap* priorityQueue);
    void addToPriorityQueue();
    void removeFromPriorityQueue();

    Sliver& getBaseSliver() { return mSliver; }

    Fiber::WaitList& getExportCompletedWaitList() { return mExportCompletedWaitList; }

private:
    SliverOwner& mSliverOwner;
    Sliver& mSliver;

    uint32_t mLoadCounter;

    OwnedSliverByPriorityMap* mPriorityQueue;
    OwnedSliverByPriorityMap::iterator mPriorityQueueIterator;

    Fiber::WaitList mExportCompletedWaitList;
};

inline void intrusive_ptr_add_ref(OwnedSliver* ptr) { ptr->incLoad(); }
inline void intrusive_ptr_release(OwnedSliver* ptr) { ptr->decLoad(); }

} // Blaze

#endif // BLAZE_OWNEDSLIVER_H
