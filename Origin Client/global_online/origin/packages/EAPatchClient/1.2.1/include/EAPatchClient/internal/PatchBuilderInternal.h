///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/PatchBuilder.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_PATCHBUILDERINTERNAL_H
#define EAPATCHCLIENT_PATCHBUILDERINTERNAL_H


#include <EAPatchClient/Base.h>
#include <EAPatchClient/Error.h>
#include <EASTL/vector.h>
#include <EASTL/list.h>


namespace EA
{
    namespace Patch
    {
        /// When building a new file which is a patched version of a previous local file,
        /// We create a set of spans which represent segments from the local and remote (server) 
        /// patch files. To build the patched file we read the spans from these two sources and   
        /// write them to the new patched file. The patched file will be identical to the 
        /// sever patched file. The only reason we don't just read the entire server file
        /// is that could be very slow over a network.
        struct BuiltFileSpan
        {
            FileType    mSourceFileType;        /// Local source or server source (kFileTypeLocal or kFileTypeRemote).
            uint64_t    mSourceStartPos;        /// Where in the source file this span starts. Don't need to have a 'mSourceEndPos' variable, since mDestEndPos-mDestStartPos already tells us the size.
            uint64_t    mDestStartPos;          /// Where in the destination patched file this span starts. If mFileSource == kFileTypeRemote, then mSourceStartPos == mDestStartPos. 
            uint64_t    mCount;                 /// Count of bytes in the span. UINT64_MAX means as many bytes as remain in the source file.
            uint64_t    mCountWritten;          /// How many bytes have been written.
            uint32_t    mSpanWrittenChecksum;   /// If 0 then the span wasn't written, else this is the expected checksum of the span's data. If the expected checksum happens to be 0 then it's written here instead as 1. This field is used for helping us resume downloads that were interrupted and later re-started.

            BuiltFileSpan(FileType fileType = kFileTypeLocal, uint64_t sourceStartPos = 0, uint64_t destStartPos = 0, uint64_t count = 0);
        };

        /// List of file spans.
        /// A list of file spans provides all the spans needed for a built patched file.
        typedef eastl::list<BuiltFileSpan, CoreAllocator> BuiltFileSpanList;



        /// BuiltFileInfo
        ///
        /// Identifies a single file to be built, possibly from the parts of local and remote files.
        /// This also applies to a file being copied entirely from the remote location with no
        /// previous local file, as that is merely a special case of this struct.
        /// This struct is built via information in DiffData and its BlockHashArray.
        ///
        struct BuiltFileInfo
        {
            static const uint64_t writeEveryXMegabyte = 100 * 1024 * 1024;

            uint64_t          mVersion;                 /// Numerical version of this struct and its serialized format.
            String            mDestinationFilePath;     /// Path to the final destination file being built. Note that may be a location within a temp directory, as the entire patch process sometimes occurs in some kind of temp directory and not the final location, which is flipped to at the last step in this case.
            String            mDestinationFilePathTemp; /// mDestinationFilePath + .eaPatchTemp. This is the temporary while-being-built version of this file.
            String            mLocalFilePath;           /// Path to local patch source file, the one that will be updated to a new file at the destination.
            uint64_t          mLocalFileSize;           /// The size of the local file.
            uint64_t          mLocalSpanCount;          /// Sum of all mCount values in mBuildFileSpanList for kFileTypeLocal. The sum of bytes that will come from the local file as opposed to the remote file.
            uint64_t          mLocalSpanWritten;        /// How many of mLocalSpanCount have already been written to the destination file.
            String            mRemoteFilePathURL;       /// 
            uint64_t          mRemoteSpanCount;         /// Sum of all mCount values in mBuildFileSpanList for kFileTypeRemote. The sum of bytes that will come from the remote file as opposed to the local file.
            uint64_t          mRemoteSpanWritten;       /// How many of mRemoteSpanCount have already been written to the destination file.
            BuiltFileSpanList mBuiltFileSpanList;       /// This defines how the file bytes from mLocalFilePath and mRemoteFilePathURL are combined into mDestinationFilePath.
            File              mFileStreamTemp;          /// File object (.eaPatchTemp). This extension will be stripped when complete.
            int32_t           mFileUseCount;            /// Count of how many users there are of this file. Allows multiple jobs

            bool mFirstWrite = true;
            uint64_t          mLastLocalSpanWritten = 0;
            uint64_t          mLastRemoteSpanWritten = 0;

            bool ShouldWrite() const {
                return mFirstWrite ||                                                                                                   // Always write the first serialization
                    mLocalSpanWritten + mRemoteSpanWritten > mLastLocalSpanWritten + mLastRemoteSpanWritten + writeEveryXMegabyte ||    // Write every subsequent X Mb.
                    mLocalSpanWritten == mLocalSpanCount ||                                                                             // Write when all local spans are written
                    mRemoteSpanWritten == mRemoteSpanCount;                                                                             // Write when all remote spans are written
            }

            void UpdateLastWritten() {
                mFirstWrite = false;
                mLastLocalSpanWritten = mLocalSpanWritten;
                mLastRemoteSpanWritten = mRemoteSpanWritten;
            }


            BuiltFileInfo();
            void Reset();
            void SetSpanListToSingleSegment(FileType fileType, uint64_t remoteFileSize);
            void SetSpanCounts();
            void InsertLocalSegment(uint64_t localPosition, uint64_t destPosition, uint64_t count, uint64_t destFileSize);
            bool ValidateSpanList(uint64_t destFileSize, bool listIsComplete) const;  // This is a debug function.
            void GetFileSizes(uint64_t& localSize, uint64_t& remoteSize) const;     // Tells how much of the file comes from the local source and how much from the remote source. This is a diagnostics function, though it can be used at runtime.

            BuiltFileSpanList::iterator FindNextRemoteSpanInRange(BuiltFileSpanList::iterator start, int minLocalSpanLength);
            
            // Merge the spans including end into a new remote span. Return the merged span.
            BuiltFileSpanList::iterator MergeIntoRemoteSpan(BuiltFileSpanList::iterator start, BuiltFileSpanList::iterator finish);

            void RemoveSmallLocalSpans(int minLocalSpanLength);
        };

        /// Array of BuiltFileInfo.
        typedef eastl::vector<BuiltFileInfo, CoreAllocator> BuiltFileInfoArray;


        /// BuiltFileInfoSerializer
        ///
        /// Serializes BuiltFileInfo to and from a stream.
        ///
        class EAPATCHCLIENT_API BuiltFileInfoSerializer : public SerializationBase
        {
        public:
            bool Serialize(BuiltFileInfo& bfi, const char8_t* pFilePath); /// Writes the binary BuiltFileInfo to the given file.
            bool Serialize(const BuiltFileInfo& bfi, EA::IO::IStream* pStream); /// Writes the binary BuiltFileInfo to the given stream.

            bool Deserialize(BuiltFileInfo& bfi, const char8_t* pFilePath);     /// Reads the binary BuiltFileInfo from the given file.
            bool Deserialize(BuiltFileInfo& bfi, EA::IO::IStream* pStream);     /// Reads the binary BuiltFileInfo from the given stream.
        };

    } // namespace Patch

} // namespace EA



#endif // Header include guard



 




