///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/PatchImpl.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/Config.h>
#include <EAPatchClient/PatchImpl.h>
#include <EAPatchClient/File.h>
#include <EAPatchClient/Debug.h>
#include <EAPatchClient/XML.h>
#include <EAPatchClient/URL.h>
#include <EAIO/EAFileStream.h>
#include <EAIO/EAStreamAdapter.h>
#include <EAIO/PathString.h>
#include <EAStdC/EABitTricks.h>
#include <EAStdC/EAEndian.h>
#include <EAStdC/EAString.h>


namespace EA
{
namespace Patch
{


/////////////////////////////////////////////////////////////////////////////
// DiffData
/////////////////////////////////////////////////////////////////////////////

const uint64_t kDiffDataVersionFlag = UINT64_C(0x1000000000000000);
const uint64_t kDiffDataVersion     = kDiffDataVersionFlag | 1; // High bit set so that we can distinguish this struct from a previous version of DiffData that didn't have an mVersion field.
const uint64_t kMinDiffBlockSize    = 256;
const uint64_t kMaxDiffBlockSize    = 16 * 1024 * 1024;


DiffData::DiffData()
  : mVersion(kDiffDataVersion)
  , mFileSize(0)
  , mDiffFileHashValue()
  , mBlockCount(0)
  , mBlockSize(0)
  , mBlockHashArray()
{
}


void DiffData::Reset()
{
    // Leave mVersion alone.
    mFileSize = 0;
    mDiffFileHashValue.Clear();
    mBlockCount = 0;
    mBlockSize = 0;
    mBlockHashArray.set_capacity(0);
}



///////////////////////////////////////////////////////////////////////////////
// DiffDataSerializer
///////////////////////////////////////////////////////////////////////////////

bool DiffDataSerializer::Serialize(const DiffData& diffData, EA::IO::IStream* pStream)
{
    // This serialization function is writing binary data, unlike other serialization 
    // functions in this library which write XML. DiffData is uncompressible, as it 
    // consists of MD5 hash data which is random. So there's no point in trying to 
    // compress this file.

    EAPATCH_ASSERT(pStream);
    mpStream = pStream;

    EAPATCH_ASSERT(diffData.mBlockCount == diffData.mBlockHashArray.size());

    // The following will maintain mbSuccess as they go.
    WriteUint64(diffData.mVersion);
    WriteUint64(diffData.mFileSize);
    WriteUint8Array(diffData.mDiffFileHashValue.mData, EAArrayCount(diffData.mDiffFileHashValue.mData));
    WriteUint64(diffData.mBlockCount);
    WriteUint64(diffData.mBlockSize);
    for(eastl_size_t i = 0, iEnd = diffData.mBlockHashArray.size(); i != iEnd; i++)
    {
        WriteUint32(diffData.mBlockHashArray[i].mChecksum);
        WriteUint8Array(diffData.mBlockHashArray[i].mHashValue.mData, EAArrayCount(diffData.mBlockHashArray[i].mHashValue.mData));
    }

    if(!mbSuccess)
    {
        uint32_t state;
        String   sName;
        uint32_t streamType = GetStreamInfo(pStream, state, sName);
    
        EA_UNUSED(streamType);
        HandleError(kECBlocking, (SystemErrorId)state, kEAPatchErrorIdFileWriteFailure, sName.c_str());
    }

    return mbSuccess;
}


bool DiffDataSerializer::Deserialize(DiffData& diffData, const char8_t* pFilePath)
{
    EAPATCH_ASSERT(EA::StdC::Strcmp(EA::IO::Path::GetFileExtension(pFilePath), kPatchDiffFileNameExtension) == 0);

    if(mbSuccess)
    {
        File file;

        mbSuccess = file.Open(pFilePath, EA::IO::kAccessFlagRead, EA::IO::kCDOpenExisting, 
                                EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential);
        if(mbSuccess)
        {
            Deserialize(diffData, file.GetStream());
            file.Close();
        }

        if(file.HasError())         // This will catch errors from file.Open or file.Close above.
            TransferError(file);    // Copy the error state from file to us.
    }

    return mbSuccess;
}


bool DiffDataSerializer::Deserialize(DiffData& diffData, EA::IO::IStream* pStream)
{
    // This serialization function is reading binary data, unlike other serialization 
    // functions in this library which use XML. 

    EAPATCH_ASSERT(pStream);
    mpStream = pStream;

    // The following will maintain mbSuccess as they go.
    ReadUint64(diffData.mVersion);
    if(diffData.mVersion < kDiffDataVersionFlag) // If the DiffData version is prior to when DiffData had an mVersion field. (we assume that no file could possibly be so big as this). (Temporary backwards compatibility code) 
    {
        diffData.mFileSize = diffData.mVersion;
        diffData.mVersion  = kDiffDataVersion;
    }
    else
        ReadUint64(diffData.mFileSize);
    ReadUint8Array(diffData.mDiffFileHashValue.mData, EAArrayCount(diffData.mDiffFileHashValue.mData));
    ReadUint64(diffData.mBlockCount);
    ReadUint64(diffData.mBlockSize);

    // We do some sanity checks here before proceeding.
    if(mbSuccess)
    {
        // To do: Check the mDiffFileHashValue, at least in debug builds.
        EAPATCH_ASSERT((((int64_t)diffData.mBlockSize * ((int64_t)diffData.mBlockCount - 1)) < (int64_t)diffData.mFileSize) &&  // cast to int64_t because mBlockCount might be 0 and result in some negative values.
                        ((int64_t)diffData.mFileSize <= ((int64_t)diffData.mBlockSize * (int64_t)diffData.mBlockCount))); 

        mbSuccess = EA::StdC::IsPowerOf2(diffData.mBlockSize) && (diffData.mBlockSize >= kMinDiffBlockSize) && 
                                                                 (diffData.mBlockSize <= kMaxDiffBlockSize);
        if(!mbSuccess)
        {
            // To do: Set Error to kEAPatchErrorIdFileCorrupt and make it so it doesn't get overwritten below.
            // This may require revising the ErrorBase::HandleError function to not set the EAPatchErrorId 
            // if it was already set. Actually the Error::SetErrorId function.
        }
    }

    if(mbSuccess)
    {
        // Here we do manually do some stream reading and endian swapping, as it's faster for 
        // us to do it this way than to do a byte-for-byte read and swap through potentially 
        // megabytes of data.
        diffData.mBlockHashArray.resize((eastl_size_t)diffData.mBlockCount); // This is a 64 to 32 bit cast on 32 bit systems, but it's OK because mBlockCount could never be so high on such systems.

        const EA::IO::size_type readSize = (EA::IO::size_type)(diffData.mBlockCount * sizeof(BlockHash));
        mbSuccess = (pStream->Read(&diffData.mBlockHashArray[0], readSize) == readSize);

        if(mbSuccess)
        {
            for(eastl_size_t i = 0, iEnd = diffData.mBlockHashArray.size(); i != iEnd; i++)
            {
                EA_COMPILETIME_ASSERT(EAPATCH_ENDIAN == EA::IO::kEndianLittle);
                diffData.mBlockHashArray[i].mChecksum = EA::StdC::FromLittleEndian(diffData.mBlockHashArray[i].mChecksum);
            }
        }
    }

    if(!mbSuccess)
    {
        uint32_t state;
        String   sName;
        uint32_t streamType = GetStreamInfo(pStream, state, sName);

        EA_UNUSED(streamType);
        HandleError(kECBlocking, (SystemErrorId)state, kEAPatchErrorIdFileReadFailure, sName.c_str());
    }

    return mbSuccess;
}



/////////////////////////////////////////////////////////////////////////////
// PatchEntry
/////////////////////////////////////////////////////////////////////////////

PatchEntry::PatchEntry()
  : mbFile()
  , mbForceOverwrite()
  , mbFileUnchanged()
  , mFileURL()
  , mRelativePath()
  , mAccessRights()
  , mFileSize()
  , mFileHashValue()
  , mLocaleArray()
  , mDiffFileURL()
  , mDiffData()
{
}



/////////////////////////////////////////////////////////////////////////////
// PatchImpl
/////////////////////////////////////////////////////////////////////////////

PatchImpl::PatchImpl()
  : mPatchHash(),
    mPreRunScript(),
    mPostRunScript(),
    mIgnoredFileArray(),
    mUsedFileArray(),
    mPatchEntryArray()
{
}


PatchImpl::~PatchImpl()
{
}


void PatchImpl::Reset()
{
    PatchInfo::Reset();

    mPatchHash.set_capacity(0);             // This causes the memory to be freed while also clearing the container.
    mPreRunScript.set_capacity(0); 
    mPostRunScript.set_capacity(0); 
    mIgnoredFileArray.set_capacity(0);
    mUsedFileArray.set_capacity(0);
    mPatchEntryArray.set_capacity(0);
}


///////////////////////////////////////////////////////////////////////////////
// PatchImplSerializer
///////////////////////////////////////////////////////////////////////////////

bool PatchImplSerializer::Serialize(const PatchImpl& patchImpl, const char8_t* pFilePath, bool bAddHelpComments)
{
    // To consider: Enable this:
    //EAPATCH_ASSERT(EA::StdC::Strcmp(EA::IO::Path::GetFileExtension(pFilePath), kPatchImplFileNameExtension) == 0 || 
    //               EA::StdC::Strcmp(EA::IO::Path::GetFileExtension(pFilePath), kPatchTempFileNameExtension) == 0);

    // We are writing the PatchImpl to disk as an XML file.
    // We do the writing in two steps:
    //    First we write the .eaPatchImpl XML file.
    //    Then we write out the individual .eaPatchDiff binary files.
    File file;

    // Write the .eaPatchImpl file
    if(mbSuccess)
    {
        mbSuccess = file.Open(pFilePath, EA::IO::kAccessFlagWrite, EA::IO::kCDCreateAlways, 
                                EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential);
        if(mbSuccess)
        {
            Serialize(patchImpl, file.GetStream(), bAddHelpComments);
            file.Close();
        }

        if(file.HasError())         // This will catch errors from file.Open or file.Close above.
            TransferError(file);    // Copy the error state from file to us.
    }

    // Write the .eaPatchDiff files
    if(mbSuccess) // If the writing of the .eaPatchImpl file succeeded... write out all the individual .eaPatchDiff files.
    {
        // Possible future optization: Instead of writing out individual .eaPatchDiff files, combine them 
        // into a single file, for more reliability, though at the likely cost of more complexity.
        DiffDataSerializer diffDataSerializer;
        String             sFilePath;

        String sFilePathBaseDirectory(pFilePath);                     // e.g. /Root/Basebrawl_2014/Patches/Patch1.eaPatchImpl
        EA::Patch::StripFileNameFromFilePath(sFilePathBaseDirectory); // e.g. /Root/Basebrawl_2014/Patches/

        for(eastl_size_t i = 0, iEnd = patchImpl.mPatchEntryArray.size(); (i != iEnd) && mbSuccess; i++)
        {
            const PatchEntry& patchEntry = patchImpl.mPatchEntryArray[i];

            sFilePath  = sFilePathBaseDirectory;         // e.g. /Root/Basebrawl_2014/Patches/
            sFilePath += patchEntry.mRelativePath;  // e.g. /Root/Basebrawl_2014/Patches/Data/Game.big
            sFilePath += kPatchDiffFileNameExtension;    // e.g. /Root/Basebrawl_2014/Patches/Data/Game.big.eaPatchDiff

            mbSuccess = file.Open(sFilePath.c_str(), EA::IO::kAccessFlagWrite, EA::IO::kCDCreateAlways, 
                                    EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential);
            if(mbSuccess)
            {
                mbSuccess = diffDataSerializer.Serialize(patchEntry.mDiffData, file.GetStream());
                file.Close();
            }

            if(file.HasError())         // This will catch errors from file.Open or file.Close above.
                TransferError(file);    // Copy the error state from file to us.
        }

        if(diffDataSerializer.HasError())       // If there was an error above during the writing of .eaPatchDiff files...
            TransferError(diffDataSerializer);  // Copy the error state from diffDataSerializer to us.
    }

    return mbSuccess;
}


// We are writing an XML file that looks like this:
// <PatchImpl>
//     <PatchInfoURL>http://patch.ea.com/Madden_2014/TeamUpdates/Patch1.eaPatchInfo</PatchInfoURL>
//     <PatchId>1234-3e21-4352-3893</PatchId>
//     <PatchClass>patch</PatchClass>
//     . . .
//     <IgnoredFileArray></IgnoredFileArray>
//     <PatchEntry>
//         <File>true</File>
//         <FileURL>http://abc/def.big</FileURL>
//         <DiffFileURL>http://abc/def.big.eaPatchDiff</DiffFileURL>
//     </PatchEntry>
//     <PatchEntry>
//         <File>true</File>
//         <FileURL>http://xyz/pdq.big</FileURL>
//         <DiffFileURL>http://xyz/pdq.big.eaPatchDiff</DiffFileURL>
//     </PatchEntry>
//     . . .
// </PatchImpl>

bool PatchImplSerializer::Serialize(const PatchImpl& patchImpl, EA::IO::IStream* pStreamImpl, bool bAddHelpComments)
{
    XMLWriter writer;

    // To do: Implement bAddHelpComments.

    writer.BeginDocument(pStreamImpl);
      writer.BeginParent("PatchImpl");
        PatchInfoSerializer::SerializePatchInfoContents(patchImpl, writer, bAddHelpComments); // Serialize the parent PatchInfo class.

        writer.WriteValue("PatchHash",                       patchImpl.mPatchHash);
        writer.WriteValue("PreRunScript",                    patchImpl.mPreRunScript);
        writer.WriteValue("PostRunScript",                   patchImpl.mPostRunScript);
        writer.WriteValue("IgnoredFileArray",                patchImpl.mIgnoredFileArray);
        writer.WriteValue("UsedFileArray",                   patchImpl.mUsedFileArray);

        for(eastl_size_t i = 0, iEnd = patchImpl.mPatchEntryArray.size(); i != iEnd; i++)
        {
            const PatchEntry& patchEntry = patchImpl.mPatchEntryArray[i];

            writer.BeginParent("PatchEntry");
              writer.WriteValue("File",               patchEntry.mbFile);
              writer.WriteValue("ForceOverwrite",     patchEntry.mbForceOverwrite);
              writer.WriteValue("FileUnchanged",      patchEntry.mbFileUnchanged);
              writer.WriteValue("FileURL",            patchEntry.mFileURL);
              writer.WriteValue("RelativePath",       patchEntry.mRelativePath);
              writer.WriteValue("AccessRights",       patchEntry.mAccessRights);
              writer.WriteValue("FileSize",           patchEntry.mFileSize, false);
              writer.WriteValue("FileHashValue",      patchEntry.mFileHashValue);
              writer.WriteValue("LocaleArray",        patchEntry.mLocaleArray);
              writer.WriteValue("DiffFileURL",        patchEntry.mDiffFileURL);
              // patchImpl.mDiffData is written as a separate binary file.
            writer.EndParent("PatchEntry");
        }
      writer.EndParent("PatchImpl");
    writer.EndDocument();

    TransferError(writer);

    return mbSuccess;
}


bool PatchImplSerializer::Deserialize(PatchImpl& patchImpl, const char8_t* pFilePath, bool bMakerMode, bool bConvertPathsToLocalForm)
{
    EAPATCH_ASSERT(EA::StdC::Strcmp(EA::IO::Path::GetFileExtension(pFilePath), kPatchImplFileNameExtension) == 0);

    if(mbSuccess)
    {
        File file;

        mbSuccess = file.Open(pFilePath, EA::IO::kAccessFlagRead, EA::IO::kCDOpenExisting, 
                                EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential);
        if(mbSuccess)
        {
            Deserialize(patchImpl, file.GetStream(), bMakerMode, bConvertPathsToLocalForm);
            file.Close();
        }

        if(file.HasError())         // This will catch errors from file.Open or file.Close above.
            TransferError(file);    // Copy the error state from file to us.
    }

    return mbSuccess;
}


bool PatchImplSerializer::Deserialize(PatchImpl& patchImpl, EA::IO::IStream* pStreamImpl, bool bMakerMode, bool bConvertPathsToLocalForm)
{
    // We are read a PatchImpl struct from an XML stream (usually a disk file).
    // We don't deserialize the PatchImpl PatchEntryArray .eaPatchDiff files, as they are separate 
    // files which are deserialized individually during patch execution upon their download.
    EA_UNUSED(bMakerMode); // It's used in debug builds alone below.

    XMLReader reader;
    bool      bFound;

    reader.BeginDocument(pStreamImpl);
      reader.BeginParent("PatchImpl", bFound); EAPATCH_ASSERT(bFound);
        PatchInfoSerializer::DeserializePatchInfoContents(patchImpl, reader, bFound, bConvertPathsToLocalForm); // Serialize the parent PatchInfo class.

        reader.ReadValue("PatchHash",               bFound, patchImpl.mPatchHash);                EAPATCH_ASSERT(bFound || bMakerMode);
        reader.ReadValue("PreRunScript",            bFound, patchImpl.mPreRunScript);             EAPATCH_ASSERT(bFound || bMakerMode);
        reader.ReadValue("PostRunScript",           bFound, patchImpl.mPostRunScript);            EAPATCH_ASSERT(bFound || bMakerMode);
        reader.ReadValue("IgnoredFileArray",        bFound, patchImpl.mIgnoredFileArray, 0);    //EAPATCH_ASSERT(bFound || bMakerMode);
        reader.ReadValue("UsedFileArray",           bFound, patchImpl.mUsedFileArray, 0);       //EAPATCH_ASSERT(bFound || bMakerMode);

        // Read each <PatchEntry> that's present.
        // We don't actually require that there be any PatchEntries in a patch, 
        // as a patch may in fact consist of no files and only executed scripts.
        reader.BeginParent("PatchEntry", bFound); 
        while(bFound)
        {
            PatchEntry patchEntry;

            reader.ReadValue("File",               bFound, patchEntry.mbFile);              EAPATCH_ASSERT(bFound);
            reader.ReadValue("ForceOverwrite",     bFound, patchEntry.mbForceOverwrite);    EAPATCH_ASSERT(bFound);
            reader.ReadValue("FileUnchanged",      bFound, patchEntry.mbFileUnchanged);     EAPATCH_ASSERT(bFound);
            reader.ReadValue("FileURL",            bFound, patchEntry.mFileURL);            EAPATCH_ASSERT(bFound);
            reader.ReadValue("RelativePath",       bFound, patchEntry.mRelativePath);       EAPATCH_ASSERT(bFound);
            reader.ReadValue("AccessRights",       bFound, patchEntry.mAccessRights);       EAPATCH_ASSERT(bFound);
            reader.ReadValue("FileSize",           bFound, patchEntry.mFileSize);           EAPATCH_ASSERT(bFound);
            reader.ReadValue("FileHashValue",      bFound, patchEntry.mFileHashValue);      EAPATCH_ASSERT(bFound);
            reader.ReadValue("LocaleArray",        bFound, patchEntry.mLocaleArray, 0);     EAPATCH_ASSERT(bFound);
            reader.ReadValue("DiffFileURL",        bFound, patchEntry.mDiffFileURL);        EAPATCH_ASSERT(bFound);
            // patchImpl.mDiffData is read as a separate binary file later.
            patchImpl.mPatchEntryArray.push_back(patchEntry);

            reader.EndParent("PatchEntry");
            reader.BeginNextParent("PatchEntry", bFound);
        }

      reader.EndParent("PatchImpl");
    reader.EndDocument();

    if(bConvertPathsToLocalForm)
    {
        // We canonicalize paths by converting to and from EAIO PathStrings. This is a little 
        // dumb, but EAIO doesn't provide a way to do it in place on a path char array.
        EA::IO::Path::PathString8 sTemp;

        for(eastl_size_t i = 0, iEnd = patchImpl.mIgnoredFileArray.size(); i != iEnd; ++i)
        {
            FileFilter& fileFilter = patchImpl.mIgnoredFileArray[i];
            CanonicalizeFilePath(fileFilter.mFilterSpec);
        }

        for(eastl_size_t i = 0, iEnd = patchImpl.mUsedFileArray.size(); i != iEnd; ++i)
        {
            FileFilter& fileFilter = patchImpl.mUsedFileArray[i];
            CanonicalizeFilePath(fileFilter.mFilterSpec);
        }

        for(eastl_size_t i = 0, iEnd = patchImpl.mPatchEntryArray.size(); i != iEnd; ++i)
        {
            PatchEntry& patchEntry = patchImpl.mPatchEntryArray[i];
            CanonicalizeFilePath(patchEntry.mRelativePath);
        }
    }

    TransferError(reader);

    return mbSuccess;
}



} // namespace Patch
} // namespace EA



