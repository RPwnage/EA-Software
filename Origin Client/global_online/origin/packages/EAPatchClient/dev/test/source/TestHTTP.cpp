/////////////////////////////////////////////////////////////////////////////
// EAPatchClientTest/TestHTTP.cpp
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClientTest/Test.h>
#include <EAPatchClientTest/ValidationFile.h>
#include <EAPatchClientTest/ValidationFileHTTP.h>
#include <EAPatchClient/Client.h>
#include <EAPatchClient/HTTP.h>
#include <UTFInternet/HTTPServer.h>
#include <UTFInternet/HTTPServerPlugin.h>
#include <UTFInternet/Allocator.h>
#include <EAIO/EAFileUtil.h>
#include <EATest/EATest.h>
#include <EAStdC/EAEndian.h>
#include <EAStdC/EAStopwatch.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EARandom.h>
#include <EAStdC/EARandomDistribution.h>
#include <EAStdC/Int128_t.h>
#include <MemoryMan/MemoryMan.h>
#include <eathread/eathread_thread.h>


const char8_t*  kPatchHTTPTestIPAddress     = "127.0.0.1";          // We host our own HTTP server.
const uint16_t  kPatchHTTPTestPort          = 24565;                // The port our HTTP server runs on.
const char8_t*  kPatchHTTPTestBaseDirectory = "PatchHTTPTest/";     // Will be used like this: http://127.0.0.1:24565/PatchHTTPTest/SomeFile.fakeValidationFile.


///////////////////////////////////////////////////////////////////////////////
// Random
///////////////////////////////////////////////////////////////////////////////

/// RandomUint32Uniform
/// Return value is from 0 to nLimit-1 inclusively, with uniform probability.
template <typename Random>
uint64_t RandomUint64Uniform(Random& r, uint64_t nLimit)
{
    // Get value from [0, MAX_UINT64]
    const EA::StdC::uint128_t nRandNoLimit(((uint64_t)r.RandomUint32Uniform() << 32) | r.RandomUint32Uniform());

    // Scale the value from [0, MAX_UINT64] to a range of [0, nLimit).
    const EA::StdC::uint128_t nReturnValue((nRandNoLimit * nLimit) >> 64);

    return nReturnValue.AsUint64(); 
}

/// RandomUint64UniformRange
/// Return value is from nBegin to nEnd-1 inclusively, with uniform probability.
template <typename Random>
uint64_t RandomUint64UniformRange(Random& r, uint64_t nBegin, uint64_t nEnd)
{
    return nBegin + RandomUint64Uniform(r, nEnd - nBegin);
}




///////////////////////////////////////////////////////////////////////////////
// TestHTTP
///////////////////////////////////////////////////////////////////////////////

/// DownloadJobExtra
///
/// This is just a minor extension of HTTP's DownloadJob class, for the purpose
/// of us attaching a FakeValidationStream to it.
///
struct DownloadJobExtra : public EA::Patch::HTTP::DownloadJob
{
    EA::Patch::FakeValidationFile mFakeValidationStream; // This is "written to" during the download, and its purpose is solely to verify the "file" bytes are as expected. Nothing is actually written to memory or disk. 
};

typedef eastl::vector<DownloadJobExtra> DownloadJobExtraArray;



/// HTTPClientJob
///
/// This is for use by our HTTPClientThread. It defines the work that the test 
/// HTTP client thread needs to do. 
///
struct HTTPClientJob
{
    volatile bool    mbShouldQuit;
    volatile bool    mBusy;
    int              mErrorCount;
    EA::StdC::Random mRandom;


    HTTPClientJob() : mbShouldQuit(false), mBusy(true), mErrorCount(0), mRandom() {}
};


