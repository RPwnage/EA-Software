///////////////////////////////////////////////////////////////////////////////
// ConnectionDetection.cpp
// 
// Created by Paul Pedriana
// Copyright (c) 2010 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "services/network/NetworkAccessManager.h"
#include "services/debug/DebugService.h"
#include "InternetConnectionDetection.h"
#include <algorithm>

#include <QEventLoop>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSharedPointer>
#include <QTimer>
#include <QUrl>
#include <QWeakPointer>

#if defined(_WIN32)
    #include <ntverp.h>         // USed to detect the Windows SDK version. // VER_PRODUCTBUILD 6001
    #include <Windows.h>
    #include <Wininet.h>
    #include <netlistmgr.h>     // This requires a somewhat recent version of the Windows SDK, for Vista+.

    #pragma comment(lib, "Wininet.lib")
    #pragma comment(lib, "ole32.lib")
    #pragma comment(lib, "Uuid.lib")
    #pragma comment(lib, "Sensapi.lib")
#else
    #include <SystemConfiguration/SCNetworkReachability.h>
#endif



namespace Origin
{

namespace Services
{

namespace Network
{
 
    bool sInitialized = false;              // True if Init was called and executed successfully.
    bool sWasCoInitialized = false;

#if ! defined(_WIN32)
    /// A function to determine the online status with respect to a given hostname.
    bool isOnlineHost(const char* host)
    {
        // Use CoreFoundation to obtain the network reachability flags to a specific address.
        SCNetworkReachabilityRef target = SCNetworkReachabilityCreateWithName(0, host);
        SCNetworkReachabilityFlags flags;
        SCNetworkReachabilityGetFlags(target, &flags);
        CFRelease(target);
        
        // Return true if the network is reachable, and does not require a connection to be established.
        if ((flags & kSCNetworkFlagsReachable) && ! (flags & kSCNetworkFlagsConnectionRequired))
            return true;
        
        // Otherwise, return false.
        else
            return false;
    }
#endif

namespace
{
    QSharedPointer<ConnectionDetection> sInstance;
};

void ConnectionDetection::init()
{
    if (sInstance.isNull())
        sInstance = QSharedPointer<ConnectionDetection>(new ConnectionDetection);
}

void ConnectionDetection::release()
{
    sInstance.clear();
}

QSharedPointer<ConnectionDetection> ConnectionDetection::instance()
{
    ORIGIN_ASSERT(sInstance.data());
    return sInstance;
}

ConnectionStatus::ConnectionStatus()
  : mbChanged(false)
  , mbConnectedToNetwork(true)
  , mbConnectedToInternet(true)
  , mbConnectedToURLs(true)
  , mConnectedURLs()
  , mUnconnectedURLs()
{
}


ConnectionDetection::ConnectionDetection()
  : mbEnabled(true),
    mbPollActive(false),
    mbPollingRequired(true),
    mbPollThreadCreated(false),
    mbThreadShouldQuit(false),
    mPollThreadTimeCheckSeconds(120),
    mThread(),
    mMutex(),   
    mICS(),
    mpListener(NULL)

#if defined(_WIN32)
  , mpNetworkListManager(NULL)
  , mpConnectionPointContainer(NULL)
  , mpConnectionPoint(NULL)
  , mCookie(0)
  , mRefCount(1)
#endif
{
}


ConnectionDetection::~ConnectionDetection()
{
    Shutdown();
}


IConnectionListener* ConnectionDetection::GetListener()
{
    return mpListener;
}


void ConnectionDetection::SetListener(IConnectionListener* pListener)
{
    mpListener = pListener;
}


bool ConnectionDetection::GetEnabled() const
{
    return mbEnabled;
}


void ConnectionDetection::SetEnabled(bool enabled)
{
    ConnectionStatus ics;

    mMutex.Lock();

    mbEnabled = enabled;

    if(enabled)
    {
        Poll(&ics);
    }
    else // We assume we are connected when we are disabled.
    {
        ics.mbChanged             = true;
        ics.mbConnectedToNetwork  = true;
        ics.mbConnectedToInternet = true;
        ics.mbConnectedToURLs     = true;
        ics.mConnectedURLs        = mURLList;
        ics.mUnconnectedURLs.clear();
    }

    mICS = ics;

    mMutex.Unlock();

    if(ics.mbChanged && mpListener)
        mpListener->ConnectionChange(ics);
}


#if defined(_WIN32)

    ULONG ConnectionDetection::AddRef()
    {
        InterlockedIncrement((volatile LONG*)&mRefCount);
        return mRefCount;
    }

