/////////////////////////////////////////////////////////////////////////////
// EAPatchClientTest/TestPatchBuilder.cpp
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Our tests are largely data-driven, and are based on the data in the 
// EAPatchClient\<ver>\test\data\PatchBuilderTest directory. Within this 
// directories are one or more subdirectories, one for each test we run here.
// Within each of those test directories are mock patch images that allow
// us to implement the data-driven test. The directories present are:
//     local       - The local directory is a directory that is to be patched. 
//                   It has files that are named the same as the patch directory, 
//                   but whose contents are in fact different.
//     patchSource - The patchSource directory is what the patch 
//                   implementation is built from.
//     patch       - The patch directory is the patch itself, built from patchSource.
//                   From the patch directory we run a web server which acts as our 
//                   patching test server.
//     expected    - The expected directory is the local directory should look 
//                   like after it's patched. We can verify that the patching process
//                   correctly applied the patch by comparing the expected directory
//                   with the patched local directory.
// In practice we don't modify the local directory in place at runtime but rather
// copy it to a temp location for the execution of the test. We do this only because
// we want to be able to keep that local directory as-is for future test runs.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClientTest/Test.h>
#include <EAPatchClient/Client.h>
#include <EAPatchClient/URL.h>
#include <EAPatchClient/PatchBuilder.h>
#include <UTFInternet/HTTPServer.h>
#include <UTFInternet/Allocator.h>
#include <EATest/EATest.h>
#include <EAStdC/EAStopwatch.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EAEndian.h>
#include <MemoryMan/MemoryMan.h>
#include <EAIO/PathString.h>
#include <EAIO/EAFileDirectory.h>
#include <EAIO/EAFileUtil.h>
#include <eathread/eathread_thread.h>


// The values here can't be changed without changing the data in our test/data directory. The .eaPatchImple files found there,
// for example, have the IP address and port below written in them. So if you want to change the address or port here then
// you need to change the data files to match your changes.
static const char8_t*  kPatchBuilderTestAddress   = "127.0.0.1";
static const uint16_t  kPatchBuilderTestPort      = 24564;
static const char8_t*  kPatchBuilderTestDirectory = "PatchBuilderTest"; // We'll testing patch URLs such as http://127.0.0.1:24564/PatchBuilderTest/test1/EAPatchBuilder%20test1.eaPatchImpl


#if (EAPATCH_DEBUG >= 2)
    static const uint32_t kTimeoutSeconds    = 7200;
    static const uint32_t kWaitForEndSeconds =   60;
#else
    static const uint32_t kTimeoutSeconds    =   20;
    static const uint32_t kWaitForEndSeconds = 7200;
#endif


