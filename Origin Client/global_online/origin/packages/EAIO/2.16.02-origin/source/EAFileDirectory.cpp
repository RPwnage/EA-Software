/////////////////////////////////////////////////////////////////////////////
// EAFileDirectory.cpp
//
// Copyright (c) 2003, Electronic Arts. All Rights Reserved.
// Created by Paul Pedriana, Maxis
//
// Implements file system directory searching and manipulation.
/////////////////////////////////////////////////////////////////////////////


#include <EAIO/internal/Config.h>
#include <EAIO/EAFileDirectory.h>
#include <EAIO/PathString.h>
#include <EAIO/FnEncode.h>
#include <EAIO/FnMatch.h>
#include <EAIO/EAFileUtil.h>
#include <EAStdC/EAString.h>
#include <coreallocator/icoreallocator_interface.h>
#include <EAStdC/EAString.h>
#include EA_ASSERT_HEADER

#if defined(EA_PLATFORM_XENON)
    #pragma warning(push, 1)
    #include <xtl.h>
    #pragma warning(pop)
#elif defined(EA_PLATFORM_MICROSOFT)
    #pragma warning(push, 0)
    #include <Windows.h>
    #pragma warning(pop)
#elif defined(EA_PLATFORM_PS3)
    #include <cell/cell_fs.h>
    #include <sdk_version.h>
#elif EAIO_USE_UNIX_IO
    #ifdef EA_PLATFORM_ANDROID
        #include <jni.h>
    #endif
    #include <sys/stat.h>
    #include <sys/types.h>
    #if EAIO_DIRENT_PRESENT
        #include <dirent.h>
    #endif
#elif defined(EA_PLATFORM_REVOLUTION)
    #include <revolution/dvd.h>
#elif defined(EA_PLATFORM_GAMECUBE)
    #include <dolphin/dvd.h>
#elif defined(__QNX__)
    #include <alloca.h>
#elif defined(EA_PLATFORM_PSP2)
    #include <kernel.h>
#endif


#if (EASTDC_VERSION_N < 10600)
    #error EAStdC 1.06.00 or later is required to compile this version of EAIO. This is due to char32_t support in EAStdC 1.06.00 and later.
#endif


namespace EA
{

namespace IO
{


/// ENTRYLIST_NAME
///
/// Defines a default container name in the absence of a user-provided name.
///
#define ENTRYLIST_NAME "DirectoryIterator"


namespace Internal
{
    template<class T>
    T* Allocate(EA::Allocator::ICoreAllocator* pAllocator, const char* pName)
    {
        T* const pT = (T*)pAllocator->Alloc(sizeof(T), pName, 0, 0);
        if(pT)
            new(pT) T;
        return pT;
    }

    template<class T>
    void Free(EA::Allocator::ICoreAllocator* pAllocator, T* pT)
    {
        if(pT)
        {
            pT->~T();
            pAllocator->Free(pT);
        }
    }

    #if defined(EA_PLATFORM_MICROSOFT)
        time_t SystemTimeToEAIOTime(const FILETIME& fileTime)
        {
            ULARGE_INTEGER ull;
            ull.LowPart  = fileTime.dwLowDateTime;
            ull.HighPart = fileTime.dwHighDateTime;
            return (time_t)((ull.QuadPart / 10000000ULL) - 11644473600ULL);
        }
    #endif


