///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/MemoryFileSystem.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_MEMORYFILESYSTEM_H
#define EAPATCHCLIENT_MEMORYFILESYSTEM_H


#include <EABase/eabase.h>
#include <EASTL/list.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/set.h>
#include <EAStdC/EARandom.h>
#include <eathread/eathread_futex.h>
#include <coreallocator/icoreallocator_interface.h>
#include <EAIO/EAStream.h>
#include <EAAssert/eaassert.h>


#ifdef _MSC_VER
    #pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
    #pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
    #pragma warning(disable: 4275)  // non dll-interface class used as base for dll-interface class.
#endif


namespace EA
{
    namespace MFS
    {
        // To do: Try to use rwfilesystem memmap support (file:///C:/packages/rwfilesystem/1.20.03/doc/html/userguide/memmapdevicedriver.html)
        //        We'll probably need to augment rwfilesystem's support somewhat to make this work. However, the ICoreFileSystem base class
        //        approach below will probably allow this to work fairly well, once we are able to enable ICoreFileSystem.

        // Defines the maximum number of bytes a path is expected to be, including the 
        // trailing 0 char. Thus the maximum path strlen is kMaxMFSPathLength - 1.
        const size_t kMaxMFSPathLength = 512;

        ///////////////////////////////////////////////////////////////////////////////////
        // Begin temporary defines
        // Temporary until we migrate to being dependent on the ICoreFileSystem package.
        ///////////////////////////////////////////////////////////////////////////////////

        typedef EA::IO::IStream      IStream;
        typedef EA::IO::size_type    size_type;
        typedef EA::IO::off_type     off_type;
        typedef EA::IO::PositionType PositionType;
        const   size_type            kSizeTypeError = EA::IO::kSizeTypeError;
        const   size_type            kSizeTypeDone  = EA::IO::kSizeTypeDone;

        enum EntryType
        {
            ENTRY_TYPE_NONE      = 0x00, 
            ENTRY_TYPE_FILE      = 0x01,
            ENTRY_TYPE_DIRECTORY = 0x02 
        };

        enum FileAttributes
        {
            FILE_ATTR_NONE       = 0x0000,
            FILE_ATTR_READABLE   = 0x0001,
            FILE_ATTR_WRITABLE   = 0x0002,
            FILE_ATTR_EXECUTABLE = 0x0004,
            FILE_ATTR_HIDDEN     = 0x0040,
            FILE_ATTR_SYSTEM     = 0x0080,
            FILE_ATTR_ENCRYPTED  = 0x0100
        };

        enum OpenFlags
        {
            OPEN_NONE          = 0x0000,    /// No specified flags. Also used to indicate that a given IO stream is closed.
            OPEN_READ          = 0x0001,    /// Used for identifying read access to an entity.
            OPEN_WRITE         = 0x0002,    /// Used for identifying write access to an entity.
            OPEN_READ_WRITE    = 0x0003,    /// Used for identifying both read and write access to an entity.

            OPEN_SEQUENTIAL    = 0x0100,    /// Access is intended to be sequential from beginning to end. The system can use this as a hint to optimize file caching. 
            OPEN_RANDOM        = 0x0200,    /// Access is intended to be random. 
            OPEN_UNBUFFERED    = 0x0400,    /// The file or device is being opened with no system caching for data reads and writes. This flag does not affect hard disk caching or memory mapped files.
            OPEN_ENCRYPTED     = 0x0800,    /// Force the file to be opened as an encrypted file. Some systems have the concept of a signed/encrypted IO API.
            OPEN_OVERLAPPED    = 0x1000,    /// The file is being opened or created for asynchronous I/O. 
            OPEN_WRITE_THROUGH = 0x2000     /// Write operations will not go through any intermediate cache, they will go directly to disk.
        };

