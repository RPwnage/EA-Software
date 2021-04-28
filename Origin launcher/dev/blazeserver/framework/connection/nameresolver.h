/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_NAMERESOLVER_H
#define BLAZE_NAMERESOLVER_H

/*** Include files *******************************************************************************/

#include "EASTL/deque.h"
#include "EASTL/intrusive_list.h"
#include "EATDF/time.h"
#include "eathread/eathread_storage.h"
#include "eathread/eathread_mutex.h"
#include "eathread/eathread_condition.h"
#include "framework/system/blazethread.h"
#include "framework/system/job.h"
#include "framework/util/lrucache.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class InetAddress;
class Logger;
class Selector;
class ResolverJob;
class Fiber;

using EA::TDF::TimeValue;

class NameResolver : public BlazeThread
{
public:
    NameResolver(const char *baseName);
    ~NameResolver() override;

    typedef uint32_t LookupJobId;
    static const LookupJobId INVALID_JOB_ID = 0xFFFFFFFF;

    void stop() override;

    BlazeRpcError DEFINE_ASYNC_RET(resolve(InetAddress& address, LookupJobId& jobId, const TimeValue& absoluteTimeout = TimeValue()));
    void cancelResolve(LookupJobId jobId);

    static bool blockingResolve(InetAddress& address);
    static bool blockingLookup(InetAddress& addr);
    static bool blockingVerify(const char8_t *hostname, const InetAddress& addr);

    static void setInterfaceAddresses(const InetAddress& internal, const InetAddress& external, bool internalHostnameOverride, bool externalHostnameOverride);
    static const InetAddress& getInternalAddress() { return mInternalAddress; }
    static const InetAddress& getExternalAddress() { return mExternalAddress; }
    static bool isInternalHostnameOverride() { return mInternalHostnameOverride; }
    static bool isExternalHostnameOverride() { return mExternalHostnameOverride; }

private:
   
   
    typedef struct Lookup
    {
        LookupJobId mJobId;
        InetAddress* mAddr;
        Fiber::EventHandle mEventHandle;

        Lookup(LookupJobId jobId, InetAddress& addr, Fiber::EventHandle handle)
            : mJobId(jobId), mAddr(&addr), mEventHandle(handle) { }
    } Lookup;

    typedef eastl::deque<Lookup> Queue;
    Queue mQueue;

    EA::Thread::Mutex mMutex;
    EA::Thread::Condition mCond;
    volatile bool mShutdown;
    LookupJobId mNextJobId;

    static InetAddress mInternalAddress;
    static InetAddress mExternalAddress;
    static bool mInternalHostnameOverride;
    static bool mExternalHostnameOverride;

    void run() override;

    static bool resolve(const char8_t* hostname, uint32_t& addr);

    // The address cache is a simple design intended to protect DNS servers from very large and sudden bursts of queries.
    // See https://eadpjira.ea.com/browse/GOSOPS-175450
    static const int32_t MAX_CACHED_ADDRESSES = 50;
    static const int32_t MAX_CACHED_ADDRESS_AGE_MS = 10 * 1000; // 10 seconds

    struct LookupResult { uint32_t _s_addr; EA::TDF::TimeValue obtained; };  // note, s_addr is a #define, hense the leading '_'.
    typedef LruCache<eastl::string, LookupResult, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> LookupResultLruCache;

    static LookupResultLruCache sLookupResultLruCache;
    static EA::Thread::Mutex sLookupResultLruCacheMutex;
};


extern EA_THREAD_LOCAL NameResolver* gNameResolver;

} // namespace Blaze

#endif // BLAZE_NAMERESOLVER_H

