///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/PatchBuilder.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/Config.h>
#include <EAPatchClient/PatchBuilder.h>
#include <EAPatchClient/Debug.h>
#include <EAPatchClient/XML.h>
#include <EAPatchClient/URL.h>
#include <EAPatchClient/Hash.h>
#include <EAPatchClient/Locale.h>
#include <EAPatchClient/Telemetry.h>
#include <EAIO/PathString.h>
#include <EAIO/EAStreamMemory.h>
#include <EAIO/EAStreamAdapter.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAFileDirectory.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EAEndian.h>
#include <coreallocator/icoreallocatormacros.h>
#include <eathread/eathread_sync.h>
#include <EAAssert/eaassert.h>


namespace EA
{
namespace Patch
{


/////////////////////////////////////////////////////////////////////////////
// PatchBuilderEventCallback
/////////////////////////////////////////////////////////////////////////////

void PatchBuilderEventCallback::EAPatchStart                (PatchBuilder*, intptr_t){}
void PatchBuilderEventCallback::EAPatchProgress             (PatchBuilder*, intptr_t, double){}
void PatchBuilderEventCallback::EAPatchRetryableNetworkError(PatchBuilder*, intptr_t, const char8_t*){}
void PatchBuilderEventCallback::EAPatchNetworkAvailability  (PatchBuilder*, intptr_t, bool){}
void PatchBuilderEventCallback::EAPatchError                (PatchBuilder*, intptr_t, Error&){}
void PatchBuilderEventCallback::EAPatchNewState             (PatchBuilder*, intptr_t, int){}
void PatchBuilderEventCallback::EAPatchBeginFileDownload    (PatchBuilder*, intptr_t, const char8_t*, const char8_t*){}
void PatchBuilderEventCallback::EAPatchEndFileDownload      (PatchBuilder*, intptr_t, const char8_t*, const char8_t*){}
void PatchBuilderEventCallback::EAPatchRenameFile           (PatchBuilder*, intptr_t, const char8_t*, const char8_t*){}
void PatchBuilderEventCallback::EAPatchDeleteFile           (PatchBuilder*, intptr_t, const char8_t*){}
void PatchBuilderEventCallback::EAPatchRenameDirectory      (PatchBuilder*, intptr_t, const char8_t*, const char8_t*){}
void PatchBuilderEventCallback::EAPatchDeleteDirectory      (PatchBuilder*, intptr_t, const char8_t*){}
void PatchBuilderEventCallback::EAPatchStop                 (PatchBuilder*, intptr_t, AsyncStatus){}



/////////////////////////////////////////////////////////////////////////////
// BuiltFileSpan
/////////////////////////////////////////////////////////////////////////////

BuiltFileSpan::BuiltFileSpan(FileType fileType, uint64_t sourceStartPos, uint64_t destStartPos, uint64_t count)
  : mSourceFileType(fileType)
  , mSourceStartPos(sourceStartPos)
  , mDestStartPos(destStartPos)
  , mCount(count)
  , mCountWritten(0)
  , mSpanWrittenChecksum(0)
{
}


/////////////////////////////////////////////////////////////////////////////
// BuiltFileInfo
/////////////////////////////////////////////////////////////////////////////

static const uint32_t kBFIVersion            = 1;
static const uint64_t kBFISerializeIncrement = 1048576;


BuiltFileInfo::BuiltFileInfo()
  : mVersion(kBFIVersion)
  , mDestinationFilePath()
  , mLocalFilePath()
  , mLocalFileSize(0)
  , mLocalSpanCount(0)
  , mLocalSpanWritten(0)
  , mRemoteFilePathURL()
  , mRemoteSpanCount(0)
  , mRemoteSpanWritten(0)
  , mBuiltFileSpanList()
  , mFileStreamTemp()
  , mFileUseCount(0)
{
}


void BuiltFileInfo::Reset()
{
    // Leave mVersion alone.
    mDestinationFilePath.set_capacity(0); // This causes the memory to be freed while also clearing the container.
    mLocalFilePath.set_capacity(0);
    mLocalFileSize = 0;
    mLocalSpanCount = 0;
    mLocalSpanWritten = 0;
    mRemoteFilePathURL.set_capacity(0);
    mRemoteSpanCount = 0;
    mRemoteSpanWritten = 0;
    mBuiltFileSpanList.clear();
    mFileStreamTemp.Close();
    mFileUseCount = 0;
}


void BuiltFileInfo::SetSpanListToSingleSegment(FileType fileType, uint64_t remoteFileSize)
{
    // This function shouldn't be called while a span list is in use, which would be the case if the written count was non-zero.
    EAPATCH_ASSERT((mLocalSpanWritten == 0) && (mRemoteSpanWritten == 0));

    if(remoteFileSize > 0)
    {
        mBuiltFileSpanList.resize(1);
        BuiltFileSpan& bfs = mBuiltFileSpanList.front();

        bfs.mSourceFileType = fileType;
        bfs.mSourceStartPos = 0;
        bfs.mDestStartPos   = 0;
        bfs.mCount          = remoteFileSize; // If this is UINT64_MAX then we mean to use the entire file.
        bfs.mCountWritten   = 0;
    }
    else
        mBuiltFileSpanList.clear();

    if(fileType == kFileTypeLocal)
    {
        mLocalSpanCount  = remoteFileSize;
        mRemoteSpanCount = 0;
    }
    else
    {
        mLocalSpanCount  = 0;
        mRemoteSpanCount = remoteFileSize;
    }
}


void BuiltFileInfo::SetSpanCounts()
{
    mLocalSpanCount  = 0;
    mRemoteSpanCount = 0;

    for(BuiltFileSpanList::iterator it = mBuiltFileSpanList.begin(); it != mBuiltFileSpanList.end(); ++it)
    {
        BuiltFileSpan& bfs = *it;

        if(bfs.mSourceFileType == kFileTypeLocal)
            mLocalSpanCount  += bfs.mCount;
        else
            mRemoteSpanCount += bfs.mCount;
    }
}


// InsertLocalSegment is the most tricky function in this package. Be careful upon revising it.
void BuiltFileInfo::InsertLocalSegment(uint64_t localPosition, uint64_t destStartPosition, uint64_t count, uint64_t destFileSize)
{
    const uint64_t destEndPostion = (destStartPosition + count);

    EAPATCH_ASSERT(!mBuiltFileSpanList.empty()); // There should always be at least an initial 

    // Find the first node the inserted node replaces or goes directly after.
    // Despite the following nested loops looking like an O(n^2) algorithm, it's actually only O(n).
    for(BuiltFileSpanList::iterator it1 = mBuiltFileSpanList.begin(); it1 != mBuiltFileSpanList.end(); ++it1)
    {
        BuiltFileSpan& bfs1 = *it1;
        const uint64_t bfs1StartPos = bfs1.mDestStartPos;
        const uint64_t bfs1EndPos   = bfs1.mDestStartPos + bfs1.mCount;

        // Find the last node the inserted node replaces or goes directly before.
        if((destStartPosition >= bfs1StartPos) && 
           (destStartPosition <  bfs1EndPos))     // If this is where the inserting segment goes...
        {
            for(BuiltFileSpanList::iterator it2 = it1; it2 != mBuiltFileSpanList.end(); )
            {
                BuiltFileSpan& bfs2 = *it2;
              //const uint64_t bfs2StartPos = bfs2.mDestStartPos;
                const uint64_t bfs2EndPos   = bfs2.mDestStartPos + bfs2.mCount;

                if(destEndPostion <= bfs2EndPos) // If this is where we stop...
                {
                    BuiltFileSpanList::iterator it3 = it2;
                    ++it3; // it3 represents the node after it2, and it may be mBuiltFileSpanList.end().

                    // It's possible that it2 == it1, in which case we are inserting within or replacing the range specified by it1.
                    // It's conceivably possible that this segment has no effect on the span list, as it might 
                    // be equivalent to an existing kFileTypeLocal span or it might be entirely internal to a kFileTypeLocal span.
                    //
                    // I tried to make the logic below more elegant, but couldn't easily do so due to the requirements of this 
                    // function. If you want to try to clean it up then be my guest, but be *very careful* as this is tricky.
                    // Make sure that some rigorous unit tests are in place before doing any such changes. It's probably possible
                    // to simplify this by removing the first set of "special case" code and making it part of the code below it.

                    if(it2 == it1) // Check for this special case, which is the only case where we could possibly need to add two segments to the list, not just the incoming local one.
                    {
                        if((destStartPosition > bfs1StartPos) && 
                           (destEndPostion    < bfs1EndPos)) // If the incoming segment is entirely within the existing segment...
                        {
                            // Revise bfs1.mCount, as it will now be a smaller value, but not zero.
                            bfs1.mCount = (destStartPosition - bfs1StartPos);

                            // Insert the new local node.
                            BuiltFileSpanList::iterator itNew = mBuiltFileSpanList.insert(it3);
                            BuiltFileSpan& bfsNew  = *itNew;
                            bfsNew.mSourceFileType = kFileTypeLocal;
                            bfsNew.mSourceStartPos = localPosition;
                            bfsNew.mDestStartPos   = destStartPosition;
                            bfsNew.mCount          = count;

                            // Insert an additional node, because the newly inserted node above is splitting the preceding node through the middle.
                            BuiltFileSpanList::iterator itNew2 = mBuiltFileSpanList.insert(it3);
                            BuiltFileSpan& bfsNew2  = *itNew2;
                            bfsNew2.mSourceFileType = bfs1.mSourceFileType;
                            bfsNew2.mDestStartPos   = bfsNew.mDestStartPos + bfsNew.mCount;
                            bfsNew2.mSourceStartPos = bfs1.mSourceStartPos + (bfsNew2.mDestStartPos - bfs1StartPos);
                            bfsNew2.mCount          = bfs1EndPos - (bfsNew.mDestStartPos + bfsNew.mCount);
                        }
                        else // Else we are either entirely replacing it1 or we are inserting before or after it1.
                        {
                            if(destStartPosition == bfs1StartPos) // If the new node starts at the same location as the existing node...
                            {
                                if(destEndPostion == bfs1EndPos) // If we are exactly replacing the it1 node...
                                {
                                    bfs1.mSourceFileType = kFileTypeLocal;
                                    bfs1.mSourceStartPos = localPosition;
                                  //bfs1.mDestStartPos   = destStartPosition; // This should already be so.
                                  //bfs1.mCount          = count;             // This should already be so.
                                }
                                else // Else we are inserting before the it1 node.
                                {
                                    BuiltFileSpanList::iterator itNew = mBuiltFileSpanList.insert(it1);
                                    BuiltFileSpan& bfsNew  = *itNew;
                                    bfsNew.mSourceFileType = kFileTypeLocal;
                                    bfsNew.mSourceStartPos = localPosition;
                                    bfsNew.mDestStartPos   = destStartPosition;
                                    bfsNew.mCount          = count;

                                    bfs1.mSourceStartPos += count;
                                    bfs1.mDestStartPos   += count;
                                    bfs1.mCount          -= count;

                                    it1 = itNew;  // We reset it1 to be itNew since itNew is the new first node in our range. We'll use it1 below.
                                }
                            }
                            else // Else the new node starts after it1 starts, and so we are inserting the new node after it1.
                            {
                                bfs1.mCount -= count;

                                BuiltFileSpanList::iterator itNew = mBuiltFileSpanList.insert(it3);
                                BuiltFileSpan& bfsNew  = *itNew;
                                bfsNew.mSourceFileType = kFileTypeLocal;
                                bfsNew.mSourceStartPos = localPosition;
                                bfsNew.mDestStartPos   = destStartPosition;
                                bfsNew.mCount          = count;
                            }
                        }
                    }
                    else // else it1 and it2 are different.
                    {
                        // The simplest way to implement this is to just insert a new node between it1 and it2 and 
                        // clean up the sizes of it1 and it2, then let the redundancy removal below do removal of 
                        // any resulting empty nodes. There should be no nodes between it1 and it2, as we would have
                        // removed them in our outer loop.

                        BuiltFileSpanList::iterator itNew = mBuiltFileSpanList.insert(it2);
                        BuiltFileSpan& bfsNew  = *itNew;
                        bfsNew.mSourceFileType = kFileTypeLocal;
                        bfsNew.mSourceStartPos = localPosition;
                        bfsNew.mDestStartPos   = destStartPosition;
                        bfsNew.mCount          = count;

                        // Fixup it1 (bfs1)
                        bfs1.mCount = (bfsNew.mDestStartPos - bfs1.mDestStartPos);
                        if(bfs1.mCount == 0)
                            it1 = mBuiltFileSpanList.erase(it1); // it1 will now be equal to itNew.

                        // Fix up it2 (bfs2)
                        bfs2.mDestStartPos = (bfsNew.mDestStartPos + bfsNew.mCount);
                        bfs2.mCount        = bfs2EndPos - bfs2.mDestStartPos;
                        if(bfs2.mCount == 0)
                            mBuiltFileSpanList.erase(it2);
                    }

                    // Now iterate from it1 to it3 and remove any redundancies. This range will have 
                    // 1 to 3 nodes, depending on what happened above. it1 may be mBuiltFileSpanList.begin()
                    // and it3 may be mBuiltFileSpanList.end().
                    if(it1 != mBuiltFileSpanList.begin()) // If there is a span before it1 (i.e. if it1 isn't the first span in the list)...
                        --it1; // Move back by one span, as it's possible this new span needs to be merged with the previous one.

                    for(BuiltFileSpanList::iterator it = it1; it != it3; ++it)
                    {
                        BuiltFileSpan& bfs = *it;
                        BuiltFileSpanList::iterator itNext(it); ++itNext;

                        if(itNext != it3) // If there is another one after this one in the range we are looking at.
                        {
                            BuiltFileSpan& bfsNext = *itNext;

                            if(bfs.mSourceFileType == bfsNext.mSourceFileType) // If these are both the same source...
                            {
                                if((bfs.mSourceStartPos + bfs.mCount) == bfsNext.mSourceStartPos) // If the current and next node are contiguous sources.
                                {
                                    bfs.mCount += bfsNext.mCount;
                                    mBuiltFileSpanList.erase(itNext);
                                }
                            } // Else nothing to do
                        }

                        EAPATCH_ASSERT(bfs.mCount != 0); // The logic of this function should prevent this.
                    }

                    break;
                }
                else if(it2 != it1)
                {
                    // it2's span has become irrelevant, as it's being completely overlapped by the new local segment.
                    it2 = mBuiltFileSpanList.erase(it2);
                }
                else
                {
                    // In this case, it1 == it2, which could only ever happen the first time through this loop.
                    ++it2;
                }

                #if EAPATCH_ASSERT_ENABLED
                    BuiltFileSpanList::iterator itTemp2(it2);
                    EAPATCH_ASSERT(++itTemp2 != mBuiltFileSpanList.end()); // Assert that an ending insertion position was found.
                #endif
            }

            break;
        }

        #if EAPATCH_ASSERT_ENABLED
            BuiltFileSpanList::iterator itTemp1(it1);
            EAPATCH_ASSERT(++itTemp1 != mBuiltFileSpanList.end()); // Assert that a starting insertion position was found.
        #endif
    }

    #if ((EAPATCH_DEBUG >= 1) && EAPATCH_ASSERT_ENABLED)
        EAPATCH_ASSERT(ValidateSpanList(destFileSize, false));
    #else
        EA_UNUSED(destFileSize);
    #endif
}


bool BuiltFileInfo::ValidateSpanList(uint64_t destFileSize, bool listIsComplete) const
{
    // The span list is a linked list of ordered file spans, with each span indicating a file position and length.
    // The spans are ordered from the beginning of the dest file to the end of it. The location indicated by the 
    // end of one span is the location of the beginning of the next span. The last span should end at the dest
    // file size. Spans can come from a local source or a remote source, and there shouldn't be any two successive
    // spans that are the same time, otherwise they should have been coalesced. There should be no empty spans.

    bool bValid = true;  // Valid until proven invalid.

    BuiltFileSpan bfsPrev;
    uint64_t      localSpanCountFound = 0;
    uint64_t      remoteSpanCountFound = 0;

    for(BuiltFileSpanList::const_iterator it = mBuiltFileSpanList.begin(); it != mBuiltFileSpanList.end(); ++it)
    {
        const BuiltFileSpan& bfs = *it;

        if(bfs.mSourceFileType == kFileTypeLocal)
        {
            localSpanCountFound  += bfs.mCount;

            if((bfs.mSourceStartPos + bfs.mCount) >= mLocalFileSize)
                bValid = false; // Invalid source file span.
        }
        else
            remoteSpanCountFound += bfs.mCount;

        if(bfs.mDestStartPos >= destFileSize)
            bValid = false; // Span begins after the end of the file.

        if(bfs.mCount == 0)
            bValid = false; // Span is empty.

        if(bfs.mCountWritten > bfs.mCount)
            bValid = false; // Written count can't possibly be more than total count.

        BuiltFileSpanList::const_iterator itTemp(it);
        if(++itTemp == mBuiltFileSpanList.end()) // If this is the last span in the list...
        {
            if((bfs.mDestStartPos + bfs.mCount) != destFileSize)
                bValid = false; // Last span in the list should end at the end of the file.
        }
        else
        {
            if(bfs.mCount >= UINT64_C(0x4000000000))            // 256 GiB sanity check.
                bValid = false; // Span is irrationally large.

            if((bfs.mDestStartPos + bfs.mCount) >= destFileSize)
                bValid = false; // spans prior to the last span should end prior to the end of the file.
        }

        if(it != mBuiltFileSpanList.begin()) // If there is a bfsPrev to test against...
        {
            if(bfsPrev.mDestStartPos >= bfs.mDestStartPos)
                bValid = false; // Previous span should start before the next span.

            if((bfsPrev.mDestStartPos + bfsPrev.mCount) != bfs.mDestStartPos)
                bValid = false; // Previous span doesn't end at start of next span.

            if(bfsPrev.mSourceFileType == bfs.mSourceFileType)
            {
                if((bfsPrev.mSourceStartPos + bfsPrev.mCount) == bfs.mSourceStartPos)
                    bValid = false; // Two spans of the same type originating from the same local span are contiguous. They should have been merged together.
            }
        }

        bfsPrev = *it;
    }

    if(listIsComplete)
    {
        if(mLocalSpanCount != localSpanCountFound)
            bValid = false;
        if(mRemoteSpanCount != remoteSpanCountFound)
            bValid = false;
    }


    #if 0 // (EAPATCH_DEBUG >= 3) // Disabled because the generated BFI appears to be correct.
        if(bValid && listIsComplete) // Currently do this only if the list is complete. If we discover problems then we can change it.
        {
            if(mLocalFilePath.find("\\data7.big") != EA::Patch::String::npos) // If it's the one file we are interested in...
            {
                FileComparer fileComparer;

                if(fileComparer.Open(mLocalFilePath.c_str(), "D:\\Temp\\EAPatchBugReport\\BigFileTestData2Patch\\data7.big"))
                {
                    // Walk through BuiltFileSpanList. 
                    // If the span is kFileTypeLocal, do a compare of local span to the actual dest file.
                    // If the span is kFileTypeRemote, there is probably nothing to do that wasn't done above.

                    for(BuiltFileSpanList::const_iterator it = mBuiltFileSpanList.begin(); it != mBuiltFileSpanList.end(); ++it)
                    {
                        const BuiltFileSpan& bfs = *it;

                        if(bfs.mSourceFileType == kFileTypeLocal)
                        {
                            bool bEqual;
                            fileComparer.CompareSpan(bfs.mSourceStartPos, bfs.mDestStartPos, bfs.mCount, bEqual);
                            if(!bEqual)
                                EAPATCH_TRACE_MESSAGE("BuiltFileInfo::ValidateSpanList: Segment inserted doesn't match file contents.\n");
                        }
                    }
                }
            }
        }
    #endif

    return bValid;
}


void BuiltFileInfo::GetFileSizes(uint64_t& localSize, uint64_t& remoteSize) const
{
    localSize  = 0;
    remoteSize = 0;

    for(BuiltFileSpanList::const_iterator it = mBuiltFileSpanList.begin(); it != mBuiltFileSpanList.end(); ++it)
    {
        const BuiltFileSpan& bfs = *it;

        if(bfs.mSourceFileType == kFileTypeLocal)
            localSize += bfs.mCount;
        else
            remoteSize += bfs.mCount;
    }
}


/////////////////////////////////////////////////////////////////////////////
// PatchBuilder::DiffFileDownloadJob
/////////////////////////////////////////////////////////////////////////////

PatchBuilder::DiffFileDownloadJob::DiffFileDownloadJob()
  : mpPatchEntry()
  , msDestFullPath()
  , msDestFullPathTemp()
  , mFileStreamTemp()
{
}



/////////////////////////////////////////////////////////////////////////////
// PatchBuilder::FileDownloadJob
/////////////////////////////////////////////////////////////////////////////

PatchBuilder::FileSpanDownloadJob::FileSpanDownloadJob()
  : mpPatchEntry(NULL)
  , mpBuiltFileInfo(NULL)
  , mBuiltFileSpanIt()
{
}



///////////////////////////////////////////////////////////////////////////////
// BuiltFileInfoSerializer
///////////////////////////////////////////////////////////////////////////////

bool BuiltFileInfoSerializer::Serialize(const BuiltFileInfo& bfi, const char8_t* pFilePath)
{
    EAPATCH_ASSERT(EA::StdC::Strcmp(EA::IO::Path::GetFileExtension(pFilePath), kPatchBFIFileNameExtension) == 0);

    if(mbSuccess)
    {
        File file;

        // It's OK if we overwrite an existing file. We may in fact be doing so in the case of updating a resumed patch.
        mbSuccess = file.Open(pFilePath, EA::IO::kAccessFlagWrite, EA::IO::kCDCreateAlways, 
                                EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential);
        if(mbSuccess)
        {
            Serialize(bfi, file.GetStream());
            file.Close();
        }

        if(file.HasError())         // This will catch errors from file.Open or file.Close above.
            TransferError(file);    // Copy the error state from file to us.
    }

    return mbSuccess;
}

bool BuiltFileInfoSerializer::Serialize(const BuiltFileInfo& bfi, EA::IO::IStream* pStream)
{
    EAPATCH_ASSERT(pStream);
    mpStream = pStream;

    // The following will maintain mbSuccess as they go.
    WriteUint64(bfi.mVersion);
    WriteString(bfi.mDestinationFilePath);  // To do: It may be a bad idea to persist the full file path (as opposed to a relative path), as it's remotely concievable the user 
    WriteString(bfi.mLocalFilePath);        //        will interrupt the download and change the app directory, at least on some platforms.
    WriteUint64(bfi.mLocalFileSize);
    WriteUint64(bfi.mLocalSpanCount);
    WriteUint64(bfi.mLocalSpanWritten);
    WriteString(bfi.mRemoteFilePathURL);
    WriteUint64(bfi.mRemoteSpanCount);
    WriteUint64(bfi.mRemoteSpanWritten);
    WriteUint64(bfi.mBuiltFileSpanList.size());
    //mFileStreamTemp Not serialized.
    //mFileUseCount   Not serialized.

    // To do: Make a checksum value for this data so we can know it's sane upon loading it from disk.    
    // To consider: We could make the loop below faster if we do large bulk reads.

    for(BuiltFileSpanList::const_iterator it = bfi.mBuiltFileSpanList.begin(); it != bfi.mBuiltFileSpanList.end(); ++it)
    {
        const BuiltFileSpan& bfs = *it;

        WriteUint32(bfs.mSourceFileType);
        WriteUint64(bfs.mSourceStartPos);
        WriteUint64(bfs.mDestStartPos);
        WriteUint64(bfs.mCount);
        WriteUint64(bfs.mCountWritten);
        WriteUint32(bfs.mSpanWrittenChecksum);
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


bool BuiltFileInfoSerializer::Deserialize(BuiltFileInfo& bfi, const char8_t* pFilePath)
{
    EAPATCH_ASSERT(EA::StdC::Strcmp(EA::IO::Path::GetFileExtension(pFilePath), kPatchBFIFileNameExtension) == 0);

    if(mbSuccess)
    {
        File file;

        mbSuccess = file.Open(pFilePath, EA::IO::kAccessFlagRead, EA::IO::kCDOpenExisting, 
                                EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential);
        if(mbSuccess)
        {
            Deserialize(bfi, file.GetStream());
            file.Close();
        }

        if(file.HasError())         // This will catch errors from file.Open or file.Close above.
            TransferError(file);    // Copy the error state from file to us.
    }

    return mbSuccess;
}


bool BuiltFileInfoSerializer::Deserialize(BuiltFileInfo& bfi, EA::IO::IStream* pStream)
{
    EAPATCH_ASSERT(pStream);
    mpStream = pStream;

    // The following will maintain mbSuccess as they go.
    ReadUint64(bfi.mVersion);              // If we change the BuiltFileInfo struct and update the version number, the code below will need to be revised to handle the differing versions.
    ReadString(bfi.mDestinationFilePath);
    ReadString(bfi.mLocalFilePath);
    ReadUint64(bfi.mLocalFileSize);
    ReadUint64(bfi.mLocalSpanCount);
    ReadUint64(bfi.mLocalSpanWritten);
    ReadString(bfi.mRemoteFilePathURL);
    ReadUint64(bfi.mRemoteSpanCount);
    ReadUint64(bfi.mRemoteSpanWritten);
    uint64_t spanListSize;
    ReadUint64(spanListSize);
    EAPATCH_ASSERT(spanListSize < 10000000); // Sanity check.

    // To do: Implement some sanity checks before proceeding, such as with a checksum.
    // To consider: We could make the loop below faster if we do large bulk writes.
    bfi.mBuiltFileSpanList.clear();

    for(uint64_t i = 0; i < spanListSize; i++)
    {
        BuiltFileSpan& bfs = bfi.mBuiltFileSpanList.push_back();

        uint32_t sourceFileType;
        ReadUint32(sourceFileType); bfs.mSourceFileType = (FileType)sourceFileType;
        ReadUint64(bfs.mSourceStartPos);
        ReadUint64(bfs.mDestStartPos);
        ReadUint64(bfs.mCount);
        ReadUint64(bfs.mCountWritten);
        ReadUint32(bfs.mSpanWrittenChecksum);
    }

    if(mbSuccess)
    {
        // Do a sanity check to verify that bfi.mLocalSpanCount, mLocalSpanWritten, mRemoteSpanCount, mRemoteSpanWritten match the sum of the data from the spans.
        uint64_t expectedLocalSpanCount    = 0;
        uint64_t expectedLocalSpanWritten  = 0;
        uint64_t expectedRemoteSpanCount   = 0;
        uint64_t expectedRemoteSpanWritten = 0;

        for(BuiltFileSpanList::iterator iter = bfi.mBuiltFileSpanList.begin(); iter != bfi.mBuiltFileSpanList.end(); ++iter)
        {
            const BuiltFileSpan& bfs = *iter;

            if(bfs.mSourceFileType == kFileTypeLocal)
            {
                expectedLocalSpanCount   += bfs.mCount;
                expectedLocalSpanWritten += bfs.mCountWritten;
            }
            else
            {
                expectedRemoteSpanCount   += bfs.mCount;
                expectedRemoteSpanWritten += bfs.mCountWritten;
            }
        }

        EAPATCH_ASSERT((expectedLocalSpanCount  == bfi.mLocalSpanCount)  && (expectedLocalSpanWritten  == bfi.mLocalSpanWritten) && 
                       (expectedRemoteSpanCount == bfi.mRemoteSpanCount) && (expectedRemoteSpanWritten == bfi.mRemoteSpanWritten));

        if((expectedLocalSpanCount  != bfi.mLocalSpanCount)  || (expectedLocalSpanWritten  != bfi.mLocalSpanWritten) || 
           (expectedRemoteSpanCount != bfi.mRemoteSpanCount) || (expectedRemoteSpanWritten != bfi.mRemoteSpanWritten))
        {
            mbSuccess = false; // Fall through and let the error handler below report the error.
        }
    }

    if(!mbSuccess)
    {
        uint32_t state;
        String   sName;
        uint32_t streamType = GetStreamInfo(pStream, state, sName);

        EA_UNUSED(streamType);
        HandleError(kECBlocking, (SystemErrorId)state, kEAPatchErrorIdFileReadFailure, sName.c_str());

        bfi.Reset();
    }

    return mbSuccess;
}




/////////////////////////////////////////////////////////////////////////////
// PatchBuilder
/////////////////////////////////////////////////////////////////////////////

// At the beginning of the given state below, we assume our patching process is the given percent complete.
// This is not a perfect way to report completion rate, but in practice it probably isn't very bad either.
const double kStateCompletionValue[] = 
{
    0.00,    // kStateNone,
    0.00,    // kStateBegin,
    0.02,    // kStateWritingPatchInfoFile,
    0.04,    // kStateDownloadingPatchImplFile,
    0.06,    // kStateProcessingPatchImplFile,
    0.07,    // kStateDownloadingPatchDiffFiles,        // At runtime this completion amount has more detail, based on how much has been downloaded.
    0.15,    // kStateBuildingDestinationFileInfo,
    0.20,    // kStateBuildingDestinationFiles,         // At runtime this completion amount has more detail, based on how much has been downloaded.
    0.90,    // kStateRenamingOriginalFiles,
    0.92,    // kStateRenamingDestinationFiles,
    0.94,    // kStateRemovingIntermediateFiles,
    0.96,    // kStateRemovingOriginalFiles,
    0.98,    // kStateWritingPatchInstalledFile,
    0.99,    // kStateFinalizing,
    1.00     // kStateEnd
};

EA_COMPILETIME_ASSERT(PatchBuilder::kStateEnd == 14); // If this fails then we need to modify the table above.


PatchBuilder::PatchBuilder()
  : mFutex()
  , mBuildDirection(kBDNone)
  , mPatchCourse(kPCNone)
  , mState(kStateNone)
  , mAsyncStatus(kAsyncStatusNone)
  , mbBuildPatchBlockingActive(false)
  , mThread()
  , mServerURLBase()
  , mPatchBaseDirectoryDest()
  , mPatchBaseDirectoryLocal()
  , mPatchInfo()
  , mPatchInfoFileName()
  , mPatchInfoFilePath()
  , mPatchImpl()
  , mPatchImplURL()
  , mPatchImplFilePath()
  , mPatchStateFilePath()
  , mpServer(NULL)
  , mClientLimits(EA::Patch::GetDefaultClientLimits())
  , msLocale()
  , mStartTimeMs(0)
  , mEndTimeMs(0)
  , mDiffFileDownloadList()
  , mDiffFileDownloadCount(0)
  , mBuiltFileInfoArray()
  , mFileSpanDownloadJobList()
  , mPatchImplDownloadVolume(0)
  , mDiffFileDownloadVolume(0)
  , mFileDownloadVolume(0)
  , mFileDownloadVolumeFinal(0)
  , mFileCopyVolume(0)
  , mFileCopyVolumeFinal(0)
  , mHTTP()
  , mbInPlacePatch(true)
  , mpPatchBuilderEventCallback(NULL)
  , mPatchBuilderEventUserContext(0)
  , mbPatchBuilderEventTraceEnabled(false)
  , mpTelemetryEventCallback(NULL)
  , mTelemetryEventUserContext(0)
  , mbTelemetryEventTraceEnabled(false)
  , mDiskWriteDataRateMeasure()
  , mProgressValue(0.0)
  , mProgressValueLastNotify(-1.0)
    // If you add something here, make sure to add it to the Reset function.
{
    #if (EAPATCH_DEBUG >= 2)
        mbPatchBuilderEventTraceEnabled = true;
        mbTelemetryEventTraceEnabled = true;
    #endif
}


PatchBuilder::~PatchBuilder()
{
    // To do: Stop any activity.
}


AsyncStatus PatchBuilder::GetAsyncStatus() const
{
    return mAsyncStatus;
}


void PatchBuilder::SetServer(Server* pServer)
{
    mpServer = pServer;
}


void PatchBuilder::SetLocale(const char8_t* pLocale)
{
    msLocale = pLocale;
}


ClientLimits& PatchBuilder::GetClientLimits()
{
    return mClientLimits;
}


void PatchBuilder::SetClientLimits(const ClientLimits& clientLimits)
{
    mClientLimits = clientLimits;
}


void PatchBuilder::SetDirPaths(const char8_t* pPatchDestinationDirPath, const char8_t* pPatchLocalDirPath, bool /*bDeprecated*/)
{
    if(pPatchDestinationDirPath && *pPatchDestinationDirPath)
    {
        mPatchBaseDirectoryDest = pPatchDestinationDirPath;
        EnsureTrailingSeparator(mPatchBaseDirectoryDest);
    }

    if(pPatchLocalDirPath && *pPatchLocalDirPath)
    {
        mPatchBaseDirectoryLocal = pPatchLocalDirPath;
        EnsureTrailingSeparator(mPatchBaseDirectoryLocal);
    }

    mbInPlacePatch = (mPatchBaseDirectoryDest == mPatchBaseDirectoryLocal) || mPatchBaseDirectoryDest.empty() || mPatchBaseDirectoryLocal.empty();
}


void PatchBuilder::GetDirPaths(String& sPatchDestinationDirPath, String& sPatchLocalDirPath, bool& bInPlacePatch)
{
    sPatchDestinationDirPath = mPatchBaseDirectoryDest;
    sPatchLocalDirPath       = mPatchBaseDirectoryLocal;
    bInPlacePatch            = mbInPlacePatch;
}


void PatchBuilder::Reset()
{
    EA::Thread::AutoFutex autoFutex(mFutex);

    ResetSession();

    // All the things that weren't cleared in ResetSession are cleared and freed here.
    mbBuildPatchBlockingActive = false;
  //mThread (nothing to do)
    mPatchBaseDirectoryDest.set_capacity(0);    // This causes the memory to be freed while also clearing the container.
    mPatchBaseDirectoryLocal.set_capacity(0);
    mPatchInfo.Reset();
    mpServer = NULL;
    mClientLimits = ClientLimits();
    msLocale.set_capacity(0);
    mbInPlacePatch = false;
    mpPatchBuilderEventCallback = NULL;
    mPatchBuilderEventUserContext = NULL;
    mpTelemetryEventCallback = NULL;
    mTelemetryEventUserContext = NULL;
}


void PatchBuilder::ResetSession()
{
    EA::Thread::AutoFutex autoFutex(mFutex);

    // We clear all state that's associated with a specific patch application.
    ErrorBase::Reset();

    mBuildDirection = kBDNone;
    mState = kStateNone;
    mAsyncStatus = kAsyncStatusNone;
  //mbBuildPatchBlockingActive          // Unchanged.
  //mThread                             // Unchanged.
    mServerURLBase.set_capacity(0);     //              This causes the memory to be freed while also clearing the container.
  //mPatchBaseDirectoryDest             // Unchanged.
  //mPatchBaseDirectoryLocal            // Unchanged.
  //mPatchInfo.Reset();                 // Unchanged.
    mPatchInfoFileName.set_capacity(0); //              This causes the memory to be freed while also clearing the container.
    mPatchInfoFilePath.set_capacity(0);
    mPatchImpl.Reset();
    mPatchImplURL.set_capacity(0);
    mPatchImplFilePath.set_capacity(0);
    mPatchStateFilePath.set_capacity(0);
  //mpServer                            // Unchanged.
  //mClientLimits                       // Unchanged.
  //msLocale                            // Unchanged.
    mStartTimeMs = 0;
    mEndTimeMs = 0;
    mDiffFileDownloadList.clear();
    mDiffFileDownloadCount = 0;
    mBuiltFileInfoArray.clear();
    mFileSpanDownloadJobList.clear();
    mPatchImplDownloadVolume = 0;
    mDiffFileDownloadVolume = 0;
    mFileDownloadVolume = 0;
    mFileDownloadVolumeFinal = 0;
    mFileCopyVolume = 0;
    mFileCopyVolumeFinal = 0;
    mHTTP.Reset();
    mHTTP.SetEventHandler(0, NULL);
  //mbInPlacePatch                      // Unchanged.
  //mpPatchBuilderEventCallback         // Unchanged.
  //mPatchBuilderEventUserContext       // Unchanged.
  //mpTelemetryEventCallback            // Unchanged.
  //mTelemetryEventUserContext          // Unchanged.
    mDiskWriteDataRateMeasure.Reset();
    mProgressValue = 0.0;
    mProgressValueLastNotify = -1.0;
}


void PatchBuilder::SetPatchInfo(const PatchInfo& patchInfo)
{
    EAPATCH_ASSERT(!patchInfo.mPatchId.empty() && patchInfo.mPatchImplURL.Count());

    mPatchInfo = patchInfo;
}


void PatchBuilder::SetPatchInfo(const char8_t* pPatchId, const char8_t* pPatchImplURL)
{
    // To do: Must do: Get rid of the pPatchId argument and have the patch id be looked
    // up from reading the PatchImpl file (from PatchImplURL). If pPatchId is NULL 
    // then download pPatchImplURL and read its mPatchId field. We need to do this because
    // it's not practical to require the caller to know the patchId.
    EAPATCH_ASSERT(pPatchId && *pPatchId && pPatchImplURL && *pPatchImplURL);

    mPatchInfo.mPatchId = pPatchId;
    mPatchInfo.mPatchImplURL.AddString(kLocaleAny, pPatchImplURL);
}


PatchInfo& PatchBuilder::GetPatchInfo()
{
    return mPatchInfo;
}


bool PatchBuilder::BuildPatchAsync()
{
    EA::Thread::ThreadParameters params;
    #if EAPATCH_DEBUG
        params.mpName = "EAPatchBuilder";
    #endif
    EA::Thread::ThreadId threadId = mThread.Begin(ThreadFunctionStatic, static_cast<PatchBuilder*>(this), &params, NULL);

    return (threadId != EA::Thread::kThreadIdInvalid);
}


intptr_t PatchBuilder::ThreadFunctionStatic(void* pContext)
{
    PatchBuilder* pThis = static_cast<PatchBuilder*>(pContext);
    return pThis->ThreadFunction();
}


intptr_t PatchBuilder::ThreadFunction()
{
    const bool bResult = BuildPatchBlocking();

    return bResult ? kThreadSuccess : kThreadError;
}


bool PatchBuilder::BuildPatchBlocking()
{
    EAPATCH_ASSERT(!mbBuildPatchBlockingActive);
    mbBuildPatchBlockingActive = true;

    ResetSession();

    // We have some basic requirements of mPatchInfo.
    EAPATCH_ASSERT(!mPatchInfo.mPatchId.empty() && mPatchInfo.mPatchImplURL.Count());

    if(!mpServer)
        mpServer = EA::Patch::GetServer();
    //EAPATCH_ASSERT(mpServer != NULL);  // Having a server is not strictly required for PatchBuilder.

    mHTTP.SetServer(mpServer);

    if(msLocale.empty())
    {
        const char8_t* pDefaultLocale = EA::Patch::GetDefaultLocale();

        if(pDefaultLocale && *pDefaultLocale)
            msLocale = pDefaultLocale;
        else
            msLocale = kLocaleAny;
    }

    NotifyStart();

    BeginPatch();

    // For initial states that are <= kStateBuildingDestinationFiles (i.e. the first phase, the building phase),
    // we always re-execute all states before it. That makes this more robust. For states after that we jump
    // directly to that state. Currently the direct jump is necessary for things to work correctly as explained in BeginPatch.

    switch (mState)
    {
        case kStateNone:
        case kStateBegin:
        case kStateWritingPatchInfoFile:
        case kStateDownloadingPatchImplFile:
        case kStateProcessingPatchImplFile:
        case kStateDownloadingPatchDiffFiles:
        case kStateBuildingDestinationFileInfo:
        case kStateBuildingDestinationFiles:
            WritePatchInfoFile();
            DownloadPatchImplFile();
            DownloadPatchDiffFiles();
            BuildDestinationFileInfo();
            BuildDestinationFiles();    // Fall through

        case kStateRenamingOriginalFiles:
            RenameOriginalFiles();      // Fall through.
        case kStateRenamingDestinationFiles:
            RenameDestinationfiles();   // Fall through.
        case kStateRemovingIntermediateFiles:
            RemoveIntermediateFiles();  // Fall through.
        case kStateRemovingOriginalFiles:
            RemoveOriginalFiles();      // Fall through.
        case kStateWritingPatchInstalledFile:
            WritePatchInstalledFile();  // Fall through.
        case kStateFinalizing:
            Finalize();                 // Fall through.
        case kStateEnd: 
            EndPatch();                 // Fall through.
    }

    if(mAsyncStatus == kAsyncStatusCancelling)
        mAsyncStatus = kAsyncStatusCancelled;

    // Clear some possible error states.
    EAPATCH_ASSERT(!mPatchInfo.HasError() || !mbSuccess); // If mPatchInfo has an error then mbSuccess better be false.
    mPatchInfo.EnableErrorAssertions(false);

    EAPATCH_ASSERT(!mPatchImpl.HasError() || !mbSuccess);
    mPatchImpl.EnableErrorAssertions(false);

    EAPATCH_ASSERT(!mHTTP.HasError() || !mbSuccess);
    mHTTP.EnableErrorAssertions(false);

    if(!mbSuccess)
        NotifyError(mError);

    EAPATCH_ASSERT(mbBuildPatchBlockingActive);
    mbBuildPatchBlockingActive = false;

    NotifyStop();

    return mbSuccess;  // Note that even if mbSuccess is true, mAsyncStatus may be something other than kAsyncStatusCompleted (e.g. kAsyncStatusCancelled).
}


bool PatchBuilder::Stop(bool waitForCompletion, bool& bPatchStopped)
{
    // To do: We need to implement thread synchronization appropriately here, else there is a race
    // condition between threads on their reading and usage of mAsyncStatus. To do this we will need
    // to have BuildPatchBlocking lock the Mutex during access of this variable and be prepared for
    // somebody (e.g. this function) changing it to Cancelled. 

    if(IsAsyncOperationInProgress(mAsyncStatus))
    {
        // To do: Allow for the user to specify this as kAsyncStatusDeferred instead of kAsyncStatusCancelled. 
        //       This would require adding a state called kAsyncStatusDeferring.

        if(mAsyncStatus != kAsyncStatusCancelled)   // If we didn't reached cancelled state already due to a previous call to this function...
            mAsyncStatus = kAsyncStatusCancelling;  // Will later be converted to kAsyncStatusCancelled. 

        // The following will have no effect if mHTTP is idle.
        mHTTP.CancelRetrieval(waitForCompletion);

        // mAsyncStatus is Cancelled, and so BuildPatchBlocking will notice this and stop at its next convenience.
        if(waitForCompletion)   
        {
            EA::StdC::LimitStopwatch limitStopwatch(EA::StdC::Stopwatch::kUnitsSeconds, 10, true);

            while(mbBuildPatchBlockingActive && !limitStopwatch.IsTimeUp())
                EA::Thread::ThreadSleep(50);
        }

        bPatchStopped = true;
    }
    else
        bPatchStopped = false;

    return mbSuccess;
}


PatchBuilder::State PatchBuilder::GetState()
{
    EA::Thread::AutoFutex autoFutex(mFutex);

    return mState;
}


const char8_t* kPatchBuilderStateTable[] = 
{
    "None",
    "Begin",
    "Writing PatchInfo file",
    "Downloading PatchImpl file",
    "Processing PatchImpl file",
    "Downloading PatchDiff files",
    "Building destination file info",
    "Building destination files",
    "Renaming original files",
    "Renaming destination files",
    "Removing intermediate files",
    "Removing original files",
    "Writing patch installed file",
    "End"
};

const char8_t* PatchBuilder::GetStateString(State state)
{
    EA_COMPILETIME_ASSERT(kStateNone == 0);

    if((state < 0) || (state >= (int)EAArrayCount(kPatchBuilderStateTable)))
        state = kStateNone;

    return kPatchBuilderStateTable[state];
}


PatchBuilder::BuildDirection PatchBuilder::GetBuildDirection() const
{
    EA::Thread::AutoFutex autoFutex(mFutex);

    return mBuildDirection;
}


double PatchBuilder::GetCompletionValue()
{
    return mProgressValue;
}


bool PatchBuilder::SetState(State state)
{
    if(mbSuccess)
    {
        mFutex.Lock();
        mState = state;
        mProgressValue = kStateCompletionValue[state];
        if(state != kStateEnd)
            WritePatchStateFile(state);
        mFutex.Unlock();

        NotifyNewState(state);
        NotifyProgress();
    }

    return mbSuccess;
}


// Reads the State from mPatchStateFilePath, or sets State to kStateNone if 
// the file at mPatchStateFilePath doesn't exist.
bool PatchBuilder::ReadPatchStateFile(State& state, bool allowFailure)
{
    bool bSuccess = false; // False until proven true below.

    if(mbSuccess)
    {
        if(EA::IO::File::Exists(mPatchStateFilePath.c_str()))
        {
            File file;

            if(file.Open(mPatchStateFilePath.c_str(), EA::IO::kAccessFlagRead, EA::IO::kCDOpenExisting))
            {
                file.SetPosition(0);

                uint64_t state64;
                uint64_t readSizeResult;
                file.Read(&state64, sizeof(state64), readSizeResult, true);
                state = (State)EA::StdC::FromLittleEndian(state64);

                file.Close();
            }

            if(file.HasError()) // If any of the File calls above failed...
            {
                if(allowFailure)
                    file.EnableErrorAssertions(false);
                else
                    TransferError(file);
            }
            else
                bSuccess = true;
        }
    }

    if(!bSuccess)
        state = kStateNone;

    return bSuccess; // Return bSuccess as opposed to mbSuccess. If mbSuccess if false then bSuccess will be false too. But the converse depends on the allowFailure argument.
}


bool PatchBuilder::WritePatchStateFile(State state)
{
    if(mbSuccess)
    {
        // To consider: If state == kStateNone, then delete the .eaPatchFile instead of writing to it.
        File file;

        // To do: Write to a temp file first, then rename to the final version. That's safer than the direct write here.
        if(file.Open(mPatchStateFilePath.c_str(), EA::IO::kAccessFlagWrite, EA::IO::kCDCreateAlways))
        {
            const uint64_t state64 = EA::StdC::ToLittleEndian((uint64_t)state);
            file.Write(&state64, sizeof(state64));
            file.Flush();   // Most modern OSs don't need this Flush, but some weaker platforms (e.g. hand-held gaming) may.
            file.Close();
        }

        if(file.HasError())
            TransferError(file);
    }

    return mbSuccess;
}


bool PatchBuilder::FileRename(const char8_t* pPathSource, const char8_t* pPathDestination, bool bOverwriteIfPresent)
{
    NotifyRenameFile(pPathSource, pPathDestination);

    bool bResult = EA::IO::File::Rename(pPathSource, pPathDestination, bOverwriteIfPresent);

    #if defined(EA_PLATFORM_WINDOWS)
        if(!bResult)
        {
            // Check if the file is set as read-only and unset the read-only and re-try.
            // We attempt to bust through the read-only file flag, though we don't attempt
            // to bust through other kinds of access rights (e.g. user-level permissions).
            int attrSource = EA::IO::File::GetAttributes(pPathSource);

            if(attrSource) // If the source file exists...
            {
                bool bSourceWasReadOnly = !(attrSource & EA::IO::kAttributeWritable);

                if(bSourceWasReadOnly)
                    EA::IO::File::SetAttributes(pPathSource, EA::IO::kAttributeWritable, true);

                int  attrDest = EA::IO::File::GetAttributes(pPathDestination);
                bool bDestWasReadOnly = (attrDest && !(attrDest & EA::IO::kAttributeWritable));

                if(bDestWasReadOnly)
                    EA::IO::File::SetAttributes(pPathDestination, EA::IO::kAttributeWritable, true);

                if(bSourceWasReadOnly || bDestWasReadOnly)
                    bResult = EA::IO::File::Remove(pPathSource); // Try again.
            }
        }
    #endif

    return bResult;
}

bool PatchBuilder::FileRemove(const char8_t* pPath)
{
    NotifyDeleteFile(pPath);

    bool bResult = EA::IO::File::Remove(pPath);

    #if defined(EA_PLATFORM_WINDOWS)
        if(!bResult)
        {
            // Check if the file is set as read-only and unset the read-only and re-try.
            // We attempt to bust through the read-only file flag, though we don't attempt
            // to bust through other kinds of access rights (e.g. user-level permissions).
            int attr = EA::IO::File::GetAttributes(pPath);

            if(attr && !(attr & EA::IO::kAttributeWritable)) // If the file exists but is read-only...
            {
                if(EA::IO::File::SetAttributes(pPath, EA::IO::kAttributeWritable, true)) // If we can disable the read-only... try again.
                    bResult = EA::IO::File::Remove(pPath);
            }
        }
    #endif

    return bResult;
}

bool PatchBuilder::DirectoryRename(const char8_t* pPathSource, const char8_t* pPathDestination)
{
    NotifyRenameDirectory(pPathSource, pPathDestination);

    return EA::IO::Directory::Rename(pPathSource, pPathDestination);
}

bool PatchBuilder::DirectoryRemove(const char8_t* pPathSource, bool bAllowRecursiveRemoval)
{
    NotifyDeleteDirectory(pPathSource);

    return EA::IO::Directory::Remove(pPathSource, bAllowRecursiveRemoval);
}


void PatchBuilder::EnsureURLIsAbsolute(String& sURL)
{
    // We currently require that absolute URLs begin with http or https (case-sensitive).
    if(sURL.find("http") != 0) // If this appears to be a relative URL...
    {
        //EAPATCH_ASSERT(!mServerURLBase.empty()); // Disabled because there are some cases (e.g. patch cancellation), where mServerURLBase can be empty, and it's OK/irrelevant.
        sURL.insert(0, mServerURLBase);            // Prepend the http server (e.g. http://www.some.server.com:80/) to the relative path.
    }
}


void PatchBuilder::NotifyStart()
{
    if(mpPatchBuilderEventCallback)
        mpPatchBuilderEventCallback->EAPatchStart(this, mPatchBuilderEventUserContext);

    if(mbPatchBuilderEventTraceEnabled)
        { EAPATCH_TRACE_FORMATTED("Patch started\n"); }
}


void PatchBuilder::NotifyProgress()
{
    const double progressChange = (mProgressValue - mProgressValueLastNotify);

    if(fabs(progressChange) >= 0.01) // Don't notify the user unless at least 1% has passed since the last notification.
    {
        mProgressValueLastNotify = mProgressValue;

        if(mpPatchBuilderEventCallback)
            mpPatchBuilderEventCallback->EAPatchProgress(this, mPatchBuilderEventUserContext, mProgressValue);

        if(mbPatchBuilderEventTraceEnabled)
            { EAPATCH_TRACE_FORMATTED("Progress: %2.0f%%\n", mProgressValue * 100); }
    }
}

void PatchBuilder::NotifyNetworkAvailability(bool available)
{
    if(mpPatchBuilderEventCallback)
        mpPatchBuilderEventCallback->EAPatchNetworkAvailability(this, mPatchBuilderEventUserContext, available);

    if(mbPatchBuilderEventTraceEnabled)
        { EAPATCH_TRACE_FORMATTED("Network availability change: now %s\n", available ? "available" : "unavailable"); }
}

void PatchBuilder::NotifyRetryableNetworkError(const char8_t* pURL)
{
    if(mpPatchBuilderEventCallback)
        mpPatchBuilderEventCallback->EAPatchRetryableNetworkError(this, mPatchBuilderEventUserContext, pURL);

    if(mbPatchBuilderEventTraceEnabled)
        { EAPATCH_TRACE_FORMATTED("Network re-tryable error: %s\n", pURL); }
}

void PatchBuilder::NotifyError(Error& error)
{
    if(mpPatchBuilderEventCallback)
        mpPatchBuilderEventCallback->EAPatchError(this, mPatchBuilderEventUserContext, error);

    if(mbPatchBuilderEventTraceEnabled)
        { EAPATCH_TRACE_FORMATTED("Error: %s\n", error.GetErrorString()); }
}

void PatchBuilder::NotifyNewState(int newState)
{
    if(mpPatchBuilderEventCallback)
        mpPatchBuilderEventCallback->EAPatchNewState(this, mPatchBuilderEventUserContext, newState);

    if(mbPatchBuilderEventTraceEnabled)
        { EAPATCH_TRACE_FORMATTED("State: %s\n", GetStateString((State)newState)); }
}

void PatchBuilder::NotifyBeginFileDownload(const char8_t* pFilePath, const char8_t* pFileURL)
{
    if(mpPatchBuilderEventCallback)
        mpPatchBuilderEventCallback->EAPatchBeginFileDownload(this, mPatchBuilderEventUserContext, pFilePath, pFileURL);

    if(mbPatchBuilderEventTraceEnabled)
        { EAPATCH_TRACE_FORMATTED("Begin file download:\n   %s ->\n   %s\n", pFilePath, pFileURL); }
}

void PatchBuilder::NotifyEndFileDownload(const char8_t* pFilePath, const char8_t* pFileURL)
{
    if(mpPatchBuilderEventCallback)
        mpPatchBuilderEventCallback->EAPatchEndFileDownload(this, mPatchBuilderEventUserContext, pFilePath, pFileURL);

    if(mbPatchBuilderEventTraceEnabled)
        { EAPATCH_TRACE_FORMATTED("End file download:\n   %s->\n   %s\n", pFilePath, pFileURL); }
}

void PatchBuilder::NotifyRenameFile(const char8_t* pPrevFilePath, const char8_t* pNewFilePath)
{
    if(mpPatchBuilderEventCallback)
        mpPatchBuilderEventCallback->EAPatchRenameFile(this, mPatchBuilderEventUserContext, pPrevFilePath, pNewFilePath);

    if(mbPatchBuilderEventTraceEnabled)
        { EAPATCH_TRACE_FORMATTED("Renaming file:\n   %s->\n   %s\n", pPrevFilePath, pNewFilePath); }
}

void PatchBuilder::NotifyDeleteFile(const char8_t* pFilePath)
{
    if(mpPatchBuilderEventCallback)
        mpPatchBuilderEventCallback->EAPatchDeleteFile(this, mPatchBuilderEventUserContext, pFilePath);

    if(mbPatchBuilderEventTraceEnabled)
        { EAPATCH_TRACE_FORMATTED("Deleting file:\n   %s\n", pFilePath); }
}

void PatchBuilder::NotifyRenameDirectory(const char8_t* pPrevDirPath, const char8_t* pNewDirPath)
{
    if(mpPatchBuilderEventCallback)
        mpPatchBuilderEventCallback->EAPatchRenameDirectory(this, mPatchBuilderEventUserContext, pPrevDirPath, pNewDirPath);

    if(mbPatchBuilderEventTraceEnabled)
        { EAPATCH_TRACE_FORMATTED("Renaming directory:\n   %s\n   %s\n", pPrevDirPath, pNewDirPath); }
}

void PatchBuilder::NotifyDeleteDirectory(const char8_t* pDirPath)
{
    if(mpPatchBuilderEventCallback)
        mpPatchBuilderEventCallback->EAPatchDeleteDirectory(this, mPatchBuilderEventUserContext, pDirPath);

    if(mbPatchBuilderEventTraceEnabled)
        { EAPATCH_TRACE_FORMATTED("Deleting directory:\n   %s\n", pDirPath); }
}

void PatchBuilder::NotifyStop()
{
    if(mpPatchBuilderEventCallback)
        mpPatchBuilderEventCallback->EAPatchStop(this, mPatchBuilderEventUserContext, mAsyncStatus);

    if(mbPatchBuilderEventTraceEnabled)
        { EAPATCH_TRACE_FORMATTED("Patch stopped with state %s\n", EA::Patch::GetAsyncStatusString(mAsyncStatus)); }
}


void PatchBuilder::RegisterPatchBuilderEventCallback(PatchBuilderEventCallback* pCallback, intptr_t userContext)
{
    mpPatchBuilderEventCallback   = pCallback;
    mPatchBuilderEventUserContext = userContext;
}


void PatchBuilder::TracePatchBuilderEvents(bool enable)
{
    mbPatchBuilderEventTraceEnabled = enable;
}


void PatchBuilder::TelemetryNotifyPatchBuildBegin()
{
    if(mpTelemetryEventCallback)
    {
        TelemetryPatchBuildBegin tpbb(*this);

        tpbb.Init();
        tpbb.mPatchImplURL = mPatchImplURL;
        tpbb.mPatchId      = mPatchInfo.mPatchId;
        tpbb.mPatchCourse  = (mPatchCourse == kPCNew) ? "New" : "Resume";

        mpTelemetryEventCallback->TelemetryEvent(mTelemetryEventUserContext, tpbb);
    }

    // To do.
    //if(mbTelemetryEventTraceEnabled)
    //    { EAPATCH_TRACE_FORMATTED(""); }
}


void PatchBuilder::TelemetryNotifyPatchBuildEnd()
{
    if(mpTelemetryEventCallback)
    {
        TelemetryPatchBuildEnd tpbe(*this);

        tpbe.Init();
        tpbe.mPatchImplURL      = mPatchImplURL;
        tpbe.mPatchId           = mPatchInfo.mPatchId;
        tpbe.mPatchCourse       = (mPatchCourse == kPCNew) ? "New" : "Resume";
        tpbe.mPatchDirection    = "Build";
        tpbe.mStatus            = GetAsyncStatusString(mAsyncStatus);
        tpbe.mDownloadSpeedBytesPerSecond.sprintf("%.1f", mHTTP.GetNetworkReadDataRate() * 1000);
        tpbe.mDiskSpeedBytesPerSecond.sprintf("%.1f", mDiskWriteDataRateMeasure.GetDataRate() * 1000);
        tpbe.mImplDownloadVolume.sprintf("%I64u", mPatchImplDownloadVolume);
        tpbe.mDiffDownloadVolume.sprintf("%I64u", mDiffFileDownloadVolume);
        tpbe.mFileDownloadVolume.sprintf("%I64u", mFileDownloadVolumeFinal);
        tpbe.mFileDownloadVolumeFinal.sprintf("%I64u", mFileDownloadVolumeFinal);
        tpbe.mFileCopyVolume.sprintf("%I64u", mFileCopyVolume);
        tpbe.mFileCopyVolumeFinal.sprintf("%I64u", mFileCopyVolumeFinal);
        tpbe.mPatchTimeEstimate.sprintf("%I64u", 0);    // To do. We need to estimate the rate at which this patch build was approaching completion.
        tpbe.mPatchTime.sprintf("%.1f", (double)(mEndTimeMs - mStartTimeMs) / 1000.0);

        mpTelemetryEventCallback->TelemetryEvent(mTelemetryEventUserContext, tpbe);
    }
}


void PatchBuilder::TelemetryNotifyPatchCancelBegin()
{
    if(mpTelemetryEventCallback)
    {
        TelemetryPatchCancelBegin tpcb(*this);

        tpcb.Init();
        tpcb.mPatchImplURL = mPatchImplURL;
        tpcb.mPatchId      = mPatchImpl.mPatchId;

        mpTelemetryEventCallback->TelemetryEvent(mTelemetryEventUserContext, tpcb);
    }
}


void PatchBuilder::TelemetryNotifyPatchCancelEnd()
{
    if(mpTelemetryEventCallback)
    {
        TelemetryPatchCancelEnd tpce(*this);

        tpce.Init();
        tpce.mPatchImplURL   = mPatchImplURL;
        tpce.mPatchId        = mPatchImpl.mPatchId;
        tpce.mPatchDirection = "Cancel";
        tpce.mStatus         = GetAsyncStatusString(mAsyncStatus);
        tpce.mCancelTimeEstimate.sprintf("%I64u", 0);    // To do. We need to estimate the rate at which this patch cancellation was approaching completion.
        tpce.mCancelTime.sprintf("%.1f", (double)(mEndTimeMs - mStartTimeMs) / 1000.0);

        mpTelemetryEventCallback->TelemetryEvent(mTelemetryEventUserContext, tpce);
    }
}


void PatchBuilder::RegisterTelemetryEventCallback(TelemetryEventCallback* pCallback, intptr_t userContext)
{
    mpTelemetryEventCallback   = pCallback;
    mTelemetryEventUserContext = userContext;
}


void PatchBuilder::TraceTelemetryEvents(bool enable)
{
    mbTelemetryEventTraceEnabled = enable;
}


double PatchBuilder::GetDiskDataRate() const
{
    return mDiskWriteDataRateMeasure.GetDataRate();
}


bool PatchBuilder::ShouldContinue()
{
    return mbSuccess && ((mAsyncStatus <= kAsyncStatusStarted) || (mAsyncStatus == kAsyncStatusCompleted));
}


bool PatchBuilder::SetupDirectoryPaths()
{
    if(mbSuccess)
    {
        // There's an order to what we set up below, and in some cases it's important.

        // Setup mPatchImplURL
        if(mPatchImplURL.empty())
        {
            // Find the URL that matches the current locale. For a lot of patches this might just 
            // always be the same locale for all locales.
            EAPATCH_ASSERT(mPatchInfo.mPatchImplURL.Count());
            const char8_t* pPatchImplURL = mPatchInfo.mPatchImplURL.GetString(msLocale.c_str());

            EAPATCH_ASSERT(pPatchImplURL != NULL);
            if(!pPatchImplURL)  
                HandleError(kECBlocking, kSystemErrorIdNone, kEAPatchErrorIdURLError, NULL); // This will set mbSuccess to false

            URL::EncodeSpacesInURL(pPatchImplURL, mPatchImplURL);                        // e.g. http://patch.ea.com/SomeApp/Patch1/Patch%201.eaPathImpl
        }

        // Setup mServerURLBase, which should be something like: https://patch.ea.com:443/ or http://patch.ea.com/
        // Question: Which should have precedence? mPatchImplURL or mpServer->GetURLBase()?
        if(mPatchImplURL.find("http") == 0) // If the .eaPatchInfo file has an absolute URL for the <PatchInfoURL> field...
        {
            mServerURLBase = mPatchImplURL;

            if(mServerURLBase.find('/', 8) == String::npos) // If somehow the reference URL doesn't have / char after (e.g.) https://patch.ea.com:443 ...
                mServerURLBase.push_back('/');
            else
                mServerURLBase.resize(mServerURLBase.find('/', 8) + 1);
        }
        else if(mpServer) // This should normally be true.
        {
            mServerURLBase = mpServer->GetURLBase();
            EAPATCH_ASSERT(!mServerURLBase.empty()); // If the user set a Server, and we need to use that Server, then the user should have set a Server URL.

            if(!mServerURLBase.empty() && (mServerURLBase.find('/', 8) == String::npos)) // If somehow the reference URL doesn't have / char after (e.g.) https://patch.ea.com:443 ...
                mServerURLBase.push_back('/');
        }
        // Else mServerURLBase will be empty, so hopefully we won't need it during execution, which would 
        // occur if URLs are relative, as we use mServerURLBase to make relative URLs be full URLs.

        // Complete the setup of mPatchImplURL
        // Must be done after mServerURLBase is setup above.
        EnsureURLIsAbsolute(mPatchImplURL);

        // Setup mPatchBaseDirectoryDest / mPatchBaseDirectoryLocal
        // mPatchBaseDirectoryDest is a dynamically set string which identifies the patch destination 
        // data root directory. The application calls SetDirPaths at runtime to set it. 
        // mPatchInfo.mPatchBaseDirectory is the patch root directory set in the PatchInfo. While it can 
        // be an absolute path, typically it is a relative path and is relative to mPatchBaseDirectoryDest.
        // mPatchBaseDirectoryDest and mPatchInfo.mPatchBaseDirectory may be empty (though mPatchBaseDirectoryDest rarely is),
        // but one of the two must not be empty, and if mPatchBaseDirectoryDest is empty then mPatchInfo.mPatchBaseDirectory
        // must be a full rooted path.
        if(mPatchBaseDirectoryDest.empty())
        {
            if(mPatchInfo.mPatchBaseDirectory.empty() || EA::IO::Path::IsRelative(mPatchInfo.mPatchBaseDirectory.c_str()))
            {
                EAPATCH_FAIL_MESSAGE("PatchBuilder: mPatchBaseDirectoryDest and mPatchInfo.mPatchBaseDirectory are empty or invalid.");
                HandleError(kECBlocking, kSystemErrorIdNone, kEAPatchErrorIdInvalidSettings, mPatchBaseDirectoryDest.c_str()); // This will set mbSuccess to false.
            }
        }

        if(mPatchBaseDirectoryDest.find(mPatchInfo.mPatchBaseDirectory) != String::npos) // If it hasn't been appended already by a previous call to this function...
            mPatchBaseDirectoryDest += mPatchInfo.mPatchBaseDirectory;

        if(mbInPlacePatch)
            mPatchBaseDirectoryLocal = mPatchBaseDirectoryDest;
        // Else leave it as it was set with the SetDirPaths function.

        // Setup mPatchInfoFileName
        // Must be done after mPatchImplURL is setup above.
        // This name is used in a number of places to identify patch files such as 
        // the .eaPatchInfo, .eaPatchImpl, .eaPatchState, and .eaPatchInstalled files.
        if(mPatchInfoFileName.empty()) // If mPatchInfoFileName wasn't manually set ahead of time...
        {
            if(!mPatchInfo.mPatchInfoURL.empty() || !mPatchImplURL.empty())     // If mPatchInfoFileName can be set from mPatchInfo...
            {
                const String sPatchInfoURL = (mPatchInfo.mPatchInfoURL.empty() ? mPatchImplURL : mPatchInfo.mPatchInfoURL); // e.g. http://patch.ea.com/SomeApp/Patch1/Patch1.eaPathInfo
                eastl_size_t pos           = sPatchInfoURL.find_last_of("/\\");

                if(pos != String::npos) // If a / was found in the path...
                {
                    mPatchInfoFileName = sPatchInfoURL.c_str() + pos + 1;                               // e.g. Patch1.eaPatchInfo
                    URL::ConvertPathFromEncodedForm(mPatchInfoFileName.c_str(), mPatchInfoFileName);    // Replace "%20" sequences with " ", etc.

                    if(mPatchInfoFileName.find(kPatchImplFileNameExtension) != String::npos)            // Possibly replace Patch1.eaPatchImpl with Patch1.eaPatchInfo.
                         mPatchInfoFileName.replace(mPatchInfoFileName.find(kPatchImplFileNameExtension), EA::StdC::Strlen(kPatchImplFileNameExtension), kPatchInfoFileNameExtension);
                }
            }

            if(mPatchInfoFileName.empty()) // If the above didn't find it...
            {
                EAPATCH_FAIL_MESSAGE("PatchBuilder: Unknown patch file name. PatchBuilder::SetPatchInfo or needs to be called with a valid PatchInfo URL");
                HandleError(kECBlocking, kSystemErrorIdNone, kEAPatchErrorIdInvalidSettings, mPatchBaseDirectoryDest.c_str()); // This will set mbSuccess to false.
            }
        }

        // Setup mPatchInfoFilePath
        // Must be done after mPatchBaseDirectoryDest is setup above.
        if(mPatchInfoFilePath.empty())     // If mPatchInfoFilePath wasn't manually set ahead of time... 
        {
            mPatchInfoFilePath = mPatchBaseDirectoryDest + mPatchInfoFileName;                      // e.g. /SomeApp/Data/Patch1.eaPathInfo
            CanonicalizeFilePath(mPatchInfoFilePath);                                               // Convert / or \ as needed for the platform.
        }

        // Setup mPatchStateFilePath
        // Must be done after mPatchInfoFilePath is setup above.
        if(mPatchStateFilePath.empty())    // If mPatchStateFilePath wasn't manually set ahead of time... 
        {
            mPatchStateFilePath = mPatchInfoFilePath;                                               // e.g. /Root/Data/SomePath.eaPatchInfo
            StripFileNameExtensionFromFilePath(mPatchStateFilePath);                                // e.g. /Root/Data/SomePath
            mPatchStateFilePath += kPatchStateFileNameExtension;                                    // e.g. /Root/Data/SomePath.eaPatchState
        }

        // Setup mPatchImplFilePath
        // Must be done after mPatchBaseDirectoryDest is setup above.
        if(mPatchImplFilePath.empty())      // If mPatchImplFilePath wasn't manually set ahead of time... 
        {
            // This is much like how we set up mPatchInfoFilePath. We can't just rename mPatchImplFilePath
            // to have a different extension because it's theoretically possible that different names 
            // were specified for them, though that would be somewhat annoying.
            String sPatchImplFileName(mPatchImplURL.c_str() + mPatchImplURL.find_last_of("/\\") + 1);   // e.g. Patch1.eaPathImpl
            URL::ConvertPathFromEncodedForm(sPatchImplFileName.c_str(), sPatchImplFileName);

            mPatchImplFilePath = (mPatchBaseDirectoryDest + sPatchImplFileName);                        // e.g. /SomeApp/Data/Patch1.eaPathImpl -- The file path for the .eaPatchImpl file in our destination directory.
            CanonicalizeFilePath(mPatchImplFilePath); // Convert / or \ as needed for the platform.
        }
    }

    return mbSuccess;
}


bool PatchBuilder::BeginPatch()
{
    // Is there a valid reason to check the value of mbSuccess here?
    //if(mbSuccess)
    {
        mStartTimeMs    = GetTimeMilliseconds();
        mBuildDirection = kBDBuilding;
        mPatchCourse    = kPCNew;   // New until we discover that it's actually a resume.
        mAsyncStatus    = kAsyncStatusStarted;

        SetupDirectoryPaths();

        // We need to read the .eaPatchState file before calling SetState, as setting the new state overwrites the .eaPatchState file.
        State persistedState = kStateNone; // This is the state (if any) that was last written to disk).
        ReadPatchStateFile(persistedState, true);

        SetState(kStateBegin); // Set it to kStateBegin, but below we might move it forward to persistedState.

        // We need to do a check to make sure that another patch isn't in-progress in the given directory.
        // To consider: There may be a situation where we allow multiple live patches occurring in the 
        // same directory, though this would require diligence on the patch creator's part to make sure 
        // patches don't step on each other's feet (e.g. update the same file).

        if(mbSuccess)
        {
            String              sError;
            PatchInfoSerializer patchInfoSerializer;
            PatchInfo           patchInfo;

            // These are temp objects and it's OK if they generate errors, as those errors are not important to our patching process integrity.
            patchInfoSerializer.EnableErrorAssertions(false);
            patchInfo.EnableErrorAssertions(false);

            // See if our .eaPatchInfo file exists. If so then we are continuing a patch that was 
            // previously interrupted. Presumably we are simply resuming this patch, but it may be
            // that there is another patch of the same name as this but a different version.
            // Question: Should we instead be doing this with our .eaPatchImpl file?
            if(EA::IO::File::Exists(mPatchInfoFilePath.c_str()))
            {
                // Check the PatchInfo for the patch at mPatchInfoFilePath and verify that it is 
                // the same patch as we are starting (continuing) now. If not the same patch then 
                // we have an error and cannot proceed unless the other patch is cancelled or completed.
                patchInfoSerializer.Deserialize(patchInfo, mPatchInfoFilePath.c_str(), false);

                if(patchInfoSerializer.HasError())  // If the .eaPatchInfo file data had an error, or simply couldn't be opened...
                {
                    // To do: Log this event.

                    // We handle this by deleting the file and proceeding. This may be a problem if this 
                    // file was indicative of an unfinished patch or corrupted patch image.
                    persistedState = kStateNone;

                    if(!FileRemove(mPatchInfoFilePath.c_str()))
                        HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRemoveError, mPatchInfoFilePath.c_str());
                }
                else
                {
                    // Currently we have it so that if the patch Id is "ignore" we ignore this equivalence check.
                    // But that's not what we want to do in the long run. We want to remove the patchId arg
                    // from SetPatchInfo and make SetPatchInfo retrieve it from the patchImplURL arg. Until 
                    // then we are stuck doing this "ignore" test.
                    if((patchInfo.mPatchId != mPatchInfo.mPatchId) & 
                       (patchInfo.mPatchId != "ignore") & 
                       (mPatchInfo.mPatchId != "ignore")) // If the patchInfo on disk refers to a different patch...
                    {
                        const char8_t* pPatchName = patchInfo.mPatchName.GetString(msLocale.c_str());
                        sError = pPatchName ? pPatchName : mPatchInfo.mPatchId.c_str();
                        HandleError(kECBlocking, kSystemErrorIdNone, kEAPatchErrorIdAlreadyActive, sError.c_str());
                    }
                    else
                    {
                        // Else we are continuing our own patch, which is fine.
                        mPatchCourse = kPCResume;
                    }
                }
            }
            else
            {
                persistedState = kStateBegin; // Ignore the original persistedState, and set it to kStateBegin.

                // See if some other .eaPatchInfo file exists. If so then there is another patch active.
                String sFileNamePattern("*"); sFileNamePattern += kPatchInfoFileNameExtension; // *.eaPatchInfo
                String sFilePath;

                // If there is already a .eaPatchInfo file... generate an error.
                if(FilePatternExists(mPatchBaseDirectoryDest.c_str(), sFileNamePattern.c_str(), &sFilePath))
                {
                    patchInfoSerializer.Deserialize(patchInfo, sFilePath.c_str(), false);

                    if(patchInfoSerializer.HasError())
                    {
                        TransferError(patchInfoSerializer);
                        sError = sFilePath;
                    }
                    else
                    {
                        const char8_t* pPatchName = patchInfo.mPatchName.GetString(msLocale.c_str());
                        sError = pPatchName ? pPatchName : mPatchInfo.mPatchId.c_str();
                    }

                    HandleError(kECBlocking, kSystemErrorIdNone, kEAPatchErrorIdAlreadyActive, sError.c_str()); // This will set mbSuccess to false.
                }
            }
        }

        if(mbSuccess)
        {
            // See if we have already completed the downloading phase of this patch and need to jump to the completion phase.
            if(persistedState > kStateBuildingDestinationFiles)
            {
                // Deserialize the disk file into mPatchImpl. We need to do this because we are about to skip a number
                // of steps and fast-forward to the end-game of this patch build. One of those skipped steps is the step
                // where the mPatchImpl is loaded from the .eaPatchImpl file. But we need that information.
                // To consider: Instead of doing this read, call DownloadPatchImplFile(), which is one of those skipped steps.
                PatchImplSerializer patchImplSerializer;
                
                patchImplSerializer.Deserialize(mPatchImpl, mPatchImplFilePath.c_str(), false, true); // We'll check the return value below with HasError().
                
                if(patchImplSerializer.HasError())
                    TransferError(patchImplSerializer);
                else
                {
                    // Originally this code jumped to kStateRenamingOriginalFiles instead of persistedState. But that didn't 
                    // work because if persistedState was kStateRemovingOriginalFiles then the jump to kStateRenamingOriginalFiles
                    // would result in attempting to rename the final files created in kStateRenamingDestinationFiles.
                    SetState(persistedState);
                }
            }
            // Else leave mState as kStateBegin.
        }

        // To consider: In a very arbitrary environment, it's theoretically possible that two patch processes
        // could start simultaneously and race to create the .eaPatchInfo file which acts as an indicator.
        // In practice this isn't currently considered possible because the application is in charge of 
        // initiating all patches and just doesn't do that, at least with the current thinking.

        TelemetryNotifyPatchBuildBegin();
    }

    return mbSuccess;
}


bool PatchBuilder::WritePatchInfoFile()
{
    using namespace EA::IO;

    if(ShouldContinue())
    {
        SetState(kStateWritingPatchInfoFile);

        // We don't need to download the PatchInfo file, as it's assumed to already be available and downloaded 
        // by the application previous to starting the patch process. 

        String sPatchInfoFilePathTemp(mPatchInfoFilePath + kPatchTempFileNameExtension);                    // e.g. /SomeApp/Data/Patch1.eaPathInfo.eaPatchTemp

        // To do: Verify that the mPatchInfoFilePath has been fixed up by the application (if needed) to make it be
        // a full path and not a path with directives in it like $USER_DIR.

        // To do: Implement a directive system that allows automatic translation of things like $USER_DIR to an actual
        // user directory in a way that's correct for the platform.

        EAPATCH_ASSERT(!EA::IO::Path::IsRelative(mPatchInfoFilePath.c_str()));
        EAPATCH_ASSERT(IsFilePathValid(mPatchInfoFilePath));

        // Check to see if it's already fully there.
        if(!EA::IO::File::Exists(mPatchInfoFilePath.c_str()))
        {
            // It's not there, so check to see if an intermediate version of it is present:
            // Check for mPatchInfoFilePath + .eaPatchTemp. All files are written as .eaPatchTemp before 
            // being renamed to their final name. Write it if not. If the temp file is there, just delete 
            // it and start over, as it's small file.
            if(EA::IO::File::Exists(sPatchInfoFilePathTemp.c_str()))
            {
                // To do: Log that a previous uncompleted temp file write was found, and removed.
                FileRemove(sPatchInfoFilePathTemp.c_str());
            }

            // Write mPatchInfo to the .eaPatchInfo file.
            PatchInfoSerializer patchInfoSerializer;

            patchInfoSerializer.Serialize(mPatchInfo, sPatchInfoFilePathTemp.c_str(), false);
            if(patchInfoSerializer.HasError())
            {
                FileRemove(sPatchInfoFilePathTemp.c_str()); // Remove it if it exists.
                TransferError(patchInfoSerializer);
            }

            if(mbSuccess)
            {
                if(!FileRename(sPatchInfoFilePathTemp.c_str(), mPatchInfoFilePath.c_str(), true))
                {
                  //FileRemove(sPatchInfoFilePathTemp.c_str()); Do we want to do this?
                    HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRenameError, sPatchInfoFilePathTemp.c_str()); // This will set mbSuccess to false.
                }
            }
        }
    }

    return mbSuccess;
}


bool PatchBuilder::DownloadPatchImplFile()
{
    if(ShouldContinue())
    {
        SetState(kStateDownloadingPatchImplFile);

        const String sPatchImplFilePathTemp(mPatchImplFilePath + kPatchTempFileNameExtension);            // e.g. /SomeApp/Data/Patch1.eaPathImpl.eaPatchTemp

        // Check to see if it's already fully there.
        if(EA::IO::File::Exists(mPatchImplFilePath.c_str()))
        {
            // Deserialize the disk file into mPatchImpl.
            PatchImplSerializer patchImplSerializer;

            patchImplSerializer.Deserialize(mPatchImpl, mPatchImplFilePath.c_str(), false, true); // We'll check the return value below with HasError().

            if(patchImplSerializer.HasError())
            {
                TransferError(patchImplSerializer);
                goto Done;
            }

            if(mPatchImpl.HasError())
            {
                TransferError(mPatchImpl);
                goto Done;
            }
        }
        else
        {
            // We download the .eaPatchImpl file to a memory stream and write that 
            // memory stream to sPatchImplFilePathTemp. Once that's complete then we 
            // rename it to sPatchImplFilePath, signifying that it was successfully written.

            // The .eaPatchImpl file is not there, so check to see if the .eaPatchImpl.eaPatchTemp file is present.
            if(EA::IO::File::Exists(sPatchImplFilePathTemp.c_str()))
            {
                // To do: Log that a previous uncompleted temp file write was found, and removed.
                FileRemove(sPatchImplFilePathTemp.c_str());
            }

            // Download the .eaPatchImpl file to a memory stream. 
            EA::IO::MemoryStream eaPatchImplStream;
            eaPatchImplStream.AddRef();
            eaPatchImplStream.SetAllocator(EA::Patch::GetAllocator());
            eaPatchImplStream.SetOption(EA::IO::MemoryStream::kOptionResizeEnabled, 1);

            if(!mHTTP.GetFileBlocking(mPatchImplURL.c_str(), &eaPatchImplStream, 0, 0))
            {
                TransferError(mHTTP); // This will set mbSuccess to false.
                goto Done;
            }
            else if(mHTTP.GetAsyncStatus() != kAsyncStatusCompleted)
                goto Done;

            mPatchImplDownloadVolume += eaPatchImplStream.GetSize();

            // Deserialize the memory stream into mPatchImpl.
            PatchImplSerializer patchImplSerializer;

            eaPatchImplStream.SetPosition(0);
            patchImplSerializer.Deserialize(mPatchImpl, &eaPatchImplStream, false, true); // We'll check the return value below with HasError().

            if(patchImplSerializer.HasError())
            {
                TransferError(patchImplSerializer);
                goto Done;
            }

            if(mPatchImpl.HasError())
            {
                TransferError(mPatchImpl);
                goto Done;
            }

            // Write the memory stream to sPatchImplFilePathTemp.
            Error error;
            eaPatchImplStream.SetPosition(0);
            if(!WriteStreamToFile(&eaPatchImplStream, sPatchImplFilePathTemp.c_str(), error))
            {
                FileRemove(sPatchImplFilePathTemp.c_str()); // Delete it if a file was created.
                HandleError(error);
                goto Done;
            }

            // Rename sPatchImplFilePathTemp to sPatchImplFilePath.
            if(mbSuccess)
            {
                if(!FileRename(sPatchImplFilePathTemp.c_str(), mPatchImplFilePath.c_str(), true))
                {
                    FileRemove(sPatchImplFilePathTemp.c_str());
                    const String sError(sPatchImplFilePathTemp + " -> " + mPatchImplFilePath);
                    HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRenameError, sError.c_str()); // This will set mbSuccess to false.
                }
            }
        }
    }