        // Specifies aspects of how to create or not create a file during opening of it.
        enum Disposition
        {
            DISPOSITION_CREATE_NEW        = 1,      /// Fails if file already exists.
            DISPOSITION_CREATE_ALWAYS     = 2,      /// Never fails, always opens or creates and truncates to 0.
            DISPOSITION_OPEN_EXISTING     = 3,      /// Fails if file doesn't exist, keeps contents.
            DISPOSITION_OPEN_ALWAYS       = 4,      /// Never fails, creates if doesn't exist, keeps contents.
            DISPOSITION_TRUNCATE_EXISTING = 5,      /// Fails if file doesn't exist, but truncates to 0 if it does.
            DISPOSITION_DEFAULT           = 6       /// Default disposition, depending on file open flags.
        };

        enum ShareFlags
        {
            SHARE_NONE   = 0x00,     /// No sharing.
            SHARE_READ   = 0x01,     /// Allow sharing for reading.
            SHARE_WRITE  = 0x02,     /// Allow sharing for writing.
            SHARE_DELETE = 0x04      /// Allow sharing for deletion.
        };

        ///////////////////////////////////////////////////////////////////////////////////
        // End temporary defines
        ///////////////////////////////////////////////////////////////////////////////////




        ///////////////////////////////////////////////////////////////////////
        // CoreAllocator
        ///////////////////////////////////////////////////////////////////////

        /// Gets the default EAIO ICoreAllocator set by the SetAllocator function.
        /// If SetAllocator is not called, the ICoreAllocator::GetDefaultAllocator 
        /// allocator is returned.
        Allocator::ICoreAllocator* GetAllocator();


        /// This function lets the user specify the default memory allocator this library will use.
        /// For the most part, this library avoids memory allocations. But there are times 
        /// when allocations are required, especially during startup. Currently the Font Fusion
        /// library which sits under EAIO requires the use of a globally set allocator.
        void SetAllocator(Allocator::ICoreAllocator* pCoreAllocator);


        /// Implements the EASTL allocator interface.
        /// Allocates memory from an instance of ICoreAllocator.
        ///
        /// Example usage:
        ///     // Use default allocator.
        ///     eastl::vector<Widget, CoreAllocator> widgetArray;
        ///
        ///     // Use custom allocator.
        ///     eastl::list<Widget, CoreAllocator> widgetList("UI/WidgetList", pSomeCoreAllocator);
        ///     widgetList.push_back(Widget());
        ///
        class CoreAllocator
        {
        public:
            typedef Allocator::ICoreAllocator  allocator_type;
            typedef CoreAllocator              this_type;

        public:
            CoreAllocator(const char* pName = EASTL_NAME_VAL("MemoryFileSystem/EASTL"));
            CoreAllocator(const char* pName, allocator_type* pAllocator);
            CoreAllocator(const char* pName, allocator_type* pAllocator, int flags);
            CoreAllocator(const CoreAllocator& x);
            CoreAllocator(const CoreAllocator& x, const char* pName);

            CoreAllocator& operator=(const CoreAllocator& x);

            void* allocate(size_t n, int flags = 0);
            void* allocate(size_t n, size_t alignment, size_t offset, int flags = 0);
            void  deallocate(void* p, size_t n);

            allocator_type* get_allocator() const;
            void            set_allocator(allocator_type* pAllocator);

            int  get_flags() const;
            void set_flags(int flags);

            const char* get_name() const;
            void        set_name(const char* pName);

        public: // Public because otherwise VC++ generates (possibly invalid) warnings about inline friend template specializations.
            allocator_type* mpCoreAllocator;
            int             mnFlags;    // Allocation flags. See ICoreAllocator/AllocFlags.

            #if EASTL_NAME_ENABLED
                const char* mpName; // Debug name, used to track memory.
            #endif
        };

        bool operator==(const CoreAllocator& a, const CoreAllocator& b);
        bool operator!=(const CoreAllocator& a, const CoreAllocator& b);


        ///////////////////////////////////////////////////////////////////////
        // Public types
        ///////////////////////////////////////////////////////////////////////

        typedef eastl::basic_string<char8_t, CoreAllocator> String8;



        ///////////////////////////////////////////////////////////////////////
        // Internal
        ///////////////////////////////////////////////////////////////////////

        class MemoryFile;

        namespace Internal
        {
            struct MFSNode;

