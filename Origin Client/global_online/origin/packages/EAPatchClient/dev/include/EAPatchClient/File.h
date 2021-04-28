///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/File.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_FILE_H
#define EAPATCHCLIENT_FILE_H


#include <EAPatchClient/Base.h>
#include <EAPatchClient/Hash.h>
#include <EAPatchClient/Error.h>
#include <EAIO/EAFileBase.h>
#include <EAIO/EAFileStream.h>
#include <EAIO/EAStreamBuffer.h>


#ifdef _MSC_VER
    #pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
    #pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
    #pragma warning(disable: 4275)  // non dll-interface class used as base for dll-interface class.

    #if defined(CopyFile)           // Windows headers #define CopyFile to CopyFileA or CopyFileW. So revert that.
        #undef CopyFile
    #endif
#endif



namespace EA
{
    namespace Patch
    {
        /// FileBuffer
        ///
        /// Type for generic file buffer, typically for reading into during file operations.
        ///
        typedef eastl::vector<uint8_t, EA::Patch::CoreAllocator> FileBuffer;


        /// FileType
        ///
        /// When building a new file which is a patched version of a previous local file,
        /// the new file will be built from the previous file to the extent possible,
        /// since that will save network usage. The parts of the resulting patched file 
        /// that aren't in the local file come from the server patch data. We enumerate
        /// these two types.
        ///
        enum FileType
        {
            kFileTypeLocal,     /// Previous file on local machine. Maybe in the same location as kFileTypeDest, though after patching the file's contents may change.
            kFileTypeRemote,    /// New file on file server machine; the patch file.
            kFileTypeDest       /// The file to be built.
        };


        /// FileFilter
        ///
        /// Allows you to specify a file or directory matching filter, the same as 
        /// with the fnmatch function seen on some Posix systems (http://linux.die.net/man/3/fnmatch) and 
        /// available in EAIO/FnMatch.h. This is like a poor man's regex, but is lightweight and works
        /// for a lot of basic file system needs and is easier to grok.
        ///
        /// You can do more with fnmatch than you can with just * and ?. For example, the kFNMPathname flag 
        /// causes the * character to not see past a directory separator, so you can do this:
        ///      fnmatch("Game/*/*.txt", "Game/AnyDirectory/AnyFile.txt", kFNMPathname) 
        /// and it will return true.
        ///
        struct FileFilter
        {
            String   mFilterSpec;     // File or directory name, optionally including filter chars (e.g. * ? [])
            uint32_t mMatchFlags;     // See EAIO/FnMatch.h, and possibly FnMatchFlagsStringToFlags.
        };

        bool operator==(const FileFilter& a, const FileFilter& b);
        typedef eastl::vector<FileFilter, CoreAllocator> FileFilterArray;


        /// FileInfo
        ///
        /// Basic file information. Used by GetFileList to enumerate files that are part of a patch.
        ///
        struct FileInfo
        {
            String      mFilePath;  /// File name only, full file path, or relative file path.
            uint64_t    mSize;      /// 
            bool        mbFile;     /// If true then it's a file, else a directory.

            FileInfo() : mFilePath(), mSize(0), mbFile(false) {}
        };

        typedef eastl::list<FileInfo, CoreAllocator> FileInfoList;


        /// GetFileList
        ///
        /// This function makes a listing of all files in the local directory that are ignored and/or all files in the 
        /// destination directory that are used. The local directory and destination directory are the same if mbInPlacePatch is true.
        /// If bFullPaths is true then the strings returned are full paths, else they are paths relative to their respective directories.
        /// Either pIgnoredFileList or pUsedFileList may be NULL, in which case they won't be written to. However, both FileFilterArray
        /// types must be supplied.
        ///
        /// Truth table
        /// A Y indicates that the file is used (part of the patch set):
        ///                    | ignored empty | ignored match | ignored unmatched
        ///     -------------------------------------------------------------------
        ///     Used empty     |       N       |       N       |        Y          
        ///     Used match     |       Y       |       Y       |        Y          
        ///     Used unmatched |       N       |       N       |        N    
        ///
        /// pAppendedExtensionList (if non-NULL) is a list of file extensions that are stripped from file entries before they are subjected
        /// to the ignored/used tests. For example, if you want to find all patch files in the destination directory that were
        /// renamed to .eaPatchOld, you would need to add ".eaPatchOld" to pAppendedExtensionList. Otherwise this function won't 
        /// realize that (e.g.) YourFile.big.eaPatchOld was part of the patchSet, for example in the case that the usedFilterArray
        /// that the patch author specified simply lists *.big. The extension list is repeatedly applied to file names and so can
        /// strip multiple extensions, such as with a file named Game.big.eaPatchDiff.eaPatchTemp.
        ///
        bool GetFileList(const char8_t* pRootDirectoryPath, const FileFilterArray& ignoredFileArray, FileInfoList* pIgnoredFileList, 
                         const FileFilterArray& usedFileArray, FileInfoList* pUsedFileList, const StringList* pAppendedExtensionList, 
                         bool bReturnFullPaths, bool bReadFileSize, Error& error);


