/////////////////////////////////////////////////////////////////////////////
// EAPatchClientTest/TestPatchDirectory.cpp
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClientTest/Test.h>
#include <EAPatchClient/Client.h>
#include <EAPatchClient/PatchDirectory.h>
#include <UTFInternet/HTTPServer.h>
#include <UTFInternet/Allocator.h>
#include <EATest/EATest.h>
#include <EAStdC/EAStopwatch.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EAEndian.h>
#include <MemoryMan/MemoryMan.h>
#include <EAIO/PathString.h>
#include <EAIO/EAFileUtil.h>
#include <eathread/eathread_thread.h>


// The values here can't be changed without changing the data in our test/data directory. The .eaPatchImple files found there,
// for example, have the IP address and port below written in them. So if you want to change the address or port here then
// you need to change the data files to match your changes.
static const char8_t*  kPatchDirectoryTestAddress   = "127.0.0.1";
static const uint16_t  kPatchDirectoryTestPort      = 24564;
static const char8_t*  kPatchDirectoryTestDirectory = "PatchDirectoryTest";

#if (EAPATCH_DEBUG >= 2)
    static const uint32_t kTimeoutSeconds    = 7200;
    static const uint32_t kWaitForEndSeconds =   60;
#else
    static const uint32_t kTimeoutSeconds    =   20;
    static const uint32_t kWaitForEndSeconds = 7200;
#endif


///////////////////////////////////////////////////////////////////////////////
// TestDirectoryRetrieverEventCallback
///////////////////////////////////////////////////////////////////////////////

class TestDirectoryRetrieverEventCallback : public EA::Patch::PatchDirectoryEventCallback
{
public:
    void EAPatchStart              (EA::Patch::PatchDirectoryRetriever* pPatchDirectoryRetriever, intptr_t userContext);
    void EAPatchProgress           (EA::Patch::PatchDirectoryRetriever* pPatchDirectoryRetriever, intptr_t userContext, double progress);
    void EAPatchNetworkAvailability(EA::Patch::PatchDirectoryRetriever* pPatchDirectoryRetriever, intptr_t userContext, bool available);
    void EAPatchError              (EA::Patch::PatchDirectoryRetriever* pPatchDirectoryRetriever, intptr_t userContext, EA::Patch::Error& error);
    void EAPatchNewState           (EA::Patch::PatchDirectoryRetriever* pPatchDirectoryRetriever, intptr_t userContext, int newState);
    void EAPatchBeginFileDownload  (EA::Patch::PatchDirectoryRetriever* pPatchDirectoryRetriever, intptr_t userContext, const char8_t* pFilePath, const char8_t* pFileURL);
    void EAPatchEndFileDownload    (EA::Patch::PatchDirectoryRetriever* pPatchDirectoryRetriever, intptr_t userContext, const char8_t* pFilePath, const char8_t* pFileURL);
    void EAPatchStop               (EA::Patch::PatchDirectoryRetriever* pPatchDirectoryRetriever, intptr_t userContext, EA::Patch::AsyncStatus status);
};


void TestDirectoryRetrieverEventCallback::EAPatchStart(EA::Patch::PatchDirectoryRetriever*, intptr_t)
{
    if(gbTraceDirectoryRetrieverEvents)
        EA::UnitTest::Report("Directory retriever start\n");
}

void TestDirectoryRetrieverEventCallback::EAPatchProgress(EA::Patch::PatchDirectoryRetriever*, intptr_t, double progress)
{
    if(gbTraceDirectoryRetrieverEvents && (EA::UnitTest::GetVerbosity() >= 1))
        EA::UnitTest::Report("Progress: %2.0f%%\n", progress * 100);
}

void TestDirectoryRetrieverEventCallback::EAPatchNetworkAvailability (EA::Patch::PatchDirectoryRetriever*, intptr_t, bool available)
{
    if(gbTraceDirectoryRetrieverEvents)
        EA::UnitTest::Report("Network availability change: now %s\n", available ? "available" : "unavailable");
}

void TestDirectoryRetrieverEventCallback::EAPatchError(EA::Patch::PatchDirectoryRetriever*, intptr_t, EA::Patch::Error& error)
{
    if(gbTraceDirectoryRetrieverEvents)
        EA::UnitTest::Report("Error: %s\n", error.GetErrorString());
}

void TestDirectoryRetrieverEventCallback::EAPatchNewState(EA::Patch::PatchDirectoryRetriever*, intptr_t, int newState)
{
    if(gbTraceDirectoryRetrieverEvents && (EA::UnitTest::GetVerbosity() >= 2))
        EA::UnitTest::Report("State: %s\n", EA::Patch::PatchDirectoryRetriever::GetStateString((EA::Patch::PatchDirectoryRetriever::State)newState));
}