            // To consider: Switch this to a generic ICoreAllocator.
            typedef String8                                     MFSName;
            typedef eastl::vector<uint8_t,       CoreAllocator> MFSData;
            typedef eastl::list<MFSNode,         CoreAllocator> MFSNodeList;
            typedef eastl::list<MFSNode*,        CoreAllocator> MFSNodePtrList;
            typedef eastl::list<MemoryFile,      CoreAllocator> MFSMemoryFileList;
            typedef eastl::list<MemoryFile*,     CoreAllocator> MFSMemoryFilePtrList;
            typedef eastl::set<MFSNode*,         CoreAllocator> MFSNodePtrSet;

            typedef eastl::set<MemoryFile*, eastl::less<MemoryFile*>, CoreAllocator> MFSMemoryFilePtrSet;

            struct MFSNameCompare{ // Case-insensitive compare.
                bool operator()(const MFSName& a, const MFSName& b)
                    { return (a.comparei(b) < 0); }
            };
            typedef eastl::set<MFSName, MFSNameCompare, CoreAllocator> MFSNameSet;



            // MFSNode represents a file or directory in a directory tree.
            struct MFSNode
            {
                MFSName              mName;                // File or directory name.
                MFSData              mData;                // The file contents themselves. We are implementing a file system in memory, so a file's contents are memory.
                uint64_t             mCreationTime;        // When the file was created.
                EntryType            mNodeType;            // File or directory type.
                MFSNode*             mpNodeParent;         // Parent MFSNode. Null for the root node and any standalone nodes (uncommon).
                MFSNodeList          mNodeList;            // Child nodes.
                MFSMemoryFilePtrList mMemoryFilePtrList;   // Could save memory by making this a pointer instead of an object.

                MFSNode();
                MFSNode(const MFSNode& node);
                MFSNode& operator=(const MFSNode& node);

                void AddOpenFilePtr(MemoryFile* pMemoryFile);
                void RemoveOpenFilePtr(MemoryFile* pMemoryFile);
            };
        }



        ///////////////////////////////////////////////////////////////////////
        // MemoryFileSystem
        //
        // This class implements the existing ICoreFileSystem interface.
        // To do: Enable the ICoreFileSystem base class.
        ///////////////////////////////////////////////////////////////////////

        class MemoryFileSystem // : public ICoreFileSystem    
        {
        public:
            MemoryFileSystem();
            virtual ~MemoryFileSystem();

            bool Init();
            void Shutdown();
            void Reset();
    
            virtual bool     FileExists(const char8_t* filePath);
            virtual bool     FileCreate(const char8_t* filePath);
            virtual bool     FileDestroy(const char8_t* filePath);
            virtual bool     FileCopy(const char8_t* filePathSource, const char8_t* filePathDestination);
            virtual bool     FileMove(const char8_t* filePathSource, const char8_t* filePathDestination);
            virtual bool     FileRename(const char8_t* filePathSource, const char8_t* filePathDestination);
            virtual uint64_t FileGetSize(const char8_t* filePath);

            // Uses enum FileAttributes as flags.
            virtual int  FileGetAttributes(const char8_t* filePath);
            virtual bool FileSetAttributes(const char8_t* filePath, int attributes, bool enable);

            virtual uint64_t FileGetTime(const char8_t* filePath);
            virtual bool     FileSetTime(const char8_t* filePath, uint64_t time);

            // Return value is platform-specific.
            virtual uint64_t FileGetLocation(const char8_t* filePath);
            virtual bool     FileMakeTemp(char8_t* filePath, size_t filePathCapacity, const char8_t* pDirectory = NULL, 
                                          const char8_t* pFileName = NULL, const char8_t* pExtension = NULL);
            // Directory functions
            virtual bool DirectoryExists(const char8_t* directoryPath);
            virtual bool DirectoryCreate(const char8_t* directoryPath);
            virtual bool DirectoryDestroy(const char8_t* directoryPath);
            virtual bool DirectoryCopy(const char8_t* directoryPathSource, const char8_t* directoryPathDestination);
            virtual bool DirectoryMove(const char8_t* directoryPathSource, const char8_t* directoryPathDestination);
            virtual bool DirectoryRename(const char8_t* directoryPathSource, const char8_t* directoryPathDestination);
            virtual int  DirectoryGetCurrent(char8_t* directoryPath, size_t directoryPathCapacity);
            virtual bool DirectorySetCurrent(const char8_t* directoryPath);
            virtual int  DirectoryGetTemp(char8_t* directoryPath, size_t directoryPathCapacity);