    void FilterEntries(EntryFindData* pEntryFindData, const wchar_t* pFilterPattern)
    {
        Path::EnsureTrailingSeparator(pEntryFindData->mDirectoryPath, kMaxPathLength);

        if(pFilterPattern)
        {
            EA::StdC::Strlcpy(pEntryFindData->mEntryFilterPattern, pFilterPattern, kMaxPathLength);
            pEntryFindData->mEntryFilterPattern[kMaxPathLength - 1] = 0;
        }
        else
        {
            pEntryFindData->mEntryFilterPattern[0] = '*';
            pEntryFindData->mEntryFilterPattern[1] = 0;
        }
    }

} // namespace Internal



///////////////////////////////////////////////////////////////////////////////
// Read
//
size_t DirectoryIterator::Read(const wchar_t* pDirectory, EntryList& entryList, 
                             const wchar_t* pFilterPattern, int nDirectoryEntryFlags, 
                             size_t maxResultCount, bool bReadFileStat)
{
    EntryFindData entryFindData, *pEntryFindData;
    size_t        resultCount = 0;

    #if EASTL_NAME_ENABLED // If the EntryList doesn't have a unique name, we give it one here.
        if(entryList.get_allocator().get_name() && !strcmp(EASTL_LIST_DEFAULT_NAME, entryList.get_allocator().get_name()))
            entryList.get_allocator().set_name(ENTRYLIST_NAME);
    #endif

    entryFindData.mbReadFileStat = bReadFileStat;

    // Iterate entries.
    for(pEntryFindData = EntryFindFirst(pDirectory, pFilterPattern, &entryFindData); pEntryFindData && (resultCount < maxResultCount); )
    {
        if(!StrEq(pEntryFindData->mName, EA_DIRECTORY_CURRENT_W) && // If it is neither "./" nor "../"
           !StrEq(pEntryFindData->mName, EA_DIRECTORY_PARENT_W))
        {
            if(pEntryFindData->mbIsDirectory) // If it's a directory...
            {
                if(nDirectoryEntryFlags & kDirectoryEntryDirectory) // If the user wants to enumerate directories...
                {
                    resultCount++;

                    entryList.push_back();
                    Entry& entry = entryList.back();

                    entry.mType             = kDirectoryEntryDirectory;
                    entry.msName            = pEntryFindData->mName;
                    entry.mCreationTime     = pEntryFindData->mCreationTime;
                    entry.mModificationTime = pEntryFindData->mModificationTime;
                    entry.mSize             = pEntryFindData->mSize;
                  //EA_ASSERT(entry.mSize < UINT64_C(10000000000)); // Sanity check.
                }
            }
            else    // Else it's a file.
            {
                if(nDirectoryEntryFlags & kDirectoryEntryFile) // If the user wants to enumerate files...
                {
                    resultCount++;

                    entryList.push_back();
                    Entry& entry = entryList.back();

                    entry.mType             = kDirectoryEntryFile;
                    entry.msName            = pEntryFindData->mName;
                    entry.mCreationTime     = pEntryFindData->mCreationTime;
                    entry.mModificationTime = pEntryFindData->mModificationTime;
                    entry.mSize             = pEntryFindData->mSize;
                  //EA_ASSERT(entry.mSize < UINT64_C(10000000000)); // Sanity check.
                }
            }
        }

        if(!EntryFindNext(pEntryFindData))
            break;
    }

    if(pEntryFindData)
    {
        EntryFindFinish(pEntryFindData);

        if((nDirectoryEntryFlags & kDirectoryEntryCurrent) && (resultCount < maxResultCount))
        {
            resultCount++;

            entryList.push_front();
            Entry& entry = entryList.front();

            entry.mType             = kDirectoryEntryDirectory;
            entry.msName            = EA_DIRECTORY_CURRENT_W;
            entry.mCreationTime     = pEntryFindData->mCreationTime;
            entry.mModificationTime = pEntryFindData->mModificationTime;
            entry.mSize             = pEntryFindData->mSize;
          //EA_ASSERT(entry.mSize < UINT64_C(10000000000)); // Sanity check.
        }

        if((nDirectoryEntryFlags & kDirectoryEntryParent) && (resultCount < maxResultCount))
        {
            // To do: We don't want to do this if the directory is a root directory. But how do we 
            // know for sure that we are at a root directory? Is it safe enough to compare to "/" on 
            // Unix-like systems and X:\\ on Microsoft systems? That seems reasonable, but what if 
            // the input pDirectory was a relative directory path and the user was dependent on the
            // system supporting relative paths?

            // if(the count of '/' and '\\' chars in pDirectory is > 1)
            {
                resultCount++;

                Entry& entry = entryList.front();

                entry.mType             = kDirectoryEntryDirectory;
                entry.msName            = EA_DIRECTORY_PARENT_W;
                entry.mCreationTime     = pEntryFindData->mCreationTime;
                entry.mModificationTime = pEntryFindData->mModificationTime;
                entry.mSize             = pEntryFindData->mSize;
              //EA_ASSERT(entry.mSize < UINT64_C(10000000000)); // Sanity check.
            }
        }
    }

    return resultCount;
}



///////////////////////////////////////////////////////////////////////////////
// ReadRecursive
//
size_t DirectoryIterator::ReadRecursive(const wchar_t* pBaseDirectory, EntryList& entryList, const wchar_t* pFilterPattern, int nEntryTypeFlags, 
                                        bool bIncludeBaseDirectoryInSearch, bool bFullPaths, size_t maxResultCount, bool bReadFileStat)
{
    EA::IO::Path::PathStringW pathTemp;

    if(mnRecursionIndex++ == 0) // If being called for the first time...
    {
        #if EASTL_NAME_ENABLED // If the EntryList doesn't have a unique name, we give it one here.
            if(entryList.get_allocator().get_name() && !strcmp(EASTL_LIST_DEFAULT_NAME, entryList.get_allocator().get_name()))
               entryList.get_allocator().set_name(ENTRYLIST_NAME);
        #endif

        mnListSize           = 0;
        mpBaseDirectory      = pBaseDirectory;
        mBaseDirectoryLength = (eastl_size_t)EA::StdC::Strlen(pBaseDirectory);
        if(!mBaseDirectoryLength || !IsFilePathSeparator(pBaseDirectory[mBaseDirectoryLength - 1]))
            mBaseDirectoryLength++;
    }

    if((nEntryTypeFlags & kDirectoryEntryFile) && 
       (bIncludeBaseDirectoryInSearch || (mnRecursionIndex > 1)) && 
       (mnListSize < maxResultCount))
    {
        // Add all files in the current directory into the list, using the filter pattern.
        const size_t additionCount = Read(pBaseDirectory, entryList, pFilterPattern, kDirectoryEntryFile, maxResultCount - mnListSize, bReadFileStat);

        EntryList::iterator it(entryList.end());
        eastl::advance(it, -(int32_t)(uint32_t)additionCount);

        for(; it != entryList.end(); ++it)
        {
            Entry& entry = *it;

            mnListSize++;

            const eastl_size_t savedLength = entry.msName.length();
            entry.msName.insert(0, pBaseDirectory);
            const eastl_size_t directoryEnd = entry.msName.length() - savedLength;

            if(directoryEnd && !IsFilePathSeparator(entry.msName[directoryEnd - 1]))
                entry.msName.insert(directoryEnd, 1, EA_FILE_PATH_SEPARATOR_W);

            if(!bFullPaths)
                entry.msName.erase(0, mBaseDirectoryLength);
        }
    }

    if(mnListSize < maxResultCount)
    {
        // To do: Find a way to avoid this temporary list.
        // Since the list is only a list of directories under the 
        // current directory, it shouldn't need all that many entries.
        EntryList entryListTemp(entryList.get_allocator());

        // Add all directories in the current directory into the list, ignoring the filter pattern.
        Read(pBaseDirectory, entryListTemp, NULL, kDirectoryEntryDirectory, kMaxEntryCountDefault, bReadFileStat);

        for(EntryList::iterator it = entryListTemp.begin(); (it != entryListTemp.end()) && (mnListSize < maxResultCount); ++it)
        {
            const Entry& entry = *it; 

            pathTemp.assign( pBaseDirectory );
            EA::IO::Path::Append( pathTemp, entry.msName.c_str() );  // This previously was Join but Join was calling Normalize, which was modifying the pBaseDirectory part of the path string, and we don't want that messed with. Actually we don't need any normalization.

            //ConcatenatePathComponents(pathTemp, pBaseDirectory, entry.msName.c_str());

            // Possibly add this directory to the entry list.
            if(nEntryTypeFlags & kDirectoryEntryDirectory)
            {
                if(!pFilterPattern || FnMatch(pFilterPattern, entry.msName.c_str(), kFNMCaseFold))
                {
                    mnListSize++;
                    entryList.push_back();
                    Entry& listEntry = entryList.back();
                    listEntry.mType  = kDirectoryEntryDirectory;
                    listEntry.msName = pathTemp.c_str();

                    if(!bFullPaths)
                        listEntry.msName.erase(0, mBaseDirectoryLength);
                }
            }

            // Now recursively read the subdirectory.
            ReadRecursive(pathTemp.c_str(), entryList, pFilterPattern, nEntryTypeFlags, true, bFullPaths, maxResultCount, bReadFileStat);
        }
    }

    mnRecursionIndex--;

    return mnListSize;
}


template <typename T>
size_t ReadRecursiveImpl(DirectoryIterator *_this, const T* pBaseDirectory, EA::IO::DirectoryIterator::EntryList& entryList, const T* pFilterPattern, int nEntryTypeFlags, 
                         bool bIncludeBaseDirectoryInSearch, bool bFullPaths, size_t maxResultCount, bool bReadFileStat)
{
    // TODO - Most part of this code could be merged with EntryFindFirstImpl
    wchar_t directoryPathBuffer[kMaxPathLength];
    wchar_t filterPatternBuffer[kMaxPathLength];
    wchar_t *pConvertedFilterPattern = NULL;

    int directoryPathConversionResult = EA::StdC::Strlcpy(directoryPathBuffer, pBaseDirectory, kMaxPathLength);
    bool directoryPathConversionSucceeded = directoryPathConversionResult >= 0 && directoryPathConversionResult < kMaxPathLength;
        
    int filterPatternConversionResult;
    bool filterPatternConversionSucceeded = true;

    if (pFilterPattern)
    {
        filterPatternConversionResult = EA::StdC::Strlcpy(filterPatternBuffer, pFilterPattern, kMaxPathLength);
        filterPatternConversionSucceeded = filterPatternConversionResult >= 0 && filterPatternConversionResult < kMaxPathLength;
        pConvertedFilterPattern = filterPatternBuffer;
    }

    if (directoryPathConversionSucceeded && filterPatternConversionSucceeded)
    {
        return _this->ReadRecursive(directoryPathBuffer, entryList, pConvertedFilterPattern, nEntryTypeFlags, bIncludeBaseDirectoryInSearch, bFullPaths, maxResultCount, bReadFileStat);
    }

    return 0;
}

size_t DirectoryIterator::ReadRecursive(const char8_t* pBaseDirectory, EntryList& entryList, const char8_t* pFilterPattern, int nDirectoryEntryFlags, 
                                        bool bIncludeBaseDirectoryInSearch, bool bFullPaths, size_t maxResultCount, bool bReadFileStat)
{
    return ReadRecursiveImpl<char8_t>(this, pBaseDirectory, entryList, pFilterPattern, nDirectoryEntryFlags, bIncludeBaseDirectoryInSearch, bFullPaths, maxResultCount, bReadFileStat);
}


#if EA_WCHAR_UNIQUE || EA_WCHAR_SIZE == 4
    size_t DirectoryIterator::ReadRecursive(const char16_t* pBaseDirectory, EntryList& entryList, const char16_t* pFilterPattern, int nDirectoryEntryFlags, 
                                            bool bIncludeBaseDirectoryInSearch, bool bFullPaths, size_t maxResultCount, bool bReadFileStat)
    {
        return ReadRecursiveImpl<char16_t>(this, pBaseDirectory, entryList, pFilterPattern, nDirectoryEntryFlags, bIncludeBaseDirectoryInSearch, bFullPaths, maxResultCount, bReadFileStat);
    }
#endif

    
#if EA_WCHAR_UNIQUE || EA_WCHAR_SIZE == 2
    size_t DirectoryIterator::ReadRecursive(const char32_t* pBaseDirectory, EntryList& entryList, const char32_t* pFilterPattern, int nDirectoryEntryFlags, 
                                            bool bIncludeBaseDirectoryInSearch, bool bFullPaths, size_t maxResultCount, bool bReadFileStat)
    {
        return ReadRecursiveImpl<char32_t>(this, pBaseDirectory, entryList, pFilterPattern, nDirectoryEntryFlags, bIncludeBaseDirectoryInSearch, bFullPaths, maxResultCount, bReadFileStat);
    }
#endif

#undef ENTRYLIST_NAME

                                      
#if defined(EA_PLATFORM_MICROSOFT)