void TestDirectoryRetrieverEventCallback::EAPatchBeginFileDownload(EA::Patch::PatchDirectoryRetriever*, intptr_t, const char8_t* pFilePath, const char8_t* pFileURL)
{
    if(gbTraceDirectoryRetrieverEvents && (EA::UnitTest::GetVerbosity() >= 1))
        EA::UnitTest::Report("Begin file download:\n   %s ->\n   %s\n", pFilePath, pFileURL);
}

void TestDirectoryRetrieverEventCallback::EAPatchEndFileDownload(EA::Patch::PatchDirectoryRetriever*, intptr_t, const char8_t* pFilePath, const char8_t* pFileURL)
{
    if(gbTraceDirectoryRetrieverEvents && (EA::UnitTest::GetVerbosity() >= 1))
        EA::UnitTest::Report("End file download:\n   %s ->\n   %s\n", pFilePath, pFileURL);
}

void TestDirectoryRetrieverEventCallback::EAPatchStop(EA::Patch::PatchDirectoryRetriever*, intptr_t, EA::Patch::AsyncStatus status)
{
    if(gbTraceDirectoryRetrieverEvents)
        EA::UnitTest::Report("Directory retriever stopped with state %s\n", GetAsyncStatusString(status));
}



///////////////////////////////////////////////////////////////////////////////
// TestDirectoryTelemetryEventCallback
///////////////////////////////////////////////////////////////////////////////

class TestDirectoryTelemetryEventCallback : public EA::Patch::TelemetryEventCallback
{
public:
    virtual void TelemetryEvent(intptr_t userContext, EA::Patch::TelemetryPatchSystemInit&);
    virtual void TelemetryEvent(intptr_t userContext, EA::Patch::TelemetryPatchSystemShutdown&);
    virtual void TelemetryEvent(intptr_t userContext, EA::Patch::TelemetryDirectoryRetrievalBegin&);
    virtual void TelemetryEvent(intptr_t userContext, EA::Patch::TelemetryDirectoryRetrievalEnd&);
    virtual void TelemetryEvent(intptr_t userContext, EA::Patch::TelemetryPatchBuildBegin&);
    virtual void TelemetryEvent(intptr_t userContext, EA::Patch::TelemetryPatchBuildEnd&);
    virtual void TelemetryEvent(intptr_t userContext, EA::Patch::TelemetryPatchCancelBegin&);
    virtual void TelemetryEvent(intptr_t userContext, EA::Patch::TelemetryPatchCancelEnd&);
};

void TestDirectoryTelemetryEventCallback::TelemetryEvent(intptr_t, EA::Patch::TelemetryPatchSystemInit& tpsi)
{
    if(gbTraceDirectoryRetrieverTelemetry)
    {
        EA::UnitTest::Report("TelemetryPatchSystemInit:\n");
        EA::UnitTest::Report("   Date: %s\n", tpsi.mDate.c_str());
    }
}

void TestDirectoryTelemetryEventCallback::TelemetryEvent(intptr_t, EA::Patch::TelemetryPatchSystemShutdown& tpss)
{
    if(gbTraceDirectoryRetrieverTelemetry)
    {
        EA::UnitTest::Report("TelemetryPatchSystemShutdown:\n");
        EA::UnitTest::Report("   Date: %s\n", tpss.mDate.c_str());
    }
}

void TestDirectoryTelemetryEventCallback::TelemetryEvent(intptr_t, EA::Patch::TelemetryDirectoryRetrievalBegin& tdrb)
{
    if(gbTraceDirectoryRetrieverTelemetry)
    {
        EA::UnitTest::Report("TelemetryDirectoryRetrievalBegin:\n");
        EA::UnitTest::Report("   Date: %s\n", tdrb.mDate.c_str());
        EA::UnitTest::Report("   URL:  %s\n", tdrb.mPatchDirectoryURL.c_str());
    }
}

void TestDirectoryTelemetryEventCallback::TelemetryEvent(intptr_t, EA::Patch::TelemetryDirectoryRetrievalEnd& tdre)
{
    if(gbTraceDirectoryRetrieverTelemetry)
    {
        EA::UnitTest::Report("TelemetryDirectoryRetrievalEnd:\n");
        EA::UnitTest::Report("   Date:              %s\n", tdre.mDate.c_str());
        EA::UnitTest::Report("   URL:               %s\n", tdre.mPatchDirectoryURL.c_str());
        EA::UnitTest::Report("   Status:            %s\n", tdre.mStatus.c_str());
        EA::UnitTest::Report("   Dir download vol:  %s\n", tdre.mDirDownloadVolume.c_str());
        EA::UnitTest::Report("   Info download vol: %s\n", tdre.mInfoDownloadVolume.c_str());
    }
}