            // Defines stream access flags, much like file access flags.
            virtual IStream* OpenFile(const char8_t* filePath, int openFlags = OPEN_READ, int disposition = DISPOSITION_DEFAULT, int sharing = SHARE_READ);
            virtual void     DestroyFile(IStream*);

            // Error functions
            virtual uint32_t GetLastError();
            virtual void     SetLastError(uint32_t error);

            // Gives a readable error string for an error id.
            // Acts like Windows' FormatMessage function.
            virtual char8_t* FormatError(uint32_t errorId, char8_t* buffer, size_t bufferCapacity);

        public:
            // Custom functions
            Internal::MFSNode* GetRootNode();

            void Lock();    // Mutex lock.
            void Unlock();  // Mutex unlock.

            uint64_t GetFileTime(); // Returns seconds since 1970, based on calendar time.
            bool     ValidatePath(const char8_t* pPath, EntryType entryType = ENTRY_TYPE_NONE, size_t* pErrorPos = NULL) const;
            bool     Validate(String8& sOutput) const;   // Internal validation.
            void     PrintTree(String8& sOutput) const;

            // Must be set before Init.
            char8_t GetFilePathSeparator() const;
            void    SetFilePathSeparator(char8_t filePathSeparator);  // Defaults to '/'

            // Must be set before Init.
            bool    GetFilePathCaseSensitive() const;
            void    SetFilePathSeparator(bool filePathCaseSensitive);  // Defaults to false

        protected:
            // Lower level internal API functions.
            friend class MemoryFile;
            friend class MemoryFileSystemIteration;

            bool     EntryExists(EntryType entryType, const char8_t* path);
            bool     EntryCreate(EntryType entryType, const char8_t* path);
            bool     EntryDestroy(EntryType entryType, const char8_t* path, bool requireEmpty);
            bool     EntryCopy(EntryType entryType, const char8_t* pathSource, const char8_t* pathDestination, bool bOverwrite);
            bool     EntryMove(EntryType entryType, const char8_t* pathSource, const char8_t* pathDestination, bool bOverwrite);
            bool     EntryRename(EntryType entryType, const char8_t* pathSource, const char8_t* pathDestination, bool bOverwrite);

            bool     PossiblyStripDirSeparator(const char8_t*& pDirPath, char8_t* dirTemp);

            void     AddOpenFilePtr(MemoryFile* pMemoryFile);
            void     RemoveOpenFilePtr(MemoryFile* pMemoryFile);

            bool     ValidateNode(String8& sOutput, const Internal::MFSNode* pNode, 
                                    Internal::MFSNameSet& filePathSet, Internal::MFSMemoryFilePtrSet& filePtrSet) const;

            MemoryFileSystem(const MemoryFileSystem&){}
            void operator=(const MemoryFileSystem&){}

        protected:
            // Lowest level internal API functions.
            static bool     MFSRemoveNode(Internal::MFSNode* pNode);
            static bool     MFSMoveNode(Internal::MFSNode* pSourceNode, Internal::MFSNode* pDestNodeParent);
            static bool     MFSOpenFilesExistRecursive(const Internal::MFSNode* pNode);
            static int      MFSGetCurrentFileOpenAccess(Internal::MFSNode* pNode);
            static void     MFSCopyDirectory(Internal::MFSNode* pNodeSource, Internal::MFSNode* pNodeDest, const char8_t* dirNameDest);
            static void     MFSCopyFile(Internal::MFSNode* pNodeSource, Internal::MFSNode* pNodeDest, const char8_t* fileNameDest);