        /// StringToFnMatchFlags
        /// FnMatchFlagsToString
        ///
        /// Converts a string such as "Pathname CaseFold" to (kFNMPathname | kFNMCaseFold).
        /// This is useful for reading a text-based config file and converting it to what the
        /// FnMatch function needs in FileFilter (above). See EAIO/FnMatch.h
        /// 
        EAPATCHCLIENT_API uint32_t StringToFnMatchFlags(const char8_t* pMatchFlagsString);
        EAPATCHCLIENT_API String   FnMatchFlagsToString(uint32_t matchFlags);


        /// PathMatchesFileFilter
        /// PathMatchesFileFilterArray
        ///
        /// Uses the EA::IO::FnMatch function to do file path filter testing.
        /// Returns true if the given file or directory path matches the FileFilter.
        /// If the file filter is empty or if the file filter array is empty then the return value is false.
        /// If bMatchEmptyFilter is true, then the path is considered a match if fileFilterArray is empty.
        /// See FnMatch's documentation to see how it behaves in the presence of an empty pPath or fileFilter.
        /// FnMatch is similar (if not identical) to Unix fnmatch: http://linux.die.net/man/3/fnmatch
        ///
        EAPATCHCLIENT_API bool PathMatchesFileFilter(const char8_t* pPath, const FileFilter& fileFilter);
        EAPATCHCLIENT_API bool PathMatchesFileFilterArray(const char8_t* pPath, const FileFilterArray& fileFilterArray, bool bMatchEmptyFilter);


        /// PathMatchesAnInternalFileType
        ///
        /// Returns true if the given file or directory path refers to one of our internal file
        /// types, such as .eaPatchInfo, .eaPatchDiff, etc.
        ///
        EAPATCHCLIENT_API bool PathMatchesAnInternalFileType(const char8_t* pPath);


        /// DirectoryIsEmpty
        ///
        /// Returns true if the given directory is empty or doesn't exist.
        /// To consider: Provide a means to return that an error occurred in executing this.
        ///
        EAPATCHCLIENT_API bool DirectoryIsEmpty(const char8_t* pDirectoryPath);


        /// IsFilePathValid
        ///
        /// This is for debugging only. A return value of true guarantees that the path is invalid,
        /// but a path of false doesn't guarantee the path is valid. This is because some platforms
        /// and file system (e.g. Microsoft Windows) have a very complex scheme for determining
        /// path validity which can in some cases only be tested by asking the file system at runtime.
        EAPATCHCLIENT_API bool IsFilePathValid(const String& sPath);


        /// FilePatternExists
        ///
        /// Returns true if a file of the given pattern 
        /// To consider: Provide a means to return that an error occurred in executing this.
        /// Currently only basic wildcards in the file name are supported (* and ? chars), 
        /// and not wildcards or other patterns in the directory path components.
        ///
        /// Example usage:
        ///    if(FilePatternExists("/SomeDir/*.eaPatchImpl"))
        ///        ...
        /// Example usage:
        ///    if(FilePatternExists("/SomeDir/", "*.eaPatchImpl"))
        ///        ...
        ///
        EAPATCHCLIENT_API bool FilePatternExists(const char8_t* pFilePathPattern, String* pFirstFilePathResult = NULL);
        EAPATCHCLIENT_API bool FilePatternExists(const char8_t* pDirectoryPath, const char8_t* pFileNamePattern, String* pFirstFilePathResult = NULL);