    Done:
    return mbSuccess;    
}


bool PatchBuilder::ProcessPatchImplFile()
{
    if(ShouldContinue())
    {
        SetState(kStateProcessingPatchImplFile);

        // This loop checks the values of patchEntry.mbForceOverwrite and patchEntry.mbFileUnchanged.
        // If patchEntry.mbFileUnchanged is true then we validate that it really can be true and if so
        // we won't be patching this file nor downloading any intermediate files for it (e.g. .eaPatchDiff).
        const PatchImpl patchImplSaved(mPatchImpl);

        for(eastl_size_t i = 0, iEnd = mPatchImpl.mPatchEntryArray.size(); (i != iEnd) && ShouldContinue(); i++)
        {
            PatchEntry& patchEntry = mPatchImpl.mPatchEntryArray[i];

            if(patchEntry.mbFile)
            {
                // In the case of an non-in-place patch (three-location patch), it's possible that a previous
                // install was uncompleted and left completed files in place. So we set mbFileUnchanged to true
                // in order to check for that possibility. The other possibility is that the patch really should
                // be an in-place patch in the destination directory, as the relevant files are there to be patched
                // but due to some mistake the patch was set to a non-in-place patch.
                if(!mbInPlacePatch)
                    patchEntry.mbFileUnchanged = true;

                // patchEntry.mbFileUnchanged indicates that the local file is likely to be the same as the 
                // server version and thus there is no diffing or downloading needed, as long as the local file is 
                // present. However, before just leaving the local file as-is, we should compare patchEntry.mHashValue
                // to the file content's hash value and verify they are the same. If so then in fact we can leave it as-is.
                if(patchEntry.mbFileUnchanged)
                {
                    HashValueMD5 fileHash;
                    Error        error;
                    const String sDestFullPath = mPatchBaseDirectoryDest + patchEntry.mRelativePath;
                    const bool   fileExists = EA::IO::File::Exists(sDestFullPath.c_str());

                    if(fileExists)
                    {
                        if(!EA::Patch::GetFileHash(sDestFullPath.c_str(), fileHash, error))
                        {
                            HandleError(error);
                            break;
                        }
                    }

                    if(fileExists && (patchEntry.mFileHashValue == fileHash))   // If the file is as expected (in final form that doesn't need diffing)...
                    {
                        patchEntry.mbForceOverwrite = false;
                        // To do: Log that we are skipping the downloading of this file and its associated files (.eaPatchDiff, etc.).
                    }
                    else
                    {
                        // We revise the mbFileUnchanged value to be false because we just found that its actually not true.
                        patchEntry.mbFileUnchanged = false;
                        // To do: Log that we are not skipping this file, despite that it was flagged as to be skipped.

                        if(!mbInPlacePatch)
                        {
                            // We have a problem: We are doing a three-location patch (non-in-place patch) and 
                            // there is a file of the same name as this one in the destination directory, yet it
                            // doesn't match this one. This implies that the patching process really was meant to 
                            // be an in-place patch; or possibly there was some file corruption. Our best solution
                            // is probably to start over with an in-place patch.
                            mbInPlacePatch = true;
                            mPatchImpl = patchImplSaved;
                            mPatchBaseDirectoryLocal = mPatchBaseDirectoryDest;
                            return ProcessPatchImplFile(); // Call ourselves recursively. This cannot recurse infinitely because now mbInPlacePatch == true.
                        }
                    }
                }
            }
            // else it's a directory. We don't currently do anything in this case.
        }
    }

    return mbSuccess;    
}



static bool HandlePatchDiffHTTPEventStatic(HTTP::Event eventType, HTTP* pHTTP, HTTP::DownloadJob* pDownloadJob, intptr_t handleEventContext,
                                           const void* /*pData*/, uint64_t /*dataSize*/, bool& /*bWriteHandled*/)
{
    PatchBuilder* pPatchBuilder = reinterpret_cast<PatchBuilder*>(handleEventContext);
    return pPatchBuilder->HandlePatchDiffHTTPEvent(eventType, pHTTP, pDownloadJob);
}

bool PatchBuilder::HandlePatchDiffHTTPEvent(HTTP::Event eventType, HTTP* /*pHTTP*/, HTTP::DownloadJob* pDownloadJob)
{
    bool bSuccess = true;

    DiffFileDownloadJob& dDJ = *static_cast<DiffFileDownloadJob*>(pDownloadJob);

    if(eventType == HTTP::kEventPreDownload)
    {
        // To do: Find a way to assert that no more than the ClientLimits::mFileLimit files are open simultaneously.

        NotifyBeginFileDownload(dDJ.msDestFullPath.c_str(), dDJ.mSourceURL.c_str());

        // First we try to open an existing version of this file for appending. If this fails (which will usually happen) then we try creating a new one.
        if(dDJ.mFileStreamTemp.Open(dDJ.msDestFullPathTemp.c_str(), EA::IO::kAccessFlagWrite, EA::IO::kCDOpenExisting, 
                                     EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential))
        {
            // We need to resume downloading the rest of the file.
            int64_t filePosition = 0;
            dDJ.mFileStreamTemp.SetPosition(0, EA::IO::kPositionTypeEnd);
            dDJ.mFileStreamTemp.GetPosition(filePosition, EA::IO::kPositionTypeBegin);
            EAPATCH_ASSERT(filePosition >= 0);
            dDJ.mRangeBegin = (uint64_t)filePosition;   // Signed to unsigned cast is safe here, as filePosition will never be negative.
            dDJ.mRangeCount = UINT64_MAX;               // Continue reading from filePosition to the end of the file.
        }
        else
        {
            //if(EA::IO::File::Exists(dDJ.msDestFullPathTemp.c_str()))
            //    We have a real problem. To do: Handle this.

            dDJ.mFileStreamTemp.ClearError(); // Ignore any possible error above related to not being able to open the file, as it may have simply not existed.

            dDJ.mFileStreamTemp.Open(dDJ.msDestFullPathTemp.c_str(), EA::IO::kAccessFlagWrite, EA::IO::kCDCreateAlways, 
                                     EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential);
        }

        if(dDJ.mFileStreamTemp.HasError())
        {
            dDJ.mError = dDJ.mFileStreamTemp.GetError();
            bSuccess = false;
        }

        // Else at this point dDJ.mFileStreamTemp is open for writing, and we are ready to download to it from dDJ.mSourceURL.
    }
    else if(eventType == HTTP::kEventPostDownload)
    {
        uint64_t fileSize = 0;
        dDJ.mFileStreamTemp.GetSize(fileSize);
        EAPATCH_ASSERT(fileSize < UINT64_C(1000000000000)); // Sanity check.

        dDJ.mFileStreamTemp.Close();

        if(dDJ.mStatus == kAsyncStatusCompleted) // If successfully downloaded...
        {
            if(dDJ.mFileStreamTemp.HasError())
            {
                // Currently we are setting the error for just the dDJ and not 'this'. Should we 
                // be setting our error too? We'll pick up the dDJ error later in any case. 
                dDJ.mStatus = kAsyncStatusAborted;
                dDJ.mError  = dDJ.mFileStreamTemp.GetError();
                bSuccess = false;
            }
            else // Else the .eaPatchDiff.eaPatchTemp file was successfully downloaded and written.
            {
                if(fileSize < UINT64_C(1000000000000)) // Sanity check.
                    mDiffFileDownloadVolume += fileSize;

                if(!FileRename(dDJ.msDestFullPathTemp.c_str(), dDJ.msDestFullPath.c_str(), true))
                {
                    String sError(dDJ.msDestFullPathTemp + " -> " + dDJ.msDestFullPath);
                    // Currently we are setting the error for just the dDJ and not 'this'. Should we 
                    // be setting our error too? We'll pick up the dDJ error later in any case. 
                    dDJ.mError.SetErrorId(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRenameError, sError.c_str());
                }
                else // Else the rename occurred and the .eaPatchDiff file is done.
                {
                    NotifyEndFileDownload(dDJ.msDestFullPath.c_str(), dDJ.mSourceURL.c_str());

                    // Maintain the completion progress stats.
                    mDiffFileDownloadCount++;
                    double stateCompletion = (double)mDiffFileDownloadCount / (double)mDiffFileDownloadList.size(); // Yields a value between 0 and 1, which indicates how far through the current state alone we are.
                    mProgressValue       = kStateCompletionValue[kStateDownloadingPatchDiffFiles] +
                                                (stateCompletion * (kStateCompletionValue[kStateDownloadingPatchDiffFiles + 1] - kStateCompletionValue[kStateDownloadingPatchDiffFiles]));
                    NotifyProgress();
                }
            }
        } 
        else // Else the download failed.
            bSuccess = false;
    
        if(!bSuccess)
        {
            // To consider : Make it so that failing to download a .eaPatchDiff file isn't fatal but rather
            // we just ignore the diffs and download the entire patch file all the time instead of doing smart diffs.

            // We don't check the return value from Remove because it's theoretically possible 
            // that it didn't exist in the first place and returned false for that reason.
            FileRemove(dDJ.msDestFullPathTemp.c_str());

            // Instead we do a check for FileExists, which we want to be false at this point.
            if(EA::IO::File::Exists(dDJ.msDestFullPathTemp.c_str()))
                HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRemoveError, dDJ.msDestFullPathTemp.c_str());
        }
    }
    else if(eventType == HTTP::kEventNetworkUnavailable)
    {
        NotifyNetworkAvailability(false);
    }
    else if(eventType == HTTP::kEventNetworkAvailable)
    {
        NotifyNetworkAvailability(true);
    }
    else if(eventType == HTTP::kEventRetryableDownloadError)
    {
        NotifyRetryableNetworkError(pDownloadJob->mSourceURL.c_str());
    }
    else if(eventType == HTTP::kEventNonretryableDownloadError)
    {
        // Do we need to do anything here?
    }