static void SetupHTTPClientJob(EA::StdC::Random& random, DownloadJobExtraArray& downloadJobArray, eastl_size_t jobCount = 0)
{
    EA::Patch::String sURLBase(EA::Patch::String::CtorSprintf(), "http://%s:%u/%s", kPatchHTTPTestIPAddress, (unsigned)kPatchHTTPTestPort, kPatchHTTPTestBaseDirectory);

    uint32_t kMinJobCount   = 20;
    uint32_t kMaxJobCount   = 30;
    uint64_t kMinFileSize32 =       0;
    uint64_t kMaxFileSize32 = 2000000;
    #if EAPATCHCLIENT_64_BIT_FILES_SUPPORTED
    uint64_t kMinFileSize64 = 1024 * 1024 * 1024 * UINT64_C(5);
    uint64_t kMaxFileSize64 = 1024 * 1024 * 1024 * UINT64_C(6);
    #endif
    uint64_t kMinRangeSize  =               1;
    uint64_t kMaxRangeSize  = 1024 * 1024 * 8;
    uint64_t fileSize;

    if(jobCount == 0)
        jobCount = (eastl_size_t)EA::StdC::RandomInt32UniformRange(random, kMinJobCount, kMaxJobCount);
    downloadJobArray.clear();           // Clearing, then resizing, will cause all 
    downloadJobArray.resize(jobCount);  // entries to re-initialize.

    for(eastl_size_t i = 0, iEnd = downloadJobArray.size(); i != iEnd; i++)
    {
        DownloadJobExtra& dje = downloadJobArray[i];

        fileSize = RandomUint64UniformRange(random, kMinFileSize32, kMaxFileSize32);

        #if EAPATCHCLIENT_64_BIT_FILES_SUPPORTED
            if(random.RandomUint32Uniform(20) == 0) // There's a 1 in N chance that we use a huge 64 bit file.
                fileSize = RandomUint64UniformRange(random, kMinFileSize64, kMaxFileSize64);
        #endif

        dje.mSourceURL.sprintf("%sTest_%I64u%s", sURLBase.c_str(), fileSize, kFakeValidationFileExtension);  // We make a URL like http://127.0.0.1:23456/PatchTest/Test_35600.fakeValidationFile
        dje.mFakeValidationStream.SetSize64(fileSize);
        dje.mDestPath.clear();
        dje.mpStream = &dje.mFakeValidationStream;

        if((fileSize > 0) && EA::StdC::RandomBool(random))
        {
            dje.mRangeBegin = RandomUint64UniformRange(random, 0, fileSize);

            // Need to pick an mRangeCount that makes mRangeBegin + mRangeCount be within the file size.
            uint64_t maxCount = (fileSize - dje.mRangeBegin) + 1; // +1 because we are generating a count and not an index.
            if(maxCount < kMinRangeSize)
               maxCount = kMinRangeSize;
            if(maxCount > kMaxRangeSize)
               maxCount = kMaxRangeSize;
            dje.mRangeCount = RandomUint64UniformRange(random, kMinRangeSize, maxCount); 

            dje.mFakeValidationStream.SetPosition((EA::IO::size_type)dje.mRangeBegin);
        }
        else
        {   // Download the entire file.
            dje.mRangeBegin = 0;
            dje.mRangeCount = 0;
        }
    }
}


