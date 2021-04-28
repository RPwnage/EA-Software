///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/PatchInfo.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/Config.h>
#include <EAPatchClient/PatchInfo.h>
#include <EAPatchClient/Debug.h>
#include <EAPatchClient/XML.h>
#include <EAStdC/EAString.h>
#include <EAIO/EAFileStream.h>
#include <EAIO/PathString.h>


namespace EA
{
namespace Patch
{


PatchInfo::PatchInfo()
  : mPatchInfoURL()
  , mPatchId()
  , mPatchClass()
  , mRequired()
  , mPatchImplURL()
  , mContentIdArray()
  , mSKUIdArray()
  , mPatchName()
  , mPatchDescription()
  , mPatchEULA()
  , mAppId()
  , mOSNameArray()
  , mMinOSVersionArray()
  , mReleaseDate()
  , mLocaleSupport()
  , mSupercedentPatchIdArray()
  , mSupercedingPatchIdArray()
  , mDependentPatchIdArray()
  , mPatchBaseDirectory()
  , mUserInfo()
  , mUserChecksum()
  , mFinalFileSizeUsage()
  , mFinalDiskSpaceUsage()
  , mIntermediateDiskSpaceUsage()
  // Protected members
  , mPatchDescriptionHTMLContent()
{
}

PatchInfo::~PatchInfo()
{
}


void PatchInfo::Reset()
{
    ErrorBase::Reset();

    mPatchInfoURL.set_capacity(0);          // This causes the memory to be freed while also clearing the container.
    mPatchId.set_capacity(0);
    mPatchClass.set_capacity(0);
    mPatchImplURL.Clear();
    mContentIdArray.set_capacity(0);
    mSKUIdArray.set_capacity(0);
    mPatchName.Clear();
    mPatchDescription.Clear();
    mPatchEULA.Clear();
    mAppId.set_capacity(0);
    mOSNameArray.set_capacity(0);
    mMinOSVersionArray.set_capacity(0);
    mReleaseDate.set_capacity(0);
    mLocaleSupport.Clear();
    mSupercedentPatchIdArray.set_capacity(0);
    mSupercedingPatchIdArray.set_capacity(0);
    mDependentPatchIdArray.set_capacity(0);
    mPatchBaseDirectory.set_capacity(0);
    mUserInfo.set_capacity(0);
    mUserChecksum.set_capacity(0);
    mPatchDescriptionHTMLContent.set_capacity(0);
}


bool PatchInfo::GetPatchDescriptionHtml(String& strHTML) const
{
    strHTML = mPatchDescription.GetString(GetDefaultLocale());

    return !strHTML.empty();
}



///////////////////////////////////////////////////////////////////////////////
// PatchInfoSerializer
///////////////////////////////////////////////////////////////////////////////

bool PatchInfoSerializer::Serialize(const PatchInfo& patchInfo, const char8_t* pFilePath, bool bAddHelpComments)
{
    // We are writing a PatchInfo struct to disk as XML.

    if(mbSuccess)
    {
        // To consider: Enable this:
        //EAPATCH_ASSERT(EA::StdC::Strcmp(EA::IO::Path::GetFileExtension(pFilePath), kPatchInfoFileNameExtension) == 0 || 
        //               EA::StdC::Strcmp(EA::IO::Path::GetFileExtension(pFilePath), kPatchTempFileNameExtension) == 0);

        File file;

        mbSuccess = file.Open(pFilePath, EA::IO::kAccessFlagWrite, EA::IO::kCDCreateAlways, EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential);

        if(mbSuccess)
        {
            Serialize(patchInfo, file.GetStream(), bAddHelpComments);
            file.Close();
        }
        else
            TransferError(file);
    }

    return mbSuccess;
}


bool PatchInfoSerializer::Serialize(const PatchInfo& patchInfo, EA::IO::IStream* pStreamInfo, bool bAddHelpComments)
{
    // We are writing a PatchInfo struct to a stream (usually a disk file) as XML.
    XMLWriter writer;

    writer.BeginDocument(pStreamInfo);
      writer.BeginParent("PatchInfo");
        SerializePatchInfoContents(patchInfo, writer, bAddHelpComments);
      writer.EndParent("PatchInfo");
    writer.EndDocument();

    TransferError(writer);

    return mbSuccess;
}


void PatchInfoSerializer::SerializePatchInfoContents(const PatchInfo& patchInfo, XMLWriter& writer, bool /*bAddHelpComments*/)
{
    // To do: Implement bAddHelpComments.
    // To consider: Don't write fields that are empty.

    writer.WriteValue("PatchInfoURL",                patchInfo.mPatchInfoURL);
    writer.WriteValue("PatchId",                     patchInfo.mPatchId);
    writer.WriteValue("PatchClass",                  patchInfo.mPatchClass);
    writer.WriteValue("Required",                    patchInfo.mRequired);
    writer.WriteValue("PatchImplURL",                patchInfo.mPatchImplURL);
    writer.WriteValue("ContentIdArray",              patchInfo.mContentIdArray);
    writer.WriteValue("SKUIdArray",                  patchInfo.mSKUIdArray);
    writer.WriteValue("PatchName",                   patchInfo.mPatchName);
    writer.WriteValue("PatchDescription",            patchInfo.mPatchDescription);
    writer.WriteValue("PatchEULA",                   patchInfo.mPatchEULA);
    writer.WriteValue("AppId",                       patchInfo.mAppId);
    writer.WriteValue("OSNameArray",                 patchInfo.mOSNameArray);
    writer.WriteValue("MinOSVersionArray",           patchInfo.mMinOSVersionArray);
    writer.WriteValue("ReleaseDate",                 patchInfo.mReleaseDate);
    writer.WriteValue("LocaleSupport",               patchInfo.mLocaleSupport);
    writer.WriteValue("SupercedentPatchIdArray",     patchInfo.mSupercedentPatchIdArray);
    writer.WriteValue("SupercedingPatchIdArray",     patchInfo.mSupercedingPatchIdArray);
    writer.WriteValue("DependentPatchIdArray",       patchInfo.mDependentPatchIdArray);
    writer.WriteValue("PatchBaseDirectory",          patchInfo.mPatchBaseDirectory);
    writer.WriteValue("UserInfo",                    patchInfo.mUserInfo);
    writer.WriteValue("UserChecksum",                patchInfo.mUserChecksum);
    writer.WriteValue("FinalFileSizeUsage",          patchInfo.mFinalFileSizeUsage, false);
    writer.WriteValue("FinalDiskSpaceUsage",         patchInfo.mFinalDiskSpaceUsage, false);
    writer.WriteValue("IntermediateDiskSpaceUsage",  patchInfo.mIntermediateDiskSpaceUsage, false);

    // The error state of writer will be picked up by the caller.
}


bool PatchInfoSerializer::Deserialize(PatchInfo& patchInfo, const char8_t* pFilePath, bool bConvertPathsToLocalForm)
{
    EAPATCH_ASSERT(EA::StdC::Strcmp(EA::IO::Path::GetFileExtension(pFilePath), kPatchInfoFileNameExtension) == 0);

    if(mbSuccess)
    {
        File file;

        mbSuccess = file.Open(pFilePath, EA::IO::kAccessFlagRead, EA::IO::kCDOpenExisting, 
                                EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential);
        if(mbSuccess)
        {
            Deserialize(patchInfo, file.GetStream(), bConvertPathsToLocalForm);
            file.Close();
        }

        if(file.HasError())         // This will catch errors from file.Open or file.Close above.
            TransferError(file);    // Copy the error state from file to us.
    }

    return mbSuccess;
}


bool PatchInfoSerializer::Deserialize(PatchInfo& patchInfo, EA::IO::IStream* pStreamInfo, bool bConvertPathsToLocalForm)
{
    // We are read a PatchInfo struct from an XML stream (usually a disk file).
    XMLReader reader;
    bool      bFound;

    reader.BeginDocument(pStreamInfo);
      reader.BeginParent("PatchInfo", bFound); EAPATCH_ASSERT(bFound);
        DeserializePatchInfoContents(patchInfo, reader, bFound, bConvertPathsToLocalForm);
      reader.EndParent("PatchInfo");
    reader.EndDocument();

    TransferError(reader);

    return mbSuccess;
}


void PatchInfoSerializer::DeserializePatchInfoContents(PatchInfo& patchInfo, XMLReader& reader, bool& bFound, bool bConvertPathsToLocalForm)
{
    reader.ReadValue("PatchInfoURL",               bFound, patchInfo.mPatchInfoURL);                 EAPATCH_ASSERT(bFound); // We may want or need to tweak the set of 
    reader.ReadValue("PatchId",                    bFound, patchInfo.mPatchId);                      EAPATCH_ASSERT(bFound); // fields that are required here.
    reader.ReadValue("PatchClass",                 bFound, patchInfo.mPatchClass);                 //EAPATCH_ASSERT(bFound);
    reader.ReadValue("Required",                   bFound, patchInfo.mRequired);                   //EAPATCH_ASSERT(bFound);
    reader.ReadValue("PatchImplURL",               bFound, patchInfo.mPatchImplURL, 1);              EAPATCH_ASSERT(bFound);
    reader.ReadValue("ContentIdArray",             bFound, patchInfo.mContentIdArray, 1);          //EAPATCH_ASSERT(bFound);
    reader.ReadValue("SKUIdArray",                 bFound, patchInfo.mSKUIdArray, 1);              //EAPATCH_ASSERT(bFound);
    reader.ReadValue("PatchName",                  bFound, patchInfo.mPatchName, 1);               //EAPATCH_ASSERT(bFound);
    reader.ReadValue("PatchDescription",           bFound, patchInfo.mPatchDescription, 0);        //EAPATCH_ASSERT(bFound);
    reader.ReadValue("PatchEULA",                  bFound, patchInfo.mPatchEULA, 0);               //EAPATCH_ASSERT(bFound);
    reader.ReadValue("AppId",                      bFound, patchInfo.mAppId);                      //EAPATCH_ASSERT(bFound);
    reader.ReadValue("OSNameArray",                bFound, patchInfo.mOSNameArray, 0);             //EAPATCH_ASSERT(bFound);
    reader.ReadValue("MinOSVersionArray",          bFound, patchInfo.mMinOSVersionArray, 0);       //EAPATCH_ASSERT(bFound);
    reader.ReadValue("ReleaseDate",                bFound, patchInfo.mReleaseDate);                //EAPATCH_ASSERT(bFound);
    reader.ReadValue("LocaleSupport",              bFound, patchInfo.mLocaleSupport, 1);           //EAPATCH_ASSERT(bFound);
    reader.ReadValue("SupercedentPatchIdArray",    bFound, patchInfo.mSupercedentPatchIdArray, 0); //EAPATCH_ASSERT(bFound);
    reader.ReadValue("SupercedingPatchIdArray",    bFound, patchInfo.mSupercedingPatchIdArray, 0); //EAPATCH_ASSERT(bFound);
    reader.ReadValue("DependentPatchIdArray",      bFound, patchInfo.mDependentPatchIdArray, 0);   //EAPATCH_ASSERT(bFound);
    reader.ReadValue("PatchBaseDirectory",         bFound, patchInfo.mPatchBaseDirectory);         //EAPATCH_ASSERT(bFound);
    reader.ReadValue("UserInfo",                   bFound, patchInfo.mUserInfo);                   //EAPATCH_ASSERT(bFound);
    reader.ReadValue("UserChecksum",               bFound, patchInfo.mUserChecksum);               //EAPATCH_ASSERT(bFound);
    reader.ReadValue("FinalFileSizeUsage",         bFound, patchInfo.mFinalFileSizeUsage);         //EAPATCH_ASSERT(bFound);
    reader.ReadValue("FinalDiskSpaceUsage",        bFound, patchInfo.mFinalDiskSpaceUsage);        //EAPATCH_ASSERT(bFound);
    reader.ReadValue("IntermediateDiskSpaceUsage", bFound, patchInfo.mIntermediateDiskSpaceUsage); //EAPATCH_ASSERT(bFound);

    if(bConvertPathsToLocalForm)
        CanonicalizeFilePath(patchInfo.mPatchBaseDirectory);
}


} // namespace Patch
} // namespace EA



