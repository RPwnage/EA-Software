///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/PatchImpl.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_PATCHIMPL_H
#define EAPATCHCLIENT_PATCHIMPL_H


#include <EAPatchClient/Base.h>
#include <EAPatchClient/Hash.h>
#include <EAPatchClient/File.h>
#include <EAPatchClient/Error.h>
#include <EAPatchClient/PatchInfo.h>
#include <EAPatchClient/Serialization.h>
#include <EASTL/vector.h>


#ifdef _MSC_VER
    #pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
    #pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
    #pragma warning(disable: 4275)  // non dll-interface class used as base for dll-interface class.
#endif


namespace EA
{
    namespace Patch
    {
        /// Diff Data
        ///
        /// This data and the local file's contents are all that's needed to tell what 
        /// the diffs are between a local and remote file. This data is stored on disk 
        /// in a binary format, as you can't store binary data in XML (e.g. within the 
        /// PatchEntry class) without encoding it in some way like Base64 which increases
        /// its size significantly.
        ///
        class EAPATCHCLIENT_API DiffData
        {
        public:
            DiffData();

            void Reset();

        public:
            // Const data published by the patch server, associated with a PatchImpl file:
            //                                  /// Generated?  | Required? | Description
            //---------------------------------------------------------------------------------------------------------------
            uint64_t       mVersion;            /// Generated   | Required  | The const version number of the DiffData struct. Used to identify if a DiffData deserialized from disk is of an older format version.
            uint64_t       mFileSize;           /// Generated   | Required  | The size of the file that the FileDiffs refer to. We need to know this because the file's size may not be an even multiple of mBlockSize. 
            HashValueMD5   mDiffFileHashValue;  /// Generated   | Required  | Hash of the following contents of the diff file (not the hash of the data in memory, which may be endian-swapped). Can be used for validating the diff file's contents. If 0 then the hash data isn't available.
            uint64_t       mBlockCount;         /// Generated   | Required  | The count of blocks. This will equal the size of mBlockHashArray. mFileSize will be equal or a little less than (mBlockSize * mBlockCount).
            uint64_t       mBlockSize;          /// Generated   | Required  | The size of data that each BlockHash refers to. Typically this is number between 1024 and 32768, and it must be a power of two, due to the rolling checksum algorithm.
            BlockHashArray mBlockHashArray;     /// Generated   | Required  | The BlockHashes for the remote file, which can be from < 64 bytes in size to multiple MB in size. Logically: ((mBlockHashArray.size() - 1) * mBlockSize) < mFileSize <= (mBlockHashArray.size() * mBlockSize).
        };

        /// DiffDataSerializer
        ///
        /// Serializes DiffData to and from a stream or .eaPatchDiff file.
        /// To consider: Move this and other Serializer classes to a seperate .h/.cpp file.
        ///
        class EAPATCHCLIENT_API DiffDataSerializer : public SerializationBase
        {
        public:
            bool Serialize(const DiffData& diffData, EA::IO::IStream* pStream); /// Writes the binary DiffData to the given IO stream.

            bool Deserialize(DiffData& diffData, const char8_t* pFilePath);     /// Reads the binary DiffData from the given file.
            bool Deserialize(DiffData& diffData, EA::IO::IStream* pStream);     /// Reads the binary DiffData from the given IO stream.
        };



        /// PatchEntry
        ///
        /// Identifies a file or directory that exists in the patch.
        /// PatchEntrys don't identify files and directories that need to be deleted from the 
        /// local directory. Rather, the PatchClient can identify that a local file needs to be
        /// deleted by recognizing that it's not present in the PatchImpl mPatchEntryArray array 
        /// nor is it identified by PatchImpl mIgnoredFileArray/mUsedFileArray. A patch is an image that 
        /// is sync'd to and not a set of instructions on how to get from some state to the patch. 
        /// This is important as it simplifies the patch implementation and allows for things like 
        /// reverting a patch to a previous patch.
        ///
        /// The non-diff-related portion of this struct can be cached to disk as an XML .eaPatchEntry (or .eaPE) file.
        /// The diff-related portion of this struct can be cached to disk as a binary .eaPatchEntryDiff (or .eaPED) file.
        /// 
        class EAPATCHCLIENT_API PatchEntry
        {
        public:
            PatchEntry();

