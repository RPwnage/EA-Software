///////////////////////////////////////////////////////////////////////////////
// TestFileNotification.cpp
//
// Copyright (c) 2007 Criterion Software Limited and its licensors. All Rights Reserved.
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#include <EABase/eabase.h>
#include <EAIO/internal/Config.h>
#include <EAIO/EAFileStream.h>
#include <EAIO/EAFileDirectory.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAFileNotification.h>
#include <EAIO/PathString.h>
#include <EAIOTest/EAIO_Test.h>
#include <EATest/EATest.h>
#include <EAStdC/EAString.h>
#include <eathread/eathread_thread.h>
#include <coreallocator/icoreallocator_interface.h>
#include EA_ASSERT_HEADER



enum Operation
{
    kOperationNone,
    kOperationFileCreate,
    kOperationFileChange,
    kOperationFileDelete,
    kOperationDirCreate,
    kOperationDirDelete
};


struct WorkData
{
    Operation                       mOperation;
    EA::IO::Path::PathStringW       mDirectoryPath;
    EA::IO::Path::PathStringW       mFileName;
    EA::IO::Path::PathStringW       mFilePath;
    EA::Thread::AtomicInt32         mbDetected;    // True if the execution of this operation has been detected by FileChangeNotification.
    EA::Thread::AtomicInt32         mnErrorCount;
    EA::IO::FileChangeNotification  mFCN;

    WorkData() : mOperation(kOperationNone), 
                 mDirectoryPath(),
                 mFileName(),
                 mFilePath(),
                 mbDetected(0),
                 mnErrorCount(0),
                 mFCN() { }
private:
    WorkData(const WorkData& rhs);
    WorkData& operator=(const WorkData& rhs);
};


///////////////////////////////////////////////////////////////////////////////
// ChangeMaker
//
static void FileChangeDetector(EA::IO::FileChangeNotification* /*pFileChangeNotification*/, const wchar_t* pDirectoryPath,
                               const wchar_t* pFileName, int nChangeTypeFlags, void* pContext)
{
    using namespace EA::IO;

    WorkData* const pWorkData = (WorkData*)pContext;

    int  nErrorCount = 0;
    bool bResult;

    pWorkData->mbDetected = 1;

    switch(pWorkData->mOperation)
    {
        case kOperationFileCreate:
            EATEST_VERIFY((nChangeTypeFlags & FileChangeNotification::kChangeTypeFlagFileName) != 0);
            break;

        case kOperationFileChange:
            EATEST_VERIFY((nChangeTypeFlags & FileChangeNotification::kChangeTypeFlagModified) != 0);
            break;

        case kOperationFileDelete:
            EATEST_VERIFY((nChangeTypeFlags & FileChangeNotification::kChangeTypeFlagFileName) != 0);
            break;

        case kOperationDirCreate:
            EATEST_VERIFY((nChangeTypeFlags & FileChangeNotification::kChangeTypeFlagDirectoryName) != 0);
            break;

        case kOperationDirDelete:
            EATEST_VERIFY((nChangeTypeFlags & FileChangeNotification::kChangeTypeFlagDirectoryName) != 0);
            break;

        case kOperationNone:
        default:
            EATEST_VERIFY(false);
            break;
    }


    switch(pWorkData->mOperation)
    {
        case kOperationFileCreate:
        case kOperationFileChange:
        case kOperationFileDelete:
        case kOperationDirCreate:
        case kOperationDirDelete:
            bResult = (EA::StdC::Stricmp(pWorkData->mFileName.c_str(), pFileName) == 0);
            EATEST_VERIFY(bResult);
            bResult = (EA::StdC::Stricmp(pWorkData->mDirectoryPath.c_str(), pDirectoryPath) == 0);
            EATEST_VERIFY(bResult);

        default:
            break;
    }

    pWorkData->mnErrorCount += nErrorCount;
}


