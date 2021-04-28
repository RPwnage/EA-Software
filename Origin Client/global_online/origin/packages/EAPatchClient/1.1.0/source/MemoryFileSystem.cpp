///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/MemoryFileSystem.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/MemoryFileSystem.h>
#include <EASTL/algorithm.h>
#include <EASTL/fixed_string.h>
#include <EAStdC/EADateTime.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>
#include <EAIO/FnMatch.h>
#include <EAAssert/eaassert.h>

#if defined(EA_PLATFORM_MICROSOFT)
    #if defined(_MSC_VER)
        #pragma warning(disable: 4505) // unreferenced local function has been removed
    #endif

    #if defined(EA_PLATFORM_WINDOWS)
        #include <WinSock.h>  // Needed for timeval declaration.
    #elif defined(EA_PLATFORM_XENON)
        #include <Xtl.h>
    #else
        #include <WinSock2.h> // Needed for timeval declaration.
    #endif

    #if defined(OpenFile) // If Windows has OpenFile #defined to OpenFileA/W, undo it.
        #undef OpenFile
    #endif
#endif



namespace EA
{
namespace MFS
{


const char8_t kFilePathSeparatorDefault = '/';


namespace Internal
{
    typedef eastl::fixed_string<char8_t, kMaxMFSPathLength, true, CoreAllocator> MFSPathString8;


    ///////////////////////////////////////////////////////////////////////////
    // MFSNode
    ///////////////////////////////////////////////////////////////////////////

    MFSNode::MFSNode()
      : mName()
      , mData()
      , mCreationTime(0)
      , mNodeType(ENTRY_TYPE_NONE)
      , mpNodeParent(NULL)
      , mNodeList()
      , mMemoryFilePtrList()
    {
    }

    MFSNode::MFSNode(const MFSNode& node)
    {
        operator=(node);
    }

    // operator= is a slightly tricky thing we use so that we can take advantage of 
    // eastl automatic recursive copy operations with a minimal amount of custom code.
    // To do: Make gParentNodeStack a member of MemoryFileSystem, as currently the 
    // implementation here effectively forces MemoryFileSystem as a singleton. We can 
    // get around this alternatively by making gParentNodeStack be thread-specific.
    // Also can get around it by making a global Mutex that guards usage of gParentNodeStack.
    eastl::vector<MFSNode*, CoreAllocator> gParentNodeStack;

    MFSNode& MFSNode::operator=(const MFSNode& node)
    {
        if(&node != this)
        {
            mName         = node.mName;
            mData         = node.mData;
            mCreationTime = node.mCreationTime;
            mNodeType     = node.mNodeType;

            EA_ASSERT(!gParentNodeStack.empty());
            mpNodeParent = gParentNodeStack.back();

            gParentNodeStack.push_back(this);
            mNodeList = node.mNodeList;
            gParentNodeStack.pop_back();

          //mMemoryFilePtrList // Intentionally do not copy this.
        }

        return *this;
    }


    void MFSNode::AddOpenFilePtr(MemoryFile* pMemoryFile)
    {
        mMemoryFilePtrList.push_back(pMemoryFile);
    }