    return bSuccess;
}


bool PatchBuilder::DownloadPatchDiffFiles()
{
    if(ShouldContinue())
    {
        SetState(kStateDownloadingPatchDiffFiles);

        // At this point we have a known-valid mPatchImpl and can proceed to download the .eaPatchDiff files listed in it.
        // We may be resuming a previously paused or restarted patch application, so we assume that any number of the 
        // .eaPatchDiff files may have already been downloaded and we need to finish downloading the rest of them. If it 
        // turns out we've already downloaded all .eaPatchDiff files then this function will detect that and exit without
        // having done anything. We write .eaPatchDiff files as .eaPatchDiff.eaPatchTemp while in progress and then rename
        // only as the last step. If we have any such .eaPatchDiff.eaPatchTemp files that already exist then that means
        // the download was interrupted before they could be finished, and so we resume them.

        // To consider: We may want to support an optimization in the future where the .eaPatchDiff files are stored in
        // a single .tar/.zip file, as opposed to multiple individual files on the server. In that case we wouldn't do
        // the processing below but instead would download a single zip file and unpack its contents (which is the set
        // of .eaPatchDiff files) into our tree.
        EAPATCH_ASSERT(mDiffFileDownloadList.empty() && (mDiffFileDownloadCount == 0));

        // We need to create a list of jobs for HTTP to handle.
        for(eastl_size_t i = 0, iEnd = mPatchImpl.mPatchEntryArray.size(); i != iEnd; i++)
        {
            PatchEntry&          patchEntry = mPatchImpl.mPatchEntryArray[i];
            DiffFileDownloadJob& dDJ = mDiffFileDownloadList.push_back(); // We push_back here and used the return value (as opposed to pushing back later and using a temp DiffFileDownloadJob) because we don't want to be copying around DiffFileDownloadJob objects. We will undo this later if we cancel this particular download.

            if(patchEntry.mbFile) // If it's a file and not a directory we are creating...
            {
                bool bGetPatchDiffs = true; // Start by assuming that we need to get the .eaPatchDiff file

                dDJ.msDestFullPath = (mPatchBaseDirectoryDest + patchEntry.mRelativePath + kPatchDiffFileNameExtension);

                // We don't need to get this .eaPatchDiff file if any of the follwing are true:
                //    1) The file was flagged as being the same locally as on the server and thus won't need to be updated.
                //    2) We already have the .eaPatchDiff file.
                //    3) The file was set to just forcefully download without even looking at diffs.
                //    4) The original file doesn't exist, in which case there's no need for diff info, as we'll just download the entire new file.
                //    5) It doesn't match the locale filter in mPatchImple::mLocaleArray.

                // 1) The file is the same locally as on the server and thus won't need to be updated.
                if(bGetPatchDiffs) 
                {
                    // We already verified if the file is truly unchanged earlier in ProcessPatchImplFile.
                    // So if mbFileUnchanged is true here then it really is unchanged and we can use it as-is.
                    if(patchEntry.mbFileUnchanged)
                        bGetPatchDiffs = false;
                }

                // 2) We already have the .eaPatchDiff file.
                if(bGetPatchDiffs)
                {
                    if(EA::IO::File::Exists(dDJ.msDestFullPath.c_str()))
                    {
                        bGetPatchDiffs = false;
                        // To do: Log that we are skipping this file.
                    }
                    // We handle the case of an .eaPatchDiff.eaPatchTemp file existing below.
                }

                // 3) The file was set to just forcefully download without even looking at diffs.
                if(bGetPatchDiffs)
                {
                    if(patchEntry.mbForceOverwrite) // If we are to do a differential patch as opposed to just downloading the entire patch file...
                    {
                        bGetPatchDiffs = false;
                        // To do: Log that we are skipping this file.
                    }
                }

                // 4) The original file doesn't exist, in which case there's no need for diff info, as we'll just download the entire new file.
                if(bGetPatchDiffs)
                {
                    const String sLocalFilePath(mPatchBaseDirectoryLocal + patchEntry.mRelativePath);

                    if(!EA::IO::File::Exists(sLocalFilePath.c_str()))
                    {
                        patchEntry.mbForceOverwrite = true; // We revise the mbForceOverwrite value to be true because that's what we need to do in this case.
                        bGetPatchDiffs = false;
                        // To do: Log that we are skipping this .eaPatchDiff file, as it's pointless to download because we don't need it.
                    }
                }

                // 5) The file doesn't match the locale filter in mPatchImpl::mLocaleArray and so it's not part of the patch.
                if(bGetPatchDiffs)
                {
                    if(!patchEntry.mLocaleArray.empty())
                    {
                        // To do: Change mLocaleArray to be LocalizedString mLocaleSupport instead of a StringArray.
                        //        That would make it consistent with other locale usage and allow simple GetString code below.
                        //if(!patchEntry.mLocaleSupport.GetString(msLocale.c_str()))
                        //    bGetPatchDiffs = false;
                    }
                }

                if(bGetPatchDiffs)
                {
                    // At this point there isn't a .eaPatchDiff file, though there may be an intermediate .eaPatchDiff.eaPatchTemp
                    // file from a previous patching attempt that interrupted.

                    // DownloadJob
                    dDJ.mSourceURL   = patchEntry.mDiffFileURL;
                    EnsureURLIsAbsolute(dDJ.mSourceURL);
                    dDJ.mpStream     = dDJ.mFileStreamTemp.GetStream();
                    dDJ.mRangeBegin  = 0;
                    dDJ.mRangeCount  = 0;  // 0 here indicates to retrieve the entire file. However, when the file is actually opened we will check to see if the file has been partially downloaded already and set these values to instead resume from there.

                    // DiffFileDownloadJob
                    dDJ.mpPatchEntry       = &patchEntry;
                    dDJ.msDestFullPathTemp = (dDJ.msDestFullPath + kPatchTempFileNameExtension);

                    // We don't open the file yet; instead we do it in the HTTP::kEventPreDownload event handler.
                    // The reason for this is that we could theoretically have many files to download, and we can't
                    // open them all at once, as some platforms have restrictions on the number of simultaneous 
                    // files open. So we open the file in the HTTP callback that occurs right before the HTTP
                    // download attempt for the file begins.
                }
                else
                {
                    dDJ.mFileStreamTemp.EnableErrorAssertions(false); // dDJ was unused, so don't even check it for errors.
                    mDiffFileDownloadList.pop_back();                 // Remove dDJ, as we won't be needing it after all.
                }

            } // if(patchEntry.mbFile)

        } // for(mPatchEntryArray)

        // mDiffFileDownloadList now contains a list of all the .eaPatchDiff file URLs we want to download.
        // We set a callback to notify us before individual download jobs begin and after they end. We'll use
        // this callback to open the files to be written and close them as-needed.
        mHTTP.SetEventHandler(HandlePatchDiffHTTPEventStatic, (intptr_t)this);

        // We need to pass an array of pointers to httpClient, and not objects, 
        // so we make an array of pointers here.
        HTTP::DownloadJobPtrArray downloadJobPtrArray;

        for(DiffFileDownloadJobList::iterator it = mDiffFileDownloadList.begin(); it != mDiffFileDownloadList.end(); ++it)
        {
            DiffFileDownloadJob& dDJ = *it;
            downloadJobPtrArray.push_back(&dDJ);
        }

        // The following could take a while, especially if we have it set to retry upon network loss.
        // It could conceivably block forever until we cancel it in this case.
        mHTTP.GetFilesBlocking(downloadJobPtrArray.data(), downloadJobPtrArray.size());

        if(mHTTP.HasError())
            TransferError(mHTTP);
        else if(mHTTP.GetAsyncStatus() != kAsyncStatusCompleted)
        {
            // mAsyncStatus = mHTTP.GetAsyncStatus(); // If HTTP was cancelled, then copy it to our state. Actually we should have already set our state to Cancelled in our Cancel function.
            EAPATCH_ASSERT((mAsyncStatus == kAsyncStatusCancelling) || (mAsyncStatus == kAsyncStatusCancelled) || (mAsyncStatus == kAsyncStatusDeferred)); 
        }

        // At this point we've downloaded all the .eaPatchDiff files, or failed doing so.
        if(mbSuccess && ShouldContinue())
        {
            // Check the individual downloads for errors, just in case. Perhaps this should be done
            // only in debug builds, as the HTTP download above should have caught any errors already.
            for(DiffFileDownloadJobList::iterator it = mDiffFileDownloadList.begin(); (it != mDiffFileDownloadList.end()) && mbSuccess; ++it)
            {
                DiffFileDownloadJob& dDJ = *it;

                if(dDJ.mFileStreamTemp.HasError())
                    TransferError(dDJ.mFileStreamTemp);
            }
        }
    }

    return mbSuccess;    
}




