/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class NameResolver

    This class provides the ability to resolve hostnames in either a blocking or non-blocking
    fashion.  It is a singleton because the underlying system calls (gethostbyname) are not
    thread safe so there must only ever be one call being made in a given process at one time.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#ifdef EA_PLATFORM_WINDOWS
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include "framework/connection/nameresolver.h"
#include "framework/connection/inetaddress.h"
#include "framework/connection/selector.h"
#include "framework/system/job.h"
#include "framework/util/shared/blazestring.h"
#include "framework/system/threadlocal.h"
#include "framework/logger.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

InetAddress NameResolver::mInternalAddress;
InetAddress NameResolver::mExternalAddress;
bool NameResolver::mInternalHostnameOverride;
bool NameResolver::mExternalHostnameOverride;

/*** Public Methods ******************************************************************************/

NameResolver::NameResolver(const char *baseName)
    : BlazeThread("name_resolver",BlazeThread::NAME_RESOLVER, 256 * 1024),
      mShutdown(false),
      mNextJobId(0)
{
    char8_t name[256];
    blaze_snzprintf(name, sizeof(name), "%sNR", baseName);
    BlazeThread::setName(name);
}

NameResolver::~NameResolver()
{
}

void NameResolver::stop()
{
    if (!mShutdown)
    {
        mShutdown = true;
        mCond.Signal(true);
        waitForEnd();
    }
}

BlazeRpcError NameResolver::resolve(InetAddress& addr, LookupJobId& jobId, const TimeValue& absoluteTimeout)
{
    BlazeRpcError rc = ERR_OK;

    //our real address can be referenced all over the place by this thread, so make a copy of it.
    InetAddress copyAddr(addr);

    Fiber::EventHandle eventHandle = Fiber::getNextEventHandle();

    mMutex.Lock();
    //increment the job id in the mutex (even though one thread should only ever run this code)
    jobId = mNextJobId++;
    mQueue.push_back(Lookup(jobId, copyAddr, eventHandle));
    mMutex.Unlock();

    mCond.Signal(true);

    BlazeRpcError err = Fiber::wait(eventHandle, "NameResolver::resolve", absoluteTimeout);
    if (err != ERR_OK)
    {
        //Cancel our job for good measure - this will remove the lookup from the queue and ensure the other thread doesn't touch it
        cancelResolve(jobId);
        rc = err;
    }
    else 
    {
        addr.setIp(copyAddr.getIp(InetAddress::NET), InetAddress::NET);
        if (addr.getIp() == 0xFFFFFFFF)
        {
            rc = ERR_SYSTEM;
        }
    }


    return rc;
}

void NameResolver::cancelResolve(LookupJobId jobId)
{
    //Pull any job out that has the id.  We're not really concerned with performance as the number
    //of jobs is going to always be very low and the number of times we cancel will be even lower than that
    mMutex.Lock();
    for (Queue::iterator itr = mQueue.begin(), end = mQueue.end(); itr != end; ++itr)
    {
        if (itr->mJobId == jobId)
        {
            Fiber::EventHandle eventHandle= itr->mEventHandle;
            
            //Set the ip to invalid and wake us like normal.
            itr->mAddr->setIp(0xFFFFFFFF, InetAddress::HOST);

            //kill the job, the other thread won't see it.
            mQueue.erase(itr);

            //unlock before the signal to ensure we don't hold the lock while doing other stuff
            mMutex.Unlock();
            
            //Signal and bail, we're done
            Fiber::signal(eventHandle, Blaze::ERR_OK);
            return;
        }
    }

    //If we got here, nothing was canceled.
    mMutex.Unlock();
}

// static
bool NameResolver::blockingResolve(InetAddress& addr)
{
    uint32_t ip = 0;
    bool result = resolve(addr.getHostname(), ip);
    if (result)
        addr.setIp(ip, InetAddress::NET);
    return result;
}

bool NameResolver::blockingVerify(const char8_t *hostname, const InetAddress& addr)
{
    BLAZE_TRACE_LOG(Log::CONNECTION, "[NameResolver].blockingLookup: Verifying hostname (" << hostname << ") resolves to correct address (" << addr << ")");
    InetAddress check(hostname);
    if (blockingResolve(check))
    {
        if (check.getIp(InetAddress::NET) == addr.getIp(InetAddress::NET))
        {
            return true;
        }
        else
        {
            BLAZE_TRACE_LOG(Log::CONNECTION, "[NameResolver].blockingLookup: The hostname (" << hostname << ") returned from reverse DNS lookup on IP address (" << addr << ") resolves to a different IP address (" << check << ")");
        }
    }
    else
    {
        BLAZE_TRACE_LOG(Log::CONNECTION, "[NameResolver].blockingLookup: The hostname (" << hostname << ") returned from reverse DNS lookup on IP address (" << addr << ") could not be resolved to an IP address");
    }

    return false;
}