            bool               MFSPathEndsWithSeparator(const char8_t* pPath) const;
            char8_t*           MFSFileNameFromPath(const char8_t* pPath, char8_t* pFileName);
            char8_t*           MFSStrstr(const char8_t* pString, const char8_t* pSubString) const;
            bool               MFSPathIsChild(const char8_t* pPathParent, const char8_t* pPathPossibleChild) const;
            bool               MFSValidatePath(const char8_t* pPath, EntryType entryType, size_t* pErrorPos) const;
            const char8_t*     MFSGetNextNodeNameFromPath(const char8_t* pPath, char8_t* pNodeName) const;
            Internal::MFSNode* MFSNodeFromPath(Internal::MFSNode* pRootNode, const char8_t* pPath, bool bReturnParent);
            Internal::MFSNode* MFSCreateFSNodeFromPath(Internal::MFSNode* pRootNode, const char8_t* pPath, uint32_t* pError);
            Internal::MFSNode* MFSCreateParentNodeFromPath(Internal::MFSNode* pRootNode, const char8_t* pPath, bool returnPreExisting, uint32_t* pError);
            char8_t*           MFSPathFromFSNode(const Internal::MFSNode* pNode, char8_t* pPath) const;

        protected:
            Internal::MFSNode              mRootNode;                            // Directory tree root.
            Internal::MFSMemoryFilePtrList mOpenFilePtrList;                     // List of open files.   To consider: this may not be necessary, since the files are already in the nodes.
            Internal::MFSMemoryFileList    mFileList;                            // List of file objects created through OpenFile.
            mutable EA::Thread::Futex      mFutex;                               // 
            char8_t                        mCurrentDirectory[kMaxMFSPathLength]; // 
            char8_t                        mTempDirectory[kMaxMFSPathLength];    // Default is /temp/
            mutable uint32_t               mLastError;                           //
            char8_t                        mFilePathSeparator;                   // Either / (default) or \. Default is /
            bool                           mFilePathCaseSensitive;               // True if file system paths are case-sensitive. Default is false (case-insensitive). Currently case-sensitivity supports ASCII only characters; other Unicode characters are treated case-sensitively.
            EA::StdC::Random               mRand;                                // 
        };


        MemoryFileSystem* GetDefaultFileSystem();
        void              SetDefaultFileSystem(MemoryFileSystem* pMemoryFileSystem);



        ///////////////////////////////////////////////////////////////////////
        // MemoryFileSystemIteration
        ///////////////////////////////////////////////////////////////////////

        class MemoryFileSystemIteration // : public ICoreFileSystemIteration  (hopefully we can enable this inheritance some day)
        {
        public:
            MemoryFileSystemIteration();
            virtual ~MemoryFileSystemIteration();

            virtual bool IterateBegin(const char8_t* directoryPath, const char8_t* wildcardFilter);
            virtual bool IterateNext();
            virtual void IterateEnd();

            virtual EntryType      GetEntryType();
            virtual const char8_t* GetEntryName();

            virtual uint64_t GetEntryTime();
            virtual uint64_t GetEntrySize();

            virtual uint32_t GetLastError();

        public:
            // Allows you to override the MemoryFileSystem instance used by this class.
            MemoryFileSystem* GetMemoryFileSystem() const;
            void SetMemoryFileSystem(MemoryFileSystem*);

        protected:
            friend class DirectoryIterator;

            MemoryFileSystem*               mpMemoryFileSystem;
            char8_t                         mWildcardFilter[kMaxMFSPathLength];
            Internal::MFSNode*              mpDirectoryNode;
            Internal::MFSNodeList::iterator mCurrentNode;
            char8_t                         mFilePathSeparator;
            bool                            mFilePathCaseSensitive;
        };


        ///////////////////////////////////////////////////////////////////////
        // DirectoryIterator
        //
        // Much the same as EAIO DirectoryIterator. Wraps MemoryFileSystemIteration.
        ///////////////////////////////////////////////////////////////////////

        class DirectoryIterator
        {
        public:
            DirectoryIterator();
          //DirectoryIterator(const DirectoryIterator& x);
          //DirectoryIterator& operator=(const DirectoryIterator& x);

            static const size_t kMaxEntryCountDefault = 1048576;

            typedef eastl::basic_string<char8_t, CoreAllocator> EntryString;

            struct Entry
            {
                Entry(EntryType entry = ENTRY_TYPE_NONE, const char8_t* pName = NULL);