/// HTTPClientThread
///
/// This is a thread which executes a HTTPClientJob. 
///
static intptr_t HTTPClientThread(void* pContext)
{
    HTTPClientJob* const  pHTTPClientJob = (HTTPClientJob*)pContext;
    EA::Patch::Server     httpServer;
    EA::Patch::HTTP       httpClient;
    bool                  bResult;
    int&                  nErrorCount = pHTTPClientJob->mErrorCount;
    EA::Patch::String     sURLBase(EA::Patch::String::CtorSprintf(), "http://%s:%u/%s", kPatchHTTPTestIPAddress, (unsigned)kPatchHTTPTestPort, kPatchHTTPTestBaseDirectory);
    DownloadJobExtraArray downloadJobArray;

    #if (EAPATCH_DEBUG >= 2)  // If we are debugging this app...
        pHTTPClientJob->mRandom.SetSeed(80808);
    #endif

    ///////////////////////////////////////////////////////////////////
    // Debug speed test. Not real test code.
    // On my machine the results below are that HTTP over 127.0.0.1 is 
    // about 580 megabits per second. A typical home internet connection
    // is about 10 megabites per second. However, small file transfers
    // will have significantly slower speed in terms of bits per second.
    //
    // EA::Patch::FakeValidationFile mFakeValidationStream;
    // mFakeValidationStream.AddRef();
    // EA::StdC::Stopwatch s(EA::StdC::Stopwatch::kUnitsSeconds, true);
    // bResult = httpClient.GetFileBlocking("http://127.0.0.1:24565/PatchTest/Test_36249992.fakeValidationFile", &mFakeValidationStream, 0, UINT64_MAX);
    //   or
    // bResult = httpClient.GetFileBlocking("http://127.0.0.1:24565/BigFile.txt", "C:\\Temp\\BigFile.txt.result", 0, UINT64_MAX);
    // EATEST_VERIFY(bResult);
    // uint64_t fileSize = EA::IO::File::GetSize("C:\\Temp\\BigFile.txt.result");
    // float    elapsedTimeSeconds = s.GetElapsedTimeFloat(); 
    // printf("%I64u %f %f, actual bytes per second: %f", fileSize, httpClient.GetNetworkReadDataRate() * 1000, elapsedTimeSeconds, fileSize / elapsedTimeSeconds);
    ///////////////////////////////////////////////////////////////////

    httpServer.SetPatchDirectoryURL(sURLBase.c_str()); // This is actually irrelevant for this test, at least as of this writing. The Server class is more for TLS/SSL than for setting base URLs.
    httpClient.SetServer(&httpServer);

    // Test HTTP::GetFileBlocking
    if(!pHTTPClientJob->mbShouldQuit)
    {
        SetupHTTPClientJob(pHTTPClientJob->mRandom, downloadJobArray, 1);

        DownloadJobExtra& dje = downloadJobArray[0];

        bResult = httpClient.GetFileBlocking(dje.mSourceURL.c_str(), dje.mpStream, dje.mRangeBegin, dje.mRangeCount);
        EATEST_VERIFY(bResult);
        EATEST_VERIFY(dje.mFakeValidationStream.GetState() == 0);

        dje.mFakeValidationStream.Reset(dje.mRangeBegin); // We'll use this again below, so Reset it.
    }

    if(httpClient.HasError())
    {
        EA::Patch::Error  error = httpClient.GetError();
        EA::Patch::String sError;
        error.GetErrorString(sError);
        EATEST_VERIFY_F(!httpClient.HasError(), "%s", sError.c_str());
    }

    for(eastl_size_t n = 0; (n < 4) && !nErrorCount; n++)
    {
        SetupHTTPClientJob(pHTTPClientJob->mRandom, downloadJobArray);

        // Test HTTP::GetFilesBlocking
        if(!pHTTPClientJob->mbShouldQuit) // If we weren't requested to quit by our owning thread (e.g. due to taking too long)...
        {
            // We need to pass an array of pointers to httpClient, and not objects, 
            // so we make an array of pointers here.
            EA::Patch::HTTP::DownloadJobPtrArray downloadJobPtrArray;

            for(eastl_size_t i = 0, iEnd = downloadJobArray.size(); i < iEnd; i++)
                downloadJobPtrArray.push_back(&downloadJobArray[i]);

            httpClient.Reset();
    
            // We use the following booleans to exercise various execution modes below.
            const bool bTestAsync      = (n >= 1);
            const bool bTestCancel     = (n >= 2);
            const bool bTestCancelWait = (n >= 3);

            if(bTestAsync)
            {
                bResult = httpClient.GetFilesAsync(downloadJobPtrArray.data(), downloadJobPtrArray.size());
                EATEST_VERIFY(bResult);

                for(int c = 0; !EA::Patch::IsAsyncOperationEnded(httpClient.GetAsyncStatus()); c++)
                {
                    EA::Thread::ThreadSleep((c == 0) ? 800 : 300);

                    if(bTestCancel)
                        httpClient.CancelRetrieval(bTestCancelWait); // This may result in CancelRetrieval executing multiple times, which we exercise here.
                }

                if(bTestCancel)
                {
                    // It's conceivably possible that we cancelled the download, but did it 
                    // just a bit too late and the download completed successfully first.
                    EATEST_VERIFY((httpClient.GetAsyncStatus() == EA::Patch::kAsyncStatusCancelled) || 
                                  (httpClient.GetAsyncStatus() == EA::Patch::kAsyncStatusCompleted));
                }
                else
                    EATEST_VERIFY(httpClient.GetAsyncStatus() == EA::Patch::kAsyncStatusCompleted);
            }
            else
            {
                bResult = httpClient.GetFilesBlocking(downloadJobPtrArray.data(), downloadJobPtrArray.size());
                EATEST_VERIFY(bResult);
            }

            // Do some testing of GetDownloadStats.
            EA::Patch::HTTP::DownloadStats stats;
            httpClient.GetDownloadStats(stats);

            if(!bTestCancel)
            {
                EATEST_VERIFY(stats.mTotalFileCount == downloadJobArray.size());
                EATEST_VERIFY(stats.mActiveFileCount == 0);
                EATEST_VERIFY(stats.mFailureFileCount == 0);
                EATEST_VERIFY(stats.mCompletedFileCount == stats.mTotalFileCount);
                EATEST_VERIFY(stats.mBytesDownloaded == stats.mBytesExpected);
                EATEST_VERIFY(stats.mBytesDownloaded != 0);
                EATEST_VERIFY(stats.mBytesExpected != UINT64_MAX);
            }

            // Verify IStream::GetState reports OK.
            for(eastl_size_t i = 0; i < downloadJobArray.size(); i++)
            {
                const DownloadJobExtra& dje = downloadJobArray[i];
                EATEST_VERIFY(dje.mFakeValidationStream.GetState() == 0);
            }
        }

        if(httpClient.HasError())
        {
            EA::Patch::Error  error = httpClient.GetError();
            EA::Patch::String sError;
            error.GetErrorString(sError);
            EATEST_VERIFY_F(!httpClient.HasError(), "%s", sError.c_str());
        }
    }

    pHTTPClientJob->mBusy = false;
    EAWriteBarrier();

    return 0; // The error count was set in pHTTPClientJob->mErrorCount. This is the thread return value, not the test result return value.
}