        /// StripFileNameFromFilePath
        ///
        /// Strips the file name from a file path.
        ///
        /// Examples:
        ///     /abc/def     -> /abc/       // Unix path
        ///     /abc/def.txt -> /abc/
        ///     def          ->             // (empty string)
        ///     def.txt      ->             // (empty string)
        ///     C:\abc\def   -> C:\abc\     // DOS path
        ///     \\abc\def    -> \\abc\      // UNC path
        ///
        EAPATCHCLIENT_API String& StripFileNameFromFilePath(String& sFilePath);

        /// StripFileNameExtensionFromFilePath
        ///
        /// Strips the file name extension from a file path.
        /// Extension is defined as the last sequence of chars starting with . after any present directory.
        ///
        /// Examples:
        ///     /abc/def     -> /abc/def        // (no effect on directories)
        ///     C:\abc\def   -> C:\abc\def
        ///     \\abc\def    -> \\abc\def
        ///     /abc/def.txt -> /abc/def
        ///     def.abc.txt  -> def.abc         // Only last . sequence is removed.
        ///     def.txt      -> def
        ///     def          -> def             // (extension is already gone)
        ///
        EAPATCHCLIENT_API String& StripFileNameExtensionFromFilePath(String& sFilePath);


        /// CanonicalizeFilePath
        ///
        /// Converts file path separators to a given form (local form by default).
        ///
        /// Examples:
        ///     CanonicalizeFilePath("/a/b/c/d/", '/')   ->   "/a/b/c/d/"
        ///     CanonicalizeFilePath("/a/b/c/d" , '/')   ->   "/a/b/c/d/"
        ///     CanonicalizeFilePath("\a\b\c\d" , '/')   ->   "/a/b/c/d/"
        ///     CanonicalizeFilePath("\a\b\c\d\", '\')   ->   "\a\b\c\d\"
        ///     CanonicalizeFilePath("/a/b/c/d" , '/')   ->   "/a/b/c/d/"
        ///     CanonicalizeFilePath("/"        , '\')   ->   "\"
        ///     CanonicalizeFilePath(""         , '\')   ->   "\"
        ///
        EAPATCHCLIENT_API String& CanonicalizeFilePath(String& sFilePath, char8_t separator = EA_FILE_PATH_SEPARATOR_8);


        /// EstimateFileDiskSpaceUsage
        ///
        /// Conservatively estimates the disk space usage for the file under the supplied set 
        /// of OSs. Returns the highest value for the OS. If the osNameArray is empty
        /// then a default disk system is assumed. The return value can be only an
        /// estimate, as the file system can vary and can't be predicted ahead of time.
        /// The estimate is conservative and represents a likely upper bound on the usage.
        /// The disk space usage for a file is >= the file size because files are stored 
        /// on disk in fixed-size blocks.
        ///
        EAPATCHCLIENT_API uint64_t EstimateFileDiskSpaceUsage(uint64_t fileSize, const StringArray& osNameArray);


        /// GetPlatformMaxFileNameLength
        ///
        /// Tells what the max file name length is for the given platform, identified
        /// by an EAPATCH_OS string.
        /// 
        /// Example usage:
        ///     size_t n = GetPlatformMaxFileNameLength(EAPATCH_OS_PS3_STR);
        ///
        EAPATCHCLIENT_API size_t GetPlatformMaxFileNameLength(const char8_t* pPlatformName);

        /// Returns the max file name length that's the minimum of the given OSs.
        EAPATCHCLIENT_API size_t GetPlatformMaxFileNameLength(const StringArray& osNameArray);

        /// Tells what the max file name length is for the given platform, identified
        /// by an EAPATCH_OS string.
        EAPATCHCLIENT_API size_t GetPlatformMaxPathLength(const char8_t* pPlatformName);

        /// Returns the max file name length that's the minimum of the given OSs.
        EAPATCHCLIENT_API size_t GetPlatformMaxPathLength(const StringArray& osNameArray);