        public: // Const data published by the patch server, as part of an PatchImpl file:
            //                                  /// Generated?     | Required? | Description
            //---------------------------------------------------------------------------------------------------------------
            bool           mbFile;              /// Generated      | Required  | True if a file, else it's a directory;
            bool           mbForceOverwrite;    /// Generated      |           | Hint which tells the PatchClient to not try to do a differential patch, and instead just copy the file from the server.
            bool           mbFileUnchanged;     /// Generated      |           | Hint which says that the file has never been changed and thus the local one can be used. The remote (patch) version of the file is guaranteed identical to the local one (assuming the local one is present).
            String         mFileURL;            /// Generated      | Required  | URL to the remote file itself. For differential patching, we will download segments from this remote file based on the diffs found by analyzing checksums and hashes. e.g. "http://patch.ea.com:1234/Basebrawl_2014/Patches/Data/GameData.big"
            String         mRelativePath;       /// Generated      |           | Relative path to the local version of the file we will be writing. It's relative to mPatchBaseDirectory set in the PatchInfo class. e.g. "Data/GameData.big"
            String         mAccessRights;       /// Generated      |           | Access rights, which may be empty for default rights, and are platform-specific.
            uint64_t       mFileSize;           /// Generated      | Required  | The size of the file that the FileDiffs refer to. We need to know this because the file's size may not be an even multiple of mBlockSize. 
            HashValueMD5   mFileHashValue;      /// Generated      | Required  | Hash of the full patched file. Useful for validation after doing a differential patch.
            StringArray    mLocaleArray;        /// User-specified |           | Locales that this PatchEntry applies to. Empty means all locales. Otherwise, this entry is ignored if the current patch locale set doesn't have any matches with this patch locale set.
            String         mDiffFileURL;        /// Generated      | Required  | Link to binary file that has the information needed to populate the diff data. It's a separate link because it's potentially pretty large. Note that this is a link not to the remote file, but to the checksums/hashes of the remote file. e.g. "http://patch.ea.com:1234/Basebrawl_2014/Patches/Data/GameData.big.eaPatchDiff"
            DiffData       mDiffData;           /// Generated      | Required  | The contents of the file located at mDiffFileURL. This is downloaded separately from PatchEntry, as it is binary (as opposed to XML) and is potentially big (~Megabytes).
        };

        typedef eastl::vector<PatchEntry, CoreAllocator> PatchEntryArray;

        // No Serialize/Deserialize functions for PatchEntry because PatchImpl handles that. PatchEntry is an intermediate class.



        /// PatchImpl
        ///
        /// Defines a patch. While the PatchInfo class describes high level information
        /// about a patch PatchImpl defines the patch itself. This means what files are 
        /// changed, ignored, the entire patch MD5 hash, the file diffs.
        ///
        /// The PatchImpl data normally wouldn't be downloaded until it was decided to 
        /// apply the patch. The PatchImpl could be considered an extension of the data
        /// present in PatchInfo, with the difference being that PatchInfo is only the
        /// data needed to present to the user to make a choice to apply the patch.
        ///
        /// This struct can be cached to disk as a .eaPatchImpl file.
        ///
        class EAPATCHCLIENT_API PatchImpl : public PatchInfo
        {
        public:
            PatchImpl();
           ~PatchImpl();

            void Reset();

        public:
            // Const data published by the patch server:    /// Generated?     | Required?  | Description
            //----------------------------------------------------------------------------------------------------------------------------------------------
            String              mPatchHash;                 /// Generated      | Required    | MD5 Hash of entire patch. The MD5 of all files in the directory read in alphabetical depth-first order, skipping ignored directories.
            String              mPreRunScript;              /// User-specified |             | Script to run before installing the patch. May be a URL to a Lua script instead of a script itself.
            String              mPostRunScript;             /// User-specified |             | Script to run after installing the patch. May be a URL to a Lua script instead of a script itself.
            FileFilterArray     mIgnoredFileArray;          /// User-specified | Required or | Array of files and directories to ignore, specified by wild-card-like filters. The files may be in subdirectories or in the root. Directories are specified by (e.g.) /SomeDir/SomeIgnoredDir/*. If empty then no files are ignored and the entire directory is considered part of the patched data. If * then all files are ignored and only files white-listed by mUsedFileArray will be considered part of the patch.
            FileFilterArray     mUsedFileArray;             /// User-specified | Required    | Array of files and directories to use, specified by wild-card-like filters. The files may be in subdirectories or in the root. Directories are specified by (e.g.) /SomeDir/SomeIgnoredDir/*. Files identified by mUsedFileArray override files identified by mIgnoredFileArray. If empty then all non-ignored files are used. If * then all files are used. 
            PatchEntryArray     mPatchEntryArray;           /// Generated      |             | List of all the files+directories in the remote patch.
        };


