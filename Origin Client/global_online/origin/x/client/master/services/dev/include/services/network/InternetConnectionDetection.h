///////////////////////////////////////////////////////////////////////////////
// ConnectionDetection.h
// 
// Created by Paul Pedriana
// Copyright (c) 2010 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// ConnectionDetection
//
// Provides basic services for telling if the Internet or some specific site
// on the Internet is available.
///////////////////////////////////////////////////////////////////////////////


#ifndef INTERNETCONNECTIONDETECTION_H
#define INTERNETCONNECTIONDETECTION_H


#include <limits>
#include <QDateTime>
#include <QObject>
#include <QString>
#include <QTime>
#include <QReadWriteLock>
#include <QSharedPointer>


#include <EAThread/eathread_thread.h>
#include <EAThread/eathread_mutex.h>
#include <list>
#include <string>

#include "services/plugin/PluginAPI.h"

///////////////////////////////////////////////////////////////////////////////
// UTFSOCKETS_NETLISTMGR_AVAILABLE
//
// Defined as 0 or 1, with the default being whether it was detected.
//
#ifndef UTFSOCKETS_NETLISTMGR_AVAILABLE
    #if defined(EA_PLATFORM_WINDOWS)
        #include <ntverp.h> // Windows SDK version identification header.

        #if (defined(VER_PRODUCTBUILD) && (VER_PRODUCTBUILD >= 6001))
            #define UTFSOCKETS_NETLISTMGR_AVAILABLE 1
        #endif
    #endif

    #ifndef UTFSOCKETS_NETLISTMGR_AVAILABLE
        #define UTFSOCKETS_NETLISTMGR_AVAILABLE 0
    #endif
#endif



#if defined(_WIN32)
    #pragma warning(push, 0)
    #include <windows.h>
    #include <netlistmgr.h> // This may fail to compile unless you have a recent Windows SDK.
    #pragma warning(pop)
#endif



namespace Origin
{

namespace Services
{

namespace Network
{
        typedef std::string       String;
        typedef std::list<String> StringList;


        struct ConnectionStatus
        {
            ORIGIN_PLUGIN_API ConnectionStatus();

            bool        mbChanged;                  // True if changed since last time the message was broadcast.
            bool        mbConnectedToNetwork;       // True if connected to any network, but not necessarily the Internet.
            bool        mbConnectedToInternet;      // True if connected to the Internet, but not necessarily to EA.
            bool        mbConnectedToURLs;          // True if connected to any of the test URLs.
            StringList  mConnectedURLs;             // List of URLs we can connect to.
            StringList  mUnconnectedURLs;           // List of URLs we can't connect to.
        };


        // IConnectionListener
        //
        // An instance of this class is optionally provided by the application in order to
        // asynchronously receive XMLSocket read events. The Application can alternatively
        // subclass the XMLSocket class and override its OnClose, OnConnect, etc. functions.

        class ORIGIN_PLUGIN_API IConnectionListener
        {
        public:
            virtual void ConnectionChange(const ConnectionStatus& ics) = 0;
        };



        // ConnectionDetection
        //
        // Example usage:
        //     // Threaded
        //     void main() {
        //         ConnectionDetection icd;
        //     
        //         icd.SetListener(pSomeListener);
        //         icd.Init(true, 60000);
        //         ...
        //         icd.Shutdown();
        //     }
        //
        //     // Polled
        //     void main() {
        //         ConnectionDetection icd;
        //     
        //         icd.SetListener(pSomeListener);
        //         icd.Init();
        //
        //         if(icd.PollingRequired())
        //         {
        //             // An alternative to the while loop below is to call StartAutoPoll.
        //             while(keepRunningApp)
        //             {
        //                 ...
        //                 icd.Poll();
        //             }
        //         }
        //         
        //         icd.Shutdown();
        //     }