// We have the diffData for the remote file, and we have the local file. The BuiltFileInfo is generated from
// the two, and it defines what spans come from the local file and what spans come from the remote file.
// This function is potentially slow, as it walks byte by byte through local files on the hard drive which are
// potentially gigabytes in size. The internal file reading is buffered and so there isn't a file reading 
// performance problem associated with this, but it can be slow simply due to the size of the files involved.
bool PatchBuilder::GenerateBuiltFileInfo(const DiffData& diffData, BuiltFileInfo& bfi, Error& error, 
                                          GenerateBuiltFileInfoEventCallback* pEventCallback, intptr_t userContext)
{
    EAPATCH_ASSERT(bfi.mLocalFilePath.empty() || EA::IO::File::Exists(bfi.mLocalFilePath.c_str())); // The caller should have filled this in and it should be a valid file

    // We are building a BlockHashWithPositionSet here from DiffData::BlockHashArray. We probably should
    // just make DiffData directly hold BlockHashWithPositionSet. That way when it's deserialized from
    // disk to memory it's already in that format. This will require revising the DiffData serialization
    // and deserialization functions.
    BlockHashWithPositionSet bhwpSet; 

    // Convert BlockHashArray to BlockHashWithPositionSet. bhwpSet is a set of the hash values for each (e.g. 4K) block
    // in the remote file and their file positions in the remote file. Thus for a local file block with a given hash 
    // value, we can see if it's present in the remote file.
    for(eastl_size_t i = 0, iEnd = diffData.mBlockHashArray.size(); i != iEnd; i++)
    {
        const BlockHash&            blockHash = diffData.mBlockHashArray[i];
        const BlockHashWithPosition bhwp(blockHash.mChecksum, blockHash.mHashValue, i * diffData.mBlockSize);

        bhwpSet.insert(bhwp);
    }

    // A file starts out as nothing but a segment from the remote file.
    bfi.SetSpanListToSingleSegment(kFileTypeRemote, diffData.mFileSize);

    // We walk through the local file block by rolling block, one byte forward at a time. 
    // You need to understand the rolling checksum algorithm to understand what this is doing.
    // If you want to tell where in the local file the rolling block is currently referring
    // to, look at [reader.mBlockPosition - reader.mBlockPosition + diffData.mBlockSize].
    FileChecksumReader reader;

    #if 0 // (EAPATCH_DEBUG >= 3) // Disabled because the generated BFI appears to be correct.
        //FileComparer fileComparer;
        //if(bfi.mLocalFilePath.find("\\data7.big") != EA::Patch::String::npos)
        //    fileComparer.Open(bfi.mLocalFilePath.c_str(), "D:\\Temp\\EAPatchBugReport\\BigFileTestData2Patch\\data7.big");
    #endif

    if(reader.Open(bfi.mLocalFilePath.c_str(), (size_t)diffData.mBlockSize))
    {
        bool           bDone;
        uint32_t       checksum;
        const uint64_t kBFINotifyIncrement = 1048576; // We notify about progress every so many bytes read..
        uint64_t       prevNotificationAmount = 0;
        uint64_t       newReadAmount = 0;

        bfi.mLocalFileSize = reader.GetFileSize();

        reader.GoToFirstByte(checksum, bDone);

        while(!bDone)
        {
            bool bMatchFound = false;
            BlockHashWithPositionSet::const_iterator it = bhwpSet.find_as(checksum, eastl::hash<uint32_t>(), BlockHashWithPosition::BlockHashHashEqualTo());

            // If the diffData indicates that the patch file has a block with the same checksum as this block... Do a more rigorous MD5-base hash test of the two blocks.
            // We have a hash_multiset here, which means there can be multiple hashtable entries for the given checksum value, and once our files start getting
            // over a gigabyte in size then we are going to start having lots of such multiple entries. To consider: Switch to a 64 bit checksum.
            #if (EAPATCH_DEBUG >= 2)
                int hashTableEntryCount = 0; // Measures the number of hash table entries for this checksum we wade through before finding a match or testing all of them.
            #endif

            while((it != bhwpSet.end()) && (it->mChecksum == checksum))
            {
                const BlockHashWithPosition& bhwp = *it;
                HashValueMD5Brief            localHashValueBrief;
                uint64_t                     localPosition = 0;
                const HashValueMD5Brief&     patchHashValueBrief = bhwp.mHashValue;

                reader.GetCurrentBriefHash(localHashValueBrief, localPosition);

                #if (EAPATCH_DEBUG >= 2)
                    hashTableEntryCount++;
                #endif

                if(localHashValueBrief == patchHashValueBrief) // If the MD5-based hash matches, then this block is 99.99999+% certain to match the patch block.
                {
                    // The local file segment [localPosition / diffBlockSize] matches the patch file segment [patchPosition / diffBlockSize]. So insert it into the bfi.
                    // There is a very remote chance that there is an MD5 collision whereby the local file span and the remote file span have the same MD5 value but different 
                    // actual file data. We currently have no means of detecting this, and rely on the fact that it should be an exceedingly small chance of happening,
                    // on the order of 1 in 2^64.
                    const uint64_t patchPosition = bhwp.mPosition;
                    const uint64_t patchSize     = ((patchPosition + diffData.mBlockSize) < diffData.mFileSize) ? diffData.mBlockSize : (diffData.mFileSize - patchPosition);

                    #if (EAPATCH_DEBUG >= 2)
                        // EAPATCH_TRACE_FORMATTED("PatchBuilder::GenerateBuiltFileInfo: Segment inserted. localPos: %I64x, patchPos: %I64x, patchSize: %I64x, fileSize: %I64x\n", localPosition, patchPosition, patchSize, diffData.mFileSize);

                        #if 0 // (EAPATCH_DEBUG >= 3) // Disabled because the generated BFI appears to be correct.
                            if(fileComparer.IsOpen())
                            {
                                bool bEqual;
                                fileComparer.CompareSpan(localPosition, patchPosition, patchSize, bEqual);
                                EAPATCH_ASSERT_MESSAGE(bEqual, "PatchBuilder::GenerateBuiltFileInfo: Segment inserted doesn't match file contents.\n");
                            }
                        #endif
                    #endif

                    bfi.InsertLocalSegment(localPosition, patchPosition, patchSize, diffData.mFileSize);

                    EAPATCH_ASSERT(!bMatchFound);   // If this fails then it means there was an MD5 collision (currently triggerable only if the 'continue' is enabled below).
                    bMatchFound = true;

                    #if (EAPATCH_DEBUG >= 2)    // We we do here is continue the looping for debug purposes. We want to make  
                        // continue looping     // sure that we don't encounter another MD5 match while looking up checksums.
                    #else
                        break;
                    #endif
                }

                ++it;
            }

            #if (EAPATCH_DEBUG >= 2)
                if(hashTableEntryCount > 4)
                    EAPATCH_TRACE_FORMATTED("PatchBuilder::GenerateBuiltFileInfo: Encountered at least %d hashtable collisions.\n", hashTableEntryCount);
            #endif

            // If !bMatchFound then we have a checksum collision, which simply means that the current block in the local file had a 32 bit checksum that 
            // matches a checksum from the remote file, but doesn't match the full MD5 from the remote file. This is not an error and is expected to occur periodically.
            // This trace is useless because for huge files there could by very many OK checksum collisions.
            // EAPATCH_TRACE_FORMATTED("PatchBuilder::GenerateBuiltFileInfo: Checksum collision, file: %s, checksum: %I32u, local pos: %I64u, remote pos: %I64u, block size: %I64u\n", bfi.mLocalFilePath.c_str(), checksum, reader.GetBlockPosition(), bhwp.mPosition, diffData.mBlockSize);
            // To consider: if (EAPATCH_DEBUG >= 2) then print out bytes from the local file.

            if(pEventCallback)
            {
                newReadAmount = reader.GetBlockPosition();

                if((prevNotificationAmount / kBFINotifyIncrement) != 
                   (newReadAmount / kBFINotifyIncrement))             // If it's been kBFINotifyIncrement bytes since we've last notified about progress.
                {
                    prevNotificationAmount = newReadAmount;
                    pEventCallback->GenerateBuiltFileInfoProgress(diffData, bfi, error, userContext, newReadAmount);
                }
            }

            if(bMatchFound)
                reader.GoToNextBlock(checksum, bDone);
            else
                reader.GoToNextByte(checksum, bDone);

            // To do: Every N bytes (e.g. ~1MB), generate a notification message about the progress of this file.
        }

        reader.Close();

        bfi.SetSpanCounts(); // This calculates the value of variables such as bfi.mRemoteSpanCount.

        // Notify the user of completion.
        if(pEventCallback)
            pEventCallback->GenerateBuiltFileInfoProgress(diffData, bfi, error, userContext, bfi.mLocalFileSize);
    }

    if(reader.HasError()) // If anything above failed...
    {
        error = reader.GetError();

        // Make it so we just use the entire remote file. This is a fallback.
        bfi.SetSpanListToSingleSegment(kFileTypeRemote, diffData.mFileSize);

        return false;
    }
    else
    {
        #if ((EAPATCH_DEBUG >= 1) && EAPATCH_ASSERT_ENABLED)
            EAPATCH_ASSERT(bfi.ValidateSpanList(diffData.mFileSize, true));
        #endif

        #if 0 // (EAPATCH_DEBUG >= 3) // Disabled because the generated bfi appears to be correct.
            // What we do here is manually build a version of the destination file, for the purpose of validating
            // the resulting bfi. We are trying to track down a bug in PatchBuilder which results in the final file 
            // being wrongly built.
            if(bfi.mLocalFilePath.find("\\data7.big") != EA::Patch::String::npos) // If it's the one file we are interested in...
            {
                FileSpanCopier fileSpanCopier;
                const char8_t* pCheckFilePathName = "D:\\Temp\\EAPatchBugReport\\BigFileTestDataResult\\data7Check.big";
                const char*    pRemoteFileName    = "D:\\Temp\\EAPatchBugReport\\BigFileTestData2Patch\\data7.big";

                if(fileSpanCopier.Open(bfi.mLocalFilePath.c_str(), pCheckFilePathName))
                {
                    for(BuiltFileSpanList::const_iterator it = bfi.mBuiltFileSpanList.begin(); it != bfi.mBuiltFileSpanList.end(); ++it)
                    {
                        const BuiltFileSpan& bfs = *it;

                        if(bfs.mSourceFileType == kFileTypeLocal)
                            fileSpanCopier.CopySpan(bfs.mSourceStartPos, bfs.mDestStartPos, bfs.mCount);
                    }

                    fileSpanCopier.Close();
                }

                if(fileSpanCopier.HasError())
                {
                    fileSpanCopier.ClearError();
                    EAPATCH_TRACE_FORMATTED("PatchBuilder::GenerateBuiltFileInfo: Failure to copy debug spans from %s to %s.\n", bfi.mLocalFilePath.c_str(), pCheckFilePathName);
                }

                if(fileSpanCopier.Open(pRemoteFileName, pCheckFilePathName))
                {
                    for(BuiltFileSpanList::const_iterator it = bfi.mBuiltFileSpanList.begin(); it != bfi.mBuiltFileSpanList.end(); ++it)
                    {
                        const BuiltFileSpan& bfs = *it;

                        if(bfs.mSourceFileType == kFileTypeRemote)
                            fileSpanCopier.CopySpan(bfs.mSourceStartPos, bfs.mDestStartPos, bfs.mCount);
                    }

                    fileSpanCopier.Close();
                }

                if(fileSpanCopier.HasError())
                {
                    fileSpanCopier.ClearError();
                    EAPATCH_TRACE_FORMATTED("PatchBuilder::GenerateBuiltFileInfo: Failure to copy debug spans from %s to %s.\n", pRemoteFileName, pCheckFilePathName);
                }
            }
        #endif
    }

    return true;    
}