        /// EnsureTrailingSeparator
        ///
        /// Directory paths always have a trailing path separator, for consistency and non-ambiguity.
        /// This EnsureTrailingSeparator has the same behavior as EA::IO::Path::EnsureTrailingSeparator.
        ///
        /// Example behavior:
        ///     Call                                    New value Unix      New value Microsoft      
        ///     -----------------------------------------------------------------------------
        ///     EnsureTrailingSeparator("");            "/"                 "\"
        ///     EnsureTrailingSeparator("/");           "/"                 "/"
        ///     EnsureTrailingSeparator("\");           "\/"                "\"
        ///     EnsureTrailingSeparator("a/b");         "a/b/"              "a/b\"
        ///     EnsureTrailingSeparator("//");          "//"                "//"
        ///     EnsureTrailingSeparator("C:");          "C:/"               "C:\"
        ///     EnsureTrailingSeparator("\a\");         "\a\/"              "\a\"
        ///
        template <typename String>
        String& EnsureTrailingSeparator(String& sPath)
        {
            typedef typename String::value_type char_type;

            if(sPath.empty() || !EA::IO::IsFilePathSeparator(sPath.back()))
                sPath.push_back((typename String::value_type)EA::IO::kFilePathSeparator8);

            return sPath;
        }


        /// CopyDirectoryTree
        /// 
        /// Copies the directory tree rooted at pDirectoryPathSource to a directory tree
        /// rooted at pDirectoryPathDest. If clearInitialDest is true then the directory
        /// tree at pDirectoryPathDest is cleared of any contents before proceeding, else
        /// the tree at pDirectoryPathSource is merged into and pDirectoryPathDest, with 
        /// duplicate files replacing the ones at pDirectoryPathDest. The destination tree
        /// subdirectories are created as-needed during the process. The copy does not 
        /// copy file dates, access rights, or any other file metadata.
        /// The return value indicates if an error was encountered.
        ///
        bool CopyDirectoryTree(const char8_t* pDirectoryPathSource, const char8_t* pDirectoryPathDest, bool clearInitialDest, Error& error);


        /// CompareDirectoryTree
        /// 
        /// Compares the contents of two directory trees recusrively. Sets bEqual to true if 
        /// they are identical. Identicality includes file name case sentivity as well as the file contents.
        /// The return value indicates if an error was encountered. The compare does not 
        /// compare file dates, access rights, or any other file metadata.
        ///
        bool CompareDirectoryTree(const char8_t* pDirectoryPathSource, const char8_t* pDirectoryPathDest, bool& bEqual, 
                                    String& sEqualFailureDescription, bool bEnableDescriptionDetail, Error& error);


        /// CreateSizedFile
        /// 
        /// Creates a file of the given size. 
        /// The file is closed upon return.
        ///
        bool CreateSizedFile(const char8_t* pFilePath, uint64_t fileSize, Error& error);


        /// GetRealStream
        ///
        /// Returns the 'real' stream associated with pStream if pStream isn't the real stream itself.
        /// Some IStream classes (e.g. StreamBuffer) are really wrappers for another stream. This function
        /// returns the wrapped stream in such cases. This is useful, for example, for getting the concrete
        /// class behind the actual stream data.
        ///
        /// To consider: Implement this function in EAIO, since it's a common need.
        ///
        EA::IO::IStream* GetRealStream(EA::IO::IStream* pStream);


        /// GetStreamInfo
        ///
        /// Gets the state and name for the stream, which will be the file path if the stream is a FileStream.
        /// This function is used in error handling and debugging.
        /// Returns the stream type (IStream::GetType) of the 'real' stream associate with pStream, if pStream
        /// isn't the real stream itself.
        ///
        uint32_t GetStreamInfo(EA::IO::IStream* pStream, uint32_t& state, String& sName);


        /// WriteStreamToFile
        ///
        /// Writes an EA::IO::IStream to a disk file.
        /// Overwrites the file if it already exists.
        ///
        bool WriteStreamToFile(EA::IO::IStream* pSourceStream, const char8_t* pDestFilePath, Error& error);


        /// GetFileHash
        /// 
        /// Gets the HashValueMD5 value for the given file.
        ///
        bool GetFileHash(const char8_t* pFilePath, HashValueMD5& fileHash, Error& error);
        bool GetFileHash(EA::IO::IStream* pSourceStream, HashValueMD5& fileHash, Error& error);


        /// FileCopier
        ///
        /// Copies a file while optionally calculating its full file hash and/or its diff hash.
        /// 
        class FileCopier : public ErrorBase
        {
        public:
            FileCopier();

            /// Copy a file from a source path to a dest path. 
            bool CopyFile(const char8_t* pFileSourceFullPath, const char8_t* pFileDestFullPath);

            /// Copy a file from a source path to a dest path, while calculating hash info.
            bool CopyFile(const char8_t* pFileSourceFullPath, const char8_t* pFileDestFullPath, 
                            bool generateFileHash, bool generateDiffHash, size_t diffBlockSize);