void TestDirectoryTelemetryEventCallback::TelemetryEvent(intptr_t, EA::Patch::TelemetryPatchBuildBegin& /*tpbb*/)
{
    // Never called in this test
}

void TestDirectoryTelemetryEventCallback::TelemetryEvent(intptr_t, EA::Patch::TelemetryPatchBuildEnd& /*tpbe*/)
{
    // Never called in this test
}


void TestDirectoryTelemetryEventCallback::TelemetryEvent(intptr_t, EA::Patch::TelemetryPatchCancelBegin& /*tpcb*/)
{
    // Never called in this test
}

void TestDirectoryTelemetryEventCallback::TelemetryEvent(intptr_t, EA::Patch::TelemetryPatchCancelEnd& /*tpce*/)
{
    // Never called in this test
}





/// TestPatchDirectoryJob
///
/// This is for use by our TestPatchDirectoryThread. 
///
struct TestPatchDirectoryJob
{
    volatile bool     mbDone;
    int               mErrorCount;
    EA::Patch::String mHTTPServerAddress;
    uint16_t          mHTTPServerPort;

    TestPatchDirectoryJob() : mbDone(false), mErrorCount(0), mHTTPServerAddress(), mHTTPServerPort(0) {}
};


/// TestPatchDirectoryThread
///
/// This is a thread which executes a HTTPClientJob. 
///
static intptr_t TestPatchDirectoryThread(void* pContext)
{
    using namespace  EA::Patch;

    TestPatchDirectoryJob*  pJob = (TestPatchDirectoryJob*)pContext;
    bool                    bResult;
    AsyncStatus             asyncStatus;
    int&                    nErrorCount = pJob->mErrorCount; // sPatchDirectoryURL looks like this: http://127.0.0.1:24564/PatchDirectoryTest/test%201/
    String                  sPatchDirectoryURL(EA::Patch::String::CtorSprintf(), "http://%s:%u/%s/test%201/test%%201%s", pJob->mHTTPServerAddress.c_str(), (unsigned)pJob->mHTTPServerPort, kPatchDirectoryTestDirectory, kPatchDirectoryFileNameExtension);
    Server                  httpServer; // Usually the global server is used, but we create a local one for this test.
    PatchDirectoryRetriever pdr;
    PatchDirectory          patchDirectory;

    // User applications don't need to call Server::SetServerURLBase unless they are using 
    // relative directories in the .eaPatchInfo and .eaPatchImpl files.
    const String sServerURLBase(EA::Patch::String::CtorSprintf(), "http://%s:%u/", pJob->mHTTPServerAddress.c_str(), (unsigned)pJob->mHTTPServerPort);
    httpServer.SetURLBase(sServerURLBase.c_str());

    // Currently PatchDirectoryRetriever always gets the patch directory URL from the Server class. We might add a 
    // SetPatchDirectoryURL function to PatchDirectoryRetriever, though in practice that would be more useful for the 
    // unit test than the actual runtime, in which directories and SSL are managed by the Server class.
    httpServer.SetPatchDirectoryURL(sPatchDirectoryURL.c_str());
    pdr.SetServer(&httpServer);

    TestDirectoryRetrieverEventCallback eventCallback;
    pdr.RegisterPatchDirectoryEventCallback(&eventCallback, 0);

    TestDirectoryTelemetryEventCallback telemetryCallback;
    pdr.RegisterTelemetryEventCallback(&telemetryCallback, 0);

    {  // RetrievePatchDirectoryBlocking
        bResult = pdr.RetrievePatchDirectoryBlocking();
        EATEST_VERIFY(bResult);
        
        if(bResult)
        {
            bResult = pdr.GetPatchDirectory(patchDirectory);
            EATEST_VERIFY(bResult);

            PatchInfoArray& patchInfoArray = patchDirectory.GetPatchInfoArray();
            for(eastl_size_t i = 0; i < patchInfoArray.size(); i++)
            {
                const PatchInfo& patchInfo = patchInfoArray[i];

                EATEST_VERIFY(!patchInfo.HasError());
                // Possible do other tests.
            }

            EATEST_VERIFY(!pdr.HasError());
        }
    }

    { // RetrievePatchDirectoryAsync
        bResult = pdr.RetrievePatchDirectoryAsync();
        EATEST_VERIFY(bResult);
        
        if(bResult)
        {
            do
            {
                EA::UnitTest::ThreadSleepRandom(100, 500);
                asyncStatus = pdr.GetAsyncStatus();
            }while(IsAsyncOperationInProgress(asyncStatus));

            EATEST_VERIFY(asyncStatus == kAsyncStatusCompleted);

            if(asyncStatus == kAsyncStatusCompleted)
            {
                bResult = pdr.GetPatchDirectory(patchDirectory);
                EATEST_VERIFY(bResult);

                PatchInfoArray& patchInfoArray = patchDirectory.GetPatchInfoArray();
                for(eastl_size_t i = 0; i < patchInfoArray.size(); i++)
                {
                    const PatchInfo& patchInfo = patchInfoArray[i];

                    EATEST_VERIFY(!patchInfo.HasError());
                    // Possible do other tests.
                }

            }

            EATEST_VERIFY(!pdr.HasError());
        }
    }

    // Cancellation test
    { // RetrievePatchDirectoryAsync
        bResult = pdr.RetrievePatchDirectoryAsync();
        EATEST_VERIFY(bResult);
        
        if(bResult)
        {
            // To do: Use the event notification callback issue the cancel while in the middle of one, instead of relying on timing here.
            EA::UnitTest::ThreadSleepRandom(1, 3);
            pdr.CancelRetrieval(true);
            // Is there anything we can test here?
            EATEST_VERIFY(!pdr.HasError());
        }
    }

    pJob->mbDone = true;
    EAWriteBarrier(); // Make sure the mbDone assignment becomes visible to other threads.

    return 0; // The error count was set in pJob->mErrorCount. This is the thread return value, not the test result return value.
}