bool PatchBuilder::GenerateBuiltFileInfo(const DiffData& diffData, BuiltFileInfo& bfi, GenerateBuiltFileInfoEventCallback* pEventCallback, intptr_t userContext)
{
    // This is a utility function, so we don't go through the trouble of calling if(ShouldContinue())...

    if(!GenerateBuiltFileInfo(diffData, bfi, mError, pEventCallback, userContext))
    {
        mbSuccess = false;
        //mError was already set above.
    }

    return mbSuccess;    
}


bool PatchBuilder::BuildDestinationFileInfo()
{
    if(ShouldContinue())
    {
        SetState(kStateBuildingDestinationFileInfo);

        // We walk through mPatchImpl.mEntryArray and handle all relevant entries.
        // For each entry:
        //     Deserialize its .eaPatchDiff file.
        //        These can be large for the case of large data. Perhaps we should not do all at once.
        //     Generate a BuiltFileInfo struct for it.
        //         Doing this requires the original files and the .eaPatchDiff files.
        //     If the original file isn't present then we don't need the eaPatchDiff file 
        //         and instead just download the entire file.
        //
        // Note:
        // We don't currently have a means to serialize BuiltFileInfo to disk for the purpose of quickly
        // resuming a patch that was interrupted in the middle of this function. It could be done via having
        // a .eaPatchBuiltFileInfo/.eaPatchBFI file which is written to disk next to the .eaPatchDiff file.
        // We'll have to see how long it takes for this function to execute. If it takes more than 2 minutes
        // or so then perhaps we should serialize to disk and check for that disk file before proceeeding
        // with calling GenerateBuiltFileInfo() below, or maybe GenerateBuiltFileInfo can check for the presence
        // of the serialized file. 
        //
        // To do: Provide a means for this function to detect that the user requested an immediate cancel.
        //
        // To consider: Implement an optimization to start downloading file parts via HTTP while processing
        // additional mPatchEntryArray elements. Probably requires using separate threads, possibly useful 
        // to use the Job Manager API.

        DiffDataSerializer dds;

        mBuiltFileInfoArray.reserve(mPatchImpl.mPatchEntryArray.size()); // We do this so that push_back below doesn't keep reallocating and copying around its elements.

        for(eastl_size_t i = 0, iEnd = mPatchImpl.mPatchEntryArray.size(); (i != iEnd) && ShouldContinue(); i++)
        {
            PatchEntry& patchEntry = mPatchImpl.mPatchEntryArray[i];

            if(!patchEntry.mbFileUnchanged) // If this dest file needs updating...
            {
                BuiltFileInfo& bfi = mBuiltFileInfoArray.push_back();

                const String sDestinationFilePath     = mPatchBaseDirectoryDest + patchEntry.mRelativePath;
                const String sDestinationFilePathTemp = sDestinationFilePath + kPatchTempFileNameExtension;
                const String sLocalFilePath           = mPatchBaseDirectoryLocal + patchEntry.mRelativePath;
                      String sRemoteFilePathURL       = patchEntry.mFileURL;
                EnsureURLIsAbsolute(sRemoteFilePathURL);

                if(patchEntry.mbFile) // If it's a file and not a directory we are creating...
                {
                    const String sDestinationDiffFilePath(sDestinationFilePath + kPatchDiffFileNameExtension);  // e.g. /GameData/Data1.big.eaPatchDiff
                    const String sDestinationBFIFilePath(sDestinationFilePath + kPatchBFIFileNameExtension);    // e.g. /GameData/Data1.big.eaPatchBFI

                    // See if the BFI file is already on disk. If so then load it and use it. If a previous patching 
                    // process was interrupted then this file might exist and indicate how much of the destination file
                    // we've already built in addition to having the already-calculated span information.

                    if(EA::IO::File::Exists(sDestinationBFIFilePath.c_str()))
                    {
                        BuiltFileInfoSerializer bfiSerializer;

                        if(!bfiSerializer.Deserialize(bfi, sDestinationBFIFilePath.c_str()))
                        {
                            // Instead of generating a serious failure, we log the error and just kill the BFI 
                            // file and regenerate it below. This allows for more seamless error recovey.
                            // To do: Log the failure.
                            // TransferError(bfiSerializer); 
                            if(!FileRemove(sDestinationBFIFilePath.c_str()))
                                HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRemoveError, sDestinationBFIFilePath.c_str()); // This will set mbSuccess to false.

                            bfi.Reset(); // Fall through to the code below which recreates the bfi file.
                        }


                        bfiSerializer.ClearError();
                    }

                    // We assign these here instead of above because the deserialize code above deserializes into bfi, 
                    // and it may be that the game's base data directory was moved since the last time we were patching.
                    // So we need to re-set these values to make sure they are current.
                    bfi.mDestinationFilePath     = sDestinationFilePath;
                    bfi.mDestinationFilePathTemp = sDestinationFilePathTemp;
                    bfi.mLocalFilePath           = sLocalFilePath;
                    bfi.mRemoteFilePathURL       = sRemoteFilePathURL;

                    if(bfi.mBuiltFileSpanList.empty()) // If the BFI was not successfully found and deserialized above and thus we need to generate it...
                    {
                        // We can check for if the .eaPatchDiff file is supposed to exist via the PatchImpl info, or we can take 
                        // the simpler route and see if it actually does exist, which is slightly more robust in theory.
                        if(EA::IO::File::Exists(sDestinationDiffFilePath.c_str()))
                        {
                            dds.ClearError(); // We're re-using dds, so clear any error it might have.
                            dds.Deserialize(patchEntry.mDiffData, sDestinationDiffFilePath.c_str());

                            if(dds.HasError())
                            {
                                TransferError(dds);
                                // We'll break out of the loop.
                                // To consider: Perhaps we should handle this error by ignoring the diff file and 
                                // downloading the entire file. This would make us a little more robust.
                            }
                            else
                            {
                                // Generate BuiltFileInfo from patchEntry.mDiffData and the source file.
                                GenerateBuiltFileInfo(patchEntry.mDiffData, bfi);
                            }

                            patchEntry.mDiffData.Reset(); // We won't need the memory allocated by this any more.
                        }
                        else
                        {
                            // If the .eaPatchDiff file doesn't exist then we either are downloading the 
                            // entire file without doing diffs or we are downloading no file and instead 
                            // using the entire local file as-is because it's already correct as-is.
                            EAPATCH_ASSERT_FORMATTED(patchEntry.mbFileUnchanged != patchEntry.mbForceOverwrite, ("PatchBuilder::BuildDestinationFileInfo: Can't be both 'unchanged' and 'force overwrite' for %s.", bfi.mDestinationFilePath.c_str())); // These are mutually exclusive; they're opposite of each other.

                            const bool bUsingLocalFileAsIs = (patchEntry.mbFileUnchanged && EA::IO::File::Exists(bfi.mDestinationFilePath.c_str()));

                            // Set the BuiltFileInfo to be a single span: the entire file.
                            bfi.SetSpanListToSingleSegment(bUsingLocalFileAsIs ? kFileTypeLocal : kFileTypeRemote, patchEntry.mFileSize);
                        }

                        // Write the .eaPatchBFI (built file info file), which is simply bfi serialized to disk.
                        BuiltFileInfoSerializer bfiSerializer;

                        bfiSerializer.Serialize(bfi, sDestinationBFIFilePath.c_str());

                        if(bfiSerializer.HasError())
                            TransferError(bfiSerializer);
                    }

                    // Create the destination file.
                    // Write the new file as .eaPatchTemp. 
                    // It will be converted to its final name later after successfully completed.
                    // Our single biggest risk here is running out of disk space, and it will be the 
                    // cause of any failures below at least 95% of the time.
                    // To do: Implement file access rights as-needed.

                    if(EA::IO::File::Exists(bfi.mDestinationFilePathTemp.c_str()))
                    {
                        if(EA::IO::File::GetSize(bfi.mDestinationFilePathTemp.c_str()) != patchEntry.mFileSize) // If the size is wrong, which may be so in rare cases if the application/system previously crashed right as the file was being created.
                            FileRemove(bfi.mDestinationFilePathTemp.c_str());
                    }

                    if(!EA::IO::File::Exists(bfi.mDestinationFilePathTemp.c_str()))
                    {
                        Error error;

                        // To consider: We don't really need to create a file that's fully sized here, as we could 
                        // alternatively create a zero-sized file and let the later file writing code handle the
                        // resizing requirement during writing.
                        if(!CreateSizedFile(bfi.mDestinationFilePathTemp.c_str(), patchEntry.mFileSize, error))
                            HandleError(error); // This will set mbSuccess to false.
                    }
                } // if(patchEntry.mbFile)
                else // Else the patch entry is a directory
                {
                    // Create the directory.
                    // Create the new file as .eaPatchTemp (e.g. /GameData/SomeDir.eaPatchTemp/
                    // It will be converted to its final name (e.g. /GameData/SomeDir/) later.
                    // To do: Implement directory access rights as-needed.
                    mBuiltFileInfoArray.pop_back(); // We won't be needing this after all.
     
                    String sDestinationDirectoryPathTemp(sDestinationFilePath); // Convert (e.g.) /abc/def/ to /abc/def.eaPatchTemp/
                    sDestinationDirectoryPathTemp.insert(sDestinationDirectoryPathTemp.size() - 2, kPatchTempFileNameExtension);

                    if(!EA::IO::Directory::Create(sDestinationDirectoryPathTemp.c_str()))
                        HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdDirCreateError, sDestinationDirectoryPathTemp.c_str()); // This will set mbSuccess to false.
                }
            }

        } // For each in mPatchImpl.mPatchEntryArray... 

        // At this point we have a filled in BuiltFileInfoArray for all files.
        
    } // if(mbSuccess)

    return mbSuccess;    
}