    ULONG ConnectionDetection::Release()
    {
        ULONG newRefCount = InterlockedDecrement((volatile LONG*)&mRefCount);

        if(mRefCount == 0)
            delete this;

        return newRefCount;
    }

    HRESULT ConnectionDetection::QueryInterface(REFIID riid, LPVOID* ppvObj)
    {
        if(ppvObj)
        {
            *ppvObj = NULL;

            if(riid == IID_IUnknown)
                *ppvObj = static_cast<IUnknown*>(this);
            else if(riid == IID_INetworkListManagerEvents)
                *ppvObj = static_cast<INetworkListManagerEvents*>(this);

            if(*ppvObj)
            {
                AddRef();
                return NOERROR;
            }

            return E_NOINTERFACE;
        }

        return E_INVALIDARG;
    }


    HRESULT ConnectionDetection::ConnectivityChanged(NLM_CONNECTIVITY newConnectivity)
    {
        ConnectionStatus ics;

        mMutex.Lock();

        if(mbEnabled)
        {
            const bool bConnectedToNetwork  = (newConnectivity != 0);
            const bool bConnectedToInternet = ((newConnectivity & (NLM_CONNECTIVITY_IPV4_INTERNET | NLM_CONNECTIVITY_IPV6_INTERNET)) != 0);

            if((bConnectedToNetwork  != mICS.mbConnectedToNetwork) || 
               (bConnectedToInternet != mICS.mbConnectedToInternet))
            {
                mICS.mbChanged             = true;
                mICS.mbConnectedToNetwork  = bConnectedToNetwork;
                mICS.mbConnectedToInternet = bConnectedToInternet;

                if(mICS.mbConnectedToInternet)
                {
                    // Question: Is it safe to do this polling within this system callback?
                    PollURLs(&mICS);
                }

                ics = mICS; // Make a copy for use outside of our mutex lock.
            }
        }

        mMutex.Unlock();

        if(ics.mbChanged && mpListener)
            mpListener->ConnectionChange(ics);

        return NOERROR;
    }

#endif



bool ConnectionDetection::Init()
{
    bool bResult = true;

    mbPollingRequired = true;

    #if defined(_WIN32)
        // Windows NetworkListManager
        // http://msdn.microsoft.com/en-us/library/aa370803%28v=VS.85%29.aspx
        // 
        // INetworkListManagerEvents code taken from http://msdn.microsoft.com/en-us/library/ee264321%28VS.85%29.aspx
        // Microsoft's means for doing this doesn't seem very pretty, and I'm inclined to distrust it for the time being
        // and go with the polling method instead.

        sWasCoInitialized = CoInitialize(NULL);

        if(!mpNetworkListManager)
        {
            HRESULT hr = CoCreateInstance(CLSID_NetworkListManager, NULL, CLSCTX_ALL, IID_INetworkListManager, (LPVOID*)&mpNetworkListManager);

            if(SUCCEEDED(hr))
            {
                // Cast NetworkListManager to a ConnectionPointContainer. This should always succeed.
                hr = mpNetworkListManager->QueryInterface(IID_IConnectionPointContainer, (void**)&mpConnectionPointContainer);

                if(SUCCEEDED(hr))
                {
                    // Find an interface for INetworkListManagerEvents. This should always succeed.
                    // We could also or instead listen for INetworkEvents or INetworkConnectionEvents.
                    // case we'd have to add an Advise for it and add our callback code for it.
                    hr = mpConnectionPointContainer->FindConnectionPoint(IID_INetworkListManagerEvents, &mpConnectionPoint);

                    if(SUCCEEDED(hr))
                    {
                        // Tell the connection point to advise us of network list manager events through our 
                        // INetworkListManagerEvents interface function ConnectivityChanged.
                        hr = mpConnectionPoint->Advise(static_cast<IUnknown*>(this), &mCookie);

                        if(SUCCEEDED(hr))
                            mbPollingRequired = true; // Note by Paul Pedriana April 17, 2011: mbPollingRequired should be set to false here, not true. I am leaving this as-is because we have an imminent release and fixing this now may be too risky to attempt now. We should set this to false after the imminent release.
                        else
                            mCookie = 0;       // Should already be zero.
                    }
                }

                if(mbPollingRequired) // If the above somehow failed...
                {
                    if(mpConnectionPoint)
                    {
                        mpConnectionPoint->Release();
                        mpConnectionPoint = NULL;
                    }

                    if(mpConnectionPointContainer)
                    {
                        mpConnectionPointContainer->Release();
                        mpConnectionPointContainer = NULL;
                    }

                    if(mpNetworkListManager)
                    {
                        mpNetworkListManager->Release();
                        mpNetworkListManager = NULL;
                    }
                }
            }
        }
    #endif

    sInitialized = bResult;

    return true;
}
 

bool ConnectionDetection::Shutdown()
{
    // EA_ASSERT(mMutex.IsLocked());

    if(mbPollThreadCreated)
    {
        EndThread();
        mbPollThreadCreated = false;
    }

    #if defined(_WIN32)
        if(mpConnectionPoint)
        {
            if(mCookie)
            {
                mpConnectionPoint->Unadvise(mCookie);
                mCookie = 0;
            }

            mpConnectionPoint->Release();
            mpConnectionPoint = NULL;
        }

        if(mpConnectionPointContainer)
        {
            mpConnectionPointContainer->Release();
            mpConnectionPointContainer = NULL;
        }

        if(mpNetworkListManager)
        {
            mpNetworkListManager->Release();
            mpNetworkListManager = NULL;
        }

        if (sWasCoInitialized)
        {
            CoUninitialize();
            sWasCoInitialized = false;
        }
    #endif

    sInitialized = false;

    return true;
}


bool ConnectionDetection::PollingRequired() const
{
    // EA_ASSERT(mbInitialized);

    return mbPollingRequired;
}


bool ConnectionDetection::StartAutoPoll(int pollThreadTimeCheckSeconds)
{
    mPollThreadTimeCheckSeconds = pollThreadTimeCheckSeconds;

    bool bResult = BeginThread();

    if(bResult) 
        mbPollThreadCreated = true;

    return bResult;
}


void ConnectionDetection::AddURL(const char* pURL)
{
    if (strcmp(pURL, "") != 0)
    {
        EA::Thread::AutoMutex autoMutex(mMutex);

        String sURL(pURL);
        StringList::iterator it = std::find(mURLList.begin(), mURLList.end(), sURL);

        if(it == mURLList.end())
            mURLList.push_back(sURL);
    }
}


void ConnectionDetection::RemoveURL(const char* pURL)
{
    EA::Thread::AutoMutex autoMutex(mMutex);

    String sURL(pURL);
    StringList::iterator it = std::find(mURLList.begin(), mURLList.end(), sURL);

    if(it != mURLList.end())
        mURLList.erase(it);
}


bool ConnectionDetection::PollConnectivity(ConnectionStatus* pICS)
{
    // EA_ASSERT(pICS && mMutex.HasLock());
    bool bChanged = false;

    if(mbEnabled)
    {
        bool bConnectedToNetworkSaved  = pICS->mbConnectedToNetwork;
        bool bConnectedToInternetSaved = pICS->mbConnectedToInternet;

        #if defined(_WIN32)
            // If we can use the newer NetworkListManager interface, do so.
            // From a web site I found: "Caution: You cannot use the NetworkListManagerClass over  
            // different threads, but only out of the thread, which instantiated it"
    
            HRESULT hr = CoInitialize(NULL); // For some reason I am getting 'coinitialize not called' hr, even though we call it above. So I added a call here.
                                // P.S. Call it once per thread http://support.microsoft.com/kb/206076
            bool wasCoInitialized = (hr == S_OK);

            //  0x8001010E: The application called an interface that was marshalled for a different thread.
            //  Intermediate fix: Simply destroy & create the interface everytime we use it, so we make sure, we call it always from the correct thread
            //
            //  TODO: This is no ideal fix. We should replace it with a proper solution, like thread specific data or something similar.
            //          http://support.microsoft.com/kb/206076    
            //
            
            if(mpNetworkListManager)
                mpNetworkListManager->Release();
            mpNetworkListManager=NULL;

            CoCreateInstance(CLSID_NetworkListManager, NULL, CLSCTX_ALL, IID_INetworkListManager, (LPVOID*)&mpNetworkListManager);

            if(mpNetworkListManager && SUCCEEDED(hr))
            {
                // The following is a means to poll for Internet connectivity. However, the NetworkListManager
                // INetworkListManagerEvents system can be used to have the system tell you about it automatically.

                NLM_CONNECTIVITY connectivity;

                // See http://msdn.microsoft.com/en-us/library/aa370773%28v=VS.85%29.aspx

                
                HRESULT hr = mpNetworkListManager->GetConnectivity(&connectivity);

                if(SUCCEEDED(hr))
                {
                    pICS->mbConnectedToNetwork  = (connectivity != NLM_CONNECTIVITY_DISCONNECTED);

                    pICS->mbConnectedToInternet = (((int)connectivity & NLM_CONNECTIVITY_IPV4_INTERNET) ||
                                                   ((int)connectivity & NLM_CONNECTIVITY_IPV6_INTERNET));

                    // To consider: Does the NLM_CONNECTIVITY_IPV4_NOTRAFFIC flag indicate actual connectivity to a network?

                    // We have nothing to say about pICS->mbConnectedToURLs. That needs to be independently checked.
                }

            }
            else // Else use the older WinInet functionality.
            {
                // "How To Detecting If You Have a Connection to the Internet"
                // http://support.microsoft.com/kb/q242558/

                // Also there is the IsNetworkAlive function:
                // http://msdn.microsoft.com/en-us/library/aa377522%28VS.85%29.aspx

                // IsNetworkAlive pathway
                // DWORD flags = 0;
                // BOOL bResult = IsNetworkAlive(&flags); // Always call GetLastError before checking the return code of this function. If the last error is not 0, the IsNetworkAlive function has failed and the following TRUE and FALSE values do not apply.
                // DWORD dwLastError = GetLastError();
                // if(dwLastError != 0)
                //     bResult = FALSE;

                // InternetGetConnectedState pathway
                DWORD flags = 0;
                bool bConnected = (InternetGetConnectedState(&flags, 0) != 0);

                // InternetCheckConnection pathway
                /* 
                bool bConnected = (InternetCheckConnectionW(NULL, 0, 0) != 0); // WinInet call

                if(bConnected)    
                {      
                    // Do another check here as api sometimes returns false postives.
                    bool bTryForcedConnection = true; // To do: Make this an option for the user.

                    if(bTryForcedConnection)
                    {
                        // As of this writing (7/20/2010), this call is sometimes mistakenly failing on a connected computer.
                        // The reason for this is not yet known, but it is interfering with the functionality of this class.
                        // bConnected = (InternetCheckConnectionW(NULL, FLAG_ICC_FORCE_CONNECTION, 0) != 0); // WinInet call
                    }
                }
                */

                pICS->mbConnectedToNetwork  = bConnected;
                pICS->mbConnectedToInternet = bConnected;
            }

        if((bConnectedToNetworkSaved  != pICS->mbConnectedToNetwork) ||
           (bConnectedToInternetSaved != pICS->mbConnectedToInternet))
        {
            bChanged = true;
        }

        if (wasCoInitialized)
            CoUninitialize();

        #else

        bool isOnline = isOnlineHost("ea.com");
        
        pICS->mbConnectedToNetwork  = isOnline;
        pICS->mbConnectedToInternet = isOnline;

        if((bConnectedToNetworkSaved  != pICS->mbConnectedToNetwork) ||
           (bConnectedToInternetSaved != pICS->mbConnectedToInternet))
        {
            bChanged = true;
        }

        #endif
        
    }

    return bChanged;
}


bool ConnectionDetection::TestURLConnection(const String& sURL)
{
    // sURL is (protocol, host, port in the form of a URL like http://ea.com:1234/...)

    bool bCanConnect = false;
    NetworkAccessManager* accessManager = NetworkAccessManager::threadDefaultInstance();
    if (accessManager != NULL)
    {
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);

        QUrl url(QString(sURL.c_str()));
        QNetworkRequest request(url);
        QNetworkReply* reply = accessManager->get(request);

        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
        QObject::connect(&timer, SIGNAL(timeout()), &timer, SLOT(stop()));

        timer.start(5000);
        loop.exec();

        QObject::disconnect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        QObject::disconnect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
        QObject::disconnect(&timer, SIGNAL(timeout()), &timer, SLOT(stop()));

        bCanConnect = timer.isActive() && reply->error() == QNetworkReply::NoError;
        timer.stop();
        reply->deleteLater();
    }