///////////////////////////////////////////////////////////////////////////////
// TestPatchDirectory
///////////////////////////////////////////////////////////////////////////////

int TestPatchDirectory()
{
    using namespace EA::Internet;

    int nErrorCount(0);
    
    const bool bDataDirectoryExists = EA::IO::Directory::Exists(gDataDirectory.c_str());

    if(bDataDirectoryExists)
    {
        EA::Patch::String sHTTPServerAddress = kPatchDirectoryTestAddress;
        uint16_t          httpServerPort     = kPatchDirectoryTestPort;

        if(!gExternalHTTPServerAddress.empty())
        {
            sHTTPServerAddress = gExternalHTTPServerAddress;
            httpServerPort     = gExternalHTTPServerPort;
        }

        HTTPServer* const pHTTPServer = EA_NEW("HTTPServer") HTTPServer;

        pHTTPServer->AddRef();
        pHTTPServer->SetPort(EA::StdC::ToBigEndian(kPatchDirectoryTestPort)); // It's not really possible for us to tell the server to run at sHTTPServerAddress, unless sHTTPServerAddress happens to be 127.0.0.1 or equivalent.
        pHTTPServer->SetServerRootDirectory(gDataDirectory.c_str());

        if(pHTTPServer->Init())
        {
            //pHTTPServer->SetTraceDebugOutput(true);
            //#if !defined(EA_PLATFORM_DESKTOP) // This is for debugging.
            //    gHTTPServerThrottle.mReadBytesPerSecond  = 65536;
            //    gHTTPServerThrottle.mWriteBytesPerSecond = 65536;
            //#endif
            pHTTPServer->SetThrottle(gHTTPServerThrottle);

            if(pHTTPServer->Start()) // To do: Don't start this HTTP server if gExternalHTTPServerAddress was specified.
            {
                EA::Thread::Thread       thread;
                EA::StdC::LimitStopwatch limitStopwatch(EA::StdC::Stopwatch::kUnitsSeconds, kTimeoutSeconds, true);
                TestPatchDirectoryJob    job;

                job.mHTTPServerAddress = sHTTPServerAddress;
                job.mHTTPServerPort    = httpServerPort;

                EA::Thread::ThreadParameters params;
                params.mpName      = "EAPatchTestDir";
                params.mnStackSize = 256 * 1024; // The thread may require more than 64K of stack space.
                thread.Begin(TestPatchDirectoryThread, &job, &params);

                do{
                    EA::UnitTest::ThreadSleepRandom(100, 500);
                    EAReadBarrier(); // Make sure a mbDone assignment becomes visible from other threads.
                } while(!job.mbDone && !limitStopwatch.IsTimeUp()); // Quit whichever comes first.    

                thread.WaitForEnd(EA::Thread::ThreadTime(EA::Thread::GetThreadTime() + (kWaitForEndSeconds * 1000)));

                EATEST_VERIFY(job.mErrorCount == 0);

                pHTTPServer->Stop();
            }
            else
            {
                nErrorCount++;
                EA::UnitTest::Report("   HTTP server start failure. Is there already a server running on port %u?\n", (unsigned)kPatchDirectoryTestPort);
            }

            pHTTPServer->Shutdown();
        }
        else
        {
            nErrorCount++;
            EA::UnitTest::Report("   HTTP server init failure.\n");
        }

        pHTTPServer->Release();
    }
    else
        EA::UnitTest::Report("   Skipping test due to no data directory.\n");

    return nErrorCount;
}