static bool HandleBuildFileHTTPEventStatic(HTTP::Event eventType, HTTP* pHTTP, HTTP::DownloadJob* pDownloadJob, intptr_t handleEventContext,
                                         const void* pData, uint64_t dataSize, bool& bWriteHandled)
{
    PatchBuilder* pPatchBuilder = reinterpret_cast<PatchBuilder*>(handleEventContext);
    return pPatchBuilder->HandleBuildFileHTTPEvent(eventType, pHTTP, pDownloadJob, pData, dataSize, bWriteHandled);
}

bool PatchBuilder::HandleBuildFileHTTPEvent(HTTP::Event eventType, HTTP* /*pHTTP*/, HTTP::DownloadJob* pDownloadJob,
                                         const void* pData, uint64_t dataSize, bool& bWriteHandled)
{
    bool bSuccess = true;

    FileSpanDownloadJob& fSDJ = *static_cast<FileSpanDownloadJob*>(pDownloadJob);

    if(eventType == HTTP::kEventPreDownload)
    {
        // We never access these files from multiple threads and so there are no mutexes.

        if(fSDJ.mpBuiltFileInfo->mFileStreamTemp.IsOpen())
        {
            EAPATCH_ASSERT(fSDJ.mpBuiltFileInfo->mFileUseCount > 0);

            fSDJ.mpBuiltFileInfo->mFileUseCount++;
        }
        else
        {
            EAPATCH_ASSERT(fSDJ.mpBuiltFileInfo->mFileUseCount == 0);

            #if 0 // (EAPATCH_DEBUG >= 3)
                EAPATCH_TRACE_FORMATTED("PatchBuilder::BuildDestinationFiles: Opening download file. DestPath: %s.\n", fSDJ.mDestPath.c_str());
            #endif

            bSuccess = fSDJ.mpBuiltFileInfo->mFileStreamTemp.Open(fSDJ.mpBuiltFileInfo->mDestinationFilePathTemp.c_str(), EA::IO::kAccessFlagWrite, EA::IO::kCDOpenAlways, 
                                                                   EA::IO::FileStream::kShareRead, EA::IO::FileStream::kUsageHintSequential);
            if(bSuccess)
                fSDJ.mpBuiltFileInfo->mFileUseCount = 1; // We are the first to open this file, so the use count is now 1.
            else
            {
                //if(EA::IO::File::Exists(fSDJ.mpBuiltFileInfo->mDestinationFilePathTemp.c_str()))
                //    We have a real problem. To do: Handle this.
            }
        }

        if(fSDJ.mpBuiltFileInfo->mFileStreamTemp.HasError())
        {
            fSDJ.mError = fSDJ.mpBuiltFileInfo->mFileStreamTemp.GetError();
            bSuccess = false;
        }
        else // Else at this point fSDJ.mpBuiltFileInfo->mFileStreamTemp is open for writing, and we are ready to download to it.
        {
            NotifyBeginFileDownload(fSDJ.mpBuiltFileInfo->mDestinationFilePathTemp.c_str(), fSDJ.mSourceURL.c_str());
        }
    }
    else if(eventType == HTTP::kEventPostDownload)
    {
        EAPATCH_ASSERT(fSDJ.mpBuiltFileInfo->mFileUseCount > 0);

        if(--fSDJ.mpBuiltFileInfo->mFileUseCount == 0)
        {
            #if 0 // (EAPATCH_DEBUG >= 3)
                char     debugBuffer[16];
                uint64_t readSizeResult;
                fSDJ.mpBuiltFileInfo->mFileStreamTemp.ReadPosition(debugBuffer, 16, 0, readSizeResult, true);
                if(debugBuffer[0] != 'B' || debugBuffer[1] != 'I')
                    EA_DEBUG_BREAK();

                //EAPATCH_TRACE_FORMATTED("PatchBuilder::BuildDestinationFiles: Closing download file. DestPath: %s.\n", fSDJ.mDestPath.c_str());
            #endif

            fSDJ.mpBuiltFileInfo->mFileStreamTemp.Close();
        }

        if(fSDJ.mStatus == kAsyncStatusCompleted) // If successfully downloaded...
        {
            if(fSDJ.mpBuiltFileInfo->mFileStreamTemp.HasError())
            {
                // Currently we are setting the error for just the fSDJ and not 'this'. Should we 
                // be setting our error too? We'll pick up the fSDJ error later in any case. 
                fSDJ.mStatus = kAsyncStatusAborted;
                fSDJ.mError  = fSDJ.mpBuiltFileInfo->mFileStreamTemp.GetError();
                bSuccess = false;
            }
            else // Else this .eaPatchTemp file segment was successfully downloaded and written.
            {
                // We've alreay updated the stats (e.g. mCountWritten) below during kEventWrite, so we don't
                // need to update them here. Similarly we've already serialized the BFI file to disk below.

                NotifyEndFileDownload(fSDJ.mpBuiltFileInfo->mDestinationFilePathTemp.c_str(), fSDJ.mSourceURL.c_str());
            }
        } // Else the download failed.
        else
            bSuccess = false;
    
        if(!bSuccess)
        {
            // Probably the Internet connection was lost or something similar. This is not a fatal error.
            // To do: Log this.
        }
    }
    else if(eventType == HTTP::kEventWrite)
    {
        // We handle all the HTTP writes to the disk here and don't let the HTTP code do the writes. The reason is that we are 
        // doing random access writes and need to update stat information as we go and possibly serialize the .eaPatchBFI file
        // to disk. These are things we know about and the HTTP class doesn't.
        BuiltFileSpan& bfs = *fSDJ.mBuiltFileSpanIt;

        EAPATCH_ASSERT((bfs.mCountWritten + dataSize) <= bfs.mCount); // We initially set up the HTTP job to download mCount bytes, but if this fails then we are receiving more than that.

        if((bfs.mCountWritten + dataSize) > bfs.mCount)   // This should never happen.
            dataSize = (bfs.mCount - bfs.mCountWritten);  // We assume that simply too much is being returned, and so we ignore the extra.

        // The code here is very similar to the equivalent code for writing local spans. We probably should find a way
        // to merge them into a single piece of code.
        const uint64_t prevWriteTotal  = fSDJ.mpBuiltFileInfo->mLocalSpanWritten + fSDJ.mpBuiltFileInfo->mRemoteSpanWritten;
        const uint64_t newWriteTotal   = prevWriteTotal + dataSize;
        const uint64_t writePosition   = bfs.mDestStartPos + bfs.mCountWritten;

        #if 0 // (EAPATCH_DEBUG >= 3)
            if(((uint8_t*)pData)[0] == 0xae && 
               ((uint8_t*)pData)[1] == 0x00 &&
               ((uint8_t*)pData)[2] == 0x01 &&
               ((uint8_t*)pData)[3] == 0x00 &&
               ((uint8_t*)pData)[4] == 0x06 &&
               ((uint8_t*)pData)[5] == 0xf5)
                EA_DEBUG_BREAK();
        #endif

        fSDJ.mpBuiltFileInfo->mFileStreamTemp.WritePosition(pData, dataSize, writePosition);

        bfs.mCountWritten += dataSize;
        fSDJ.mpBuiltFileInfo->mRemoteSpanWritten += dataSize;

        #if 0 // (EAPATCH_DEBUG >= 3)
            {
                char     debugBuffer[16];
                uint64_t readSizeResult;
                fSDJ.mpBuiltFileInfo->mFileStreamTemp.ReadPosition(debugBuffer, 16, 0, readSizeResult, true);
                if(debugBuffer[0] != 'B' || debugBuffer[1] != 'I')
                    EA_DEBUG_BREAK();

                // EAPATCH_TRACE_FORMATTED("PatchBuilder::HandleBuildFileHTTPEvent: Job written. RangeBegin: %9I64u, RangeCount: %9I64u, Bytes:%02x%02x%02x%02x..., DestPath: %s.\n", writePosition, dataSize, ((uint8_t*)pData)[0], ((uint8_t*)pData)[1], ((uint8_t*)pData)[2], ((uint8_t*)pData)[3], fSDJ.mDestPath.c_str());
            }
        #endif

        const bool currentSpanDone = (bfs.mCountWritten == bfs.mCount);

        if(currentSpanDone)
        {
            // To do: Update bfs.mSpanWrittenChecksum. As of this writing there 
            //        isn't any code anywhere that actually checks this.
        }

        // Update metrics
        {
            mFileDownloadVolume += dataSize;
            const double stateCompletion = (double)(mFileDownloadVolume + mFileCopyVolume) / (double)(mFileDownloadVolumeFinal + mFileCopyVolumeFinal); // Yields a value between 0 and 1, which indicates how far through the current state alone we are.
            mProgressValue = kStateCompletionValue[kStateBuildingDestinationFiles] + (stateCompletion * (kStateCompletionValue[kStateBuildingDestinationFiles + 1] - kStateCompletionValue[kStateBuildingDestinationFiles]));
            NotifyProgress();
        }

        if(((prevWriteTotal / kBFISerializeIncrement) != (newWriteTotal / kBFISerializeIncrement)) || currentSpanDone) // If it's been kSerializeIncrement bytes since we've last saved off a copy of the BFI to disk or if we've finished the current span....
        {
            fSDJ.mpBuiltFileInfo->mFileStreamTemp.Flush();

            // Write the .eaPatchBFI (built file info file), which is simply bfi serialized to disk.
            const String            sDestinationBFIFilePath(fSDJ.mpBuiltFileInfo->mDestinationFilePath + kPatchBFIFileNameExtension);    // e.g. /GameData/Data1.big.eaPatchBFI
            BuiltFileInfoSerializer bfiSerializer;

            bfiSerializer.Serialize(*fSDJ.mpBuiltFileInfo, sDestinationBFIFilePath.c_str());

            if(bfiSerializer.HasError())
                fSDJ.mError = bfiSerializer.GetError();
        }

        bWriteHandled = true;
    }
    else if(eventType == HTTP::kEventNetworkUnavailable)
    {
        NotifyNetworkAvailability(false);
    }
    else if(eventType == HTTP::kEventNetworkAvailable)
    {
        NotifyNetworkAvailability(true);
    }
    else if(eventType == HTTP::kEventRetryableDownloadError)
    {
        NotifyRetryableNetworkError(pDownloadJob->mSourceURL.c_str());
    }
    else if(eventType == HTTP::kEventNonretryableDownloadError)
    {
        // Do we need to do anything here?
    }

    return bSuccess;
}


bool PatchBuilder::BuildDestinationFiles()
{
    if(ShouldContinue())
    {
        SetState(kStateBuildingDestinationFiles);

        EAPATCH_ASSERT(mFileSpanDownloadJobList.empty() && (mFileDownloadVolume == 0) && (mFileCopyVolume == 0));

        // Execute downloads of required file spans from server.
        // The BuiltFileInfo indicates the current state of the download. It may be that the file is  
        // already partially or fully downloaded, which completed in a previously interrupted session.

        EAPATCH_ASSERT(mPatchImpl.mPatchEntryArray.size() == mBuiltFileInfoArray.size());

        for(eastl_size_t i = 0, iEnd = mPatchImpl.mPatchEntryArray.size(); (i != iEnd) && ShouldContinue(); i++) // For each file in the patch...
        {
            PatchEntry& patchEntry = mPatchImpl.mPatchEntryArray[i];

            if(!patchEntry.mbFileUnchanged) // If this dest file needs updating...
            {
                BuiltFileInfo& bfi = mBuiltFileInfoArray[i];

                if(bfi.mRemoteSpanWritten < bfi.mRemoteSpanCount) // If there is more to download for this file...
                {
                    // Go through bfi.mBuiltFileSpanList and add jobs for all uncompleted downloads.
                    for(BuiltFileSpanList::iterator it = bfi.mBuiltFileSpanList.begin(); it != bfi.mBuiltFileSpanList.end(); ++it) // For each span in the file...
                    {
                        const BuiltFileSpan& bfs = *it;

                        if((bfs.mSourceFileType == kFileTypeRemote) && (bfs.mCountWritten < bfs.mCount)) // If there is more to write for this span...
                        {
                            FileSpanDownloadJob& fSDJ = mFileSpanDownloadJobList.push_back(); // We push_back here and used the return value (as opposed to pushing back later and using a temp DiffFileDownloadJob) because we don't want to be copying around DiffFileDownloadJob objects. We will undo this later if we cancel this particular download.

                            // FileSpanDownloadJob
                            fSDJ.mpPatchEntry     = &patchEntry;
                            fSDJ.mpBuiltFileInfo  = &bfi;
                            fSDJ.mBuiltFileSpanIt = it;

                            // DownloadJob
                            fSDJ.mSourceURL  = patchEntry.mFileURL;
                            EnsureURLIsAbsolute(fSDJ.mSourceURL);
                            fSDJ.mDestPath   = bfi.mDestinationFilePath; // mDestionationPath is the final path. When downloading we'll write to <file name>.eaPatchTemp, and it will be renamed to the final path later.
                            fSDJ.mpStream    = fSDJ.mpBuiltFileInfo->mFileStreamTemp.GetStream();
                            fSDJ.mRangeBegin = bfs.mSourceStartPos + bfs.mCountWritten;
                            fSDJ.mRangeCount = bfs.mCount - bfs.mCountWritten;

                            // Metrics upkeep
                            mFileDownloadVolumeFinal += fSDJ.mRangeCount; // This is the download volume required to complete the patch build.

                            #if 0 // (EAPATCH_DEBUG >= 3)
                                EAPATCH_TRACE_FORMATTED("PatchBuilder::BuildDestinationFiles: Job added. RangeBegin: %9I64u, RangeCount: %9I64u, DestPath: %s.\n", fSDJ.mRangeBegin, fSDJ.mRangeCount, fSDJ.mDestPath.c_str());
                            #endif
                        }
                        else if((bfs.mSourceFileType == kFileTypeLocal) && (bfs.mCountWritten < bfs.mCount))
                        {
                            mFileCopyVolumeFinal += (bfs.mCount - bfs.mCountWritten); // This is the copied file volume required to complete the patch build.
                        }
                    }
                }
            }
        } // For each entry in mPatchEntryArray... 

        // mFileSpanDownloadJobList now contains a list of all remote file segments we want to download.
        // We set a callback to notify us before individual download jobs begin and after they end. We'll use
        // this callback to open the files to be written and close them as-needed.
        mHTTP.SetEventHandler(HandleBuildFileHTTPEventStatic, (intptr_t)this);

        // We need to pass an array of pointers to httpClient, and not objects, 
        // so we make an array of pointers here.
        HTTP::DownloadJobPtrArray downloadJobPtrArray;

        for(FileSpanDownloadJobList::iterator it = mFileSpanDownloadJobList.begin(); it != mFileSpanDownloadJobList.end(); ++it)
        {
            FileSpanDownloadJob& fSDJ = *it;
            downloadJobPtrArray.push_back(&fSDJ);
        }

        // The following could take a while, especially if we have it set to retry upon network loss.
        // It could conceivably block forever until we cancel it in this case.
        mHTTP.GetFilesBlocking(downloadJobPtrArray.data(), downloadJobPtrArray.size());
        // We effectively check the result of this GetFilesBlocking below in a call to ShouldContinue.

        // At this point we've downloaded and written all the remote file spans, or failed doing so.
        for(FileSpanDownloadJobList::iterator it = mFileSpanDownloadJobList.begin(); it != mFileSpanDownloadJobList.end(); ++it)
        {
            FileSpanDownloadJob& fSDJ = *it;

            #if EAPATCH_ASSERT_ENABLED
                const int32_t useCount = fSDJ.mpBuiltFileInfo->mFileUseCount;
                const bool    isOpen   = fSDJ.mpBuiltFileInfo->mFileStreamTemp.IsOpen();

			    EAPATCH_ASSERT_FORMATTED(((useCount == 0) && !isOpen) || 
                                         ((useCount != 0) &&  isOpen), 
                                         ("PatchBuilder::BuildDestinationFiles: file use/open failure for %s: use count: %d, open: %s", 
                                         fSDJ.mDestPath.c_str(), useCount, isOpen ? "yes" : "no"));
            #endif

            if(fSDJ.mpBuiltFileInfo->mFileStreamTemp.IsOpen())
                fSDJ.mpBuiltFileInfo->mFileStreamTemp.Close();
        }

        #if 0 // (EAPATCH_DEBUG >= 3)
            {
                char     debugBuffer[16];
                uint64_t readSizeResult;
                fSDJ.mpBuiltFileInfo->mFileStreamTemp.ReadPosition(debugBuffer, 16, 0, readSizeResult, true);
                if(debugBuffer[0] != 'B')
                    EA_DEBUG_BREAK();
            }
        #endif

        // For special debug builds, check to make sure everything was downloaded and written as-expected.
        #if 0 // (EAPATCH_DEBUG >= 3)
            for(eastl_size_t i = 0, iEnd = mPatchImpl.mPatchEntryArray.size(); (i != iEnd) && ShouldContinue(); i++) // For each file in the patch...
            {
                BuiltFileInfo& bfi = mBuiltFileInfoArray[i];

                if(bfi.mLocalFilePath.find("\\data7.big") != EA::Patch::String::npos) // If it's the one file we are interested in...
                {
                    FileComparer fileComparer;

                    if(fileComparer.Open("D:\\Temp\\EAPatchBugReport\\BigFileTestData2Patch\\data7.big", 
                                         "D:\\Temp\\EAPatchBugReport\\BigFileTestDataResult\\data7.big.eaPatchTemp"))
                    {
                        // Walk through BuiltFileSpanList. 
                        // If the span is kFileTypeRemote, there is probably nothing to do that wasn't done above.
                        for(BuiltFileSpanList::const_iterator it = bfi.mBuiltFileSpanList.begin(); it != bfi.mBuiltFileSpanList.end(); ++it)
                        {
                            const BuiltFileSpan& bfs = *it;

                            if(bfs.mSourceFileType == kFileTypeRemote)
                            {
                                bool bEqual;
                                fileComparer.CompareSpan(bfs.mSourceStartPos, bfs.mDestStartPos, bfs.mCount, bEqual);
                                if(!bEqual)
                                    EAPATCH_TRACE_MESSAGE("PatchBuilder::BuildDestinationFiles: Segment inserted doesn't match file contents.\n");
                            }
                        }
                    }
                }
            }
        #endif

        // Now we write all the local spans that haven't been written yet.
        EAPATCH_ASSERT(!mDiskWriteDataRateMeasure.IsStarted());
        mDiskWriteDataRateMeasure.Start();

        for(eastl_size_t i = 0, iEnd = mPatchImpl.mPatchEntryArray.size(); (i != iEnd) && ShouldContinue(); i++) // For each file in the patch...
        {
            const PatchEntry& patchEntry = mPatchImpl.mPatchEntryArray[i];

            if(!patchEntry.mbFileUnchanged) // If this dest file needs updating...
            {
                BuiltFileInfo& bfi = mBuiltFileInfoArray[i];

                if(bfi.mLocalSpanWritten < bfi.mLocalSpanCount) // If there is more to copy for this file...
                {
                    FileSpanCopier fileSpanCopier;

                    if(fileSpanCopier.Open(bfi.mLocalFilePath.c_str(), bfi.mDestinationFilePathTemp.c_str()))
                    {
                        for(BuiltFileSpanList::iterator it = bfi.mBuiltFileSpanList.begin(); it != bfi.mBuiltFileSpanList.end(); ++it) // For each local span in the file...
                        {
                            BuiltFileSpan& bfs = *it;

                            if((bfs.mSourceFileType == kFileTypeLocal) && (bfs.mCountWritten < bfs.mCount)) // If there is more to write for this span...
                            {
                                const uint64_t sourceStartPos = bfs.mSourceStartPos + bfs.mCountWritten;
                                const uint64_t destStartPos   = bfs.mDestStartPos + bfs.mCountWritten;
                                const uint64_t copyCount      = bfs.mCount - bfs.mCountWritten;

                                fileSpanCopier.CopySpan(sourceStartPos, destStartPos, copyCount);

                                if(fileSpanCopier.HasError())
                                    TransferError(fileSpanCopier);
                                else
                                {
                                    bfs.mCountWritten     += copyCount;
                                    bfi.mLocalSpanWritten += copyCount;
                                    mDiskWriteDataRateMeasure.ProcessData(copyCount * 2); // *2 because we are reading N bytes and writing N bytes, and we measure both.

                                    // Update metrics
                                    mFileCopyVolume += copyCount;
                                    const double stateCompletion = (double)(mFileDownloadVolume + mFileCopyVolume) / (double)(mFileDownloadVolumeFinal + mFileCopyVolumeFinal); // Yields a value between 0 and 1, which indicates how far through the current state alone we are.
                                    mProgressValue = kStateCompletionValue[kStateBuildingDestinationFiles] + (stateCompletion * (kStateCompletionValue[kStateBuildingDestinationFiles + 1] - kStateCompletionValue[kStateBuildingDestinationFiles]));
                                    NotifyProgress();
                                }
                            }
                        } // For each local span in the file.

                        fileSpanCopier.Close();

                        // Write the .eaPatchBFI (built file info file), which is simply bfi serialized to disk.
                        const String            sDestinationBFIFilePath(bfi.mDestinationFilePath + kPatchBFIFileNameExtension);    // e.g. /GameData/Data1.big.eaPatchBFI
                        BuiltFileInfoSerializer bfiSerializer;

                        bfiSerializer.Serialize(bfi, sDestinationBFIFilePath.c_str());

                        if(bfiSerializer.HasError())
                            TransferError(bfiSerializer);
                    }

                    if(fileSpanCopier.HasError())
                    {
                        if(mbSuccess) // Use the first reported error if it exists, not this one.
                            TransferError(fileSpanCopier);
                        fileSpanCopier.ClearError();
                    }

                } // If there was more to copy for this file.
                // Else the file is fully copied and the .eaPatchBFI file on disk should be up to date.

                // We should have caught any errors already, but we do this to make sure.
                if(bfi.mFileStreamTemp.HasError())
                    TransferError(bfi.mFileStreamTemp);

                // To do: Verify the disk file's hash against patchEntry.mFileHashValue.
                // Is there a way to calculate the hash as it's being written to disk instead of afterwards?               
                // DiffData::mBlockHashArray could provide a way to calculate or validate the hash as we go.
            }

        } // For each file in mBuiltFileInfoArray... 

        mDiskWriteDataRateMeasure.Stop();

    } // if(mbSuccess)

    return mbSuccess;    
}


