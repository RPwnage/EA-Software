///////////////////////////////////////////////////////////////////////////////
// EAPatchMaker/Maker.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHMAKER_MAKER_H
#define EAPATCHMAKER_MAKER_H


#include <EAPatchMaker/Config.h>
#include <EAPatchClient/Error.h>
#include <EAPatchClient/PatchImpl.h>
#include <EASTL/list.h>


namespace EA
{
    namespace Patch
    {
        /// The default max number of files in a patch, for sanity checking. 
        /// can be overridden on the patch maker command line.
        static const uint64_t kMaxPatchFileCountDefault = 100000;


        /// These are user-supplied settings which are used by the Maker to generate a patch.
        ///
        struct MakerSettings
        {
            PatchImpl   mPatchImpl;               /// Some of the fields from this (e.g. patch footprint size) are filled in by the Patch Maker.
            String      mPatchImplPath;           /// The path to the .eaPatchImpl file for the patch. This is an XML file which deserializes to mPatchImpl. If this is empty then the first .eaPatchImpl file found in the input directory is used.
            String      mInputDirectory;          /// The full path to the local directory from which to build the patch. mPatchInfo has the relative path which is assoicated with the patch, which must be a subdirectory of this directory.
            String      mOutputDirectory;         /// The directory where the patch is created. This directory gets transferred to the patch web server when ready to publish.
            String      mPreviousDirectory;       /// Directory which holds the contents of the previous version of the data which the patch replaces. This is used for optimization purposes only. Specifying this directory isn't required but can result in reduced download sizes and thus increased patch application performance.
            bool        mbValidatePatchImpl;      /// If true then we validate that the user has supplied a .eaPatchImpl starter file and its user-defined fields are present and sane. False by default.
            bool        mbCleanOutputDirectory;   /// If true then delete the contents of the output directory before writing the patch. True by default.
            bool        mbEnableOptimizations;    /// If true then optimizations are enabled. However, this will result in significantly longer to build the patch, as it needs to try various settings over potentially many large files. True by default.
            bool        mbEnableErrorCleanup;     /// If true then upon an error the output directory is deleted. True by default.
            uint64_t    mMaxFileCount;            /// Max number of files allowed in a patch. Used for detecting unintended pathological input parameters. Default value is kMaxPatchFileCountDefault.
            uint32_t    mBlockSize;               /// Fixed block size to use. Zero if un-set.

            MakerSettings();
            void Reset();
        };


        /// Describes a file that's part of a patch.
        /// 
        struct PatchFileInfo
        {
            bool         mbFile;                /// True if this refers to a file as opposed to a directory. If false then mFilePath will end with a file path separator (e.g. / char).
            String       mInputFilePath;        /// Path to the file or directory.
            String       mOutputFilePath;       /// Path to the file or directory.
            uint64_t     mFileSize;             /// Equal to zero for directory entries.
            String       mAccessRights;         /// Describes the desired access rights of the patch file.
            HashValueMD5 mFileHashValue;        /// Hash value of the entire file's contents. Equal to zeros for a directory, which has no contents.
            uint64_t     mBlockSize;            /// DiffData block size.
        };

        typedef eastl::list<PatchFileInfo, CoreAllocator> PatchFileInfoList;


        /// Maker
        ///
        /// This class makes patches.
        /// Given a MakerSettings struct, this function builds a patch in the mOutputDirectory
        /// field of MakerSettings. 
        ///
        /// If you want to understand how this class and patches in general work, you want to 
        /// read the EAPatchTechnicalDesign.html doc and some of the key references. Once you 
        /// understand them then this class and everything it does will be self-evident.
        ///
        class Maker : public ErrorBase
        {
        public:
            Maker();
           ~Maker();

            /// Independent function for testing patch maker settings. This function doesn't     
            /// set the class Error, but merely returns if there was an error and returns a 
            /// non-localized string describing the error if there is one. sError is untouched
            /// if there was no error.
            bool ValidateMakerSettings(const MakerSettings& settings, String& sError);

            /// Resets this class to the newly constructed state, usually for the purpose of    
            /// making another call to MakePatch.
            void Reset();

            /// High level one-shot patch generating function.
            /// Makes a patch with the given settings.
            /// Returns true upon success, but also sets the class Error, which can be queried.
            bool MakePatch(const MakerSettings& settings);

        protected:
            void   FixUserDirectoryPaths();
            void   CreateDefaultPatchImpl(PatchImpl& patchImpl);
            String FindPatchImplFile(const char8_t* pInputDirectoryPath);
            bool   CleanOutputDirectory();
            bool   ValidateMakerSettings();
            bool   LoadPatchImplFile();
            bool   BuildPatchFileInfoList();
            bool   MakePatchImpl();
            bool   CopyFileWhileCalculatingHashes(const char8_t* pFileSourceFullPath, const char8_t* pFileSourceBaseDirectory, const char8_t* pFileDestBaseDirectory, 
                                                DiffData& diffData, String& destRelativePath, String& destFullPath);
            bool   WritePatchImpl();
            bool   VerifyPatch();
            void   PrintUserReport();
            void   PrintUserInstructions();
            void   Cleanup();

        protected:
            MakerSettings     mMakerSettings;
            PatchFileInfoList mPatchDirectoryList;          /// List of directories that constitute the patch.
            PatchFileInfoList mPatchFileList;               /// List of files that constitute the patch. These can include files whose directories are listed in mPatchDirectoryList.
            StringList        mIgnoredPatchDirectoryList;   /// List of directory paths from MakerSettings::mInputDirectory that are ignored.
            StringList        mIgnoredPatchFileList;        /// List of file paths from MakerSettings::mInputDirectory that are ignored. These can include files whose directories are listed in mIgnoredPatchDirectoryList.
            PatchImpl         mPatchImpl;                   /// The generated mPatchImpl, initially derived from mMakerSettings.mPatchImpl.
            String            mPatchInfoFilePath;           /// The generated .eaPatchInfo file path.
            String            mPatchImplFilePath;           /// The generated .eaPatchImpl file path.
        };

    } // namespace Patch

} // namespace EA



#endif // Header include guard



 




