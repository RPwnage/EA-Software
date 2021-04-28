///////////////////////////////////////////////////////////////////////////////
// EAPatchMaker/Maker.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAPatchMaker/Maker.h>
#include <EAPatchMaker/Report.h>
#include <EAPatchClient/PatchInfo.h>
#include <EAPatchClient/PatchImpl.h>
#include <EAPatchClient/XML.h>
#include <EAPatchClient/File.h>
#include <EAPatchClient/URL.h>
#include <EAPatchClient/PatchBuilder.h>
#include <EAPatchClient/internal/PatchBuilderInternal.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAFileDirectory.h>
#include <EAIO/EAStreamMemory.h>
#include <EAIO/FnMatch.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EABitTricks.h>

// Windows headers #define CopyFile to CopyFileA or CopyFileW. So revert that.
#if defined(CopyFile)
    #undef CopyFile
#endif



namespace EA
{
namespace Patch
{


///////////////////////////////////////////////////////////////////////////////
// MakerSettings
///////////////////////////////////////////////////////////////////////////////


MakerSettings::MakerSettings()
{
    Reset();
}

void MakerSettings::Reset()
{
    mPatchImpl.Reset();

    mPatchImplPath.set_capacity(0);     // This causes the memory to be freed while also clearing the container.
    mInputDirectory.set_capacity(0);
    mOutputDirectory.set_capacity(0);
    mPreviousDirectory.set_capacity(0);
    mbValidatePatchImpl = false;
    mbCleanOutputDirectory = true;
    mbEnableOptimizations = true;
    mbEnableErrorCleanup = true;
    mMaxFileCount = kMaxPatchFileCountDefault;
    mBlockSize = 0;
}


///////////////////////////////////////////////////////////////////////////////
// Maker
///////////////////////////////////////////////////////////////////////////////

Maker::Maker()
  : mMakerSettings()
  , mPatchDirectoryList()
  , mPatchFileList()
  , mIgnoredPatchDirectoryList()
  , mIgnoredPatchFileList()
  , mPatchInfoFilePath()
  , mPatchImplFilePath()
{
}


Maker::~Maker()
{
}


bool Maker::ValidateMakerSettings()
{
    if(mbSuccess)
    {
        String sError;

        // The following doesn't internally set the class Error, as it's intended to be just a checking function.
        if(!ValidateMakerSettings(mMakerSettings, sError))
            HandleError(kECFatal, kSystemErrorIdNone, kEAPatchErrorIdInvalidSettings, sError.c_str());

        if(mMakerSettings.mPatchImpl.HasError())
            TransferError(mMakerSettings.mPatchImpl);
    }

    return mbSuccess;
}


void Maker::CreateDefaultPatchImpl(PatchImpl& patchImpl)
{
    #define kDefaultPatchName  "Your Patch 1"
    #define kDefaultPatchName2 "Your%20Patch%201"
    #define kDefaultAppName    "Your Game"
    #define kDefaultServerName "YourPatchServer"

    String sTemp;

    patchImpl.mPatchInfoURL.sprintf("http://%s/%s%s (This is a required field)", kDefaultServerName, kDefaultPatchName2, kPatchInfoFileNameExtension);
    patchImpl.mPatchId.sprintf("%s Id (some identifying tag or number) - fill this in. (This is a required field)", kDefaultPatchName);
    patchImpl.mPatchClass.sprintf("patch (could be add-on or some application defined value.) - fill this in. (this is a optional field)");
    patchImpl.mRequired = false;
    sTemp.sprintf("http://%s/%s%s (This is a required field)", kDefaultServerName, kDefaultPatchName2, kPatchImplFileNameExtension);
    patchImpl.mPatchImplURL.AddString("en_us", sTemp.c_str());
    patchImpl.mContentIdArray.push_back(String("12345 - fill this in or remove this entry, as it's optional."));
    patchImpl.mContentIdArray.push_back(String("23456 - fill this in or remove this entry, as it's optional."));
    sTemp.sprintf("%s SKU abcde (this is a optional field)", kDefaultPatchName);
    patchImpl.mSKUIdArray.push_back(sTemp);
    sTemp.sprintf("%s SKU pdqxyz (this is a optional field)", kDefaultPatchName);
    patchImpl.mSKUIdArray.push_back(sTemp);
    sTemp.sprintf("%s (this is a optional field)", kDefaultPatchName);
    patchImpl.mPatchName.AddString("en_us", sTemp.c_str());
    sTemp.sprintf("Description for %s. Can be text, HTML, or a URL. (this is a optional field)", kDefaultPatchName);
    patchImpl.mPatchDescription.AddString("en_us", sTemp.c_str());
    sTemp.sprintf("By a agreeing to install %s, ... Can be text or HTML. (this is a optional field)", kDefaultPatchName);
    patchImpl.mPatchEULA.AddString("en_us", sTemp.c_str());
    sTemp.sprintf("(spanish) By a agreeing to install %s, ... (this is a optional field)", kDefaultPatchName);
    patchImpl.mPatchEULA.AddString("es_es", sTemp.c_str());
    patchImpl.mAppId.sprintf("%s -- fill this in or remove it, as it's optional.", kDefaultAppName);
    sTemp.sprintf("%s - use this in or remove it, as it's optional.", EAPATCH_OS_PS3_STR);
    patchImpl.mOSNameArray.push_back(sTemp);
    sTemp.sprintf("%s - use this in or remove it, as it's optional.", EAPATCH_OS_PS3_STR);
    patchImpl.mOSNameArray.push_back(sTemp);
    patchImpl.mOSNameArray.push_back(String("3460 (some XBox 360 version) (this is a optional field)"));
    patchImpl.mOSNameArray.push_back(String("430 (some PS3 version) (this is a optional field)"));
    patchImpl.mReleaseDate.sprintf("yyyy-MM-dd hh:mm:ss GMT (this is a optional field)");
    patchImpl.mLocaleSupport.AddString("en_us", "(this is a optional field)");
    patchImpl.mLocaleSupport.AddString("es_es", "(this is a optional field)");
  //patchImpl.mSupercedentPatchIdArray
  //patchImpl.mSupercedingPatchIdArray  
  //patchImpl.mDependentPatchIdArray
    patchImpl.mPatchBaseDirectory.sprintf("%s/data/ (this is a optional field)", kDefaultAppName);
  //patchImpl.mUserInfo
  //patchImpl.mUserChecksum
  //patchImpl.mFinalFileSizeUsage
  //patchImpl.mFinalDiskSpaceUsage
  //patchImpl.mIntermediateDiskSpaceUsage
  //patchImpl.mPatchHash
  //patchImpl.mPreRunScript
  //patchImpl.mPostRunScript
  //patchImpl.mIgnoredFileArray
    FileFilter fileFilter;
    fileFilter.mFilterSpec = "*";
    fileFilter.mMatchFlags = 0;
    patchImpl.mUsedFileArray.push_back(fileFilter);
}


bool Maker::ValidateMakerSettings(const MakerSettings& settings, String& sError)
{
    using namespace EA::Patch;

    int errorCount = 0;

    // We do a first level validation of MakerSettings. It's useful to do that 
    // here instead of letting the patching process start and possibly create 
    // data, whereas we can catch basic problems here and save the user some 
    // time and headaches. We can also give more informative usage help here 
    // than the MakePatch can while doing its work.


    //////////////////////////////////////////////////////////////////////
    // Make sure all required settings.mPatchImpl fields are sane.
    //////////////////////////////////////////////////////////////////////

    // We don't currently require this, as it's generated:
    //if(mPatchImpl.mPatchInfoURL.empty())
    //{
    //    sError.append_sprintf("%s", "No Patch Info URL was specified.\n");
    //    errorCount++;
    //}

    if(!settings.mPatchImplPath.empty()) // If the user specified a path to a .eaPatchImpl file...
    {
        if(!EA::IO::File::Exists(settings.mPatchImplPath.c_str())) // If that file doesn't seem to exist...
        {
            // Note that we ignore settings.mbValidatePatchImpl, as the user specified a 
            // file path and we take that to mean a file was in fact intended to be used. 
            sError.append_sprintf("EAPatchImpl file \"%s\" doesn't exist.\n", settings.mPatchImplPath.c_str());
            errorCount++;
        }
    }

    // If the patchImpl contents weren't already specified and if there is a 
    // file to specify them... read the contents from the file.
    String    sPatchImplPath;
    PatchImpl patchImpl;
    bool      patchImplRead = false;

    if(settings.mPatchImplPath.empty())
       sPatchImplPath = FindPatchImplFile(settings.mInputDirectory.c_str());
    else
       sPatchImplPath = settings.mPatchImplPath;

    if(EA::IO::File::Exists(sPatchImplPath.c_str()))
    {
        // We read the XML file here now. 
        EA::Patch::PatchImplSerializer patchImplSerializer;

        if(patchImplSerializer.Deserialize(patchImpl, sPatchImplPath.c_str(), true, true))
            patchImplRead = true;
        else
        {
            sError.append_sprintf("%s", "EAPatchImpl file \"%s\" was invalid and couldn't be completely read.\n", settings.mPatchImplPath.c_str());
            errorCount++;
        }
    }
    else if(!settings.mbValidatePatchImpl)
        CreateDefaultPatchImpl(patchImpl);
    else
    {
        // Else the caller better have somehow already set up settings.mPatchImpl, as we'll detect below.
    }

    if(!patchImplRead)
        patchImpl = settings.mPatchImpl;

    if(settings.mbValidatePatchImpl)
    {
        if(patchImpl.mPatchId.empty())
        {
            sError.append_sprintf("%s", "No Patch Id was specified.\n");
            errorCount++;
        }

        if(patchImpl.mPatchClass.empty())
        {
            sError.append_sprintf("%s", "No Patch class was specified.\n");
            errorCount++;
        }

        if(!patchImpl.mPatchImplURL.Count())
        {
            sError.append_sprintf("%s", "No Patch Implementation URL was specified.\n");
            errorCount++;
        }

        // To do if we enable this test: Need to verify not just that there are entries, but also that no entries are empty.
        //if(patchImpl.mContentIdArray.empty())
        //{
        //    sError.append_sprintf("%s", "No content ids were specified.\n");
        //    errorCount++;
        //}

        // To do if we enable this test: Need to verify not just that there are entries, but also that no entries are empty.
        //if((patchImpl.mSKUIdArray.empty())
        //{
        //    sError.append_sprintf("%s", "No SKU ids were specified.\n");
        //    errorCount++;
        //}

        // To do: to verify not just that there are entries, but also that no entries are empty.
        if(!patchImpl.mPatchName.Count())
        {
            sError.append_sprintf("%s", "No patch name was specified.\n");
            errorCount++;
        }

        // To do: to verify not just that there are entries, but also that no entries are empty.
        if(!patchImpl.mPatchDescription.Count())
        {
            sError.append_sprintf("%s", "No Patch description was specified.\n");
            errorCount++;
        }

        // To do: to verify not just that there are entries, but also that no entries are empty.
        //if(patchImpl.mAppId.empty())
        //{
        //    sError.append_sprintf("%s", "No App Id was specified.\n");
        //    errorCount++;
        //}

        // To do if we enable this test: Need to verify not just that there are entries, but also that no entries are empty.
        //if(patchImpl.mOSNameArray.empty())
        //{
        //    sError.append_sprintf("%s", "No OS names were specified.\n");
        //    errorCount++;
        //}

        if(patchImpl.mPatchBaseDirectory.empty())
        {
            sError.append_sprintf("%s", "No patch base directory was specified.\n");
            errorCount++;
        }

        if(patchImpl.HasError())
        {
            patchImpl.GetError(); // We ignore the error, as it was already printed above.
            errorCount++;
        }
    }
    else
    {
        // Even if we are not validating we still need to retrieve the errorstate for patchImpl
        if (patchImpl.HasError())
        {
            EA::Patch::String errorMessage;
            patchImpl.GetError().GetErrorString(errorMessage);
            sError.append_sprintf("%s", "Error [%s] occured processing the patchImpl. Pass the -validatePatchImpl switch for more info.\n", errorMessage.c_str());
            errorCount++;
        }
    }

    //////////////////////////////////////////////////////////////////////
    // Make sure specified directories are OK.
    //////////////////////////////////////////////////////////////////////

    if(!EA::IO::Directory::Exists(settings.mInputDirectory.c_str()))
    {
        sError.append_sprintf("Input directory \"%s\" doesn't exist.\n", settings.mInputDirectory.c_str());
        errorCount++;
    }

    if(!EA::Patch::DirectoryIsEmpty(settings.mOutputDirectory.c_str()))
    {
        sError.append_sprintf("Output directory \"%s\" isn't empty, but needs to be.\n", settings.mOutputDirectory.c_str());
        errorCount++;
    }

    if(!EA::IO::Directory::EnsureExists(settings.mOutputDirectory.c_str()))
    {
        sError.append_sprintf("Unable to create output directory \"%s\".\n", settings.mOutputDirectory.c_str());
        errorCount++;
    }

    if(settings.mPreviousDirectory.length()) // If the previous directory is specified...
    {
        if(!EA::IO::Directory::Exists(settings.mPreviousDirectory.c_str()))
        {
            sError.append_sprintf("A previous directory \"%s\" was specified but doesn't exist.\n", settings.mPreviousDirectory.c_str());
            errorCount++;
        }
    }
    else
    {
        // To consider: Make this not be an error.
        if(settings.mbEnableOptimizations)
        {
            sError.append_sprintf("%s", "Optimizations were requested but a previous directory wasn't specified. Optimizations require this. The previous directory can be empty.\n");
            errorCount++;
        }
    }

    if(errorCount)
        ReportVerbosity(kReportVerbosityError, "%s", sError.c_str());

    // We don't call HandleError here, and don't report these errors outside the 

    return (errorCount == 0);
}


void Maker::Reset()
{
    ErrorBase::Reset();

    mMakerSettings.Reset();
    mPatchDirectoryList.clear();
    mPatchFileList.clear();
    mIgnoredPatchDirectoryList.clear();
    mIgnoredPatchFileList.clear();
    mPatchImpl.Reset();
    mPatchInfoFilePath.set_capacity(0);     // This causes the memory to be freed while also clearing the container.
    mPatchImplFilePath.set_capacity(0);
}


void Maker::FixUserDirectoryPaths()
{
    // To do (must do): Look at all user-provided paths and make sure that any which 
    // look like directories have terminating path separators. This is important because
    // all our internal code depends on it, else the code would be messy and error-prone.
    // If we ensure all code has the right path format here on entry, the rest will all work.

    // We check that the directories are not empty because empty paths are invalid, but adding
    // a path separator makes them valid, the file system root directory. So we don't want to 
    // do this and set a directory to a root directory.
    if(!mMakerSettings.mInputDirectory.empty())
        EnsureTrailingSeparator(mMakerSettings.mInputDirectory);
    if(!mMakerSettings.mOutputDirectory.empty())
        EnsureTrailingSeparator(mMakerSettings.mOutputDirectory);
    if(!mMakerSettings.mPatchImpl.mPatchBaseDirectory.empty())
        EnsureTrailingSeparator(mMakerSettings.mPatchImpl.mPatchBaseDirectory);
}


String Maker::FindPatchImplFile(const char8_t* pInputDirectoryPath)
{
    String sResult;

    // Find the first file in the input directory which has the .eaPatchImpl file name extension.
    EA::IO::DirectoryIterator dir;
    EA::IO::DirectoryIterator::EntryList entryList;

    wchar_t directoryW[EA::IO::kMaxPathLength]; // For EAIO_VERSION_N >= 21600 we don't to do this directory path copying.
    EA::StdC::Strlcpy(directoryW, pInputDirectoryPath, EAArrayCount(directoryW));

    wchar_t patternW[32];
    EA::StdC::Sprintf(patternW, L"*%I8s", kPatchImplFileNameExtension); // %I8s means char8_t string.

    size_t entryCount = dir.ReadRecursive(directoryW, entryList, patternW, 
                                          EA::IO::kDirectoryEntryFile, true, true, 1, false);
    if(entryCount == 1)
        sResult = EA::StdC::Strlcpy<String, EA::IO::DirectoryIterator::EntryString>(entryList.front().msName);

    return sResult;
}


bool Maker::CleanOutputDirectory()
{
    if(mbSuccess)
    {
        if(mMakerSettings.mbCleanOutputDirectory) // If cleaning is enabled...
        {
            if(EA::IO::Directory::Exists(mMakerSettings.mOutputDirectory.c_str()))
            {
                if(!EA::IO::Directory::Remove(mMakerSettings.mOutputDirectory.c_str(), true))    
                    HandleError(kECFatal, GetLastSystemErrorId(), kEAPatchErrorIdFileRemoveError, mMakerSettings.mOutputDirectory.c_str());
            }
        }
    }

    return mbSuccess;
}


// What we are creating:
//     One .eaPatchInfo file.
//     One .eaPatchImpl file.
//     Multiple .eaPatchDiff files.
//     Copy of the input directory, minus ignored directories and files.
//
bool Maker::MakePatch(const MakerSettings& settings)
{
    EA_UNUSED(settings);

    Reset();
    mMakerSettings = settings;
    CleanOutputDirectory();
    ValidateMakerSettings();
    FixUserDirectoryPaths();
    LoadPatchImplFile();
    BuildPatchFileInfoList();
    MakePatchImpl();
    WritePatchImpl();
    VerifyPatch();
    PrintUserReport();
    Cleanup();

    // Do final checks to make sure mPatchImpl and mMakerSettings have no error.
    if(mPatchImpl.HasError())
    {
        TransferError(mPatchImpl);

        // Disabled because we assume the error would already have been printed.
        // EA::Patch::String sError;
        // mError.GetErrorString(sError);
        // Report("%s", sError.c_str());
    }

    if(mMakerSettings.mPatchImpl.HasError())
        TransferError(mMakerSettings.mPatchImpl);

    return mbSuccess;
}


bool Maker::LoadPatchImplFile()
{
    if(mbSuccess)
    {
        if(mMakerSettings.mPatchImplPath.empty())
            mMakerSettings.mPatchImplPath = FindPatchImplFile(mMakerSettings.mInputDirectory.c_str());

        if(mMakerSettings.mPatchImplPath.empty())
        {
            if(mMakerSettings.mbValidatePatchImpl)
                HandleError(kECFatal, kSystemErrorIdNone, kEAPatchErrorIdFileNotSpecified, kPatchImplFileNameExtension);
            else
                CreateDefaultPatchImpl(mMakerSettings.mPatchImpl);
        }
        else if(!EA::IO::File::Exists(mMakerSettings.mPatchImplPath.c_str()))
        {
            // We don't read settings.mbValidatePatchImpl nor auto-create a default patch. If the user specified
            // a file path then we assume the user had a specific file in mind to use.
            HandleError(kECFatal, kSystemErrorIdNone, kEAPatchErrorIdFileNotFound, mMakerSettings.mPatchImplPath.c_str());
        }
        else // Else an eaPatchImpl file path was specified and the file exists...
        {
            EA::Patch::PatchImplSerializer patchImplSerializer;
    
            if(!patchImplSerializer.Deserialize(mMakerSettings.mPatchImpl, mMakerSettings.mPatchImplPath.c_str(), true, true))
                TransferError(patchImplSerializer);
        }
    }

    if(mbSuccess)
    {
        // We clear out PatchImpl::mPatchHash and mPatchEntryArray, as these are feilds we generate.
        mMakerSettings.mPatchImpl.mPatchHash.clear();
        mMakerSettings.mPatchImpl.mPatchEntryArray.clear();
    }

    return mbSuccess;
}


bool Maker::BuildPatchFileInfoList()
{
    if(mbSuccess)
    {
        FileInfoList ignoredFileList; // Note that these may be either directories or files.
        FileInfoList usedFileList;

        if(EA::Patch::GetFileList(mMakerSettings.mInputDirectory.c_str(), mMakerSettings.mPatchImpl.mIgnoredFileArray, &ignoredFileList, 
                                   mMakerSettings.mPatchImpl.mUsedFileArray, &usedFileList, NULL, true, true, mError))
        {
            // Go through the usedFileList and possibly move some items to the ignoredFileList.
            // We could possibly accomplish this by adding some filter specs to mIgnoredFileArray before
            // calling GetFileList above, but until we are sure that works exactly as expected, we use
            // the explicit method below.
            for(FileInfoList::iterator it = usedFileList.begin(); it != usedFileList.end(); ++it)
            {
                const FileInfo& fileInfo = *it;
                const String    sPathRelative = fileInfo.mFilePath.c_str() + mMakerSettings.mInputDirectory.length();

                // If the file is the patch's .eaPatchInfo or .eaPatchImpl files themselves, 
                // then we ignore them because they are patch description files and not patch content themselves.
                const char8_t* const kIgnoredFileExtensions[] = { kPatchDirectoryFileNameExtension, kPatchInfoFileNameExtension, kPatchImplFileNameExtension };
                const char8_t* const pFileExtension           = EA::IO::Path::GetFileExtension(sPathRelative.c_str());

                for(size_t e = 0; e < EAArrayCount(kIgnoredFileExtensions); e++)
                {
                    if(EA::StdC::Stricmp(pFileExtension, kIgnoredFileExtensions[e]) == 0)
                    {
                        FileInfoList::iterator itRemove = it--;   // Use -- here because it will cause the ++it above to do the right thing.
                        ignoredFileList.splice(ignoredFileList.end(), usedFileList, itRemove); // Move this node into ignoredFileList
                        break;
                    }
                }
            }

            // Go through the list of files we ignore.
            for(FileInfoList::const_iterator it = ignoredFileList.begin(); it != ignoredFileList.end(); ++it)
            {
                const FileInfo& fileInfo = *it;

                if(fileInfo.mbFile)
                    mIgnoredPatchFileList.push_back(fileInfo.mFilePath);
                else
                    mIgnoredPatchDirectoryList.push_back(fileInfo.mFilePath);
            }

            // Go through the list of files we don't ignore (i.e. files we add to the patch).
            for(FileInfoList::const_iterator it = usedFileList.begin(); it != usedFileList.end(); ++it)
            {
                const FileInfo& fileInfo = *it;
                PatchFileInfo   patchFileInfo;

                patchFileInfo.mbFile          = fileInfo.mbFile;
                patchFileInfo.mInputFilePath  = fileInfo.mFilePath;
              //patchFileInfo.mOutputFilePath = // Will be set later during the copying from mInputFilePath to the patch output directory.
                patchFileInfo.mFileSize       = fileInfo.mSize;
                patchFileInfo.mAccessRights.clear();  // To do: Figure out how we are going to let the user set the rights. Hopefully in a data-driven way or similar.
                patchFileInfo.mFileHashValue.Clear(); // We calculate the hash value for the file while we are copying the file to the patch directory and calculating its diff hash values.

                if(fileInfo.mbFile)
                    mPatchFileList.push_back(patchFileInfo);
                else
                    mPatchDirectoryList.push_back(patchFileInfo);
            }
        }
        else
        {
            mbSuccess = false;
            // mError was already set above.
        }
    }

    // At this point, if successful, we have a list of patch files+directories and a list of ignored files+directories.

    return mbSuccess;
}


bool Maker::CopyFileWhileCalculatingHashes(const char8_t* pFileSourceFullPath, const char8_t* pFileSourceBaseDirectory, const char8_t* pFileDestBaseDirectory, 
                                            DiffData& diffData, String& destRelativePath, String& destFullPath)
{
    uint64_t fileSize = 0;

    if(mbSuccess)
    {
        fileSize = EA::IO::File::GetSize(pFileSourceFullPath);

        if(fileSize == EA::IO::kSizeTypeError)
            HandleError(kECFatal, kSystemErrorIdNone, kEAPatchErrorIdFileNotFound, pFileSourceFullPath);
    }

    if(mbSuccess)
    {
        String sSourceFileName(pFileSourceFullPath + EA::StdC::Strlen(pFileSourceBaseDirectory));
        String sPreviousFilePath(mMakerSettings.mPreviousDirectory + sSourceFileName);
        bool   bPreviousFileExists(EA::IO::File::Exists(sPreviousFilePath.c_str()));   // Can't do optimization without a previous file.
        bool   bFilesAreIdentical = false;
        size_t bestDiffBlockSize  = kDiffBlockSizeMin;
        size_t bestDiffBlockShift = kDiffBlockShiftMin;

        if(mMakerSettings.mbEnableOptimizations && bPreviousFileExists && (fileSize > kDiffBlockSizeMin))
        {
            // We test all block sizes from kDiffBlockSizeMin to kDiffBlockSizeMax and see which one 
            // is the most optimal. Most optimal is defined as requiring the least amount of total 
            // transfer from the server when the current source is updated to this patch. Total transfer
            // from the server is the size of the .eaPatchDiff file plus the total volume of file segments
            // read from the server for this file transition. Calculating this optimal value can be slow
            // for large files.

            // Start out assuming that we'll be testing the full range of possible block sizes. 
            // We'll adjust this range based on the size of the files we are working with.
            size_t diffBlockSizeMin  = kDiffBlockSizeMin;
            size_t diffBlockShiftMin = kDiffBlockShiftMin;
            size_t diffBlockSizeMax  = kDiffBlockSizeMax;
            size_t diffBlockShiftMax = kDiffBlockShiftMax;
        
            // It's pointless to test a block size that's bigger than the file size, so cap diffBlockSizeMax.
            while((diffBlockSizeMax > fileSize) && 
                  (diffBlockSizeMax > kDiffBlockSizeMin))
            {
                diffBlockSizeMax  /= 2;
                diffBlockShiftMax -= 1;
            }

            FileReader fileReader;
            uint64_t   bestDownloadVolume = UINT64_MAX;  // Set it to a high value to start. When we are done, the best will be download volume that's the lowest.

            // This loop could take a while to complete if we are working on gigabyte+ sized files.
            for(size_t size = diffBlockSizeMin, shift = diffBlockShiftMin; (size <= diffBlockSizeMax) && mbSuccess; size *= 2, shift += 1)
            {
                fileReader.ReadFile(pFileSourceFullPath, true, true, size);

                if(fileReader.HasError())
                {
                    TransferError(fileReader);
                    break;
                }
                else
                {
                    // Get transfer volume for the patching of the original file to this patch file.
                    // The transfer volume is the size of the .eaPatchDiff file plus the sum of the blocks
                    // needed to convert the original file to the patch file. We ignore that in practice 
                    // on users machines that they might be patching an older file to this patch file and 
                    // thus the measuments aren't applicable to that patching.

                    DiffData diffDataTemp;
                    fileReader.GetFileSize(diffDataTemp.mFileSize);
                    fileReader.GetFileHash(diffDataTemp.mDiffFileHashValue);
                    fileReader.GetDiffHash(diffDataTemp.mBlockHashArray);
                    diffDataTemp.mBlockCount = diffDataTemp.mBlockHashArray.size();
                    diffDataTemp.mBlockSize  = size;

                    BuiltFileInfo  bfi;
                    bfi.mLocalFilePath = sPreviousFilePath; // We pretend that we are updating the previous file to a new file, and see what the resulting metrics are.

                    if(PatchBuilder::GenerateBuiltFileInfo(diffDataTemp, bfi, mError))
                    {
                        EA::IO::MemoryStream eaPatchDiffStream;
                        eaPatchDiffStream.AddRef();
                        eaPatchDiffStream.SetAllocator(EA::Patch::GetAllocator());
                        eaPatchDiffStream.SetOption(EA::IO::MemoryStream::kOptionResizeEnabled, 1);

                        DiffDataSerializer diffDataSerializer;
                        diffDataSerializer.Serialize(diffDataTemp, &eaPatchDiffStream);

                        if(diffDataSerializer.HasError())
                            TransferError(diffDataSerializer);
                        else
                        {
                            const uint64_t currentDownloadVolume = ((uint64_t)eaPatchDiffStream.GetSize() + bfi.mRemoteSpanCount);

                            if(currentDownloadVolume < bestDownloadVolume) // If this block size results in a lower download than the current low size champ...
                            {
                                bestDiffBlockSize  = size;
                                bestDiffBlockShift = shift;
                                bestDownloadVolume = currentDownloadVolume;
                            }

                            if(bfi.mRemoteSpanCount == 0) // If the files are identical
                            {
                                // If previous file and the source file are identical, then don't execute the 
                                // optimization code below, as it will result in the largest possible block size.
                                // It's probably better if we make the assumption that there will be users with 
                                // non-identical prev files, and their downloads will be non-optimal if the block
                                // size is the largest possible.
                                bFilesAreIdentical = true;
                            }
                        }
                    }
                    else
                    {
                        mbSuccess = false;
                        // mError was set above.
                    }

                    // mFileStreamTemp is unused in the code above. 
                    bfi.mFileStreamTemp.EnableErrorAssertions(false);
                }
            }

            if(bFilesAreIdentical)
            {
                // When the pre-patch and patch files are identical, the chosen best block size will always 
                // be the largest block size that's under the file size. That does in fact optimize the download
                // volume for that case, but we reduce the block size anyway. The reason is that the end user
                // may encounter a pre-patch image that's not the same as ours here, and that user will be 
                // punished with larger downloads because their image is not the same one we optimized against
                // here. So we implement a fudge factor here which reduces the block size somewhat.

                // We make sure it doesn't go below kDiffBlockSizeMin / kDiffBlockShiftMin.
                if(bestDiffBlockShift > 16)
                {
                    bestDiffBlockSize  /= 8;
                    bestDiffBlockShift -= 3;
                }
                else
                {
                    bestDiffBlockSize  /= 4;
                    bestDiffBlockShift -= 2;
                }

                if(bestDiffBlockSize < kDiffBlockSizeMin)
                    bestDiffBlockSize = kDiffBlockSizeMin;

                if(bestDiffBlockShift < kDiffBlockShiftMin)
                    bestDiffBlockShift = kDiffBlockShiftMin;
            }
        }
        else
        {
            // The table directly below is hard-coded to an assumption of kDiffBlockShiftMin / kDiffBlockShiftMax values.
            EA_COMPILETIME_ASSERT((kDiffBlockShiftMin == 10) && (kDiffBlockShiftMax == 20));

            const uint64_t sizeShiftTable[10] = 
            {
                 1048576,   // 10 / 1024        Files up to this size use a block shift of 10 (block size of 2014).
                 2097152,   // 11 / 2048
                 4194304,   // 12 / 4096
                 8388608,   // 13 / 8192
                16777216,   // 14 / 16384
                33554432,   // 15 / 32768
                67108864,   // 16 / 65536
               134217728,   // 17 / 131072
               268435456,   // 18 / 262144
               536870912    // 19 / 524288      Files up to this size use a block shift of 19 (block size of 524288).
                            // 20 / 1048576     Files beyond the above size use a block shift of 20.
            };

            // In this case we choose the block size based on the file size, or we use a block size the user told us to use.

            if(mMakerSettings.mBlockSize >= kDiffBlockSizeMin) // If the user specified a block size.
            {
                bestDiffBlockSize  = EA::StdC::RoundUpToPowerOf2(mMakerSettings.mBlockSize);
                bestDiffBlockShift = (size_t)EA::StdC::Log2((uint32_t)bestDiffBlockSize);
            }
            else
            {
                size_t i = 0;
                while((sizeShiftTable[i] < fileSize) && (i < EAArrayCount(sizeShiftTable))) // We could alternatively use an eastl::lower_bound() call.
                    ++i;

                bestDiffBlockSize  = ((size_t)1 << (10 + i));
                bestDiffBlockShift = (10 + i);
            }
        }

        EAPATCH_ASSERT(((size_t)1 << bestDiffBlockShift) == bestDiffBlockSize);

        // Now that we have the best diff block size, do the final actual diff file generation and patch file copy.
        EA::Patch::FileCopier fileCopier;

        //if(fileSize > (32 * 1024 * 1024)) // Debug code.
        //    Report("File size: %I64u, bestDiffBlockShift: %I64u, bestDiffBlockSize: %I64u.\n", (uint64_t)fileSize, (uint64_t)bestDiffBlockShift, (uint64_t)bestDiffBlockSize);
        fileCopier.CopyFile(pFileSourceFullPath, pFileSourceBaseDirectory, pFileDestBaseDirectory, true, true, bestDiffBlockSize);
        TransferError(fileCopier);

        fileCopier.GetFileSize(diffData.mFileSize);
        fileCopier.GetFileHash(diffData.mDiffFileHashValue);
        fileCopier.GetDiffHash(diffData.mBlockHashArray);
        fileCopier.GetDestPath(destRelativePath, destFullPath);
        diffData.mBlockCount = diffData.mBlockHashArray.size();
        diffData.mBlockSize  = bestDiffBlockSize;
    }

    return mbSuccess;
}


bool Maker::MakePatchImpl()
{
    if(mbSuccess)
    {
        // We start out by copying the user-supplied part of PatchImpl to the 
        // finished version. We'll write the rest of it ourselves.
        mPatchImpl = mMakerSettings.mPatchImpl;

        // If the user didn't specify a PatchInfo URL, we generate a default one ourselves.
        if(mPatchImpl.mPatchInfoURL.empty()) // If mPatchInfoURL is not specified...
        {
            if(mPatchImpl.mPatchImplURL.Count()) // If we can copy it from mPatchImplURL.
            {
                // Get the first URL in mPatchImplURL and copy it to mPatchInfoURL.
                mPatchImpl.mPatchInfoURL = mPatchImpl.mPatchImplURL.GetEnglishValue();
                eastl_size_t pos = mPatchImpl.mPatchInfoURL.rfind(kPatchImplFileNameExtension);
                if(pos != String::npos)
                    mPatchImpl.mPatchInfoURL.erase(pos);
                mPatchImpl.mPatchInfoURL += kPatchInfoFileNameExtension;
            }
            else
                HandleError(kECFatal, kSystemErrorIdNone, kEAPatchErrorIdURLError, "PatchImpl mPatchInfoURL not specified.");
        }

        // Do some URL encoding.
        URL::EncodeSpacesInURL(mPatchImpl.mPatchInfoURL.c_str(), mPatchImpl.mPatchInfoURL); // Fix this in case the user hasn't already.
        mPatchImpl.mPatchImplURL.URLEncodeStrings();
        mPatchImpl.mPatchDescription.URLEncodeStrings(); // Will encode only if it's a URL.

        // PatchImpl::PatchInfo values stay the same as the user supplied them.
        mPatchImpl.mPatchHash.clear();              // We'll write this in later near the end of the process.
      //mPatchImpl.mPreRunScript;                   // Keep whatever the user put here, if anything.
      //mPatchImpl.mPostRunScript;                  // Keep whatever the user put here, if anything.
      //mPatchImpl.mIgnoredFileArray;               // Keep whatever the user put here, if anything.
      //mPatchImpl.mUsedFileArray;                  // Keep whatever the user put here, if anything.
        mPatchImpl.mFinalFileSizeUsage = 0;         // This will be updated below.
        mPatchImpl.mFinalDiskSpaceUsage = 0;        // This will be updated below.
        mPatchImpl.mIntermediateDiskSpaceUsage = 0; // This will be updated below.

        // We need to set other values below after we've processed the files

        String sRemoteURLBase;      // e.g. http://patch.ea.com:1234/Basebrawl_2014/Patches/ All the URLs we create in the eaPatchImpl file will be with this as the root.
        String sDestRelativePath;   // e.g. Data/GameData.big

        // Setup sRemoteURLBase
        // Calculate the URL for the patch directory. We base this off the URL specified by
        // the user in PatchImpl::mPatchImplURL. As it currently stands, PatchImpl.mPatchImplURL is 
        // not a single string, but a LocalizedString (which is a set strings, one per locale).
        // We don't currently have a great way of building a patch against multiple locales and may
        // have to revisit this. For the time being we assume there is just one locale and use
        // the first one present in PatchImpl.mPatchImplURL.
        
        if(mPatchImpl.mPatchImplURL.Count())
            sRemoteURLBase = mPatchImpl.mPatchImplURL.GetEnglishValue();
        else
            HandleError(kECFatal, kSystemErrorIdNone, kEAPatchErrorIdURLError, "PatchImpl mPatchInfoURL not specified.");

        // e.g. http://patch.ea.com:1234/Basebrawl_2014/Patches/Patch1.eaPatchImpl ->
        //      http://patch.ea.com:1234/Basebrawl_2014/Patches/
        //
        // e.g. Basebrawl_2014/Patches/Patch1.eaPatchImpl ->
        //      http://patch.ea.com:1234/Basebrawl_2014/Patches/
        EA::Patch::StripFileNameFromURLPath(sRemoteURLBase);

        if(mbSuccess)
        {
            ReportVerbosity(kReportVerbosityRemark, "%s\n", "Processing input files...");

            mMakerSettings.mPatchImpl.mPatchEntryArray.clear();

            for(PatchFileInfoList::iterator it = mPatchFileList.begin(); (it != mPatchFileList.end()) && mbSuccess; ++it)
            {   
                PatchFileInfo& patchFileInfo = *it;

                // For each file in the list of patch files, add a PatchEntry to PatchEntryArray.
                PatchEntry& patchEntry = mPatchImpl.mPatchEntryArray.push_back();

                ReportVerbosity(kReportVerbosityRemark, "Processing file: %s\n", patchFileInfo.mInputFilePath.c_str());

                if(CopyFileWhileCalculatingHashes(patchFileInfo.mInputFilePath.c_str(), mMakerSettings.mInputDirectory.c_str(), mMakerSettings.mOutputDirectory.c_str(), 
                                                    patchEntry.mDiffData, sDestRelativePath, patchFileInfo.mOutputFilePath))
                {
                    patchFileInfo.mBlockSize = patchEntry.mDiffData.mBlockSize;         // Record this for reporting back to the user when we print a report.

                    patchEntry.mbFile             = patchFileInfo.mbFile;
                    patchEntry.mbForceOverwrite   = false;                              // We don't currently have a means for the user to specify this for files.
                    patchEntry.mbFileUnchanged    = false;                              // We don't currently have a means for the user to specify this for files.
                    patchEntry.mFileURL           = sRemoteURLBase + sDestRelativePath; // e.g. "http://patch.ea.com:1234/Basebrawl_2014/Patches/Data/GameData.big"
                    URL::EncodeSpacesInURL(patchEntry.mFileURL.c_str(), patchEntry.mFileURL);
                    patchEntry.mRelativePath      = sDestRelativePath;                  // e.g. "Data/GameData.big"
                    patchEntry.mAccessRights      = patchFileInfo.mAccessRights;        // We don't currently have a means for the user to specify this for files.
                    patchEntry.mFileSize          = patchFileInfo.mFileSize;
                    patchEntry.mFileHashValue     = patchEntry.mDiffData.mDiffFileHashValue;
                  //patchEntry.mLocaleArray                                             // We don't currently have a means for the user to specify per-file locale support.
                    patchEntry.mDiffFileURL       = patchEntry.mFileURL + kPatchDiffFileNameExtension;
                    URL::EncodeSpacesInURL(patchEntry.mDiffFileURL.c_str(), patchEntry.mDiffFileURL);
                  //patchEntry.mDiffData                                                // This was already written during CopyFileWhileCalculatingHashes above.

                    mPatchImpl.mFinalFileSizeUsage         += patchEntry.mFileSize;
                    mPatchImpl.mFinalDiskSpaceUsage        += EA::Patch::EstimateFileDiskSpaceUsage(patchEntry.mFileSize, mPatchImpl.mOSNameArray);
                    mPatchImpl.mIntermediateDiskSpaceUsage += mPatchImpl.mFinalDiskSpaceUsage + (patchEntry.mDiffData.mBlockCount * sizeof(DiffData)) + 1024;
                    // To do: This mIntermediateDiskSpaceUsage .eaPatchDiff usage calculation here needs to 
                    // take into account disk space and not bytes, and it needs to take into account
                    // the size of the .eaPatchInfo, .eaPatchImpl, .eaPatchDone files. Plus probably some extra.
                }
                // else mbSuccess will have been set to false and we'll exit this loop.
            }
        }
    }

    return mbSuccess;
}


bool Maker::WritePatchImpl()
{
    ReportVerbosity(kReportVerbosityRemark, "%s\n", "Generating patch files...");

    // Write the .eaPatchInfo file.
    if(mbSuccess)
    {
        // For the code here, recall that PatchInfo is a base class of PatchImpl.
        PatchInfoSerializer patchInfoSerializer;

        mPatchInfoFilePath.sprintf("%s%s%s", mMakerSettings.mOutputDirectory.c_str(), mPatchImpl.mPatchId.c_str(), kPatchInfoFileNameExtension);

        // Assert that the file name length isn't too long for the target platform(s) file system(s).
        const size_t platformMaxFileNameLength = GetPlatformMaxFileNameLength(mMakerSettings.mPatchImpl.mOSNameArray);

        if(mPatchInfoFilePath.length() < platformMaxFileNameLength)
        {
            if(!patchInfoSerializer.Serialize(mPatchImpl, mPatchInfoFilePath.c_str(), false)) // This writes the .eaPatchInfo file. Recall that the PatchImpl contains the PatchInfo.
                TransferError(patchInfoSerializer); // This will set mbSuccess to false.
        }
        else
            HandleError(kECFatal, kSystemErrorIdNone, kEAPatchErrorIdFileNameLength, mPatchInfoFilePath.c_str());
    }

    // Write the .eaPatchImpl file and .eaPatchDiff files.
    if(mbSuccess)
    {
        PatchImplSerializer patchImplSerializer;

        mPatchImplFilePath.sprintf("%s%s%s", mMakerSettings.mOutputDirectory.c_str(), mPatchImpl.mPatchId.c_str(), kPatchImplFileNameExtension);

        // Assert that the file name length isn't too long for the target platform(s) file system(s).
        const size_t platformMaxFileNameLength = GetPlatformMaxFileNameLength(mMakerSettings.mPatchImpl.mOSNameArray);

        if(mPatchImplFilePath.length() < platformMaxFileNameLength)
        {
            if(!patchImplSerializer.Serialize(mPatchImpl, mPatchImplFilePath.c_str(), false)) // This writes the .eaPatchImpl file as well as the .eaPatchDiff files. 
                TransferError(patchImplSerializer); // This will set mbSuccess to false.
        }
        else
            HandleError(kECFatal, kSystemErrorIdNone, kEAPatchErrorIdFileNameLength, mPatchImplFilePath.c_str());
    }

    return mbSuccess;
}


bool Maker::VerifyPatch()
{
    // To do.
    // Make sure all the destination patch files are present.
    // Read back the MD5 values of all the destination patch files and verify them.
    return mbSuccess;
}


void Maker::PrintUserReport()
{
    if(mbSuccess)
    {
        Report("Successfully created patch \"%s\".\n", mPatchImpl.mPatchName.GetEnglishValue());
        Report("Source input directory: %s\n", mMakerSettings.mInputDirectory.c_str());
        Report("Patch output directory: %s\n", mMakerSettings.mOutputDirectory.c_str());
        Report("Previous directory: %s\n", mMakerSettings.mPreviousDirectory.empty() ? "(unspecified)" : mMakerSettings.mPreviousDirectory.c_str());
        Report("Patch info file: %s\n", mPatchInfoFilePath.c_str());
        Report("Patch info URL: %s\n", mPatchImpl.mPatchInfoURL.c_str());
        Report("Patch impl file: %s\n", mPatchImplFilePath.c_str());
        Report("Patch impl URL: %s\n", mPatchImpl.mPatchImplURL.GetEnglishValue());
        Report("Patch file size sum: %I64u.\n", mPatchImpl.mFinalFileSizeUsage);
        Report("Patch disk space usage: %I64u.\n", mPatchImpl.mFinalDiskSpaceUsage);
        Report("Patch intermediate disk space requirement: %I64u.\n", mPatchImpl.mIntermediateDiskSpaceUsage);

        if(mIgnoredPatchDirectoryList.empty()) // If there were no directories found which were to be ignored...
        {
            if(mMakerSettings.mPatchImpl.mIgnoredFileArray.empty())
                Report("Ignored directories: none specified.\n");
            else
                Report("Ignored files or directories were specified, but no such directories were found in the input directory.\n");
        }
        else
        {
            Report("Ignored directories/files: %s\n", mIgnoredPatchDirectoryList.empty() && mIgnoredPatchFileList.empty() ? "none" : "");
            for(StringList::iterator it = mIgnoredPatchDirectoryList.begin(); it != mIgnoredPatchDirectoryList.end(); ++it)
            {
                const String& sDirectoryPath = *it;

                Report("    %s %s\n", sDirectoryPath.c_str(), EA::IO::Directory::Exists(sDirectoryPath.c_str()) ? "" : "(not actually present)");
            }
            for(StringList::iterator it = mIgnoredPatchFileList.begin(); it != mIgnoredPatchFileList.end(); ++it)
            {
                const String& sFilePath = *it;

                Report("    %s %s\n", sFilePath.c_str(), EA::IO::File::Exists(sFilePath.c_str()) ? "" : "(not actually present)");
            }
        }

        if(mPatchFileList.empty())
        {
            Report("\n");
            Report("Patch files:\n");

            if(mMakerSettings.mPatchImpl.mUsedFileArray.empty())
                Report("None specified.\n");
            else
                Report("Used files or directories were specified, but no such files were found in the input directory.\n");
        }
        else
        {
            Report("\n");
            Report("Patch files:\n");
            Report("         size | block size | path\n");
            Report("   -------------------------------------------------------------------------------------------\n");

            for(PatchFileInfoList::iterator it = mPatchFileList.begin(); it != mPatchFileList.end(); ++it)
            {
                const PatchFileInfo& patchFileInfo = *it;

                Report("   %10I64u   %10I64u   %s\n", patchFileInfo.mFileSize, patchFileInfo.mBlockSize, patchFileInfo.mOutputFilePath.c_str());
            }
        }

        PrintUserInstructions();
    }
    else
    {
        Report("Failure creating patch.\n");
        // The reason for the failure should already have been printed as part 
        // of the earlier processing before we got here.

        // To consider: Provide an option to pause here and let the user  
        // inspect the output directory before we oossibly delete it.
    }
}


void Maker::PrintUserInstructions()
{
    Report("\n");
    Report("Instructions:\n");

    Report("   It's important that you test your resulting patch, especially if you changed\n");
    Report("   the file filters from the last time you built a patch like this. This is because\n");
    Report("   mis-implemented file filters can potentially result in deleted files and \n");
    Report("   directories that you didn't intend during patch application.\n");

    Report("   The output directory's contents represent the patch. These contents include\n");
    Report("   files from the source directory, as well as .eaPatchInfo, .eaPatchImpl, and \n");
    Report("   .eaPatchDiff files. All are part of the patch and there are no unused files.\n");

    Report("   \n");
    Report("   You can test the patch with the EAPatchServer applet provided with \n");
    Report("   the EAPatch package. You can run EAPatchServer with -? to get help on how to\n");
    Report("   test with it. Alternatively you can just about any regular web server, such\n");
    Report("   as Apache.\n");

    Report("   \n");
    Report("   If you are using a patch directory (list of available patches presented\n");
    Report("   to the user) then you will want to add a line for this patch to the patch\n");
    Report("   directory file on the server. A patch directory is simply a single file which\n");
    Report("   is a list of URLs (one per line) to .eaPatchInfo files on the server.\n");

    Report("   \n");
    Report("   The .eaPatchInfo and .eaPatchImpl files have some fields that can be\n");
    Report("   manually edited before being published. The .eaPatchInfo file can can be\n");
    Report("   placed in alternative location as long as the .eaPatchImpl(s) URL\n");
    Report("   specified in it point to the patch directory on the server.\n");

    if(mMakerSettings.mPreviousDirectory.empty())
    {
        Report("\n");
        Report("   No previous directory was specified. Specifying a previous directory \n");
        Report("   allows for optimization of the expected download bandwidth for most users.\n");
        Report("   The previous directory refers to the patched directory contents before the\n");
        Report("   patch is applied on a user's machine. It's OK if this patched directory \n");
        Report("   varies between users; just specify a version of it which is likely to be\n");
        Report("   most often encountered. If the patch is the first patch to an application's\n");
        Report("   data, the previous directory would be the original shipping version of\n");
        Report("   that directory. Potentially megabytes of bandwidth can be saved by \n");
        Report("   providing this.\n");
    }

    Report("   \n");
    Report("   See the EAPatch documentation for more information about patch making, testing,\n");
    Report("   and deploying. Or contact EATech Core Support by email for help and bug reports.\n");
    Report("   \n");
}


void Maker::Cleanup()
{
    if(!mbSuccess)
    {
        if(mMakerSettings.mbEnableErrorCleanup)
        {
            const bool removeResult = EA::IO::Directory::Remove(mMakerSettings.mOutputDirectory.c_str(), true);

            if(!removeResult)
                ReportVerbosity(kReportVerbosityError, "Unable to remove output directory \"%s\".\n", mMakerSettings.mOutputDirectory.c_str());
        }
    }
}



} // namespace Patch
} // namespace EA