// Builds a list of directories which have build test data in them.
// These are dierctories whose name begins with "test", such as "test1".
// If you look at the EAPatchClient test data directory, it will be fairly obvious what's going on here.
static void GetPatchBuilderTestDirectoryList(PathList& directoryPathList, PathList& patchInfoFilePathList)
{
    using namespace EA::IO;

    if(!gDataDirectory.empty())
    {
        Path::PathStringW testDirectoryPath = gDataDirectory;
        testDirectoryPath += L"PatchBuilderTest";
        testDirectoryPath += EA_FILE_PATH_SEPARATOR_W;

        DirectoryIterator            di;
        DirectoryIterator::EntryList entryList;

        di.Read(testDirectoryPath.c_str(), entryList, EA_WCHAR("test*"), kDirectoryEntryDirectory, 20, false);

        for(DirectoryIterator::EntryList::iterator it = entryList.begin(); it != entryList.end(); ++it)
        {
            DirectoryIterator::Entry& entry = *it;

            // Record the directory path
            entry.msName.insert(0, testDirectoryPath.c_str());
            directoryPathList.push_back(entry.msName.c_str());

            // Record the path to the .eaPatchImpl file
            entry.msName += L"patch";
            entry.msName += EA_FILE_PATH_SEPARATOR_W;

            DirectoryIterator::EntryList entryListImpl; // Find the .eaPatchImpl file in this dir.
            di.Read(entry.msName.c_str(), entryListImpl, EA_WCHAR("*.eaPatchInfo"), kDirectoryEntryFile, 1, false);

            if(!entryListImpl.empty())
            {
                entry.msName += entryListImpl.front().msName;
                patchInfoFilePathList.push_back(entry.msName.c_str());
            }
            else
                directoryPathList.pop_back();
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
// TestPatchBuilderEventCallback
///////////////////////////////////////////////////////////////////////////////

class TestPatchBuilderEventCallback : public EA::Patch::PatchBuilderEventCallback
{
public:
    void EAPatchStart                (EA::Patch::PatchBuilder* pPatchBuilder, intptr_t userContext);
    void EAPatchProgress             (EA::Patch::PatchBuilder* pPatchBuilder, intptr_t userContext, double patchProgress);
    void EAPatchRetryableNetworkError(EA::Patch::PatchBuilder* pPatchBuilder, intptr_t userContext, const char8_t* pURL);
    void EAPatchNetworkAvailability  (EA::Patch::PatchBuilder* pPatchBuilder, intptr_t userContext, bool available);
    void EAPatchError                (EA::Patch::PatchBuilder* pPatchBuilder, intptr_t userContext, EA::Patch::Error& error);
    void EAPatchNewState             (EA::Patch::PatchBuilder* pPatchBuilder, intptr_t userContext, int newState);
    void EAPatchBeginFileDownload    (EA::Patch::PatchBuilder* pPatchBuilder, intptr_t userContext, const char8_t* pFilePath, const char8_t* pFileURL);
    void EAPatchEndFileDownload      (EA::Patch::PatchBuilder* pPatchBuilder, intptr_t userContext, const char8_t* pFilePath, const char8_t* pFileURL);
    void EAPatchRenameFile           (EA::Patch::PatchBuilder* pPatchBuilder, intptr_t userContext, const char8_t* pPrevFilePath, const char8_t* pNewFilePath);
    void EAPatchDeleteFile           (EA::Patch::PatchBuilder* pPatchBuilder, intptr_t userContext, const char8_t* pFilePath);
    void EAPatchRenameDirectory      (EA::Patch::PatchBuilder* pPatchBuilder, intptr_t userContext, const char8_t* pPrevDirPath, const char8_t* pNewDirPath);
    void EAPatchDeleteDirectory      (EA::Patch::PatchBuilder* pPatchBuilder, intptr_t userContext, const char8_t* pDirPath);
    void EAPatchStop                 (EA::Patch::PatchBuilder* pPatchBuilder, intptr_t userContext, EA::Patch::AsyncStatus status);
};


void TestPatchBuilderEventCallback::EAPatchStart(EA::Patch::PatchBuilder*, intptr_t)
{
    if(gbTracePatchBuilderEvents)
        EA::UnitTest::Report("Patch started\n");
}

void TestPatchBuilderEventCallback::EAPatchProgress(EA::Patch::PatchBuilder*, intptr_t, double patchProgress)
{
    if(gbTracePatchBuilderEvents && (EA::UnitTest::GetVerbosity() >= 1)) // If this unit test app was run with -v:1 or greater...
        EA::UnitTest::Report("Progress: %2.0f%%\n", patchProgress * 100);
}

void TestPatchBuilderEventCallback::EAPatchRetryableNetworkError(EA::Patch::PatchBuilder*, intptr_t, const char8_t* pURL)
{
    if(gbTracePatchBuilderEvents)
        EA::UnitTest::Report("Network re-tryable error: %s\n", pURL);
}

void TestPatchBuilderEventCallback::EAPatchNetworkAvailability(EA::Patch::PatchBuilder*, intptr_t, bool available)
{
    if(gbTracePatchBuilderEvents)
        EA::UnitTest::Report("Network availability change: now %s\n", available ? "available" : "unavailable");
}

void TestPatchBuilderEventCallback::EAPatchError(EA::Patch::PatchBuilder*, intptr_t, EA::Patch::Error& error)
{
    if(gbTracePatchBuilderEvents)
        EA::UnitTest::Report("Error: %s\n", error.GetErrorString());
}

void TestPatchBuilderEventCallback::EAPatchNewState(EA::Patch::PatchBuilder*, intptr_t, int newState)
{
    if(gbTracePatchBuilderEvents && (EA::UnitTest::GetVerbosity() >= 2))
        EA::UnitTest::Report("State: %s\n", EA::Patch::PatchBuilder::GetStateString((EA::Patch::PatchBuilder::State)newState));
}

void TestPatchBuilderEventCallback::EAPatchBeginFileDownload(EA::Patch::PatchBuilder*, intptr_t, const char8_t* pFilePath, const char8_t* pFileURL)
{
    if(gbTracePatchBuilderEvents && (EA::UnitTest::GetVerbosity() >= 1))
        EA::UnitTest::Report("Begin file download:\n   %s ->\n   %s\n", pFilePath, pFileURL);
}

void TestPatchBuilderEventCallback::EAPatchEndFileDownload(EA::Patch::PatchBuilder*, intptr_t, const char8_t* pFilePath, const char8_t* pFileURL)
{
    if(gbTracePatchBuilderEvents && (EA::UnitTest::GetVerbosity() >= 1))
        EA::UnitTest::Report("End file download:\n   %s->\n   %s\n", pFilePath, pFileURL);
}

void TestPatchBuilderEventCallback::EAPatchRenameFile(EA::Patch::PatchBuilder*, intptr_t, const char8_t* pPrevFilePath, const char8_t* pNewFilePath)
{
    if(gbTracePatchBuilderEvents && (EA::UnitTest::GetVerbosity() >= 1))
        EA::UnitTest::Report("Renaming file:\n   %s->\n   %s\n", pPrevFilePath, pNewFilePath);
}

void TestPatchBuilderEventCallback::EAPatchDeleteFile(EA::Patch::PatchBuilder*, intptr_t, const char8_t* pFilePath)
{
    if(gbTracePatchBuilderEvents && (EA::UnitTest::GetVerbosity() >= 1))
        EA::UnitTest::Report("Deleting file:\n   %s\n", pFilePath);
}

void TestPatchBuilderEventCallback::EAPatchRenameDirectory(EA::Patch::PatchBuilder* pPatchBuilder, intptr_t, const char8_t* pPrevDirPath, const char8_t* pNewDirPath)
{
    if(gbTracePatchBuilderEvents && (EA::UnitTest::GetVerbosity() >= 1))
        EA::UnitTest::Report("Renaming directory:\n   %s\n   %s\n", pPrevDirPath, pNewDirPath);
}

void TestPatchBuilderEventCallback::EAPatchDeleteDirectory(EA::Patch::PatchBuilder* pPatchBuilder, intptr_t, const char8_t* pDirPath)
{
    if(gbTracePatchBuilderEvents && (EA::UnitTest::GetVerbosity() >= 1))
        EA::UnitTest::Report("Deleting directory:\n   %s\n", pDirPath);
}

void TestPatchBuilderEventCallback::EAPatchStop(EA::Patch::PatchBuilder*, intptr_t, EA::Patch::AsyncStatus status)
{
    if(gbTracePatchBuilderEvents)
        EA::UnitTest::Report("Patch stopped with state %s\n", EA::Patch::GetAsyncStatusString(status));
}




///////////////////////////////////////////////////////////////////////////////
// TestTelemetryEventCallback
///////////////////////////////////////////////////////////////////////////////

class TestTelemetryEventCallback : public EA::Patch::TelemetryEventCallback
{
public:
    void TelemetryEvent(intptr_t userContext, EA::Patch::TelemetryPatchSystemInit&);
    void TelemetryEvent(intptr_t userContext, EA::Patch::TelemetryPatchSystemShutdown&);
    void TelemetryEvent(intptr_t userContext, EA::Patch::TelemetryDirectoryRetrievalBegin&);
    void TelemetryEvent(intptr_t userContext, EA::Patch::TelemetryDirectoryRetrievalEnd&);
    void TelemetryEvent(intptr_t userContext, EA::Patch::TelemetryPatchBuildBegin&);
    void TelemetryEvent(intptr_t userContext, EA::Patch::TelemetryPatchBuildEnd&);
    void TelemetryEvent(intptr_t userContext, EA::Patch::TelemetryPatchCancelBegin&);
    void TelemetryEvent(intptr_t userContext, EA::Patch::TelemetryPatchCancelEnd&);
};

void TestTelemetryEventCallback::TelemetryEvent(intptr_t, EA::Patch::TelemetryPatchSystemInit& tpsi)
{
    if(gbTracePatchBuilderTelemetry)
    {
        EA::UnitTest::Report("TelemetryPatchSystemInit:\n");
        EA::UnitTest::Report("   Date: %s\n", tpsi.mDate.c_str());
    }
}

void TestTelemetryEventCallback::TelemetryEvent(intptr_t, EA::Patch::TelemetryPatchSystemShutdown& tpss)
{
    if(gbTracePatchBuilderTelemetry)
    {
        EA::UnitTest::Report("TelemetryPatchSystemShutdown:\n");
        EA::UnitTest::Report("   Date: %s\n", tpss.mDate.c_str());
    }
}

void TestTelemetryEventCallback::TelemetryEvent(intptr_t, EA::Patch::TelemetryDirectoryRetrievalBegin& /*tdrb*/)
{
    // Never called in this test
}

void TestTelemetryEventCallback::TelemetryEvent(intptr_t, EA::Patch::TelemetryDirectoryRetrievalEnd& /*tdre*/)
{
    // Never called in this test
}

void TestTelemetryEventCallback::TelemetryEvent(intptr_t, EA::Patch::TelemetryPatchBuildBegin& tpbb)
{
    if(gbTracePatchBuilderTelemetry)
    {
        EA::UnitTest::Report("TelemetryPatchBuildBegin:\n");
        EA::UnitTest::Report("   Date:   %s\n", tpbb.mDate.c_str());
        EA::UnitTest::Report("   URL:    %s\n", tpbb.mPatchImplURL.c_str());
        EA::UnitTest::Report("   Id:     %s\n", tpbb.mPatchId.c_str());
        EA::UnitTest::Report("   Course: %s\n", tpbb.mPatchCourse.c_str());
    }
}

void TestTelemetryEventCallback::TelemetryEvent(intptr_t, EA::Patch::TelemetryPatchBuildEnd& tpbe)
{
    if(gbTracePatchBuilderTelemetry)
    {
        EA::UnitTest::Report("TelemetryPatchBuildEnd:\n");
        EA::UnitTest::Report("   Date:                    %s\n", tpbe.mDate.c_str());
        EA::UnitTest::Report("   URL:                     %s\n", tpbe.mPatchImplURL.c_str());
        EA::UnitTest::Report("   Id:                      %s\n", tpbe.mPatchId.c_str());
        EA::UnitTest::Report("   Course:                  %s\n", tpbe.mPatchCourse.c_str());
        EA::UnitTest::Report("   Direction:               %s\n", tpbe.mPatchDirection.c_str());
        EA::UnitTest::Report("   Status:                  %s\n", tpbe.mStatus.c_str());
        EA::UnitTest::Report("   HTTP Bps:                %s\n", tpbe.mDownloadSpeedBytesPerSecond.c_str());
        EA::UnitTest::Report("   Disk Bps:                %s\n", tpbe.mDiskSpeedBytesPerSecond.c_str());
        EA::UnitTest::Report("   Impl download vol:       %s\n", tpbe.mImplDownloadVolume.c_str());
        EA::UnitTest::Report("   Diff download vol:       %s\n", tpbe.mDiffDownloadVolume.c_str());
        EA::UnitTest::Report("   File download vol:       %s\n", tpbe.mFileDownloadVolume.c_str());
        EA::UnitTest::Report("   File download vol total: %s\n", tpbe.mFileDownloadVolumeFinal.c_str());
        EA::UnitTest::Report("   File copy vol:           %s\n", tpbe.mFileCopyVolume.c_str());
        EA::UnitTest::Report("   File copy vol total:     %s\n", tpbe.mFileCopyVolumeFinal.c_str());
        EA::UnitTest::Report("   Time estimate:           %s\n", tpbe.mPatchTimeEstimate.c_str());
        EA::UnitTest::Report("   Time actual:             %s\n", tpbe.mPatchTime.c_str());
    }
}


void TestTelemetryEventCallback::TelemetryEvent(intptr_t, EA::Patch::TelemetryPatchCancelBegin& tpcb)
{
    if(gbTracePatchBuilderTelemetry)
    {
        EA::UnitTest::Report("TelemetryPatchCancelBegin:\n");
        EA::UnitTest::Report("   Date: %s\n", tpcb.mDate.c_str());
        EA::UnitTest::Report("   URL:  %s\n", tpcb.mPatchImplURL.c_str());
        EA::UnitTest::Report("   Id:   %s\n", tpcb.mPatchId.c_str());
    }
}

void TestTelemetryEventCallback::TelemetryEvent(intptr_t, EA::Patch::TelemetryPatchCancelEnd& tpce)
{
    if(gbTracePatchBuilderTelemetry)
    {
        EA::UnitTest::Report("TelemetryPatchCancelEnd:\n");
        EA::UnitTest::Report("   Date:          %s\n", tpce.mDate.c_str());
        EA::UnitTest::Report("   URL:           %s\n", tpce.mPatchImplURL.c_str());
        EA::UnitTest::Report("   Id:            %s\n", tpce.mPatchId.c_str());
        EA::UnitTest::Report("   Direction:     %s\n", tpce.mPatchDirection.c_str());
        EA::UnitTest::Report("   Status:        %s\n", tpce.mStatus.c_str());
        EA::UnitTest::Report("   Time estimate: %s\n", tpce.mCancelTimeEstimate.c_str());
        EA::UnitTest::Report("   Time actual:   %s\n", tpce.mCancelTime.c_str());
    }
}




/// TestPatchBuilderJob
///
/// This is for use by our TestPatchDirectoryThread. 
///
struct TestPatchBuilderJob
{
    volatile bool     mbDone;
    int               mErrorCount;
    EA::Patch::String mHTTPServerAddress;
    uint16_t          mHTTPServerPort;


    TestPatchBuilderJob() : mbDone(false), mErrorCount(0), mHTTPServerAddress(), mHTTPServerPort(0) {}
};

static intptr_t TestPatchBuilderThread(void* pContext)
{
    using namespace EA::Patch;

    TestPatchBuilderJob* pJob = (TestPatchBuilderJob*)pContext;
    bool                 bResult;
    Error                error;
    String               sError;
    Server               httpServer; // Usually the global server is used, but we create a local one for this test.
    int&                 nErrorCount = pJob->mErrorCount; // Make this alias because the EATEST macros below reference "nErrorCount".
    PathList             directoryPathList;
    PathList             patchInfoFilePathList;
    String               sTempDirectory = EA::StdC::Strlcpy<EA::Patch::String, wchar_t>(gTempDirectory.c_str()); // Convert from wchar_t to char8_t String.

    GetPatchBuilderTestDirectoryList(directoryPathList, patchInfoFilePathList);

    for(PathList::const_iterator itDir = directoryPathList.begin(), itInfo = patchInfoFilePathList.begin(); itDir != directoryPathList.end(); ++itDir, ++itInfo)
    {
        const EA::IO::Path::PathStringW& sDirPathW  = *itDir;     // e.g. C:\EAPatchClient\test\data\PatchBuilderTest\test1\                                            // 
        const EA::IO::Path::PathStringW& sInfoPathW = *itInfo;    // e.g. C:\EAPatchClient\test\data\PatchBuilderTest\test1\patch\EAPatchBuilder%20test1.eaPatchInfo    //

        const String sDirPath  = EA::StdC::Strlcpy<EA::Patch::String, wchar_t>(sDirPathW.c_str()); // Convert from wchar_t to char8_t String.
        const String sInfoPath = EA::StdC::Strlcpy<EA::Patch::String, wchar_t>(sInfoPathW.c_str());

        // User applications don't need to call Server::SetServerURLBase unless they are using 
        // relative directories in the .eaPatchInfo and .eaPatchImpl files.
        const String sServerURLBase(EA::Patch::String::CtorSprintf(), "http://%s:%u/", pJob->mHTTPServerAddress.c_str(), (unsigned)pJob->mHTTPServerPort);
        httpServer.SetURLBase(sServerURLBase.c_str());

        // Read the .eaPatchInfo file. 
        // In a production environment the .eaPatchInfo would have been previously downloaded.
        PatchInfoSerializer patchInfoSerializer;
        PatchInfo           patchInfo;

        patchInfoSerializer.Deserialize(patchInfo, sInfoPath.c_str(), true);

        if(patchInfoSerializer.HasError())
        {
            error = patchInfoSerializer.GetError();
            error.GetErrorString(sError);
            EATEST_VERIFY_MSG(!patchInfoSerializer.HasError(), sError.c_str());
        }

        if(patchInfo.HasError())
        {
            error = patchInfo.GetError();
            error.GetErrorString(sError);
            EATEST_VERIFY_MSG(!patchInfo.HasError(), sError.c_str());
        }

        // If the .eaPatchInfo file read above succeeded, use it to build our test patch.
        if(nErrorCount == 0)
        {
            // We need to do a test of patching the "local" directory with the patch, then verify
            // the resulting output against the "expected" directory contents. We don't want to modify
            // the "local" directory in our data set but rather want to do all of this in a temp 
            // directory, so we copy the "local" directory to a temp location and work on it there.
            //
            // We execute the test possibly twice: once for patching C:\Temp\PatchBuilderTest\local\ in-place, 
            // and once for patching C:\Temp\PatchBuilderTest\local\ to C:\Temp\PatchBuilderTest\different\.
            // In the former case, the expected results are in our unit test's "expected" data directory,
            // and in the latter case the expected results are in our unit test's "expectedAtDifferentLocation' directory.
            // We use the existence of either or both of these directories to detect if we should run the 
            // given test against them. The expected results of patching to a different location are not
            // necessarily the same as patching in place, as there may be directories in the in-placed
            // patch directory that are ignored (not patched), and thus would not be present in the directory
            // at the different location (since they are irrelevant to the patch).

            // If there is currently a file in the root of the test directory named IgnoreThisTest.txt then we skip this test.
            const String sShouldIgnoreTestFilePath = sDirPath + "IgnoreThisTest.txt";
            const bool   bShouldIgnoreThisTest     = EA::IO::File::Exists(sShouldIgnoreTestFilePath.c_str());

            if(!bShouldIgnoreThisTest)
            {
                const String sTempLocalDirectory                 = sTempDirectory + kPatchBuilderTestDirectory + EA_FILE_PATH_SEPARATOR_8 + "local" + EA_FILE_PATH_SEPARATOR_8;     // e.g. C:\Temp\PatchBuilderTest\local\               //
                const String sTempDifferentDirectory             = sTempDirectory + kPatchBuilderTestDirectory + EA_FILE_PATH_SEPARATOR_8 + "different" + EA_FILE_PATH_SEPARATOR_8; // e.g. C:\Temp\PatchBuilderTest\different\           //
                const String sLocalDirectory                     = sDirPath + "local" + EA_FILE_PATH_SEPARATOR_8;
                const String sExpectedDirectory                  = sDirPath + "expected" + EA_FILE_PATH_SEPARATOR_8;
                const bool   bTestTwoLocationPatch               = EA::IO::Directory::Exists(sExpectedDirectory.c_str());
                const String sExpectedDirectoryDifferentLocation = sDirPath + "expectedAtDifferentLocation" + EA_FILE_PATH_SEPARATOR_8;
                const bool   bTestThreeLocationPatch             = EA::IO::Directory::Exists(sExpectedDirectoryDifferentLocation.c_str());
                bool         removeResult;

                EA::IO::Directory::Remove(sTempLocalDirectory.c_str());
                removeResult = DirectoryIsEmpty(sTempLocalDirectory.c_str());
                EATEST_VERIFY(removeResult);

                if(CopyDirectoryTree(sLocalDirectory.c_str(), sTempLocalDirectory.c_str(), true, error))
                {
                    EA::IO::Directory::Remove(sTempDifferentDirectory.c_str());
                    removeResult = DirectoryIsEmpty(sTempDifferentDirectory.c_str());
                    EATEST_VERIFY(removeResult);

                    PatchBuilder patchBuilder;
                    patchBuilder.SetServer(&httpServer);

                    TestPatchBuilderEventCallback eventCallback;
                    patchBuilder.RegisterPatchBuilderEventCallback(&eventCallback, 0);

                    TestTelemetryEventCallback telemetryCallback;
                    patchBuilder.RegisterTelemetryEventCallback(&telemetryCallback, 0);

                    for(int i = 0; i < 2; i++) // We execute the test possibly twice: once for patching C:\Temp\PatchBuilderTest\local\ in-place, and once for patching C:\Temp\PatchBuilderTest\local\ to C:\Temp\PatchBuilderTest\different\   //
                    {
                        if(((i == 0) && bTestThreeLocationPatch) || 
                           ((i == 1) && bTestTwoLocationPatch))
                        {
                            patchBuilder.ClearError();

                            const bool bInPlacePatch = ((i == 1) && bTestTwoLocationPatch);
                            const bool bTestAsync    = (i > 100); // Currently always false.

                            EA::UnitTest::Report("   %s %s\n", sDirPath.c_str(), bInPlacePatch ? "(in place)" : "(three location)");

                            if(bInPlacePatch)
                                patchBuilder.SetDirPaths(sTempLocalDirectory.c_str(), sTempLocalDirectory.c_str());      // We are testing the patching of C:\Temp\PatchBuilderTest\local\ in-place.
                            else
                                patchBuilder.SetDirPaths(sTempDifferentDirectory.c_str(), sTempLocalDirectory.c_str());  // We are testing the patching of C:\Temp\PatchBuilderTest\local\ to C:\Temp\PatchBuilderTest\different\   //

                            patchBuilder.SetPatchInfo(patchInfo);

                            if(bTestAsync)
                            {
                                bResult = patchBuilder.BuildPatchAsync();

                                while(!IsAsyncOperationEnded(patchBuilder.GetAsyncStatus()))
                                    EA::Thread::ThreadSleep(300);
                            }
                            else
                                bResult = patchBuilder.BuildPatchBlocking();

                            if(patchBuilder.HasError())
                            {
                                EATEST_VERIFY(!bResult);

                                error = patchBuilder.GetError();
                                error.GetErrorString(sError);
                                EATEST_VERIFY_MSG(!patchBuilder.HasError(), sError.c_str());
                            }
                            else
                            {
                                EATEST_VERIFY(bResult);

                                bool           bEqual = false;
                                String         sEqualFailureDescription;
                                const char8_t* pPatchedDirectoryPath  = (bInPlacePatch ? sTempLocalDirectory.c_str() : sTempDifferentDirectory.c_str());
                                const char8_t* pExpectedDirectoryPath = (bInPlacePatch ? sExpectedDirectory.c_str()  : sExpectedDirectoryDifferentLocation.c_str());

                                bResult = CompareDirectoryTree(pPatchedDirectoryPath, pExpectedDirectoryPath, bEqual, sEqualFailureDescription, true, error);

                                if(bResult)
                                {
                                    if(!bEqual) // We have this 'if' here so we can set debugger breakpoints. Not all debuggers provide conditional breakpoints.
                                    {
                                        EATEST_VERIFY_F(bEqual, "Directory mismatch:\n   %s\n      <->\n   %s\n", pPatchedDirectoryPath, pExpectedDirectoryPath);
                                        EA::UnitTest::Report("   %s\n", sEqualFailureDescription.c_str());
                                    }
                                }
                                else
                                {
                                    error.GetErrorString(sError);
                                    EATEST_VERIFY_MSG(sError.empty(), sError.c_str()); // This verify will intentionally always fail.
                                }
                            }
                        }
                    }

                    EA::IO::Directory::Remove(sTempLocalDirectory.c_str());
                    removeResult = DirectoryIsEmpty(sTempLocalDirectory.c_str());
                    EATEST_VERIFY(removeResult);

                    EA::IO::Directory::Remove(sTempDifferentDirectory.c_str());
                    removeResult = DirectoryIsEmpty(sTempDifferentDirectory.c_str());
                    EATEST_VERIFY(removeResult);
                }
                else
                {
                    error.GetErrorString(sError);
                    EATEST_VERIFY_MSG(sError.empty(), sError.c_str()); // This verify will intentionally always fail.
                }
            }
        }
    }

    pJob->mbDone = true;
    EAWriteBarrier(); // Make sure the mbDone assignment becomes visible to other threads.

    return 0; // The error count was set in pJob->mErrorCount. This is the thread return value, not the test result return value.
}



///////////////////////////////////////////////////////////////////////////////
// TestPatchBuilder
///////////////////////////////////////////////////////////////////////////////

int TestPatchBuilder()
{
    using namespace EA::Internet;

    int nErrorCount(0);

    const bool bDataDirectoryExists = EA::IO::Directory::Exists(gDataDirectory.c_str());

    if(bDataDirectoryExists)
    {
        EA::Patch::String sHTTPServerAddress = kPatchBuilderTestAddress;
        uint16_t          httpServerPort     = kPatchBuilderTestPort;

        if(!gExternalHTTPServerAddress.empty())
        {
            sHTTPServerAddress = gExternalHTTPServerAddress;
            httpServerPort     = gExternalHTTPServerPort;
        }

        HTTPServer* const pHTTPServer = EA_NEW("HTTPServer") HTTPServer;

        pHTTPServer->AddRef();
        pHTTPServer->SetPort(EA::StdC::ToBigEndian(kPatchBuilderTestPort)); // It's not really possible for us to tell the server to run at sHTTPServerAddress, unless sHTTPServerAddress happens to be 127.0.0.1 or equivalent.
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
                // We run the unit tests in another thread.
                EA::Thread::Thread       thread;
                EA::StdC::LimitStopwatch limitStopwatch(EA::StdC::Stopwatch::kUnitsSeconds, kTimeoutSeconds, true);
                TestPatchBuilderJob      job;

                job.mHTTPServerAddress = sHTTPServerAddress;
                job.mHTTPServerPort    = httpServerPort;

                EA::Thread::ThreadParameters params;
                params.mpName      = "EAPatchTestBuilder";
                params.mnStackSize = 256 * 1024; // The thread may require more than 64K of stack space.
                thread.Begin(TestPatchBuilderThread, &job, &params);

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
                EA::UnitTest::Report("   HTTP server start failure. Is there already a server running on port %u?\n", (unsigned)kPatchBuilderTestPort);
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





// Builds a list of directories which have build test data in them.
// These are dierctories whose name begins with "cancelTest", such as "cancelTest20".
static void GetPatchBuilderCancelTestDirectoryList(PathList& directoryPathList)
{
    using namespace EA::IO;

    // Retrieve the list of directories.
    if(!gDataDirectory.empty())
    {
        Path::PathStringW testDirectoryPath = gDataDirectory;
        testDirectoryPath += L"PatchBuilderTest";
        testDirectoryPath += EA_FILE_PATH_SEPARATOR_W;

        DirectoryIterator            di;
        DirectoryIterator::EntryList entryList;

        di.Read(testDirectoryPath.c_str(), entryList, EA_WCHAR("cancelTest*"), kDirectoryEntryDirectory, 20, false);

        for(DirectoryIterator::EntryList::iterator it = entryList.begin(); it != entryList.end(); ++it)
        {
            DirectoryIterator::Entry& entry = *it;

            // Record the directory path
            entry.msName.insert(0, testDirectoryPath.c_str());
            directoryPathList.push_back(entry.msName.c_str());
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
// TestPatchBuilderCancel
///////////////////////////////////////////////////////////////////////////////

int TestPatchBuilderCancel()
{
    using namespace EA::Patch;

    bool      bResult;
    Error     error;
    String    sError;
    int       nErrorCount = 0;
    PathList  directoryPathList;
    String    sTempDirectory = EA::StdC::Strlcpy<EA::Patch::String, wchar_t>(gTempDirectory.c_str()); // Convert from wchar_t to char8_t String.

    GetPatchBuilderCancelTestDirectoryList(directoryPathList);

    for(PathList::const_iterator itDir = directoryPathList.begin(); itDir != directoryPathList.end(); ++itDir)
    {
        const EA::IO::Path::PathStringW& sDirPathW  = *itDir;     // e.g. C:\EAPatchClient\test\data\PatchBuilderTest\test1\                                            // 

        String sDirPath  = EA::StdC::Strlcpy<EA::Patch::String, wchar_t>(sDirPathW.c_str()); // Convert from wchar_t to char8_t String.

        // If there is currently a file in the root of the test directory named IgnoreThisTest.txt then we skip this test.
        const String sShouldIgnoreTestFilePath = sDirPath + "IgnoreThisTest.txt";
        const bool   bShouldIgnoreThisTest     = EA::IO::File::Exists(sShouldIgnoreTestFilePath.c_str());

        if(!bShouldIgnoreThisTest)
        {
            const String sTempLocalDirectory = sTempDirectory + kPatchBuilderTestDirectory + EA_FILE_PATH_SEPARATOR_8 + "local" + EA_FILE_PATH_SEPARATOR_8;     // e.g. C:\Temp\PatchBuilderTest\local\               //
            const String sLocalDirectory     = sDirPath + "local" + EA_FILE_PATH_SEPARATOR_8;
            const String sExpectedDirectory  = sDirPath + "expected" + EA_FILE_PATH_SEPARATOR_8;

            // We copy the local directory to a temporary location and work on that, as we are modifying it.
            if(CopyDirectoryTree(sLocalDirectory.c_str(), sTempLocalDirectory.c_str(), true, error))
            {
                PatchBuilder patchBuilder;

                TestPatchBuilderEventCallback eventCallback;
                patchBuilder.RegisterPatchBuilderEventCallback(&eventCallback, 0);

                TestTelemetryEventCallback telemetryCallback;
                patchBuilder.RegisterTelemetryEventCallback(&telemetryCallback, 0);

                // When cancelling a patch, it doesn't matter whether it's an in-place patch or a three-directory patch;
                // the cancellation process is the same.
                patchBuilder.SetDirPaths(sTempLocalDirectory.c_str(), sTempLocalDirectory.c_str(), true);

                // To do: Have an option to look for the patchInfo and load it if present.
                //patchBuilder.SetPatchInfo(patchInfo);

                EA::UnitTest::Report("   %s\n", sLocalDirectory.c_str());

                bResult = patchBuilder.CancelPatchBlocking();

                if(patchBuilder.HasError())
                {
                    EATEST_VERIFY(!bResult);

                    error = patchBuilder.GetError();
                    error.GetErrorString(sError);
                    EATEST_VERIFY_MSG(!patchBuilder.HasError(), sError.c_str());
                }
                else
                {
                    EATEST_VERIFY(bResult);

                    bool   bEqual = false;
                    String sEqualFailureDescription;

                    bResult = CompareDirectoryTree(sTempLocalDirectory.c_str(), sExpectedDirectory.c_str(), bEqual, sEqualFailureDescription, true, error);

                    if(bResult)
                    {
                        if(!bEqual) // We have this 'if' here so we can set debugger breakpoints. Not all debuggers provide conditional breakpoints.
                            EATEST_VERIFY_F(bEqual, "Directory mismatch:\n   %s\n      <->\n   %s\n   %s\n", sTempLocalDirectory.c_str(), sExpectedDirectory.c_str(), sEqualFailureDescription.c_str());
                    }
                    else
                    {
                        error.GetErrorString(sError);
                        EATEST_VERIFY_MSG(sError.empty(), sError.c_str()); // This verify will intentionally always fail.
                    }
                }

                EA::IO::Directory::Remove(sTempLocalDirectory.c_str());
            }
            else
            {
                error.GetErrorString(sError);
                EATEST_VERIFY_MSG(sError.empty(), sError.c_str()); // This verify will intentionally always fail.
            }
        }
    }

    return nErrorCount;
}