bool PatchBuilder::RenameOriginalFiles()
{
    if(ShouldContinue())
    {
        SetState(kStateRenamingOriginalFiles);

        if(mbInPlacePatch) // If the patch is replacing a current installation as opposed to writing a new installation and leaving the old one alone...
        {
            // Need to rename not just files being replaced, but also files that will be being 
            // removed due to not being part of the patch and thus aren't being replaced by anything.
            // All the files that we want to leave as-is have names that end with .eaPatchXXX (e.g. .eaPatchDiff)
            // and files that match the mPatchImpl.mIgnoredFileArray/mUsedFileArray filters. We want to flag all other files 
            // and directories for removal by renaming them to end with .eaPatchOld. Later after we've completed
            // all the required renaming, we'll do a round of removing these files.

            FileInfoList usedFileList;
            String       sLocalFilePathRenamed;

            // The list we use here is based on the file system and not mPatchImpl.mPatchEntryArray. The reason for
            // this is that mPatchEntryArray lists only files that will be in the new patch image retrieved from 
            // the server, and doesn't include local files that might need to be removed as part of the patching process.
            if(GetFileList(mPatchBaseDirectoryDest.c_str(), mPatchImpl.mIgnoredFileArray, NULL, mPatchImpl.mUsedFileArray, &usedFileList, NULL, true, false, mError)) // Get a list of just the relevant files for the patch, which we'll rename below to .eaPatchOld.
            {
                for(FileInfoList::iterator it = usedFileList.begin(), itEnd = usedFileList.end(); (it != itEnd) && ShouldContinue(); ++it)
                {
                    const FileInfo& fileInfo = *it;

                    if(fileInfo.mbFile) // We don't rename directories that are part of our patch set.
                    {
                        if(!PathMatchesAnInternalFileType(fileInfo.mFilePath.c_str())) // If the file isn't one of our internal types (e.g. .eaPatchDiff)...
                        {
                            // Rename the local file from (e.g.) SomeFile.big to SomeFile.big.eaPatchOld. 
                            sLocalFilePathRenamed = fileInfo.mFilePath + kPatchOldFileNameExtension;

                            //if(fileInfo.mbFile) // If this is a file as opposed to a directory...
                            //{
                                if(!FileRename(fileInfo.mFilePath.c_str(), sLocalFilePathRenamed.c_str(), true)) // If we failed to rename the file...
                                {
                                    if(EA::IO::File::Exists(fileInfo.mFilePath.c_str())) // If the file still in fact exists at the time of the above Rename...
                                    {
                                        const String sError(fileInfo.mFilePath + " -> " + sLocalFilePathRenamed);
                                        HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRenameError, sError.c_str()); // This will set mbSuccess to false.
                                    }
                                }
                                // Else the original file didn't exist. That's not necessarily an error, and even if it was an error it should be something we neither caused nor could do anything about.
                            //}
                            //else
                            //{
                            //    if(!DirectoryRename(fileInfo.mFilePath.c_str(), sLocalFilePathRenamed.c_str())) // If we failed to rename the directory...
                            //    {
                            //        if(EA::IO::Directory::Exists(fileInfo.mFilePath.c_str())) // If the directory still in fact exists at the time of the above Rename...
                            //        {
                            //            const String sError(fileInfo.mFilePath + " -> " + sLocalFilePathRenamed);
                            //            HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdDirRenameError, sError.c_str()); // This will set mbSuccess to false.
                            //        }
                            //    }
                            //    // Else the original file didn't exist. That's not necessarily an error, and even if it was an error it should be something we neither caused nor could do anything about.
                            //}
                        }
                    }
                }
            }
            else
            {
                mbSuccess = false;
              //mError was already set above.
            }


            // We may somehow (pathologically?) have the situation where a file that is in our patch set was also set to 
            // be ignored via the filters, in which case we'll have a file that wasn't renamed above but we have a patch file 
            // that's about to try to replace it. We can handle this in the most graceful way by doing a pass through mPatchImpl.mPatchEntryArray
            // and rename any of those files to .eaPatchOld. And log this finding so we can see the unexpected behavior. I think 
            // the main case this could happen is if a user error was made while setting up the patch in the first place via PatchMaker.
            String sLocalFilePath;

            for(eastl_size_t i = 0, iEnd = mPatchImpl.mPatchEntryArray.size(); (i != iEnd) && ShouldContinue(); i++)
            {
                const PatchEntry& patchEntry = mPatchImpl.mPatchEntryArray[i];

                sLocalFilePath = mPatchBaseDirectoryDest + patchEntry.mRelativePath;

                if(EA::IO::File::Exists(sLocalFilePath.c_str()))
                {
                    // Rename the local file from (e.g.) SomeFile.big to SomeFile.big.eaPatchOld. 
                    sLocalFilePathRenamed = sLocalFilePath + kPatchOldFileNameExtension;

                    if(!FileRename(sLocalFilePath.c_str(), sLocalFilePathRenamed.c_str(), true)) // If there was an error in the renaming...
                    {
                        if(EA::IO::File::Exists(sLocalFilePath.c_str())) // If the file still in fact exists at the time of the above Rename...
                            HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRenameError, sLocalFilePath.c_str()); // This will set mbSuccess to false.
                    }
                }
                // Else the file was renamed as expected earlier in this function.
            }
        } // if(mbInPlacePatch)
    } // if(mbSuccess)

    return mbSuccess;
}


bool PatchBuilder::RenameDestinationfiles()
{
    if(ShouldContinue())
    {
        SetState(kStateRenamingDestinationFiles);

        // We need to rename the patch files from their temp names to their final names. 
        // This means simply stripping the .eaPatchTemp file extension from their name. 
        // There should be no files named .eaPatchTemp in this file tree other than these files.

        for(eastl_size_t i = 0, iEnd = mPatchImpl.mPatchEntryArray.size(); (i != iEnd) && ShouldContinue(); i++)
        {
            const PatchEntry& patchEntry = mPatchImpl.mPatchEntryArray[i];

            if(!patchEntry.mbFileUnchanged) // If this dest file needed updating...
            {
                const String sFinalName     = mPatchBaseDirectoryDest + patchEntry.mRelativePath;
                const String sFinalNameTemp = sFinalName + kPatchTempFileNameExtension;
                const bool   origFileExists = patchEntry.mbFile ? EA::IO::File::Exists(sFinalName.c_str())     : EA::IO::Directory::Exists(sFinalName.c_str());
                const bool   tempFileExists = patchEntry.mbFile ? EA::IO::File::Exists(sFinalNameTemp.c_str()) : EA::IO::Directory::Exists(sFinalNameTemp.c_str());

                EAPATCH_ASSERT(!origFileExists && tempFileExists); // If this fails then somebody messed with the directory while we were working on it.

                if(!origFileExists && tempFileExists)
                {
                    bool renameSucceeded = patchEntry.mbFile ? FileRename(sFinalNameTemp.c_str(), sFinalName.c_str(), false) :
                                                               DirectoryRename(sFinalNameTemp.c_str(), sFinalName.c_str());
                    if(!renameSucceeded)
                    {
                        // Serious error. Need to restart patching process or cancel it.
                        const String sError(sFinalNameTemp + " -> " + sFinalName);
                        HandleError(kECFatal, GetLastSystemErrorId(), patchEntry.mbFile ? kEAPatchErrorIdFileRenameError : kEAPatchErrorIdDirRenameError, sError.c_str());
                    }
                }
                else
                {
                    // Serious error. Need to restart patching process or cancel it. This shouldn't happen unless
                    // some outside entity messed with our directory tree while we were executing.
                    if(!tempFileExists)
                        HandleError(kECFatal, kSystemErrorIdNone, patchEntry.mbFile ? kEAPatchErrorIdExpectedFileMissing : kEAPatchErrorIdExpectedDirMissing, sFinalNameTemp.c_str());
                    else if(origFileExists)
                        HandleError(kECFatal, kSystemErrorIdNone, patchEntry.mbFile ? kEAPatchErrorIdUnexpectedFilePresent : kEAPatchErrorIdUnexpectedDirPresent, sFinalName.c_str());
                }
            }
        }

        #if (EAPATCH_DEBUG >= 2) // If using extra debug mode... Verify there are no .eaPatchTemp files in our used tree.
            FileInfoList usedFileList;

            if(GetFileList(mPatchBaseDirectoryDest.c_str(), mPatchImpl.mIgnoredFileArray, NULL, mPatchImpl.mUsedFileArray, &usedFileList, NULL, true, false, mError)) // Get a list of just the relevant files for the patch, which we'll rename below to .eaPatchOld.
            {
                for(FileInfoList::iterator it = usedFileList.begin(), itEnd = usedFileList.end(); (it != itEnd) && ShouldContinue(); ++it)
                {
                    const FileInfo& fileInfo = *it;

                    if(fileInfo.mFilePath.find(kPatchTempFileNameExtension) != String::npos) // If the file's name ends with .eaPatchTemp
                    {
                        mbSuccess = false;
                        HandleError(kECFatal, kSystemErrorIdNone, fileInfo.mbFile ? kEAPatchErrorIdUnexpectedFilePresent : kEAPatchErrorIdUnexpectedDirPresent, fileInfo.mFilePath.c_str());
                    }
                }
            }
            else
            {
                mbSuccess = false;
              //mError was already set above.
            }
        #endif
    }

    return mbSuccess;
}


bool PatchBuilder::RemoveIntermediateFiles()
{
    if(ShouldContinue())
    {
        SetState(kStateRemovingIntermediateFiles);

        // We need to remove the .eaPatchDiff, .eaPatchBFI, etc. files, leaving the 
        // .eaPatchImpl file (which will later be renamed to .eaPatchInstalled).
        // We also leave the .eaPatchInfo file to be removed at the next step after this one, 
        // because we use the .eaPatchInfo file to help resume disrupted downloads, so we want
        // to delay its removal.
        // There should be no relevant files named .eaPatchTemp, as we couldn't possibly be in
        // a success state here if any such files existed.

        String sFilePathBase;
        String sFilePath;

        for(eastl_size_t i = 0, iEnd = mPatchImpl.mPatchEntryArray.size(); (i != iEnd) && ShouldContinue(); i++)
        {
            const PatchEntry& patchEntry = mPatchImpl.mPatchEntryArray[i];

            sFilePathBase = mPatchBaseDirectoryDest + patchEntry.mRelativePath;

            // Remove the .eaPatchDiff file.
            sFilePath = sFilePathBase + kPatchDiffFileNameExtension;

            if(EA::IO::File::Exists(sFilePath.c_str()))
            {
                if(!FileRemove(sFilePath.c_str())) // If there was an error in the removing...
                    HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRemoveError, sFilePath.c_str()); // This will set mbSuccess to false.
            }

            // Remove the .eaPatchBFI file.
            sFilePath = sFilePathBase + kPatchBFIFileNameExtension;

            if(EA::IO::File::Exists(sFilePath.c_str()))
            {
                if(!FileRemove(sFilePath.c_str())) // If there was an error in the removing...
                    HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRemoveError, sFilePath.c_str()); // This will set mbSuccess to false.
            }
        }

        // To consider: Implement some (EAPATCH_DEBUG >= 2) code here which validates that there are
        // no relevant .eaPatchTemp files in the destination tree. If such files existed then that would
        // indicate that we didn't complete some earlier patching step successfully.
    }

    return mbSuccess;
}


bool PatchBuilder::RemoveOriginalFiles()
{
    if(ShouldContinue())
    {
        SetState(kStateRemovingOriginalFiles);

        if(mbInPlacePatch) // If the patch is replacing a current installation as opposed to writing a new installation and leaving the old one alone...
        {
            // We need to remove all the original files that were named .eaPatchOld.
            // To consider: Merge this implementation with RenameOriginalFiles(), as they are pretty similar.

            FileInfoList usedFileList;
            String       sLocalFilePathRenamed;

            // The list we use here is based on the file system and not mPatchImpl.mPatchEntryArray. The reason for
            // this is that mPatchEntryArray lists only files that will be in the new patch image retrieved from 
            // the server, and doesn't include local files that might need to be removed as part of the patching process.

            const StringList eaPatchOldStringList(1, String(kPatchOldFileNameExtension)); //.eaPatchOld files.

            if(GetFileList(mPatchBaseDirectoryDest.c_str(), mPatchImpl.mIgnoredFileArray, NULL, 
                            mPatchImpl.mUsedFileArray, &usedFileList, &eaPatchOldStringList, true, false, mError)) // Get a list of just the relevant files for the patch, which we'll rename below to .eaPatchOld.
            {
                for(FileInfoList::iterator it = usedFileList.begin(), itEnd = usedFileList.end(); (it != itEnd) && ShouldContinue(); ++it)
                {
                    const FileInfo& fileInfo = *it;

                    const char8_t* pFileNameExtension = EA::IO::Path::GetFileExtension(fileInfo.mFilePath.c_str());

                    if(EA::StdC::Stricmp(pFileNameExtension, kPatchOldFileNameExtension) == 0) // If this is a .eaPatchOld file...
                    {
                        if(fileInfo.mbFile)
                        {
                            if(EA::IO::File::Exists(fileInfo.mFilePath.c_str())) // If the file still in fact exists at the time of the above Remove...
                            {
                                if(!FileRemove(fileInfo.mFilePath.c_str()))
                                    HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRemoveError, fileInfo.mFilePath.c_str()); // This will set mbSuccess to false.
                            }
                            // Else the original file didn't exist. That's not necessarily an error, and even if it was an error it should be something we neither caused nor could do anything about.
                        }
                        else
                        {
                            if(EA::IO::Directory::Exists(fileInfo.mFilePath.c_str())) // If the directory still in fact exists at the time of the above Rename...
                            {
                                if(!DirectoryRemove(fileInfo.mFilePath.c_str(), true))
                                    HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdDirRemoveError, fileInfo.mFilePath.c_str()); // This will set mbSuccess to false.
                            }
                            // Else the original directory didn't exist. That's not necessarily an error, and even if it was an error it should be something we neither caused nor could do anything about.
                        }
                    }
                }
            }
            else
            {
                mbSuccess = false;
              //mError was already set above.
            }


            // We may somehow (pathologically?) have the situation where a file that is in our patch set was also set to 
            // be ignored via the filters, in which case we'll have a file that wasn't renamed above but we have a patch file 
            // that's about to try to replace it. We can handle this in the most graceful way by doing a pass through mPatchImpl.mPatchEntryArray.

            for(eastl_size_t i = 0, iEnd = mPatchImpl.mPatchEntryArray.size(); (i != iEnd) && ShouldContinue(); i++)
            {
                const PatchEntry& patchEntry = mPatchImpl.mPatchEntryArray[i];

                sLocalFilePathRenamed = mPatchBaseDirectoryDest + patchEntry.mRelativePath + kPatchOldFileNameExtension;

                if(EA::IO::File::Exists(sLocalFilePathRenamed.c_str()))
                {
                    if(!FileRemove(sLocalFilePathRenamed.c_str())) // If there was an error in the removing...
                        HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRemoveError, sLocalFilePathRenamed.c_str()); // This will set mbSuccess to false.
                }
                // Else the file was removed as expected earlier in this function.
            }
        } // if(mbInPlacePatch)


        // Remove the .eaPatchInfo file if it's present.
        if(EA::IO::File::Exists(mPatchInfoFilePath.c_str()))
        {
            if(!FileRemove(mPatchInfoFilePath.c_str())) // If there was an error in the removing...
                HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRemoveError, mPatchInfoFilePath.c_str()); // This will set mbSuccess to false.
        }

    } // if(mbSuccess)

    return mbSuccess;
}


bool PatchBuilder::WritePatchInstalledFile()
{
    if(ShouldContinue())
    {
        SetState(kStateWritingPatchInstalledFile);

        // We rename the .eaPatchImpl file to .eaPatchInstalled.
        // This should be the only remaining administrative file upon the completion of the patching process.
        // The very last thing we do is remove the .eaPatchState file, which needs to be done after .eaPatchInstalled is written.

        String sFilePathInstalled  = mPatchBaseDirectoryDest + mPatchInfoFileName;      // e.g. /Root/Data/SomePath.eaPatchInfo
        StripFileNameExtensionFromFilePath(sFilePathInstalled);                         // e.g. /Root/Data/SomePath
        sFilePathInstalled  += kPatchInstalledFileNameExtension;                        // e.g. /Root/Data/SomePath.eaPatchInstalled

        if(EA::IO::File::Exists(mPatchImplFilePath.c_str()))
        {
            // Disabled because Rename below handles overwriting a previously-existing file.
            //if(EA::IO::File::Exists(sFilePathInstalled.c_str())) // This really shouldn't exist.
            //{
            //    if(!FileRemove(sFilePathInstalled.c_str())) // We try to be friendly and remove it.
            //        HandleError(kECBlocking, kSystemErrorIdNone, kEAPatchErrorIdUnexpectedFilePresent, sFilePathInstalled.c_str()); // This will set mbSuccess to false.
            //}

            // To consider: Remove unnecessary XML data from the .eaPatchImpl file in order to make it smaller.

            // Rename .eaPatchImpl to .eaPatchInstalled.
            if(!FileRename(mPatchImplFilePath.c_str(), sFilePathInstalled.c_str(), true)) // If there was an error in the removing...
            {
                const String sError(mPatchImplFilePath + " -> " + sFilePathInstalled);
                HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRenameError, sError.c_str()); // This will set mbSuccess to false.
            }
        }
        else
            HandleError(kECBlocking, kSystemErrorIdNone, kEAPatchErrorIdExpectedFileMissing, mPatchImplFilePath.c_str());
    }

    return mbSuccess;
}


bool PatchBuilder::Finalize()
{
    if(ShouldContinue())
    {
        SetState(kStateFinalizing);

        // Delete the .eaPatchState file.
        if(!FileRemove(mPatchStateFilePath.c_str()))
            HandleError(kECBlocking, kSystemErrorIdNone, kEAPatchErrorIdFileRemoveError, mPatchStateFilePath.c_str()); // This will set mbSuccess to false.

        if(mAsyncStatus == kAsyncStatusStarted)
            mAsyncStatus = kAsyncStatusCompleted;
        // Else it is a cancelled, deferred, or aborted state, so leave it like that.
    }
    else
    {
        if(mAsyncStatus == kAsyncStatusStarted)
        {
            EAPATCH_ASSERT(!mbSuccess); // kAsyncStatusAborted means an error occurred, so we verify that's what happened.
            mAsyncStatus = kAsyncStatusAborted;
        }
    }

    mEndTimeMs = GetTimeMilliseconds();
    TelemetryNotifyPatchBuildEnd();

    return mbSuccess;
}


bool PatchBuilder::EndPatch()
{
    if(ShouldContinue())
        SetState(kStateEnd);

    return mbSuccess;
}


// Cancels the application of a patch and attempts to restore the destination directory to 
// its state prior to the patch application. 
bool PatchBuilder::Cancel(bool waitForCompletion, bool& bPatchCancelled)
{
    //mFutex.Lock(); Need to figure out the right way of doing this.

    // We have no choice but to call Stop with its waitForCompletion arg set to true, 
    // though we can put the Stop in CancelPatchBlocking once our code has been
    // updated here to create a thread to do the cancellation.

    // If this is calling during patching, and we are past a certain state (kStateRemovingOriginalFiles), 
    // then ignore the request to cancel, as we were too close to finishing.

    if(mState < kStateRemovingOriginalFiles)
    {
        if(Stop(true, bPatchCancelled) && bPatchCancelled)
        {
            // To do: Replace this blocking call with code that creates a thread 
            // which implements this blocking call itself.
            EA_UNUSED(waitForCompletion);
            CancelPatchBlocking();
        }
    }
    else
    {
        // Else ignore the request, as we are too close to finishing, and we can't undo a patch after kStateRemovingOriginalFiles.
        // Aside from pathological cases, this state will take only milliseconds or maybe a couple second to complete in most cases.
        //mFutex.Unlock();
    }

    return mbSuccess;
}


bool PatchBuilder::CancelPatchBlocking()
{
    // We have a problem with this 'mbSuccess': If the app calls us right after calling BuildPatchBlocking but,
    // BuildPatchBlocking had an error and set mbSuccess to false, then we will start executing here with mbSuccess
    // still set to false. So we don't check the value of mbSuccess here, but rather just Reset and start cancelling.

    //if(mbSuccess)
    {
        // Execution Steps
        //
        // These steps need to be crash-resistant the same way that patching needs to be able to deal with
        // crashes. If the app crashes during patch cancelling then we need to be able to resume the
        // cancellation upon restart of the app. And ideally even be able to go back and resume installing
        // the patch which was cancelled. It should help to look at the EAPatch Technical Design doc, 
        // which has illustrations showing each of the patch building steps.
        //
        // Our approach is to follow the opposite steps that we followed for building the patch, starting where we 
        // last were building the patch and working backwards. We know where we last were because the .eaPatchState
        // file tells us about it. The State enum is a list of ordered states. Start at the last one we were at 
        // and do the operations in reverse. Undoing the operations is much simpler than doing them, because we need
        // do no file calculations or network downloads. We just need to rename and delete files. Also, we don't need
        // to do every step backwards and can skip some steps.
        //
        // It's likely impossible to restore the directory if the previous patch state was kStateRemovingOriginalFiles.
        // This is because at that point the patch is fully applied and kStateRemovingOriginalFiles consists of deleting
        // the original files which were saved as .eaPatchOld. Our best solution in this situation is to do the opposite
        // of restore the original patch and instead continue with the patch installation by removing the .eaPatchOld files.
        // That has the benefit of cleaning up the directory and letting the user undo the patch by installing a previous
        // patch instead of approaching the problem by cancelling the current patch.
        //
        // There are some pathological cases where we cannot reliably cancel a patch due to the expected files being
        // corrupted or removed (e.g by a user or by an anti-virus program). We attempt to avoid this by making such 
        // critical files more obviously named and not have binary data that would trigger an anti-virus program.
        // But if we still run into this case then we need to have some kind of cleanup script that nukes the patch 
        // directory and does a clean restore or install. This probably won't happen much or at all on closed file 
        // systems like those on console platforms, but could possibly happen on PC platforms in rare cases.

        // Do some setup.
        ResetSession();

        mStartTimeMs    = GetTimeMilliseconds();
        mBuildDirection = kBDCancelling;
        mPatchCourse    = kPCNone;
        mAsyncStatus    = kAsyncStatusStarted;

        // If the user never called SetPatchInfo, then we need to cancel whatever patch is in mPatchBaseDirectoryDest.
        bool bPatchFound = true;

        if(mPatchInfoFileName.empty() && mPatchInfo.mPatchInfoURL.empty() && !mPatchBaseDirectoryDest.empty())  // If SetupDirectoryPaths below won't be able to determine the .eaPatchInfo info itself...
        {
            String sPattern = "*"; sPattern += kPatchStateFileNameExtension; // "*.eaPatchState

            if(FilePatternExists(mPatchBaseDirectoryDest.c_str(), sPattern.c_str(), &mPatchInfoFileName))
            {
                const eastl_size_t fileNamePos = (eastl_size_t)(EA::IO::Path::GetFileName(mPatchInfoFileName.c_str()) - mPatchInfoFileName.c_str());

                mPatchInfoFileName.erase(0, fileNamePos);
                mPatchInfoFileName.resize(mPatchInfoFileName.size() - EA::StdC::Strlen(kPatchStateFileNameExtension)); // Strip off kPatchStateFileNameExtension.
                mPatchInfoFileName += kPatchInfoFileNameExtension;
                // It doesn't matter if the file name actually exists, as our code currently just cares about the base file name of it.

                bPatchFound = true;
            }
            else
                bPatchFound = false;
        }

        if(mPatchImplURL.empty() && !mPatchInfo.mPatchImplURL.Count() && !mPatchBaseDirectoryDest.empty()) // If SetupDirectoryPaths below won't be able to determine the .eaPatchImpl info itself...
        {
            String sPattern = "*"; sPattern += kPatchImplFileNameExtension; // "*.eaPatchImpl

            FilePatternExists(mPatchBaseDirectoryDest.c_str(), sPattern.c_str(), &mPatchImplFilePath); // Ignore the return value.
            mPatchImplURL = "(unused)"; // This is an intentionally wrong but non-empty value. It exists only to placate the SetupDirectoryPaths call below. To do: Make this handling cleaner.
        }

        if(bPatchFound)
        {
            SetupDirectoryPaths();

            // To do: Load mPatchInfo from disk if it looks like SetPatchInfo was not called.

            // Retrieve the previous state of the patch.
            State persistedState = kStateNone;          // This is the state (if any) that was last written to disk).
            ReadPatchStateFile(persistedState, true);
            if((persistedState < kStateNone) || (persistedState > kStateEnd)) // If the state is invalid (corrupted)...
                persistedState = kStateRemovingIntermediateFiles; // To do: Come up with a better solution to this.

            // Read the .eaPatchImpl file if it's present.
            // Read the .eaPatchImpl file into mPatchImpl. Under pathological cases it might not exist, 
            // such as the user messing with the file system, and we try to deal with that.

            if(EA::IO::File::Exists(mPatchImplFilePath.c_str()))
            {
                // Deserialize the disk file into mPatchImpl.
                PatchImplSerializer patchImplSerializer;
                patchImplSerializer.Deserialize(mPatchImpl, mPatchImplFilePath.c_str(), false, true); // We'll check the return value below with HasError().

                if(patchImplSerializer.HasError())
                    patchImplSerializer.EnableErrorAssertions(false);

                if(mPatchImpl.HasError())
                    mPatchImpl.EnableErrorAssertions(false);
            }

            TelemetryNotifyPatchCancelBegin();

            SetState(persistedState);

            switch (mState)
            {
                // We can't undo the patch if it's at >= kStateRemovingOriginalFiles, as the patch is installed and we were last doing 
                // some cleanup of the old files. We complete the installation of the original patch. To consider: make this optional.
                case kStateEnd:                         // 
                case kStateFinalizing:                  // During patching, this step removed the .eaPatchState file.
                case kStateWritingPatchInstalledFile:   // During patching, this step renamed .eaPatchImpl to .eaPatchInstalled.
                case kStateRemovingOriginalFiles:       // During patching, this step implemented the deletion of .eaPatchOld files.
                    BuildPatchBlocking();
                    break;

                case kStateRemovingIntermediateFiles:
                    UndoRemoveIntermediateFiles();   // Fall through

                case kStateRenamingDestinationFiles:
                    UndoRenameDestinationFiles();    // Fall through

                case kStateRenamingOriginalFiles:
                    UndoRenameOriginalFiles();       // Fall through

                case kStateBuildingDestinationFiles:
                    UndoBuildDestinationFiles();     // Fall through

                case kStateBuildingDestinationFileInfo:
                    UndoBuildDestinationFileInfo();  // Fall through

                case kStateDownloadingPatchDiffFiles:
                    UndoDownloadPatchDiffFiles();    // Fall through

                case kStateProcessingPatchImplFile:
                    UndoProcessPatchImplFile();      // Fall through

                case kStateDownloadingPatchImplFile:
                    UndoDownloadPatchImplFile();     // Fall through

                case kStateWritingPatchInfoFile:
                    UndoWritePatchInfoFile();        // Fall through

                case kStateBegin:
                    UndoStateBegin();                // Fall through

                case kStateNone:
                    // This should never happen except for pathological cases.
                    break;
            }
        }
        else
        {
            // There was no patch to be found. It was either a mistake to try to run a patch cancel on 
            // this directory, or the patch application was corrupted. How do we deal with this? 
            EAPATCH_FAIL_MESSAGE("PatchBuilder::CancelPatchBlocking: There was no patch to be found. It was either a mistake to try to run a patch cancel on this directory, or the patch application was corrupted.");
        }

        if(mAsyncStatus == kAsyncStatusStarted)
            mAsyncStatus = kAsyncStatusCompleted;
        else if(mAsyncStatus == kAsyncStatusCancelling)
            mAsyncStatus = kAsyncStatusCancelled;
        else if(mAsyncStatus != kAsyncStatusDeferred)
            mAsyncStatus = kAsyncStatusAborted;
        // Else leave it as-is.
    }

    // Clear some possible error states.
    EAPATCH_ASSERT(!mPatchInfo.HasError() || !mbSuccess); // If mPatchInfo has an error then mbSuccess better be false.
    mPatchInfo.EnableErrorAssertions(false);

    EAPATCH_ASSERT(!mPatchImpl.HasError() || !mbSuccess);
    mPatchImpl.EnableErrorAssertions(false);

    EAPATCH_ASSERT(!mHTTP.HasError() || !mbSuccess);
    mHTTP.EnableErrorAssertions(false);

    mEndTimeMs = GetTimeMilliseconds();
    TelemetryNotifyPatchCancelEnd();

    return mbSuccess;
}


