///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/File.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/Config.h>
#include <EAPatchClient/File.h>
#include <EAPatchClient/Debug.h>
#include <EAPatchClient/Hash.h>
#include <EAStdC/EABitTricks.h>
#include <EAStdC/EAAlignment.h>
#include <EAStdC/EAMemory.h>
#include <EAStdC/EAString.h>
#include <EASTL/algorithm.h>
#include <EASTL/sort.h>
#include <EAIO/EAFileStream.h>
#include <EAIO/EAFileDirectory.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAStreamBuffer.h>
#include <EAIO/EAStreamAdapter.h>
#include <EAIO/EAStreamChild.h>
#include <EAIO/PathString.h>
#include <EAIO/FnMatch.h>

#if defined(_MSC_VER)
    #pragma warning(disable: 4127) // conditional expression is constant
#endif


namespace EA
{
namespace Patch
{


/////////////////////////////////////////////////////////////////////////////
// FileFilter
/////////////////////////////////////////////////////////////////////////////

// To do: Make this inline, so it doesn't need to be DLL-exported for a DLL build.
bool operator==(const FileFilter& a, const FileFilter& b)
{ 
    // Case insensitive compare. To consider: Should this be case-sensitive on Unix and similar platforms?
    return (a.mFilterSpec.comparei(b.mFilterSpec) == 0) && 
           (a.mMatchFlags == b.mMatchFlags); 
}


/////////////////////////////////////////////////////////////////////////////
// GetFileList
/////////////////////////////////////////////////////////////////////////////

enum MatchType
{
    kMTEmpty,
    kMTMatch,
    kMTUnmatched
};

// See the function declaration documentation for a description of this truth table.
static const bool truthTable[3][3] =  // Each of the 3 matches MatchType types.
{
    { false, false, true  }, // truthTable[0][0], truthTable[0][1], truthTable[0][2]
    { true,  true,  true  }, // truthTable[1][0], truthTable[1][1], truthTable[1][2]
    { false, false, false }  // truthTable[2][0], truthTable[2][1], truthTable[2][2]
};

bool GetFileList(const char8_t* pRootDirectoryPath, const FileFilterArray& ignoredFileArray, FileInfoList* pIgnoredFileList, 
                 const FileFilterArray& usedFileArray, FileInfoList* pUsedFileList, const StringList* pAppendedExtensionList, 
                 bool bReturnFullPaths, bool bReadFileSize, Error& error)
{
    // To consider: This code has a lot of overlap with a similar function in EAPatchMaker's BuildPatchFileInfoList. There may be
    // a way to coalesce part of them, for the purpose of ensuring that what we think of as ignored files here gets the
    // same treatment in EAPatchMaker's BuildPatchFileInfoList.
    bool bSuccess = true;

    EA::IO::DirectoryIterator dir;
    EA::IO::DirectoryIterator::EntryList entryList;
    static const uint64_t kMaxPatchFileCountDefault = 100000; // Arbitrary sanity check limit. If this is breached then the root patch directory is probably wrong.

    // We unilaterally set the bFullPaths argument of ReadRecursive below to true, because it turns out that ReadRecursive
    // internally always reads the full paths and just chops them if bFullPaths is false. So in practice it's more efficient 
    // and more memory-friendly for us to do this full/relative handling on our side.

    wchar_t directoryW[EA::IO::kMaxPathLength]; // For EAIO_VERSION_N >= 21600 we don't to do this directory path copying.
    EA::StdC::Strlcpy(directoryW, pRootDirectoryPath, EAArrayCount(directoryW));

    const size_t entryCount = dir.ReadRecursive(directoryW, entryList, NULL, 
                                                EA::IO::kDirectoryEntryFile | EA::IO::kDirectoryEntryDirectory, 
                                                true, true, static_cast<size_t>(kMaxPatchFileCountDefault), bReadFileSize);
    if(entryCount == EA::IO::kSizeTypeError)
    {
        error.SetErrorId(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdDirReadError, pRootDirectoryPath);
        bSuccess = false;
    }
    else if(entryCount >= kMaxPatchFileCountDefault) // If we hit the user limit...
    {
        error.SetErrorId(kECBlocking, kSystemErrorIdNone, kEAPatchErrorIdTooManyFiles, pRootDirectoryPath); 
        bSuccess = false;
    }
    else
    {
        // We now have an EntryList populated with all the files and directory paths under pRootDirectoryPath.
        // We next need to move all entries which are set to be ignored, based on the mPatchInfo.mIgnoredFileArray filters.

        String sFilePath;       // This is the path to the file
        String sFilePathTest;   // This is a possibly altered path to a file, for the purpose of doing comparisons.

        // For each file and directory found in pRootDirectoryPath...
        for(EA::IO::DirectoryIterator::EntryList::iterator it = entryList.begin(); it != entryList.end(); ++it)
        {
            EA::IO::DirectoryIterator::Entry& entry = *it;
            FileInfo* pFileInfo = NULL;

            // If turns out that DirectoryIterator entries are wchar_t, but we are working with char8_t. So we need to do a translation copy.
            sFilePath     = EA::StdC::Strlcpy<String, EA::IO::DirectoryIterator::EntryString>(entry.msName);
            sFilePathTest = sFilePath;

            if(pAppendedExtensionList)
            {
                // We run this in a loop like this because we need to be able to strip multiple present extensions, 
                // such as with a file that has the name Game.big.eaPatchDiff.eaPatchTemp where we want to strip both   
                // .eaPatchTemp and .eaPatchDiff.
                bool bChangeOccurred = false;

                do{
                    bChangeOccurred = false;

                    const char8_t* pFileExtension = EA::IO::Path::GetFileExtension(sFilePathTest.c_str());

                    for(StringList::const_iterator itStr = pAppendedExtensionList->begin(); itStr != pAppendedExtensionList->end(); ++itStr)
                    {
                        const String& sExtension = *itStr;

                        if(sExtension.comparei(pFileExtension) == 0)
                        {
                            sFilePathTest.resize(sFilePathTest.size() - sExtension.length());
                            bChangeOccurred = true;
                        }
                    }
                } while(bChangeOccurred);
            }

            // Test this entry against the user-specified filters. The filters are specified in relative paths, 
            // so we need to get the relative path of the entry and pass that to PathMatchesFileFilterArray below.
            const String sFilePathRelative(sFilePath.c_str() + EA::StdC::Strlen(pRootDirectoryPath));
            const String sFilePathRelativeTest(sFilePathTest.c_str() + EA::StdC::Strlen(pRootDirectoryPath));

            const MatchType mtIgnored  = ignoredFileArray.empty() ? kMTEmpty : (PathMatchesFileFilterArray(sFilePathRelativeTest.c_str(), ignoredFileArray, false) ? kMTMatch : kMTUnmatched);
            const MatchType mtUsed     = usedFileArray.empty()    ? kMTEmpty : (PathMatchesFileFilterArray(sFilePathRelativeTest.c_str(), usedFileArray,    false) ? kMTMatch : kMTUnmatched);
            const bool      bEntryUsed = truthTable[mtUsed][mtIgnored];

            if(bEntryUsed)
            {
                if(pUsedFileList) // If the user wants us to list used files...
                    pFileInfo = &pUsedFileList->push_back();
            }
            else
            {
                if(pIgnoredFileList) // If the user wants us to list ignored files...
                    pFileInfo = &pIgnoredFileList->push_back();
            }

            if(pFileInfo) // If we are to add this entry to a user-provided list...
            {
                pFileInfo->mbFile = (entry.mType == EA::IO::kDirectoryEntryFile);
                pFileInfo->mSize  = entry.mSize;  // If bReadFileSize was true then mSize will be valid here.

                if(bReturnFullPaths)
                    pFileInfo->mFilePath = sFilePath;
                else
                    pFileInfo->mFilePath = sFilePathRelative;

            }
        }
    }

    return bSuccess;
}



EAPATCHCLIENT_API uint32_t StringToFnMatchFlags(const char8_t* pMatchFlagsString)
{
    uint32_t flags = 0;

    if(EA::StdC::Stristr(pMatchFlagsString, "Pathname"))
        flags |= EA::IO::kFNMPathname;
    if(EA::StdC::Stristr(pMatchFlagsString, "Escape"))
        flags |= EA::IO::kFNMNoEscape;
    if(EA::StdC::Stristr(pMatchFlagsString, "Period"))
        flags |= EA::IO::kFNMPeriod;
    if(EA::StdC::Stristr(pMatchFlagsString, "LeadingDir"))
        flags |= EA::IO::kFNMLeadingDir;
    if(EA::StdC::Stristr(pMatchFlagsString, "PrefixDir"))
        flags |= EA::IO::kFNMPrefixDir;
    if(EA::StdC::Stristr(pMatchFlagsString, "CaseFold"))
        flags |= EA::IO::kFNMCaseFold;
    if(EA::StdC::Stristr(pMatchFlagsString, "DosPath"))
        flags |= EA::IO::kFNMDosPath;
    if(EA::StdC::Stristr(pMatchFlagsString, "UnixPath"))
        flags |= EA::IO::kFNMUnixPath;

    return flags;
}


EAPATCHCLIENT_API String FnMatchFlagsToString(uint32_t matchFlags)
{
    String sMatchFlags;

    if(matchFlags & EA::IO::kFNMPathname)
        sMatchFlags += "Pathname ";
    if(matchFlags & EA::IO::kFNMNoEscape)
        sMatchFlags += "NoEscape ";
    if(matchFlags & EA::IO::kFNMPeriod)
        sMatchFlags += "Period ";
    if(matchFlags & EA::IO::kFNMLeadingDir)
        sMatchFlags += "LeadingDir ";
    if(matchFlags & EA::IO::kFNMPrefixDir)
        sMatchFlags += "PrefixDir ";
    if(matchFlags & EA::IO::kFNMCaseFold)
        sMatchFlags += "CaseFold ";
    if(matchFlags & EA::IO::kFNMDosPath)
        sMatchFlags += "DosPath ";
    if(matchFlags & EA::IO::kFNMUnixPath)
        sMatchFlags += "UnixPath ";

    if(!sMatchFlags.empty())
        sMatchFlags.pop_back();

    return sMatchFlags;
}


EAPATCHCLIENT_API bool PathMatchesFileFilter(const char8_t* pPath, const FileFilter& fileFilter)
{
    // See FnMatch's documentation to see how it behaves in the presence of an empty pPath or fileFilter.
    // To consider: Do something special if the path or filter spec is empty.

    return EA::IO::FnMatch(fileFilter.mFilterSpec.c_str(), pPath, (int)fileFilter.mMatchFlags);
}


EAPATCHCLIENT_API bool PathMatchesFileFilterArray(const char8_t* pPath, const FileFilterArray& fileFilterArray, bool bMatchEmptyFilter)
{
    for(eastl_size_t i = 0, iEnd = fileFilterArray.size(); i != iEnd; i++)
    {
        const FileFilter& fileFilter = fileFilterArray[i];

        if(EA::IO::FnMatch(fileFilter.mFilterSpec.c_str(), pPath, (int)fileFilter.mMatchFlags))
            return true;
    }

    return bMatchEmptyFilter;
}


static const char8_t* kInternalFileNameExtensionArray[] = 
{
    kPatchDirectoryFileNameExtension,
    kPatchInfoFileNameExtension,
    kPatchImplFileNameExtension,
    kPatchDiffFileNameExtension,
    kPatchBFIFileNameExtension,
    kPatchTempFileNameExtension,
    kPatchStateFileNameExtension,
    kPatchOldFileNameExtension,
    kPatchInstalledFileNameExtension
};
EA_COMPILETIME_ASSERT(kEAPatchFileTypeCount == 9);


EAPATCHCLIENT_API bool PathMatchesAnInternalFileType(const char8_t* pPath)
{
    const char8_t* pFileNameExtension = EA::IO::Path::GetFileExtension(pPath);

    if(*pFileNameExtension) // If the file name has any extension...
    {
        for(size_t i = 0; i < EAArrayCount(kInternalFileNameExtensionArray); i++)
        {
            if(EA::StdC::Stricmp(pFileNameExtension, kInternalFileNameExtensionArray[i]) == 0)
                return true;
        }
    }

    return false;
}



/////////////////////////////////////////////////////////////////////////////
// Directory
/////////////////////////////////////////////////////////////////////////////

EAPATCHCLIENT_API bool DirectoryIsEmpty(const char8_t* pDirectoryPath)
{
    size_t entryCount = 0;

    if(EA::IO::Directory::Exists(pDirectoryPath))
    {
        EA::IO::DirectoryIterator dir;
        EA::IO::DirectoryIterator::EntryList entryList;

        wchar_t directoryW[EA::IO::kMaxPathLength]; // For EAIO_VERSION_N >= 21600 we don't to do this directory path copying.
        EA::StdC::Strlcpy(directoryW, pDirectoryPath, EAArrayCount(directoryW));

        entryCount = dir.ReadRecursive(directoryW, entryList, NULL, EA::IO::kDirectoryEntryFile | EA::IO::kDirectoryEntryDirectory, true, true, 1, false);
    }

    return (entryCount == 0);
}


EAPATCHCLIENT_API bool IsFilePathValid(const String& sPath)
{
    #if defined(EA_PLATFORM_MICROSOFT) // Microsoft has some file path character limitations.
        // To do: We have a much more elaborate version of this function available 
        // in EAIO/compat/EAFilePath.h's IsPathValid() function. But that's part of deprecated
        // code of which the IsPathValid function itself wasn't intended to be deprecated.
        
        if(sPath.find_first_of("?*<>|\"") != String::npos)
            return false;

        if((sPath.length() >= 2) && (sPath.find(":", 2) != String::npos))
            return false;
    #else
        // Unix platforms typically allow any characters.
        // Console platforms sometimes have character limitations which we might want to catch here.
        EA_UNUSED(sPath);
    #endif

    return true;
}


EAPATCHCLIENT_API bool FilePatternExists(const char8_t* pFilePathPattern, String* pFirstFilePathResult)
{
    size_t entryCount = 0;

    if(pFilePathPattern)
    {
        const char8_t* pFileNamePattern(EA::IO::Path::GetFileName(pFilePathPattern));
        const String   sFileDirectory(pFilePathPattern, (eastl_size_t)(pFileNamePattern - pFilePathPattern));

        return FilePatternExists(sFileDirectory.c_str(), pFileNamePattern, pFirstFilePathResult);
    }

    return (entryCount != 0);
}


EAPATCHCLIENT_API bool FilePatternExists(const char8_t* pDirectoryPath, const char8_t* pFileNamePattern, String* pFirstFilePathResult)
{
    using namespace EA::IO;

    size_t entryCount = 0;

    if(pDirectoryPath && pFileNamePattern)
    {
        DirectoryIterator dir;
        DirectoryIterator::EntryList entryList;

        // Need to convert pDirectoryPath and pFileNamePattern from char8_t to wchar_t.
        Path::PathStringW sDirectoryPathW(EA::StdC::Strlcpy<Path::PathStringW, char8_t>(pDirectoryPath));
        Path::PathStringW sFileNamePatternW(EA::StdC::Strlcpy<Path::PathStringW, char8_t>(pFileNamePattern));

        // Do a directory iteration, finding at most 1 file.
        entryCount = dir.Read(sDirectoryPathW.c_str(), entryList, sFileNamePatternW.c_str(), EA::IO::kDirectoryEntryFile, 1, false);

        if(entryCount && pFirstFilePathResult)
        {
            const EA::IO::DirectoryIterator::Entry& entry = entryList.front();
            pFirstFilePathResult->assign(pDirectoryPath);

            const String sEntryName8(EA::StdC::Strlcpy<EA::Patch::String, wchar_t>(entry.msName.c_str())); // Need to convert from wchar_t to char8_t.
            pFirstFilePathResult->append(sEntryName8.c_str());
        }
    }

    return (entryCount != 0);
}


EAPATCHCLIENT_API String& StripFileNameFromFilePath(String& sFilePath)
{
    // e.g. sFilePath = Basebrawl_2014/Patches/Patch1.eaPatchImpl
    //   -> sFilePath = Basebrawl_2014/Patches/

    // The call to GetFileName assumes that PathString8::iterator == char8_t*. 
    // That's not strictly guaranteed, but it's been so for years, with no change in sight.
    const char8_t* pFileName = EA::IO::Path::GetFileName(sFilePath.c_str());
    sFilePath.erase((eastl_size_t)(pFileName - sFilePath.begin())); // pFileName may be at the end of sFilePath, which makes this a no-op.

    return sFilePath;
}


EAPATCHCLIENT_API String& StripFileNameExtensionFromFilePath(String& sFilePath)
{
    // e.g. sFilePath = Basebrawl_2014/Patches/Patch1.eaPatchImpl
    //   -> sFilePath = Basebrawl_2014/Patches/Patch1.

    // The call to GetFileExtension assumes that PathString8::iterator == char8_t*. 
    // That's not strictly guaranteed, but it's been so for years, with no change in sight.
    const char8_t* pExtension = EA::IO::Path::GetFileExtension(sFilePath.c_str());
    sFilePath.erase((eastl_size_t)(pExtension - sFilePath.begin())); // pExtension may be at the end of sFilePath, which makes this a no-op.

    return sFilePath;
}


EAPATCHCLIENT_API String& CanonicalizeFilePath(String& sFilePath, char8_t separator)
{
    // We canonicalize paths by converting to and from EAIO PathStrings. This is a little 
    // dumb, but EAIO doesn't provide a way to do it in place on a path char array.
    EA::IO::Path::PathString8 sTemp(sFilePath.data(), sFilePath.length());
    EA::IO::Path::Canonicalize(sTemp, separator);
    sFilePath.assign(sTemp.data(), sTemp.length());

    return sFilePath;
}


EAPATCHCLIENT_API uint64_t EstimateFileDiskSpaceUsage(uint64_t fileSize, const StringArray& /*osNameArray*/)
{
    // To do: Use the osNameArray to provide a better estimate, if needed.
    const size_t   clusterSize   = 65536;
    const uint64_t usageEstimate = EA::StdC::AlignUp(fileSize ? fileSize : 1, clusterSize);

    return usageEstimate;
}



/////////////////////////////////////////////////////////////////////////////
// GetPlatformMaxFileNameLength, etc.
/////////////////////////////////////////////////////////////////////////////

EAPATCHCLIENT_API size_t GetPlatformMaxFileNameLength(const char8_t* pPlatformName)
{
    using namespace EA::StdC;

    if(Stricmp(pPlatformName, EAPATCH_OS_WIN32_STR))
        return 260;
    if(Stricmp(pPlatformName, EAPATCH_OS_WIN64_STR))
        return 260;
    if(Stricmp(pPlatformName, EAPATCH_OS_XBOX_360_STR))
        return 40;
    if(Stricmp(pPlatformName, EAPATCH_OS_PS3_STR))
        return 512;
    if(Stricmp(pPlatformName, EAPATCH_OS_WII_STR))
        return 256;

    EAPATCH_FAIL_MESSAGE("GetPlatformMaxFileNameLength: Unknown platform.");
    return 0;
}


EAPATCHCLIENT_API size_t GetPlatformMaxFileNameLength(const StringArray& osNameArray)
{
    size_t minValue = (size_t)-1; // By default there is no limit.

    for(eastl_size_t i = 0, iEnd = osNameArray.size(); i != iEnd; i++)
    {
        const String& sOSName = osNameArray[i];
        size_t max = GetPlatformMaxFileNameLength(sOSName.c_str());

        if(max < minValue)
            minValue = max;
    }

    return minValue;
}

EAPATCHCLIENT_API size_t GetPlatformMaxPathLength(const char8_t* pPlatformName)
{
    using namespace EA::StdC;

    if(Stricmp(pPlatformName, EAPATCH_OS_WIN32_STR))
        return 260;
    if(Stricmp(pPlatformName, EAPATCH_OS_WIN64_STR))
        return 260;
    if(Stricmp(pPlatformName, EAPATCH_OS_XBOX_360_STR))
        return 260;
    if(Stricmp(pPlatformName, EAPATCH_OS_PS3_STR))
        return 512;
    if(Stricmp(pPlatformName, EAPATCH_OS_WII_STR))
        return 256;

    EAPATCH_FAIL_MESSAGE("GetPlatformMaxPathLength: Unknown platform.");
    return 0;
}

EAPATCHCLIENT_API size_t GetPlatformMaxPathLength(const StringArray& osNameArray)
{
    size_t minValue = (size_t)-1; // By default there is no limit.

    for(eastl_size_t i = 0, iEnd = osNameArray.size(); i != iEnd; i++)
    {
        const String& sOSName = osNameArray[i];
        size_t max = GetPlatformMaxPathLength(sOSName.c_str());

        if(max < minValue)
            minValue = max;
    }

    return minValue;
}


bool CopyDirectoryTree(const char8_t* pDirectoryPathSource, const char8_t* pDirectoryPathDest, bool clearInitialDest, Error& error)
{
    bool bSuccess = true;

    if(clearInitialDest && EA::IO::Directory::Exists(pDirectoryPathDest))
    {
        if(!EA::IO::Directory::Remove(pDirectoryPathDest, true))
        {
            error.SetErrorId(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdDirRemoveError, pDirectoryPathDest);
            bSuccess = false;
        }
    }

    if(bSuccess)
    {
        wchar_t directoryW[EA::IO::kMaxPathLength]; // For EAIO_VERSION_N >= 21600 we don't to do this directory path copying.
        EA::StdC::Strlcpy(directoryW, pDirectoryPathSource, EAArrayCount(directoryW));

        EA::IO::DirectoryIterator dir;
        EA::IO::DirectoryIterator::EntryList entryList;
        size_t sourceDirLength = EA::StdC::Strlen(pDirectoryPathSource);
        size_t entryCount      = dir.ReadRecursive(directoryW, entryList, NULL, EA::IO::kDirectoryEntryFile | EA::IO::kDirectoryEntryDirectory, true, true, 1000000, false);

        if(entryCount != EA::IO::kSizeTypeError)
        {
            String sSourcePath8;
            String sDestPath8;

            for(EA::IO::DirectoryIterator::EntryList::const_iterator it = entryList.begin(), itEnd = entryList.end(); (it != itEnd) && bSuccess; ++it)
            {
                const EA::IO::DirectoryIterator::Entry& entry = *it;

                sSourcePath8 = (EA::StdC::Strlcpy<EA::Patch::String, wchar_t>(entry.msName.c_str())); // Need to convert from wchar_t to char8_t.
                sDestPath8 = pDirectoryPathDest;
                sDestPath8.append(sSourcePath8.c_str() + sourceDirLength);

                if(entry.mType == EA::IO::kDirectoryEntryFile) // If we need to create a file as opposed to a directory...
                {
                    FileCopier fileCopier;

                    fileCopier.CopyFile(sSourcePath8.c_str(), sDestPath8.c_str()); // This will create the directory for the file if needed.

                    if(fileCopier.HasError())
                    {
                        error = fileCopier.GetError();
                        bSuccess = false;
                    }
                }
                else if(entry.mType == EA::IO::kDirectoryEntryDirectory) // If we need to create a directory as opposed to a file...
                {
                    if(!EA::IO::Directory::Exists(sDestPath8.c_str()))
                    {
                        if(!EA::IO::Directory::Create(sDestPath8.c_str())) // Directory::Create will create the directory tree as needed.
                        {
                            error.SetErrorId(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdDirCreateError, sDestPath8.c_str());
                            bSuccess = false;
                        }
                    }
                }
            }
        }
        else
        {
            error.SetErrorId(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdDirReadError, pDirectoryPathSource);
            bSuccess = false;
        }
    }

    return bSuccess;
}


struct EntryListCompare
{
    bool operator()(const EA::IO::DirectoryIterator::Entry& a, const EA::IO::DirectoryIterator::Entry& b)
        { return a.msName < b.msName; }
};

 
bool CompareDirectoryTree(const char8_t* pDirectoryPathSource, const char8_t* pDirectoryPathDest, bool& bEqual, 
                            String& sEqualFailureDescription, bool bEnableDescriptionDetail, Error& error)
{
    bool bSuccess = true;

    bEqual = true;
    sEqualFailureDescription.clear();

    EA::IO::DirectoryIterator dir;

    if(bSuccess)
    {
        wchar_t directoryW[EA::IO::kMaxPathLength]; // For EAIO_VERSION_N >= 21600 we don't to do this directory path copying.
        EA::StdC::Strlcpy(directoryW, pDirectoryPathSource, EAArrayCount(directoryW));

        EA::IO::DirectoryIterator::EntryList sourceEntryList;
        size_t sourceEntryCount = dir.ReadRecursive(directoryW, sourceEntryList, NULL, EA::IO::kDirectoryEntryFile | EA::IO::kDirectoryEntryDirectory, true, true, 1000000, false);

        if(sourceEntryCount == EA::IO::kSizeTypeError)
        {
            error.SetErrorId(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdDirReadError, pDirectoryPathSource);
            bSuccess = false;
        }

        if(bSuccess)
        {
            EA::StdC::Strlcpy(directoryW, pDirectoryPathDest, EAArrayCount(directoryW));

            EA::IO::DirectoryIterator::EntryList destEntryList;
            size_t destEntryCount = dir.ReadRecursive(directoryW, destEntryList, NULL, EA::IO::kDirectoryEntryFile | EA::IO::kDirectoryEntryDirectory, true, true, 1000000, false);

            if(destEntryCount == EA::IO::kSizeTypeError)
            {
                error.SetErrorId(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdDirReadError, pDirectoryPathDest);
                bSuccess = false;
            }

            if(bSuccess)
            {
                if(sourceEntryCount == destEntryCount) // We expect that identical directories have equal entry counts.
                {
                    // We need to compare every file and directory in sourceEntryList to destEntryList. 
                    // But the two lists aren't necessarily in identical order. The simplest solution for
                    // us may be to sort both lists alphabetically and then compare them entry-by-entry.
                    // The eastl::list class, of which EntryList is an instance, has a sort member function.
                    // However, it's a recursive heap sort which cannot handle arbitrarily large-sized 
                    // containers. So we instead for large lists we copy our entries to two arrays and sort them.

                    sourceEntryList.sort(EntryListCompare());
                    destEntryList.sort(EntryListCompare());

                    for(EA::IO::DirectoryIterator::EntryList::const_iterator itSource    = sourceEntryList.begin(), 
                                                                             itSourceEnd = sourceEntryList.end(), 
                                                                             itDest      = destEntryList.begin(); 
                        (itSource != itSourceEnd) && bSuccess && bEqual; 
                        ++itSource, ++itDest)
                    {
                        const EA::IO::DirectoryIterator::Entry& sourceEntry = *itSource;
                        const EA::IO::DirectoryIterator::Entry& destEntry   = *itDest;

                        const wchar_t* pSourceEntryFileName = EA::IO::Path::GetFileName(sourceEntry.msName.c_str());
                        const wchar_t* pDestEntryFileName   = EA::IO::Path::GetFileName(destEntry.msName.c_str());

                        if((EA::StdC::Strcmp(pSourceEntryFileName, pDestEntryFileName) == 0)) // Case-sensitive compare
                        {
                            if(*pSourceEntryFileName) // If a file and not a directory...
                            {
                                // To do: Compare file contents.
                                // If the file has an extension of .fakeValidationFile then we don't compare the file contents,
                                // as fake ValidationFiles are special files we use which are often not backed with actual contents.
                                // First open the files and make sure they are the same size. Then read the files in chunks and 
                                // compare the chunks.
                                File     sourceFile;
                                String   sSourcePath8 = EA::StdC::Strlcpy<EA::Patch::String, wchar_t>(sourceEntry.msName.c_str());

                                File     destFile;
                                String   sDestPath8 = EA::StdC::Strlcpy<EA::Patch::String, wchar_t>(destEntry.msName.c_str());

                                if(OpenTwoFiles(sourceFile, sSourcePath8.c_str(), EA::IO::kAccessFlagRead, EA::IO::kCDOpenExisting, EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential, 
                                                destFile, sDestPath8.c_str(), EA::IO::kAccessFlagRead, EA::IO::kCDOpenExisting, EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential,
                                                error))
                                {
                                    char     sourceBuffer[65536];
                                    char     destBuffer[65536];
                                    uint64_t sourceReadSize;
                                    uint64_t destReadSize;
                                    uint64_t readPosition = 0;

                                    do
                                    {
                                        if(sourceFile.Read(sourceBuffer, sizeof(sourceBuffer), sourceReadSize, false))
                                        {
                                            if(destFile.Read(destBuffer, sizeof(destBuffer), destReadSize, false))
                                            {
                                                if((sourceReadSize != destReadSize) || EA::StdC::Memcmp(sourceBuffer, destBuffer, (size_t)sourceReadSize))
                                                {
                                                    bEqual = false;

                                                    // Find the range of mismatches in the buffer.
                                                    uint64_t mismatchBegin;
                                                    uint64_t mismatchEnd;

                                                    for(mismatchBegin = 0, mismatchEnd = sourceReadSize; mismatchBegin < sourceReadSize; mismatchBegin++)
                                                    {
                                                        if(((uint8_t*)sourceBuffer)[mismatchBegin] != ((uint8_t*)destBuffer)[mismatchBegin])
                                                        {
                                                            for(mismatchEnd = mismatchBegin; mismatchBegin < sourceReadSize; mismatchBegin++)
                                                            {
                                                                if(((uint8_t*)sourceBuffer)[mismatchBegin] == ((uint8_t*)destBuffer)[mismatchBegin])
                                                                    break;
                                                            }
                                                            break;
                                                        }
                                                    }

                                                    sEqualFailureDescription.sprintf("File contents don't match:\n      %s\n      %s\n      at position: %I64u - %I64u", sSourcePath8.c_str(), sDestPath8.c_str(), readPosition + mismatchBegin, readPosition + mismatchEnd);
                                                }
                                            }
                                            else
                                            {
                                                error = destFile.GetError();
                                                bSuccess = false;
                                            }

                                            readPosition += sourceReadSize;
                                        }
                                        else
                                        {
                                            error = sourceFile.GetError();
                                            bSuccess = false;
                                        }
                                    }while(bSuccess && bEqual && sourceReadSize);

                                    sourceFile.Close();
                                    destFile.Close();
                                }
                                else
                                    bSuccess = false;

                                if(sourceFile.HasError())
                                {
                                    error = sourceFile.GetError();
                                    bSuccess = false;
                                }

                                if(destFile.HasError())
                                {
                                    error = destFile.GetError();
                                    bSuccess = false;
                                }
                            } // Else it's a directory and we don't compare its entry itself (though later we compare the files within it).
                        }
                        else
                        {
                            bEqual = false;
                            sEqualFailureDescription.sprintf("Directories have different entries:\n      %s\n      %s", pSourceEntryFileName, pDestEntryFileName);
                        }
                    }
                }
                else
                {
                    bEqual = false;
                    sEqualFailureDescription.sprintf("Directories have different entry counts:\n      %s (%u)\n      %s (%u)", pDirectoryPathSource, (unsigned)sourceEntryCount, pDirectoryPathDest, (unsigned)destEntryCount);

                    if(bEnableDescriptionDetail)
                    {
                        sEqualFailureDescription.append_sprintf("\nSource entry list:\n");
                        for(EA::IO::DirectoryIterator::EntryList::iterator it = sourceEntryList.begin(); it != sourceEntryList.end(); ++it)
                            sEqualFailureDescription.append_sprintf("    %ls\n", it->msName.c_str());

                        sEqualFailureDescription.append_sprintf("\nDest entry list:\n");
                        for(EA::IO::DirectoryIterator::EntryList::iterator it = destEntryList.begin(); it != destEntryList.end(); ++it)
                            sEqualFailureDescription.append_sprintf("    %ls\n", it->msName.c_str());
                    }
                }
            }
        }
    }

    if(!bSuccess)
        bEqual = false;

    return bSuccess;
}


bool CreateSizedFile(const char8_t* pFilePath, uint64_t fileSize, Error& error)
{
    bool bSuccess = true;

    // Create the directory path if not already present.
    String sDirectoryPath(pFilePath);
    StripFileNameFromFilePath(sDirectoryPath);

    if(!EA::IO::Directory::Exists(sDirectoryPath.c_str()))
    {
        if(!EA::IO::Directory::Create(sDirectoryPath.c_str())) // Directory::Create will create the directory tree as needed.
        {
            error.SetErrorId(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdDirCreateError, sDirectoryPath.c_str());
            bSuccess = false;
        }
    }

    if(bSuccess)
    {
        File file;

        if(file.Open(pFilePath, EA::IO::kAccessFlagWrite, EA::IO::kCDCreateNew, EA::IO::FileStream::kShareRead, 0, 0, 0))
        {
            if(fileSize) // If there is anything to write below...
            {
                if(file.SetPosition((int64_t)fileSize - 1, EA::IO::kPositionTypeBegin))
                {
                    // Some file systems won't implement a file resize until you write something at the file extent.
                    uint8_t byte(0);
                    file.Write(&byte, 1);
                }
            }

            file.Close();
        }

        if(file.HasError())
        {
            error = file.GetError();
            bSuccess = false;
        }
    }

    return bSuccess;
}


// Some streams are just wrappers for other streams. An example is a stream buffer which implements
// buffering around a regular file stream. This can theoretically involve multiple layers, though 
// the majority of the time it's just a single layer of wrapping (stream buffer over file stream or
// stream child over file stream), though it's conceivable that there's a stream buffer over a stream
// child over a file stream. This function takes the high level stream and finds the file stream that
// it's wrapping and returns that.
EA::IO::IStream* GetRealStream(EA::IO::IStream* pStream)
{
    if(pStream) 
    {
        uint32_t streamType = pStream->GetType();

        // If it's a child stream or stream buffer, walk up to its parent. Usually there is but one parent.
        // Currently we recognize only child streams and buffer stream wrappers, though they constitute
        // the large majority of stream wrapper types and for EAPatch are the only types of stream wrapper types.
        while(pStream && ((streamType == EA::IO::StreamChild::kTypeStreamChild) || 
                          (streamType == EA::IO::StreamBuffer::kTypeStreamBuffer)))
        {
            if(streamType == EA::IO::StreamChild::kTypeStreamChild)
                pStream = static_cast<EA::IO::StreamChild*>(pStream)->GetStream();
            else
                pStream = static_cast<EA::IO::StreamBuffer*>(pStream)->GetStream();

            streamType = pStream->GetType();
        }
    }

    return pStream;
}


uint32_t GetStreamInfo(EA::IO::IStream* pStream, uint32_t& state, String& sName)
{
    uint32_t streamType = 0;

    // Get the "real" stream associated with pStream, as original 
    // pStream might have been a wrapping StreamBuffer or wrapped StreamChild.
    pStream = GetRealStream(pStream);

    if(pStream) 
    {
        streamType = (uint32_t)pStream->GetType();
        state      = (uint32_t)pStream->GetState();

        if(streamType == EA::IO::FileStream::kTypeFileStream)
        {
            EA::IO::FileStream* pFileStream = static_cast<EA::IO::FileStream*>(pStream);

            char8_t filePath[EA::IO::kMaxPathLength];
            pFileStream->GetPath(filePath, EAArrayCount(filePath));
            sName = filePath;
        }
        else
            sName.sprintf("IStream type 0x%08x", (unsigned)streamType); // To do: Come up with a better solution. There are also MemoryStreams, StreamBuffers, etc.
    }

    if(!pStream)
    {
        state = 0;
        sName = "NULL IStream";
    }

    return streamType;
}


bool WriteStreamToFile(EA::IO::IStream* pSourceStream, const char8_t* pDestFilePath, Error& error)
{
    bool bSuccess = true;
    File file;

    if(!file.Open(pDestFilePath, EA::IO::kAccessFlagReadWrite, EA::IO::kCDCreateAlways, 
                  EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential))
    {
        error = file.GetError();
        file.ClearError();
        return false;
    }

    // Copy the data from pSourceStream to file.
    EA::IO::size_type result = EA::IO::CopyStream(pSourceStream, file.GetStream());

    if(result == EA::IO::kSizeTypeError)
    {
        error.SetErrorId(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileWriteFailure, pDestFilePath);
        bSuccess = false;
    }

    file.Close();

    if(file.HasError()) // If the Close failed or it was somehow else in an error state from before...
    {
        if(bSuccess)   // If the CopyStream operation was otherwise successful... Transfer the error from the file. Otherwise let's use the CopyStream operation error take precedence.
            error = file.GetError();
    }

    return bSuccess;
}


bool GetFileHash(const char8_t* pFilePath, HashValueMD5& hashValue, Error& error)
{
    bool bSuccess = false;
    File file;

    if(file.Open(pFilePath, EA::IO::kAccessFlagRead, EA::IO::kCDOpenExisting, 
                  EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential))
    {
        bSuccess = GetFileHash(file.GetStream(), hashValue, error);
    }

    file.Close();

    if(file.HasError()) // If the Close failed or it was somehow else in an error state from before...
    {
        if(bSuccess) // If GetFileHash was successful... transfer the error from the file. Otherwise let's use the GetFileHash operation error take precedence.
            error = file.GetError();
    }

    return bSuccess;
}


bool GetFileHash(EA::IO::IStream* pSourceStream, HashValueMD5& fileHash, Error& error)
{
    bool           bSuccess = false;
    bool           bDoneReadingFile = false;
    uint64_t       readSizeResult = 0;
    uint64_t       fileSize = 0;
    const size_t   kBufferSize = 8192;
    uint8_t        buffer[kBufferSize];     // Is kBufferSize too big for us to create a buffer with?
    EA::Patch::MD5 md5;

    md5.Begin();

    while(bSuccess && !bDoneReadingFile)
    {
        readSizeResult = pSourceStream->Read(&buffer[0], kBufferSize);

        if(readSizeResult == EA::IO::kSizeTypeError)
        {
            String   sName;
            uint32_t state;

            EA::Patch::GetStreamInfo(pSourceStream, state, sName);
            error.SetErrorId(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileReadFailure, sName.c_str());
            bSuccess = false;
        }
        else
        {
            EAPATCH_ASSERT(readSizeResult <= kBufferSize);

            fileSize += readSizeResult;
            md5.AddData(&buffer[0], (size_t)readSizeResult);

            if(readSizeResult < kBufferSize) // If we have reached the end of the file/stream...
                bDoneReadingFile = true;
        }
    }

    if(bSuccess)
        md5.End(fileHash);

    return bSuccess;
}



/////////////////////////////////////////////////////////////////////////////
// FileCopier
/////////////////////////////////////////////////////////////////////////////

FileCopier::FileCopier()
  : mBuffer()
  , mFileSize()
  , mFileHash()
  , mDiffHashArray()
  , mDestRelativePath()
  , mDestFullPath()
{
    mBuffer.resize(kBufferSize);
    #if EAPATCH_DEBUG
        EA::Patch::RandomizeMemory(&mBuffer[0], kBufferSize);
    #endif
}


bool FileCopier::CopyFile(const char8_t* pFileSourceFullPath, const char8_t* pFileDestFullPath)
{
    return CopyFile(pFileSourceFullPath, pFileDestFullPath, false, false, kDiffBlockSizeDefault);
}


bool FileCopier::CopyFile(const char8_t* pFileSourceFullPath, const char8_t* pFileDestFullPath, 
                          bool generateFileHash, bool generateDiffHash, size_t diffBlockSize)
{
    mFileSize = 0;
    mFileHash.Clear();
    mDiffHashArray.clear();

    // Assert that the user has handled any previous error state and called ClearError when done.
    EAPATCH_ASSERT(!HasError());
    EAPATCH_ASSERT((diffBlockSize > 0) && (diffBlockSize <= kBufferSize) && EA::StdC::IsPowerOf2(diffBlockSize));

    EA::Patch::File fileSource;
    EA::Patch::File fileDest;

    if(OpenTwoFiles(fileSource, pFileSourceFullPath, EA::IO::kAccessFlagRead, EA::IO::kCDOpenExisting, 
                                EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential, 
                    fileDest, pFileDestFullPath, EA::IO::kAccessFlagWrite, EA::IO::kCDCreateAlways, 
                              EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential,
                    mError))
    {
        bool                 bDoneReadingFile = false;
        EA::Patch::BlockHash blockHash;
        EA::Patch::MD5       md5;
        uint64_t             readSizeResult = 0;
        size_t               i;

        if(generateFileHash)
            md5.Begin();

        while(!fileSource.HasError() && !fileDest.HasError() && !bDoneReadingFile)
        {
            if(fileSource.Read(&mBuffer[0], kBufferSize, readSizeResult, false))
            {
                EAPATCH_ASSERT(readSizeResult <= kBufferSize);

                mFileSize += readSizeResult;
                uint64_t readSizeAlignedUp = EA::StdC::AlignUp(readSizeResult, diffBlockSize);

                if(readSizeResult < kBufferSize) // If the file ended on boundary != kBufferFileSize... fill in the rest of the buffer as necessary with 0.
                {
                    EA::StdC::Memset8(&mBuffer[0] + readSizeResult, 0, (size_t)(readSizeAlignedUp - readSizeResult));
                    bDoneReadingFile = true;
                }

                if(generateFileHash)
                    md5.AddData(&mBuffer[0], (size_t)readSizeResult);

                if(generateDiffHash)
                {                    
                    // We are guaranteed that kBufferSize is a multiple of diffBlockSize. So we can just do a simple loop.
                    for(i = 0; i < readSizeAlignedUp; i += diffBlockSize)
                    {
                        // Calculate mChecksum for the block.
                        blockHash.mChecksum = EA::Patch::GetRollingChecksum(&mBuffer[0] + i, diffBlockSize);

                        // Calculate mHashValue for the block.
                        HashValueMD5 hashMD5;
                        MD5::GetHashValue(&mBuffer[0] + i, diffBlockSize, hashMD5);
                        GenerateBriefHash(hashMD5, blockHash.mHashValue);

                        mDiffHashArray.push_back(blockHash);
                    }

                    EAPATCH_ASSERT(EA::StdC::IsAligned(i, diffBlockSize));
                }

                fileDest.Write(&mBuffer[0], readSizeResult);
            }
        }

        fileSource.Close();    
        fileDest.Close();    

        if(generateFileHash)
            md5.End(mFileHash);

        // Move the error state of fileSource or fileDest to us.
        if(fileSource.HasError())
            TransferError(fileSource);
        else if(fileDest.HasError())
            TransferError(fileDest);
    }
    else // else OpenTwoFiles will have set mError.
        HandleError(mError); // This will set mbSuccess to false.

    return mbSuccess;
}


bool FileCopier::CopyFile(const char8_t* pFileSourceFullPath, const char8_t* pFileSourceBaseDirectry, 
                          const char8_t* pFileDestBaseDirectory, bool generateFileHash, bool generateDiffHash, size_t diffBlockSize)
{
    EAPATCH_ASSERT(EA::IO::Path::GetHasTrailingSeparator(pFileSourceBaseDirectry)); // If this fails then our code has a bug, but possibly the user mis-specified a directory by neglecting to put a path separator at the end and we didn't correct it.
    EAPATCH_ASSERT(EA::IO::Path::GetHasTrailingSeparator(pFileDestBaseDirectory));

    // Calculate mDestRelativePath and sFileDestFullPath;
    mDestRelativePath = pFileSourceFullPath + EA::StdC::Strlen(pFileSourceBaseDirectry);
    mDestFullPath     = pFileDestBaseDirectory;
    mDestFullPath    += mDestRelativePath;

    return CopyFile(pFileSourceFullPath, mDestFullPath.c_str(), generateFileHash, generateDiffHash, diffBlockSize);
}


void FileCopier::GetFileSize(uint64_t& fileSize) const
{
    fileSize = mFileSize;
}


void FileCopier::GetFileHash(HashValueMD5& fileHash) const
{
    fileHash = mFileHash;
}


void FileCopier::GetDiffHash(BlockHashArray& diffHashArray) const
{
    diffHashArray = mDiffHashArray;
}


void FileCopier::GetDestPath(String& destRelativePath, String& destFullPath) const
{
    destRelativePath = mDestRelativePath;
    destFullPath     = mDestFullPath;
}




/////////////////////////////////////////////////////////////////////////////
// FileReader
/////////////////////////////////////////////////////////////////////////////

FileReader::FileReader()
  : mBuffer()
  , mFileSize()
  , mFileHash()
  , mDiffHashArray()
{
    mBuffer.resize(kBufferSize);
    #if EAPATCH_DEBUG
        EA::Patch::RandomizeMemory(&mBuffer[0], kBufferSize);
    #endif
}


bool FileReader::ReadFile(const char8_t* pFileSourceFullPath, bool generateFileHash, bool generateDiffHash, size_t diffBlockSize)
{
    mFileSize = 0;
    mFileHash.Clear();
    mDiffHashArray.clear();

    // Assert that the user has handled any previous error state and called ClearError when done.
    EAPATCH_ASSERT(!HasError());
    EAPATCH_ASSERT((diffBlockSize <= kBufferSize) && EA::StdC::IsPowerOf2(diffBlockSize));

    EA::Patch::File file;

    if(file.Open(pFileSourceFullPath, EA::IO::kAccessFlagRead, EA::IO::kCDOpenExisting, 
                  EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential))
    {
        bool                 bDoneReadingFile = false;
        EA::Patch::BlockHash blockHash;
        EA::Patch::MD5       md5;
        uint64_t             readSizeResult = 0;
        size_t               i;

        if(generateFileHash)
            md5.Begin();

        while(!file.HasError() && !bDoneReadingFile)
        {
            if(file.Read(&mBuffer[0], kBufferSize, readSizeResult, false))
            {
                EAPATCH_ASSERT(readSizeResult <= kBufferSize);

                mFileSize += readSizeResult;
                uint64_t readSizeAlignedUp = EA::StdC::AlignUp(readSizeResult, diffBlockSize);

                if(readSizeResult < kBufferSize) // If the file ended on boundary != kBufferFileSize... then we have reached the end of the file, so fill in the rest of the buffer as necessary with 0.
                {
                    EA::StdC::Memset8(&mBuffer[0] + readSizeResult, 0, (size_t)(readSizeAlignedUp - readSizeResult));
                    bDoneReadingFile = true;
                }

                if(generateFileHash)
                    md5.AddData(&mBuffer[0], (size_t)readSizeResult);

                if(generateDiffHash)
                {                    
                    // We are guaranteed that kBufferSize is a multiple of diffBlockSize. So we can just do a simple loop.
                    for(i = 0; i < readSizeAlignedUp; i += diffBlockSize)
                    {
                        // Calculate mChecksum for the block.
                        blockHash.mChecksum = EA::Patch::GetRollingChecksum(&mBuffer[0] + i, diffBlockSize);

                        // Calculate mHashValue for the block.
                        HashValueMD5 hashMD5;
                        MD5::GetHashValue(&mBuffer[0] + i, diffBlockSize, hashMD5);
                        GenerateBriefHash(hashMD5, blockHash.mHashValue);

                        mDiffHashArray.push_back(blockHash);
                    }

                    EAPATCH_ASSERT(EA::StdC::IsAligned(i, diffBlockSize));
                }
            }
        }

        file.Close();    

        if(generateFileHash)
            md5.End(mFileHash);

        // Move the error state of file to us.
        TransferError(file);
    }
    // else Open will have set the Error state.

    // Move the error state of file to us.
    TransferError(file);

    return mbSuccess;
}


void FileReader::GetFileSize(uint64_t& fileSize) const
{
    fileSize = mFileSize;
}


void FileReader::GetFileHash(HashValueMD5& fileHash) const
{
    fileHash = mFileHash;
}


void FileReader::GetDiffHash(BlockHashArray& diffHashArray) const
{
    diffHashArray = mDiffHashArray;
}



/////////////////////////////////////////////////////////////////////////////
// FileChecksumReader
/////////////////////////////////////////////////////////////////////////////

FileChecksumReader::FileChecksumReader()
    : mBuffer()
    , mBufferSize(0)
    , mBufferReadPosition(0)
    , mFile()
    , mDiffBlockSize(0)
    , mDiffBlockShift(0)
    , mBlockPosition(0)
    , mPrevChecksum(0)
    , mbDone(false)
    , mByteBufferIndex(-1)
    , mValidBytes(0)
{
        memset(mByteBuffer, 0, kHelperBufferSize);
}


bool FileChecksumReader::Open(const char8_t* pFileSourceFullPath, size_t diffBlockSize)
{
    if(mbSuccess)   
    {
        EAPATCH_ASSERT((diffBlockSize <= kDiffBlockSizeMax) && (diffBlockSize <= kBufferSize));

        mBufferSize          = diffBlockSize * 2;   // We are implementing a ring buffer. This size can be any size > diffBlockSize.
        mBufferReadPosition  = 0;
        mDiffBlockSize       = diffBlockSize;
        mDiffBlockShift      = EA::StdC::Log2((uint32_t)diffBlockSize);
        mBlockPosition       = 0;
        mPrevChecksum        = 0;
        mbDone               = false;

        mBuffer.resize(mBufferSize);
        #if EAPATCH_DEBUG
            EA::Patch::RandomizeMemory(&mBuffer[0], mBufferSize);
        #endif

        if(!mFile.Open(pFileSourceFullPath, EA::IO::kAccessFlagRead, EA::IO::kCDOpenExisting, 
                        EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential, 8192, 0))
        {
            TransferError(mFile);
        }
    }

    return mbSuccess;
}


bool FileChecksumReader::Close()
{
    if(!mFile.Close()) // This returns true if the file wasn't open.
    {
        if(mbSuccess) // Only transfer the error if we don't have any error already.
            TransferError(mFile);
        else
            mFile.ClearError();
    }

    return mbSuccess;
}


bool FileChecksumReader::GoToFirstByte(uint32_t& checksum, bool& bDone, size_t offset)
{
    if(mbSuccess && !mbDone)
    {
        // Set mBlockPosition to -mDiffBlockSize because we take advantage of GoToNextBlock below, 
        // which will do a += mDiffBlockSize and thus read from byte 0. So here we are pretending
        // to be at a negative position temporarily, and GoToNextBlock will move that forward to 0.
        // This allows us to simply have GoToNextBlock do our work instead of having redundant code here.
        mBlockPosition  = offset;
        mBlockPosition -= mDiffBlockSize;

        return GoToNextBlock(checksum, bDone);
    }

    bDone = mbDone;

    return mbSuccess;
}


bool FileChecksumReader::GoToNextByte(uint32_t& checksum, bool& bDone)
{
    if (mbSuccess && !mbDone)
    {

        mbDone = true; // True until proven false below.

        ++mBlockPosition;
        if (mBufferReadPosition >= mBufferSize)  // If we reached the end of this working buffer...
            mBufferReadPosition = 0;            // wrap around to the beginning of the buffer. We are implementing a ring buffer here.

        size_t   prevBufferReadPosition = (mBufferReadPosition >= mDiffBlockSize) ?
            (mBufferReadPosition - mDiffBlockSize) :
            ((mBufferSize + mBufferReadPosition) - mDiffBlockSize);

        if (mByteBufferIndex == -1 || mByteBufferIndex >= kHelperBufferSize
            || (mValidBytes != 0 && mByteBufferIndex >= mValidBytes))
        {
            mValidBytes = 0;
            mFile.Read(&mByteBuffer[0], kHelperBufferSize, mValidBytes, false);
            mByteBufferIndex = 0;
        }

        // Read the next byte. This File class is already buffered, so this won't be hitting the disk slowly every time.
        if (mValidBytes)
        {
            EAPATCH_ASSERT(mByteBufferIndex <= mValidBytes);
            mBuffer[mBufferReadPosition] = mByteBuffer[mByteBufferIndex++];
            mPrevChecksum = UpdateRollingChecksum(mPrevChecksum, mBuffer[prevBufferReadPosition], mBuffer[mBufferReadPosition], mDiffBlockShift);
            mbDone = false;
            mBufferReadPosition++;
        }
        checksum = mPrevChecksum;

        if (mFile.HasError())
        {
            TransferError(mFile);
            checksum = kChecksumError; // This shouldn't be required, but should help avoid accidental errors.
            mbDone = true;
        }

    }
    bDone = mbDone;

    return mbSuccess;
}


bool FileChecksumReader::GoToNextBlock(uint32_t& checksum, bool& bDone)
{
    if (!mbSuccess && mbDone)
    {
        bDone = mbDone;
        return false;
    }

    mByteBufferIndex = -1;
    mbDone = true; // True until proven false below.

    mBlockPosition += mDiffBlockSize;
    mBufferReadPosition = 0;

    if (mFile.SetPosition((int64_t)mBlockPosition, EA::IO::kPositionTypeBegin))
    {
        // To consider: Implement an optimization for smaller files that results in reading the 
        //              entire file into a buffer instead of working on it incrementally.
        uint64_t readSizeResult = 0;

        if (mFile.Read(&mBuffer[0], mDiffBlockSize, readSizeResult, false)) // Don't need to read mBufferSize, which is > mDiffBlockSize.
        {
            if (readSizeResult > 0) // If anything more existed to read...
            {
                mbDone = false;

                if (readSizeResult < mDiffBlockSize) // If we didn't read an entire block, clear the rest of the block so the GetRollingChecksum call below works on predictable data.
                    EA::StdC::Memclear(&mBuffer[0] + (size_t)readSizeResult, (mDiffBlockSize - (size_t)readSizeResult));

                // Since this is the first time reading the block, we do a block checksum read and not an incremental one.
                mPrevChecksum = GetRollingChecksum(&mBuffer[0], mDiffBlockSize);
                checksum = mPrevChecksum;
                mBufferReadPosition += (size_t)readSizeResult;
            }
        }
    }

    if (mFile.HasError())
    {
        TransferError(mFile);
        checksum = kChecksumError;  // This shouldn't be required, but should help avoid accidental errors.
        mbDone = true;
    }

    bDone = mbDone;

    return mbSuccess;
}


void FileChecksumReader::GetCurrentBriefHash(HashValueMD5Brief& hashValueBrief, uint64_t& blockPosition) const
{
    // We have assumed (and optimize for the case) that the user won't be calling this function often.
    if(mbSuccess)
    {
        const size_t  bufferStartPosition = (mBufferReadPosition >= mDiffBlockSize) ? (mBufferReadPosition - mDiffBlockSize) : ((mBufferSize + mBufferReadPosition) - mDiffBlockSize);
        const ssize_t wraparoundSize      = (((ssize_t)bufferStartPosition + (ssize_t)mDiffBlockSize) - (ssize_t)mBufferSize); // wraparoundSize = number of bytes to read from the beginning of mBuffer
        HashValueMD5  hashValueMD5;

        if(wraparoundSize <= 0) // If the entire current diffBlock is contiguously within the ring buffer...
            MD5::GetHashValue(&mBuffer[bufferStartPosition], mDiffBlockSize, hashValueMD5);
        else // Else we need to calc the hashValue from a block of bytes at the end of the buffer plus a block at the beginning, since it's wrapping around.
        {
            EAPATCH_ASSERT(mBufferReadPosition > 0); // Sanity check on the above math.
            MD5::GetHashValue(&mBuffer[bufferStartPosition], mBufferSize - bufferStartPosition, &mBuffer[0], (size_t)wraparoundSize, hashValueMD5);
        }

        GenerateBriefHash(hashValueMD5, hashValueBrief);

        blockPosition = mBlockPosition;
    }
}


uint64_t FileChecksumReader::GetFileSize()
{
    if(mFile.GetStream())
        return mFile.GetStream()->GetSize();

    return 0;
}



/////////////////////////////////////////////////////////////////////////////
// DirectoryCleaner
/////////////////////////////////////////////////////////////////////////////

bool DirectoryCleaner::CleanDirectory(const char8_t* pDirectoryFullPath)
{
    if(mbSuccess)
    {
        if(!EA::IO::Directory::Remove(pDirectoryFullPath, true))
        {
            SystemErrorId systemErrorId = GetLastSystemErrorId();
            HandleError(kECBlocking, systemErrorId, kEAPatchErrorIdFileRemoveError, pDirectoryFullPath); // This will set mbSuccess to false.
        }
    }

    return mbSuccess;
}



/////////////////////////////////////////////////////////////////////////////
// File
/////////////////////////////////////////////////////////////////////////////


int32_t File::mOpenFileCount = 0;

int32_t File::GetOpenFileCount()
{
    return mOpenFileCount;
}


File::File()
  : mpStream(NULL)
  , mFileStream()
  , mFilePath()
  , mStreamBuffer()
  , mReadBufferSize(kReadBufferSizeDefault)
  , mWriteBufferSize(kWriteBufferSizeDefault)
{
    mFileStream.AddRef();       // We do this in case some code uses an AddRef/Release pair against mFileStream.
    mStreamBuffer.AddRef();     // 
    mpStream = &mFileStream;    // By default it points to our own mFileStream, though the user can override it.
}


File::File(const File& file)
  : ErrorBase(file)
  , mpStream(NULL)
  , mFileStream()
  , mFilePath(file.mFilePath)   // The only thing we copy is the path that the other file has.
  , mStreamBuffer()
  , mReadBufferSize(kReadBufferSizeDefault)
  , mWriteBufferSize(kWriteBufferSizeDefault)
{
    mFileStream.AddRef();       // We do this in case some code uses an AddRef/Release pair against mFileStream.
    mStreamBuffer.AddRef();     // 
    mpStream = &mFileStream;    // By default it points to our own mFileStream, though the user can override it.
}


void File::operator=(const File& file)
{
    mFilePath = file.mFilePath; // The only thing we copy is the path that the other file has.
}


File::~File()
{
    if(mpStream)
    {
        // We assert that the user has explicitly closed the file before we are destroyed. 
        // The reason we do this is because we want to capture file close errors the same 
        // as file open/read/write errors. But we couldn't do that if the user let us
        // lazy close in the destructor.
        EAPATCH_ASSERT(mpStream->GetAccessFlags() == 0);

        if(mpStream->GetAccessFlags())  // If is open...
            mpStream->Close();          // Lose the return value of this call.

        mStreamBuffer.SetStream(NULL);  // This will Release() the mpStream associated with mStreamBuffer.
    }
}


void File::SetStream(EA::IO::IStream* pStream, const char8_t* pName,
                      EA::IO::size_type readBufferSize, EA::IO::size_type writeBufferSize)
{
    // To consider: Do we expect the user to call ClearError if this happens to be a File that
    // was previously used and encountered an error? Currently we clear the error ourselves upon SetStream:
    mbSuccess = true;
    mError.Clear();

    mpStream  = pStream;
    mFilePath = pName;
    mStreamBuffer.SetStream(pStream);

    mReadBufferSize  = readBufferSize;
    mWriteBufferSize = writeBufferSize;
    mStreamBuffer.SetBufferSizes(readBufferSize, writeBufferSize);
}


EA::IO::IStream* File::GetStream()
{
    return &mStreamBuffer; // The problem with directly returning mpStream is that usage of it by the caller bypasses mStreamBuffer and could result in unexpected state.
}


bool File::Open(const char8_t* pPath, int nAccessFlags, int nCreationDisposition, int nSharing, int nUsageHints,
                      EA::IO::size_type readBufferSize, EA::IO::size_type writeBufferSize)
{
    EAPATCH_ASSERT(pPath);
    EAPATCH_ASSERT(!IsOpen());  // Can't doubly open. We'll catch the error at runtime below in the attempt to open the file.

    // To consider: Do we expect the user to call ClearError if this happens to be a File that
    // was previously used and encountered an error? Currently we clear the error ourselves upon Open:
    ClearError();

    // Create the directory for the file if needed.
    if(nAccessFlags & EA::IO::kAccessFlagWrite) // We assume that all write requests want the directory to exist. It's strictly possible with kCDOpenExisting that this isn't the case, but that combination is odd and uncommon.
    {
        String sDirectoryPath(pPath);
        StripFileNameFromFilePath(sDirectoryPath);

        if(!EA::IO::Directory::Exists(sDirectoryPath.c_str()))
        {
            if(!EA::IO::Directory::Create(sDirectoryPath.c_str())) // Directory::Create will create the directory tree as needed.
                HandleFileError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdDirCreateError, sDirectoryPath.c_str());
        }

        #if (EAPATCH_DEBUG >= 1)
            // The following aids in debugging.
            nAccessFlags |= EA::IO::kAccessFlagRead;
            nSharing      = EA::IO::FileStream::kShareRead;
        #endif
    }