int TestHTTP()
{
    using namespace EA::Internet;

    int nErrorCount(0);

    // This test depends on a custom feature in our HTTPServer related to serving .fakeValidationFile files.
    // These are files which have a name like Test_36249992.fakeValidationFile. When you ask for such a file
    // from the server, it returns a file of (e.g.) 36249992 bytes. The files doesn't exist on disk, but rather
    // it is fabricated by the server with custom code we added to the server. With some effort we could add
    // this functionality to a web server like Apache, but it's not worth it for us here.

    HTTPServer* const pHTTPServer = EA_NEW("HTTPServer") HTTPServer;

    pHTTPServer->AddRef();
    pHTTPServer->SetPort(EA::StdC::ToBigEndian(kPatchHTTPTestPort));
    pHTTPServer->RegisterDataSource(FakeValidationFileFactory, NULL, "FakeValidationFileFactory", kFakeValidationFileExtensionPattern);

    if(pHTTPServer->Init())
    {
        //pHTTPServer->SetTraceDebugOutput(true);

        // Slow down the server so that we can do some timing tests in our HTTPClientThread test function.
        // Disabled until needed.
        //HTTPServer::Throttle httpServerThrottle;
        //httpServerThrottle.mWriteBytesPerSecond = 500000;
        //pHTTPServer->SetThrottle(httpServerThrottle);

        pHTTPServer->SetServerRootDirectory(L"C:\\Temp\\");  // This is just for a few debugging hacks we've added above and isn't needed by the real test code.

        if(pHTTPServer->Start())
        {
            #if (EAPATCH_DEBUG >= 2)
                const uint32_t kTimeoutSeconds    = 7200;
                const uint32_t kWaitForEndSeconds =   60;
            #else
                const uint32_t kTimeoutSeconds    =  360;
                const uint32_t kWaitForEndSeconds =   60;
            #endif

            HTTPClientJob            job;
            EA::Thread::Thread       jobThread;
            EA::StdC::LimitStopwatch limitStopwatch(EA::StdC::Stopwatch::kUnitsSeconds, kTimeoutSeconds, true);

            EA::Thread::ThreadParameters params;
            params.mpName = "EAPatchTestHTTP";
            jobThread.Begin(HTTPClientThread, &job, &params);

            do{
                EA::UnitTest::ThreadSleepRandom(100, 500);
            } while(job.mBusy && !limitStopwatch.IsTimeUp()); // Quit whichever comes first.    

            EATEST_VERIFY_MSG(!limitStopwatch.IsTimeUp(), "   HTTP test seems to have not finished before timing out.\n");

            job.mbShouldQuit = true;
            jobThread.WaitForEnd(EA::Thread::ThreadTime(EA::Thread::GetThreadTime() + (kWaitForEndSeconds * 1000)));

            EATEST_VERIFY(!job.mBusy); // Verify that the job finished before the time was up.
            EATEST_VERIFY(job.mErrorCount == 0);

            pHTTPServer->Stop();
        }
        else
        {
            nErrorCount++;
            EA::UnitTest::Report("   HTTP server start failure. Is there already a server running on port %u?\n", (unsigned)kPatchHTTPTestPort);
        }

        pHTTPServer->Shutdown();
    }
    else
    {
        nErrorCount++;
        EA::UnitTest::Report("   HTTP server init failure.\n");
    }

    pHTTPServer->Release();

    return nErrorCount;
}
