    ////////////////////////////////////////////////////////////////////////////
    // EntryFindFirst
    //
    EAIO_API EntryFindData* EntryFindFirst(const wchar_t* pDirectoryPath, const wchar_t* pFilterPattern, EntryFindData* pEntryFindData)
    {
        using namespace Internal;

        if(pDirectoryPath[0] || (!pFilterPattern || pFilterPattern[0]))
        {
            const wchar_t pAny[] = { '*', 0 };
            Path::PathStringW pPathSpecification;
            if(pDirectoryPath)
                pPathSpecification += pDirectoryPath;
            Path::Append(pPathSpecification, pFilterPattern ? pFilterPattern : pAny);

            if(!pEntryFindData)
            {
                pEntryFindData = Allocate<EntryFindData>(IO::GetAllocator(), EAIO_ALLOC_PREFIX "EAFileDirectory/EntryFindData");
                pEntryFindData->mbIsAllocated = true;
            }

            #if defined(EA_PLATFORM_XENON) 

                Path::PathString8 directory8;
                // Measure how many UTF-8 chars we'll need (EASTL factors-in a hidden + 1 for NULL terminator)
                size_t nCharsNeeded = EA::StdC::Strlcpy((char8_t*)NULL, pPathSpecification.c_str(), 0);
                directory8.resize(nCharsNeeded);
                EA::StdC::Strlcpy(&directory8[0], pPathSpecification.c_str(), nCharsNeeded + 1);

                WIN32_FIND_DATAA win32FindDataA;
                HANDLE hFindFile = FindFirstFileA(directory8.c_str(), &win32FindDataA);

                if(hFindFile != INVALID_HANDLE_VALUE)
                {
                    EA::StdC::Strlcpy(pEntryFindData->mName, win32FindDataA.cFileName, kMaxPathLength);

                    pEntryFindData->mCreationTime     = Internal::SystemTimeToEAIOTime(win32FindDataA.ftCreationTime);
                    pEntryFindData->mModificationTime = Internal::SystemTimeToEAIOTime(win32FindDataA.ftLastWriteTime);
                    pEntryFindData->mSize             = ((uint64_t)win32FindDataA.nFileSizeHigh << 32i64) + win32FindDataA.nFileSizeLow;

                    pEntryFindData->mbIsDirectory = (win32FindDataA.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                    if(pEntryFindData->mbIsDirectory)
                        Path::EnsureTrailingSeparator(pEntryFindData->mName, kMaxPathLength);

                    if(pDirectoryPath)
                        EA::StdC::Strlcpy(pEntryFindData->mDirectoryPath, pDirectoryPath, kMaxPathLength);
                    else
                        pEntryFindData->mDirectoryPath[0] = 0;

                    if(pFilterPattern)
                        EA::StdC::Strlcpy(pEntryFindData->mEntryFilterPattern, pFilterPattern, kMaxPathLength);
                    else
                        pEntryFindData->mEntryFilterPattern[0] = 0;

                    pEntryFindData->mPlatformHandle = (uintptr_t)hFindFile;

                    return pEntryFindData;
                }

            #else // If wide character functions are supported...

                WIN32_FIND_DATAW win32FindDataW;
                #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
                    HANDLE hFindFile = FindFirstFileW(pPathSpecification.c_str(), &win32FindDataW);
                #else
                    HANDLE hFindFile = FindFirstFileExW(pPathSpecification.c_str(), FindExInfoBasic, &win32FindDataW, FindExSearchNameMatch, NULL, 0);
                #endif

                if(hFindFile != INVALID_HANDLE_VALUE)
                {
                    EA::StdC::Strlcpy(pEntryFindData->mName, win32FindDataW.cFileName, kMaxPathLength);

                    pEntryFindData->mbIsDirectory  = (win32FindDataW.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                    if(pEntryFindData->mbIsDirectory)
                        Path::EnsureTrailingSeparator(pEntryFindData->mName, kMaxPathLength);

                    pEntryFindData->mCreationTime     = Internal::SystemTimeToEAIOTime(win32FindDataW.ftCreationTime);
                    pEntryFindData->mModificationTime = Internal::SystemTimeToEAIOTime(win32FindDataW.ftLastWriteTime);
                    pEntryFindData->mSize             = ((uint64_t)win32FindDataW.nFileSizeHigh << 32i64) + win32FindDataW.nFileSizeLow;

                    if(pDirectoryPath)
                        EA::StdC::Strlcpy(pEntryFindData->mDirectoryPath, pDirectoryPath, kMaxPathLength);
                    else
                        pEntryFindData->mDirectoryPath[0] = 0;

                    if(pFilterPattern)
                        EA::StdC::Strlcpy(pEntryFindData->mEntryFilterPattern, pFilterPattern, kMaxPathLength);
                    else
                        pEntryFindData->mEntryFilterPattern[0] = 0;

                    pEntryFindData->mPlatformHandle = (uintptr_t)hFindFile;

                    return pEntryFindData;
                }
            #endif

            if (pEntryFindData->mbIsAllocated)
                Free(EA::IO::GetAllocator(), pEntryFindData);
        }

        return NULL;
    }

    

    ////////////////////////////////////////////////////////////////////////////
    // EntryFindNext
    //
    EAIO_API EntryFindData* EntryFindNext(EntryFindData* pEntryFindData)
    {
        if(pEntryFindData)
        {
            HANDLE hFindFile = (HANDLE)pEntryFindData->mPlatformHandle;

            #if defined(EA_PLATFORM_WINDOWS) // If wide character functions are supported...
                WIN32_FIND_DATAW win32FindDataW;

                if(FindNextFileW(hFindFile, &win32FindDataW))
                {
                    EA::StdC::Strlcpy(pEntryFindData->mName, win32FindDataW.cFileName, kMaxPathLength);

                    pEntryFindData->mbIsDirectory = (win32FindDataW.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                    if(pEntryFindData->mbIsDirectory)
                        Path::EnsureTrailingSeparator(pEntryFindData->mName, kMaxPathLength);

                    pEntryFindData->mCreationTime     = Internal::SystemTimeToEAIOTime(win32FindDataW.ftCreationTime);
                    pEntryFindData->mModificationTime = Internal::SystemTimeToEAIOTime(win32FindDataW.ftLastWriteTime);
                    pEntryFindData->mSize             = ((uint64_t)win32FindDataW.nFileSizeHigh << 32i64) + win32FindDataW.nFileSizeLow;

                    return pEntryFindData;
                }
            #else
                WIN32_FIND_DATAA win32FindDataA;

                if(FindNextFileA(hFindFile, &win32FindDataA))
                {
                    EA::StdC::Strlcpy(pEntryFindData->mName, win32FindDataA.cFileName, kMaxPathLength);

                    pEntryFindData->mbIsDirectory = (win32FindDataA.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                    if(pEntryFindData->mbIsDirectory)
                        Path::EnsureTrailingSeparator(pEntryFindData->mName, kMaxPathLength);

                    pEntryFindData->mCreationTime     = Internal::SystemTimeToEAIOTime(win32FindDataA.ftCreationTime);
                    pEntryFindData->mModificationTime = Internal::SystemTimeToEAIOTime(win32FindDataA.ftLastWriteTime);
                    pEntryFindData->mSize             = ((uint64_t)win32FindDataA.nFileSizeHigh << 32i64) + win32FindDataA.nFileSizeLow;

                    return pEntryFindData;
                }

                // Problem: The XBox and XBox 360 platforms don't always return "./" and "../" 
                // directories. We need to provide those if they aren't present and if they are
                // appropriate ("../" shouldn't be present for root directories).
            #endif
        }
        return NULL;
    }

    
    ////////////////////////////////////////////////////////////////////////////
    // EntryFindFinish
    //
    EAIO_API void EntryFindFinish(EntryFindData* pEntryFindData)
    {
        using namespace Internal;

        if(pEntryFindData)
        {
            HANDLE hFindFile = (HANDLE)pEntryFindData->mPlatformHandle;

            if((hFindFile != 0) && (hFindFile != INVALID_HANDLE_VALUE))
                FindClose(hFindFile);

            if(pEntryFindData->mbIsAllocated)
                Free(EA::IO::GetAllocator(), pEntryFindData);
        }
    }


#elif EAIO_USE_UNIX_IO && EAIO_DIRENT_PRESENT

    #ifdef EA_PLATFORM_ANDROID

        // include the header in the EA::IO namespace
        #ifndef ASSETMANAGER_INIT_FAILURE_MESSAGE
            #define ASSETMANAGER_INIT_FAILURE_MESSAGE "Note: EAIO.Startup(activityInstance) must be called from Java prior to accessing appbundle files."
        #endif
        #include "Android/assetmanagerjni.h"

        
        // The mPlatformHandle is set to point to this variable when the EntryFindData structure is
        // created for a appbundle:/ directory.  This avoids an expensive string operation to see
        // if the original directory was an appbundle directory.
        static char AndroidAppBundleEntryIndicator;

        struct AndroidEntryFindData
        {
            jobject list;
            int curListIndex;
        };

        static EntryFindData* AndroidEntryFindNext(const AutoJNIEnv &autoEnv, EntryFindData* pEntryFindData)
        {
            AndroidEntryFindData *pAndroidData = reinterpret_cast<AndroidEntryFindData*>(pEntryFindData->mPlatformData);
            wchar_t* entryName = pEntryFindData->mName;

            for(;;)
            {     
                if(AssetManagerJNI::GetListElement( autoEnv, pAndroidData->list, pAndroidData->curListIndex, entryName, kMaxPathLength ))
                {
                    pAndroidData->curListIndex++;   // Move to the next element

                    // HACK
                    // There doesn't seem to currently be a nice efficient way to determine if an entry is a directory
                    // or not.  So, we make the assumption that any entry without an extension is a directory.
                        wchar_t* pFileExtension = Path::GetFileExtension(entryName);

                    if(*pFileExtension)
                    {
                        pEntryFindData->mbIsDirectory = false;
                    }
                    else
                    {
                        pEntryFindData->mbIsDirectory = true;
                        pFileExtension[0] = '/';
                        pFileExtension[1] = '\0';
                    }

                    if((pEntryFindData->mEntryFilterPattern[0] == 0) || 
                        FnMatch(pEntryFindData->mEntryFilterPattern, entryName, kFNMNone))
                    {
                        // Return the item if it passes the filter
                        return pEntryFindData;
                    }
                }
                else
                {
                    // There are no more items available so return NULL to indicate completion.
                    return NULL;
                }
            }
        }

        static EntryFindData* AndroidEntryFindNext(EntryFindData* pEntryFindData)
        {
            AutoJNIEnv autoEnv;
            return AndroidEntryFindNext(autoEnv, pEntryFindData);
        }

        static void AndroidEntryFindFinish(const AutoJNIEnv &autoEnv, EntryFindData* pEntryFindData)
        {
            using namespace Internal;

            AndroidEntryFindData *pAndroidData = reinterpret_cast<AndroidEntryFindData*>(pEntryFindData->mPlatformData);
            AssetManagerJNI::FreeList(autoEnv, pAndroidData->list);

            if(pEntryFindData->mbIsAllocated)
                Free(EA::IO::GetAllocator(), pEntryFindData);
        }

        static void AndroidEntryFindFinish(EntryFindData* pEntryFindData)
        {
            AutoJNIEnv autoEnv;
            return AndroidEntryFindFinish(autoEnv, pEntryFindData);
        }

        static EntryFindData* AndroidEntryFindFirst(const wchar_t* pDirectoryPath, const wchar_t* pFilterPattern, EntryFindData* pEntryFindData)
        {
            using namespace Internal;

            AutoJNIEnv autoEnv;
                const wchar_t* path = pDirectoryPath + APPBUNDLE_PATH_PREFIX_LENGTH;
                jobject        list = AssetManagerJNI::List(autoEnv, path, EA::StdC::Strlen(path));

            if(list)
            {
                if(!pEntryFindData)
                {
                    pEntryFindData = Allocate<EntryFindData>(IO::GetAllocator(), EAIO_ALLOC_PREFIX "EAFileDirectory/EntryFindData");
                    memset(pEntryFindData, 0, sizeof(EntryFindData));   // Clear fields of structure
                    pEntryFindData->mbIsAllocated = true;
                }
                else
                {
                    memset(pEntryFindData, 0, sizeof(EntryFindData));   // Clear fields of structure
                }

                pEntryFindData->mPlatformHandle = reinterpret_cast<uintptr_t>(&AndroidAppBundleEntryIndicator);
                AndroidEntryFindData* pAndroidData = reinterpret_cast<AndroidEntryFindData*>(pEntryFindData->mPlatformData);

                FilterEntries(pEntryFindData, pFilterPattern);

                pAndroidData->list = list;
                pAndroidData->curListIndex = 0;
                EntryFindData* result = AndroidEntryFindNext(autoEnv, pEntryFindData);

                if(!result)
                    AndroidEntryFindFinish(autoEnv, pEntryFindData);

                return result;
            }

            return NULL;
        }

    #endif // #if defined(EA_PLATFORM_ANDROID)


    EAIO_API EntryFindData* EntryFindFirst(const wchar_t* pDirectoryPath, const wchar_t* pFilterPattern, EntryFindData* pEntryFindData)
    {
        using namespace Internal;

        if(pDirectoryPath[0] || (!pFilterPattern || pFilterPattern[0]))
        {
            Path::PathString8  directory8;

            // Measure how many UTF-8 chars we'll need (EASTL factors-in a hidden + 1 for NULL terminator)
            size_t nCharsNeeded = EA::StdC::Strlcpy((char8_t*)NULL, pDirectoryPath, 0);
            directory8.resize(nCharsNeeded);
            EA::StdC::Strlcpy(&directory8[0], pDirectoryPath, nCharsNeeded + 1);

          #ifdef EA_PLATFORM_ANDROID
            if(strstr(directory8.c_str(), APPBUNDLE_PATH_PREFIX) == directory8.c_str()) // If the path begins with the APPBUNDLE_PATH_PREFIX prefix..
                return AndroidEntryFindFirst(pDirectoryPath, pFilterPattern, pEntryFindData);
            else
          #endif
            {
                    DIR* const dir = opendir(directory8.c_str());
                    if(!dir)
                        return NULL;
                 
                // Follow windows semantics: don't distinguish between an empty directory and not finding anything.
                #if EAIO_READDIR_R_PRESENT
                    #if defined(__QNX__)
                        dirent* dirEntryData =  (dirent*)alloca(sizeof(*dirEntryData) + NAME_MAX + 1);
                    #else
                        dirent  dirEntryDataStorage;
                        dirent* dirEntryData = &dirEntryDataStorage;
                    #endif
                #endif
                dirent*  dirEntry;
                bool     bPatternMatch = false;
                wchar_t  entryName[kMaxPathLength]; // match pEntryFindData->mName

                #if defined(EA_PLATFORM_AIRPLAY)        // The Airplay mobile platform converts files to lower case.
                  const int kMatchFlags = kFNMCaseFold;
                #else
                  const int kMatchFlags = kFNMNone;
                #endif

                do // Run a loop so we can filter out any "." and ".." entries that might be present and entries that don't match the input pattern.
                {
                  #if EAIO_READDIR_R_PRESENT
                    if((readdir_r(dir, dirEntryData, &dirEntry) != 0) || !dirEntry)
                  #else
                    if((dirEntry = readdir(dir)) == NULL)
                  #endif
                    {
                        closedir(dir);
                        return NULL; // If no directory entry matches the user-specified pattern, we'll usually be exiting via this line due to (dirEntry == NULL).
                    }

                    if((strcmp(dirEntry->d_name, ".")  == 0) ||
                       (strcmp(dirEntry->d_name, "..") == 0))
                    {
                        bPatternMatch = false;
                    }
                    else
                    {
                        EA::StdC::Strlcpy(entryName, dirEntry->d_name, kMaxPathLength, kLengthNull);
                        bPatternMatch = !pFilterPattern || FnMatch(pFilterPattern, entryName, kMatchFlags);
                    }
                } while(!bPatternMatch);

                if(!pEntryFindData)
                {
                    pEntryFindData = Allocate<EntryFindData>(IO::GetAllocator(), EAIO_ALLOC_PREFIX "EAFileDirectory/EntryFindData");
                    pEntryFindData->mbIsAllocated = true;
                }

                EA::StdC::Strlcpy(pEntryFindData->mName, entryName, kMaxPathLength);

                #if defined(__CYGWIN__) || defined(__QNX__)// Cygwin chooses to take the most limited view of the Posix standard and doesn't implement d_type.
                    Path::EnsureTrailingSeparator(directory8);
                    directory8 += dirEntry->d_name;
                    pEntryFindData->mbIsDirectory = Directory::Exists(directory8.c_str());
                #else
                    pEntryFindData->mbIsDirectory = (dirEntry->d_type == DT_DIR);
                #endif

                if(pEntryFindData->mbIsDirectory)
                    Path::EnsureTrailingSeparator(pEntryFindData->mName, kMaxPathLength);

                EA::StdC::Strlcpy(pEntryFindData->mDirectoryPath, pDirectoryPath, kMaxPathLength);
                Path::EnsureTrailingSeparator(pEntryFindData->mDirectoryPath, kMaxPathLength);

                // mCreationTime / mModificationTime / mSize
                pEntryFindData->mCreationTime     = 0;
                pEntryFindData->mModificationTime = 0;
                pEntryFindData->mSize             = 0;

                if(pEntryFindData->mbReadFileStat)
                {
                    Path::PathString8 path8;
                    ConvertPath(path8, pEntryFindData->mDirectoryPath);
                    path8 += dirEntry->d_name;

                    struct stat tempStat;
                    const int result = stat(path8.c_str(), &tempStat);

                    if(result == 0)
                    {
                        pEntryFindData->mCreationTime     = (time_t)tempStat.st_ctime;
                        pEntryFindData->mModificationTime = (time_t)tempStat.st_mtime;
                        pEntryFindData->mSize             = (uint64_t)tempStat.st_size;
                    }
                }

                // Clean up and return
                FilterEntries(pEntryFindData, pFilterPattern);

                pEntryFindData->mPlatformHandle = (uintptr_t) dir;
                 
                return pEntryFindData;
            }
        }

        return NULL;
    }


    EAIO_API EntryFindData* EntryFindNext(EntryFindData* pEntryFindData)
    {
        if(pEntryFindData)
        {
            #ifdef EA_PLATFORM_ANDROID
                if( pEntryFindData->mPlatformHandle == reinterpret_cast<uintptr_t>(&AndroidAppBundleEntryIndicator))
                    return AndroidEntryFindNext(pEntryFindData);
            #endif

            // Unix defines DIR as an opaque pointer whose type is undefined. It would be useful to verify that in code here.
            // EA_COMPILETIME_ASSERT(sizeof(DIR) <= sizeof(pEntryFindData->mPlatformData));
            DIR* const dir = (DIR*)pEntryFindData->mPlatformHandle;
              
            #if EAIO_READDIR_R_PRESENT
                #if defined(__QNX__)
                    dirent* dirEntryData = (dirent*)alloca(sizeof(*dirEntryData) + NAME_MAX + 1);
                #else
                    dirent  dirEntryDataStorage;
                    dirent *dirEntryData = &dirEntryDataStorage;
                #endif
            #endif
            dirent* dirEntry;

            #if defined(EA_PLATFORM_AIRPLAY)         // The Airplay mobile platform converts files to lower case.
              const int kMatchFlags = kFNMCaseFold;
            #else
              const int kMatchFlags = kFNMNone;
            #endif

            #if EAIO_READDIR_R_PRESENT
              if(readdir_r(dir, dirEntryData, &dirEntry) != 0)
            #else
              if((dirEntry = readdir(dir)) == NULL)
            #endif
                return NULL; // If no directory entry matches the user-specified pattern, we'll usually be exiting via this line due to (dirEntry == NULL).

            wchar_t entryName[kMaxPathLength]; // match pEntryFindData->mName
              
            while(dirEntry)
            {     
                EA::StdC::Strlcpy(entryName, dirEntry->d_name, kMaxPathLength, kLengthNull);

                if((pEntryFindData->mEntryFilterPattern[0] == 0) || 
                    FnMatch(pEntryFindData->mEntryFilterPattern, entryName, kMatchFlags))
                {
                    EA::StdC::Strlcpy(pEntryFindData->mName, entryName, kMaxPathLength);

                    #if defined(__CYGWIN__) || defined(__QNX__)// Cygwin chooses to take the most limited view of the Posix standard and doesn't implement d_type.
                        Path::PathStringW tempW(pEntryFindData->mDirectoryPath);
                        tempW += entryName;
                        pEntryFindData->mbIsDirectory = Directory::Exists(tempW.c_str());
                    #else
                        pEntryFindData->mbIsDirectory = (dirEntry->d_type == DT_DIR);
                    #endif

                    if(pEntryFindData->mbIsDirectory)
                        Path::EnsureTrailingSeparator(pEntryFindData->mName, kMaxPathLength);

                    // mCreationTime / mModificationTime / mSize
                    pEntryFindData->mCreationTime     = 0;
                    pEntryFindData->mModificationTime = 0;
                    pEntryFindData->mSize             = 0;

                    if(pEntryFindData->mbReadFileStat)
                    {
                        Path::PathString8 path8;
                        ConvertPath(path8, pEntryFindData->mDirectoryPath);
                        path8 += dirEntry->d_name;

                        struct stat tempStat;
                        const int result = stat(path8.c_str(), &tempStat);

                        if(result == 0)
                        {
                            pEntryFindData->mCreationTime     = (time_t)tempStat.st_ctime;
                            pEntryFindData->mModificationTime = (time_t)tempStat.st_mtime;
                            pEntryFindData->mSize             = (uint64_t)tempStat.st_size;
                        }
                    }

                    // Clean up and return
                    return pEntryFindData;
                }

                #if EAIO_READDIR_R_PRESENT
                  if(readdir_r(dir, dirEntryData, &dirEntry) != 0)
                #else
                  if((dirEntry = readdir(dir)) == NULL)
                #endif
                {
                    return NULL;
                }
            }  
        }
         
        return NULL;
    }


    EAIO_API void EntryFindFinish(EntryFindData* pEntryFindData)
    {
        using namespace Internal;

        if(pEntryFindData)
        {
            #ifdef EA_PLATFORM_ANDROID
                if( pEntryFindData->mPlatformHandle == reinterpret_cast<uintptr_t>(&AndroidAppBundleEntryIndicator))
                {
                    AndroidEntryFindFinish(pEntryFindData);
                    return;
                }
            #endif

            DIR* dir = (DIR*)pEntryFindData->mPlatformHandle;

            if(dir)
                closedir(dir);

            if(pEntryFindData->mbIsAllocated)
                Free(EA::IO::GetAllocator(), pEntryFindData);
        }
    }

#elif defined(EA_PLATFORM_PS3)

    struct EntryFindDataPS3
    {
        int  mFD;
        bool mbDotEnumerated;
        bool mbDotDotEnumerated;
    };


    EAIO_API EntryFindData* EntryFindFirst(const wchar_t* pDirectory, const wchar_t* pFilterPattern, EntryFindData* pEntryFindData)
    {
        using namespace Internal;

        if(pDirectory[0] || (!pFilterPattern || pFilterPattern[0]))
        {
            Path::PathString16 directoryTemp16;
            if(pDirectory)
                directoryTemp16 += pDirectory;
            StripTrailingSeparator(directoryTemp16);

            Path::PathString8 directory8;
            // Measure how many UTF-8 chars we'll need (EASTL factors-in a hidden + 1 for NULL terminator)
            size_t nCharsNeeded = EA::StdC::Strlcpy((char8_t*)NULL, directoryTemp16.c_str(), 0);
            directory8.resize(nCharsNeeded);
            EA::StdC::Strlcpy(&directory8[0], directoryTemp16.c_str(), nCharsNeeded + 1);

            int fd;
            CellFsErrno result = cellFsOpendir(directory8.c_str(), &fd);

            if(result == CELL_FS_SUCCEEDED)
            {
                if(!pEntryFindData)
                {
                    pEntryFindData = Allocate<EntryFindData>(IO::GetAllocator(), EAIO_ALLOC_PREFIX "EAFileDirectory/EntryFindData");
                    pEntryFindData->mbIsAllocated = true;
                }

                EA_COMPILETIME_ASSERT(sizeof(EntryFindDataPS3) <= sizeof(pEntryFindData->mPlatformData));
                EntryFindDataPS3 efdPS3 = { fd, false, false };
                memcpy(pEntryFindData->mPlatformData, &efdPS3, sizeof(efdPS3));

                EA::StdC::Strlcpy(pEntryFindData->mDirectoryPath, directoryTemp16.c_str(), kMaxPathLength);
                FilterEntries(pEntryFindData, pFilterPattern);

                // The following should always succeed, because "./" is always present.
                pEntryFindData = EntryFindNext(pEntryFindData);

                if(pEntryFindData == NULL)
                    cellFsClosedir(fd);

                return pEntryFindData;
            }
        }

        return NULL;
    }

    EAIO_API EntryFindData* EntryFindNext(EntryFindData* pEntryFindData)
    {
        EntryFindDataPS3 efdPS3;
        memcpy(&efdPS3, pEntryFindData->mPlatformData, sizeof(efdPS3));

        CellFsDirent cellFsDirent;
        uint64_t n = 0;

        // A return of CELL_FS_SUCCEEDED with n == 0 means that 
        // there are simply no more entries to read.
        // If the directory exists but it has no entries, 
        // we expect that result == success and n == 0.
        CellFsErrno result = cellFsReaddir(efdPS3.mFD, &cellFsDirent, &n);

        while((result == CELL_FS_SUCCEEDED) && (n != 0))
        {
            EA::StdC::Strlcpy(pEntryFindData->mName, cellFsDirent.d_name, kMaxPathLength);

            if(!pEntryFindData->mEntryFilterPattern[0] || FnMatch(pEntryFindData->mEntryFilterPattern, pEntryFindData->mName, kFNMCaseFold))
            {
                // As of this writing, the PS3 SDK is broken and reports all entries as files.
                // Thus the d_type field is wrong. So we attempt to work around it here.
                //    Doesn't work: efdPS3.mbIsDirectory = (cellFsDirent.d_type == CELL_FS_TYPE_DIRECTORY);
                // Our attempted workaround:
                if(strcmp(cellFsDirent.d_name, "./") == 0)
                    pEntryFindData->mbIsDirectory = true;
                else if(strcmp(cellFsDirent.d_name, "../") == 0)
                    pEntryFindData->mbIsDirectory = true;
                else
                {
                    // Obtain full path in UTF16
                    Path::PathString16 fullPath16(pEntryFindData->mDirectoryPath);
                    Join(fullPath16, pEntryFindData->mName);
                    
                    // Convert to UTF8 and test for existence
                    // Measure how many UTF-8 chars we'll need (EASTL factors-in a hidden + 1 for NULL terminator)
                    size_t nCharsNeeded = EA::StdC::Strlcpy((char8_t*)NULL, fullPath16.c_str(), 0);
                    
                    Path::PathString8 fullPath8;
                    fullPath8.resize(nCharsNeeded);
                    EA::StdC::Strlcpy(&fullPath8[0], fullPath16.c_str(), nCharsNeeded + 1);

                    // PS3 doesn't give us a way to find out if directories exist, 
                    // but we can possibly use the cellFsOpenDir function for this purpose.
                    int fd;
                    pEntryFindData->mbIsDirectory = (cellFsOpendir(fullPath8.c_str(), &fd) == CELL_FS_SUCCEEDED);
                    if(pEntryFindData->mbIsDirectory)
                        cellFsClosedir(fd);

                    // Make sure directories are /-terminated.
                    // if(pEntryFindData->mbIsDirectory) Disabled until we can prove that user code isn't dependent on the current behaviour and the current behavior isn't to to have a /.
                    //     Path::EnsureTrailingSeparator(pEntryFindData->mDirectoryPath, kMaxPathLength);
                }

                // mCreationTime / mModificationTime / mSize
                if(pEntryFindData->mbReadFileStat)
                {
                    wchar_t filePath[EA::IO::kMaxPathLength];
                    EA::StdC::Strlcpy(filePath, pEntryFindData->mDirectoryPath, EA::IO::kMaxPathLength);    // Important to use mDirectoryPath here because it is guaranteed to be terminated with a / char.
                    EA::StdC::Strlcat(filePath, pEntryFindData->mName, EA::IO::kMaxPathLength);             
                    pEntryFindData->mCreationTime = EA::IO::File::GetTime(filePath, EA::IO::kFileTimeTypeCreation);

                    pEntryFindData->mModificationTime = EA::IO::File::GetTime(filePath, EA::IO::kFileTimeTypeLastModification);

                    pEntryFindData->mSize = EA::IO::File::GetSize(filePath);
                }

                // We put this code after the above code because we use mName as the OS gives it to us and not necessarily with a trailing path separator like we force-append below.
                if(pEntryFindData->mbIsDirectory)
                    Path::EnsureTrailingSeparator(pEntryFindData->mName, kMaxPathLength);

                return pEntryFindData;
            }

            pEntryFindData->mName[0] = 0;
            result = cellFsReaddir(efdPS3.mFD, &cellFsDirent, &n);
        }

        return false;
    }

    EAIO_API void EntryFindFinish(EntryFindData* pEntryFindData)
    {
        using namespace Internal;

        if(pEntryFindData)
        {
            EntryFindDataPS3 efdPS3;
            memcpy(&efdPS3, pEntryFindData->mPlatformData, sizeof(efdPS3));

            cellFsClosedir(efdPS3.mFD);

            if(pEntryFindData->mbIsAllocated)
                Free(EA::IO::GetAllocator(), pEntryFindData);
        }
    }

#elif defined(EA_PLATFORM_GAMECUBE) || defined(EA_PLATFORM_REVOLUTION)

    struct EntryFindDataGC
    {
        DVDDir mDir;
        bool   mbDotEnumerated;
        bool   mbDotDotEnumerated;
    };


    EAIO_API EntryFindData* EntryFindFirst(const wchar_t* pDirectory, const wchar_t* pFilterPattern, EntryFindData* pEntryFindData)
    {
        using namespace Internal;

        // To consider: If pDirectory is empty, use the current working directory.
        if(pDirectory[0] || (!pFilterPattern || pFilterPattern[0]))
        {
            Path::PathString16 directoryTemp16(pDirectory);
            StripTrailingSeparator(directoryTemp16);
    
            Path::PathString8 directory8;
            // Measure how many UTF-8 chars we'll need (EASTL factors-in a hidden + 1 for NULL terminator)
            size_t nCharsNeeded = EA::StdC::Strlcpy((char8_t*)NULL, directoryTemp16.c_str(), 0);
            directory8.resize(nCharsNeeded);
            EA::StdC::Strlcpy(&directory8[0], directoryTemp16.c_str(), nCharsNeeded + 1);
    
            DVDDir dir;
            bool result = DVDOpenDir(directory8.c_str(), &dir) == TRUE;
    
            if(result)
            {
                if(!pEntryFindData)
                {
                    pEntryFindData = Allocate<EntryFindData>(IO::GetAllocator(), EAIO_ALLOC_PREFIX "EAFileDirectory/EntryFindData");
                    pEntryFindData->mbIsAllocated = true;
                }

                EA_COMPILETIME_ASSERT(sizeof(EntryFindDataGC) <= sizeof(pEntryFindData->mPlatformData));
                EntryFindDataGC* const pGC = (EntryFindDataGC*)pEntryFindData->mPlatformData;
    
                pGC->mDir = dir;
                pGC->mbDotEnumerated = false;
                pGC->mbDotDotEnumerated = false;
    
                EA::StdC::Strlcpy(pEntryFindData->mDirectoryPath, directoryTemp16.c_str(), kMaxPathLength);
                FilterEntries(pEntryFindData, pFilterPattern);
    
                // This should always succeed, because '.' is always present.
                pEntryFindData = EntryFindNext(pEntryFindData);
    
                return pEntryFindData;
            }
        }

        return NULL;
    }


    EAIO_API EntryFindData* EntryFindNext(EntryFindData* pEntryFindData)
    {
        EntryFindDataGC* const pGC = (EntryFindDataGC*)pEntryFindData->mPlatformData;

        DVDDirEntry dirEntry;
        bool result = DVDReadDir(&pGC->mDir, &dirEntry) == TRUE;

        while(result)
        {
            EA::StdC::Strlcpy(pEntryFindData->mName, dirEntry.name, kMaxPathLength);

            if(!pEntryFindData->mEntryFilterPattern[0] || FnMatch(pEntryFindData->mEntryFilterPattern, pEntryFindData->mName, kFNMNone))
            {
                pEntryFindData->mbIsDirectory = (dirEntry.isDir == TRUE);

                // mCreationTime / mModificationTime / mSize
                if(pEntryFindData->mbReadFileStat)
                {
                    wchar_t filePath[EA::IO::kMaxPathLength];
                    EA::StdC::Strlcpy(filePath, pEntryFindData->mDirectoryPath, EA::IO::kMaxPathLength);    // Important to use mDirectoryPath here because it is guaranteed to be terminated with a / char.
                    EA::StdC::Strlcat(filePath, pEntryFindData->mName, EA::IO::kMaxPathLength); 
                    pEntryFindData->mCreationTime = EA::IO::File::GetTime(filePath, EA::IO::kFileTimeTypeCreation);

                    pEntryFindData->mModificationTime = EA::IO::File::GetTime(filePath, EA::IO::kFileTimeTypeLastModification);

                    pEntryFindData->mSize = EA::IO::File::GetSize(filePath);
                }

                // Make sure directories are /-terminated.
                // if(pEntryFindData->mbIsDirectory) Disabled until we can prove that user code isn't dependent on the current behaviour and the current behavior isn't to to have a /.
                //     Path::EnsureTrailingSeparator(pEntryFindData->mDirectoryPath, kMaxPathLength);

                return pEntryFindData;
            }

            pEntryFindData->mName[0] = 0;
            result = DVDReadDir(&pGC->mDir, &dirEntry) == TRUE;
        }

        return NULL;
    }
    
    
    EAIO_API void EntryFindFinish(EntryFindData* pEntryFindData)
    {
        using namespace Internal;

        if(pEntryFindData)
        {
            EntryFindDataGC* const pGC = (EntryFindDataGC*)pEntryFindData->mPlatformData;

            DVDCloseDir(&pGC->mDir);

            if(pEntryFindData->mbIsAllocated)
                Free(EA::IO::GetAllocator(), pEntryFindData);
        }
    }
#elif defined(EA_PLATFORM_PSP2)

    struct EntryFindDataPSP2
    {
        SceUID dirHandle;
        
        enum
        {
            NEITHER_LISTED,
            CURRENT_DIR_LISTED,
            PREVIOUS_DIR_LISTED
        } relativeDirListingState;
    };

    EAIO_API EntryFindData* EntryFindFirst(const char16_t* pDirectoryPath, const char16_t* pFilterPattern, EntryFindData* pEntryFindData)
    {
        using namespace Internal;

        // To consider: If pDirectory is empty, use the current working directory.
        if(pDirectoryPath && *pDirectoryPath)
        {
            Path::PathString16 directoryTemp16(pDirectoryPath);
            StripTrailingSeparator(directoryTemp16);
    
            Path::PathString8 directory8;
            // Measure how many UTF-8 chars we'll need (EASTL factors-in a hidden + 1 for NULL terminator)
            size_t nCharsNeeded = StrlcpyUTF16ToUTF8(NULL, 0, directoryTemp16.c_str());
            directory8.resize(nCharsNeeded);
            StrlcpyUTF16ToUTF8(&directory8[0], nCharsNeeded + 1, directoryTemp16.c_str());
    
            SceUID result = sceIoDopen(directory8.c_str());

            if(result >= 0)
            {
                if(!pEntryFindData)
                {
                    pEntryFindData = Allocate<EntryFindData>(IO::GetAllocator(), EAIO_ALLOC_PREFIX "EAFileDirectory/EntryFindData");
                    pEntryFindData->mbIsAllocated = true;
                }

                EA_COMPILETIME_ASSERT(sizeof(EntryFindDataPSP2) <= sizeof(pEntryFindData->mPlatformData));
                EntryFindDataPSP2* const psp2Data = reinterpret_cast<EntryFindDataPSP2*>(pEntryFindData->mPlatformData);
    
                psp2Data->dirHandle = result;
                psp2Data->relativeDirListingState = EntryFindDataPSP2::NEITHER_LISTED;
    
                EAIOStrlcpy16(pEntryFindData->mDirectoryPath, directoryTemp16.c_str(), kMaxPathLength);
                FilterEntries(pEntryFindData, pFilterPattern);

                pEntryFindData = EntryFindNext(pEntryFindData);

                if(pEntryFindData == NULL)
                    sceIoDclose(result);
    
                return pEntryFindData;
            }
        }

        return NULL;
    }

    EAIO_API EntryFindData* EntryFindNext(EntryFindData* pEntryFindData)
    {
        EA_ASSERT(pEntryFindData != NULL);

        EntryFindDataPSP2* const psp2Data = reinterpret_cast<EntryFindDataPSP2*>(pEntryFindData->mPlatformData);

        for(;;)
        {
            pEntryFindData->mName[0] = 0;
            SceIoDirent entry;
            memset(static_cast<void*>(&entry), 0, sizeof(SceIoDirent));
            int32_t result = sceIoDread(psp2Data->dirHandle, &entry);

            if (result > 0)
            {
                StrlcpyUTF8ToUTF16(pEntryFindData->mName, kMaxPathLength, entry.d_name);
                pEntryFindData->mbIsDirectory = (bool)(entry.d_stat.st_mode & SCE_STM_FDIR);
            }
            else
            {
                // on psp2 we must manually enumerate the relative directory names after all of the 
                // other entires have been listed
                if (psp2Data->relativeDirListingState == EntryFindDataPSP2::NEITHER_LISTED)
                {
                    psp2Data->relativeDirListingState = EntryFindDataPSP2::CURRENT_DIR_LISTED;
                    pEntryFindData->mbIsDirectory = true;
                    EAIOStrlcpy16(pEntryFindData->mName, EA_DIRECTORY_CURRENT_16, kMaxPathLength);
                }
                else if (psp2Data->relativeDirListingState == EntryFindDataPSP2::CURRENT_DIR_LISTED)
                {
                    psp2Data->relativeDirListingState = EntryFindDataPSP2::PREVIOUS_DIR_LISTED;
                    pEntryFindData->mbIsDirectory = true;
                    EAIOStrlcpy16(pEntryFindData->mName, EA_DIRECTORY_PARENT_16, kMaxPathLength);
                }
                else
                {
                    //at this point, we've listed all possible files, so exit and return NULL
                    EA_ASSERT(psp2Data->relativeDirListingState == EntryFindDataPSP2::PREVIOUS_DIR_LISTED);
                    break;
                }
            }

            if (pEntryFindData->mbIsDirectory)
            {
                //ensure that directories have the '/'
                Path::EnsureTrailingSeparator(pEntryFindData->mName, kMaxPathLength);
            }

            if(pEntryFindData->mEntryFilterPattern[0] == '\0' || FnMatch(pEntryFindData->mEntryFilterPattern, pEntryFindData->mName, kFNMNone))
            {
                return pEntryFindData;
            }
        }

        return NULL;
    }

    EAIO_API void EntryFindFinish(EntryFindData* pEntryFindData)
    {
        using namespace Internal;

        if(pEntryFindData)
        {
            EntryFindDataPSP2* const psp2Data = reinterpret_cast<EntryFindDataPSP2*>(pEntryFindData->mPlatformData);

            int32_t result = sceIoDclose(psp2Data->dirHandle);
            EA_ASSERT(result == SCE_OK);

            if(pEntryFindData->mbIsAllocated)
            {
                Free(EA::IO::GetAllocator(), pEntryFindData);
            }
        }
    }

#else

    EAIO_API EntryFindData* EntryFindFirst(const wchar_t* /*pDirectoryPath*/, const wchar_t* /*pFilterPattern*/, EntryFindData* /*pEntryFindData*/)
    {
        // To do.
        return NULL;
    }

    EAIO_API EntryFindData* EntryFindNext(EntryFindData* /*pEntryFindData*/)
    {
        // To do.
        return NULL;
    }

    EAIO_API void EntryFindFinish(EntryFindData* /*pEntryFindData*/)
    {
        // To do.
    }

#endif // defined(<platform>)

    template<typename T>
    EntryFindData* EntryFindFirstImpl(const T* pDirectoryPath, const T* pFilterPattern, EntryFindData* pEntryFindData)
    {
        // TODO - Most part of this code could be merged with ReadRecursiveImpl
        wchar_t directoryPathBuffer[kMaxPathLength];
        wchar_t filterPatternBuffer[kMaxPathLength];
        wchar_t *pConvertedFilterPattern = NULL;

        int directoryPathConversionResult = EA::StdC::Strlcpy(directoryPathBuffer, pDirectoryPath, kMaxPathLength);
        bool directoryPathConversionSucceeded = directoryPathConversionResult >= 0 && directoryPathConversionResult < kMaxPathLength;
        
        int filterPatternConversionResult;
        bool filterPatternConversionSucceeded = true;

        if (pFilterPattern)
        {
            filterPatternConversionResult = EA::StdC::Strlcpy(filterPatternBuffer, pFilterPattern, kMaxPathLength);
            filterPatternConversionSucceeded = filterPatternConversionResult >= 0 && filterPatternConversionResult < kMaxPathLength;
            pConvertedFilterPattern = filterPatternBuffer;
        }

        if (directoryPathConversionSucceeded && filterPatternConversionSucceeded)
        {
            return EntryFindFirst(directoryPathBuffer, pConvertedFilterPattern, pEntryFindData);
        }

        return NULL;
    }

    EntryFindData* EntryFindFirst(const char8_t*   pDirectoryPath, const char8_t*  pFilterPattern, EntryFindData* pEntryFindData)
    {
        return EntryFindFirstImpl<char8_t>(pDirectoryPath, pFilterPattern, pEntryFindData);
    }

    #if EA_WCHAR_UNIQUE || EA_WCHAR_SIZE == 4
        EntryFindData* EntryFindFirst(const char16_t*  pDirectoryPath, const char16_t* pFilterPattern, EntryFindData* pEntryFindData)
        {
            return EntryFindFirstImpl<char16_t>(pDirectoryPath, pFilterPattern, pEntryFindData);
        }
    #endif

    #if EA_WCHAR_UNIQUE || EA_WCHAR_SIZE == 2
        EntryFindData* EntryFindFirst(const char32_t*  pDirectoryPath, const char32_t* pFilterPattern, EntryFindData* pEntryFindData)
        {
            return EntryFindFirstImpl<char32_t>(pDirectoryPath, pFilterPattern, pEntryFindData);
        }
    #endif

} // namespace IO


} // namespace EA