    if(mbSuccess)
    {
        mFileStream.SetPath(pPath);
        SetStream(&mFileStream, pPath, readBufferSize, writeBufferSize);

        if(mFileStream.Open(nAccessFlags, nCreationDisposition, nSharing, nUsageHints))
            mOpenFileCount++;
        else
            HandleFileError(kECBlocking, GetStreamSystemErrorId(), kEAPatchErrorIdFileOpenFailure);
    }

    return mbSuccess;
}


bool File::Read(void* pData, uint64_t readSize, uint64_t& readSizeResult, bool expectReadSize)
{
    if(mbSuccess)
    {
        // To do: Use mStreamBuffer here insted of mpStream directly. 

        // To consider: Make this a runtime failure as opposed to a debug assertion.
        // EAIO defaults to a 32 bit size_type on 32 bit platforms, though there's an option to make it 64 bit. 
        EAPATCH_ASSERT((sizeof(EA::IO::size_type) >= 8) || (readSize <= UINT32_MAX)); // Verify that readSize doesn't overflow the max size Write can handle.
        EAPATCH_ASSERT(mpStream != NULL);

        const EA::IO::size_type result = mStreamBuffer.Read(pData, (EA::IO::size_type)readSize);

        if(result == EA::IO::kSizeTypeError) // If there was a file reading failure.
            HandleFileError(kECBlocking, GetStreamSystemErrorId(), kEAPatchErrorIdFileReadFailure);
        else
        {
            readSizeResult = result;

            if(expectReadSize && (readSizeResult != readSize)) // If the user expected readSize bytes but got less...
                HandleFileError(kECFatal, kSystemErrorIdNone, kEAPatchErrorIdFileReadTruncate);
        }        
    }

    return mbSuccess;
}


bool File::ReadPosition(void* pData, uint64_t readSize, uint64_t readPosition, uint64_t& readSizeResult, bool expectReadSize)
{
    // Due to the way this class works, we can call this and ignore the 
    // result, as any error would be picked up by the subsequent Read.
    SetPosition((int64_t)readPosition, EA::IO::kPositionTypeBegin); // This int64_t cast is OK because nobody will ever use this class to write anywhere near this many bytes.

    return Read(pData, readSize, readSizeResult, expectReadSize);
}


bool File::Write(const void* pData, uint64_t writeSize)
{
    if(mbSuccess)
    {
        // To consider: Make this a runtime failure as opposed to a debug assertion.
        // EAIO defaults to a 32 bit size_type on 32 bit platforms, though there's an option to make it 64 bit. 
        EAPATCH_ASSERT((sizeof(EA::IO::size_type) >= 8) || (writeSize <= UINT32_MAX)); // Verify that readSize doesn't overflow the max size Write can handle.
        EAPATCH_ASSERT(mpStream != NULL);

        if(!mStreamBuffer.Write(pData, (EA::IO::size_type)writeSize)) // If there was a file writing failure.
            HandleFileError(kECBlocking, GetStreamSystemErrorId(), kEAPatchErrorIdFileWriteFailure);
    }

    return mbSuccess;
}


bool File::WritePosition(const void* pData, uint64_t writeSize, uint64_t writePosition)
{
    // Due to the way this class works, we can call this and ignore the 
    // result, as any error would be picked up by the subsequent Write.
    SetPosition((int64_t)writePosition, EA::IO::kPositionTypeBegin); // This int64_t cast is OK because nobody will ever use this class to write anywhere near this many bytes.

    return Write(pData, writeSize);
}


bool File::GetPosition(int64_t& position, EA::IO::PositionType positionType)
{
    if(mbSuccess)
    {
        EAPATCH_ASSERT(mpStream != NULL);
        EA::IO::off_type result = mStreamBuffer.GetPosition(positionType);

        if(result == (EA::IO::off_type)EA::IO::kSizeTypeError) // If there was a file system error...
            HandleFileError(kECFatal, GetStreamSystemErrorId(), kEAPatchErrorIdFilePosFailure);
        else
            position = result;
    }

    return mbSuccess;
}


bool File::SetPosition(int64_t position, EA::IO::PositionType positionType)
{
    if(mbSuccess)
    {
        // To consider: Make this a runtime failure as opposed to a debug assertion.
        // EAIO defaults to a 32 bit size_type on 32 bit platforms, though there's an option to make it 64 bit. 
        EAPATCH_ASSERT((sizeof(EA::IO::size_type) >= 8) || (position <= INT32_MAX)); // Verify that readSize doesn't overflow the max size SetPosition can handle.
        EAPATCH_ASSERT((mpStream != NULL) && (mStreamBuffer.GetStream() != NULL));

        if(!mStreamBuffer.SetPosition((EA::IO::off_type)position, positionType)) // If there was a file system error...
            HandleFileError(kECBlocking, GetStreamSystemErrorId(), kEAPatchErrorIdFilePosFailure);
    }

    return mbSuccess;
}


bool File::GetSize(uint64_t& size)
{
    size = mStreamBuffer.GetSize();
    return mbSuccess;
}


bool File::Flush()
{
    mStreamBuffer.Flush();
    return mbSuccess;
}


bool File::Close()
{
    bool bFileAlreadyInError = HasError();

    // We currently don't check mbSuccess because we want to close the file if it's open in any case.
    //if(mbSuccess)
    {
        if(mpStream)
        {
            EAPATCH_ASSERT((mpStream == mStreamBuffer.GetStream()) || (mStreamBuffer.GetStream() == NULL)); // Assert that if mStreamBuffer is set that it is set to mpStream.
            EAPATCH_ASSERT((mpStream->GetAccessFlags() == 0) == (mStreamBuffer.GetAccessFlags() == 0));     // Assert that if mStreamBuffer is open that mpStream is open, and same for closed.

            if(mStreamBuffer.GetAccessFlags()) // If open...
            {
                if(mStreamBuffer.Close())  // The user explicitly called Close, so we close the stream even if it's a user-supplied stream.
                    mOpenFileCount--;
                else
                {
                    if(!bFileAlreadyInError)
                        HandleFileError(kECBlocking, GetStreamSystemErrorId(), kEAPatchErrorIdFileCloseFailure);
                }
            }
        }
    }

    mStreamBuffer.SetStream(NULL);

    return mbSuccess;
}


SystemErrorId File::GetStreamSystemErrorId() const
{
    if(mpStream)
    {
        SystemErrorId systemErrorId = (SystemErrorId)mStreamBuffer.GetState(); // This returns the state of the underlying mpStream.
        return systemErrorId;

        // We don't really know if the stream error is a OS-level error id. It could a user-defined 
        // error code, or it could also be one of enum EA::IO::IOResultCode. We should probably 
        // be as friendly as possible and match systemErrorId against IOResultCode and return that if so.
        // Otherwise if we just return systemErrorId when it's not really an OS error then the user 
        // might see some odd unrelated error string printed on the screen.
    }

    return kSystemErrorIdNone;
}


void File::HandleFileError(ErrorClass errorClass, SystemErrorId systemErrorId, EAPatchErrorId eaPatchErrorId, const char8_t* pPath)
{
    if(mpStream == &mFileStream)
        mpStream->Close();

    ErrorBase::HandleError(errorClass, systemErrorId, eaPatchErrorId, pPath ? pPath : mFilePath.c_str());
}




/////////////////////////////////////////////////////////////////////////////
// FileComparer
/////////////////////////////////////////////////////////////////////////////

FileComparer::FileComparer()
  : mFileSource()
  , mFileDest()
{
}


FileComparer::~FileComparer()
{
    Close();
}

bool FileComparer::Open(const char8_t* pSourceFilePath, const char8_t* pDestinationFilePath)
{
    ClearError();

    if(!OpenTwoFiles(mFileSource, pSourceFilePath, EA::IO::kAccessFlagRead, EA::IO::kCDOpenExisting, EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential, 
                     mFileDest, pDestinationFilePath, EA::IO::kAccessFlagRead, EA::IO::kCDOpenExisting, EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential,
                     mError))
    {
        HandleError(mError); // This will set mbSuccess to false.
    }

    return mbSuccess;
}


bool FileComparer::IsOpen() const
{
    return (mFileSource.IsOpen() && mFileDest.IsOpen());
}


bool FileComparer::Close()
{
    // We close the files regardless of mbSuccess.

    if(!mFileSource.Close()) // Close returns true if the file wasn't open, so this expression will be true only if there was an error closing the file.
    {
        if(mbSuccess) // Only transfer the error if we don't have any error already.
            TransferError(mFileSource);
        else
            mFileSource.ClearError();
    }

    if(!mFileDest.Close()) // Close returns true if the file wasn't open, so this expression will be true only if there was an error closing the file.
    {
        if(mbSuccess) // Only transfer the error if we don't have any error already.
            TransferError(mFileDest);
        else
            mFileDest.ClearError();
    }

    return mbSuccess;
}


bool FileComparer::CompareSpan(uint64_t sourceStartPos, uint64_t destStartPos, uint64_t compareCount, bool& bEqual)
{
    bEqual = false; // False until proven true.

    if(mbSuccess)
    {
        const size_t kBufferSize = 65536;
        char         bufferSource[kBufferSize];
        char         bufferDest[kBufferSize];
        uint64_t     readCount   = 0;                // How much of compareCount has thus far been read.
        uint64_t     remainingCount = compareCount;  // How much of compareCount remains to be read. readCount + remainingCount = compareCount.

        while(remainingCount && mbSuccess)
        {
            uint64_t currentCount = eastl::min_alt(remainingCount, (uint64_t)kBufferSize);
            uint64_t currentCountResult = 0;

            if(mFileSource.ReadPosition(bufferSource, currentCount, sourceStartPos + readCount, currentCountResult, true) && (currentCount == currentCountResult))
            {
                if(mFileDest.ReadPosition(bufferDest, currentCount, destStartPos + readCount, currentCountResult, true) && (currentCount == currentCountResult))
                {
                    if(EA::StdC::Memcmp(bufferSource, bufferDest, (size_t)currentCount) != 0)
                        break;

                    remainingCount -= currentCount;
                    readCount      += currentCount;
                }
                else
                    TransferError(mFileDest);
            }
            else
                TransferError(mFileSource);
        }

        if(remainingCount == 0) // If it made it here then all reads and comparisons succeeded.
        {
            EAPATCH_ASSERT(mbSuccess);
            bEqual = true;
        }
    }

    return mbSuccess;
}



/////////////////////////////////////////////////////////////////////////////
// FileSpanCopier
/////////////////////////////////////////////////////////////////////////////

FileSpanCopier::FileSpanCopier()
  : mFileSource()
  , mFileDest()
{
}


bool FileSpanCopier::Open(const char8_t* pSourceFilePath, const char8_t* pDestinationFilePath)
{
    ClearError();

    if(!OpenTwoFiles(mFileSource, pSourceFilePath, EA::IO::kAccessFlagRead, EA::IO::kCDOpenExisting, EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential, 
                     mFileDest, pDestinationFilePath, EA::IO::kAccessFlagWrite, EA::IO::kCDOpenAlways, EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential,
                     mError))
    {
        HandleError(mError); // This will set mbSuccess to false.
    }

    return mbSuccess;
}


bool FileSpanCopier::Close()
{
    // We close the files regardless of mbSuccess.

    if(!mFileSource.Close()) // Close returns true if the file wasn't open, so this expression will be true only if there was an error closing the file.
    {
        if(mbSuccess) // Only transfer the error if we don't have any error already.
            TransferError(mFileSource);
        else
            mFileSource.ClearError();
    }

    if(!mFileDest.Close()) // Close returns true if the file wasn't open, so this expression will be true only if there was an error closing the file.
    {
        if(mbSuccess) // Only transfer the error if we don't have any error already.
            TransferError(mFileDest);
        else
            mFileDest.ClearError();
    }

    return mbSuccess;
}


// We currently do an all-or-nothing write of the entire span, regardless of its size.
// For some files this could be 10+ gigabytes on big modern generation systems.
// To consider: Provide an option to write in a way that lets the rest of the system 
// execute more, including its own disk usage, as this write will saturate the disk
// until it completes. Additionally, under Windows such big writes will cause the VM 
// to switch all system memory to the disk task and cause the system to be slow for 
// minutes after the write.
bool FileSpanCopier::CopySpan(uint64_t sourceStartPos, uint64_t destStartPos, uint64_t copyCount)
{
    if(mbSuccess)
    {
        char     buffer[65536];
        uint64_t writtenCount   = 0;          // How much of copyCount has thus far been written.
        uint64_t remainingCount = copyCount;  // How much of copyCount remains to be written. writtenCount + remainingCount = copyCount.

        while(remainingCount && mbSuccess)
        {
            uint64_t currentCount = eastl::min_alt(remainingCount, (uint64_t)EAArrayCount(buffer));

            if(mFileSource.ReadPosition(buffer, currentCount, sourceStartPos + writtenCount, currentCount, true))
            {
                if(mFileDest.WritePosition(buffer, currentCount, destStartPos + writtenCount))
                {
                    remainingCount -= currentCount;
                    writtenCount   += currentCount;
                }
                else
                    TransferError(mFileDest);
            }
            else
                TransferError(mFileSource);
        }
    }

    return mbSuccess;
}



/////////////////////////////////////////////////////////////////////////////
// OpenTwoFiles
/////////////////////////////////////////////////////////////////////////////

bool OpenTwoFiles(File& file1, const char8_t* pPath1, int nAccessFlags1, int nCreationDisposition1, int nSharing1, int nUsageHints1, 
                  File& file2, const char8_t* pPath2, int nAccessFlags2, int nCreationDisposition2, int nSharing2, int nUsageHints2,
                  Error& error)
{
    bool bResult = false;

    if(file1.Open(pPath1, nAccessFlags1, nCreationDisposition1, nSharing1, nUsageHints1))
    {
        if(file2.Open(pPath2, nAccessFlags2, nCreationDisposition2, nSharing2, nUsageHints2))
            bResult = true;
        else
        {
            file1.Close();
            error = file2.GetError();
        }
    }
    else
        error = file1.GetError();

    return bResult;
}



} // namespace Patch
} // namespace EA








