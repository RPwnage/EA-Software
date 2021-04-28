/////////////////////////////////////////////////////////////////////////////
// TestPS3DumpFile.cpp
//
// Copyright (c) 2008, Electronic Arts Inc. All rights reserved.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#include <EABase/eabase.h>
#include <EATest/EATest.h>

#if defined(EA_PLATFORM_WINDOWS)

#include <EACallstackTest/EACallstackTest.h>
#include <EAStdC/EAString.h>
#include <EACallstack/PS3DumpFile.h>
#include <EACallstack/DWARF2File.h>
#include <EAIO/EAFileStream.h>
#include <EAIO/EAStreamAdapter.h>
#include <EAIO/EAFileUtil.h>


int TestPS3DumpFile()
{
    using namespace EA::Callstack;
    using namespace EA::StdC;

    EA::UnitTest::ReportVerbosity(1, "\nTestPS3DumpFile\n");

    int nErrorCount = 0;

    PS3DumpFile     ps3DumpFile;
    const char16_t* pDatabaseFilePath = EA_CHAR16("D:\\Temp\\PS3 Core Dump Test Files\\MOHA RSX 1\\MOHAGame-PS3Release.elf");
    const char16_t* pDumpFilePath     = EA_CHAR16("D:\\Temp\\PS3 Core Dump Test Files\\MOHA RSX 1\\ps3core-1248203528-0x01010400-MOHAGame-PS3Release.self.elf");

    if(EA::IO::File::Exists(pDatabaseFilePath) && EA::IO::File::Exists(pDumpFilePath))
    {
        ps3DumpFile.AddDatabaseFile(pDatabaseFilePath);

        if(ps3DumpFile.Init(pDumpFilePath, false))
        {
            eastl::string sOutput;

            if(sOutput.size()) // debug code.
            {
                // Here we create a three-way deadlock situation and add it to the PS3DumpFile data, for testing purposes.
                // Create three new threads.
                eastl_size_t t = ps3DumpFile.mPPUThreadRegisterDataArray.size();
                ps3DumpFile.mPPUThreadRegisterDataArray.resize(t + 3);
                ps3DumpFile.mPPUThreadDataArray.resize(t + 3);

                ps3DumpFile.mPPUThreadRegisterDataArray[t + 0].mPPUThreadID = 200;
                ps3DumpFile.mPPUThreadRegisterDataArray[t + 1].mPPUThreadID = 201;
                ps3DumpFile.mPPUThreadRegisterDataArray[t + 2].mPPUThreadID = 202;

                ps3DumpFile.mPPUThreadDataArray[t + 0].mPPUThreadID = 200;
                ps3DumpFile.mPPUThreadDataArray[t + 1].mPPUThreadID = 201;
                ps3DumpFile.mPPUThreadDataArray[t + 2].mPPUThreadID = 202;

                Strcpy(ps3DumpFile.mPPUThreadDataArray[t + 0].mName, "Thr_200");
                Strcpy(ps3DumpFile.mPPUThreadDataArray[t + 1].mName, "Thr_201");
                Strcpy(ps3DumpFile.mPPUThreadDataArray[t + 2].mName, "Thr_202");

                // Create three new mutexes.
                eastl_size_t m = ps3DumpFile.mMutexDataArray.size();
                ps3DumpFile.mMutexDataArray.resize(m + 3);

                ps3DumpFile.mMutexDataArray[m + 0].mID = 100;
                ps3DumpFile.mMutexDataArray[m + 1].mID = 101;
                ps3DumpFile.mMutexDataArray[m + 2].mID = 102;

                Strcpy(ps3DumpFile.mMutexDataArray[m + 0].mName, "Mtx_100");
                Strcpy(ps3DumpFile.mMutexDataArray[m + 1].mName, "Mtx_101");
                Strcpy(ps3DumpFile.mMutexDataArray[m + 2].mName, "Mtx_102");

                ps3DumpFile.mMutexDataArray[m + 0].mOwnerThreadID = 200;
                ps3DumpFile.mMutexDataArray[m + 1].mOwnerThreadID = 201;
                ps3DumpFile.mMutexDataArray[m + 2].mOwnerThreadID = 202;

                ps3DumpFile.mMutexDataArray[m + 0].mWaitThreadIDArray.push_back(302);  // Intentionally bogus entry.
                ps3DumpFile.mMutexDataArray[m + 0].mWaitThreadIDArray.push_back(202);
                ps3DumpFile.mMutexDataArray[m + 1].mWaitThreadIDArray.push_back(200); 
                ps3DumpFile.mMutexDataArray[m + 1].mWaitThreadIDArray.push_back(300);  // Intentionally bogus entry.
                ps3DumpFile.mMutexDataArray[m + 2].mWaitThreadIDArray.push_back(201);


                // Here we create a "possible implicit deadlock" (uses the above threads and mutexes).
                // Create a semaphore and pretend one of the threads is waiting on it.
                eastl_size_t s = ps3DumpFile.mSemaphoreDataArray.size();
                ps3DumpFile.mSemaphoreDataArray.resize(s + 1);

                ps3DumpFile.mSemaphoreDataArray[s].mID = 401;
                Strcpy(ps3DumpFile.mSemaphoreDataArray[s].mName, "Sem_401");
                ps3DumpFile.mSemaphoreDataArray[s].mWaitThreadIDArray.push_back(200);
            }

            ps3DumpFile.AddSourceCodeDirectory(L"D:\\Projects\\EAOS\\UTF\\DL\\");

            ps3DumpFile.Trace(sOutput);

            if(sOutput.size() < 64000)
                EA::UnitTest::ReportVerbosity(1, sOutput.c_str());
            else
            {
                EA::IO::FileStream fs("C:\\temp\\PS3DumpFile.txt");

                if(fs.Open(EA::IO::kAccessFlagReadWrite, EA::IO::kCDCreateAlways))
                {
                    fs.Write(sOutput.data(), sOutput.length());
                    fs.Close();
                    EA::UnitTest::ReportVerbosity(1, "Output written to C:\\temp\\PS3DumpFile.txt\n");
                }
                else
                    EA::UnitTest::ReportVerbosity(1, sOutput.c_str());
            }
        }
    }

    return nErrorCount;
}

#else

int TestPS3DumpFile()
{
    EA::UnitTest::ReportVerbosity(1, "\nTestPS3DumpFile\n");

    return 0;
}

#endif // WINDOWS

