///////////////////////////////////////////////////////////////////////////////
// TestFile.cpp
//
// Copyright (c) 2003 Electronic Arts. All rights reserved.
//
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#include <EABase/eabase.h>

#include <EAIO/internal/Config.h>
#include <EAIO/EAStream.h>
#include <EAIO/EAFileStream.h>
#include <EAIO/EAFileDirectory.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/FnEncode.h>
#include <EAIO/PathString.h>
#include <EASTL/string.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EADateTime.h>
#include <EAIOTest/EAIO_Test.h>
#include <EATest/EATest.h>
#include <coreallocator/icoreallocator_interface.h>
#include EA_ASSERT_HEADER

#if defined(EA_PLATFORM_REVOLUTION)
    #include <EAIO/Wii/EAFileStreamWiiNand.h>
    #include <EAIO/Wii/EAFileStreamWiiHio2.h>
#endif

#ifdef _MSC_VER
    #pragma warning(push, 0)
#endif

#include <stdio.h>
#include <limits.h>
#if defined(EA_PLATFORM_MICROSOFT)
    #include <crtdbg.h>
#endif

#if defined(EA_PLATFORM_MICROSOFT)
    #if defined(EA_PLATFORM_XENON)
        #include <xtl.h>
    #else
        #if !(defined(EA_PLATFORM_WINDOWS) && !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP))
            #include <WinSock2.h>
        #endif
        #include <windows.h>
    #endif
#elif defined(EA_PLATFORM_PS3)
    #include <cell/cell_fs.h>
#endif

#ifdef _MSC_VER
    #pragma warning(pop)
#endif


namespace TestLocal
{
    /// is_filled
    ///
    /// Returns true if all elements in the range [first, last] are equal to value.
    ///
    template <typename ForwardIterator, typename T>
    bool is_filled(ForwardIterator first, ForwardIterator last, const T& value)
    {
        while((first != last) && (*first == value))
            ++first;

        return (first == last);
    }


    // Writes the time_t value into pBuffer as local time.
    // pBuffer must hold at least 64 characters.
    // This function is thread-safe only if you pass in your own pBuffer.
    char* FormatTimeAsString(time_t t, char* pBuffer = NULL)
    {
        static char buffer[64];

        if(!pBuffer)
            pBuffer = buffer;

        #ifdef EA_PLATFORM_REVOLUTION
            OSCalendarTime ct;
            OSTicksToCalendarTime(t, &ct);
            sprintf(pBuffer, "%u/%u/%u %u:%u:%u", (uint32_t)ct.year, (uint32_t)ct.mon + 1, (uint32_t)ct.mday, (uint32_t)ct.hour, (uint32_t)ct.min, (uint32_t)ct.sec);
        #else
            struct tm* const pTime = localtime(&t);
            sprintf(pBuffer, "%u/%u/%u %u:%u:%u", (uint32_t)pTime->tm_year + 1900, (uint32_t)pTime->tm_mon + 1, (uint32_t)pTime->tm_mday, (uint32_t)pTime->tm_hour, (uint32_t)pTime->tm_min, (uint32_t)pTime->tm_sec);
        #endif

        return pBuffer;
    }

} // namespace



///////////////////////////////////////////////////////////////////////////////
// CreateExpectedFile
//
// This function creates an 'expected' file, which is a file that has a certain
// pattern of bytes. The reason for this pattern of bytes is that we want to verify
// copying, renaming, and moving of files and a way to do this is to verify that
// the destination file is the right size and byte pattern.
//
// The byte pattern we use is simply:
//     0x00 0x01 0x02 0x03 0x04... 0xEE 0xEF 0x00 0x01 0x02...
//
static bool CreateExpectedFile(const wchar_t* pFilePath, EA::IO::size_type expectedFileSize)
{
    bool bReturnValue = false;
    EA::IO::FileStream fileStream(pFilePath);

    if(fileStream.Open(EA::IO::kAccessFlagWrite, EA::IO::kCDCreateAlways, 0, 0)) // Create if non-existent; resize to zero if existent.
    {
        bReturnValue = true;

        for(EA::IO::size_type i = 0, iWrite = 0; (i < expectedFileSize) && bReturnValue; i++, iWrite++)
        {
            if(iWrite == (0xEF + 1))
                iWrite = 0;

            uint8_t c = (uint8_t)iWrite;
            bReturnValue = fileStream.Write(&c, 1);
        }
    }

    return bReturnValue;
}


///////////////////////////////////////////////////////////////////////////////
// ValidateExpectedFile
//
// This function verifies an 'expected' file of the given size. See the
// CreateExpectedFile function for an explanation of an 'expected' file.
//
static bool ValidateExpectedFile(const wchar_t* pFilePath, EA::IO::size_type expectedFileSize)
{
    bool bReturnValue = false;
    EA::IO::FileStream fileStream(pFilePath);

    if(fileStream.Open(EA::IO::kAccessFlagRead, EA::IO::kCDOpenExisting, 0, 0))
    {
        bReturnValue = true;

        for(EA::IO::size_type i = 0, iExpected = 0; (i < expectedFileSize) && bReturnValue; i++, iExpected++)
        {
            if(iExpected == (0xEF + 1))
                iExpected = 0;

            uint8_t c = 0xFF;
            if(fileStream.Read(&c, 1) == 1)
                bReturnValue = (c == (uint8_t)iExpected);
            else
                bReturnValue = false;
        }
    }

    return bReturnValue;
}



