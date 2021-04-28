#include <signal.h>
#ifdef EA_PLATFORM_LINUX
#include <unistd.h>
#endif
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/shared/framework/locales.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/componentmanager.h"
#include "BlazeSDK/loginmanager/loginmanager.h"
#include "BlazeSDK/loginmanager/loginmanagerlistener.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/blazenetworkadapter/connapiadapter.h"
#include "BlazeSDK/component/redirectorcomponent.h"
#include "EASTL/hash_map.h"
#include "EAStdC/EASprintf.h"
#include "BlazeSDK/statsapi/statsapi.h"

using namespace Blaze;

#define BLAZE3
#ifdef BLAZE3
#define GET_ERROR_NAME(x) mHub->getComponentManager()->getErrorName(x)
#else
#define GET_ERROR_NAME(x) ErrorHelp::getErrorName(x)
#endif

void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags,
        const char* file, int line)
{
    return new uint8_t[size];
}
void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName,
        int flags, unsigned debugFlags, const char* file, int line)
{
    return new uint8_t[size];
}

class TestAllocator : public EA::Allocator::ICoreAllocator
{
public:
    typedef enum { FRAMEWORK, DEFAULT, TEMP } HeapType;

    TestAllocator(HeapType type) : mType(type), mAllocations(0), mIndex(0) { }

    virtual void *Alloc(size_t size, const char *name, unsigned int flags)
    {
        return malloc(size);
    }

    virtual void *AllocDebug(size_t size,
            const EA::Allocator::ICoreAllocator::DebugParams debugParams, unsigned int flags)
    {
        return Alloc(size, nullptr, 0);
    }

    virtual void *Alloc(size_t size, const char *name, unsigned int flags,
        unsigned int align, unsigned int alignOffset = 0)
    {
        return Alloc(size, nullptr, 0);
    }

    virtual void *AllocDebug(size_t size,
            const EA::Allocator::ICoreAllocator::DebugParams debugParams, unsigned int flags,
            unsigned int align, unsigned int alignOffset = 0)
    {
        return Alloc(size, nullptr, 0);
    }

    virtual void Free(void *block, size_t size=0)
    {
        free(block);
    }

    bool ValidateHeap()
    {
        return (mAllocations == 0);
    }

    int32_t getNumAllocations() const
    {
        return mAllocations;
    }

    void printAllocations() const
    {
        AllocationPtrs::const_iterator itr = mPtrs.begin();
        AllocationPtrs::const_iterator end = mPtrs.end();
        for(; itr != end; ++itr)
        {
            printf("%d : %p\n", itr->second, itr->first);
        }
    }

    void GetHeapAllocationInfo(uint64_t &numAllocations, uint64_t &totalBytesAllocated) { }

private:
    typedef eastl::hash_map<void*, int32_t> AllocationPtrs;

    HeapType mType;
    int32_t mAllocations;
    int32_t mIndex;
    AllocationPtrs mPtrs;
};

//------------------------------------------------------------------------------
ICOREALLOCATOR_INTERFACE_API EA::Allocator::ICoreAllocator *EA::Allocator::ICoreAllocator::GetDefaultAllocator()
{
    // NOTE: we can't just use another global allocator object, since we'd have an initialization race condition between the two globals.
    //   to avoid that, we use a static function var, which is init'd the first time the function is called.
    static TestAllocator sTestHarnessAllocator(TestAllocator::DEFAULT); // static function members are only constructed on first function call
    return &sTestHarnessAllocator;
}


extern "C"
{
void *DirtyMemAlloc(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    return malloc(static_cast<size_t>(iSize));
}


void DirtyMemFree(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    free(pMem);
}
}


static void logFunction(const char8_t *msg, void *context)
{
    printf("%s", msg);
}