            /// Copy a file from a source path to a dest directory. 
            /// This is useful for copying a file from the source tree to the dest patch tree, while calculating
            /// the resulting file hash and relative path.
            bool CopyFile(const char8_t* pFileSourceFullPath, const char8_t* pFileSourceBaseDirectry, 
                            const char8_t* pFileDestBaseDirectory, bool generateFileHash, bool generateDiffHash, size_t diffBlockSize);

            // The following functions give information that you might want to retrieve after the copy is successfully completed.
            void GetFileSize(uint64_t&) const;
            void GetFileHash(HashValueMD5&) const;
            void GetDiffHash(BlockHashArray&) const;
            void GetDestPath(String& destRelativePath, String& destFullPath) const;

        protected:
            static const size_t kBufferSize = kDiffBlockSizeMax;

            FileBuffer     mBuffer;                 /// 
            uint64_t       mFileSize;               /// The size of the copied file.
            HashValueMD5   mFileHash;               /// Calculated hash for the file.
            BlockHashArray mDiffHashArray;          /// The calculated BlockHash array for the file. This is for diff information.
            String         mDestRelativePath;       /// The calculated relative file path of the copied file.
            String         mDestFullPath;           /// The calculated full path of the copied file.
        };


        /// FileReader
        ///
        /// Reads a file for the sole purpose of calculating its full file hash and/or its diff hash.
        /// To consider: Remove this class and use FileCopier (without copying) instead, as its 
        ///              CopyFile implementation may be very similar to our ReadFile implementation.
        /// 
        class FileReader : public ErrorBase
        {
        public:
            FileReader();

            /// diffBlockSize must be a power of two
            bool ReadFile(const char8_t* pFileSourceFullPath, bool generateFileHash, bool generateDiffHash, size_t diffBlockSize);

            void GetFileSize(uint64_t&) const;
            void GetFileHash(HashValueMD5&) const;
            void GetDiffHash(BlockHashArray&) const;

        protected:
            static const size_t kBufferSize = kDiffBlockSizeMax;
            typedef eastl::vector<uint8_t, EA::Patch::CoreAllocator> Buffer;

            Buffer         mBuffer;                 /// 
            uint64_t       mFileSize;               /// The size of the copied file.
            HashValueMD5   mFileHash;               /// Calculated hash for the file.
            BlockHashArray mDiffHashArray;          /// The calculated BlockHash array for the file. This is for diff information.
        };


        /// DirectoryCleaner
        ///
        /// Clears a directory, deleting its contents.
        /// Usually this is used to clean up a failed or cancelled patch install.
        /// This is used for only patch directories generated by the Maker.
        /// 
        class DirectoryCleaner : public ErrorBase
        {
        public:
            /// Deletes all files under pDirectoryFullPath. If deleteBaseDirectory is true then
            /// the base pDirectoryFullPath is deleted as well. Returns false and sets the Error
            /// if there was any failure.
            bool CleanDirectory(const char8_t* pDirectoryFullPath);
        };


        /// File
        /// 
        /// Implements a wrapper to blocking file IO through EAIO.
        /// This wrapper is useful because it handles Error setting for us and has a consistent interface that
        /// is different from fopen and friends but makes for streamlined handling in our code. You can safely
        /// chain multiple calls in a row and check the status at the end of all of them. Upon the first error
        /// encountered, the file is closed and all further usage has no effect. Once an error is returned then
        /// any future calls will return error until ClearLastError is called.
        ///
        /// This class is not internally thread-safe. You cannot use a single instance of it simultaneously 
        /// between multiple threads. Nor can two instances of this class safely refer to the same disk file.
        /// This latter restriction is because the ReadPosition and WritePosition functions may not be atomic
        /// for the given platform.
        ///
        /// To do: Make it so that EA::IO::StreamBuffer is optionally applied to the File class.
        /// 
        class File : public ErrorBase
        {
        public:
            File();
           ~File();

            File(const File&);
            void operator=(const File&);

            /// SetStream
            /// Sets an EA::IO::IStream interface to use.
            void SetStream(EA::IO::IStream* pStream, const char8_t* pName,
                           EA::IO::size_type readBufferSize = kReadBufferSizeDefault, 
                           EA::IO::size_type writeBufferSize = kWriteBufferSizeDefault);
            EA::IO::IStream* GetStream();