                EntryType   mType;          /// Either ENTRY_TYPE_FILE or ENTRY_TYPE_DIRECTORY.
                EntryString msName;         /// This may refer to the directory or file name alone, or may be a full path. It depends on the documented use.
                uint64_t    mTime;          /// See enum FileTimeType.
                uint64_t    mSize;          /// Refers to the last saved size of the file.
            };

            typedef eastl::list<Entry, CoreAllocator> EntryList;

            size_t Read(const char8_t* pDirectory, EntryList& entryList, const char8_t* pFilterPattern = NULL, 
                        int nDirectoryEntryFlags = ENTRY_TYPE_FILE, size_t maxResultCount = kMaxEntryCountDefault,
                        bool bReadFileStat = true);

            size_t ReadRecursive(const char8_t* pBaseDirectory, EntryList& entryList, const char8_t* pFilterPattern = NULL, 
                                 int nDirectoryEntryFlags = ENTRY_TYPE_FILE, bool bIncludeBaseDirectoryInSearch = true, 
                                 bool bFullPaths = true, size_t maxResultCount = kMaxEntryCountDefault, bool bReadFileStat = true);

        public:
            // Allows you to override the MemoryFileSystem instance used by this class.
            MemoryFileSystem* GetMemoryFileSystem() const;
            void SetMemoryFileSystem(MemoryFileSystem*);

        protected:
            size_t            mnListSize;             /// Size of list. Used to compare to maxResultCount. Can't use list::size at runtime due to performance reasons and because the user list may be non-empty.
            int               mnRecursionIndex;       /// The ReadRecursive recursion level. Used by recursive member functions.
            const char8_t*    mpBaseDirectory;        /// Saved copy of pBaseDirectory for first ReadRecursive call. Used by recursive member functions.
            eastl_size_t      mBaseDirectoryLength;   /// Strlen of mpBaseDirectory.
            MemoryFileSystem* mpMemoryFileSystem;     /// 
        };



        ///////////////////////////////////////////////////////////////////////
        // MemoryFile
        ///////////////////////////////////////////////////////////////////////

        class MemoryFile : public IStream
        {
        public:
            enum { kTypeMemoryFile = 0x0eaf918e };

            enum Share
            {
                kShareNone   = 0x00,     /// No sharing.
                kShareRead   = 0x01,     /// Allow sharing for reading.
                kShareWrite  = 0x02,     /// Allow sharing for writing.
                kShareDelete = 0x04      /// Allow sharing for deletion.
            };

            enum UsageHints
            {
                kUsageHintNone       = 0x00,
                kUsageHintSequential = 0x01,
                kUsageHintRandom     = 0x02
            };

        public:
            MemoryFile(const char8_t* pPath8 = NULL);
            MemoryFile(const MemoryFile& mf);
            MemoryFile& operator=(const MemoryFile& mf);
           ~MemoryFile();

            int       AddRef();
            int       Release();

            void      SetPath(const char8_t* pPath8);
            size_t    GetPath(char8_t* pPath8, size_t nPathCapacity);

            bool      Open(int nAccessFlags = OPEN_READ, int nCreationDisposition = DISPOSITION_DEFAULT, 
                            int nSharing = SHARE_READ, int nUsageHints = 0); 
            bool      Close();
            uint32_t  GetType() const { return kTypeMemoryFile; }
            int       GetAccessFlags() const;
            int       GetState() const;

            size_type GetSize() const;
            bool      SetSize(size_type size);

            off_type  GetPosition(PositionType positionType = EA::IO::kPositionTypeBegin) const;
            bool      SetPosition(off_type position, PositionType positionType = EA::IO::kPositionTypeBegin);

            size_type GetAvailable() const;

            size_type Read(void* pData, size_type nSize);
            bool      Write(const void* pData, size_type nSize);
            bool      Flush();

        public:
            // Allows you to override the MemoryFileSystem instance used by this class.
            MemoryFileSystem* GetMemoryFileSystem() const;
            void SetMemoryFileSystem(MemoryFileSystem*);

        protected:
            friend class MemoryFileSystem;

