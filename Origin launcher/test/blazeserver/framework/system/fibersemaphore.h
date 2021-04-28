/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*
    IMPORTANT: FiberSemaphore will be deprecated. Use FiberEvents or Fiber::join instead.
*/

#ifndef BLAZE_FIBERSEMAPHORE_H
#define BLAZE_FIBERSEMAPHORE_H

/*** Include files *******************************************************************************/
#include "EASTL/list.h"
#include "EASTL/intrusive_list.h"
#include "EASTL/intrusive_hash_map.h"
#include "EASTL/string.h"
#include "framework/system/fiber.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class FiberManager;
typedef uint64_t SemaphoreId;


struct SemaphoreListNode : public eastl::intrusive_list_node {};
struct SemaphoresByIdMapNode : public eastl::intrusive_hash_node {};

class FiberSemaphore : public SemaphoreListNode,
                       public SemaphoresByIdMapNode
{
    NON_COPYABLE(FiberSemaphore)

public:
    SemaphoreId getId() const { return mId; }

private:
    friend class FiberManager;

    FiberSemaphore();
    ~FiberSemaphore();

    void initialize(const char8_t* name, SemaphoreId id, int32_t threshold = 1, int32_t count = 0);
    void clear();

    BlazeRpcError wait(bool* releaseWhenDone);
    void signal(BlazeRpcError reason);
    Fiber::Priority getNextWaitingFiberPriority();

    typedef eastl::list<eastl::pair<const Fiber::EventHandle, Fiber::Priority> > EventList;
    EventList mWaitingEvents;
    int32_t mCount;
    int32_t mThreshold;
    eastl::string mName;
    SemaphoreId mId;
    BlazeRpcError mSignalError;
};

} // Blaze

namespace eastl
{
    template <>
    struct use_intrusive_key<Blaze::SemaphoresByIdMapNode, Blaze::SemaphoreId>
    {
        Blaze::SemaphoreId operator() (const Blaze::SemaphoresByIdMapNode& x) const
        {
            return static_cast<const Blaze::FiberSemaphore &>(x).getId();
        }
    };
}

#endif // BLAZE_FIBERSEMAPHORE_H