class TestHarness
    : private Blaze::BlazeStateEventHandler
{
public:
    TestHarness()
        : mHub(nullptr),
          mFrameworkAllocator(TestAllocator::FRAMEWORK),
          mDefaultAllocator(TestAllocator::DEFAULT),
          mConnected(false),
          mShutdown(false),
          mReconnect(false)
    {
    }

    ~TestHarness()
    {
        Blaze::BlazeHub::shutdown(mHub);
        NetConnShutdown(0);

        printAllocationInfo();
    }

    void printAllocationInfo()
    {
        printf("FrameworkAllocator: %d\n", mFrameworkAllocator.getNumAllocations());
        printf("DefaultAllocator: %d\n", mDefaultAllocator.getNumAllocations());
        mDefaultAllocator.printAllocations();
    }

    bool init(const char8_t* serviceName, const char8_t* rdir, const char8_t* masterAccount,
            const char8_t* password, const char8_t* persona)
    {
        if (masterAccount != nullptr)
            blaze_strnzcpy(mMasterAccount, masterAccount, sizeof(mMasterAccount));
        else
            blaze_strnzcpy(mMasterAccount, "blaze-stress00001@ea.com", sizeof(mMasterAccount));
        if (password != nullptr)
            blaze_strnzcpy(mPassword, password, sizeof(mPassword));
        else
            blaze_strnzcpy(mPassword, "password", sizeof(mPassword));
        if (persona != nullptr)
            blaze_strnzcpy(mPersona, persona, sizeof(mPersona));
        else
            blaze_strnzcpy(mPersona, "s00001", sizeof(mPersona));
        NetConnStartup("-servicename=blaze_testharness");

        strncpy(mInitParams.ServiceName, serviceName, sizeof(mInitParams.ServiceName)-1);
        strcpy(mInitParams.ClientName, "LinuxTestHarness");
        strcpy(mInitParams.ClientVersion, "1.0");
        strcpy(mInitParams.ClientSkuId, "1234");
        mInitParams.Client = CLIENT_TYPE_DEDICATED_SERVER;
        mInitParams.Secure = true;
        mInitParams.Environment = Blaze::ENVIRONMENT_SDEV;
        mInitParams.UserCount = 1;
        mInitParams.OutgoingBufferSize = 8192;
        mInitParams.MaxIncomingPacketSize = 65535;
        mInitParams.Locale = Blaze::LOCALE_DEFAULT;
        mInitParams.MaxCachedUserCount = 32;
        mInitParams.UserCount = 1;

        if (rdir != nullptr)
            strcpy(mInitParams.Override.RedirectorAddress, rdir);

//         Blaze::Allocator::setAllocator(&mDefaultAllocator);
//         Blaze::Allocator::setAllocator(Blaze::MEM_GROUP_FRAMEWORK, &mFrameworkAllocator);

        mHub = nullptr;
        Blaze::BlazeError rc = Blaze::BlazeHub::initialize(
                &mHub, mInitParams, &mDefaultAllocator, &logFunction, nullptr);
        if (rc != Blaze::ERR_OK)
        {
            printf("BlazeHub failed to initialize: 0x%08x\n", rc);
            return false;
        }
        mHub->addUserStateEventHandler(this);
        return true;
    }

    void connect()
    {
        Blaze::ConnectionManager::ConnectionManager* mgr = mHub->getConnectionManager();
        Blaze::ConnectionManager::ConnectionManager::ConnectionMessagesCb cb(this, &TestHarness::onConnect);
        mgr->connect(cb);
    }

    void onConnect(Blaze::BlazeError result, const Blaze::Redirector::DisplayMessageList* msgs)
    {
        Blaze::Redirector::DisplayMessageList::const_iterator itr = msgs->begin();
        Blaze::Redirector::DisplayMessageList::const_iterator end = msgs->end();
        for(; itr != end; ++itr)
            printf("***RDIR_MESSAGE***: %s\n", (*itr).c_str());
    }

    bool idle()
    {
        if (mShutdown)
            return false;
        mHub->idle();

        if (mReconnect)
        {
            mReconnect = false;
            connect();
        }
        return true;
    }

private:
    Blaze::BlazeHub::InitParameters mInitParams;
    Blaze::BlazeHub* mHub;
    TestAllocator mFrameworkAllocator;
    TestAllocator mDefaultAllocator;
    bool mConnected;
    bool mShutdown;
    bool mReconnect;
    Blaze::JobId mRetryJobId;
    char8_t mMasterAccount[256];
    char8_t mPassword[256];
    char8_t mPersona[256];

    void onRetry()
    {
        connect();
    }

    void onDisconnected(BlazeError errorCode)
    {
        printf("# got state change to disconnected; err=%u\n", errorCode);
        if (mConnected == false)
        {
            Blaze::Functor cb(this, &TestHarness::onRetry);
            mRetryJobId = mHub->getScheduler()->scheduleFunctor(cb, this, 250);
            return;
        }
        mConnected = false;
    }

    void onConnected()
    {
        printf("# got state change to connected\n");
        mConnected = true;
        startLogin();
    }

    void onAuthenticated(uint32_t userIndex)
    {
        // We're now authenticated
        printf("## authenticated\n");
    }

    void ExpressLoginCb(const Authentication::LoginResponse *response, BlazeError blazeError, JobId JobId)
    {
        if (blazeError != ERR_OK)
        {
            // Login failed, disconnect here, but first set mConnected=false so that onDisconnected schedules a retry.
            mConnected = false;
            mHub->getConnectionManager()->disconnect(blazeError);
        }
    }

    void startLogin()
    {
        Blaze::Authentication::ExpressLoginRequest request;
        request.setEmail(mMasterAccount);
        request.setPassword(mPassword);
        request.setPersonaName(mPersona);

        Authentication::AuthenticationComponent *auth = mHub->getComponentManager(0)->getAuthenticationComponent();
        auth->expressLogin(request, Authentication::AuthenticationComponent::ExpressLoginCb(this, &TestHarness::ExpressLoginCb));
    }
};