bool NameResolver::blockingLookup(InetAddress& addr)
{
    BLAZE_TRACE_LOG(Log::CONNECTION, "[NameResolver].blockingLookup: Start hostname lookup for (" << addr << ")");
    char8_t hostname[NI_MAXHOST];
    char8_t servInfo[NI_MAXSERV];

    sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = addr.getIp(InetAddress::NET);
    sa.sin_port = addr.getPort(InetAddress::NET);

    // First, perform a reverse DNS lookup
BLAZE_PUSH_ALLOC_OVERRIDE(MEM_GROUP_FRAMEWORK_NOLEAK, "getnameinfo");
    int32_t result = getnameinfo((sockaddr*)&sa, sizeof(sockaddr_in), hostname, NI_MAXHOST, servInfo, NI_MAXSERV, NI_NAMEREQD);
BLAZE_POP_ALLOC_OVERRIDE();

    if (result == 0)
    {
        // getnameinfo() found a hostname for the IP address, now we need
        // to verify that it actually resolves to the correct IP.
        if (blockingVerify(hostname, addr))
        {
            BLAZE_TRACE_LOG(Log::CONNECTION, "[NameResolver].blockingLookup: Setting hostname to (" << hostname << ")");
            addr.setHostname(hostname);
            return true;
        }
    }
    else
    {
        BLAZE_TRACE_LOG(Log::CONNECTION, "[NameResolver].blockingLookup: getnameinfo() failed with (" << result << ") when looking up IP address (" << addr << ")");
    }

    // Reverse DNS lookup was not able to find a hostname that can be resolved to the specified IP address.
    // Now, we're going to see if we can find the FQDN by using the hostname and the suffix 'ea.com'.
    hostname[0] = '\0';
    result = gethostname(hostname, sizeof(hostname));
    if (result == 0)
    {
        BLAZE_TRACE_LOG(Log::CONNECTION, "[NameResolver].blockingLookup: gethostname() returned hostname (" << hostname << ")");
        char8_t tmp[NI_MAXHOST];
        char8_t *dot = hostname;
        do
        {
            dot = strchr(dot, '.');
            if (dot != nullptr)
                *dot = '\0';

            blaze_strnzcpy(tmp, hostname, sizeof(tmp));
            blaze_strnzcat(tmp, ".ea.com", sizeof(tmp));

            if (blockingVerify(tmp, addr))
            {
                BLAZE_TRACE_LOG(Log::CONNECTION, "[NameResolver].blockingLookup: Setting hostname to (" << tmp << ")");
                addr.setHostname(tmp);
                return true;
            }

            if (dot != nullptr)
                *dot++ = '.';
        }
        while (dot != nullptr);
    }
    else
    {
        BLAZE_TRACE_LOG(Log::CONNECTION, "[NameResolver].blockingLookup: gethostname() failed with (" << result << ")");
    }

    BLAZE_TRACE_LOG(Log::CONNECTION, "[NameResolver].blockingLookup: could not find hostname");

    return false;
}


/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

void NameResolver::run()
{
    while (!mShutdown)
    {
        mMutex.Lock();
        if (mQueue.empty())
            mCond.Wait(&mMutex);
        if (mShutdown)
        {
            mMutex.Unlock();
            break;
        }

        if (mQueue.empty())
        {
            mMutex.Unlock();
            continue;
        }

        Lookup& lookup = mQueue.front();
        InetAddress addr = *lookup.mAddr;
        LookupJobId id = lookup.mJobId;
        mMutex.Unlock();

        uint32_t ip = 0;
        bool result = resolve(addr.getHostname(), ip);
        if (result)
            addr.setIp(ip, InetAddress::NET);
        else
            addr.setIp((uint32_t)-1, InetAddress::NET);


        //Relock on the job (make sure we have a job waiting and its the same one)
        mMutex.Lock();
        if (!mQueue.empty())
        {
            Lookup& item = mQueue.front();
            if (item.mJobId == id)
            {
                //Set the result and schedule the call.
                *item.mAddr = addr;
                Fiber::signal(item.mEventHandle, Blaze::ERR_OK);
                mQueue.pop_front();
            }
        }
        mMutex.Unlock();
    }
}

// static

NameResolver::LookupResultLruCache NameResolver::sLookupResultLruCache(MAX_CACHED_ADDRESSES, BlazeStlAllocator("NameResolver::sLookupResultLruCache", MEM_GROUP_FRAMEWORK_DEFAULT));
EA::Thread::Mutex NameResolver::sLookupResultLruCacheMutex;