    void MFSNode::RemoveOpenFilePtr(MemoryFile* pMemoryFile)
    {
        // This would be faster if MFSMemoryFilePtrList was a hash_set or set.
        MFSMemoryFilePtrList::iterator it = eastl::find(mMemoryFilePtrList.begin(), mMemoryFilePtrList.end(), pMemoryFile);

        EA_ASSERT(it != mMemoryFilePtrList.end());

        if(it != mMemoryFilePtrList.end())
            mMemoryFilePtrList.erase(it);
    }

    
} // namespace Local



///////////////////////////////////////////////////////////////////////////
// CoreAllocator
///////////////////////////////////////////////////////////////////////////

EA::Allocator::ICoreAllocator* gpCoreAllocator = NULL;


Allocator::ICoreAllocator* GetAllocator()
{
    if(!gpCoreAllocator)
        gpCoreAllocator = EA::Allocator::ICoreAllocator::GetDefaultAllocator();

    return gpCoreAllocator;
}


void SetAllocator(Allocator::ICoreAllocator* pCoreAllocator)
{
    gpCoreAllocator = pCoreAllocator;
}




///////////////////////////////////////////////////////////////////////////////
// MemoryFileSystem
///////////////////////////////////////////////////////////////////////////////


MemoryFileSystem* gpMemoryFileSystem = NULL;

MemoryFileSystem* GetDefaultFileSystem()
{
    return gpMemoryFileSystem;
}

void SetDefaultFileSystem(MemoryFileSystem* pMemoryFileSystem)
{
    gpMemoryFileSystem = pMemoryFileSystem;
}


MemoryFileSystem::MemoryFileSystem()
  : mRootNode()
  , mOpenFilePtrList()
  , mFileList()
  , mFutex()
  //mCurrentDirectory
  //mTempDirectory
  , mLastError(EA::IO::kStateSuccess)
  , mFilePathSeparator(kFilePathSeparatorDefault)
  , mFilePathCaseSensitive(false)
  , mRand((uint32_t)EA::StdC::GetTime())
{
    using namespace Internal;

    mRootNode.mNodeType     = ENTRY_TYPE_DIRECTORY;
    mRootNode.mCreationTime = GetFileTime();

    // Hard-coded to /temp/ for now.
    EA::StdC::Sprintf(mTempDirectory, "%ctemp%d", mFilePathSeparator, EAArrayCount(mTempDirectory), mFilePathSeparator);
}


MemoryFileSystem::~MemoryFileSystem()
{
    Shutdown(); // Just in case.
}


bool MemoryFileSystem::Init()
{
    if(!GetDefaultFileSystem())
        SetDefaultFileSystem(this);

    return true;
}


void MemoryFileSystem::Shutdown()
{
    using namespace Internal;

    EA::Thread::AutoFutex autoFutex(mFutex);

    // Assert that there are no open files.
    EA_ASSERT(mOpenFilePtrList.empty());

    // Close any open files.
    for(MFSMemoryFilePtrList::iterator it = mOpenFilePtrList.begin(); it != mOpenFilePtrList.end(); ++it)
    {
        MemoryFile* pMemoryFile = *it;
        pMemoryFile->Close();
    }

    mOpenFilePtrList.clear();
    mFileList.clear();
    EA_ASSERT(!MFSOpenFilesExistRecursive(&mRootNode));

    // Clear the file tree
    mRootNode.mNodeList.clear();
    mRootNode.mMemoryFilePtrList.clear(); // This should already be empty, as is asserted so above.

    // Leave other member variables as the already are, even if changed from the default.
    // We leave GetDefaultFileSystem as it is, even if it's us.
}


void MemoryFileSystem::Reset()
{
    // Currently calling Shutdown has the effect we need.
    Shutdown();
}


bool MemoryFileSystem::EntryExists(EntryType entryType, const char8_t* path)
{
    using namespace Internal;

    EA::Thread::AutoFutex autoFutex(mFutex);

    MFSNode* pNode = MFSNodeFromPath(&mRootNode, path, false);

    // Don't set mLastError if the return value is false, as it's not an execution error.
    return (pNode && (pNode->mNodeType == entryType));
}


// This function must be called with any trailing directory path / chars removed.
// Thus when calling this for the root node "/", the passed in path will be "" (empty).
bool MemoryFileSystem::EntryCreate(EntryType entryType, const char8_t* pPath)
{
    using namespace Internal;

    EA::Thread::AutoFutex autoFutex(mFutex);

    bool returnValue = false;
    const char8_t* pFilePathEnd = (pPath + EA::StdC::Strlen(pPath));

    if((pPath[0] == mFilePathSeparator) && 
       (pFilePathEnd > pPath) && 
       (pFilePathEnd[-1] != mFilePathSeparator)) // Make sure there is a non-empty name at the end of the pPath.
    {
        MFSNode* pNode = MFSCreateFSNodeFromPath(&mRootNode, pPath, &mLastError);

        if(pNode)
        {
            char8_t fileName[kMaxMFSPathLength];

            pNode->mName     = MFSFileNameFromPath(pPath, fileName);
            pNode->mNodeType = entryType;

            returnValue = true;
        }
        // MFSCreateFSNodeFromPath will set mLastError.
    }
    else if(pPath[0] == 0)
        returnValue = true; // The root node always already exists.    
    else
        mLastError = MFS_ERROR_BAD_PATHNAME;

    mLastError = (uint32_t)EA::IO::kStateError;
    return returnValue;
}


// Destroys file or directory entries. Directory destorys are recursive and remove all children.
// If requireEmpty is true then the operation fails for a directory entryType if the directory has
// any child entries (files or directories). Use requireEmpty = false if you want to recursively 
// delete the file/directory structure under the path.
// Currently we follow the Microsoft approach in that the operation fails if anybody has any child node
// open for access or iteration, recursively.
// Returns true if the entry was destroyed. Can fail to destroy the entry if it was open for access
// or if it wasn't empty but requireEmpty == true.
bool MemoryFileSystem::EntryDestroy(EntryType entryType, const char8_t* pPath, bool requireEmpty)
{
    using namespace Internal;

    EA::Thread::AutoFutex autoFutex(mFutex);

    bool     returnValue = false;
    MFSNode* pNode = MFSNodeFromPath(&mRootNode, pPath, false);

    if(pNode) 
    {
        if(pNode != &mRootNode) // If not trying to destroy the root node (which we disallow)...
        {
            if(pNode->mNodeType == entryType)
            {
                bool canRemove;

                // We currently follow the Microsoft-style file system rule that you can't delete in-use directories or files.
                // To consider: Make it so we can destroy files that are open, by simply unlinking
                // them from the node list. That would make this be more like Unix than like Windows.

                if(pNode->mNodeType == ENTRY_TYPE_DIRECTORY)
                {
                    if(requireEmpty)
                    {
                        canRemove = !pNode->mNodeList.empty();

                        if(!canRemove)
                            mLastError = MFS_ERROR_DIR_NOT_EMPTY;
                    }
                    else
                    {
                        canRemove = !MFSOpenFilesExistRecursive(pNode); // Check for any open files recursively. If there are none then we can delete this directory.

                        if(!canRemove)
                            mLastError = MFS_ERROR_ACCESS_DENIED;
                    }
                }
                else
                {
                    canRemove = pNode->mMemoryFilePtrList.empty();

                    if(!canRemove)
                        mLastError = MFS_ERROR_ACCESS_DENIED;
                }

                if(canRemove)
                {
                    returnValue = MFSRemoveNode(pNode);
                    EA_ASSERT(returnValue);
                }
                // mLastError would have been set above.
            }
            else
                mLastError = MFS_ERROR_FILE_NOT_FOUND;
        }
        else
            mLastError = MFS_ERROR_ACCESS_DENIED;
    }
    else
        mLastError = MFS_ERROR_FILE_NOT_FOUND;

    return returnValue;
}


bool MemoryFileSystem::EntryCopy(EntryType entryType, const char8_t* pathSource, const char8_t* pathDestination, bool bOverwrite)
{
    // See EntryMove for cases we need to handle, which are a similar set to here.

    using namespace Internal;

    EA::Thread::AutoFutex autoFutex(mFutex);

    bool     returnValue = false;
    MFSNode* pNodeSource = MFSNodeFromPath(&mRootNode, pathSource, false);

    if(pNodeSource && (pNodeSource->mNodeType == entryType)) // If the source exists...
    {
        MFSNode* pNodeDest = MFSCreateFSNodeFromPath(&mRootNode, pathDestination, NULL); // Fails if already exists.

        if(pNodeDest == pNodeSource) // Check for copying self to self.
            returnValue = true;
        else
        {
            // If pathDestination is a child of pathSource, then disallow, like Windows.
            const bool bDestIsChild = MFSPathIsChild(pathSource, pathDestination);

            if(!bDestIsChild)
            {
                // If pathDestination is a parent of pathSource...
                const bool bDestIsParent = MFSPathIsChild(pathDestination, pathSource);

                if(!bDestIsParent) // We currently don't allow copying to parent, but it's possible with some work.
                {
                    if(!pNodeDest && bOverwrite) // If the dest already existed (pNodeDest was assigned NULL in MFSCreateFSNodeFromPath above), but overwrite is enabled...
                    {
                        if(EntryDestroy(entryType, pathDestination, false))
                        {
                            pNodeDest = MFSCreateFSNodeFromPath(&mRootNode, pathDestination, NULL);
                            EA_ASSERT(pNodeDest); // This should always succeed.
                        }
                        // EntryDestroy will set mLastError if it fails.
                    }

                    if(pNodeDest)
                    {
                        const MFSName nameSaved(pNodeDest->mName);
                        EA_ASSERT(!nameSaved.empty() && MFSStrstr(pathDestination, nameSaved.c_str()));

                        if(entryType == ENTRY_TYPE_DIRECTORY)
                            MFSCopyDirectory(pNodeSource, pNodeDest, nameSaved.c_str()); // This should always succeed.
                        else
                            MFSCopyFile(pNodeSource, pNodeDest, nameSaved.c_str());      // This should always succeed.

                        returnValue = true;
                    }
                    else if(!bOverwrite)
                        mLastError = MFS_ERROR_ALREADY_EXISTS;
                }
                else
                    mLastError = MFS_ERROR_ACCESS_DENIED; // This is not the right error value.
            }
            else
                mLastError = MFS_ERROR_ACCESS_DENIED; // This is not the right error value.
        }
    }
    else
        mLastError = MFS_ERROR_PATH_NOT_FOUND;

    return returnValue;
}


bool MemoryFileSystem::EntryMove(EntryType entryType, const char8_t* pathSource, const char8_t* pathDestination, bool bOverwrite)
{
    // We have to handle the following cases: 
    //     Source path doesn't exist or is invalid.
    //     Dest path is invalid.
    //     Moving to self (no-op).
    //     Moving to a location that already exists (overwrite).
    //     Moving either a file or a directory, including directory tree.
    //     Open files may exist at either the source or dest, blocking the operation.
    //     Moving a directory to a location in its own hierarchy (e.g. /a/b move to /a/b/c/d/).
    //         - Moving parent to child is invalid, but moving child to parent is valid.

    using namespace Internal;

    EA::Thread::AutoFutex autoFutex(mFutex);

    bool     returnValue = false;
    MFSNode* pSourceNode = MFSNodeFromPath(&mRootNode, pathSource, false);

    if(pSourceNode && (pSourceNode->mNodeType == entryType)) // If the source exists...
    {
        bool canMoveSource;

        if(entryType == ENTRY_TYPE_DIRECTORY)
            canMoveSource = !MFSOpenFilesExistRecursive(pSourceNode);
        else
            canMoveSource =  MFSGetCurrentFileOpenAccess(pSourceNode) == 0;

        bool     canWriteDest = false;
        MFSNode* pNodeDest = MFSNodeFromPath(&mRootNode, pathDestination, false);

        if(!pNodeDest)
            canWriteDest = true;
        else if(bOverwrite)
        {
            if(pNodeDest == pSourceNode) // If moving from self to self... then there's nothing to do.
                returnValue = true;
            else
            {
                // If pathDestination is a child of pathSource, then disallow, like Windows.
                const bool bDestIsChild = MFSPathIsChild(pathSource, pathDestination);

                if(!bDestIsChild)
                {
                    // If pathDestination is a parent of pathSource, then allow, but be careful not to trash the hierarchy.
                    const bool bDestIsParent = MFSPathIsChild(pathDestination, pathSource);

                    // We currently don't allow moving to parent, but it's possible with some work. Primarily we 
                    // need to detach this node, delete the dest node, then reattach this node to the dest location.
                    if(!bDestIsParent)
                    {
                        if(EntryDestroy(entryType, pathDestination, !bOverwrite))
                        {
                            canWriteDest = true;
                            pNodeDest    = NULL;
                        }
                        // EntryDestroy will set mLastError if it fails.
                    }
                    else
                        mLastError = MFS_ERROR_ACCESS_DENIED; // This is not the right error value.
                }
                else
                    mLastError = MFS_ERROR_ACCESS_DENIED; // This is not the right error value.
            }
        }

        if(canMoveSource && canWriteDest && !returnValue) // If we can move the source and can write to the dest and if we didn't set returnValue to true above for no-op...
        {
            EA_ASSERT(pSourceNode && EntryExists(entryType, pathSource));
            EA_ASSERT(!pNodeDest && !EntryExists(entryType, pathDestination));

            // We need to remove pSourceNode from its parent and move it to the  
            // new location, even though that may be under the same parent.
            MFSNode* pDestNodeParent = MFSCreateParentNodeFromPath(&mRootNode, pathDestination, true, NULL);
            EA_ASSERT(pDestNodeParent != NULL); // This should always succeed.

            char8_t fileNameDest[kMaxMFSPathLength];
            pSourceNode->mName = MFSFileNameFromPath(pathDestination, fileNameDest);
            EA_ASSERT(!pSourceNode->mName.empty());

            returnValue = MFSMoveNode(pSourceNode, pDestNodeParent); // Splices the node out of one list and into another, with no copying, allocation, etc.
            EA_ASSERT(returnValue);
        }
    }
    else
        mLastError = MFS_ERROR_PATH_NOT_FOUND;

    return returnValue;
}


bool MemoryFileSystem::EntryRename(EntryType entryType, const char8_t* pathSource, const char8_t* pathDestination, bool bOverwrite)
{
    // Currently a rename is identical to a move, as we allow renames to execute across the tree and not just in the same subdir.
    // Also, we disallow a rename, even to the same subdirectory, when file(s) in the source path are open.
    return EntryMove(entryType, pathSource, pathDestination, bOverwrite);
}


void MemoryFileSystem::AddOpenFilePtr(MemoryFile* pMemoryFile)
{
    EA::Thread::AutoFutex autoFutex(mFutex);

    mOpenFilePtrList.push_back(pMemoryFile);
}


void MemoryFileSystem::RemoveOpenFilePtr(MemoryFile* pMemoryFile)
{
    using namespace Internal;

    EA::Thread::AutoFutex autoFutex(mFutex);

    // To do: Make this a hash_set or set, so this is much faster for large file counts.
    MFSMemoryFilePtrList::iterator it = eastl::find(mOpenFilePtrList.begin(), mOpenFilePtrList.end(), pMemoryFile);

    EA_ASSERT(it != mOpenFilePtrList.end());

    if(it != mOpenFilePtrList.end())
        mOpenFilePtrList.erase(it);
}


bool MemoryFileSystem::FileExists(const char8_t* filePath)
{
    return EntryExists(ENTRY_TYPE_FILE, filePath);
}


bool MemoryFileSystem::FileCreate(const char8_t* filePath)
{
    return EntryCreate(ENTRY_TYPE_FILE, filePath);
}


bool MemoryFileSystem::FileDestroy(const char8_t* filePath)
{
    return EntryDestroy(ENTRY_TYPE_FILE, filePath, false);
}


bool MemoryFileSystem::FileCopy(const char8_t* filePathSource, const char8_t* filePathDest)
{
    bool bOverwrite = true;
    return EntryCopy(ENTRY_TYPE_FILE, filePathSource, filePathDest, bOverwrite);
}


bool MemoryFileSystem::FileMove(const char8_t* filePathSource, const char8_t* filePathDest)
{
    bool bOverwrite = true;
    return EntryMove(ENTRY_TYPE_FILE, filePathSource, filePathDest, bOverwrite);
}


bool MemoryFileSystem::FileRename(const char8_t* filePathSource, const char8_t* filePathDest)
{
    bool bOverwrite = true;
    return EntryRename(ENTRY_TYPE_FILE, filePathSource, filePathDest, bOverwrite);
}


uint64_t MemoryFileSystem::FileGetSize(const char8_t* filePath)
{
    using namespace Internal;

    EA::Thread::AutoFutex autoFutex(mFutex);

    MFSNode* pNode = MFSNodeFromPath(&mRootNode, filePath, false);

    if(pNode && (pNode->mNodeType == ENTRY_TYPE_FILE)) 
        return pNode->mData.size();
    else
        mLastError = MFS_ERROR_FILE_NOT_FOUND; // It's possible that the path is malformed, but we don't return MFS_ERROR_BAD_PATHNAME.

    return UINT64_MAX;
}


int MemoryFileSystem::FileGetAttributes(const char8_t* filePath)
{
    using namespace Internal;

    EA::Thread::AutoFutex autoFutex(mFutex);

    MFSNode* pNode = MFSNodeFromPath(&mRootNode, filePath, false);

    if(pNode && (pNode->mNodeType == ENTRY_TYPE_FILE))         
        return (FILE_ATTR_READABLE | FILE_ATTR_WRITABLE); // To consider: Implement attribute functionality.
    else
        mLastError = MFS_ERROR_FILE_NOT_FOUND; // It's possible that the path is malformed, but we don't return MFS_ERROR_BAD_PATHNAME.

    return 0;
}


bool MemoryFileSystem::FileSetAttributes(const char8_t* filePath, int /*attributes*/, bool /*enable*/)
{
    using namespace Internal;

    EA::Thread::AutoFutex autoFutex(mFutex);

    MFSNode* pNode = MFSNodeFromPath(&mRootNode, filePath, false);

    if(pNode && (pNode->mNodeType == ENTRY_TYPE_FILE)) 
        return true; // To consider: Implement attribute functionality.
    else
        mLastError = MFS_ERROR_FILE_NOT_FOUND; // It's possible that the path is malformed, but we don't return MFS_ERROR_BAD_PATHNAME.

    return 0;
}


uint64_t MemoryFileSystem::FileGetTime(const char8_t* filePath)
{
    using namespace Internal;

    EA::Thread::AutoFutex autoFutex(mFutex);

    MFSNode* pNode = MFSNodeFromPath(&mRootNode, filePath, false);

    if(pNode && (pNode->mNodeType == ENTRY_TYPE_FILE))
        return pNode->mCreationTime;
    else
        mLastError = MFS_ERROR_FILE_NOT_FOUND; // It's possible that the path is malformed, but we don't return MFS_ERROR_BAD_PATHNAME.

    return 0;
}


bool MemoryFileSystem::FileSetTime(const char8_t* filePath, uint64_t time)
{
    using namespace Internal;

    EA::Thread::AutoFutex autoFutex(mFutex);

    MFSNode* pNode = MFSNodeFromPath(&mRootNode, filePath, false);

    if(pNode && (pNode->mNodeType == ENTRY_TYPE_FILE)) 
        pNode->mCreationTime = time;
    else
        mLastError = MFS_ERROR_FILE_NOT_FOUND; // It's possible that the path is malformed, but we don't return MFS_ERROR_BAD_PATHNAME.

    return false;
}


uint64_t MemoryFileSystem::FileGetLocation(const char8_t* filePath)
{
    using namespace Internal;

    EA::Thread::AutoFutex autoFutex(mFutex);

    MFSNode* pNode = MFSNodeFromPath(&mRootNode, filePath, false);

    if(pNode && (pNode->mNodeType == ENTRY_TYPE_FILE)) 
        return (uintptr_t)pNode;
    else
        mLastError = MFS_ERROR_FILE_NOT_FOUND; // It's possible that the path is malformed, but we don't return MFS_ERROR_BAD_PATHNAME.

    return 0;
}


bool MemoryFileSystem::FileMakeTemp(char8_t* filePath, size_t filePathCapacity, const char8_t* pDirectory, 
                                      const char8_t* pFileName, const char8_t* pExtension)
{
    using namespace Internal;

    // No mutex lock needed, as this is a high level function.

    static const char8_t pFileNameDefault[]  = { 't', 'e', 'm', 'p', 0 };
    static const char8_t pExtensionDefault[] = { '.', 't', 'm', 'p', 0 };

    const uint32_t nTime = (uint32_t)EA::StdC::GetTime();
    uint32_t       nRand = mRand.RandomUint32Uniform();
    char8_t        tempPath[kMaxMFSPathLength];

    if(!pFileName)
        pFileName = pFileNameDefault;

    if(!pExtension)
        pExtension = pExtensionDefault;

    if(!pDirectory)
    {
        DirectoryGetTemp(tempPath, EAArrayCount(tempPath));
        pDirectory = tempPath;
    }

    for(size_t i = 0; i < 100000; i++, nRand = mRand.RandomUint32Uniform())
    {
        char8_t        buffer[20];
        MFSPathString8 tempFilePath(pDirectory);

        if(tempFilePath.empty() || (tempFilePath.back() != mFilePathSeparator))
            tempFilePath += mFilePathSeparator;
        tempFilePath += pFileName;
        tempFilePath += EA::StdC::U32toa(nTime ^ nRand, buffer, 10);
        tempFilePath += pExtension;

        eastl_size_t sourcePathLength = tempFilePath.length();
        if(sourcePathLength > filePathCapacity)
        {
            mLastError = MFS_ERROR_NOT_ENOUGH_MEMORY;
            break; // Not enough space. Don't even try to create it.
        }

        EA::StdC::Strlcpy(filePath, tempFilePath.c_str(), filePathCapacity);

        IStream* pStream = OpenFile(filePath, OPEN_READ_WRITE, DISPOSITION_CREATE_NEW, 0);

        if(pStream)
        {
            static_cast<MemoryFile*>(pStream)->Close();
            return true;
        }
    }

    mLastError = MFS_ERROR_ALREADY_EXISTS;
    return false;
}


// Our node manipulation functions treat all nodes the same, as if they were all files.
// But we accept naming directories with trailing '/' chars, as that makes for much easier
// user string manipulation. The following function provides a means to strip a trailing
// '/' char from a directory if it's present, but leave it alone if not.
bool MemoryFileSystem::PossiblyStripDirSeparator(const char8_t*& pDirPath, char8_t* dirTemp)
{
    size_t length = EA::StdC::Strlen(pDirPath);

    if(length && (pDirPath[length - 1] == mFilePathSeparator)) // If the dir path ends with a path separator...
    {
        if(EA::StdC::Strlcpy(dirTemp, pDirPath, kMaxMFSPathLength) >= kMaxMFSPathLength) // If the input path is too long for our buffer...
            return false;
        dirTemp[length - 1] = 0; // Remove the trailing '/'
        pDirPath = dirTemp;      // Rename pDirPath to be dirTemp.
    }

    return true;
}


bool MemoryFileSystem::DirectoryExists(const char8_t* directoryPath)
{
    char8_t dirTemp[kMaxMFSPathLength];

    if(!PossiblyStripDirSeparator(directoryPath, dirTemp))
        return false;

    return EntryExists(ENTRY_TYPE_DIRECTORY, directoryPath);
}


bool MemoryFileSystem::DirectoryCreate(const char8_t* directoryPath)
{
    char8_t dirTemp[kMaxMFSPathLength];

    if(!PossiblyStripDirSeparator(directoryPath, dirTemp))
        return false;

    return EntryCreate(ENTRY_TYPE_DIRECTORY, directoryPath);
}


bool MemoryFileSystem::DirectoryDestroy(const char8_t* directoryPath)
{
    char8_t dirTemp[kMaxMFSPathLength];

    if(!PossiblyStripDirSeparator(directoryPath, dirTemp))
        return false;

    return EntryDestroy(ENTRY_TYPE_DIRECTORY, directoryPath, false);
}


bool MemoryFileSystem::DirectoryCopy(const char8_t* directoryPathSource, const char8_t* directoryPathDest)
{
    char8_t dirTempSource[kMaxMFSPathLength];
    char8_t dirTempDest[kMaxMFSPathLength];

    if(!PossiblyStripDirSeparator(directoryPathSource, dirTempSource))
        return false;
    if(!PossiblyStripDirSeparator(directoryPathDest, dirTempDest))
        return false;

    bool bOverwrite = true;
    return EntryCopy(ENTRY_TYPE_DIRECTORY, directoryPathSource, directoryPathDest, bOverwrite);
}


bool MemoryFileSystem::DirectoryMove(const char8_t* directoryPathSource, const char8_t* directoryPathDest)
{
    char8_t dirTempSource[kMaxMFSPathLength];
    char8_t dirTempDest[kMaxMFSPathLength];

    if(!PossiblyStripDirSeparator(directoryPathSource, dirTempSource))
        return false;
    if(!PossiblyStripDirSeparator(directoryPathDest, dirTempDest))
        return false;

    bool bOverwrite = true;
    return EntryMove(ENTRY_TYPE_DIRECTORY, directoryPathSource, directoryPathDest, bOverwrite);
}


bool MemoryFileSystem::DirectoryRename(const char8_t* directoryPathSource, const char8_t* directoryPathDest)
{
    char8_t dirTempSource[kMaxMFSPathLength];
    char8_t dirTempDest[kMaxMFSPathLength];

    if(!PossiblyStripDirSeparator(directoryPathSource, dirTempSource))
        return false;
    if(!PossiblyStripDirSeparator(directoryPathDest, dirTempDest))
        return false;

    bool bOverwrite = true;
    return EntryRename(ENTRY_TYPE_DIRECTORY, directoryPathSource, directoryPathDest, bOverwrite);
}


int MemoryFileSystem::DirectoryGetCurrent(char8_t* directoryPath, size_t directoryPathCapacity)
{
    size_t result = EA::StdC::Strlcpy(directoryPath, mCurrentDirectory, directoryPathCapacity);

    if(result < directoryPathCapacity)
        return (int)result;

    return 0;
}


bool MemoryFileSystem::DirectorySetCurrent(const char8_t* directoryPath)
{
    if((EA::StdC::Strlen(directoryPath) + 1) < EAArrayCount(mCurrentDirectory))
    {
        if(MFSValidatePath(directoryPath, ENTRY_TYPE_DIRECTORY, NULL))
        {
            EA::StdC::Strcpy(mCurrentDirectory, directoryPath);
            return true;
        }
        else
            mLastError = MFS_ERROR_BAD_PATHNAME;
    }
    else
        mLastError = MFS_ERROR_NOT_ENOUGH_MEMORY;

    return false;
}


int MemoryFileSystem::DirectoryGetTemp(char8_t* directoryPath, size_t directoryPathCapacity)
{
    size_t result = EA::StdC::Strlcpy(directoryPath, mTempDirectory, directoryPathCapacity);

    if(result < directoryPathCapacity)
        return (int)result;

    return 0;
}


IStream* MemoryFileSystem::OpenFile(const char8_t* filePath, int openFlags, int disposition, int sharing)
{
    EA::Thread::AutoFutex autoFutex(mFutex);

    IStream*    pReturnValue = NULL;
    MemoryFile& memoryFile   = mFileList.push_back();

    memoryFile.SetPath(filePath);
    memoryFile.AddRef(); // AddRef so it can never be deleted. We don't match this with a Release because we are storing instances and not AddRef'd pointers.

    if(memoryFile.Open(openFlags, disposition, sharing, 0))
        pReturnValue = &memoryFile;
    else
    {
        mLastError = (uint32_t)memoryFile.GetState();
        mFileList.pop_back();
    }

    return pReturnValue;
}


void MemoryFileSystem::DestroyFile(IStream* pStream)
{
    using namespace Internal;

    EA::Thread::AutoFutex autoFutex(mFutex);

    // This is a bit slow if there are many files open. We can improve this drastically
    // by switching this list to a hash_set or set.
    for(MFSMemoryFileList::iterator it = mFileList.begin(); it != mFileList.end(); ++it)
    {
        MemoryFile& memoryFile = *it;

        if(pStream == &memoryFile)
        {
            EA_ASSERT(pStream->GetAccessFlags() == 0);  // Verify the file is closed. 
            mFileList.erase(it);                        // ~MemoryFile will be called here, and it will clean things up if not done already.
            break;
        }
    }
}


uint32_t MemoryFileSystem::GetLastError()
{
    return mLastError;
}


void MemoryFileSystem::SetLastError(uint32_t error)
{
    mLastError = error;
}


char8_t* MemoryFileSystem::FormatError(uint32_t errorId, char8_t* buffer, size_t bufferCapacity)
{
    const char* pErrorString = NULL;

    switch ((int32_t)errorId)
    {
        case MFS_ERROR_SUCCESS:               pErrorString = "The operation completed successfully."; break;
        case MFS_ERROR_FILE_NOT_FOUND:        pErrorString = "The system cannot find the file specified."; break;
        case MFS_ERROR_PATH_NOT_FOUND:        pErrorString = "The system cannot find the path specified."; break;
        case MFS_ERROR_TOO_MANY_OPEN_FILES:   pErrorString = "The system cannot open the file."; break;
        case MFS_ERROR_ACCESS_DENIED:         pErrorString = "Access is denied."; break;
        case MFS_ERROR_NOT_ENOUGH_MEMORY:     pErrorString = "Not enough storage is available to process this command."; break;
        case MFS_ERROR_OUTOFMEMORY:           pErrorString = "Not enough storage is available to complete this operation."; break;
        case MFS_ERROR_CURRENT_DIRECTORY:     pErrorString = "The directory cannot be removed."; break;
        case MFS_ERROR_NOT_SAME_DEVICE:       pErrorString = "The system cannot move the file to a different disk drive."; break;
        case MFS_ERROR_WRITE_PROTECT:         pErrorString = "The media is write protected."; break;
        case MFS_ERROR_SHARING_VIOLATION:     pErrorString = "The process cannot access the file because it is being used by another entity."; break;
        case MFS_ERROR_LOCK_VIOLATION:        pErrorString = "The process cannot access the file because another entity has locked a portion of the file."; break;
      //case MFS_ERROR_FILE_EXISTS:           pErrorString = "The file exists."; break;
        case MFS_ERROR_CANNOT_MAKE:           pErrorString = "The directory or file cannot be created."; break;
        case MFS_ERROR_INVALID_PARAMETER:     pErrorString = "The parameter is incorrect."; break;
        case MFS_ERROR_OPEN_FAILED:           pErrorString = "The system cannot open the device or file specified."; break;
        case MFS_ERROR_INVALID_NAME:          pErrorString = "The filename, directory name, or volume label syntax is incorrect."; break;
        case MFS_ERROR_NEGATIVE_SEEK:         pErrorString = "An attempt was made to move the file pointer before the beginning of the file."; break;
        case MFS_ERROR_DIR_NOT_ROOT:          pErrorString = "The directory is not a subdirectory of the root directory."; break;
        case MFS_ERROR_DIR_NOT_EMPTY:         pErrorString = "The directory is not empty."; break;
        case MFS_ERROR_BAD_PATHNAME:          pErrorString = "The specified path is invalid."; break;
        case MFS_ERROR_ALREADY_EXISTS:        pErrorString = "Cannot create a file when that file already exists."; break;
        case MFS_ERROR_DIRECTORY:             pErrorString = "The directory name is invalid."; break;
        case MFS_ERROR_GENERIC:               pErrorString = "Generic error."; break;
        case MFS_ERROR_NOT_OPEN:              pErrorString = "File not open."; break;
        default:                              pErrorString = "Unknown."; EA_FAIL_FORMATTED(("MemoryFileSystem::FormatError: Unknown 0x%08x", errorId)); break;
    }

    EA::StdC::Snprintf(buffer, bufferCapacity, "(0x%08x): %s", errorId, pErrorString);

    return buffer;
}

Internal::MFSNode* MemoryFileSystem::GetRootNode()
{
    return &mRootNode;
}


void MemoryFileSystem::Lock()
{
    mFutex.Lock();
}


void MemoryFileSystem::Unlock()
{
    mFutex.Unlock();
}


bool MemoryFileSystem::ValidateNode(String8& sOutput, const Internal::MFSNode* pNode, 
                                    Internal::MFSNameSet& filePathSet, Internal::MFSMemoryFilePtrSet& filePtrSet) const
{
    using namespace Internal;

    bool bValid = true;

    // Validate this node's properties.
    char8_t filePath[kMaxMFSPathLength];
    MFSPathFromFSNode(pNode, filePath);

    if(pNode->mName.find((char8_t)'\0') != MFSName::npos) // File names are not allows to have 0 chars in them.
    {
        bValid = false;
        sOutput.append_sprintf("pNode->mName (%s) has nul char.\n", filePath);
    }

    if(pNode->mCreationTime == UINT64_MAX)               // Time cnanot be the 'invalid' time.
    {
        bValid = false;
        sOutput.append_sprintf("pNode->mCreationTime (%I64u) is invalid.\n", pNode->mCreationTime);
    }

    if(((int)pNode->mNodeType != ENTRY_TYPE_FILE) && 
       ((int)pNode->mNodeType != ENTRY_TYPE_DIRECTORY))  // mNodeType should be a valid enum.
    {
        bValid = false;
        sOutput.append_sprintf("Node (%s) is of an invalid type (%d)\n", filePath, (int)pNode->mNodeType);
    }

    if(pNode->mNodeType == ENTRY_TYPE_FILE)
    {
        if(!pNode->mNodeList.empty())                   // File nodes should not have children
        {
            bValid = false;
            sOutput.append_sprintf("File node (%s) has children.\n", filePath);
        }
    }
    else
    {
        if(!pNode->mMemoryFilePtrList.empty())           // Directory nodes should have be open as file streams (at least currently, this may change).
        {
            bValid = false;
            sOutput.append_sprintf("Directory node (%s) has open files.\n", filePath);
        }
    }

    // Validate node name and path uniqueness.
    MFSName sFilePath = filePath;
    sFilePath.make_lower();

    if(filePathSet.find(sFilePath) != filePathSet.end())   // No file stream should ever be attached to more than one node.
    {
        bValid = false;
        sOutput.append_sprintf("Duplicated file path (%s)\n", filePath);
    }
    else
        filePathSet.insert(sFilePath);
    
    // Validate the set of open files.
    for(MFSMemoryFilePtrList::const_iterator itF = pNode->mMemoryFilePtrList.begin(); itF != pNode->mMemoryFilePtrList.end(); ++itF)
    {
        MemoryFile* pMemoryFile = const_cast<MemoryFile*>(*itF);

        if(filePtrSet.find(pMemoryFile) != filePtrSet.end())  // No file stream should ever be attached to more than one node.
        {
            bValid = false;
            sOutput.append_sprintf("Duplicated memory files path (%s)\n", filePath);
        }
        else
            filePtrSet.insert(pMemoryFile);
    }

    // Validate this node's children.
    for(MFSNodeList::const_iterator itN = pNode->mNodeList.begin(); itN != pNode->mNodeList.end(); ++itN)
    {
        const MFSNode& nodeChild = *itN;

        if(nodeChild.mpNodeParent != pNode) // All pNode's children should have pNode as a parent.
        {
            bValid = false;
            sOutput.append_sprintf("File (%s) has inconsistent parent node pointer.\n", filePath);
        }

        if(!ValidateNode(sOutput, &nodeChild, filePathSet, filePtrSet))
        {
            bValid = false;
            // Nothing to print, as the ValidateNode call will do any printing itself.
        }
    }

    return bValid;
}


bool MemoryFileSystem::ValidatePath(const char8_t* pPath, EntryType entryType, size_t* pErrorPos) const
{
    return MFSValidatePath(pPath, entryType, pErrorPos);
}


bool MemoryFileSystem::Validate(String8& sOutput) const
{
    using namespace Internal;

    EA::Thread::AutoFutex autoFutex(const_cast<EA::Thread::Futex&>(mFutex));

    MFSNameSet          filePathSet;        // No path should ever be represented more than once.
    MFSMemoryFilePtrSet openFilePtrSet;     // No file stream should ever be attached to more than one node.

    sOutput.clear();

    // Check our file tree, based at mRootNode.
    bool returnValue = ValidateNode(sOutput, &mRootNode, filePathSet, openFilePtrSet);

    // Verify that all members of mFileList are listed in mOpenFilePtrList.
    // The inverse isn't necessarily true, as there could be independent 
    // MemoryFiles that aren't in mFileList.
    for(MFSMemoryFileList::const_iterator it = mFileList.begin(); it != mFileList.end(); ++it)
    {
        const MemoryFile& memoryFile = *it;

        MFSMemoryFilePtrList::const_iterator itPtr = eastl::find(mOpenFilePtrList.begin(), mOpenFilePtrList.end(), &memoryFile);

        if(itPtr == mOpenFilePtrList.end())
        {
            returnValue = false;
            sOutput.append_sprintf("Inconsistent mFileList for (%s).\n", memoryFile.mFilePath);
        }
    }

    return returnValue;
}


static void PrintTreeRecurse(const Internal::MFSNode* pNode, int depth, String8& sOutput)
{
    using namespace Internal;

    String8 sTemp;

    for(MFSNodeList::const_iterator it = pNode->mNodeList.begin(); it != pNode->mNodeList.end(); ++it)
    {
        const MFSNode& nodeChild = *it;

        // Print leading space.
        for(int i = 0; i < depth; i++)
            sOutput.append_sprintf("  ");

        // Handle the case of mName having control charaters (e.g. \n). We might want to deal with UTF8 chars too.
        sTemp.clear();

        for(eastl_size_t i = 0, iEnd = nodeChild.mName.size(); i < iEnd; i++)
        {
            const char8_t c = nodeChild.mName[i];

            if(nodeChild.mName[i] < ' ')
                sTemp.append_sprintf("\\%2x", (uint8_t)c);
            else
                sTemp += c;
        }

        sOutput.append_sprintf("%s%s\n", sTemp.c_str(), nodeChild.mNodeType == ENTRY_TYPE_DIRECTORY ? "/" : "");

        if(nodeChild.mNodeType == ENTRY_TYPE_DIRECTORY)
            PrintTreeRecurse(&nodeChild, depth + 1, sOutput);
    }
}


void MemoryFileSystem::PrintTree(String8& sOutput) const
{
    sOutput = "/\n";
    PrintTreeRecurse(&mRootNode, 1, sOutput);
}


char8_t MemoryFileSystem::GetFilePathSeparator() const
{
    return mFilePathSeparator;
}


void MemoryFileSystem::SetFilePathSeparator(char8_t filePathSeparator)
{
    EA_ASSERT(mRootNode.mNodeList.empty()); // Assert that the file system is in an unused state.
    mFilePathSeparator = filePathSeparator;
}


bool MemoryFileSystem::GetFilePathCaseSensitive() const
{
    return mFilePathCaseSensitive;
}


void MemoryFileSystem::SetFilePathSeparator(bool filePathCaseSensitive)
{
    EA_ASSERT(mRootNode.mNodeList.empty()); // Assert that the file system is in an unused state.
    mFilePathCaseSensitive = filePathCaseSensitive;
}


uint64_t MemoryFileSystem::GetFileTime()
{
    timeval tv;
    EA::StdC::GetTimeOfDay(&tv, NULL, true);

    return (uint64_t)tv.tv_sec;
}


// Returns pFileName
// To consider: Have a string class version of this function.
// Assumes that pPath is a non-empty valid file (not directory) path.
char8_t* MemoryFileSystem::MFSFileNameFromPath(const char8_t* pPath, char8_t* pFileName)
{
    if(pPath[0] == mFilePathSeparator) // If the path is valid...
    {
        const char8_t* pPathEnd = pPath + EA::StdC::Strlen(pPath);

        while(pPathEnd[-1] != mFilePathSeparator)
            --pPathEnd;

        EA::StdC::Strlcpy(pFileName, pPathEnd, kMaxMFSPathLength); // This chops the name if too long.
    }
    else
        pFileName[0] = 0;

    return pFileName;
}


bool MemoryFileSystem::MFSPathEndsWithSeparator(const char8_t* pPath) const
{
    const size_t length = EA::StdC::Strlen(pPath);

    return (length && (pPath[length - 1] == mFilePathSeparator));
}


// Assumes that the rights to remove the node have been acquired (e.g. no open files).
// Returns false if the node isn't in its parent. Such a false return should never occur with a valid file structure.
// Returns true if it as no parent.
bool MemoryFileSystem::MFSRemoveNode(Internal::MFSNode* pNode)
{
    using namespace Internal;

    EA_ASSERT(pNode && pNode->mMemoryFilePtrList.empty());

    bool returnValue = false;

    if(pNode->mpNodeParent) // If this node isn't the root node...  
    {
        for(MFSNodeList::iterator it = pNode->mpNodeParent->mNodeList.begin(), 
            itEnd = pNode->mpNodeParent->mNodeList.end(); it != itEnd; ++it)
        {
            MFSNode& node = *it;

            if(pNode == &node)
            {
                pNode->mpNodeParent->mNodeList.erase(it);
                returnValue = true;
                break;
            }
        }
    }
    else
        returnValue = true;

    return returnValue;
}


// It's OK if pDestNodeParent is NULL, which results in pSourceNode becoming detached.
// Returns false if pSourceNode isn't in its parent. Such a false return should never occur with a valid file structure.
// Not yet supported: It's OK if pSourceNode doesn't have a parent initially.
bool MemoryFileSystem::MFSMoveNode(Internal::MFSNode* pSourceNode, Internal::MFSNode* pDestNodeParent)
{
    using namespace Internal;

    EA_ASSERT(pSourceNode && pSourceNode->mMemoryFilePtrList.empty());

    bool returnValue = false;

    if(pDestNodeParent == pSourceNode)
        returnValue = true;
    else
    {
        // Find the list iterator for pSourceNode in its parent list.
        if(pSourceNode->mpNodeParent) // If pSourceNode has a parent
        {
            for(MFSNodeList::iterator it = pSourceNode->mpNodeParent->mNodeList.begin(), 
                itEnd = pSourceNode->mpNodeParent->mNodeList.end(); it != itEnd; ++it)
            {
                MFSNode& node = *it;

                if(pSourceNode == &node) // If we have found the iterator for pSourceNode...
                {
                    if(pDestNodeParent)
                        pDestNodeParent->mNodeList.splice(pDestNodeParent->mNodeList.end(), pDestNodeParent->mNodeList, it);
                    else
                        pSourceNode->mpNodeParent->mNodeList.erase(it);
                    
                    returnValue = true;
                    break;
                }
            }

            EA_ASSERT(returnValue); // If this fails then pSourceNode had a parent, but pSourceNode wasn't in the parent's mNodeList, which should be impossible.
        }
        else // Else pSourceNode is detached.
        {
            // We don't yet support doing this.
            //if(pDestNodeParent)
            //{
            //    EA_ASSERT(eastl::find(pDestNodeParent->mNodeList.begin(), pDestNodeParent->mNodeList.end(), pSourceNode) == pDestNodeParent->mNodeList.end());
            //    pDestNodeParent->mNodeList.push_back(pSourceNode); Can't do this; need to splice from some list.
            //}
            //
            //returnValue = true;
        }
            
        pSourceNode->mpNodeParent = pDestNodeParent;
    }

    return returnValue;
}


bool MemoryFileSystem::MFSOpenFilesExistRecursive(const Internal::MFSNode* pNode)
{
    using namespace Internal;

    bool bExist = false;

    if(!pNode->mMemoryFilePtrList.empty()) // If we are open...
        bExist = true;
    else
    {
        // This is a loop that calls ourself recursively. However, upon finding the first 
        // open file this loop and recursion will immediately end and return.
        for(MFSNodeList::const_iterator it = pNode->mNodeList.begin(); !bExist && (it != pNode->mNodeList.end()); ++it)
        {
            const MFSNode& node = *it;

            if(MFSOpenFilesExistRecursive(&node)) // Call ourselves recursively.
                bExist = true;
        }
    }

    return bExist;
}

int MemoryFileSystem::MFSGetCurrentFileOpenAccess(Internal::MFSNode* pNode)
{
    using namespace Internal;

    int accessFlags = OPEN_NONE;

    if(pNode) // By design, pNode may be NULL.
    {
        for(MFSMemoryFilePtrList::iterator it = pNode->mMemoryFilePtrList.begin(); it != pNode->mMemoryFilePtrList.end(); ++it)
        {
            MemoryFile* pMemoryFile = *it;
            accessFlags |= pMemoryFile->GetAccessFlags();
        }
    }

    return accessFlags;
}


// Assumes that pNodeDest can be safely overwritten (e.g. no files are open).
void MemoryFileSystem::MFSCopyDirectory(Internal::MFSNode* pNodeSource, Internal::MFSNode* pNodeDest, const char8_t* dirNameDest)
{
    using namespace Internal;

    EA_ASSERT(!MFSOpenFilesExistRecursive(pNodeDest));

    gParentNodeStack.push_back(pNodeDest->mpNodeParent); // We do this because the = assignment below uses it.
    *pNodeDest = *pNodeSource;
    if(dirNameDest)
        pNodeDest->mName = dirNameDest;
    gParentNodeStack.pop_back();

    // To consider: have a failure for the case of running out of memory.
}


// Assumes that pNodeDest can be safely overwritten (e.g. no files are open).
void MemoryFileSystem::MFSCopyFile(Internal::MFSNode* pNodeSource, Internal::MFSNode* pNodeDest, const char8_t* fileNameDest)
{
    using namespace Internal;

    EA_ASSERT(MFSGetCurrentFileOpenAccess(pNodeDest) == 0);

    if(fileNameDest)
        pNodeDest->mName = fileNameDest;
    pNodeDest->mNodeType = pNodeSource->mNodeType;
    pNodeDest->mData     = pNodeSource->mData; // Note that if the source file is open, this copies whatever its last written contents were.
}


char8_t* MemoryFileSystem::MFSStrstr(const char8_t* pString, const char8_t* pSubString) const
{
    if(mFilePathCaseSensitive)
        return EA::StdC::Strstr(pString, pSubString);
    else
        return EA::StdC::Stristr(pString, pSubString);
}


// Returns true if pPathPossibleChild is a child path of pPathParent or equal to pPathParent.
// Require caller to strip any trailing / 
bool MemoryFileSystem::MFSPathIsChild(const char8_t* pPathParent, const char8_t* pPathPossibleChild) const
{
    EA_ASSERT(!MFSPathEndsWithSeparator(pPathParent));          // Require caller to strip any trailing / because otherwise 
    EA_ASSERT(!MFSPathEndsWithSeparator(pPathPossibleChild));   // comparing /a/b/ to /a/b is problematic.

    return (MFSStrstr(pPathPossibleChild, pPathParent) != NULL);
}

// EntryType can be ENTRY_TYPE_NONE, ENTRY_TYPE_FILE, ENTRY_TYPE_DIRECTORY.
// ENTRY_TYPE_NONE means to accept either a file or directory path.
// We don't validate that strlen(pPath) < kMaxMFSPathLength.
bool MemoryFileSystem::MFSValidatePath(const char8_t* pPath, EntryType entryType, size_t* pErrorPos) const
{
    int            nameCount   = 0;
    const char8_t* pCurrent    = pPath;
    const char8_t* pPrev       = pPath;
    bool           returnValue = true;

    while(*pCurrent)
    {
        while(*pCurrent && (*pCurrent != mFilePathSeparator))  // Walk to the next / char.
            pCurrent++;
        
        if(*pCurrent)   // If / was found...
        {
            if(pCurrent == pPrev)   // If the name from the previous / to this one is empty...
            {
                if(nameCount != 0)  // If we have a non-initial directory which has an empty directory path name (e.g. /abc//ghi)
                {
                    returnValue = false;
                    break;
                }
            }
            else                    // If the name from the previous / (or beginning of path) to this one is non-empty...
            {
                if(nameCount == 0)  // If we have a initial directory which has a non-empty directory path name (e.g. abc/def)...
                {
                    returnValue = false;
                    break;
                }
            }

            pPrev = ++pCurrent;     // Move pCurrent to one-past the / char.
            nameCount++;
        }
    }

    if(returnValue)
    {
        if(pCurrent == pPath)    // If the path was empty...
            returnValue = false;
        if(pCurrent == pPrev)    // If the path ended with a / char...
        {
            if(entryType == ENTRY_TYPE_FILE)
                returnValue = false;
        }
        else                     // If the path ended with a non-/ char...
        {
            if(entryType == ENTRY_TYPE_DIRECTORY)
                returnValue = false;
        }
    }

    if(pErrorPos)
        *pErrorPos = (size_t)(pCurrent - pPath);

    return returnValue;
}


// For the path /abc/defg this will return three strings when called three times: an empty string for
// the root node, an "abc" string for the root's child, and "defg" for the next child, then NULL.
// Returns NULL when there are no more names left in the path.
const char8_t* MemoryFileSystem::MFSGetNextNodeNameFromPath(const char8_t* pPath, char8_t* pNodeName) const
{
    if(pPath[0])
    {
        const char8_t* p;
        const char8_t* pDestEnd = pNodeName + kMaxMFSPathLength - 1; // -1 because we need a space for the trailing 0 char.

        for(p = pPath; (*p != '\0') && (*p != mFilePathSeparator); ++p)
        {
            if(pNodeName < pDestEnd)
                *pNodeName++ = *p;
        }

        *pNodeName = 0;

        return *p ? ++p : p;
    }

    return NULL;
}


// Returns the MFSNode that corresponds to pPath.
// If bReturnParent then return the parent node of pPath if it exists (even if the leaf of pPath doesn't exist).
// Returns NULL if not found or if parent path of root node is requested.
// Returns NULL if pPath is malformed.
Internal::MFSNode* MemoryFileSystem::MFSNodeFromPath(Internal::MFSNode* pRootNode, const char8_t* pPath, bool bReturnParent)
{
    using namespace Internal;

    if(pPath[0] == mFilePathSeparator) // Make sure pPath starts at the root, which refers to pRootNode.
    {
        char8_t pNodeName[kMaxMFSPathLength];
        int     depth = 0;

        while((pPath = MFSGetNextNodeNameFromPath(pPath, pNodeName)) != NULL)
        {
            if(depth > 0) // If not the un-named root node...
            {
                // Make sure the name isn't empty, as might happen with a path like /abc//def.
                // However, the root node itself is unnamed. It is conceptually the text that 
                // is before the first / in a path like /abc/def.
                if(!pNodeName[0])
                    return NULL;    // Invalid path.

                MFSNodeList::iterator it;

                for(it = pRootNode->mNodeList.begin(); it != pRootNode->mNodeList.end(); ++it)
                {
                    MFSNode& node = *it;
                    const bool bNameMatch = (mFilePathCaseSensitive ? (node.mName.comparei(pNodeName) == 0) : (node.mName == pNodeName));

                    if(bNameMatch)
                    {
                        pRootNode = &node;
                        break; // Entry was found.
                    }
                }

                if(it == pRootNode->mNodeList.end()) // If we didn't find the current node...
                {
                    if(bReturnParent)   // If we are set to return the parent node of pPath instead of the leaf node.
                    {
                        if(!pPath[0])   // If the current node we were looking for was the leaf (last) node...
                            return pRootNode;
                    }

                    return NULL; // Entry not found.
                }
            }

            depth++;
        }
        
        return pRootNode; // Reached the end of the path, and this is the last node.
    }
    else if(pPath[1] == 0) // If the path is just "/"... return the root node.
    {
        if(!bReturnParent)      // Can't return parent of root node.
            return pRootNode;
    }

    return NULL; // Path doesn't begin with '/'
}


// Returns the MFSNode that corresponds to pPath.
// Returns NULL if pPath already exists.
// Returns NULL if pPath is malformed.
Internal::MFSNode* MemoryFileSystem::MFSCreateFSNodeFromPath(Internal::MFSNode* pRootNode, const char8_t* pPath, uint32_t* pError)
{
    using namespace Internal;

    if(pPath[0] == mFilePathSeparator) // Make sure pPath starts at the root, which refers to pRootNode.
    {
        char8_t  pNodeName[kMaxMFSPathLength];
        int      depth       = 0;
        MFSNode* pNode       = pRootNode;
        MFSNode* pNodeParent = NULL;

        while((pPath = MFSGetNextNodeNameFromPath(pPath, pNodeName)) != NULL)
        {
            if(depth > 0) // If not the un-named root node...
            {
                // Make sure the name isn't empty, as might happen with a path like /abc//def.
                // However, the root node itself is unnamed. It is conceptually the text that 
                // is before the first / in a path like /abc/def.
                if(!pNodeName[0])
                {
                    if(pError)  
                        *pError = MFS_ERROR_BAD_PATHNAME;
                    return NULL;
                }

                pNodeParent = pNode;

                MFSNodeList::iterator it;

                for(it = pNode->mNodeList.begin(); it != pNode->mNodeList.end(); ++it)
                {
                    MFSNode& node = *it;
                    const bool bNameMatch = (mFilePathCaseSensitive ? (node.mName.comparei(pNodeName) == 0) : (node.mName == pNodeName));

                    if(bNameMatch) // If this node matches...
                    {
                        if(!*pPath) // If this is the last node...
                        {
                            // The node being requested to be made already exists.
                            if(pError)  
                                *pError = MFS_ERROR_ALREADY_EXISTS;
                            return NULL;
                        }
                        else
                        {
                            pNode = &node;
                            break;
                        }
                    }
                }

                if(it == pNode->mNodeList.end()) // If the node doesn't already exist...
                {
                    EA_ASSERT(pNodeParent == pNode); // The logic above guarantees this.
                    pNode = &pNode->mNodeList.push_back();

                    pNode->mName         = pNodeName;
                    pNode->mNodeType     = ENTRY_TYPE_DIRECTORY;
                    pNode->mpNodeParent  = pNodeParent;
                    pNode->mCreationTime = GetFileTime();
                }
            }

            depth++;
        }

        // The caller needs ot set pNode->mNodeType appropriately.
        EA_ASSERT(pNode != NULL);
        return pNode; // Reached the end of the path, and this is the last node.
    }
    else
    {
        if(pError)  
            *pError = MFS_ERROR_BAD_PATHNAME;
    }

    return NULL; // Path doesn't begin with '/'
}


// Returns the MFSNode that corresponds to the parent of pPath.
// if(returnPreExisting) then return NULL if pPath already exists
// Returns NULL if pPath is malformed.
Internal::MFSNode* MemoryFileSystem::MFSCreateParentNodeFromPath(Internal::MFSNode* pRootNode, const char8_t* pPath, 
                                                                 bool returnPreExisting, uint32_t* pError)
{
    using namespace Internal;

    // This is the lazy way of solving this. A slightly better way would be to not do 
    // this via a temp bogus node like below. But the lazy way lets us take advantage
    // of existing functionality (e.g. directory creation in MFSCreateFSNodeFromPath).
    // However, there's probably a way to revise things to optimal.
    MFSNode* pNodeParent = NULL;

    if(returnPreExisting)
        pNodeParent = MFSNodeFromPath(pRootNode, pPath, true);

    if(!pNodeParent)
    {
        MFSNode* pNode = MFSCreateFSNodeFromPath(pRootNode, pPath, pError); // returns NULL if pPath already exists.

        if(pNode)
        {
            pNodeParent = pNode->mpNodeParent;
            MFSRemoveNode(pNode);
        }
    }

    return pNodeParent;
}

// Fills pPath in with path to MFSNode, including the MFSNode.
// pPath must have a capacity of at least kMaxMFSPathLength.
// Returns pPath.
// Always succeeds.
char8_t* MemoryFileSystem::MFSPathFromFSNode(const Internal::MFSNode* pNode, char8_t* pPath) const
{
    using namespace Internal;

    // To do: Revise this function so we don't use a C++ string, which allocates memory.
    //        Also, this function works by inserting in front of a string, which is a little slow.
    MFSName sPath;

    while(pNode)
    {
        if(pNode->mNodeType == ENTRY_TYPE_DIRECTORY)
            sPath.insert((eastl_size_t)0, 1, mFilePathSeparator);

        sPath.insert(0, pNode->mName);

        pNode = pNode->mpNodeParent;
    }

    EA::StdC::Strlcpy(pPath, sPath.c_str(), kMaxMFSPathLength);

    return pPath;
}


///////////////////////////////////////////////////////////////////////////////
// MemoryFileSystemIteration
///////////////////////////////////////////////////////////////////////////////

MemoryFileSystemIteration::MemoryFileSystemIteration()
  : mpMemoryFileSystem(NULL)
  //mWildcardFilter
  , mpDirectoryNode(NULL)
  , mCurrentNode()
  , mFilePathSeparator(kFilePathSeparatorDefault) // Default, but will be overwritten in IterateBegin based on mpMemoryFileSystem.
  , mFilePathCaseSensitive(true)                  // Default, but will be overwritten in IterateBegin based on mpMemoryFileSystem.
{
}


MemoryFileSystemIteration::~MemoryFileSystemIteration()
{
    EA_ASSERT(mpDirectoryNode == NULL);
}


bool MemoryFileSystemIteration::IterateBegin(const char8_t* directoryPath, const char8_t* wildcardFilter)
{
    using namespace Internal;

    EA_ASSERT(mpDirectoryNode == NULL);

    if(!mpDirectoryNode) // If not busy iterating already...
    {
        if(!wildcardFilter)
            wildcardFilter = "*";

        if(EA::StdC::Strlcpy(mWildcardFilter, wildcardFilter, EAArrayCount(mWildcardFilter)) >= EAArrayCount(mWildcardFilter))
            return false;  // wildcardFilter is too long.

        if(!mpMemoryFileSystem)
            mpMemoryFileSystem = GetDefaultFileSystem();

        if(mpMemoryFileSystem)
        {
            char8_t dirTemp[kMaxMFSPathLength];

            if(!mpMemoryFileSystem->PossiblyStripDirSeparator(directoryPath, dirTemp))
                return false; // directoryPath is too long.

            mFilePathSeparator     = mpMemoryFileSystem->GetFilePathSeparator();
            mFilePathCaseSensitive = mpMemoryFileSystem->GetFilePathCaseSensitive();
            mpDirectoryNode        = mpMemoryFileSystem->MFSNodeFromPath(mpMemoryFileSystem->GetRootNode(), directoryPath, false);
            mCurrentNode           = MFSNodeList::iterator();

            if(mpDirectoryNode && (mpDirectoryNode->mNodeType == ENTRY_TYPE_DIRECTORY))
            {
                const int kFnMatchFlags = (mFilePathSeparator ? EA::IO::kFNMUnixPath : EA::IO::kFNMDosPath) | 
                                          (mFilePathCaseSensitive ? 0 : EA::IO::kFNMCaseFold);

                for(MFSNodeList::iterator it = mpDirectoryNode->mNodeList.begin(); it != mpDirectoryNode->mNodeList.end(); ++it)
                {
                    MFSNode& node = *it;

                    if(EA::IO::FnMatch(mWildcardFilter, node.mName.c_str(), kFnMatchFlags))
                    {
                        // To do: We don't currently have a way to lock the directory from being deleted while we are iterating it.
                        //        We may want to solve this by adding opening the directory. Note that if we do that we'll need to modify some other code in this file.
                        mCurrentNode = it;
                        return true;
                    }
                }
            }
        }
    }

    mpDirectoryNode = NULL;
    return false;
}


bool MemoryFileSystemIteration::IterateNext()
{
    using namespace Internal;

    EA_ASSERT(mpDirectoryNode);

    if(mpDirectoryNode && (mCurrentNode != mpDirectoryNode->mNodeList.end())) // If actively iterating and not done iterating...
    {
        const int kFnMatchFlags = (mFilePathSeparator ? EA::IO::kFNMUnixPath : EA::IO::kFNMDosPath) | 
                                  (mFilePathCaseSensitive ? 0 : EA::IO::kFNMCaseFold);

        for(MFSNodeList::iterator it = ++mCurrentNode; it != mpDirectoryNode->mNodeList.end(); ++it)
        {
            MFSNode& node = *it;

            if(EA::IO::FnMatch(mWildcardFilter, node.mName.c_str(), kFnMatchFlags))
            {
                mCurrentNode = it;
                return true;
            }
        }
    }

    return false;
}


void MemoryFileSystemIteration::IterateEnd()
{
    using namespace Internal;

    EA_ASSERT(mpDirectoryNode);

    mpDirectoryNode = NULL;
    mCurrentNode    = MFSNodeList::iterator();
}


EntryType MemoryFileSystemIteration::GetEntryType()
{
    using namespace Internal;

    if(mpDirectoryNode)
        return (mCurrentNode->mNodeType == ENTRY_TYPE_FILE) ? ENTRY_TYPE_FILE : ENTRY_TYPE_DIRECTORY;

    return ENTRY_TYPE_NONE;
}


const char8_t* MemoryFileSystemIteration::GetEntryName()
{
    using namespace Internal;

    if(mpDirectoryNode) // If mCurrenNode is valid...
        return mCurrentNode->mName.c_str();

    return NULL;
}


uint64_t MemoryFileSystemIteration::GetEntryTime()
{
    if(mpDirectoryNode) // If mCurrenNode is valid...
        return mCurrentNode->mCreationTime;
    
    return 0;
}


uint64_t MemoryFileSystemIteration::GetEntrySize()
{
    using namespace Internal;

    if(mpDirectoryNode)
        return mCurrentNode->mData.size();

    return 0;
}


uint32_t MemoryFileSystemIteration::GetLastError()
{
    // To do: We need to come up with a universal error identification system for ICoreFileSystem.
    return 0;
}


MemoryFileSystem* MemoryFileSystemIteration::GetMemoryFileSystem() const
{
    return mpMemoryFileSystem;
}


void MemoryFileSystemIteration::SetMemoryFileSystem(MemoryFileSystem* pMemoryFileSystem)
{
    mpMemoryFileSystem = pMemoryFileSystem;
}



///////////////////////////////////////////////////////////////////////////////
// DirectoryIterator
///////////////////////////////////////////////////////////////////////////////

DirectoryIterator::Entry::Entry(EntryType entryType, const char8_t* pName)
  : mType(entryType)
  , msName(pName ? pName : "")
  , mTime(0)
  , mSize(0)
{
}


DirectoryIterator::DirectoryIterator()
  : mnListSize(0)
  , mnRecursionIndex(0)
  , mpBaseDirectory(NULL)
  , mBaseDirectoryLength(0)
  , mpMemoryFileSystem(NULL)
{
}


size_t DirectoryIterator::Read(const char8_t* pDirectory, EntryList& entryList, 
                               const char8_t* pFilterPattern, int nDirectoryEntryFlags, 
                               size_t maxResultCount, bool /*bReadFileStat*/)
{
    size_t resultCount = 0;
    MemoryFileSystemIteration iterator;

    if(!mpMemoryFileSystem)
        mpMemoryFileSystem = GetDefaultFileSystem();

    if(mpMemoryFileSystem)
    {
        char8_t pathSeparator = mpMemoryFileSystem->GetFilePathSeparator();
        iterator.SetMemoryFileSystem(mpMemoryFileSystem);

        bool bResult = iterator.IterateBegin(pDirectory, pFilterPattern);

        if(bResult) // If anything exists at pDirectory...
        {
            const char8_t kDotSlash[]    = {      '.', pathSeparator, '\0' }; // ./
            const char8_t kDotDotSlash[] = { '.', '.', pathSeparator, '\0' }; // ../ 

            do
            {
                const char8_t* pEntryName = iterator.GetEntryName();

                if((EA::StdC::Strcmp(pEntryName, kDotSlash) != 0) && // If it is neither "./" nor "../"
                   (EA::StdC::Strcmp(pEntryName, kDotDotSlash) != 0))
                {
                    EntryType entryType = iterator.GetEntryType();

                    if(entryType == ENTRY_TYPE_DIRECTORY)
                    {
                        if(nDirectoryEntryFlags & ENTRY_TYPE_DIRECTORY) // If the user wants to enumerate directories...
                        {
                            resultCount++;

                            entryList.push_back();
                            Entry& entry = entryList.back();

                            entry.mType  = ENTRY_TYPE_DIRECTORY;
                            entry.msName = iterator.GetEntryName();
                            entry.mTime  = iterator.GetEntryTime();
                            entry.mSize  = iterator.GetEntrySize();
                        }
                    }
                    else
                    {
                        if(nDirectoryEntryFlags & ENTRY_TYPE_FILE) // If the user wants to enumerate files...
                        {
                            resultCount++;

                            entryList.push_back();
                            Entry& entry = entryList.back();

                            entry.mType  = ENTRY_TYPE_FILE;
                            entry.msName = iterator.GetEntryName();
                            entry.mTime  = iterator.GetEntryTime();
                            entry.mSize  = iterator.GetEntrySize();
                        }
                    }
                }
            } while ((resultCount < maxResultCount) && iterator.IterateNext());

            iterator.IterateEnd();
        }
    }

    return resultCount;
}


///////////////////////////////////////////////////////////////////////////////
// ReadRecursive
//
size_t DirectoryIterator::ReadRecursive(const char8_t* pBaseDirectory, EntryList& entryList, const char8_t* pFilterPattern, int nEntryTypeFlags, 
                                        bool bIncludeBaseDirectoryInSearch, bool bFullPaths, size_t maxResultCount, bool bReadFileStat)
{
    if(!mpMemoryFileSystem)
        mpMemoryFileSystem = GetDefaultFileSystem();

    if(mpMemoryFileSystem)
    {
        char8_t pathSeparator = mpMemoryFileSystem->GetFilePathSeparator();

        if(mnRecursionIndex++ == 0) // If being called for the first time...
        {
            mnListSize           = 0;
            mpBaseDirectory      = pBaseDirectory;
            mBaseDirectoryLength = (eastl_size_t)EA::StdC::Strlen(pBaseDirectory);

            if((mBaseDirectoryLength == 0) || (pBaseDirectory[mBaseDirectoryLength - 1] != pathSeparator))
                mBaseDirectoryLength++;
        }

        if((nEntryTypeFlags & ENTRY_TYPE_FILE) && 
           (bIncludeBaseDirectoryInSearch || (mnRecursionIndex > 1)) && 
           (mnListSize < maxResultCount))
        {
            // Add all files in the current directory into the list, using the filter pattern.
            const size_t additionCount = Read(pBaseDirectory, entryList, pFilterPattern, ENTRY_TYPE_FILE, maxResultCount - mnListSize, bReadFileStat);

            EntryList::iterator it(entryList.end());
            eastl::advance(it, -(int32_t)(uint32_t)additionCount);

            for(; it != entryList.end(); ++it)
            {
                Entry& entry = *it;

                mnListSize++;

                const eastl_size_t savedLength = entry.msName.length();
                entry.msName.insert(0, pBaseDirectory);
                const eastl_size_t directoryEnd = entry.msName.length() - savedLength;

                if(directoryEnd && (entry.msName[directoryEnd - 1] != pathSeparator))
                    entry.msName.insert(directoryEnd, 1, pathSeparator);

                if(!bFullPaths)
                    entry.msName.erase(0, mBaseDirectoryLength);
            }
        }

        if(mnListSize < maxResultCount)
        {
            EntryString pathTemp;

            // To do: Find a way to avoid this temporary list.
            // Since the list is only a list of directories under the 
            // current directory, it shouldn't need all that many entries.
            EntryList entryListTemp(entryList.get_allocator());

            // Add all directories in the current directory into the list, ignoring the filter pattern.
            Read(pBaseDirectory, entryListTemp, NULL, ENTRY_TYPE_DIRECTORY, kMaxEntryCountDefault, bReadFileStat);

            for(EntryList::iterator it = entryListTemp.begin(); (it != entryListTemp.end()) && (mnListSize < maxResultCount); ++it)
            {
                const Entry& entry = *it; 

                pathTemp.assign(pBaseDirectory);
                if(pathTemp.empty() || (pathTemp.back() != pathSeparator))
                    pathTemp += pathSeparator;
                pathTemp += entry.msName;

                // Possibly add this directory to the entry list.
                if(nEntryTypeFlags & ENTRY_TYPE_DIRECTORY)
                {
                    if(!pFilterPattern || EA::IO::FnMatch(pFilterPattern, entry.msName.c_str(), EA::IO::kFNMCaseFold))
                    {
                        mnListSize++;
                        entryList.push_back();
                        Entry& listEntry = entryList.back();
                        listEntry.mType  = ENTRY_TYPE_DIRECTORY;
                        listEntry.msName = pathTemp.c_str();

                        if(!bFullPaths)
                            listEntry.msName.erase(0, mBaseDirectoryLength);
                    }
                }

                // Now recursively read the subdirectory.
                ReadRecursive(pathTemp.c_str(), entryList, pFilterPattern, nEntryTypeFlags, true, bFullPaths, maxResultCount, bReadFileStat);
            }
        }

        mnRecursionIndex--;
    }
    else
        mnListSize = 0;

    return mnListSize;
}


MemoryFileSystem* DirectoryIterator::GetMemoryFileSystem() const
{
    return mpMemoryFileSystem;
}


void DirectoryIterator::SetMemoryFileSystem(MemoryFileSystem* pMemoryFileSystem)
{
    mpMemoryFileSystem = pMemoryFileSystem;
}

///////////////////////////////////////////////////////////////////////////////
// MemoryFile
///////////////////////////////////////////////////////////////////////////////

MemoryFile::MemoryFile(const char8_t* pPath8)
  : mRefCount(0)
  , mAccessFlags(0)
  , mLastError(MFS_ERROR_NOT_OPEN)
  , mpFSNode(NULL)
  , mpFileSystem(NULL)
  //mFilePath
  , mPosition(0)
{
    if(pPath8)
        MemoryFile::SetPath(pPath8);
    else
        mFilePath[0] = 0;
}

MemoryFile::MemoryFile(const MemoryFile& mf)
  : IStream()
{
    operator=(mf);
}

MemoryFile& MemoryFile::operator=(const MemoryFile& mf)
{
    if(&mf != this)
    {
        Close();

        // Don't copy some members.
        mAccessFlags = 0;
        mLastError   = MFS_ERROR_NOT_OPEN;
        mPosition    = 0;
        EA::StdC::Strcpy(mFilePath, mf.mFilePath); // This always succeeds and requires no error checking.
    }

    return *this;
}


MemoryFile::~MemoryFile()
{
    MemoryFile::Close();
}

int MemoryFile::AddRef()
{
    return ++mRefCount;
}

int MemoryFile::Release()
{
    if(mRefCount > 1)
        return --mRefCount;
    delete this; // To do: Replace this with an alternative memory allocation scheme.
    return 0;
}


MemoryFileSystem* MemoryFile::GetMemoryFileSystem() const
{
    return mpFileSystem;
}


void MemoryFile::SetMemoryFileSystem(MemoryFileSystem* pFileSystem)
{
    if(mpFSNode && (pFileSystem != mpFileSystem))
        { EA_FAIL(); } // Cannot change the file system when the file is open.
    else
        mpFileSystem = pFileSystem;
}


void MemoryFile::SetPath(const char8_t* pPath8)
{
    if(!mpFSNode && pPath8)
    {
        size_t n = EA::StdC::Strlcpy(mFilePath, pPath8, EAArrayCount(mFilePath));

        // This function doesn't return an error, so we have little option other than
        // to silently cancel the path assignment if the input path was too long.
        if(n >= EAArrayCount(mFilePath))
        {
            mFilePath[0] = 0;
            mLastError = MFS_ERROR_BAD_PATHNAME;
        }
    }
}

size_t MemoryFile::GetPath(char8_t* pPath8, size_t nPathCapacity)
{
    return EA::StdC::Strlcpy(pPath8, mFilePath, nPathCapacity);
}

bool MemoryFile::Open(int accessFlags, int cd, int /*sharing*/, int /*usageHints*/)
{
    using namespace Internal;

    // Currently we unilaterally set this here.
    if(!mpFileSystem)
        mpFileSystem = GetDefaultFileSystem();

    if(mpFileSystem)
        mpFileSystem->Lock();

    bool returnValue = false;

    if(!mAccessFlags && accessFlags) // If not already open and requesting some kind of open...
    {
        if(mpFileSystem)
        {
            bool canOpenFile = true;

            // Get the file name
            char8_t fileName[kMaxMFSPathLength];
            mpFileSystem->MFSFileNameFromPath(mFilePath, fileName);

            if(!fileName[0])
            {
                canOpenFile = false;
                mLastError  = MFS_ERROR_INVALID_NAME;
            }

            // Handle sharing
            // We don't currently support user-settable sharing, but we act as if it's the following.
            // sharing = ((accessFlags & kAccessWrite) ? 0 : kShareRead);

            // Handle DISPOSITION_DEFAULT
            if(cd == DISPOSITION_DEFAULT)
            {
                if((accessFlags & OPEN_WRITE) && (accessFlags & OPEN_READ))
                    cd = DISPOSITION_OPEN_ALWAYS;       // Never fails, creates if doesn't exist, keeps contents.
                else if(accessFlags & OPEN_WRITE)
                    cd = DISPOSITION_CREATE_ALWAYS;     // Never fails, always opens or creates and truncates to 0.
                else
                    cd = DISPOSITION_OPEN_EXISTING;  // Fails if file doesn't exist, keeps contents.
            }

            // Test for open-ability.
            MFSNode* pNode                 = mpFileSystem->MFSNodeFromPath(&mpFileSystem->mRootNode, mFilePath, false);
            int      currentFileOpenAccess = mpFileSystem->MFSGetCurrentFileOpenAccess(pNode); // Is the file open for somebody else for reading and/or writing.

            EA_COMPILETIME_ASSERT((DISPOSITION_CREATE_NEW == 1) && (DISPOSITION_DEFAULT == 6)); // Some of the logic below assumes this.

            // Fail if it's already open for write.
            if((currentFileOpenAccess & OPEN_WRITE))
            {
                canOpenFile = false;
                mLastError = MFS_ERROR_ACCESS_DENIED;
            }

            // Fail if we are trying to open for write and it's already open for read or write.
            else if((accessFlags & OPEN_WRITE) && currentFileOpenAccess)
            {
                canOpenFile = false;
                mLastError = MFS_ERROR_ACCESS_DENIED;
            }

            // Fail if we are opening with a cd that necessarily requires write, but didn't specify write access.
            else if(((cd == DISPOSITION_CREATE_NEW) || (cd == DISPOSITION_CREATE_ALWAYS)) && !(accessFlags & OPEN_WRITE))
            {
                canOpenFile = false;
                mLastError = MFS_ERROR_INVALID_PARAMETER;
            }

            // Fail if DISPOSITION_OPEN_ALWAYS was requested, access is read, but the file doesn't already exist.
            else if((cd == DISPOSITION_OPEN_ALWAYS) && !(accessFlags & OPEN_WRITE) && !pNode)
            {
                canOpenFile = false;
                mLastError = MFS_ERROR_FILE_NOT_FOUND;
            }

            // Fail if DISPOSITION_CREATE_NEW was requested, but the file already exists.
            else if((cd == DISPOSITION_CREATE_NEW) && pNode)
            {
                canOpenFile = false;
                mLastError = MFS_ERROR_ALREADY_EXISTS;
            }

            // Fail if DISPOSITION_OPEN_EXISTING was requested, but the file doesn't exist.
            else if((cd == DISPOSITION_OPEN_EXISTING) && !pNode)
            {
                canOpenFile = false;
                mLastError = MFS_ERROR_FILE_NOT_FOUND;
            }

            // Fail if DISPOSITION_TRUNCATE_EXISTING was requested, but the file doesn't exist.
            else if((cd == DISPOSITION_TRUNCATE_EXISTING) && !pNode)
            {
                canOpenFile = false;
                mLastError = MFS_ERROR_FILE_NOT_FOUND;
            }

            // Fail if an unknown/invalid cd value was specified.
            else if((cd < DISPOSITION_CREATE_NEW) || (cd >= DISPOSITION_DEFAULT))
            {
                canOpenFile = false;
                mLastError = MFS_ERROR_INVALID_PARAMETER;
            }

            if(canOpenFile)
            {
                // We are certain that we can open the file, so execute whatever cd requires.
                if(cd == DISPOSITION_CREATE_NEW) // Fails if file already exists.
                {
                    EA_ASSERT(!pNode);
                    mpFSNode = mpFileSystem->MFSCreateFSNodeFromPath(&mpFileSystem->mRootNode, mFilePath, NULL);
                }
                else if(cd == DISPOSITION_CREATE_ALWAYS) // Never fails, always opens or creates and truncates to 0.
                {
                    if(pNode)
                        mpFSNode = pNode;
                    else
                        mpFSNode = mpFileSystem->MFSCreateFSNodeFromPath(&mpFileSystem->mRootNode, mFilePath, NULL);

                    mpFSNode->mData.clear();
                }
                else if(cd == DISPOSITION_OPEN_EXISTING) // Fails if file doesn't exist, keeps contents.
                {
                    EA_ASSERT(pNode);
                    mpFSNode = pNode;
                }
                else if(cd == DISPOSITION_OPEN_ALWAYS) // Never fails, creates if doesn't exist, keeps contents.
                {
                    if(pNode)
                        mpFSNode = pNode;
                    else
                        mpFSNode = mpFileSystem->MFSCreateFSNodeFromPath(&mpFileSystem->mRootNode, mFilePath, NULL);
                }
                else if(cd == DISPOSITION_TRUNCATE_EXISTING) // Fails if file doesn't exist, but truncates to 0 if it does.
                {
                    EA_ASSERT(pNode);
                    mpFSNode = pNode;
                    mpFSNode->mData.clear();
                }

                mAccessFlags = accessFlags;
                mPosition    = 0;
                mLastError   = 0;

                EA_ASSERT(mpFSNode != NULL);
                mpFSNode->mName     = fileName;
                mpFSNode->mNodeType = ENTRY_TYPE_FILE;

                if(mpFileSystem)
                    mpFileSystem->AddOpenFilePtr(this);

                mpFSNode->AddOpenFilePtr(this);

                returnValue = true;
            }
        }
    }

    if(mpFileSystem)
        mpFileSystem->Unlock();

    return returnValue;
}


bool MemoryFile::Close()
{
    bool returnValue;

    if(mpFSNode)
    {
        if(mpFileSystem)
            mpFileSystem->Lock();

        mpFSNode->RemoveOpenFilePtr(this);

        if(mpFileSystem)
            mpFileSystem->RemoveOpenFilePtr(this);

        mAccessFlags = 0;
        mLastError   = MFS_ERROR_NOT_OPEN;
        mpFSNode     = NULL;
      //mpFileSystem = NULL;  // Don't set this to NULL. If we do need to set it to NULL then we need to do it after its use below.
        mPosition    = 0;

        if(mpFileSystem)
            mpFileSystem->Unlock();

        returnValue = true;
    }
    else
        returnValue = false;

    return returnValue;
}


int MemoryFile::GetAccessFlags() const
{
    return mAccessFlags;
}


int MemoryFile::GetState() const
{
    return mLastError;
}


size_type MemoryFile::GetSize() const
{
    if(mpFileSystem)
        mpFileSystem->Lock();

    size_type size;

    if(mpFSNode)
        size = mpFSNode->mData.size();
    else
    {
        size = kSizeTypeError;
        mLastError = MFS_ERROR_ACCESS_DENIED;
    }

    if(mpFileSystem)
        mpFileSystem->Unlock();

    return size;
}


bool MemoryFile::SetSize(size_type size)
{
    if(mpFileSystem)
        mpFileSystem->Lock();

    bool returnValue = false;

    if(mAccessFlags & OPEN_WRITE)
    {
        EA_ASSERT(mpFSNode != NULL);
        if(mpFSNode)
            mpFSNode->mData.resize((eastl_size_t)size);
        returnValue = true;
    }
    else
        mLastError = MFS_ERROR_ACCESS_DENIED;

    if(mpFileSystem)
        mpFileSystem->Unlock();

    return returnValue;
}


off_type MemoryFile::GetPosition(PositionType positionType) const
{
    if(mpFileSystem)
        mpFileSystem->Lock();

    off_type returnValue = (off_type)kSizeTypeError;

    if(mAccessFlags & OPEN_READ)
    {
        // We have a small problem here: off_type is signed while mPosition is 
        // unsigned but of equal size to off_type. Thus, if mPosition is of a 
        // high enough value then the return value can be negative. 

        switch(positionType)
        {
            case EA::IO::kPositionTypeBegin:
                returnValue = (off_type)mPosition;
                break;

            case EA::IO::kPositionTypeEnd:
                returnValue = (off_type)(mPosition - mpFSNode->mData.size());
                break;

            case EA::IO::kPositionTypeCurrent:
                returnValue = 0;
                break;

            default:
                mLastError = MFS_ERROR_INVALID_PARAMETER;
                break;
        }
    }
    else
        mLastError = MFS_ERROR_ACCESS_DENIED;

    if(mpFileSystem)
        mpFileSystem->Unlock();

    return returnValue; // For kPositionTypeCurrent the result is always zero for a 'get' operation.
}


bool MemoryFile::SetPosition(off_type position, PositionType positionType)
{
    if(mpFileSystem)
        mpFileSystem->Lock();

    bool returnValue = false;

    if(mAccessFlags & (OPEN_READ | OPEN_WRITE))
    {
        off_type positionNew;

        switch(positionType)
        {
            case EA::IO::kPositionTypeBegin:
                EA_ASSERT(position >= 0);
                positionNew = position;
                break;

            case EA::IO::kPositionTypeEnd:
                positionNew = (off_type)mpFSNode->mData.size() + position;
                break;

            case EA::IO::kPositionTypeCurrent:
                positionNew = (off_type)mPosition + position;
                break;

            default:
                positionNew = 0;
                mLastError  = MFS_ERROR_INVALID_PARAMETER;
                break;
        }

        // Deal with out-of-bounds situations that result from the above.
        if(positionNew < 0)
            mLastError = MFS_ERROR_NEGATIVE_SEEK;
        else
        {
            EA_ASSERT(mPosition < (eastl_size_t((off_type)-1) / 2)); // Sanity check.

            if(mPosition > mpFSNode->mData.size()) // If the new position represents a file size change (and thus a write)...
            {
                if(mAccessFlags & OPEN_WRITE)
                {
                    // To consider: Have a provision for handling memory allocation errors on the resize below.
                    mPosition = (size_type)positionNew;
                    mpFSNode->mData.resize(mPosition + 1); // +1 because position is a 0-based index, but size is a count.
                    returnValue = true;
                }
                else
                    mLastError = MFS_ERROR_ACCESS_DENIED;
            }
            else
            {
                mPosition = (size_type)positionNew;
                returnValue = true;
            }
        }
    }
    else
        mLastError = MFS_ERROR_ACCESS_DENIED;

    if(mpFileSystem)
        mpFileSystem->Unlock();

    return returnValue;
}


size_type MemoryFile::GetAvailable() const
{
    if(mpFileSystem)
        mpFileSystem->Lock();

    size_type availableSize;

    if(mAccessFlags & OPEN_READ)
        availableSize = (size_type)(mpFSNode->mData.size() - mPosition);
    else
    {
        mLastError    = MFS_ERROR_ACCESS_DENIED;
        availableSize = kSizeTypeError;
    }

    if(mpFileSystem)
        mpFileSystem->Unlock();

    return availableSize;
}


size_type MemoryFile::Read(void* pData, size_type readSize)
{
    if(mpFileSystem)
        mpFileSystem->Lock();

    size_type readCount = 0;

    if(mAccessFlags & OPEN_READ)
    {
        if(readSize > 0)
        {
            const size_type fileSize       = (size_type)mpFSNode->mData.size();
            const size_type bytesAvailable = (size_type)(fileSize - mPosition);
            EA_ASSERT(mPosition <= fileSize);

            if(bytesAvailable > 0)
            {
                if(readSize > bytesAvailable)
                    readSize = bytesAvailable;

                memcpy(pData, mpFSNode->mData.data() + mPosition, (size_t)readSize);
                mPosition += readSize;

                readCount = readSize;
            } // Else no more to read, so return 0.
        } // Else user requested 0 read size, so return 0.
    }
    else
    {
        mLastError = MFS_ERROR_ACCESS_DENIED;
        readCount  = kSizeTypeError;
    }

    if(mpFileSystem)
        mpFileSystem->Unlock();

    return readCount;
}


bool MemoryFile::Write(const void* pData, size_type writeSize)
{
    if(mpFileSystem)
        mpFileSystem->Lock();

    bool writeSuccess = false;

    if(mAccessFlags & OPEN_WRITE)
    {
        if(writeSize > 0)
        {
            EA_ASSERT(mPosition <= mpFSNode->mData.size());

            size_type requiredFileSize = (mPosition + writeSize);

            if(requiredFileSize > mpFSNode->mData.size()) // If we need to increase our size...
            {
                // It would be less efficient to call mData.resize() because that initializes bytes while resizing. So we use mData.insert(end(), ...).
                size_type      sizeBefore     = (size_type)(mpFSNode->mData.size() - mPosition);
                size_type      sizeAfter      = writeSize - sizeBefore;
                const uint8_t* pData8After    = static_cast<const uint8_t*>(pData) + sizeBefore;
                const uint8_t* pData8AfterEnd = pData8After + sizeAfter;

                if(sizeBefore)
                    memcpy(mpFSNode->mData.data() + mPosition, pData, (size_t)sizeBefore);

                mpFSNode->mData.insert(mpFSNode->mData.end(), pData8After, pData8AfterEnd);
            }
            else
                memcpy(mpFSNode->mData.data() + mPosition, pData, (size_t)writeSize);

            mPosition += writeSize;
        }

        writeSuccess = true; 
    }
    else
        mLastError = MFS_ERROR_ACCESS_DENIED;

    if(mpFileSystem)
        mpFileSystem->Unlock();

    return writeSuccess;
}


bool MemoryFile::Flush()
{
    return true; // Nothing to do.
}



} // namespace Patch
} // namespace EA