            // Default buffer sizes.
            #if (EAPATCH_DEBUG >= 4)
                static const EA::IO::size_type kReadBufferSizeDefault  = 0;    // Zero sizes aid in debugging.
                static const EA::IO::size_type kWriteBufferSizeDefault = 0;
            #else
                static const EA::IO::size_type kReadBufferSizeDefault  = 65536;
                static const EA::IO::size_type kWriteBufferSizeDefault = 65536;
            #endif

            /// Opens a file. 
            /// If the open fails then the Error object filled out and false is returned.
            /// pPath must be a valid (e.g. non-NULL) string, else this function will crash.
            /// This function will also create the directory for written files if the directory
            /// doesn't already exist. 
            bool Open(const char8_t* pPath, int nAccessFlags, int nCreationDisposition, 
                      int nSharing = EA::IO::FileStream::kShareRead, 
                      int nUsageHints = EA::IO::FileStream::kUsageHintSequential,
                      EA::IO::size_type readBufferSize = kReadBufferSizeDefault, 
                      EA::IO::size_type writeBufferSize = kWriteBufferSizeDefault);

            /// Tells if the file is open. There is no error reporting associated with this function.
            bool IsOpen() const
                { return mpStream ? (mpStream->GetAccessFlags() != 0) : false; }

            /// Reads from a file. If the read fails then the Error object is filled out, 
            /// the file is closed, and false is returned.
            /// pData must be a valid (e.g. non-NULL) string, else this function will crash.
            /// If expectReadSize is true, then the function fails if readSizeResult != readSize (i.e. if the file ended prematurely).
            bool Read(void* pData, uint64_t readSize, uint64_t& readSizeResult, bool expectReadSize);

            /// Reads from a file at the specified offset. 
            /// If the offset is beyond the current file limit, the read is considered failed. 
            /// If the read fails then the Error object is filled out, the file is closed, and false is returned.
            /// pData must be a valid (e.g. non-NULL) string, else this function will crash.
            bool ReadPosition(void* pData, uint64_t readSize, uint64_t readPosition, uint64_t& readSizeResult, bool expectReadSize);

            /// Writes to a file. If the write fails then the Error object is filled out, 
            /// the file is closed, and false is returned.
            /// pData must be a valid (e.g. non-NULL) string, else this function will crash.
            bool Write(const void* pData, uint64_t writeSize);

            /// Writes to a file at the specified offset. 
            /// If the offset is beyond the current file limit, the file is extended. 
            /// If the write fails then the Error object is filled out, the file is closed, and false is returned.
            /// pData must be a valid (e.g. non-NULL) string, else this function will crash.
            bool WritePosition(const void* pData, uint64_t writeSize, uint64_t writePosition);

            /// Gets the current file position.
            /// If the operation fails then the Error object is filled out, the file is closed, and false is returned.
            bool GetPosition(int64_t& position, EA::IO::PositionType positionType = EA::IO::kPositionTypeBegin);

            /// Sets the current file position.
            /// If the operation fails then the Error object is filled out, the file is closed, and false is returned.
            bool SetPosition(int64_t position, EA::IO::PositionType positionType = EA::IO::kPositionTypeBegin);

            /// Sets the current file size.
            bool GetSize(uint64_t& size);

            /// Flushes the file write buffer to disk.
            bool Flush();

            /// Closes a file. If the close fails then the Error object is filled out and false is returned.
            bool Close();

            /// Returns the total number of File objects that are open. This is useful for diagnostic and debugging purposes.
            static int32_t GetOpenFileCount();

        protected:
            SystemErrorId GetStreamSystemErrorId() const;
            void          HandleFileError(ErrorClass errorClass, SystemErrorId systemErrorId, EAPatchErrorId eaPatchErrorId, const char8_t* pPath = NULL);

        protected:
            static int32_t       mOpenFileCount;    /// Count of total number of Files that are open.

            EA::IO::IStream*     mpStream;          /// The stream we work with, which may be set by the user or may refer to mFileStream.
            EA::IO::FileStream   mFileStream;       /// Used if we opened a file stream ourselves, in which case (mpStream == &mFileStream);
            String               mFilePath;         /// The file path, if mFileStream is used.
            EA::IO::StreamBuffer mStreamBuffer;     /// Optional buffering for the stream.
            EA::IO::size_type    mReadBufferSize;   /// Size of read buffer. 0 if buffering is disabled.
            EA::IO::size_type    mWriteBufferSize;  /// Size of write buffer. 0 if buffering is disabled.
        };