        class ORIGIN_PLUGIN_API ConnectionDetection : public EA::Thread::IRunnable
            #if defined(_WIN32)
                , public INetworkListManagerEvents
            #endif
        {
        public:

            static void init();
            static void release();
            static QSharedPointer<ConnectionDetection> instance();

            virtual ~ConnectionDetection();

            virtual IConnectionListener* GetListener();
            virtual void SetListener(IConnectionListener* pListener);

            // Enables disconnection detection. When disabled it will act as if we are always connected.
            bool GetEnabled() const;
            void SetEnabled(bool enabled);

            // Adds a URL to check for connectivity on top of regular connectivity checks.
            // Can be called multiple times to add multiple URLs.
            // pURL is (protocol, host [, port] in the form of a URL, such as http://ea.com:1234/...
            virtual void AddURL(const char* pURL);
            virtual void RemoveURL(const char* pURL);

            virtual bool Init(); 
            virtual bool Shutdown();

            // Returns true if polling is required. You should call this after calling Init to tell 
            // if you need to poll instead of have the system automatically push connection events to you.
            // This is required because we can only tell at runtime if the system supports this.
            virtual bool PollingRequired() const;

            // This is an alternative to the manual Poll function.
            // This function creates a background task which auto-polls for connectivity.
            virtual bool StartAutoPoll(int pollThreadTimeCheckSeconds);

            // If bThreadedPoll is false, then you need to call Poll to tell what the ConnectionStatus is.
            // This function might not return immediately, so it might need to be called from a separate thread.
            // Returns true if the poll was able to complete. If you want to know if there was a connection
            // status change, look at the resulting pICS.
            virtual bool Poll(ConnectionStatus* pICS = NULL);

            // Returns the result from the last Poll.
            virtual ConnectionStatus& GetLastConnectionStatus() { return mICS; }
                
            // Pretends that the connection status has changed to the given status.
            virtual void DebugSetConnectionStatus(ConnectionStatus& ics, bool bSetCurrent);

        protected:
            virtual bool TestURLConnection(const String& sURL);

            virtual bool PollConnectivity(ConnectionStatus* pICS);
            virtual bool PollURLs(ConnectionStatus* pICS);

            virtual bool     BeginThread();         // Called from the main application thread.
            virtual bool     EndThread();           // Called from the main application thread.
            virtual intptr_t Run(void* pContext);   // EAThread run function.

            #if defined(_WIN32)
                HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
                ULONG   STDMETHODCALLTYPE AddRef();
                ULONG   STDMETHODCALLTYPE Release();
                HRESULT STDMETHODCALLTYPE ConnectivityChanged(NLM_CONNECTIVITY newConnectivity);
            #endif

        protected:

            ConnectionDetection();

            // Declare, but don't define. These are simply here to prevent compiler warnings.
            ConnectionDetection(const ConnectionDetection&);
            ConnectionDetection& operator=(const ConnectionDetection&);

        protected:

            bool                        mbEnabled;                  // 
            bool                        mbPollActive;               // Flag that prevents Poll from being called recursively.
            bool                        mbPollingRequired;          // True if the system doesn't natively support automatic connectivity detection.
            bool                        mbPollThreadCreated;        // True if a polling thread exists.
            volatile bool               mbThreadShouldQuit;         // Used as a message to our polling thread.
            int                         mPollThreadTimeCheckSeconds;// Time between polls by the thread. See class constructor for the default value.
            EA::Thread::Thread          mThread;                    // 
            EA::Thread::Mutex           mMutex;                     // 
            ConnectionStatus            mICS;                       // 
            IConnectionListener*        mpListener;                 // 
            StringList                  mURLList;                   // List of URLs to sites we should connect to (protocol, host, port in the form of a URL like http://ea.com:1234)

            #if defined(_WIN32)
                INetworkListManager*       mpNetworkListManager;       // Will be NULL in the case of Windows versions prior to Vista
                IConnectionPointContainer* mpConnectionPointContainer; // 
                IConnectionPoint*          mpConnectionPoint;          // 
                DWORD                      mCookie;                    // 
                volatile ULONG             mRefCount;                  // 
            #endif
        };

}

}

}

#endif // Header include guard
