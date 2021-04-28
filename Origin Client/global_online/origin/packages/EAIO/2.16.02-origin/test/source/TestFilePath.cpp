/////////////////////////////////////////////////////////////////////////////
// TestPathString.cpp
//
// Copyright (c) 2011, Electronic Arts Inc. All rights reserved.
//
/////////////////////////////////////////////////////////////////////////////


#include <EAIO/internal/Config.h>
#include <EAIOTest/EAIO_Test.h>
#include <EAStdC/EAString.h>
#include <EATest/EATest.h>
#include <stdio.h>
#include EA_ASSERT_HEADER

#if EAIO_FILEPATH_ENABLED
    #include <EAIO/compat/EAFilePath.h>
#endif



#if EAIO_FILEPATH_ENABLED

static bool TestResolvePath(const char16_t* pPath, const char16_t* pBaseDirectory, const char16_t* pExpectedResult) 
{
    char16_t pathResult[EA::IO::kMaxPathLength];

    EA::IO::ResolvePath(pathResult, pPath, pBaseDirectory, EA::IO::kFileSystemDefault);

    return (EA::StdC::Strcmp(pathResult, pExpectedResult) == 0);
}

static bool TestFullPath(const char16_t* pPath, const char16_t* pBaseDirectory, const char16_t* pExpectedResult) 
{
    char16_t pathResult[EA::IO::kMaxPathLength];

    EA::IO::FullPath(pathResult, pPath, pBaseDirectory, EA::IO::kFileSystemDefault);

    return (EA::StdC::Strcmp(pathResult, pExpectedResult) == 0);
}

#endif



///////////////////////////////////////////////////////////////////////////////
// TestFilePath
//
int TestFilePath()
{
    using namespace EA::IO;

    int nErrorCount = 0;

    #if EAIO_FILEPATH_ENABLED
        // char16_t* ResolvePath(char16_t* pPathResult, const char16_t* pPath, const char16_t* pBaseDirectory, FileSystem fileSystemType = kFileSystemDefault);
        // char16_t* FullPath(char16_t* pPathResult, const char16_t* pPath, const char16_t* pBaseDirectory, FileSystem fileSystemType = kFileSystemDefault);

        #if defined(EA_FILE_PATH_SEPARATOR_TYPE_UNIX) && EA_FILE_PATH_SEPARATOR_TYPE_UNIX
            EATEST_VERIFY(TestResolvePath(EA_CHAR16("../data2"), EA_CHAR16("/vol/content/data/"), EA_CHAR16("/vol/content/data2")));
            EATEST_VERIFY(TestFullPath(EA_CHAR16("../data2"), EA_CHAR16("/vol/content/data/"), EA_CHAR16("/vol/content/data2")));
        #else
            EATEST_VERIFY(TestResolvePath(EA_CHAR16("..\\data2"), EA_CHAR16("C:\\vol\\content\\data\\"), EA_CHAR16("C:\\vol\\content\\data2")));
            EATEST_VERIFY(TestFullPath(EA_CHAR16("..\\data2"), EA_CHAR16("C:\\vol\\content\\data\\"), EA_CHAR16("C:\\vol\\content\\data2")));
        #endif

    #endif // EAIO_FILEPATH_ENABLED

    return nErrorCount;
}