bool NameResolver::resolve(const char8_t* hostname, uint32_t& addr)
{
    addr = 0;

    in_addr sinAddr;
    int result = inet_pton(AF_INET, hostname, &sinAddr);
    if (result <= 0)
    {
        eastl::string hostnameString = hostname;

        LookupResult lookupResult;
        sLookupResultLruCacheMutex.Lock();
        if (sLookupResultLruCache.get(hostnameString, lookupResult) && (TimeValue::getTimeOfDay() - lookupResult.obtained > MAX_CACHED_ADDRESS_AGE_MS * 1000))
        {
            sLookupResultLruCache.remove(hostnameString);
            lookupResult = { 0, 0 };
        }
        sLookupResultLruCacheMutex.Unlock();

        // Check if we have not yet obtained this addr info before proceeding with call to getaddrinfo().
        if (lookupResult.obtained == 0)
        {
            struct addrinfo hints;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            struct addrinfo* list = nullptr;

BLAZE_PUSH_ALLOC_OVERRIDE(MEM_GROUP_FRAMEWORK_NOLEAK, "getaddrinfo");
            result = getaddrinfo(hostnameString.c_str(), nullptr, &hints, &list);
BLAZE_POP_ALLOC_OVERRIDE();

            if (result == 0)
            {
                sinAddr.s_addr = ((sockaddr_in*)list->ai_addr)->sin_addr.s_addr;
                freeaddrinfo(list);
            }
            else
            {
                sinAddr.s_addr = 0;
                BLAZE_WARN_LOG(Log::CONNECTION, "[NameResolver].resolve: The hostname(" << hostnameString << ") resolution failed, result=" << result);
            }

            lookupResult._s_addr = sinAddr.s_addr;
            lookupResult.obtained = TimeValue::getTimeOfDay();

            // If the DNS query succeeded OR failed for whatever reason, we still push the result into the LRU cache.
            // This just a simple protection mechanism that assumes not much will change in the next few seconds (MAX_CACHED_ADDRESS_AGE_MS)
            // for the same hostname lookup against the same DNS server.  But, what it does do is reduce the pressure on the DNS
            // server just-in-case it's overloaded right now.
            sLookupResultLruCacheMutex.Lock();
            sLookupResultLruCache.add(hostnameString, lookupResult);
            sLookupResultLruCacheMutex.Unlock();
        }
        else
        {
            if (lookupResult._s_addr == 0)
            {
                // This LRU cache mechanism is obviously meant to avoid making excessive calls to getaddrinfo().  But, because
                // in this case, where lookupResult._s_addr is == 0, that means that the last attempt to getaddrinfo() for the hostname
                // FAILED.  That said, it's entirely plausible that the calling code has retry logic in-place.  That retry logic
                // may be written (poorly) so as to retry *immediately* thinking that it would be okay because of the blocking nature
                // of network related calls.  This that presumption, we don't want to allow such poorly written retry logic
                // to enter a busy-wait, chewing up the cpu, all because we skipped over the blocking call to getaddrinfo().
                // 
                // All of the above reasoning leads to the following seemingly arbitrary call to sleep()....

                EA::Thread::ThreadSleep(100); // 100 milliseconds.
            }

            // We have a cached copy of the previous lookup attempt.  We take the cached value even if it is invalid.
            sinAddr.s_addr = lookupResult._s_addr;
            BLAZE_TRACE_LOG(Log::CONNECTION, "[NameResolver].resolve: Found hostname(" << hostnameString << ") cached with ipAddr=" << InetAddress(lookupResult._s_addr, 0, InetAddress::NET));
        }
    }

    uint32_t ip = sinAddr.s_addr;

    // Remap localhost to the IP address of the internal interface on the server
    if ((ip == htonl(0x7f000001)) && mInternalAddress.isResolved())
        ip = mInternalAddress.getIp();

    addr = ip;
    return (ip != 0);
}

// static
void NameResolver::setInterfaceAddresses(const InetAddress& internal, const InetAddress& external, bool internalHostnameOverride, bool externalHostnameOverride)
{
    mInternalAddress = internal;
    mExternalAddress = external;
    mInternalHostnameOverride = internalHostnameOverride;
    mExternalHostnameOverride = externalHostnameOverride;
}

} // namespace Blaze


#ifdef EA_PLATFORM_LINUX
#ifdef BLAZE_PROFILE

/*
 * The following functions override the getaddrinfo() and gethostbyname_r() glibc calls when
 * building a statically linked, gprof-profiled version of the code.  This is necessary because
 * the server will crash when running a load test against any components which make frequent
 * use of these calls.  For some reason statically linked applications still need to use the
 * shared glibc library when doing name resolution.  This appears to mess things up and cause
 * the crash.  By overriding these functions, it allows people to run stress tests with gprof'd
 * executables properly.
 *
 * These implementation simply resolve hostname from a file called hosts.cfg in the current
 * working directory.  The hosts.cfg file is just a list of hostname,ip pairs.  When a name
 * cannot be resolved, the server will halt immediately and print the name of the host it was
 * unable to resolve.  You can use this to build up a hosts.cfg file.  Generally the server doesn't
 * need too many entries in it.  An example file might look:
 *
 * localhost,127.0.0.1
 * integration.nucleus.ea.com,10.102.219.37
 * novafusion.integration.ea.com,10.102.219.42
 * peach.online.ea.com,10.30.80.231
 * betos.ea.com,10.120.48.31
 * sdevbesl01.online.ea.com,10.30.80.231
 * betacsrapp01.beta.eao.abn-iad.ea.com,10.120.48.140
 * gosredirector.online.ea.com,10.30.80.241
 */