static bool gShutdown = false;

static void shutdownHandler(int sig)
{
    gShutdown = true;
}

static int usage(const char8_t* prg)
{
    printf("Usage: %s -s serviceName [-r rdir] [-m masterAccount] [-p persona] [-P password]\n",
            prg);
    return 1;
}

int main(int argc, char** argv)
{
    signal(SIGINT, shutdownHandler);

    TestHarness harness;

    const char8_t* rdir = nullptr;
    const char8_t* master = nullptr;
    const char8_t* persona = nullptr;
    const char8_t* password = nullptr;
    const char8_t* serviceName = nullptr;

#ifdef EA_PLATFORM_LINUX
    int o;
    while ((o = getopt(argc, argv, "m:p:P:r:s:")) != -1)
    {
        switch (o)
        {
            case 'm':
                master = optarg;
                break;
            case 'p':
                persona = optarg;
                break;
            case 'P':
                password = optarg;
                break;
            case 'r':
                rdir = optarg;
                break;
            case 's':
                serviceName = optarg;
                break;
            default:
                return usage(argv[0]);
        }
    }
#endif

    if (serviceName == nullptr)
        return usage(argv[0]);

    if (!harness.init(serviceName, rdir, master, password, persona))
        return 1;
    harness.connect();

    uint32_t iteration = 0;

    // Added check for iteration # to force an exit after an extended period of time
    // change this 61 value as needed
    while (!gShutdown&&(iteration<61))
    {
        NetConnControl('poll', 0, 0, 0, 0);
        if (!harness.idle())
            break;
        NetConnSleep(100);
        ++iteration;
        printf("ITERATION #: %d \n", iteration);
    }

    return 0;
}

//Vsnprintf8 for eastl
int Vsnprintf8 (char8_t*  pDestination, size_t n, const char8_t*  pFormat, va_list arguments); //necessary for stupid codewarrior warning.
int Vsnprintf8 (char8_t*  pDestination, size_t n, const char8_t*  pFormat, va_list arguments)
{
    return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
}
