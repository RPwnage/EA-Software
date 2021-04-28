/////////////////////////////////////////////////////////////////////////////
// EAIO_Test.cpp
//
// Copyright (c) 2006, Electronic Arts Inc. All rights reserved.
// Written by Paul Pedriana.
/////////////////////////////////////////////////////////////////////////////


#include <EAIOTest/EAIO_Test.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/PathString.h>
#include <EAIO/FnEncode.h>
#include <EAStdC/EAString.h>
#include <EATest/EATest.h>
#include <EATest/EATestMain.inl>

//Prevents false positive memory leaks on GCC platforms (such as PS3)
#if defined(EA_COMPILER_GNUC)
    #define EA_MEMORY_GCC_USE_FINALIZE
#endif
#include <MemoryMan/MemoryMan.inl>

#include <MemoryMan/CoreAllocator.inl>
#include <stdio.h>

#ifdef EA_DLL
    #include <MemoryMan/MemoryManDLL.h>
#endif

#if defined(EA_PLATFORM_PS3)
    #include <sdk_version.h>
    #include <cell/sysmodule.h>
    #include <sysutil/sysutil_gamecontent.h> 
#elif defined(__APPLE__)
    #include <stdio.h>
    #include <unistd.h>
    #include <errno.h>
    #include <fcntl.h>
#endif


char8_t gRootDirectory8[EA::IO::kMaxPathLength];
wchar_t gRootDirectoryW[EA::IO::kMaxPathLength];

bool gbVerboseOutput = false;


CoreAllocatorMalloc::CoreAllocatorMalloc()
  : mAllocationCount(0), mFreeCount(0)
{
}

CoreAllocatorMalloc::CoreAllocatorMalloc(const CoreAllocatorMalloc &)
  : mAllocationCount(0), mFreeCount(0)
{
}

CoreAllocatorMalloc::~CoreAllocatorMalloc()
{
}

CoreAllocatorMalloc& CoreAllocatorMalloc::operator=(const CoreAllocatorMalloc&)
{
    return *this;
}


int SetupDirectories()
{
    int nErrorCount = 0;

    // Get app root directory
    #if defined(EA_PLATFORM_PS3)
        EA::StdC::Strlcpy(gRootDirectory8,  "/app_home/EAIOTest/", EA::IO::kMaxPathLength);   // This might be changed below.
        EA::StdC::Strlcpy(gRootDirectoryW, L"/app_home/EAIOTest/", EA::IO::kMaxPathLength);

        // To do: Set the root and temp directories based on the GameData API.
        // CellGameContentSize size;
        // int result = cellGameDataCheck(CELL_GAME_GAMETYPE_HDD, "EAIOTest", &size);
        // if(result == CELL_GAME_RET_OK) ...

        EA::IO::SetTempDirectory(EA_CHAR16("/app_home/tmp"));

	#elif defined(EA_PLATFORM_MICROSOFT)
		#if defined(EA_PLATFORM_XENON)
			EA::StdC::Strlcpy(gRootDirectory8,  "D:\\EAIOTest\\", EA::IO::kMaxPathLength);
         	EA::StdC::Strlcpy(gRootDirectoryW, L"D:\\EAIOTest\\", EA::IO::kMaxPathLength);
		#elif defined(EA_PLATFORM_WINDOWS) && !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
            EA::IO::GetTempDirectory(gRootDirectory8, EA::IO::kMaxPathLength);
            EA::IO::GetTempDirectory(gRootDirectoryW, EA::IO::kMaxPathLength);

            EA::IO::Path::EnsureTrailingSeparator(gRootDirectory8, EA::IO::kMaxPathLength);
            EA::StdC::Strlcat(gRootDirectory8, "EAIOTest\\", EA::IO::kMaxPathLength);
            EA::IO::Path::EnsureTrailingSeparator(gRootDirectoryW, EA::IO::kMaxPathLength);
            EA::StdC::Strlcat(gRootDirectoryW, "EAIOTest\\", EA::IO::kMaxPathLength);
        #else
            EA::StdC::Strlcpy(gRootDirectory8,  "C:\\temp\\EAIOTest\\", EA::IO::kMaxPathLength);
            EA::StdC::Strlcpy(gRootDirectoryW, L"C:\\temp\\EAIOTest\\", EA::IO::kMaxPathLength);   
		#endif

    #elif defined(EA_PLATFORM_ANDROID)
        EA::IO::GetTempDirectory(gRootDirectory8, EA::IO::kMaxPathLength);
        EA::IO::GetTempDirectory(gRootDirectoryW, EA::IO::kMaxPathLength);

        EA::IO::Path::EnsureTrailingSeparator(gRootDirectory8, EA::IO::kMaxPathLength);
        EA::StdC::Strlcat(gRootDirectory8, "EAIOTest/", EA::IO::kMaxPathLength);
        EA::IO::Path::EnsureTrailingSeparator(gRootDirectoryW, EA::IO::kMaxPathLength);
        EA::StdC::Strlcat(gRootDirectoryW, "EAIOTest/", EA::IO::kMaxPathLength);

    #elif defined(EA_PLATFORM_FREEBOX)
        EA::StdC::Strlcpy(gRootDirectory8,  "/tmp/EAIOTest/", EA::IO::kMaxPathLength);
        EA::StdC::Strlcpy(gRootDirectoryW, L"/tmp/EAIOTest/", EA::IO::kMaxPathLength);

    #elif defined(EA_PLATFORM_UNIX)
        // These paths were changed from "/tmp/..." to "./tmp/..." as creating temporary files 
        // in /tmp causes difficulties if the test harness crashes and these files are not
        // cleaned up. As the permissions are set by the creating user this makes it difficult
        // to run the same tests as another user on the same computer. 
        EA::StdC::Strlcpy(gRootDirectory8,  "./tmp/EAIOTest/", EA::IO::kMaxPathLength);
        EA::StdC::Strlcpy(gRootDirectoryW, L"./tmp/EAIOTest/", EA::IO::kMaxPathLength);

    #elif defined(__APPLE__)
        EA::IO::GetTempDirectory(gRootDirectory8, EA::IO::kMaxPathLength);
        EA::IO::GetTempDirectory(gRootDirectoryW, EA::IO::kMaxPathLength);
        
        EA::IO::Path::EnsureTrailingSeparator(gRootDirectory8, EA::IO::kMaxPathLength);
        EA::StdC::Strlcat(gRootDirectory8, "EAIOTest/", EA::IO::kMaxPathLength);
        EA::IO::Path::EnsureTrailingSeparator(gRootDirectoryW, EA::IO::kMaxPathLength);
        EA::StdC::Strlcat(gRootDirectoryW, "EAIOTest/", EA::IO::kMaxPathLength);

    #elif defined(EA_PLATFORM_PLAYBOOK)
        //The '.' signifies the Application's tmp dir (writeable) rather than the system's (readonly)
        //This notation is ok, since all playbook applications must use './' to refer to the application
        //root directory
        EA::StdC::Strlcpy(gRootDirectory8,  "./tmp/EAIOTest/", EA::IO::kMaxPathLength);
        EA::StdC::Strlcpy(gRootDirectoryW, L"./tmp/EAIOTest/", EA::IO::kMaxPathLength);

    #elif defined(EA_PLATFORM_PSP2)
        EA::StdC::Strlcpy(gRootDirectory8,  "app0:./EAIOTest/", EA::IO::kMaxPathLength);
        EA::StdC::Strlcpy(gRootDirectoryW, L"app0:./EAIOTest/", EA::IO::kMaxPathLength);

    #else
        EA::StdC::Strlcpy(gRootDirectory8,  "/EAIOTest/", EA::IO::kMaxPathLength);
        EA::StdC::Strlcpy(gRootDirectoryW, L"/EAIOTest/", EA::IO::kMaxPathLength);

    #endif

    //Ensure that we are working with a clean test directory
    
    EA::IO::Directory::Remove(gRootDirectory8, true);
    bool testDirExists = EA::IO::Directory::Create(gRootDirectory8);
    EATEST_VERIFY(testDirExists);

    // Get app temp directory
    char8_t tempDirectory[EA::IO::kMaxPathLength];
    EA::IO::GetTempDirectory(tempDirectory, EA::IO::kMaxPathLength);
    bool tmpDirExists = EA::IO::Directory::EnsureExists(tempDirectory);
    EATEST_VERIFY(tmpDirExists);

    return nErrorCount;
}