bool PatchBuilder::UndoRemoveIntermediateFiles()
{
    if(mbSuccess)
    {
        // During patching, this step caused deletion of .eaPatchInfo, .eaPatchDiff, .eaPatchBFI files.
        // Nothing to do. If we were to truly undo this step then we would be recreating
        // intermediate files, which is a non-sensical thing to do. But we are nuking a patch, 
        // not gently un-applying it backwards. We remove the intermediate files below in 
        // a later step which is the opposite of the patching step that created them. Actually
        // it makes some sense to restore the .eaPatchInfo file, despite that it will eventually
        // get deleted again below. The reason is that it allows us to abort this patch cancel and 
        // resume it later in the case that the patch process was aborted right after the .eaPatchInfo
        // file was deleted.

        if(!EA::IO::File::Exists(mPatchInfoFileName.c_str()))
        {
            FileCopier fileCopier;

            fileCopier.CopyFile(mPatchImplFilePath.c_str(), mPatchInfoFilePath.c_str());
            fileCopier.EnableErrorAssertions(false); // Just ignore errors, as we are in a messed up situation in any case.
        }

        SetState(kStateRenamingDestinationFiles);
    }

    return mbSuccess;
}


bool PatchBuilder::UndoRenameDestinationFiles()
{
    if(mbSuccess)
    {
        // During patching, this step implemented the naming of (e.g.) Blah.big.eaPatchTemp to Blah.big.
        // To undo this step, we can rename these files back to .eaPatchTemp, but we can also delete them
        // outright, as we're just going to delete the .eaPatchTemp files in a later step anyway.

        const bool bPatchImplUsable = !mPatchImpl.mPatchId.empty();

        if(bPatchImplUsable)
        {
            FileInfoList usedFileList;
            String       sLocalFilePathTemp;

            // The list we use here is based on the file system and not mPatchImpl.mPatchEntryArray. The reason for
            // this is that mPatchEntryArray lists only files that will be in the new patch image retrieved from 
            // the server, and doesn't include local files that might need to be removed as part of the patching process.
            if(GetFileList(mPatchBaseDirectoryDest.c_str(), mPatchImpl.mIgnoredFileArray, NULL, mPatchImpl.mUsedFileArray, &usedFileList, NULL, true, false, mError)) // Get a list of just the relevant files for the patch, which we'll rename below to .eaPatchOld.
            {
                for(FileInfoList::iterator it = usedFileList.begin(), itEnd = usedFileList.end(); (it != itEnd) && ShouldContinue(); ++it)
                {
                    const FileInfo& fileInfo = *it;

                    if(fileInfo.mbFile) // We don't rename directories that are part of our patch set.
                    {
                        if(!PathMatchesAnInternalFileType(fileInfo.mFilePath.c_str())) // If the file isn't one of our internal types (e.g. .eaPatchDiff)...
                        {
                            // We just remove it instead of renaming it to .eaPatchTemp. See the discussion above.
                            if(!FileRemove(fileInfo.mFilePath.c_str()))
                            {
                                if(EA::IO::File::Exists(fileInfo.mFilePath.c_str())) // If the file still in fact exists at the time of the above Rename...
                                    HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRemoveError, fileInfo.mFilePath.c_str()); // This will set mbSuccess to false.
                                // Else the original file didn't exist. That's not necessarily an error, and even if it was an error it should be something we neither caused nor could do anything about.
                            }

                            // The following code should be unnecessary and there really shouldn't be any such 
                            // temp files. But we're going to leave this code around a little while to see if 
                            // there are any such files, just as a sanity check. One possible cause for the 
                            // existence is a pathological pattern of cancelled patches and user directory manipulation.
                            sLocalFilePathTemp = fileInfo.mFilePath + kPatchTempFileNameExtension;
                            EAPATCH_ASSERT(!EA::IO::File::Exists(sLocalFilePathTemp.c_str()));

                            if(!FileRemove(sLocalFilePathTemp.c_str()))
                            {
                                if(EA::IO::File::Exists(sLocalFilePathTemp.c_str())) // If the file still in fact exists at the time of the above Rename...
                                    HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRemoveError, sLocalFilePathTemp.c_str()); // This will set mbSuccess to false.
                            }
                        }
                    }
                }
            }
            else
            {
                mbSuccess = false;
              //mError was already set above.
            }
        }
        else
        {
            // Else we have no way to identify these files except to find any .eaPatchOld files and assume
            // the .eaPatchOld files were replaced by their equivalent patch files. We might as well handle
            // this case in the step where we rename the .eaPatchOld files to their original names, by
            // deleting files of the original names if they exist.
            // To do.
        }

        SetState(kStateRenamingOriginalFiles);
    }

    return mbSuccess;
}


bool PatchBuilder::UndoRenameOriginalFiles()
{
    if(mbSuccess)
    {
        // During patching, this step implemented the naming of (e.g.) Blah.big to Blah.big.eaPatchOld.
        // We need to rename any .eaPatchOld files back to their original names.

        const bool bPatchImplUsable = !mPatchImpl.mPatchId.empty();

        if(bPatchImplUsable)
        {
            FileInfoList usedFileList;
            String       sLocalFilePathRenamed;

            // The GetFileList-derived list we use here is based on the file system and not mPatchImpl.mPatchEntryArray. 
            // The reason for this is that mPatchEntryArray lists only files that will be in the new patch image retrieved  
            // from the server, and doesn't include local files that might need to be removed as part of the patching process.

            // We need to find the set of .eaPatchOld files that need to be renamed back to their original names.
            // When the original files were renamed to .eaPatchOld in the RenameOriginalFiles function, we used
            // the GetFileList function to identify the original files. In order to identify those files that were
            // renamed to .eaPatchOld, we can use GetFileList again but GetFileList would need to see the files 
            // as their old name before appending .eaPatchOld, otherwise GetFileList won't necessarily recognize 
            // the .eaPatchOld files as matching the used files specified in the PatchImpl. It turns out that
            // GetFileList has an option for us to pass a set of file extensions which it strips before evaluating
            // whether a file is part of a patch set, and we use that feature with the .eaPatchOld extension. So 
            // any files in the directory named .eaPatchOld, whose name without that extension is part of the patch,
            // will be returned as part of the used file array.

            const StringList eaPatchOldStringList(1, String(kPatchOldFileNameExtension));

            if(GetFileList(mPatchBaseDirectoryDest.c_str(), mPatchImpl.mIgnoredFileArray, NULL, mPatchImpl.mUsedFileArray, &usedFileList, 
                            &eaPatchOldStringList, true, false, mError)) // Get a list of just the relevant files for the patch, which we'll rename below to .eaPatchOld.
            {
                for(FileInfoList::iterator it = usedFileList.begin(), itEnd = usedFileList.end(); (it != itEnd) && ShouldContinue(); ++it)
                {
                    const FileInfo& fileInfo = *it;

                    if(fileInfo.mbFile) // We don't rename directories that are part of our patch set.
                    {
                        const char8_t* pFileExtension = EA::IO::Path::GetFileExtension(fileInfo.mFilePath.c_str());

                        if(EA::StdC::Stricmp(pFileExtension, kPatchOldFileNameExtension) == 0) // If the file name ends in .eaPatchOld...
                        {
                            // Rename the local file from (e.g.) SomeFile.big.eaPatchOld to SomeFile.big. 
                            sLocalFilePathRenamed.assign(fileInfo.mFilePath);
                            sLocalFilePathRenamed.resize(sLocalFilePathRenamed.size() - EA::StdC::Strlen(kPatchOldFileNameExtension)); // Strip off .eaPatchOld.

                            if(!FileRename(fileInfo.mFilePath.c_str(), sLocalFilePathRenamed.c_str(), true)) // If we failed to rename the file...
                            {
                                if(EA::IO::File::Exists(fileInfo.mFilePath.c_str())) // If the file still in fact exists at the time of the above Rename...
                                {
                                    const String sError(fileInfo.mFilePath + " -> " + sLocalFilePathRenamed);
                                    HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRenameError, sError.c_str()); // This will set mbSuccess to false.
                                }
                            }
                            // Else the original file didn't exist. That's not necessarily an error, and even if it was an error it should be something we neither caused nor could do anything about.
                        }
                    }
                }
            }
            else
            {
                mbSuccess = false;
              //mError was already set above.
            }
        }
        else
        {
            // To do: Rename all .eaPatchOld files recursively to remove the extension. 
            //        Delete the dest file if it exists, which actually should never be 
            //        the case except for pathological cases.
        }

        if(mbSuccess)
            SetState(kStateBuildingDestinationFiles);
    }

    return mbSuccess;
}


bool PatchBuilder::UndoBuildDestinationFiles()
{
    if(mbSuccess)
    {
        // During the patching process, this step downloaded the file segments and put together the new files with .eaPatchTemp names (to get their final names in a later step).
        // We undo this by deleting all found patch files with the .eaPatchTemp file name extension.
        // Even though the UndoRenameDestinationFiles function deleted the patch files instead of renaming them to 
        // .eaPatchTemp, we still have to delete .eaPatchTemp files here. The reason for this is that this cancellation
        // may have started while the patch was at an earlier stage, before renaming .eaPatchTemp to their final names,
        // and so UndoRenameDestinationFiles would not get called, because RenameDestinationFiles was not called during patching.

        const bool bPatchImplUsable = !mPatchImpl.mPatchId.empty();

        if(bPatchImplUsable)
        {
            // To know which files we need to delete we do the reverse of what BuildDestinationFiles did. 
            // BuildDestinationFiles walked through mPatchImpl.mPatchEntryArray and downloaded all the files
            // listed there to .eaPatchTemp versions. We walk through mPatchImpl.mPatchEntryArray and delete
            // all of those files present named as .eaPatchTemp. We can't just recursively iterate all of   
            // the destination directory tree and remove all .eaPatchTemp files, as some subdirectory may
            // be implementing its own patch which is unrelated to this patch. We don't use the GetFileList
            // function, because GetFileList is about identifying files originally on disk that are affected 
            // by a patch and not exactly the files that part of the new patch. In practice using GetFileList
            // might work most or all of the time, but it's always correct to do the reverse of what we did
            // in BuildDestinationFiles and use mPatchImpl.mPatchEntryArray.
            String sFilePath;

            for(eastl_size_t i = 0, iEnd = mPatchImpl.mPatchEntryArray.size(); (i != iEnd) && ShouldContinue(); i++) // For each file in the patch...
            {
                const PatchEntry& patchEntry = mPatchImpl.mPatchEntryArray[i];

                if(patchEntry.mbFile)
                {
                    sFilePath = mPatchBaseDirectoryDest + patchEntry.mRelativePath + kPatchTempFileNameExtension;

                    if(!FileRemove(sFilePath.c_str())) // The file might not exist, which is OK and we test for that below.
                    {
                        if(EA::IO::File::Exists(sFilePath.c_str())) // If the file still in fact exists at the time of the above Remove...
                            HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRemoveError, sFilePath.c_str()); // This will set mbSuccess to false.
                    }
                }
                // Else nothing to do. We don't create .eaPatchTemp directories while patching, so there couldn't be any to delete.
            }
        }
        else
        {
            // Else we have no way to identify these files except to find any .eaPatchOld files and assume
            // the .eaPatchOld files were replaced by their equivalent patch files. We might as well handle
            // this case in the step where we rename the .eaPatchOld files to their original names, by
            // deleting files of the original names if they exist.
            // To do.
        }

        SetState(kStateBuildingDestinationFileInfo);
    }

    return mbSuccess;
}


bool PatchBuilder::UndoBuildDestinationFileInfo()
{
    if(mbSuccess)
    {
        // During patching, this step calculated and created .eaPatchBFI files.
        // We use this step to remove the .eaPatchBFI files.

        // To consider: The code here is similar to the code in UndoDownloadPatchDiffFiles, 
        // so there's probably a single shared function we can factor out of these functions.
        const bool bPatchImplUsable = !mPatchImpl.mPatchId.empty();

        if(bPatchImplUsable)
        {
            String sFilePath;

            for(eastl_size_t i = 0, iEnd = mPatchImpl.mPatchEntryArray.size(); (i != iEnd) && ShouldContinue(); i++)
            {
                const PatchEntry& patchEntry = mPatchImpl.mPatchEntryArray[i];

                // Remove the .eaPatchBFI file, if present.
                sFilePath = mPatchBaseDirectoryDest + patchEntry.mRelativePath + kPatchBFIFileNameExtension;

                if(!FileRemove(sFilePath.c_str()))
                {
                    if(EA::IO::File::Exists(sFilePath.c_str())) // If there was an error in the removing...
                        HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRemoveError, sFilePath.c_str()); // This will set mbSuccess to false.
                }

                // Remove the .eaPatchBFI.eaPatchTemp file, if present.
                sFilePath += kPatchTempFileNameExtension;

                if(!FileRemove(sFilePath.c_str()))
                {
                    if(EA::IO::File::Exists(sFilePath.c_str())) // If there was an error in the removing...
                        HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRemoveError, sFilePath.c_str()); // This will set mbSuccess to false.
                }
            }
        }
        else
        {
            // If we want to do the best we can, we remove all .ea temp files recursively from the dest directory.
            // It's an emergency fallback solution though, because it's -theoretically- possible the dest directory has
            // ignored subdirectories and one of those ignored subdirectories has a different active patch in it.

            EA::IO::DirectoryIterator dir;
            EA::IO::DirectoryIterator::EntryList entryList;
            static const uint64_t kMaxPatchFileCountDefault = 100000; // Arbitrary sanity check limit. If this is breached then the root patch directory is probably wrong.

            wchar_t directoryW[EA::IO::kMaxPathLength]; // For EAIO_VERSION_N >= 21600 we don't to do this directory path copying.
            EA::StdC::Strlcpy(directoryW, mPatchBaseDirectoryDest.c_str(), EAArrayCount(directoryW));

            const size_t entryCount = dir.ReadRecursive(directoryW, entryList, L"*.ea*", EA::IO::kDirectoryEntryFile, 
                                                        true, true, static_cast<size_t>(kMaxPatchFileCountDefault), false);
            if(entryCount == EA::IO::kSizeTypeError)
                HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdDirReadError, mPatchBaseDirectoryDest.c_str());
            else
            {
                // We now have an EntryList populated with all the file paths under mPatchBaseDirectoryDest.
                String sFilePath;

                // For each file and directory found in mPatchBaseDirectoryDest...
                for(EA::IO::DirectoryIterator::EntryList::iterator it = entryList.begin(); it != entryList.end(); ++it)
                {
                    EA::IO::DirectoryIterator::Entry& entry = *it;

                    // If turns out that DirectoryIterator entries are wchar_t, but we are working with char8_t. So we need to do a translation copy.
                    sFilePath = EA::StdC::Strlcpy<String, EA::IO::DirectoryIterator::EntryString>(entry.msName);
                    const char8_t* pFileExtension = EA::IO::Path::GetFileExtension(sFilePath.c_str());

                    if(EA::StdC::Stricmp(pFileExtension, kPatchDiffFileNameExtension) == 0)
                    {
                        FileRemove(sFilePath.c_str());
                        break;
                    }
                }
            }
        }

        if(mbSuccess)
            SetState(kStateDownloadingPatchDiffFiles);
    }

    return mbSuccess;
}


bool PatchBuilder::UndoDownloadPatchDiffFiles()
{
    if(mbSuccess)
    {
        // During patching, this step downloaded and wrote to disk .eaPatchDiff files.
        // Here we remove the .eaPatchDiff files.

        const bool bPatchImplUsable = !mPatchImpl.mPatchId.empty();

        if(bPatchImplUsable)
        {
            String sFilePath;

            for(eastl_size_t i = 0, iEnd = mPatchImpl.mPatchEntryArray.size(); (i != iEnd) && ShouldContinue(); i++)
            {
                const PatchEntry& patchEntry = mPatchImpl.mPatchEntryArray[i];

                // Remove the .eaPatchDiff file, if present.
                sFilePath = mPatchBaseDirectoryDest + patchEntry.mRelativePath + kPatchDiffFileNameExtension;

                if(!FileRemove(sFilePath.c_str()))
                {
                    if(EA::IO::File::Exists(sFilePath.c_str())) // If there was an error in the removing...
                        HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRemoveError, sFilePath.c_str()); // This will set mbSuccess to false.
                }

                // Remove the .eaPatchBFI.eaPatchTemp file, if present.
                sFilePath += kPatchTempFileNameExtension;

                if(!FileRemove(sFilePath.c_str()))
                {
                    if(EA::IO::File::Exists(sFilePath.c_str())) // If there was an error in the removing...
                        HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdFileRemoveError, sFilePath.c_str()); // This will set mbSuccess to false.
                }
            }
        }
        else
        {
            // If we want to do the best we can, we remove all .ea temp files recursively from the dest directory.
            // It's an emergency fallback solution though, because it's -theoretically- possible the dest directory has
            // ignored subdirectories and one of those ignored subdirectories has a different active patch in it.

            EA::IO::DirectoryIterator dir;
            EA::IO::DirectoryIterator::EntryList entryList;
            static const uint64_t kMaxPatchFileCountDefault = 100000; // Arbitrary sanity check limit. If this is breached then the root patch directory is probably wrong.

            wchar_t directoryW[EA::IO::kMaxPathLength]; // For EAIO_VERSION_N >= 21600 we don't to do this directory path copying.
            EA::StdC::Strlcpy(directoryW, mPatchBaseDirectoryDest.c_str(), EAArrayCount(directoryW));

            const size_t entryCount = dir.ReadRecursive(directoryW, entryList, L"*.ea*", EA::IO::kDirectoryEntryFile, 
                                                        true, true, static_cast<size_t>(kMaxPatchFileCountDefault), false);
            if(entryCount == EA::IO::kSizeTypeError)
                HandleError(kECBlocking, GetLastSystemErrorId(), kEAPatchErrorIdDirReadError, mPatchBaseDirectoryDest.c_str());
            else
            {
                // We now have an EntryList populated with all the file paths under mPatchBaseDirectoryDest.
                String sFilePath;

                // For each file and directory found in mPatchBaseDirectoryDest...
                for(EA::IO::DirectoryIterator::EntryList::iterator it = entryList.begin(); it != entryList.end(); ++it)
                {
                    EA::IO::DirectoryIterator::Entry& entry = *it;

                    // If turns out that DirectoryIterator entries are wchar_t, but we are working with char8_t. So we need to do a translation copy.
                    sFilePath = EA::StdC::Strlcpy<String, EA::IO::DirectoryIterator::EntryString>(entry.msName);
                    const char8_t* pFileExtension = EA::IO::Path::GetFileExtension(sFilePath.c_str());

                    if(EA::StdC::Stricmp(pFileExtension, kPatchDiffFileNameExtension) == 0)
                    {
                        FileRemove(sFilePath.c_str());
                        break;
                    }
                }
            }
        }

        if(mbSuccess)
            SetState(kStateProcessingPatchImplFile);
    }

    return mbSuccess;
}


bool PatchBuilder::UndoProcessPatchImplFile()
{
    if(mbSuccess)
    {
        // We don't do anything here. There's no equivalent undo for this step.
        SetState(kStateDownloadingPatchImplFile);
    }

    return mbSuccess;
}


bool PatchBuilder::UndoDownloadPatchImplFile()
{
    if(mbSuccess)
    {
        // Remove the .eaPatchImpl file.
        if(FileRemove(mPatchImplFilePath.c_str()))
            SetState(kStateWritingPatchInfoFile);
    }

    return mbSuccess;
}


bool PatchBuilder::UndoWritePatchInfoFile()
{
    if(mbSuccess)
    {
        // Remove the .eaPatchInfo file.
        if(FileRemove(mPatchInfoFilePath.c_str()))
            SetState(kStateBegin);
    }

    return mbSuccess;
}


bool PatchBuilder::UndoStateBegin()
{
    if(mbSuccess)
    {
        // Delete the .eaPatchState file.
        if(FileRemove(mPatchStateFilePath.c_str()))
            mState = kStateNone; // Don't call SetState(kStateNone), because currently SetState writes the .eaPatchState file, which we don't want.
    }

    return mbSuccess;
}


} // namespace Patch
} // namespace EA