///////////////////////////////////////////////////////////////////////////////
// TestFileNotification
//
int TestFileNotification()
{
    using namespace EA::IO;

    EA::UnitTest::Report("TestFileNotification\n");

    int nErrorCount(0);

    const float    fDiskSpeed     = EA::UnitTest::GetSystemSpeed(EA::UnitTest::kSpeedTypeDisk);
    const float    fDiskSpeedUsed = (fDiskSpeed >= 0.2f) ? fDiskSpeed : 0.2f;
    const uint32_t kMinWaitTimeMs = (uint32_t)( 500 / fDiskSpeedUsed);
    const uint32_t kMaxWaitTimeMs = (uint32_t)(1000 / fDiskSpeedUsed);

    Path::PathStringW sTestDir(gRootDirectoryW);
    Path::Append(sTestDir, Path::PathStringW(EA_WCHAR("FileNotificationTest")));
    Path::EnsureTrailingSeparator(sTestDir);

    bool bResult = Directory::EnsureExists(sTestDir.c_str());
    EATEST_VERIFY(bResult);

    if(bResult)
    {
        EA::UnitTest::ThreadSleepRandom(kMinWaitTimeMs, kMaxWaitTimeMs);

        WorkData workData;

        bResult = workData.mFCN.Init();
        EATEST_VERIFY(bResult);
        bResult = workData.mFCN.SetWatchedDirectory(sTestDir.c_str());
        workData.mFCN.SetChangeTypeFlags(0xffff);
        EATEST_VERIFY(bResult);
        workData.mFCN.SetOptionFlags(FileChangeNotification::kOptionFlagWatchSubdirectories); // | kOptionFlagMultithreaded
        workData.mFCN.SetNotificationCallback(FileChangeDetector, &workData);
        bResult = workData.mFCN.Start();
        EATEST_VERIFY(bResult);


        // kOperationFileDelete
        EA::UnitTest::ThreadSleepRandom(kMinWaitTimeMs, kMaxWaitTimeMs);
        workData.mOperation     = kOperationFileDelete;
        workData.mDirectoryPath = sTestDir;
        workData.mFileName      = EA_WCHAR("FileA.txt");
        workData.mFilePath      = workData.mDirectoryPath; workData.mFilePath += workData.mFileName;
        workData.mbDetected     = 0;

        if(File::Exists(workData.mFilePath.c_str()))
        {
            bResult = File::Remove(workData.mFilePath.c_str());
            EATEST_VERIFY(bResult);
            if(bResult)
            {
                EA::UnitTest::ThreadSleepRandom(kMinWaitTimeMs, kMaxWaitTimeMs);
                workData.mFCN.Poll((int)kMaxWaitTimeMs);
                EATEST_VERIFY(workData.mbDetected.GetValue() == 1);
            }
        }


        // kOperationFileCreate
        EA::UnitTest::ThreadSleepRandom(kMinWaitTimeMs, kMaxWaitTimeMs);
        workData.mOperation     = kOperationFileCreate;
        workData.mDirectoryPath = sTestDir;
        workData.mFileName      = EA_WCHAR("FileA.txt");
        workData.mFilePath      = workData.mDirectoryPath; workData.mFilePath += workData.mFileName;
        workData.mbDetected     = 0;

        bResult = File::Create(workData.mFilePath.c_str());
        EATEST_VERIFY(bResult);
        if(bResult)
        {
            EA::UnitTest::ThreadSleepRandom(kMinWaitTimeMs, kMaxWaitTimeMs);
            workData.mFCN.Poll((int)kMaxWaitTimeMs);
            EATEST_VERIFY(workData.mbDetected.GetValue() == 1);  // Verify that a change was detected (in this case that a file was created).
        }


        // kOperationDirDelete
        EA::UnitTest::ThreadSleepRandom(kMinWaitTimeMs, kMaxWaitTimeMs);
        workData.mOperation     = kOperationDirDelete;
        workData.mDirectoryPath = sTestDir;
        workData.mFileName      = EA_WCHAR("DirA");
        workData.mFilePath      = workData.mDirectoryPath; workData.mFilePath += workData.mFileName; workData.mFilePath += EA_FILE_PATH_SEPARATOR_16;
        workData.mbDetected     = 0;

        if(Directory::Exists(workData.mFilePath.c_str()))
        {
            bResult = Directory::Remove(workData.mFilePath.c_str());
            EATEST_VERIFY(bResult);
            if(bResult)
            {
                EA::UnitTest::ThreadSleepRandom(kMinWaitTimeMs, kMaxWaitTimeMs);
                workData.mFCN.Poll((int)kMaxWaitTimeMs);

                // Currently, the console (polled) versio nof EAFileNotification doesn't detect directory creation/removal.
                #if !FCN_POLL_ENABLED
                    EATEST_VERIFY(workData.mbDetected.GetValue() == 1);
                #endif
            }
        }


        // kOperationDirCreate
        EA::UnitTest::ThreadSleepRandom(kMinWaitTimeMs, kMaxWaitTimeMs);
        workData.mOperation     = kOperationDirCreate;
        workData.mDirectoryPath = sTestDir;
        workData.mFileName      = EA_WCHAR("DirA");
        workData.mFilePath      = workData.mDirectoryPath; workData.mFilePath += workData.mFileName; workData.mFilePath += EA_FILE_PATH_SEPARATOR_16;
        workData.mbDetected     = 0;

        bResult = Directory::Create(workData.mFilePath.c_str());
        EATEST_VERIFY(bResult);
        if(bResult)
        {
            EA::UnitTest::ThreadSleepRandom(kMinWaitTimeMs, kMaxWaitTimeMs);
            workData.mFCN.Poll((int)kMaxWaitTimeMs);

            // Currently, the console (polled) versio nof EAFileNotification doesn't detect directory creation/removal.
            #if !FCN_POLL_ENABLED
                EATEST_VERIFY(workData.mbDetected.GetValue() == 1);
            #endif
        }


        // kOperationFileChange
        EA::UnitTest::ThreadSleepRandom(kMinWaitTimeMs, kMaxWaitTimeMs);
        workData.mOperation     = kOperationFileChange;
        workData.mDirectoryPath = sTestDir;
        workData.mFileName      = EA_WCHAR("FileA.txt");
        workData.mFilePath      = workData.mDirectoryPath; workData.mFilePath += workData.mFileName;
        workData.mbDetected     = 0;

        FileStream fileStream(workData.mFilePath.c_str());
        bResult = fileStream.Open(kAccessFlagReadWrite, kCDOpenAlways);
        EATEST_VERIFY(bResult);
        if(bResult)
        {
            char c = 0;
            fileStream.SetPosition(0);
            fileStream.Write(&c, 1);
            fileStream.Close();

            EA::UnitTest::ThreadSleepRandom(kMinWaitTimeMs, kMaxWaitTimeMs);
            workData.mFCN.Poll((int)kMaxWaitTimeMs);
            EATEST_VERIFY(workData.mbDetected.GetValue() == 1);
        }


        // kOperationFileDelete
        EA::UnitTest::ThreadSleepRandom(kMinWaitTimeMs, kMaxWaitTimeMs);
        workData.mOperation     = kOperationFileDelete;
        workData.mDirectoryPath = sTestDir;
        workData.mFileName      = EA_WCHAR("FileA.txt");
        workData.mFilePath      = workData.mDirectoryPath; workData.mFilePath += workData.mFileName;
        workData.mbDetected     = 0;

        bResult = File::Remove(workData.mFilePath.c_str());
        EATEST_VERIFY(bResult);
        if(bResult)
        {
            EA::UnitTest::ThreadSleepRandom(kMinWaitTimeMs, kMaxWaitTimeMs);
            workData.mFCN.Poll((int)kMaxWaitTimeMs);
            EATEST_VERIFY(workData.mbDetected.GetValue() == 1);
        }


        // kOperationDirDelete
        EA::UnitTest::ThreadSleepRandom(kMinWaitTimeMs, kMaxWaitTimeMs);
        workData.mOperation     = kOperationDirDelete;
        workData.mDirectoryPath = sTestDir;
        workData.mFileName      = EA_WCHAR("DirA");
        workData.mFilePath      = workData.mDirectoryPath; workData.mFilePath += workData.mFileName; workData.mFilePath += EA_FILE_PATH_SEPARATOR_16;
        workData.mbDetected     = 0;

        bResult = Directory::Remove(workData.mFilePath.c_str());
        EATEST_VERIFY(bResult);
        if(bResult)
        {
            EA::UnitTest::ThreadSleepRandom(kMinWaitTimeMs, kMaxWaitTimeMs);
            workData.mFCN.Poll((int)kMaxWaitTimeMs);

            // Currently, the console (polled) versio nof EAFileNotification doesn't detect directory creation/removal.
            #if !FCN_POLL_ENABLED
                EATEST_VERIFY(workData.mbDetected.GetValue() == 1);
            #endif
        }

        nErrorCount += workData.mnErrorCount.GetValue();
    }

    return nErrorCount;
}