int  TeardownDirectories()
{
    int nErrorCount = 0;

    EA::IO::Directory::Remove(gRootDirectory8, true);
    bool testDirExists = EA::IO::Directory::Exists(gRootDirectory8);
    EATEST_VERIFY(!testDirExists);

    return nErrorCount;
}


int InitFileSystem()
{
    int nErrorCount = 0;

    EA::IO::SetAllocator(EA::Allocator::ICoreAllocator::GetDefaultAllocator());
    EA::IO::InitializeFileSystem(true);

    return nErrorCount;
}


int ShutdownFileSystem()
{
    int nErrorCount = 0;
    return nErrorCount;
}


int EATestMain(int argc, char** argv)
{  
    int nErrorCount(0);

    using namespace EA::UnitTest;

    #if defined(EA_DLL) && defined(EA_MEMORY_ENABLED) && EA_MEMORY_ENABLED
        EA::Allocator::InitSharedDllAllocator();
    #endif

    nErrorCount += InitFileSystem();
    nErrorCount += SetupDirectories();

    if(nErrorCount)
        EA::UnitTest::Report("Initialization error.\n");

    PlatformStartup();

    // Add the tests
    TestApplication testSuite("EAIO Unit Tests", argc, argv);

    testSuite.AddTest("FileBase",         TestFileBase);
    testSuite.AddTest("IniFile",          TestIniFile);
    testSuite.AddTest("PathString",       TestPathString);
    testSuite.AddTest("FnMatch",          TestFnMatch);
    testSuite.AddTest("EAStreamCpp",      TestEAStreamCpp);
    testSuite.AddTest("Stream",           TestStream);
    testSuite.AddTest("TextEncoding",     TestTextEncoding);
    testSuite.AddTest("File",             TestFile);
    testSuite.AddTest("FileNotification", TestFileNotification);
    testSuite.AddTest("FilePath",         TestFilePath);

    // Parse command line arguments
    CommandLine   commandLine(argc, argv);
    eastl::string sResult;

    if((commandLine.FindSwitch("-verbose", false, &sResult) >= 0) || 
       (commandLine.FindSwitch("-v",       false, &sResult) >= 0))
    {
        gbVerboseOutput = (sResult != "no");
    }
    
    nErrorCount += testSuite.Run();
    nErrorCount += TeardownDirectories();

    if(ShutdownFileSystem() != 0)
    {
        EA::UnitTest::Report("Shutdown error.\n");
        nErrorCount++;
    }

    PlatformShutdown(nErrorCount);
 
    return nErrorCount;
}