    return bCanConnect;
}


bool ConnectionDetection::PollURLs(ConnectionStatus* pICS)
{
    // EA_ASSERT(pICS && mMutex.HasLock());
    bool bChanged = false;

    if(mbEnabled)
    {
        StringList connectedURLs(pICS->mConnectedURLs);
        StringList unconnectedURLs(pICS->mUnconnectedURLs);

        pICS->mConnectedURLs.clear();
        pICS->mUnconnectedURLs.clear();

        if(!mURLList.empty())
        {
            for(StringList::iterator it = mURLList.begin(); it != mURLList.end(); ++it)
            {
                const String& sURL = *it;
                const bool    bConnected = TestURLConnection(sURL);

                if(bConnected)
                    pICS->mConnectedURLs.push_back(sURL);
                else
                    pICS->mUnconnectedURLs.push_back(sURL);
            }
        }

        pICS->mbConnectedToURLs = !pICS->mConnectedURLs.empty();

        if((connectedURLs   != pICS->mConnectedURLs) ||
           (unconnectedURLs != pICS->mUnconnectedURLs))
        {
            bChanged = true;
        }
    }

    return bChanged;
}


bool ConnectionDetection::Poll(ConnectionStatus* pICS)
{
    bool bSuccess = true;

    if(mbEnabled)
    {
        if(!pICS)
            pICS = &mICS;

        pICS->mbChanged = false;

        if(!mbPollActive)
        {
            mbPollActive = true;

            mMutex.Lock();

            ConnectionStatus icsSaved = *pICS; // pICS is guaranteed to be non-NULL.

            // PollConnectivity asks the OS if it thinks we are connected to the Internet. 
            // It turns out that PollConnectivity, which on Windows uses Windows APIs to detect connectivity,  
            // sometimes returns false negatives (it says we aren't connected when we really are, such as through a VPN).
            // When PollConnectivity returns false, we want to poll some URLs in order to make sure it wasn't 
            // a false negative.
            PollConnectivity(pICS);

            if(!pICS->mbConnectedToInternet) // If PollConnedtivity thinks we are disconnected (which it might be mistaken about)...
            {
                PollURLs(pICS);

                if(pICS->mbConnectedToURLs) // If PollURLs thinks that we are connected...
                {
                    // Override PollConnectivity with our PollURLs results.
                    pICS->mbConnectedToNetwork  = true;
                    pICS->mbConnectedToInternet = true;
                }
            }
            // We don't call PollURLs, as we currently trust that PollConnectivity's determination that we 
            // are connected is correct. This policy may need to be modified per-platform.

            if((pICS->mbConnectedToInternet != icsSaved.mbConnectedToInternet) || 
               (pICS->mbConnectedToNetwork  != icsSaved.mbConnectedToNetwork) || 
               (pICS->mbConnectedToURLs     != icsSaved.mbConnectedToURLs))
            {
                pICS->mbChanged = true;
            }

            mMutex.Unlock();

            if(pICS->mbChanged && mpListener)
                mpListener->ConnectionChange(*pICS);

            mbPollActive = false;
        }
    }

    return bSuccess;
}


void ConnectionDetection::DebugSetConnectionStatus(ConnectionStatus& ics, bool bSetCurrent)
{
    mMutex.Lock();

    if(bSetCurrent)
        mICS = ics;

    if(ics.mbChanged && mpListener)
        mpListener->ConnectionChange(ics);

    mMutex.Unlock();
}


bool ConnectionDetection::BeginThread()
{
    //EA_ASSERT(mbThreadedPoll);

    mbThreadShouldQuit = false;
    mThread.Begin(this);

    return true;
}


bool ConnectionDetection::EndThread()
{
    //EA_ASSERT(mbPollThreadCreated);

    if(mbPollThreadCreated)
    {
        mbThreadShouldQuit = true;

        while(EA::Thread::Thread::kStatusEnded != mThread.WaitForEnd(20000))
        {
            EA::Thread::ThreadSleep(1000);
        }
    }

    return true;
}


// This is the thread function.
intptr_t ConnectionDetection::Run(void* /*pContext*/) 
{
    // To consider: We may have a problem here with millisecond time wrapping around after 22 days.
    // However, all that should do is cause the poll to not occur when expected once.

    EA::Thread::ThreadTime timeNow    = 0; // milliseconds
    EA::Thread::ThreadTime timeToPoll = 0;

    while(!mbThreadShouldQuit)
    {
        if(mbEnabled)
        {
            timeNow = EA::Thread::GetThreadTime();

            if(timeNow >= timeToPoll)
            {
                Poll(&mICS);

                timeToPoll = EA::Thread::GetThreadTime() + (mPollThreadTimeCheckSeconds * 1000);

                if(timeToPoll < timeNow) // If there was integer wraparound...
                    timeNow = timeToPoll;
            }
        }

        // Sleeping like this isn't very optimal and we'd rather have a more direct mechanism
        // for waiting for the event of quitting or the time of doing another poll.
        // To consider: Find a better way of doing this in the future.
        EA::Thread::ThreadSleep(1000);
    }

    return 0;
}

}

}

}