struct HostEntry
{
    char name[256];
    uint32_t addr;
};

typedef eastl::vector<HostEntry> HostEntryList;
static HostEntryList sHostEntries;
static bool sHostEntriesInitialized = false;
static EA::Thread::Mutex sHostEntriesMutex;

static void _profileLoadHostsTable()
{
    sHostEntriesMutex.Lock();
    if (!sHostEntriesInitialized)
    {
        FILE* fp = fopen("hosts.cfg", "r");
        if (fp != nullptr)
        {
            char8_t buf[256];
            while (fgets(buf, sizeof(buf), fp) != nullptr)
            {
                char8_t* s = strchr(buf, ',');
                if (s != nullptr)
                {
                    sHostEntries.push_back();
                    HostEntry& entry = sHostEntries.back();
                    blaze_strnzcpy(entry.name, buf, sizeof(entry.name));
                    entry.name[s - buf] = '\0';
                    entry.addr = inet_addr(s+1);
                }
            }
            fclose(fp);
        }
        sHostEntriesInitialized = true;
    }
    sHostEntriesMutex.Unlock();
}


extern "C" uint32_t _gethostname(const char* name)
{
    uint32_t addr = inet_addr(name);
    if (addr == INADDR_NONE)
    {
        if (!sHostEntriesInitialized)
            _profileLoadHostsTable();

        HostEntryList::iterator itr = sHostEntries.begin();
        HostEntryList::iterator end = sHostEntries.end();
        for(; itr != end; ++itr)
        {
            if (blaze_stricmp(name, itr->name) == 0)
            {
                addr = itr->addr;
                break;
            }
        }
    }

    if (addr == INADDR_NONE)
    {
        eastl::string buf;
        buf.append("*** Profiled builds are statically linked and due to problems with the MySql\n"
               "*** client API must override the name resolution functions (gethostbyname, etc) "
               "*** to prevent the server from crashing.  The overridden functions resolve \n"
               "*** hostnames from the hosts.cfg file located in the directory where you run \n"
               "*** the blaze server. The file simply contains pairs of hostname,ip entries.\n");
        buf.append(("***\n");
        buf.append(("*** _gethostname unable to resolve '%s' from hosts.cfg\n", name);
        buf.append(("***\n");

        printf(buf.c_str());
#if defined EA_PLATFORM_WINDOWS
        OutputDebugString(buf.c_str());
#endif

        exit(ProcessController::EXIT_FATAL);
    }
    return addr;
}

extern "C" int getaddrinfo(const char* node, const char* service, const struct addrinfo* hints, struct addrinfo** res)
{
    uint32_t addr = _gethostname(node);
    if (addr == INADDR_NONE)
        return -1;

    *res = (struct addrinfo*)BLAZE_ALLOC(sizeof(struct addrinfo));
    (*res)->ai_addr = (sockaddr*)BLAZE_ALLOC(sizeof(sockaddr_in));
    sockaddr_in* a =(sockaddr_in*)((*res)->ai_addr);
    a->sin_addr.s_addr = addr;
    return 0;
}

extern "C" void freeaddrinfo(struct addrinfo* res)
{
    if (res != nullptr)
    {
        BLAZE_FREE(res->ai_addr);
        BLAZE_FREE(res);
    }
}

extern "C" int gethostbyname_r(const char* name, struct hostent* ret, char* buf, size_t buflen, struct hostent** result, int* h_errnop)
{
    uint32_t addr = _gethostname(name);
    if (addr == 0)
    {
        *result = nullptr;
        *h_errnop = EINVAL;
        return -1;
    }
    *result = ret;
    memset(ret, 0, sizeof(ret));
    ret->h_addr_list = (char**)buf;
    buf += sizeof(char*) * 2;
    ret->h_length = sizeof(uint32_t);
    ret->h_addr_list[0] = buf;
    ret->h_addr_list[1] = buf + sizeof(char*);
    memcpy(ret->h_addr_list[0], &addr, sizeof(addr));
    memset(ret->h_addr_list[1], 0, sizeof(addr));
    *h_errnop = 0;
    return 0;
}

#endif // BLAZE_PROFILE
#endif // EA_PLATFORM_LINUX