///////////////////////////////////////////////////////////////////////////////
// TestFileUtil
//
static int TestFileUtil()
{
    using namespace EA::IO;

    EA::UnitTest::Report("    TestFileUtil\n");

    int             nErrorCount(0);
    wchar_t         pDirectoryW[kMaxDirectoryLength];
    char8_t         pDirectory8[kMaxDirectoryLength];
    eastl::wstring  sTempW;
    eastl::string8  sTemp8;
    eastl::wstring  sLongFileNameW;
    eastl::string8  sLongFileName8;
    size_type       n;
    int             i;
    uint64_t        g = 0;
    bool            b;
    time_t          t;
    const float     fDiskSpeed = EA::UnitTest::GetSystemSpeed(EA::UnitTest::kSpeedTypeDisk);

    {
        sLongFileNameW.resize(kMaxPathLength * 4);
        sLongFileName8.resize(kMaxPathLength * 4);

        for(eastl_size_t f = 0, fEnd = sLongFileNameW.length(); f < fEnd; f++)
        {
            sLongFileNameW[f] = 'a';
            sLongFileName8[f] = 'a';
        }
    }

    {
        // bool EA::IO::File::Exists(const char8_t* pPath);
        // bool EA::IO::File::Exists(const char16_t* pPath);
        // bool EA::IO::File::Exists(const char32_t* pPath);

        // bool EA::IO::File::PatternExists(const char8_t* pPathPattern); 
        // bool EA::IO::File::PatternExists(const char16_t* pPathPattern); 
        // bool EA::IO::File::PatternExists(const char32_t* pPathPattern); 

        // bool EA::IO::File::Create(const char8_t* pPath, bool bTruncate = false);
        // bool EA::IO::File::Create(const char16_t* pPath, bool bTruncate = false);
        // bool EA::IO::File::Create(const char32_t* pPath, bool bTruncate = false);

        // bool EA::IO::File::Remove(const char8_t* pPath);
        // bool EA::IO::File::Remove(const char16_t* pPath);
        // bool EA::IO::File::Remove(const char32_t* pPath);

        // Test empty paths
        b = File::Exists(EA_WCHAR(""));
        EATEST_VERIFY(!b);
        b = File::Exists("");
        EATEST_VERIFY(!b);

        #if EAIOTEST_APPLICATION_BUNDLES_SUPPORTED
            if(gbVerboseOutput)
                EA::UnitTest::Report("Testing appbundle exists\n");
            b = File::Exists(EA_WCHAR("appbundle:/filea.txt"));
            EATEST_VERIFY(b);
            b = File::Exists("appbundle:/filea.txt");
            EATEST_VERIFY(b);
            b = File::Exists(EA_WCHAR("appbundle:/missing_file.txt"));
            EATEST_VERIFY(!b);
            b = File::Exists("appbundle:/missing_file.txt");
            EATEST_VERIFY(!b);
            
            b = Directory::Exists(EA_WCHAR("appbundle:/subfolder/"));
            EATEST_VERIFY(b);
            b = Directory::Exists("appbundle:/subfolder/");
            EATEST_VERIFY(b);
            b = Directory::Exists("appbundle:/non_existant_folder/");
            EATEST_VERIFY(!b);
        #endif

        b = File::PatternExists(EA_WCHAR("")); 
        EATEST_VERIFY(!b);
        b = File::PatternExists(""); 
        EATEST_VERIFY(!b);

        b = File::Create(EA_WCHAR(""), true);
        EATEST_VERIFY(!b);
        b = File::Create("", true);
        EATEST_VERIFY(!b);

        b = File::Create(EA_WCHAR(""), false);
        EATEST_VERIFY(!b);
        b = File::Create("", false);
        EATEST_VERIFY(!b);

        b = File::Remove(EA_WCHAR(""));
        EATEST_VERIFY(!b);
        b = File::Remove("");
        EATEST_VERIFY(!b);

        // Test known-nonexistant paths
        b = File::Exists(EA_WCHAR("asdfasdfasdf.asdf"));
        EATEST_VERIFY(!b);
        b = File::Exists("asdfasdfasdf.asdf");
        EATEST_VERIFY(!b);

        b = File::PatternExists(EA_WCHAR("asdfasdfasdf.asdf")); 
        EATEST_VERIFY(!b);
        b = File::PatternExists("asdfasdfasdf.asdf"); 
        EATEST_VERIFY(!b);

        b = File::Remove(EA_WCHAR("asdfasdfasdf.asdf"));
        EATEST_VERIFY(!b);
        b = File::Remove("asdfasdfasdf.asdf");
        EATEST_VERIFY(!b);


        // Test paths that are too long to be valid.
        sTempW = gRootDirectoryW; sTempW += sLongFileNameW;
        sTemp8  = gRootDirectory8;  sTemp8  += sLongFileName8;

        b = File::Exists(sTempW.c_str());
        EATEST_VERIFY(!b);
        b = File::Exists(sTemp8.c_str());
        EATEST_VERIFY(!b);

        b = File::PatternExists(sTempW.c_str()); 
        EATEST_VERIFY(!b);
        b = File::PatternExists(sTemp8.c_str()); 
        EATEST_VERIFY(!b);

        b = File::Create(sTempW.c_str(), true);
        EATEST_VERIFY(!b);
        b = File::Create(sTemp8.c_str(), true);
        EATEST_VERIFY(!b);

        b = File::Create(sTempW.c_str(), false);
        EATEST_VERIFY(!b);
        b = File::Create(sTemp8.c_str(), false);
        EATEST_VERIFY(!b);

        b = File::Remove(sTempW.c_str());
        EATEST_VERIFY(!b);
        b = File::Remove(sTemp8.c_str());
        EATEST_VERIFY(!b);


        // Test some file that is known to exist.
        sTempW = gRootDirectoryW; sTempW += EA_WCHAR("EAIO test file16.txt");  // The code below depends on this being at least a certain length.
        sTemp8  = gRootDirectory8;  sTemp8  +=  "EAIO test file8.txt";

        if(File::Create(sTempW.c_str(), true) && 
           File::Create(sTemp8.c_str(), true))
        {
            b = File::Exists(sTempW.c_str());
            EATEST_VERIFY(b);
            b = File::Exists(sTemp8.c_str());
            EATEST_VERIFY(b);
    
            int attributes = File::GetAttributes(sTempW.c_str());
            #if !defined(EA_PLATFORM_PSP2) // For some reason this is failing on this platform. To do: find out what's going on.
                EATEST_VERIFY((attributes & (kAttributeReadable | kAttributeWritable)) == (kAttributeReadable | kAttributeWritable));
            #endif
            
            attributes = File::GetAttributes(sTemp8.c_str());
            #if !defined(EA_PLATFORM_PSP2) // For some reason this is failing on this platform. To do: find out what's going on.
                EATEST_VERIFY((attributes & (kAttributeReadable | kAttributeWritable)) == (kAttributeReadable | kAttributeWritable));
            #endif

            b = File::PatternExists(sTempW.c_str()); 
            EATEST_VERIFY(b);
            b = File::PatternExists(sTemp8.c_str()); 
            EATEST_VERIFY(b);

            eastl::wstring sTempPatternW(sTempW); sTempPatternW.resize(sTempW.size() - 8); sTempPatternW += '*';
            eastl::string8 sTempPattern8(sTemp8); sTempPattern8.resize(sTemp8.size() - 8); sTempPattern8 += '*';

            b = File::PatternExists(sTempPatternW.c_str()); 
            EATEST_VERIFY(b);
            b = File::PatternExists(sTempPattern8.c_str()); 
            EATEST_VERIFY(b);
        }

        if(File::Exists(sTempW.c_str()))
        {
            b = File::Remove(sTempW.c_str());
            EATEST_VERIFY(b);
        }

        if(File::Exists(sTemp8.c_str()))
        {
            b = File::Remove(sTemp8.c_str());
            EATEST_VERIFY(b);
        }
    }

    {
        // bool EA::IO::File::Move(const char8_t*  pPathSource,  const char8_t*  pPathDestination, bool bOverwriteIfPresent = true);
        // bool EA::IO::File::Move(const char16_t* pPathSource,  const char16_t* pPathDestination, bool bOverwriteIfPresent = true);
        // bool EA::IO::File::Move(const char32_t* pPathSource,  const char16_t* pPathDestination, bool bOverwriteIfPresent = true);

        // bool EA::IO::File::Rename(const char8_t*  pPathSource,  const char8_t*  pPathDestination, bool bOverwriteIfPresent = true);
        // bool EA::IO::File::Rename(const char16_t* pPathSource,  const char16_t* pPathDestination, bool bOverwriteIfPresent = true);
        // bool EA::IO::File::Rename(const char32_t* pPathSource,  const char32_t* pPathDestination, bool bOverwriteIfPresent = true);

        // bool EA::IO::File::Copy(const char8_t*  pPathSource,  const char8_t*  pPathDestination, bool bOverwriteIfPresent = true);
        // bool EA::IO::File::Copy(const char16_t* pPathSource,  const char16_t* pPathDestination, bool bOverwriteIfPresent = true);
        // bool EA::IO::File::Copy(const char32_t* pPathSource,  const char32_t* pPathDestination, bool bOverwriteIfPresent = true);

        // Test empty file paths
        b = File::Move(EA_WCHAR(""), EA_WCHAR(""), false);
        EATEST_VERIFY(!b);

        b = File::Rename(EA_WCHAR(""), EA_WCHAR(""), false);
        EATEST_VERIFY(!b);

        b = File::Copy(EA_WCHAR(""), EA_WCHAR(""), false);
        EATEST_VERIFY(!b);


        // Test non-existant source file paths.
        sTempW = gRootDirectoryW; sTempW += EA_WCHAR("EAIO test file16.txt");  // The code below depends on this being at least a certain length.
        sTemp8  = gRootDirectory8;  sTemp8  +=  "EAIO test file8.txt";

        sLongFileNameW = sTempW + 'x';
        sLongFileName8  = sTemp8 + 'x';

        File::Remove(sTempW.c_str());          // Make sure they don't exist.
        File::Remove(sTemp8.c_str());
        File::Remove(sLongFileNameW.c_str());
        File::Remove(sLongFileName8.c_str());

        EATEST_VERIFY(!File::Exists(sTempW.c_str()));
        EATEST_VERIFY(!File::Exists(sTemp8.c_str()));
        EATEST_VERIFY(!File::Exists(sLongFileNameW.c_str()));
        EATEST_VERIFY(!File::Exists(sLongFileName8.c_str()));

        b = File::Copy(sTempW.c_str(), sLongFileNameW.c_str(), true);
        EATEST_VERIFY(!b);
        b = File::Copy(sTemp8.c_str(),  sLongFileName8.c_str(),  true);
        EATEST_VERIFY(!b);

        // To do: Verify that the copied contents are equivalent, and do this for multiple file sizes.

        b = File::Move(sTempW.c_str(), sLongFileNameW.c_str(), true);
        EATEST_VERIFY(!b);
        b = File::Move(sTemp8.c_str(),  sLongFileName8.c_str(),  true);
        EATEST_VERIFY(!b);

        // To do: Verify that the moved contents are equivalent, and do this for multiple file sizes.

        b = File::Rename(sTempW.c_str(), sLongFileNameW.c_str(), true);
        EATEST_VERIFY(!b);
        b = File::Rename(sTemp8.c_str(),  sLongFileName8.c_str(),  true);
        EATEST_VERIFY(!b);

        // Do some tests of 'expected' file content.
        const EA::IO::size_type sizeArray[] = { 1, 40, 256, 1000, 2700 };

        for(EA::IO::size_type j = 0; j < EAArrayCount(sizeArray); j++)
        {
            if((sizeArray[j] > 100) && (fDiskSpeed < 0.1f)) // If this test will take too long due to terrible debug disk speed...
                continue;

            wchar_t filePath1[EA::IO::kMaxPathLength];
            wchar_t filePath2[EA::IO::kMaxPathLength];

            EA::StdC::Strcpy(filePath1, gRootDirectoryW);
            EA::StdC::Strcat(filePath1, EA_WCHAR("EAIO expected file 1.txt"));
            EA::StdC::Strcpy(filePath2, gRootDirectoryW);
            EA::StdC::Strcat(filePath2, EA_WCHAR("EAIO expected file 2.txt"));

            b = CreateExpectedFile(filePath1, sizeArray[j]);
            EATEST_VERIFY(b);
            b = ValidateExpectedFile(filePath1, sizeArray[j]);
            EATEST_VERIFY(b);
            b = File::Copy(filePath1, filePath2, true);
            EATEST_VERIFY(b);
            b = ValidateExpectedFile(filePath2, sizeArray[j]);
            EATEST_VERIFY(b);
            File::Remove(filePath2);

            b = File::Move(filePath1, filePath2, true);
            EATEST_VERIFY(b);
            b = ValidateExpectedFile(filePath2, sizeArray[j]);
            EATEST_VERIFY(b);

            b = File::Rename(filePath2, filePath1, true);
            EATEST_VERIFY(b);
            b = ValidateExpectedFile(filePath1, sizeArray[j]);
            EATEST_VERIFY(b);
            File::Remove(filePath1);
        }

        // Test valid file paths.
        if(File::Create(sTempW.c_str(), true) && 
           File::Create(sTemp8.c_str(), true))
        {
            eastl::wstring sMoveDestinationDirW(gRootDirectoryW); sMoveDestinationDirW += EA_WCHAR("DirW"); sMoveDestinationDirW += EA_FILE_PATH_SEPARATOR_16;
            eastl::string8 sMoveDestinationDir8(gRootDirectory8); sMoveDestinationDir8 +=          "Dir";  sMoveDestinationDir8 += EA_FILE_PATH_SEPARATOR_8;

            if(Directory::Create(sMoveDestinationDirW.c_str()) && 
               Directory::Create(sMoveDestinationDir8.c_str()))
            {
                eastl::wstring sMoveDestinationW(sMoveDestinationDirW); sMoveDestinationW += EA_WCHAR("Test16.txt"); 
                eastl::string8 sMoveDestination8(sMoveDestinationDir8); sMoveDestination8 +=          "Test8.txt"; 

                // Move it to a subdirectory.
                b = File::Move(sTempW.c_str(), sMoveDestinationW.c_str(), true);
                EATEST_VERIFY(b);
                b = File::Move(sTemp8.c_str(), sMoveDestination8.c_str(), true);
                EATEST_VERIFY(b);

                // Make sure the old version is gone.
                b = File::Exists(sTempW.c_str());
                EATEST_VERIFY(!b);
                b = File::Exists(sTemp8.c_str());
                EATEST_VERIFY(!b);

                // Make sure the new version is present.
                b = File::Exists(sMoveDestinationW.c_str());
                EATEST_VERIFY(b);
                b = File::Exists(sMoveDestination8.c_str());
                EATEST_VERIFY(b);

                // Rename it.
                eastl::wstring sRenamedW(sMoveDestinationW); sRenamedW += EA_WCHAR(".xyz");
                eastl::string8 sRenamed8(sMoveDestination8); sRenamed8 +=  ".xyz";

                b = File::Rename(sMoveDestinationW.c_str(), sRenamedW.c_str(), true);
                EATEST_VERIFY(b);
                b = File::Rename(sMoveDestination8.c_str(), sRenamed8.c_str(), true);
                EATEST_VERIFY(b);

                // Make sure the new version is present.
                b = File::Exists(sRenamedW.c_str());
                EATEST_VERIFY(b);
                b = File::Exists(sRenamed8.c_str());
                EATEST_VERIFY(b);

                // Copy it back to the original location with the original name.
                b = File::Copy(sRenamedW.c_str(), sTempW.c_str(), true);
                EATEST_VERIFY(b);
                b = File::Copy(sRenamed8.c_str(), sTemp8.c_str(), true);
                EATEST_VERIFY(b);

                // Delete the renamed source file.
                b = File::Remove(sRenamedW.c_str());
                EATEST_VERIFY(b);
                b = File::Remove(sRenamed8.c_str());
                EATEST_VERIFY(b);

                // Make sure the removed files are removed.
                b = File::Exists(sRenamedW.c_str());
                EATEST_VERIFY(!b);
                b = File::Exists(sRenamed8.c_str());
                EATEST_VERIFY(!b);
            }
        }

        if(File::Exists(sTempW.c_str()))
        {
            b = File::Remove(sTempW.c_str());
            EATEST_VERIFY(b);
        }

        if(File::Exists(sTemp8.c_str()))
        {
            b = File::Remove(sTemp8.c_str());
            EATEST_VERIFY(b);
        }
    }

    {
        // size_type EA::IO::File::GetSize(const char16_t* pPath);
        // bool EA::IO::File::IsWritable(const char16_t* pPath);
        // int EA::IO::File::GetAttributes(const char16_t* pPath);
        // bool EA::IO::File::SetAttributes(const char16_t* pPath, int nAttributeMask, bool bEnable);
        // time_t EA::IO::File::GetTime(const char16_t* pPath, FileTimeType timeType);
        // bool EA::IO::File::SetTime(const char16_t* pPath, int nFileTimeTypeFlags, time_t nTime); 

        // size_type EA::IO::File::GetSize(const char8_t* pPath);
        // bool EA::IO::File::IsWritable(const char8_t* pPath);
        // int EA::IO::File::GetAttributes(const char8_t* pPath);
        // bool EA::IO::File::SetAttributes(const char8_t* pPath, int nAttributeMask, bool bEnable);
        // time_t EA::IO::File::GetTime(const char8_t* pPath, FileTimeType timeType);
        // bool EA::IO::File::SetTime(const char8_t* pPath, int nFileTimeTypeFlags, time_t nTime); 

        // Test empty paths.
        n = File::GetSize(EA_WCHAR(""));
        EATEST_VERIFY(n == EA::IO::kSizeTypeError);

        b = File::IsWritable(EA_WCHAR(""));
        EATEST_VERIFY(!b);

        i = File::GetAttributes(EA_WCHAR(""));
        EATEST_VERIFY(i == 0);

        b = File::SetAttributes(EA_WCHAR(""), 0, true);
        EATEST_VERIFY(!b);

        t = File::GetTime(EA_WCHAR(""), EA::IO::kFileTimeTypeNone);
        EATEST_VERIFY(t == 0);

        b = File::SetTime(EA_WCHAR(""), 0, 0); 
        EATEST_VERIFY(!b);
    }

    {
        // ResolveAliasResult EA::IO::File::ResolveAlias(const char16_t* pPathSource, char16_t* pPathDestination);
        // bool EA::IO::File::CreateAlias(const char16_t* pDestinationPath, const char16_t* pShortcutPath, 
        //                               const char16_t* pShortcutDescription, const char16_t* pShortcutArguments);

        // ResolveAliasResult EA::IO::File::ResolveAlias(const char8_t* pPathSource, char8_t* pPathDestination);
        // bool EA::IO::File::CreateAlias(const char8_t* pDestinationPath, const char8_t* pShortcutPath, 
        //                               const char8_t* pShortcutDescription, const char8_t* pShortcutArguments);

        // Test empty paths.
        EA::IO::File::ResolveAliasResult r = File::ResolveAlias(EA_WCHAR(""), pDirectoryW, kMaxDirectoryLength);
        EATEST_VERIFY(r == EA::IO::File::kRARInvalid); // kRARNone, kRARAlias

        b = File::CreateAlias(EA_WCHAR(""), EA_WCHAR(""), EA_WCHAR(""), EA_WCHAR(""));
        EATEST_VERIFY(!b);

        // Windows supports explicit aliases (.lnk files) and implicit aliases (junctions).
        // Unix supports implicit aliases (symbolic links and hard links).
        // Mac supports implicit aliases (symbolic links and hard links) and smart aliases (called simply "aliases" by Apple).
        // Console platforms support no kind of file system aliases.
        //
        // As of this writing, we have support only for Windows explicit aliases.
        // 
        #ifdef EA_PLATFORM_WINDOWS
            // To do.
        #endif
    }

    {
        // bool EA::IO::Directory::Exists(const char16_t* pDirectoryW);
        // bool EA::IO::Directory::EnsureExists(const char16_t* pDirectoryW);
        // bool EA::IO::Directory::Create(const char16_t* pDirectoryW);
        // bool EA::IO::Directory::Remove(const char16_t* pDirectoryW, bool bAllowRecursiveRemoval = true);
        // bool EA::IO::Directory::Rename(const char16_t* pDirectoryOld, const char16_t* pDirectoryNew);

        // bool EA::IO::Directory::Exists(const char8_t* pDirectoryW);
        // bool EA::IO::Directory::EnsureExists(const char8_t* pDirectoryW);
        // bool EA::IO::Directory::Create(const char8_t* pDirectoryW);
        // bool EA::IO::Directory::Remove(const char8_t* pDirectoryW, bool bAllowRecursiveRemoval = true);
        // bool EA::IO::Directory::Rename(const char8_t* pDirectoryOld, const char8_t* pDirectoryNew);

        // Test empty paths.
        b = Directory::Exists(EA_WCHAR(""));
        EATEST_VERIFY(!b);

        b = Directory::EnsureExists(EA_WCHAR(""));
        EATEST_VERIFY(!b);

        #ifndef EA_PLATFORM_AIRPLAY // Why is this disabled? Can we get a comment describing this?
            b = Directory::Create(EA_WCHAR(""));
            EATEST_VERIFY(!b);

            b = Directory::Remove(EA_WCHAR(""), true);
            EATEST_VERIFY(!b);

            b = Directory::Rename(EA_WCHAR(""), EA_WCHAR(""));
            EATEST_VERIFY(!b);
        #endif
    }

    {
        using namespace EA::IO::Path;
        // bool EA::IO::Directory::Copy(const char16_t* pDirectorySource, const char16_t* pDirectoryDestination, bool bRecursive, bool bOverwriteIfAlreadyPresent = true);
        // bool EA::IO::Directory::Copy(const char8_t* pDirectorySource, const char8_t* pDirectoryDestination, bool bRecursive, bool bOverwriteIfAlreadyPresent = true);
        PathStringW pSrcDirW(gRootDirectoryW);
        EnsureTrailingSeparator(pSrcDirW);
        EA::IO::Path::Append(pSrcDirW, PathStringW(EA_WCHAR("CopyTestSrc")));

        PathStringW pCopyDirW(gRootDirectoryW);
        EnsureTrailingSeparator(pCopyDirW);

        PathStringW pInnerDirW(pSrcDirW);
        EnsureTrailingSeparator(pInnerDirW);
        EA::IO::Path::Append(pInnerDirW, PathStringW(EA_WCHAR("InnerDir")));
        
        PathStringW pFileW(pInnerDirW);
        EnsureTrailingSeparator(pFileW);
        EA::IO::Path::Append(pFileW, PathStringW(EA_WCHAR("Batman.txt")));

        EA::IO::Path::Append(pCopyDirW, PathStringW(EA_WCHAR("CopyTestDest")));

        PathString8 pSrcDir8;
        PathString8 pCopyDir8;
        PathString8 pInnerDir8;
        PathString8 pFile8;
        ConvertPath(pSrcDir8, pSrcDirW.c_str());
        ConvertPath(pCopyDir8, pCopyDirW.c_str());
        ConvertPath(pInnerDir8, pInnerDirW.c_str());
        ConvertPath(pFile8, pFileW.c_str());

        // Test empty paths.
        b = Directory::Copy(EA_WCHAR(""), EA_WCHAR(""), true, false);
        EATEST_VERIFY(!b);

        //Test copy, nonrecursive
        b = Directory::EnsureExists(pSrcDirW.c_str());
        EATEST_VERIFY(b);

        if(Directory::Exists(pCopyDirW.c_str()))
            Directory::Remove(pCopyDirW.c_str(), true);

        b = Directory::Copy(pSrcDirW.c_str(), pCopyDirW.c_str(), false, false);
        EATEST_VERIFY(b);
        EATEST_VERIFY(Directory::Exists(pCopyDirW.c_str()));

        b = Directory::Copy(pSrcDirW.c_str(), pCopyDirW.c_str(), false, true);
        EATEST_VERIFY(b);

        //Test copy, recursive

        //Create something to be copied recursively
        Directory::Create(pInnerDirW.c_str());
        File::Create(pFileW.c_str());

        if(Directory::Exists(pCopyDirW.c_str()))
            Directory::Remove(pCopyDirW.c_str(), true);

        b = Directory::Copy(pSrcDirW.c_str(), pCopyDirW.c_str(), true, false);
        EATEST_VERIFY(b);

        Directory::Remove(pCopyDirW.c_str(), true);
        b = Directory::Copy(pSrcDirW.c_str(), pCopyDirW.c_str(), false, false);
        EATEST_VERIFY(b);

        b = Directory::Copy(pSrcDirW.c_str(), pCopyDirW.c_str(), true, true);
        EATEST_VERIFY(b);

        //char8_t
         // Test empty paths.
        b = Directory::Copy("", "", true, false);
        EATEST_VERIFY(!b);

        //Test copy, nonrecursive
        b = Directory::EnsureExists(pSrcDir8.c_str());
        EATEST_VERIFY(b);

        if(Directory::Exists(pCopyDir8.c_str()))
            Directory::Remove(pCopyDir8.c_str(), true);

        b = Directory::Copy(pSrcDir8.c_str(), pCopyDir8.c_str(), false, false);
        EATEST_VERIFY(b);
        EATEST_VERIFY(Directory::Exists(pCopyDir8.c_str()));

        b = Directory::Copy(pSrcDir8.c_str(), pCopyDir8.c_str(), false, true);
        EATEST_VERIFY(b);

        //Test copy, recursive

        //Create something to be copied recursively
        Directory::Create(pInnerDir8.c_str());
        File::Create(pFile8.c_str());

        if(Directory::Exists(pCopyDir8.c_str()))
            Directory::Remove(pCopyDir8.c_str(), true);

        b = Directory::Copy(pSrcDir8.c_str(), pCopyDir8.c_str(), true, false);
        EATEST_VERIFY(b);

        Directory::Remove(pCopyDir8.c_str(), true);
        b = Directory::Copy(pSrcDir8.c_str(), pCopyDir8.c_str(), false, false);
        EATEST_VERIFY(b);

        b = Directory::Copy(pSrcDir8.c_str(), pCopyDir8.c_str(), true, true);
        EATEST_VERIFY(b);
    }

    {
        // int EA::IO::Directory::GetAttributes(const char16_t* pPath);
        // bool EA::IO::Directory::SetAttributes(const char16_t* pPath, int nAttributeMask, bool bEnable);
        // bool EA::IO::Directory::SetAttributes(const char16_t* pBaseDirectory, int nAttributeMask, bool bEnable, bool bRecursive, bool bIncludeBaseDirectory = true);
        // time_t EA::IO::File::GetTime(const char16_t* pPath, FileTimeType timeType);
        // bool EA::IO::File::SetTime(const char16_t* pPath, int nFileTimeTypeFlags, time_t nTime); 

        // int EA::IO::Directory::GetAttributes(const char8_t* pPath);
        // bool EA::IO::Directory::SetAttributes(const char8_t* pPath, int nAttributeMask, bool bEnable);
        // bool EA::IO::Directory::SetAttributes(const char8_t* pBaseDirectory, int nAttributeMask, bool bEnable, bool bRecursive, bool bIncludeBaseDirectory = true);
        // time_t EA::IO::File::GetTime(const char8_t* pPath, FileTimeType timeType);
        // bool EA::IO::File::SetTime(const char8_t* pPath, int nFileTimeTypeFlags, time_t nTime); 

        // Test empty paths.
        i = Directory::GetAttributes(EA_WCHAR(""));
        EATEST_VERIFY(i == 0);

        b = Directory::SetAttributes(EA_WCHAR(""), 0, false);
        EATEST_VERIFY(!b);

        b = Directory::SetAttributes(EA_WCHAR(""), 0, false, false, true);
        EATEST_VERIFY(!b);

        t = File::GetTime(EA_WCHAR(""), EA::IO::kFileTimeTypeNone);
        EATEST_VERIFY(t == 0);

        b = File::SetTime(EA_WCHAR(""), 0, 0);
        EATEST_VERIFY(!b);
    }

    {
        // int  EA::IO::Directory::GetCurrentWorkingDirectory(char16_t* pDirectoryW);
        // bool EA::IO::Directory::SetCurrentWorkingDirectory(const char16_t* pDirectoryW);

        // int  EA::IO::Directory::GetCurrentWorkingDirectory(char8_t* pDirectoryW);
        // bool EA::IO::Directory::SetCurrentWorkingDirectory(const char8_t* pDirectoryW);

        // Test empty paths.
        Directory::GetCurrentWorkingDirectory(pDirectoryW);
        Directory::SetCurrentWorkingDirectory(pDirectoryW);

        // Test user-reported bug
        char8_t bufferBefore[64];                       memset(bufferBefore,  '!',  sizeof(bufferBefore));
        char8_t inputDir[EA::IO::kMaxPathLength + 1];   memset(inputDir,     0xcc,  sizeof(inputDir));
        char8_t bufferAfter[64];                        memset(bufferAfter,   '!',  sizeof(bufferAfter));

        inputDir[EA::IO::kMaxPathLength] = '\0'; // Initialize to empty string.
        EA::IO::Directory::GetCurrentWorkingDirectory(inputDir, EA::IO::kMaxPathLength);

        EATEST_VERIFY(TestLocal::is_filled(bufferBefore, bufferBefore + 64, '!'));
        EATEST_VERIFY(TestLocal::is_filled(bufferAfter,  bufferAfter  + 64, '!'));

        eastl::fill(inputDir, inputDir + EA::IO::kMaxPathLength, ' '); // Initialize to really long string.
        inputDir[EA::IO::kMaxPathLength] = '\0';
        EA::IO::Directory::GetCurrentWorkingDirectory(inputDir, EA::IO::kMaxPathLength);

        EATEST_VERIFY(TestLocal::is_filled(bufferBefore, bufferBefore + 64, '!'));
        EATEST_VERIFY(TestLocal::is_filled(bufferAfter,  bufferAfter  + 64, '!'));        
    }


    { 
        // bool MakeTempPathName(char16_t* pDestPath, const char16_t* pDirectoryW = NULL, const char16_t* pFileName = NULL, const char16_t* pExtension = NULL, uint32_t nDestPathLength = kMaxPathLength);
        // bool MakeTempPathName(char8_t* pDestPath, const char8_t* pDirectoryW = NULL, const char8_t* pFileName = NULL, const char8_t* pExtension = NULL, uint32_t nDestPathLength = kMaxPathLength);

        // int GetTempDirectory(char16_t* pDirectoryW, uint32_t nPathLength = kMaxPathLength);
        // int GetTempDirectory(char8_t* pDirectoryW, uint32_t nPathLength = kMaxPathLength);

        // bool SetTempDirectory(const char16_t* pDirectoryW);
        // bool SetTempDirectory(const char8_t* pDirectoryW);

        // Temp file functionality
        {
            i = GetTempDirectory(pDirectoryW);
            EATEST_VERIFY((i > 0) && (*pDirectoryW != 0));

            #if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_UNIX)
                b = SetTempDirectory(pDirectoryW);
                EATEST_VERIFY(b);
            #endif

            #if !defined(EA_PLATFORM_REVOLUTION) && !defined(EA_PLATFORM_GAMECUBE) && !defined(EA_PLATFORM_IPHONE)// These platforms have standard file systems that are read-only.
                wchar_t pTempPath[kMaxPathLength];

                b = MakeTempPathName(pTempPath);
                EATEST_VERIFY(b && pTempPath[0]);
                if(b)
                    File::Remove(pTempPath);
        
                b = MakeTempPathName(pTempPath, pDirectoryW);
                EATEST_VERIFY(b && pTempPath[0]);
                if(b)
                    File::Remove(pTempPath);
        
                b = MakeTempPathName(pTempPath, NULL, EA_WCHAR("abc"));
                EATEST_VERIFY(b && pTempPath[0] && EA::StdC::Strstr(pTempPath, EA_WCHAR("abc")));
                if(b)
                    File::Remove(pTempPath);
        
                b = MakeTempPathName(pTempPath, NULL, EA_WCHAR("abc"), EA_WCHAR(".def"));
                EATEST_VERIFY(b && pTempPath[0] && EA::StdC::Strstr(pTempPath, EA_WCHAR("abc")) && EA::StdC::Strstr(pTempPath, EA_WCHAR("def")));
                if(b)
                    File::Remove(pTempPath);

                // Test the creation of many temp paths in a short period of time.
                const size_t kCount = 100;
                eastl::wstring pathArray[kCount];

                for(n = 0; n < kCount; n++)
                {
                    b = MakeTempPathName(pTempPath);
                    pathArray[n] = pTempPath;
                    EATEST_VERIFY(b && pTempPath[0]);
                }
                for(n = 0; n < kCount; n++)
                   File::Remove(pathArray[n].c_str());
            #endif
        }

        {
            i = GetTempDirectory(pDirectory8);
            EATEST_VERIFY((i > 0) && (*pDirectory8 != 0));

            #if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_UNIX)
                b = SetTempDirectory(pDirectory8);
                EATEST_VERIFY(b);
            #endif

            #if !defined(EA_PLATFORM_REVOLUTION) && !defined(EA_PLATFORM_GAMECUBE) && !defined(EA_PLATFORM_IPHONE)// These platforms have standard file systems that are read-only.
                char8_t pTempPath[kMaxPathLength];

                b = MakeTempPathName(pTempPath);
                EATEST_VERIFY(b && pTempPath[0]);
                if(b)
                    File::Remove(pTempPath);
        
                b = MakeTempPathName(pTempPath, pDirectory8);
                EATEST_VERIFY(b && pTempPath[0]);
                if(b)
                    File::Remove(pTempPath);
        
                b = MakeTempPathName(pTempPath, NULL, "abc");
                EATEST_VERIFY(b && pTempPath[0] && EA::StdC::Strstr(pTempPath, "abc"));
                if(b)
                    File::Remove(pTempPath);
        
                b = MakeTempPathName(pTempPath, NULL, "abc", ".def");
                EATEST_VERIFY(b && pTempPath[0] && EA::StdC::Strstr(pTempPath, "abc") && EA::StdC::Strstr(pTempPath, "def"));
                if(b)
                    File::Remove(pTempPath);

                // Test the creation of many temp paths in a short period of time.
                const size_t kCount = 100;
                eastl::string8 pathArray[kCount];

                for(n = 0; n < kCount; n++)
                {
                    b = MakeTempPathName(pTempPath);
                    pathArray[n] = pTempPath;
                    EATEST_VERIFY(b && pTempPath[0]);
                }
                for(n = 0; n < kCount; n++)
                   File::Remove(pathArray[n].c_str());
            #endif
        }

        #if defined(EA_PLATFORM_XENON)
            // Test problem with MakeTempPathName (actually with CreateFile(CREATE_NEW)) when XFileEnableTransparentDecompression is active.

            HRESULT hrTD = XFileEnableTransparentDecompression(NULL);

            char8_t pTempPathCache[kMaxPathLength];

            if(EA::IO::MakeTempPathName(pTempPathCache, "d:\\", "abc", ".def"))
                EA::IO::File::Remove(pTempPathCache);

            if(SUCCEEDED(hrTD))
                XFileDisableTransparentDecompression();
        #endif

        #if 0 // defined(EA_PLATFORM_XENON)
            // Test problem with CreateFile(CREATE_NEW) when XFileEnableTransparentDecompression is active.

            HRESULT hrTD = XFileEnableTransparentDecompression(NULL);

            if(GetFileAttributes("D:\\test.abc") != 0xffffffff) // If the file exists...
                DeleteFileA("D:\\test.abc");

            HANDLE hFile = CreateFile("D:\\test.abc", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 
                                        NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

            if(hFile == INVALID_HANDLE_VALUE)
            {
                OutputDebugString("CreateFile is mistaken.\n");

                if(GetFileAttributes("D:\\test.abc") != 0xffffffff) // If the file exists...
                    OutputDebugString("And yet the file was created anyway.\n");
            }
            else
                CloseHandle(hFile);

            if(GetFileAttributes("D:\\test.abc") != 0xffffffff) // If the file exists...
                DeleteFileA("D:\\test.abc");

            if(SUCCEEDED(hrTD))
                XFileDisableTransparentDecompression();
        #endif
    }

    {
        // uint64_t GetDriveFreeSpace(const char32_t* pPath);
        // uint64_t GetDriveFreeSpace(const char16_t* pPath);
        // uint64_t GetDriveFreeSpace(const char8_t* pPath);

        // int GetDriveName(const char32_t* pPath, char32_t* pName);
        // int GetDriveName(const char16_t* pPath, char16_t* pName);
        // int GetDriveName(const char8_t* pPath, char8_t* pName);

        // int GetDriveSerialNumber(const char32_t* pPath, char32_t* pSerialNumber);
        // int GetDriveSerialNumber(const char16_t* pPath, char16_t* pSerialNumber);
        // int GetDriveSerialNumber(const char8_t* pPath, char8_t* pSerialNumber);

        // DriveType GetDriveTypeValue(const char32_t* pPath);
        // DriveType GetDriveTypeValue(const char16_t* pPath);
        // DriveType GetDriveTypeValue(const char8_t* pPath);

        // bool IsVolumeAvailable(const char32_t* pPath, int timeoutMS = 2000);
        // bool IsVolumeAvailable(const char16_t* pPath, int timeoutMS = 2000);
        // bool IsVolumeAvailable(const char8_t* pPath, int timeoutMS = 2000);
    }

    {
        // size_t GetDriveInfo(DriveInfo16* pDriveInfoArray = NULL, size_t nDriveInfoArrayLength = 0);
        // size_t GetDriveInfo(DriveInfo8* pDriveInfoArray = NULL, size_t nDriveInfoArrayLength = 0);

        DriveInfoW driveInfoArray[12];
        n = EA::IO::GetDriveInfo(driveInfoArray, 12);
        
        #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP) // We don't currently require that other platforms enumerate drives
            EATEST_VERIFY(n > 0);
        #endif

        for(size_t j = 0; j < n; j++)
        {
            EA::IO::DriveInfoW::DriveInfoName driveInfoNameTemp(driveInfoArray[j].mName);

            // EATEST_VERIFY(driveInfoNameTemp.length() > 0);  // This isn't necessarily so. 
            // const wchar_t c = driveInfoNameTemp.back();
            // EATEST_VERIFY((c == ':') || !IsFilePathSeparator(c)); // This isn't necessarily so. 

            // We'll use mName with a '/' appended from here on.
            driveInfoNameTemp += EA_FILE_PATH_SEPARATOR_8;
            sTempW = driveInfoNameTemp.c_str();
            sTempW += EA_WCHAR("abc");
            sTempW += EA_FILE_PATH_SEPARATOR_W;
            sTempW += EA_WCHAR("def");

            #if !defined(EA_PLATFORM_PS3) // Cell SDK doesn't expose GetDriveTypeValue functionality.
                // DriveType GetDriveTypeValue(const wchar_t* pPath);
                DriveType driveType = GetDriveTypeValue(driveInfoNameTemp.c_str());
                EATEST_VERIFY(driveType != kDriveTypeUnknown);
            #endif
        
            if(driveInfoArray[j].mType == kDriveTypeFixed) // Any other drive will likely be absent and give error values.
            {
                // int EA::IO::GetDriveName(const wchar_t* pPath, wchar_t* pName);
                wchar_t pName[kMaxVolumeNameLength];
                i = GetDriveName(sTempW.c_str(), pName);

                #if defined(EA_PLATFORM_MICROSOFT)
                    EATEST_VERIFY_F(i >= 0, "GetDriveName failed. Return value: %d, Type: kDriveTypeFixed, Name: %ls, GetLastError: 0x%08x (%u)", i, sTempW.c_str(), GetLastError(), GetLastError());
                #else
                    EATEST_VERIFY_F(i >= 0, "GetDriveName failed. Return value: %d, Type: kDriveTypeFixed, Name: %ls", i, sTempW.c_str());
                #endif

                #if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XENON) || defined(EA_PLATFORM_PS3) // So far only implemented on these platforms.
                    // uint64_t EA::IO::GetDriveFreeSpace(const wchar_t* pPath);
                    g = GetDriveFreeSpace(driveInfoNameTemp.c_str());
                    #if defined(EA_PLATFORM_PS3)
                        EATEST_VERIFY(g != UINT64_C(0xffffffffffffffff) || (j != 0)); // The PS3 drive(s) beyond the first one include network drives which we can't get the drive space for.
                    #else
                        EATEST_VERIFY(g != UINT64_C(0xffffffffffffffff));
                    #endif

                    g = GetDriveFreeSpace(sTempW.c_str());
                    #if defined(EA_PLATFORM_PS3)
                        EATEST_VERIFY(g != UINT64_C(0xffffffffffffffff) || (j != 0)); // The PS3 drive(s) beyond the first one include network drives which we can't get the drive space for.
                    #else
                        EATEST_VERIFY(g != UINT64_C(0xffffffffffffffff));
                    #endif

                    #if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XENON)
                        // int EA::IO::GetDriveSerialNumber(const wchar_t* pPath, wchar_t* pSerialNumber);
                        wchar_t pSerialNumber[kMaxVolumeSerialNumberLength];
                        i = GetDriveSerialNumber(driveInfoNameTemp.c_str(), pSerialNumber);
                        EATEST_VERIFY((i > 0) && (*pSerialNumber != 0));
                    #endif
                #else
                    EATEST_VERIFY(g == 0);
                #endif
            }
        }
    }


    {
        // int GetSpecialDirectory(SpecialDirectory specialDirectory, wchar_t* pDirectoryW, bool bEnsureDirectoryExistence = false, uint32_t nPathLength = kMaxDirectoryLength);
        // int GetSpecialDirectory(SpecialDirectory specialDirectory, char8_t* pDirectoryW, bool bEnsureDirectoryExistence = false, uint32_t nPathLength = kMaxDirectoryLength);

        using namespace EA::IO::Path;

        i = GetSpecialDirectory(kSpecialDirectoryTemp, pDirectoryW, false);
        EATEST_VERIFY(i > 0);
        EATEST_VERIFY(GetHasTrailingSeparator(pDirectoryW)); 

        i = GetSpecialDirectory(kSpecialDirectoryOperatingSystem, pDirectoryW, false);
        EATEST_VERIFY(i > 0);
        EATEST_VERIFY(GetHasTrailingSeparator(pDirectoryW)); 

        // Microsoft appears to have deprecated this functionality on their systems.
        //i = GetSpecialDirectory(kSpecialDirectoryOperatingSystemTrash, pDirectoryW, false);
        //EATEST_VERIFY(i > 0);
        //EATEST_VERIFY(GetHasTrailingSeparator(pDirectoryW)); 

        i = GetSpecialDirectory(kSpecialDirectoryOperatingSystemFonts, pDirectoryW, false);
        EATEST_VERIFY(i > 0);
        EATEST_VERIFY(GetHasTrailingSeparator(pDirectoryW)); 

        #if !defined(EAIO_USE_UNIX_IO)
            i = GetSpecialDirectory(kSpecialDirectoryCurrentApplication, pDirectoryW, false);
            EATEST_VERIFY(i > 0);
            EATEST_VERIFY(GetHasTrailingSeparator(pDirectoryW)); 
        #endif

        i = GetSpecialDirectory(kSpecialDirectoryUserDesktop, pDirectoryW, false);
        EATEST_VERIFY(i > 0);
        EATEST_VERIFY(GetHasTrailingSeparator(pDirectoryW)); 

        i = GetSpecialDirectory(kSpecialDirectoryCommonDesktop, pDirectoryW, false);
        EATEST_VERIFY(i > 0);
        EATEST_VERIFY(GetHasTrailingSeparator(pDirectoryW)); 

        i = GetSpecialDirectory(kSpecialDirectoryUserApplicationData, pDirectoryW, false);
        EATEST_VERIFY(i > 0);
        EATEST_VERIFY(GetHasTrailingSeparator(pDirectoryW)); 

        i = GetSpecialDirectory(kSpecialDirectoryCommonApplicationData, pDirectoryW, false);
        EATEST_VERIFY(i > 0);
        EATEST_VERIFY(GetHasTrailingSeparator(pDirectoryW)); 

        i = GetSpecialDirectory(kSpecialDirectoryUserDocuments, pDirectoryW, false);
        EATEST_VERIFY(i > 0);
        EATEST_VERIFY(GetHasTrailingSeparator(pDirectoryW)); 

        i = GetSpecialDirectory(kSpecialDirectoryCommonDocuments, pDirectoryW, false);
        EATEST_VERIFY(i > 0);
        EATEST_VERIFY(GetHasTrailingSeparator(pDirectoryW)); 

        i = GetSpecialDirectory(kSpecialDirectoryUserMusic, pDirectoryW, false);
        EATEST_VERIFY(i > 0);
        EATEST_VERIFY(GetHasTrailingSeparator(pDirectoryW)); 

        i = GetSpecialDirectory(kSpecialDirectoryCommonMusic, pDirectoryW, false);
        EATEST_VERIFY(i > 0);
        EATEST_VERIFY(GetHasTrailingSeparator(pDirectoryW)); 

        i = GetSpecialDirectory(kSpecialDirectoryUserProgramsShortcuts, pDirectoryW, false);
        EATEST_VERIFY(i > 0);
        EATEST_VERIFY(GetHasTrailingSeparator(pDirectoryW)); 

        i = GetSpecialDirectory(kSpecialDirectoryCommonProgramsShortcuts, pDirectoryW, false);
        EATEST_VERIFY(i > 0);
        EATEST_VERIFY(GetHasTrailingSeparator(pDirectoryW)); 
    }

    return nErrorCount;
}


///////////////////////////////////////////////////////////////////////////////
// TestFileDirectory
//
static int TestFileDirectory()
{
    int nErrorCount(0);

    EA::UnitTest::Report("    TestFileDirectory\n");

    using namespace EA::IO;

    #if EAIOTEST_APPLICATION_BUNDLES_SUPPORTED
        if(gbVerboseOutput)
            EA::UnitTest::Report("\nAppBundle EntryFindFirst / EntryFindNext test\n");

    {
        EntryFindData* p = EntryFindFirst(EA_WCHAR("appbundle:/"), NULL);
        if(p)
        {
            do{
                if(gbVerboseOutput)
                    EA::UnitTest::Report("%ls\n", p->mName);

                    if(EA::StdC::Strcmp(p->mName, EA_WCHAR("subfolder/")) == 0 || 
                       EA::StdC::Strcmp(p->mName, EA_WCHAR("webkit/"))    == 0 || 
                       EA::StdC::Strcmp(p->mName, EA_WCHAR("images/"))    == 0 || 
                       EA::StdC::Strcmp(p->mName, EA_WCHAR("sounds/"))    == 0)
                {
                    EATEST_VERIFY(p->mbIsDirectory);
                }
                else
                    EATEST_VERIFY(!p->mbIsDirectory);

            } while(EntryFindNext(p));

            EntryFindFinish(p);
        }
    }

    {
        int numMatches = 0;
        EntryFindData* p = EntryFindFirst(EA_WCHAR("appbundle:/"), EA_WCHAR("*.txt"));
        if(p)
        {
            do{
                if(gbVerboseOutput)
                    EA::UnitTest::Report("text file:, %ls\n", p->mName);
                numMatches++;
            } while(EntryFindNext(p));

            EntryFindFinish(p);

        }
        // We expect 2 text files in the application bundle
        EATEST_VERIFY(numMatches == 2);
    }

    {
        int numMatches = 0;
        EntryFindData* p = EntryFindFirst(EA_WCHAR("appbundle:/subfolder/"), EA_WCHAR("*.txt"));
        if(p)
        {
            do{
                if(gbVerboseOutput)
                    EA::UnitTest::Report("text file:, %ls\n", p->mName);
                numMatches++;
            } while(EntryFindNext(p));
            EntryFindFinish(p);
        }
        // We expect 1 text file in the application bundle subfolder named 'subfolder'
        EATEST_VERIFY(numMatches == 1);
    }
    #endif // #if EAIOTEST_APPLICATION_BUNDLES_SUPPORTED

    {
        if(gbVerboseOutput)
            EA::UnitTest::Report("\nEntryFindFirst / EntryFindNext test #1\n");

        bool result = Directory::Exists(gRootDirectoryW);
        EATEST_VERIFY(result);

        EntryFindData* p = EntryFindFirst(gRootDirectoryW, NULL);

        if(p)
        {
            do{
                if(gbVerboseOutput)
                    EA::UnitTest::Report("%ls\n", p->mName);
            } while(EntryFindNext(p));

            EntryFindFinish(p);
        }
    }


    {
        if(gbVerboseOutput)
            EA::UnitTest::Report("\nEntryFindFirst / EntryFindNext test #2\n");

        bool result = Directory::Exists(gRootDirectoryW);
        EATEST_VERIFY(result);

        {
        timeval tv;
        EA::StdC::GetTimeOfDay(&tv, NULL, true);    // We use EAStdC time functionality instead of built-in standard C time because the time function is broken on some platforms (e.g. PS3, Wii).
        if(gbVerboseOutput)
            EA::UnitTest::Report("date now: %s\n", TestLocal::FormatTimeAsString(tv.tv_sec));

        EntryFindData efd;

        if(EntryFindFirst(gRootDirectoryW, EA_WCHAR("*"), &efd))
        {
            do{
                // wchar_t efd.mName[kMaxPathLength];
                if(gbVerboseOutput)
                    EA::UnitTest::Report("%ls size: %I64u date modified: %s\n", efd.mName, efd.mSize, TestLocal::FormatTimeAsString(efd.mModificationTime));

                // bool efd.mbIsDirectory;
                EA::IO::Path::PathStringW sPath(efd.mDirectoryPath);
                sPath += efd.mName;

                if(efd.mbIsDirectory)
                {
                    // For some reason 'caferun' is special and can't be opened as a directory
                    if(EA::StdC::Strcmp(sPath.c_str(), EA_WCHAR("/vol/save/caferun")))
                        EATEST_VERIFY(EA::IO::Directory::Exists(sPath.c_str()));
                }
                else
                    EATEST_VERIFY(EA::IO::File::Exists(sPath.c_str()));

                // The following time tests can fail if the file system is on another computer
                // and that computer has a different date setting than the currently running 
                // computer. An example of this is debug testing on a console platform where
                // the time() function returns the currently executing console's time, but the 
                // files themselves reside on a test PC which is set to a different date.
                    #if !defined(EA_PLATFORM_PS3)
                        const time_t expectedTime = (time_t)(tv.tv_sec + 86400);

                // time_t efd.mCreationTime;                
                        EATEST_VERIFY(efd.mCreationTime < expectedTime);              // This can fail if the file system was messed with. We use 86400 becuase that's the number of seconds in a day, and some file systems (e.g. PS3) write file times in local time instead of UTC.

                // time_t efd.mModificationTime;
                        EATEST_VERIFY(efd.mModificationTime < expectedTime);          // This can fail if the file system was messed with.
              //EATEST_VERIFY(efd.mCreationTime < entry.mModificationTime);   // Disabled because this simply often gets messed up on file systems.
                    #endif

                // uint64_t efd.mSize;
                EATEST_VERIFY(efd.mSize < UINT64_C(10000000000));   // Sanity check

                // wchar_t efd.mDirectoryPath[kMaxPathLength];
                EATEST_VERIFY(EA::StdC::Strcmp(efd.mDirectoryPath, gRootDirectoryW) == 0);

                // wchar_t efd.mEntryFilterPattern[kMaxPathLength];
                EATEST_VERIFY(EA::StdC::Strcmp(efd.mEntryFilterPattern, EA_WCHAR("*")) == 0);

            } while(EntryFindNext(&efd));

            EntryFindFinish(&efd);
        }
    }
    }


    // the Airplay runtime libraries incorrectly assert on some of these tests.
    #if !defined(EA_PLATFORM_AIRPLAY)
    {
        wchar_t kDirSeparator[2] = { EA_FILE_PATH_SEPARATOR_8, 0 };

        if(gbVerboseOutput)
            EA::UnitTest::Report("\nDirectoryIterator tests\n");

        // Make some files and directories if they don't already exist.
        wchar_t pTempDir[kMaxPathLength];
        EA::StdC::Strcpy(pTempDir, gRootDirectoryW);
        EA::StdC::Strcat(pTempDir, EA_WCHAR("temp"));
        EA::StdC::Strcat(pTempDir, kDirSeparator);

        Directory::Remove(pTempDir);
        if(!Directory::Exists(pTempDir))
            Directory::Create(pTempDir);

        if(Directory::Exists(pTempDir))
        {
            wchar_t pTempDirA[kMaxPathLength];
            EA::StdC::Strcpy(pTempDirA, pTempDir);
            EA::StdC::Strcat(pTempDirA, EA_WCHAR("A"));
            EA::StdC::Strcat(pTempDirA, kDirSeparator);

            Directory::Remove(pTempDirA);
            if(!Directory::Exists(pTempDirA))
                Directory::Create(pTempDirA);

            wchar_t pTempFile1[kMaxPathLength];
            EA::StdC::Strcpy(pTempFile1, pTempDir);
            EA::StdC::Strcat(pTempFile1, EA_WCHAR("1.txt"));

            wchar_t pTempFile2[kMaxPathLength];
            EA::StdC::Strcpy(pTempFile2, pTempDir);
            EA::StdC::Strcat(pTempFile2, EA_WCHAR("2.doc"));

            wchar_t pTempFileA1[kMaxPathLength];
            EA::StdC::Strcpy(pTempFileA1, pTempDirA);
            EA::StdC::Strcat(pTempFileA1, EA_WCHAR("A1.txt"));

            wchar_t pTempFileA2[kMaxPathLength];
            EA::StdC::Strcpy(pTempFileA2, pTempDirA);
            EA::StdC::Strcat(pTempFileA2, EA_WCHAR("A2.doc"));

            if(!File::Exists(pTempFile1))
                if(!File::Create(pTempFile1))
                    EATEST_VERIFY(false);

            if(!File::Exists(pTempFile2))
                if(!File::Create(pTempFile2))
                    EATEST_VERIFY(false);

            if(!File::Exists(pTempFileA1))
                if(!File::Create(pTempFileA1))
                    EATEST_VERIFY(false);

            if(!File::Exists(pTempFileA2))
                if(!File::Create(pTempFileA2))
                    EATEST_VERIFY(false);


            // Try iterating the above
            DirectoryIterator dirIterator;
            DirectoryIterator::EntryList entryList;
            size_t result;

            // Test Read of a directory.
            entryList.clear();
            result = dirIterator.Read(pTempDirA, entryList, NULL, kDirectoryEntryFile, DirectoryIterator::kMaxEntryCountDefault, false);
            EATEST_VERIFY(result == 2);
            EATEST_VERIFY(entryList.size() == 2);
            for(DirectoryIterator::EntryList::iterator it = entryList.begin(); it != entryList.end(); ++it)
            {
                const DirectoryIterator::Entry& entry = *it;
                if(gbVerboseOutput)
                    EA::UnitTest::Report("%ls%ls\n", pTempDirA, entry.msName.c_str());
            }
            if(gbVerboseOutput)
                EA::UnitTest::Report("\n");


            // Test Read of a directory with count limit.
            entryList.clear();
            result = dirIterator.Read(pTempDirA, entryList, NULL, kDirectoryEntryFile, 1, false);
            EATEST_VERIFY(result == 1);
            EATEST_VERIFY(entryList.size() == 1);
            for(DirectoryIterator::EntryList::iterator it = entryList.begin(); it != entryList.end(); ++it)
            {
                const DirectoryIterator::Entry& entry = *it;

                // wchar_t entry.mName[kMaxPathLength];
                if(gbVerboseOutput)
                    EA::UnitTest::Report("%ls%ls\n", pTempDirA, entry.msName.c_str());
            }
            if(gbVerboseOutput)
                EA::UnitTest::Report("\n");

            {
            // Test ReadRecursive of a directory.
            timeval tv;
            EA::StdC::GetTimeOfDay(&tv, NULL, true);    // We use EAStdC time functionality instead of built-in standard C time because the time function is broken on some platforms (e.g. PS3, Wii).
            if(gbVerboseOutput)
                EA::UnitTest::Report("date now: %s\n", TestLocal::FormatTimeAsString(tv.tv_sec));

            entryList.clear();
            result = dirIterator.ReadRecursive(pTempDir, entryList, EA_WCHAR("*"), kDirectoryEntryFile | kDirectoryEntryDirectory, true, true, DirectoryIterator::kMaxEntryCountDefault, true);
            EATEST_VERIFY(result == 5);
            EATEST_VERIFY(entryList.size() == 5);
            for(DirectoryIterator::EntryList::iterator it = entryList.begin(); it != entryList.end(); ++it)
            {
                const DirectoryIterator::Entry& entry = *it;

                // wchar_t entry.mName[kMaxPathLength];
                if(gbVerboseOutput)
                    EA::UnitTest::Report("%ls size: %I64u\n date modified: %s", entry.msName.c_str(), entry.mSize, TestLocal::FormatTimeAsString(entry.mModificationTime));

                // The following time tests can fail if the file system is on another computer
                // and that computer has a different date setting than the currently running 
                // computer. An example of this is debug testing on a console platform where
                // the time() function returns the currently executing console's time, but the 
                // files themselves reside on a test PC which is set to a different date.
                    #if !defined(EA_PLATFORM_PS3)
                        const time_t expectedTime = (time_t)(tv.tv_sec + 86400);

                // time_t entry.mCreationTime;
                        EATEST_VERIFY(entry.mCreationTime < expectedTime);              // This can fail if the file system was messed with. We use 86400 becuase that's the number of seconds in a day, and some file systems (e.g. PS3) write file times in local time instead of UTC.

                // time_t entry.mModificationTime;
                        EATEST_VERIFY(entry.mModificationTime < expectedTime);          // This can fail if the file system was messed with.
              //EATEST_VERIFY(entry.mCreationTime < entry.mModificationTime);   // Disabled because this simply often gets messed up on file systems.
                    #endif

                // uint64_t entry.mSize;
                EATEST_VERIFY(entry.mSize < UINT64_C(10000000000));   // Sanity check
            }
            if(gbVerboseOutput)
                EA::UnitTest::Report("\n");
            }


            // Test ReadRecursive of a directory with filter.
            entryList.clear();
            result = dirIterator.ReadRecursive(pTempDir, entryList, EA_WCHAR("*.doc"), kDirectoryEntryFile | kDirectoryEntryDirectory, true, true, DirectoryIterator::kMaxEntryCountDefault, false);
            EATEST_VERIFY(result == 2);
            EATEST_VERIFY(entryList.size() == 2);
            for(DirectoryIterator::EntryList::iterator it = entryList.begin(); it != entryList.end(); ++it)
            {
                const DirectoryIterator::Entry& entry = *it;
                if(gbVerboseOutput)
                    EA::UnitTest::Report("%ls\n", entry.msName.c_str());
            }
            if(gbVerboseOutput)
                EA::UnitTest::Report("\n");


            // Test ReadRecursive of a directory with count limit.
            entryList.clear();
            result = dirIterator.ReadRecursive(pTempDir, entryList, EA_WCHAR("*"), kDirectoryEntryFile | kDirectoryEntryDirectory, true, true, 3, false);
            EATEST_VERIFY(result == 3);
            EATEST_VERIFY(entryList.size() == 3);
            for(DirectoryIterator::EntryList::iterator it = entryList.begin(); it != entryList.end(); ++it)
            {
                const DirectoryIterator::Entry& entry = *it;
                if(gbVerboseOutput)
                    EA::UnitTest::Report("%ls\n", entry.msName.c_str());
            }
            if(gbVerboseOutput)
                EA::UnitTest::Report("\n");


            // Test ReadRecursive of a directory with partial paths.
            entryList.clear();
            result = dirIterator.ReadRecursive(pTempDir, entryList, EA_WCHAR("*"), kDirectoryEntryFile | kDirectoryEntryDirectory, true, false, DirectoryIterator::kMaxEntryCountDefault, false);
            EATEST_VERIFY(result == 5);
            EATEST_VERIFY(entryList.size() == 5);
            for(DirectoryIterator::EntryList::iterator it = entryList.begin(); it != entryList.end(); ++it)
            {
                const DirectoryIterator::Entry& entry = *it;
                if(gbVerboseOutput)
                    EA::UnitTest::Report("%ls\n", entry.msName.c_str());
            }
            if(gbVerboseOutput)
                EA::UnitTest::Report("\n");
        }
    }
    #endif

    return nErrorCount;
}


///////////////////////////////////////////////////////////////////////////////
// TestFileStream0
//
static int TestFileStream0()
{
    int nErrorCount(0);

    EA::UnitTest::Report("    TestFileStream0\n");

    using namespace EA::IO;

    {   // FileStream path tests
        eastl::string8 path8(gRootDirectory8);
        eastl::wstring pathW(gRootDirectoryW);
        wchar_t kDirSeparator[2] = { EA_FILE_PATH_SEPARATOR_8, 0 };

        path8  +=  "temp";
        path8  +=  EA_FILE_PATH_SEPARATOR_STRING_8;
        path8  +=  "hello world.txt";

        pathW += EA_WCHAR("temp");
        pathW += kDirSeparator;
        pathW += EA_WCHAR("hello world.txt");

        EA::IO::FileStream fileStream8(path8.c_str());
        EA::IO::FileStream fileStream16(pathW.c_str());

        char8_t pPath8[EA::IO::kMaxPathLength];
        wchar_t pPathW[EA::IO::kMaxPathLength];

        fileStream8.GetPath(pPath8, EA::IO::kMaxPathLength);
        EATEST_VERIFY(path8 == pPath8);

        fileStream8.GetPath(pPathW, EA::IO::kMaxPathLength);
        EATEST_VERIFY(pathW == pPathW);

        fileStream16.GetPath(pPath8, EA::IO::kMaxPathLength);
        EATEST_VERIFY(path8 == pPath8);

        fileStream16.GetPath(pPathW, EA::IO::kMaxPathLength);
        EATEST_VERIFY(pathW == pPathW);
    }


    {   // FileStream read/write, CreationDisposition, sharing
        // 0
        // kCDOpenExisting
        // kCDCreateNew
        // kCDCreateAlways
        // kCDOpenAlways
        // kCDTruncateExisting
    }


    #if defined(EA_PLATFORM_REVOLUTION) // The following is Wii-specific code.
        {
            EA::IO::off_type  nPosition;
            EA::IO::size_type nSize;
            bool              bResult;
          //int               nResult;
            const char* const pPath = "test.txt";
            FileStreamWiiNand fileStreamNand(pPath);

            if(fileStreamNand.Open(kAccessFlagReadWrite, kCDCreateAlways, 0, 0))
            {
                EA_ALIGNED(char, c[32], 32) = { 'a' };

                bResult = fileStreamNand.Write(c, 1);
                EATEST_VERIFY(bResult);

                nPosition = fileStreamNand.GetPosition();
                EATEST_VERIFY(nPosition == 1);

                bResult = fileStreamNand.SetPosition(0);
                EATEST_VERIFY(bResult);

                nSize = fileStreamNand.Read(c, 32); // Nand file system requires reads to be in increments of 32 bytes.
                EATEST_VERIFY((nSize == 1) && (c[0] == 'a'));

                fileStreamNand.Close();

                // We don't currently have a wrapper for Nand file system manipulation.
                s32 result = NANDDelete(pPath);
                EATEST_VERIFY(result == 0);
            }
        }

        {
            EA::IO::off_type  nPosition;
            EA::IO::size_type nSize;
            bool              bResult;
            FileStreamWiiHio2 fileStreamHio2A("TestA.txt");
            FileStreamWiiHio2 fileStreamHio2B("TestB.txt");

            // The user needs to run the NintendoWare MCS server applet for this to work.
            // We test access of two files simultaneously.
            if(fileStreamHio2A.Open(kAccessFlagReadWrite, kCDCreateAlways, 0, 0))
            {
                if(fileStreamHio2B.Open(kAccessFlagReadWrite, kCDCreateAlways, 0, 0))
                {
                    char c[32];
    
                    c[0] = 'A';
                    bResult = fileStreamHio2A.Write(c, 1);
                    EATEST_VERIFY(bResult);

                    c[0] = 'B';
                    bResult = fileStreamHio2B.Write(c, 1);
                    EATEST_VERIFY(bResult);

                    nPosition = fileStreamHio2A.GetPosition();
                    EATEST_VERIFY(nPosition == 1);

                    nPosition = fileStreamHio2B.GetPosition();
                    EATEST_VERIFY(nPosition == 1);

                    bResult = fileStreamHio2A.SetPosition(0);
                    EATEST_VERIFY(bResult);

                    bResult = fileStreamHio2B.SetPosition(0);
                    EATEST_VERIFY(bResult);

                    c[0] = 'x';
                    nSize = fileStreamHio2A.Read(c, 1);
                    EATEST_VERIFY((nSize == 1) && (c[0] == 'A'));

                    c[0] = 'x';
                    nSize = fileStreamHio2B.Read(c, 1);
                    EATEST_VERIFY((nSize == 1) && (c[0] == 'B'));

                    fileStreamHio2B.Close();
                }

                fileStreamHio2A.Close();
            }

            // We don't currently have a means to delete files via Hio2.
        }

        {
            /*
            extern HFILE* hstdin;
            extern HFILE* hstdout;
            extern HFILE* hstderr;

            bool    hfinit();
            HFILE*  hfalloc();
            void    hffree(HFILE*);
            void    hclearerr(HFILE*);
            int     hfclose(HFILE*);
            int     hfeof(HFILE*);
            int     hferror(HFILE*);
            int     hfflush(HFILE*);
            int     hfgetc(HFILE*);
            int     hfgetpos(HFILE*, fpos_t* position);
            char*   hfgets(char* pString, int, HFILE*);
            HFILE*  hffopen(HFILE*, const char* pFilePath, const char* mode);
            HFILE*  hfopen(const char* pFilePath, const char* mode);
            int     hfileno(HFILE*);
            int     hfprintf(HFILE*, const char* pFormat, ...);
            int     hfputc(int, HFILE*);
            int     hfputs(const char* pString, HFILE*);
            size_t  hfread(void* data, size_t size, size_t count, HFILE*);
          //HFILE*  hfreopen(const char* pFilePath, const char* mode, HFILE*);
          //int     hfscanf(HFILE*, const char* pFormat, ...);
            int     hfseek(HFILE*, long offset, int origin);
            int     hfsetpos(HFILE*, const fpos_t* position);
            long    hftell(HFILE*);
            size_t  hfwrite(const void* data, size_t size, size_t count, HFILE*);
            char*   hgets(char* pString);
          //void    hperror(const char* pString);
            int     hprintf(const char* pFormat, ...);
            int     hputs(const char* pString);
          //int     hremove(const char* pFilePath);
          //int     hrename(const char* pFileNameOld, const char* pFileNameNew);
            void    hrewind(HFILE*);
          //int     hscanf(const char* pFormat, ...);
          //void    hsetbuf(HFILE*, char* buffer);
          //int     hsetvbuf(HFILE*, char* buffer, int mode, size_t size);
          //HFILE*  htmpfile();
          //char*   htmpnam(char* pFilePath);
          //int     hungetc(int character, HFILE*);
            int     hvfprintf(HFILE*, const char* pFormat, va_list);
            int     hvprintf(const char* pFormat, va_list);
            */

            const char* const pPath = "test.txt";

            // The user needs to run the NintendoWare MCS server applet for this to work.
            HFILE* pFile = hfopen(pPath, "a+");

            if(pFile)
            {
                char c[32] = { 'a' };

                size_t nCount = hfwrite(c, 1, 1, pFile);
                EATEST_VERIFY(nCount == 1);

                long nTellPosition = hftell(pFile);
                EATEST_VERIFY(nTellPosition == 1);

                int nSeekResult = hfseek(pFile, 0, SEEK_SET);
                EATEST_VERIFY(nSeekResult == 0);

                c[0] = 'b';
                size_t nReadCount = hfread(c, 1, 1, pFile);
                EATEST_VERIFY((nReadCount == 1) && (c[0] == 'a'));

                hfclose(pFile);

                // We don't currently have a means to delete files via Hio2.
            }
        }
    #endif

    #if !defined(EA_PLATFORM_REVOLUTION)   // Question: Why is this disabled for Wii?
        {   // FileStream read/write tests.
            EA::IO::off_type  nPosition;
            EA::IO::size_type nSize;
            bool              bResult;
            int               nResult;
            eastl::string8    path8(gRootDirectory8); path8 += "hello world.txt";

            // Remove this test file if it already exists.
            if(File::Exists(path8.c_str()))
                File::Remove(path8.c_str());

            bResult = File::Exists(path8.c_str());
            EATEST_VERIFY_MSG(!bResult, "Unable to delete test file.");

            EA::IO::FileStream fileStream(path8.c_str());

            nResult = fileStream.GetAccessFlags();
            EATEST_VERIFY(nResult == 0);

            nResult = fileStream.GetState();
            EATEST_VERIFY(nResult == kStateNotOpen);

            nSize = fileStream.GetSize();
            EATEST_VERIFY(nSize == EA::IO::kSizeTypeError);

            bResult = fileStream.Open(kAccessFlagReadWrite, kCDCreateAlways, 0, 0);
            EATEST_VERIFY_MSG(bResult, "Unable to open test file.");

            if(bResult)
            {
                nResult = fileStream.GetAccessFlags();
                EATEST_VERIFY(nResult == kAccessFlagReadWrite);

                nResult = fileStream.GetState();
                EATEST_VERIFY(nResult == 0);

                nSize = fileStream.GetSize();
                EATEST_VERIFY(nSize == 0);

                nPosition = fileStream.GetPosition();
                EATEST_VERIFY(nPosition == 0);

                bResult = fileStream.SetSize(5555);
                EATEST_VERIFY(bResult);

                nSize = fileStream.GetSize();
                EATEST_VERIFY(nSize == 5555);

                nSize = fileStream.GetAvailable();
                EATEST_VERIFY(nSize == 5555);

                nPosition = fileStream.GetPosition();
                EATEST_VERIFY(nPosition == 0);

                nPosition = fileStream.GetPosition(EA::IO::kPositionTypeEnd);
                EATEST_VERIFY(nPosition == (EA::IO::off_type)-5555);

                bResult = fileStream.SetPosition(200);
                EATEST_VERIFY(bResult);

                nPosition = fileStream.GetPosition(EA::IO::kPositionTypeBegin);
                EATEST_VERIFY(nPosition == 200);

                nPosition = fileStream.GetPosition(EA::IO::kPositionTypeCurrent);
                EATEST_VERIFY(nPosition == 0);

                nPosition = fileStream.GetPosition(EA::IO::kPositionTypeEnd);
                EATEST_VERIFY(nPosition == (EA::IO::off_type)(-5555 + 200));

                nSize = fileStream.GetAvailable();
                EATEST_VERIFY(nSize == (5555 - 200));

                bResult = fileStream.SetPosition(100, EA::IO::kPositionTypeCurrent);
                EATEST_VERIFY(bResult);

                nPosition = fileStream.GetPosition(EA::IO::kPositionTypeBegin);
                EATEST_VERIFY(nPosition == 300);

                bResult = fileStream.Write(path8.data(), 4);
                EATEST_VERIFY(bResult);

                nPosition = fileStream.GetPosition(EA::IO::kPositionTypeBegin);
                EATEST_VERIFY(nPosition == 304);

                bResult = fileStream.Flush();
                EATEST_VERIFY(bResult);

                #if EAIO_64_BIT_SIZE_ENABLED
                    const EA::IO::size_type kLargeFileSize = UINT64_C(0x100001010); // This is > 4GB

                    bResult = fileStream.SetSize(kLargeFileSize);

                    #if defined(EA_PLATFORM_MICROSOFT) && !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
                        // It turns out that the Xenon emulated D: drive won't let you create large files, even if the space exists. 
                        // So we detect this and skip the test, as there is no way to test this functionality.
                        if(!bResult && (fileStream.GetState() == kFSErrorDiskFull))
                            EA::UnitTest::Report("Cannot create > 4GB file on the kit. Skipping test.\n");
                        else
                            EATEST_VERIFY(bResult);
                    #else
                        EATEST_VERIFY(bResult);
                    #endif

                    if(bResult)
                    {
                        nSize = fileStream.GetSize();
                        //EA::UnitTest::Report("%llu %lld\n", (int64_t)nSize, kLargeFileSize);
                        EATEST_VERIFY(nSize == kLargeFileSize);

                        nPosition = fileStream.GetPosition(EA::IO::kPositionTypeBegin);
                        EATEST_VERIFY(nPosition == 304);

                        bResult = fileStream.SetPosition(0, EA::IO::kPositionTypeEnd);
                        EATEST_VERIFY(bResult);

                        nPosition = fileStream.GetPosition(EA::IO::kPositionTypeBegin);
                        //EA::UnitTest::Report("%lld %llu %lld\n", (int64_t)nPosition, kLargeFileSize, (EA::IO::off_type)kLargeFileSize);
                        EATEST_VERIFY(nPosition == (EA::IO::off_type)kLargeFileSize);

                        bResult = fileStream.SetPosition(-4, EA::IO::kPositionTypeEnd);
                        EATEST_VERIFY(bResult);

                        bResult = fileStream.Write(path8.data(), 4); // This write takes a long time to execute on some platforms, as it is filling in a lot of new file space.
                        EATEST_VERIFY(bResult);

                        bResult = fileStream.SetPosition(-4, EA::IO::kPositionTypeCurrent);
                        EATEST_VERIFY(bResult);

                        char buffer[5] = { 0, 0, 0, 0, 0 };
                        nSize = fileStream.Read(buffer, 4);
                        EATEST_VERIFY(nSize == 4);
                        EATEST_VERIFY(memcmp(buffer, path8.c_str(), 4) == 0);
                    }
                #endif

                fileStream.Close();

                File::Remove(path8.c_str());
            }
        }
    #endif

    return nErrorCount;
}


///////////////////////////////////////////////////////////////////////////////
// TestFileStream1
//
// Tests reading of a Uint8.txt file. This is a file which is simply 256
// bytes long and each byte has the value 0, 1, 2, etc. We do random reads
// from that file into a buffer and verify that read proceeded as expected.
//
static int TestFileStream1(EA::IO::FileStream* pFileStream = NULL)
{
    using namespace EA;
    using namespace EA::IO;

    EA::UnitTest::Report("    TestFileStream1\n");

    int         nErrorCount = 0;
    const float fDiskSpeed  = EA::UnitTest::GetSystemSpeed(EA::UnitTest::kSpeedTypeDisk);

    if(fDiskSpeed > 0.1f)
    {
        bool              bResult = true;
        wchar_t           pFilePath[kMaxPathLength];
        EA::IO::size_type n;
        const uint32_t    kTest1WriteCount = 256;

        bResult = MakeTempPathName(pFilePath, gRootDirectoryW, NULL, EA_WCHAR(".temp"));
        EATEST_VERIFY(bResult);

        if(bResult)
        {
            FileStream file;

            if(!pFileStream)
                pFileStream = &file;

            pFileStream->SetPath(pFilePath);

            if(File::Exists(pFilePath))
                File::Remove(pFilePath);

            // Write the test file.
            bResult = pFileStream->Open(kAccessFlagWrite, kCDOpenAlways);
            EATEST_VERIFY(bResult);

            if(bResult)
            {
                for(uint32_t i = 0; i < kTest1WriteCount; i++)
                {
                    uint8_t i8 = (uint8_t)i;
                    bResult = pFileStream->Write(&i8, sizeof(i8));
                    EATEST_VERIFY(bResult);
                }

                pFileStream->Close();
            }

            if(bResult)
            {
                // Read the test file back.
                if(pFileStream->Open(kAccessFlagRead))
                {
                    const uint8_t  kUnusedValue = 0x01;
                    UnitTest::Rand rng(UnitTest::GetRandSeed());
                    uint32_t       j;

                    uint8_t* const pBuffer = new uint8_t[kTest1WriteCount * 2];

                    // Set the first kTest1WriteCount bytes to the same value we wrote into the file.
                    for(j = 0; j < kTest1WriteCount; j++)
                        pBuffer[j] = (uint8_t)j;

                    // Set the next kTest1WriteCount bytes to some other value.
                    for(; j < kTest1WriteCount * 2; j++)
                        pBuffer[j] = kUnusedValue;

                    #if defined(EA_PLATFORM_DESKTOP)
                        const int kTest1LoopCount = 1000;
                    #else
                        const int kTest1LoopCount = 200;
                    #endif

                    for(int i = 0; i < kTest1LoopCount; i++)
                    {
                        const uint32_t nReadCount    = rng.RandLimit(35);
                        const uint32_t nFilePosition = rng.RandLimit(kTest1WriteCount - nReadCount);

                        bResult = pFileStream->SetPosition((off_type)(int32_t)nFilePosition);
                        EATEST_VERIFY(bResult);

                        n = pFileStream->Read(pBuffer + nFilePosition, nReadCount);
                        EATEST_VERIFY(n == (size_type)nReadCount);

                        n = pFileStream->Read(pBuffer + nFilePosition + nReadCount, nReadCount);
                        EATEST_VERIFY(n != kSizeTypeError); // This (intentionally) might go past the end of the file.

                        for(j = 0; j < kTest1WriteCount; j++)
                            EATEST_VERIFY(pBuffer[j] == (uint8_t)j);

                        for(; j < kTest1WriteCount * 2; j++)
                            EATEST_VERIFY(pBuffer[j] == kUnusedValue);
                    }

                    pFileStream->Close();

                    delete[] pBuffer;
                }
            }

            if(File::Exists(pFilePath))
                File::Remove(pFilePath);
        }
    }

    return nErrorCount;
}


///////////////////////////////////////////////////////////////////////////////
// TestFileStream2
//
// Tests reading of a uint16_t.txt file. This is a file which is simply 65536
// bytes long and each byte has the value 0, 1, 2, etc. We do random reads
// from that file into a buffer and verify that read proceeded as expected.
//
static int TestFileStream2(EA::IO::FileStream* pFileStream = NULL)
{
    using namespace EA;
    using namespace EA::IO;

    EA::UnitTest::Report("    TestFileStream2\n");

    int         nErrorCount = 0;
    const float fDiskSpeed  = EA::UnitTest::GetSystemSpeed(EA::UnitTest::kSpeedTypeDisk);

    if(fDiskSpeed > 0.1f)
    {
        bool              bResult = true;
        wchar_t           pFilePath[kMaxPathLength];
        EA::IO::size_type n;
        const uint32_t    kTest2WriteCount = 2048;

        bResult = MakeTempPathName(pFilePath, gRootDirectoryW, NULL, EA_WCHAR(".temp"));
        EATEST_VERIFY(bResult);

        if(bResult)
        {
            FileStream file;

            if(!pFileStream)
                pFileStream = &file;

            pFileStream->SetPath(pFilePath);

            if(File::Exists(pFilePath))
                File::Remove(pFilePath);

            // Write the test file.
            bResult = pFileStream->Open(kAccessFlagWrite, kCDOpenAlways);
            EATEST_VERIFY(bResult);

            if(bResult)
            {
                for(uint32_t i = 0; i < kTest2WriteCount; i++)
                {
                    uint16_t i16 = (uint16_t)i;
                    bResult = pFileStream->Write(&i16, sizeof(i16));
                    EATEST_VERIFY(bResult);
                }

                pFileStream->Close();
            }

            if(bResult)
            {
                // Read the test file back.
                if(pFileStream->Open(kAccessFlagRead))
                {
                    const uint16_t kUnusedValue = 0x0001;
                    UnitTest::Rand rng(UnitTest::GetRandSeed());
                    uint32_t       j;

                    uint16_t* const pBuffer = new uint16_t[kTest2WriteCount * 2];

                    // Set the first kTest2WriteCount bytes to the same value we wrote into the file.
                    for(j = 0; j < kTest2WriteCount; j++)
                        pBuffer[j] = (uint16_t)j;

                    // Set the next kTest2WriteCount bytes to some other value.
                    for(; j < kTest2WriteCount * 2; j++)
                        pBuffer[j] = kUnusedValue;

                    #if defined(EA_PLATFORM_DESKTOP)
                        const int kTest2LoopCount = 1000;
                    #else
                        const int kTest2LoopCount = 200;
                    #endif

                    for(int i = 0; i < kTest2LoopCount; i++)
                    {
                        const uint32_t nReadCount    = rng.RandLimit(1500);
                        const uint32_t nFilePosition = rng.RandLimit(kTest2WriteCount - nReadCount);

                        bResult = pFileStream->SetPosition((off_type)(nFilePosition * sizeof(uint16_t)));
                        EATEST_VERIFY(bResult);

                        n = pFileStream->Read(pBuffer + nFilePosition, nReadCount * sizeof(uint16_t));
                        EATEST_VERIFY(n == (size_type)(nReadCount * sizeof(uint16_t)));

                        n = pFileStream->Read(pBuffer + nFilePosition + nReadCount, nReadCount * sizeof(uint16_t));
                        EATEST_VERIFY(n != kSizeTypeError); // This (intentionally) might go past the end of the file.

                        for(j = 0; j < kTest2WriteCount; j++)
                            EATEST_VERIFY(pBuffer[j] == (uint16_t)j);

                        for(; j < kTest2WriteCount * 2; j++)
                            EATEST_VERIFY(pBuffer[j] == kUnusedValue);
                    }

                    pFileStream->Close();

                    delete[] pBuffer;
                }
            }

            if(File::Exists(pFilePath))
                File::Remove(pFilePath);
        }
    }

    return nErrorCount;
}


///////////////////////////////////////////////////////////////////////////////
// TestFileStream3
//
// Tests buffered reading and writing to a file called uint16_tWrite.txt. This is
// the most stringent test of out test suite here. We do random seeks within
// a uint16_t-setup file and randomly read or write chunks of data that should
// match the expected data in the file. After doing this many many time, we
// re-read the entire file and verify that it contains the expected data. Also,
// we manually inspect the file to make sure it looks proper.
// 
static int TestFileStream3(EA::IO::FileStream* pFileStream = NULL)
{
    using namespace EA;
    using namespace EA::IO;

    EA::UnitTest::Report("    TestFileStream3\n");

    int         nErrorCount = 0;
    const float fDiskSpeed  = EA::UnitTest::GetSystemSpeed(EA::UnitTest::kSpeedTypeDisk);

    if(fDiskSpeed > 0.1f)
    {
        bool              bResult = true;
        wchar_t           pFilePath[kMaxPathLength];
        EA::IO::size_type n;

        #define NUM_WRITES_TESTSTREAM3 4096

        bResult = MakeTempPathName(pFilePath, gRootDirectoryW, NULL, EA_WCHAR(".temp"));
        EATEST_VERIFY(bResult);

        if(bResult)
        {
            FileStream file;

            if(!pFileStream)
                pFileStream = &file;

            pFileStream->SetPath(pFilePath);

            if(File::Exists(pFilePath))
                File::Remove(pFilePath);

            bResult = pFileStream->Open(kAccessFlagReadWrite, kCDOpenAlways);
            EATEST_VERIFY(bResult);

            if(bResult)
            {
                const uint16_t kUnusedValue = 0x0001;
                UnitTest::Rand rng(UnitTest::GetRandSeed());
                uint32_t       i;

                uint16_t* const pBuffer = new uint16_t[NUM_WRITES_TESTSTREAM3 * 2];

                // Setup
                for(i = 0; i < NUM_WRITES_TESTSTREAM3; i++)
                    pBuffer[i] = (uint16_t)i;

                for(; i < NUM_WRITES_TESTSTREAM3 * 2; i++)
                    pBuffer[i] = kUnusedValue;


                // Test a simple sequential write.
                for(i = 0; i < NUM_WRITES_TESTSTREAM3; i++)
                {
                    uint16_t i16 = (uint16_t)i;
                    pFileStream->Write(&i16, sizeof(i16));
                }

                pFileStream->SetPosition(0);

                for(i = 0; i < NUM_WRITES_TESTSTREAM3; i++)
                {
                    uint16_t i16;
                    pFileStream->Read(&i16, sizeof(i16));

                    EATEST_VERIFY(i16 == (uint16_t)i);
                }

                for(i = NUM_WRITES_TESTSTREAM3; i < NUM_WRITES_TESTSTREAM3 * 2; i++)
                {
                    EATEST_VERIFY(pBuffer[i] == kUnusedValue);
                }


                // Arbitrary random read/write tests.
                uint32_t iterationCount = (uint32_t)(10000 * EA::UnitTest::GetSystemSpeed(EA::UnitTest::kSpeedTypeDisk));

                for(i = 0; i < iterationCount; i++)
                {
                    uint32_t nFilePosition(rng.RandLimit((NUM_WRITES_TESTSTREAM3 / sizeof(uint16_t)) - 32) * sizeof(uint16_t));
                    uint16_t buffer[48];
                    int      nBlockSize(rng.RandRange(2, 48));

                    bResult = pFileStream->SetPosition((EA::IO::off_type)nFilePosition); // nFilePosition is an even value from 0-131070.
                    EATEST_VERIFY(bResult);

                    if(rng.RandLimit(2) == 0) // RandLimit(2) is the same thing as a RandBool.
                    {
                        // We write values into the file with the same value they were originally, 
                        // so it's easy to tell if things went correctly.
                        for(uint16_t j = 0; j < nBlockSize; j++)
                            buffer[j] = (uint16_t)((nFilePosition / sizeof(uint16_t)) + j);

                        bResult = pFileStream->Write(buffer, nBlockSize * sizeof(uint16_t));
                        EATEST_VERIFY(bResult);
                    }
                    else
                    {
                        // Test reading a segment of the file, verifying that it is what we expect.
                        n = pFileStream->Read(buffer, nBlockSize * sizeof(uint16_t));
                        EATEST_VERIFY(n == (nBlockSize * sizeof(uint16_t)));

                        for(uint16_t x = 0; x < nBlockSize; x++)
                            EATEST_VERIFY(buffer[x] == (uint16_t)((nFilePosition / sizeof(uint16_t)) + x));
                    }
                }

                // Do final pass at verifying the file is as we expect.
                bResult = pFileStream->SetPosition(0);
                EATEST_VERIFY(bResult);

                for(uint32_t y = 0; y < NUM_WRITES_TESTSTREAM3; y++)
                {
                    uint16_t y16;
                    pFileStream->Read(&y16, sizeof(y16));

                    EATEST_VERIFY(y16 == (uint16_t)y);
                }

                pFileStream->Close();

                delete[] pBuffer;
            }
        }
    }

    return nErrorCount;
}


///////////////////////////////////////////////////////////////////////////////
// TestFile
//
int TestFile()
{
    EA::UnitTest::Report("TestFile\n");

    int nErrorCount(0);

    nErrorCount += TestFileUtil();  
    nErrorCount += TestFileDirectory();

    nErrorCount += TestFileStream0();
    nErrorCount += TestFileStream1();
    nErrorCount += TestFileStream2();
    nErrorCount += TestFileStream3();

    return nErrorCount;
}












#if 0

///////////////////////////////////////////////////////////////////////////////
// File IO speed test from the old GZ framework.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// main
//
// Tests speed of reading a file with cRZFile and C stream IO.
//
struct cIGZFileFStream{
   virtual bool Open(const char* pPath) = 0;
   virtual int  SetVBuf(char* pBuffer, int type, int size) = 0;
   virtual int  Read(char* pBuffer, int count, int size) = 0;
   virtual int  Seek(int amount, int type) = 0;
   virtual void Close() = 0;
};

struct cRZFileFStream : public cIGZFileFStream{
   virtual bool Open(const char* pPath)                    { pFile = ::fopen(pPath, "rb"); return (pFile != NULL); }
   virtual int  SetVBuf(char* pBuffer, int type, int size) { return ::setvbuf(pFile, pBuffer, type, size); }
   virtual int  Read(char* pBuffer, int count, int size)   { return ::fread(pBuffer, count, size, pFile);  }
   virtual int  Seek(int amount, int type)                 { return ::fseek(pFile, amount, type);  }
   virtual void Close()                                    { ::fclose(pFile); pFile = NULL; }
   FILE* pFile;
};

int main(int argc, char* argv[])
{
   cRZCmdLine cmdLine(argc, argv);
   char*      pBuffer = new char[2000000];

   ::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

   if(cmdLine.argc() >= 4){
      int nBlockSize(1);

      while(nBlockSize <= 16384)
      {
         cRZFileFStream fileFStream;

         if(fileFStream.Open(cmdLine[1].c_str())){
            cRZTimer timer1(cRZTimer::nTimerResolutionMicroseconds);

            fileFStream.SetVBuf(NULL, _IONBF, 4098);
            fileFStream.Read(pBuffer, 1, 1);
            timer1.Start();
            for(int i(0); i<16384/nBlockSize; i++){
               Uint32 nCount(1*nBlockSize);
               nCount = fileFStream.Read(pBuffer, 1, nCount);
               //fileFStream.Seek(8, SEEK_CUR);
            }
            timer1.Stop();
            if(gbVerboseOutput)
                EA::UnitTest::Report("time1 (BlockSize = %d): %d.\n", nBlockSize, timer1.GetElapsedTime());

            fileFStream.Close();
         }

         cRZFile file1(cmdLine[2]);
         if(file1.Open()){
            cRZTimer timer1(cRZTimer::nTimerResolutionMicroseconds);

            file1.SetBuffering(4098, 0);
            file1.Read(pBuffer, 1);
            timer1.Start();
            for(int i(0); i<16384/nBlockSize; i++){
               Uint32 nCount(1*nBlockSize);
               file1.ReadWithCount(pBuffer, nCount);
               //file1.Seek(8, cIGZFile::kSeekCurrent);
            }
            timer1.Stop();
            if(gbVerboseOutput)
                EA::UnitTest::Report("time2 (BlockSize = %d): %d.\n",  nBlockSize, timer1.GetElapsedTime());
            file1.Close();
         }

         nBlockSize *= 2;
         EA::UnitTest::Report("\n");
      }
   }
   delete[] pBuffer;

   ::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

   EA::UnitTest::Report("\nDone. Please press the return key.\n");
   getchar();
   return 0;
}
#endif