        /// PatchImplSerializer
        ///
        /// Serializes PatchImpl to and from a stream.
        /// To consider: Move this and other Serializer classes to a seperate .h/.cpp file.
        ///
        class EAPATCHCLIENT_API PatchImplSerializer : public SerializationBase
        {
        public:
            /// Write the PatchImpl and PatchDiff files. This is essentially a serialization function.
            /// Writes the PatchImpl as an XML file to a file at the given path binary Diff files to their paths.
            /// The PatchImpl file is the PatchImpl class data minus the diff data for each PatchEntry. 
            /// PatchDiff files are the diff data from the PatchEntry class.
            /// If the return value is false, consult GetError to determine the error cause.
            /// pFilePath is the base file name of the pactch, such as /MyGame/data/Patch1234. The StreamImpl and StreamDiff will
            /// be written to (e.g.) /MyGame/data/Patch1234.eaStreamImpl and /MyGame/data/Patch1234.eaStreamDiff.
            bool Serialize(const PatchImpl& patchImpl, const char8_t* pFilePath, bool bAddHelpComments);

            /// Stream version of Serialize. This is essentially a serialization function.
            /// Writes the PatchImpl as an XML file to the given IO stream.
            bool Serialize(const PatchImpl& patchImpl, EA::IO::IStream* pStream, bool bAddHelpComments);

            /// File version of Deserialize. This is essentially a deserialization function.
            /// Reads the PatchImpl as an XML file from the given file path.
            /// We do not read the mDiffData here, as that's a separate read due to its binary nature.
            /// If bMakerMode is true then we don't require that some of the fields be present, as it's
            /// assumed that the input .eaPatchImpl XML file is a user-created template for the purposes
            /// of making a patch and not a shipping completed .eaPatchImpl file.
            /// If bConvertPathsToLocalForm is true, file paths (not URL paths) are converted
            /// to local form (e.g. convert / to \ if running on Microsoft platforms).
            bool Deserialize(PatchImpl& patchImpl, const char8_t* pFilePath, bool bMakerMode, bool bConvertPathsToLocalForm);

            /// Stream version of Deserialize. This is essentially a deserialization function.
            /// Reads the PatchImpl as an XML file from the given IO stream.
            /// We do not read the mDiffData here, as that's a separate read due to its binary nature.
            /// If bMakerMode is true then we don't require that some of the fields be present, as it's
            /// assumed that the input .eaPatchImpl XML file is a user-created template for the purposes
            /// of making a patch and not a shipping completed .eaPatchImpl file.
            /// If bConvertPathsToLocalForm is true, file paths (not URL paths) are converted
            /// to local form (e.g. convert / to \ if running on Microsoft platforms).
            bool Deserialize(PatchImpl& patchImpl, EA::IO::IStream* pStream, bool bMakerMode, bool bConvertPathsToLocalForm);
        };



        /// PatchImplRetriever
        ///
        /// Retrieves a PatchImpl from the patch server, given a PatchInfo which indicates 
        /// what the PatchImpl is that we want to retrieve. This class is typically used
        /// after a patch (identified by a PatchInfo) has been selected. This class downloads
        /// all the information needed previous to starting the file diffing, downloading, and
        /// writing. Once this class is done, then the PatchBuilder class takes over and does
        /// the aforementioned steps of diffing, downloading, and writing.
        ///
        /// Executing this class (via RetrievePatchImpl) amounts to starting the application 
        /// of a patch. During the process of patching, the destination directory (where the patch
        /// will be applied) is considered owned by this patching process, and RetrievePatchImpl
        /// writes a .eaPatchImpl file to the root of that directory to indicate that it is busy
        /// applying the given patch.
        ///
        class EAPATCHCLIENT_API PatchImplRetriever : public ErrorBase
        {
        public:
            PatchImplRetriever();
           ~PatchImplRetriever();

            // If the return value is false, you can use GetError to find out about it.
            // As with the rest of the EAPatchClient, errors are reported as they occur to 
            // the handler set by RegisterUserErrorHandler. Only a single download can be 
            // active at a time for an instance of PatchImplRetriever, and false will 
            // be returned (kEAPatchErrorIdAlreadyActive) if a retrieval is active.
            // This is a blocking call which probably should be called in a thread other 
            // than the main thread.
            //
            // Calling this function amounts to starting the application of a patch, though 
            // the caller will have to follow up with other calls to complete the patching.
            // *** As of this writing it hasn't been decided if this retrieval can be paused 
            //     and later continued. ***
            bool RetrievePatchImpl(PatchInfo* pPatchImpl);

            // Tells if RetrievePatchImpl is busy downloading a PatchImpl, for example.
            AsyncStatus GetAsyncStatus() const;

            // Cancels an existing download if it's active. 
            // If waitForCompletion is true then this function doesn't return until the 
            // cancel operation is complete and all of its resources are released.
            void CancelRetrieval(bool waitForCompletion);

        protected:  
          //PatchDirectory* mpPatchDirectory;
        };

    } // namespace Patch

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard



 