        /// FileChecksumReader
        ///
        /// Reads a file for the sole purpose of calculating rolling checksums of its data.
        ///
        class FileChecksumReader : public ErrorBase
        {
        public:
            FileChecksumReader();

            /// diffBlockSize must be a power of two
            bool Open(const char8_t* pFileSourceFullPath, size_t diffBlockSize);
            bool Close();

            bool GoToFirstByte(uint32_t& checksum, bool& bDone, size_t offset = 0);
            bool GoToNextByte(uint32_t& checksum, bool& bDone);     // Call this repeatedly until there is a block match or bDone gets set to true.
            bool GoToNextBlock(uint32_t& checksum, bool& bDone);    // Used instead of GoToNextByte when there was a block match.

            /// Gets the HashValueMD5Brief for the current block (the one last moved to with GoToFirstBlock or GoToNextBlock).
            void GetCurrentBriefHash(HashValueMD5Brief& hashValueBrief, uint64_t& blockPosition) const;

            /// Gets the size of the file that was opened by Open. 
            /// Returns 0 for a file that's not open.
            uint64_t GetFileSize();

            /// Returns mBlockPosition
            uint64_t GetBlockPosition() const
                { return mBlockPosition; }

        protected:
            static const size_t kBufferSize = kDiffBlockSizeMax;
            typedef eastl::vector<uint8_t, EA::Patch::CoreAllocator> Buffer;

        protected:
            Buffer   mBuffer;                   /// What we read the file bytes into. This is a ring buffer.
            size_t   mBufferSize;               /// Same as mBuffer.size().
            size_t   mBufferReadPosition;       /// Where within the mBuffer (ring buffer) we read the last byte.
            File     mFile;                     /// The File object.
            size_t   mDiffBlockSize;            /// See Hash.h, and kDiffBlockSizeMin/Max, etc.
            size_t   mDiffBlockShift;           /// log2(mDiffBlockSize).
            uint64_t mBlockPosition;            /// Where in the file the current block starting position refers to.
            uint32_t mPrevChecksum;             /// Used for maintaining the rolling checksum.
            size_t   mByteBufferIndex;
            uint64_t   mValidBytes;
            uint8_t  mByteBuffer[kHelperBufferSize];
            bool     mbDone;                    /// True if GoToFirstByte or GoToNextByte resulted in reading the last byte of the file.
        };


        /// FileComparer
        ///
        /// Compares segments from two different files.
        /// To consider: Merge this with FileSpanCopier, as there is some similarity.
        ///
        class FileComparer : public ErrorBase
        {
        public:
            FileComparer();
           ~FileComparer();

            bool Open(const char8_t* pSourceFilePath, const char8_t* pDestinationFilePath);
            bool IsOpen() const;
            bool Close();

            // Returns true if the given spans have the same data in both files.
            bool CompareSpan(uint64_t sourceStartPos, uint64_t destStartPos, uint64_t compareCount, bool& bEqual);

        protected:
            File mFileSource;
            File mFileDest;
        };



        /// FileSpanCopier
        ///
        /// Copies spans of a file to a location in another file.
        /// 
        class FileSpanCopier : public ErrorBase
        {
        public:
            FileSpanCopier();

            bool Open(const char8_t* pSourceFilePath, const char8_t* pDestinationFilePath);
            bool Close();

            // Copies a span from [sourceStartPos, sourceStarPos + copyCount) to destStartPos.
            bool CopySpan(uint64_t sourceStartPos, uint64_t destStartPos, uint64_t copyCount);

        protected:
            File mFileSource;
            File mFileDest;
        };


        /// Opens two files. 
        /// If either file open fails then the other is closed, the Error object filled out, and false is returned.
        bool OpenTwoFiles(File& file1, const char8_t* pPath1, int nAccessFlags1, int nCreationDisposition1, int nSharing1, int nUsageHints1, 
                          File& file2, const char8_t* pPath2, int nAccessFlags2, int nCreationDisposition2, int nSharing2, int nUsageHints2,
                          Error& error);

    } // namespace Patch

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard



 