        protected:
            int                 mRefCount;                          // 
            int                 mAccessFlags;                       // Read and/or write access.
            mutable int         mLastError;                         // 
            Internal::MFSNode*  mpFSNode;                           // This MemoryFileSystem node this file is associated with.
            MemoryFileSystem*   mpFileSystem;                       // 
            char8_t             mFilePath[kMaxMFSPathLength];       // 
            eastl_size_t        mPosition;                          // Current read/write position.
        };


        // MFS file system error types
        // These are returned by the MemoryFileSystem GetLastError() function.
        // These happen to mirror Microsoft file error types and values.
        #define MFS_ERROR_SUCCESS                0  // (0x0000) The operation completed successfully.
        #define MFS_ERROR_FILE_NOT_FOUND         2  // (0x0002) The system cannot find the file specified.
        #define MFS_ERROR_PATH_NOT_FOUND         3  // (0x0003) The system cannot find the path specified.
        #define MFS_ERROR_TOO_MANY_OPEN_FILES    4  // (0x0004) The system cannot open the file.
        #define MFS_ERROR_ACCESS_DENIED          5  // (0x0005) Access is denied.
        #define MFS_ERROR_NOT_ENOUGH_MEMORY      8  // (0x0008) Not enough storage is available to process this command.
        #define MFS_ERROR_OUTOFMEMORY           14  // (0x000E) Not enough storage is available to complete this operation.
        #define MFS_ERROR_CURRENT_DIRECTORY     16  // (0x0010) The directory cannot be removed.
        #define MFS_ERROR_NOT_SAME_DEVICE       17  // (0x0011) The system cannot move the file to a different disk drive.
        #define MFS_ERROR_WRITE_PROTECT         19  // (0x0013) The media is write protected.
        #define MFS_ERROR_SHARING_VIOLATION     32  // (0x0020) The process cannot access the file because it is being used by another entity.
        #define MFS_ERROR_LOCK_VIOLATION        33  // (0x0021) The process cannot access the file because another entity has locked a portion of the file.
      //#define MFS_ERROR_FILE_EXISTS           80  // (0x0050) The file exists.
        #define MFS_ERROR_CANNOT_MAKE           82  // (0x0052) The directory or file cannot be created.
        #define MFS_ERROR_INVALID_PARAMETER     87  // (0x0057) The parameter is incorrect.
        #define MFS_ERROR_OPEN_FAILED          110  // (0x006E) The system cannot open the device or file specified.
        #define MFS_ERROR_INVALID_NAME         123  // (0x007B) The filename, directory name, or volume label syntax is incorrect.
        #define MFS_ERROR_NEGATIVE_SEEK        131  // (0x0083) An attempt was made to move the file pointer before the beginning of the file.
        #define MFS_ERROR_DIR_NOT_ROOT         144  // (0x0090) The directory is not a subdirectory of the root directory.
        #define MFS_ERROR_DIR_NOT_EMPTY        145  // (0x0091) The directory is not empty.
        #define MFS_ERROR_BAD_PATHNAME         161  // (0x00A1) The specified path is invalid.
        #define MFS_ERROR_ALREADY_EXISTS       183  // (0x00B7) Cannot create a file when that file already exists.
        #define MFS_ERROR_DIRECTORY            267  // (0x010B) The directory name is invalid.
        #define MFS_ERROR_GENERIC               -1  // (0xffffffff) Generic error, EA::IO::kStateError
        #define MFS_ERROR_NOT_OPEN              -2  // (0xfffffffe) File not open, EA::IO::kStateNotOpen

    } // namespace MFS

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif









///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace EA
{
    namespace MFS
    {

        inline CoreAllocator::CoreAllocator(const char* EASTL_NAME(pName))
            : mpCoreAllocator(EA::MFS::GetAllocator()), mnFlags(0)
        {
            #if EASTL_NAME_ENABLED
                mpName = pName ? pName : "MemoryFileSystem/EASTL";
            #endif
            EA_ASSERT(mpCoreAllocator != NULL);
        }

        inline CoreAllocator::CoreAllocator(const char* EASTL_NAME(pName), allocator_type* pCoreAllocator)
            : mpCoreAllocator(pCoreAllocator), mnFlags(0)
        {
            #if EASTL_NAME_ENABLED
                mpName = pName ? pName : "MemoryFileSystem/EASTL";
            #endif
            EA_ASSERT(mpCoreAllocator != NULL);
        }

        inline CoreAllocator::CoreAllocator(const char* EASTL_NAME(pName), allocator_type* pCoreAllocator, int flags)
            : mpCoreAllocator(pCoreAllocator), mnFlags(flags)
        {
            #if EASTL_NAME_ENABLED
                mpName = pName ? pName : "MemoryFileSystem/EASTL";
            #endif
            EA_ASSERT(mpCoreAllocator != NULL);
        }

        inline CoreAllocator::CoreAllocator(const CoreAllocator& x)
            : mpCoreAllocator(x.mpCoreAllocator), mnFlags(x.mnFlags)
        {
            #if EASTL_NAME_ENABLED
                mpName = x.mpName;
            #endif
            EA_ASSERT(mpCoreAllocator != NULL);
        }

        inline CoreAllocator::CoreAllocator(const CoreAllocator& x, const char* EASTL_NAME(pName))
            : mpCoreAllocator(x.mpCoreAllocator), mnFlags(x.mnFlags)
        {
            #if EASTL_NAME_ENABLED
                mpName = pName ? pName : "MemoryFileSystem/EASTL";
            #endif
            EA_ASSERT(mpCoreAllocator != NULL);
        }

        inline CoreAllocator& CoreAllocator::operator=(const CoreAllocator& x)
        {
            // In order to be consistent with EASTL's allocator implementation, 
            // we don't copy the name from the source object.
            mpCoreAllocator = x.mpCoreAllocator;
            mnFlags         = x.mnFlags;
            EA_ASSERT(mpCoreAllocator != NULL);
            return *this;
        }

        inline void* CoreAllocator::allocate(size_t n, int /*flags*/)
        {
            // It turns out that EASTL itself doesn't use the flags parameter, 
            // whereas the user here might well want to specify a flags 
            // parameter. So we use ours instead of the one passed in.
            return mpCoreAllocator->Alloc(n, EASTL_NAME_VAL(mpName), (unsigned)mnFlags);
        }

        inline void* CoreAllocator::allocate(size_t n, size_t alignment, size_t offset, int /*flags*/)
        {
            // It turns out that EASTL itself doesn't use the flags parameter, 
            // whereas the user here might well want to specify a flags 
            // parameter. So we use ours instead of the one passed in.
            return mpCoreAllocator->Alloc(n, EASTL_NAME_VAL(mpName), (unsigned)mnFlags, (unsigned)alignment, (unsigned)offset);
        }

        inline void CoreAllocator::deallocate(void* p, size_t n)
        {
            return mpCoreAllocator->Free(p, n);
        }

        inline CoreAllocator::allocator_type* CoreAllocator::get_allocator() const
        {
            return mpCoreAllocator;
        }

        inline void CoreAllocator::set_allocator(allocator_type* pAllocator)
        {
            mpCoreAllocator = pAllocator;
        }

        inline int CoreAllocator::get_flags() const
        {
            return mnFlags;
        }

        inline void CoreAllocator::set_flags(int flags)
        {
            mnFlags = flags;
        }

        inline const char* CoreAllocator::get_name() const
        {
            #if EASTL_NAME_ENABLED
                return mpName;
            #else
                return "MemoryFileSystem/EASTL";
            #endif
        }

        inline void CoreAllocator::set_name(const char* pName)
        {
            #if EASTL_NAME_ENABLED
                mpName = pName;
            #else
                (void)pName;
            #endif
        }

        inline bool operator==(const CoreAllocator& a, const CoreAllocator& b)
        {
            return (a.mpCoreAllocator == b.mpCoreAllocator) &&
                   (a.mnFlags         == b.mnFlags);
        }

        inline bool operator!=(const CoreAllocator& a, const CoreAllocator& b)
        {
            return (a.mpCoreAllocator != b.mpCoreAllocator) ||
                   (a.mnFlags         != b.mnFlags);
        }
    }
}


#endif // Header include guard



 




