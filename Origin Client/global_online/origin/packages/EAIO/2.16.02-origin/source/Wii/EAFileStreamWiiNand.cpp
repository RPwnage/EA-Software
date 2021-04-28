/////////////////////////////////////////////////////////////////////////////
// EAFileStreamWiiNand.cpp
//
// Copyright (c) 2007 Criterion Software Limited and its licensors. All Rights Reserved.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#include <EAIO/internal/Config.h>
#include <EAIO/Wii/EAFileStreamWiiNand.h>
#include <EAStdC/EAString.h>
#include <coreallocator/icoreallocator_interface.h>
#include <revolution/os.h>
#include EA_ASSERT_HEADER


namespace EA
{

namespace IO
{


///////////////////////////////////////////////////////////////////////////////
// Utility functions
///////////////////////////////////////////////////////////////////////////////

template <typename T>
inline T Clamp(T x, T low, T high)
{
    return (x > high) ? high : ((x < low) ? low : x);
}

template <typename T>
inline T Max(T a, T b)
{
    return (a < b) ? b: a;
}




///////////////////////////////////////////////////////////////////////////////
// FilePosition
///////////////////////////////////////////////////////////////////////////////

size_type FilePosition::Skip(off_type offset)
{
    if(offset)
    {
        int64_t position = mPosition + offset;
        position  = Clamp<int64_t>(position, 0, (int64_t)mFileSize);
        mPosition = (size_type)position;
    }

    return mPosition;
}


// Moves the file pointer by adding data.
// If the file end is exceeded, the file size is increased.
// Returns the file position after the move.
size_type FilePosition::Append(off_type offset)
{
    int64_t position = mPosition + offset;
    
    if(position >= 0)
    {
        mPosition = (size_type)position;
        mFileSize = Max(mPosition, mFileSize);
    }
    else
        mPosition = 0;

    return mPosition;
}


void FilePosition::Seek(off_type offset, int origin)
{
    switch (origin)
    {
        default:
        case kPositionTypeBegin:
            mPosition = 0;
            break;

        case kPositionTypeCurrent:
            break;

        case kPositionTypeEnd:
            mPosition = mFileSize;
            break;
    }

    Skip(offset);
}



///////////////////////////////////////////////////////////////////////////////
// FileStreamWiiNand
///////////////////////////////////////////////////////////////////////////////

FileStreamWiiNand::FileStreamWiiNand(const char8_t* pPath8)
{
    mnRefCount = 0;
    Reset();
    FileStreamWiiNand::SetPath(pPath8); // Note that in a constructor, the virtual function mechanism is inoperable.
}


FileStreamWiiNand::FileStreamWiiNand(const char16_t* pPath16)
{
    mnRefCount = 0;
    Reset();
    FileStreamWiiNand::SetPath(pPath16); // Note that in a constructor, the virtual function mechanism is inoperable.
}


FileStreamWiiNand::FileStreamWiiNand(const FileStreamWiiNand& fs)
{
    mnRefCount = 0;
    Reset();
    FileStreamWiiNand::SetPath(fs.mpPath8); // Note that in a constructor, the virtual function mechanism is inoperable.
}


FileStreamWiiNand::~FileStreamWiiNand()
{
    FileStreamWiiNand::Close(); // Note that in a destructor, the virtual function mechanism is inoperable.
}


FileStreamWiiNand& FileStreamWiiNand::operator=(const FileStreamWiiNand& fs)
{
    Close();
    Reset();
    SetPath(fs.mpPath8);

    return *this;
}


int FileStreamWiiNand::AddRef()
{
    return ++mnRefCount;
}


int FileStreamWiiNand::Release()
{
    if(mnRefCount > 1)
        return --mnRefCount;
    delete this;
    return 0;
}


void FileStreamWiiNand::Reset()
{
    memset(&mFileInfo, 0, sizeof(mFileInfo));
    mFileInfo.mpFileStreamWiiNand = this;

    mnAccessFlags    = 0;
    mnCD             = 0;
    mnSharing        = 0;
    mnUsageHints     = kUsageHintNone;
    mnLastError      = kStateNotOpen;

    // Async processing data
    mAsyncResult     = 0;
    mCallback        = NULL;
    mpContext        = NULL;
    mIsBusy          = false;
}


void FileStreamWiiNand::SetPath(const char8_t* pPath8)
{
    if(!mnAccessFlags && pPath8)
        EA::StdC::Strlcpy(mpPath8, pPath8, kMaxPathLength);
}


void FileStreamWiiNand::SetPath(const char16_t* pPath16)
{
    if(!mnAccessFlags && pPath16)
        EA::StdC::Strlcpy(mpPath8, pPath16, kMaxPathLength);
}


size_t FileStreamWiiNand::GetPath(char8_t* pPath8, size_t nPathLength)
{
    // Return the length of the path we -tried- to create.
    return EA::StdC::Strlcpy(pPath8, mpPath8, nPathLength);
}


size_t FileStreamWiiNand::GetPath(char16_t* pPath16, size_t nPathLength)
{
    if(pPath16 && mpPath8)
        return EA::StdC::Strlcpy(pPath16, mpPath8, nPathLength);

    // In this case the user wants just the length of the return value.
    char16_t pathTemp[kMaxPathLength];
    return GetPath(pathTemp, kMaxPathLength);
}


bool FileStreamWiiNand::Open(int nAccessFlags, int nCreationDisposition, int /*nSharing*/, int /*nUsageHints*/)
{
    if(!mnAccessFlags && nAccessFlags) // If not already open and if some kind of access is requested...
    {
        NANDStatus nandStatus;

        // It is convenient that the Wii read and write flags are identical to the EAIO read and write flags.
        EA_COMPILETIME_ASSERT((kAccessFlagRead  == NAND_ACCESS_READ) && (kAccessFlagWrite == NAND_ACCESS_WRITE));

        if(nCreationDisposition == kCDDefault)
        {
            if(nAccessFlags & kAccessFlagWrite)
                nCreationDisposition = kCDOpenAlways;
            else
                nCreationDisposition = kCDOpenExisting;
        }

        switch(nCreationDisposition)
        {
            case kCDOpenExisting:       // Fails if file doesn't exist, keeps contents.
            case kCDTruncateExisting:   // Fails if file doesn't exist, but truncates to 0 if it does.
            {
                mnLastError = NANDGetStatus(mpPath8, &nandStatus);

                if(mnLastError != NAND_RESULT_OK) // If the file doesn't exist...
                    return false;

                break;
            }

            case kCDCreateNew: // Fails if file already exists.
            {
                EA_ASSERT((mnAccessFlags & kAccessFlagWrite) != 0);

                mnLastError = NANDGetStatus(mpPath8, &nandStatus);

                if(mnLastError == NAND_RESULT_OK) // If the file already exists...
                {
                    mnLastError = NAND_RESULT_EXISTS;
                    return false;
                }

                break;
            }
        }

        // NANDOpen will never create a file, it will only open an existing file.
        mnLastError = NANDOpen(mpPath8, &mFileInfo.mNandInfo, (u8)nAccessFlags);

        if(mnLastError == NAND_RESULT_NOEXISTS) // If the file doesn't exist...
        {
            if((nCreationDisposition != kCDOpenExisting) && 
               (nCreationDisposition != kCDTruncateExisting)) // If we should try to create it...
            {
                mnLastError = NANDCreate(mpPath8, 0x3f, 0); // Nintendo does not document what the third parameter means, but uses zero in its demo code.
    
                if(mnLastError == NAND_RESULT_OK)
                    mnLastError = NANDOpen(mpPath8, &mFileInfo.mNandInfo, (u8)nAccessFlags);
            }
        }
        else if((nCreationDisposition == kCDCreateNew) && (mnLastError == NAND_RESULT_OK)) // If the user wanted to create the file anew.
        {
            NANDClose(&mFileInfo.mNandInfo);
            mnLastError = NAND_RESULT_EXISTS;
        }

        if(mnLastError == NAND_RESULT_OK)
        {
            u32 fileSize;
            NANDGetLength(&mFileInfo.mNandInfo, &fileSize);

            mFilePosition.SetFileSize(fileSize);
            mFilePosition.Seek(0, kPositionTypeBegin);
            mnAccessFlags = nAccessFlags;

            // Unfortunately, the Wii doesn't provide a way to deal with kCDTruncateExisting.
            EA_ASSERT((nCreationDisposition != kCDTruncateExisting) || (fileSize == 0));

            return true;
        }
    }

    return false;
}


bool FileStreamWiiNand::Close()
{
    if(mnAccessFlags)
    {
        mnLastError   = NANDClose(&mFileInfo.mNandInfo);
        mnAccessFlags = kStateNotOpen;

        return (mnLastError == NAND_RESULT_OK);
    }

    return false;
}


int FileStreamWiiNand::GetAccessFlags() const
{
    return mnAccessFlags;
}


int FileStreamWiiNand::GetState() const
{
    return mnLastError;
}


size_type FileStreamWiiNand::GetSize() const
{
    EA_ASSERT(mnAccessFlags != 0);
    return mFilePosition.GetFileSize();
}


bool FileStreamWiiNand::SetSize(size_type size)
{
    bool            bReturnValue = false;
    const size_type currentSize  = GetSize();
    const off_type  currentPos   = GetPosition();

    if(size > currentSize)
    {
        if(SetPosition(size - 1))
        {
            char zero = 0;
            bReturnValue = Write(&zero, 1);
            SetPosition(currentPos);
        }
    }
    else if(size < currentSize)
    {
        // Currently the NAND file system doesn't appear to directly allow truncating.
        // However, we can create a new empty file and copy our contents to it and 
        // rename the file to this file name.
        EA_ASSERT(false);
        SetPosition((off_type)size);
    }
    else
        bReturnValue = true;

    return bReturnValue;
}


off_type FileStreamWiiNand::GetPosition(PositionType positionType) const
{
    EA_ASSERT(mnAccessFlags != 0);

    off_type result;
    const size_type nCurrentPosition = mFilePosition.Tell();

    if(positionType == kPositionTypeBegin)
        result = (off_type)nCurrentPosition;
    else if(positionType == kPositionTypeEnd)
        result = (off_type)nCurrentPosition - (off_type)mFilePosition.GetFileSize();
    else // Else the position relative to current, which is by definition zero.
        result = 0;

    return result;
}


bool FileStreamWiiNand::SetPosition(off_type position, PositionType positionType)
{
    EA_ASSERT(mnAccessFlags != 0);
    mFilePosition.Seek(position, positionType);
    return true;
}


size_type FileStreamWiiNand::GetAvailable() const
{
    EA_ASSERT(mnAccessFlags != 0);
    return GetSize() - GetPosition();
}


size_type FileStreamWiiNand::Read(void* pData, size_type nSize)
{
    // pData and nSize must be 32-byte aligned. To do: Hide this requirement from the user.
    EA_ASSERT(pData && (((uintptr_t)pData & 31) == 0) && ((nSize & 31) == 0) && mnAccessFlags);

    s32 currentPosition = (s32)mFilePosition.Tell();
    s32 result          = NANDSeek(&mFileInfo.mNandInfo, currentPosition, NAND_SEEK_SET);

    if(result >= 0)
    { 
        result = NANDRead(&mFileInfo.mNandInfo, pData, nSize);

        if(result >= 0)
        {
            mFilePosition.Skip(result);
            return (size_type)result;
        }
    }

    mnLastError = result;
    return kSizeTypeError;
}


bool FileStreamWiiNand::Write(const void* pData, size_type nSize)
{
    // pData must be 32-byte aligned. To do: Hide this requirement from the user.
    EA_ASSERT(pData && (((uintptr_t)pData & 31) == 0) && (mnAccessFlags & kAccessFlagWrite));

    s32 currentPosition = (s32)mFilePosition.Tell();
    s32 result          = NANDSeek(&mFileInfo.mNandInfo, currentPosition, NAND_SEEK_SET);

    if(result >= 0)
    {
        result = NANDWrite(&mFileInfo.mNandInfo, pData, nSize);

        if(result >= 0)
        {
            mFilePosition.Append(result);
            return true;
        }
    }

    mnLastError = result;
    return false;
}


bool FileStreamWiiNand::WriteAsync(const void* pData, size_type nSize, IStreamCallback callback, void* pContext)
{
    // pData must be 32-byte aligned. To do: Hide this requirement from the user.
    EA_ASSERT(!mIsBusy && pData && (((uintptr_t)pData & 31) == 0) && (mnAccessFlags & kAccessFlagWrite));

    mCallback = callback;
    mpContext = pContext;

    s32 currentPosition = (s32)mFilePosition.Tell();
    s32 result          = NANDSeek(&mFileInfo.mNandInfo, currentPosition, NAND_SEEK_SET);

    if(result >= 0)
    {
        mIsBusy = true;
        result  = NANDWriteAsync(&mFileInfo.mNandInfo, pData, nSize, StaticNandAsyncCallback, &mFileInfo.mCommandBlock);

        if(result == NAND_RESULT_OK)
        {
            mFilePosition.Append((s32)nSize);
            return true;
        }
        else
            mIsBusy = false;
    }

    mnLastError = result;
    return false;
}


int32_t FileStreamWiiNand::WaitAsync() const
{
    while(mIsBusy)
    {
        OSSleepMilliseconds(1);

        // I don't think we actually need the following.
        // #define EAIOReadWriteBarrier() __asm__ __volatile__ ("lwsync" : : : "memory")
        // EAIOReadWriteBarrier();
    }

    return mAsyncResult;
}


bool FileStreamWiiNand::Flush()
{
    EA_ASSERT(mnAccessFlags != 0);

    // The NAND file system doesn't support this. Possibly it commits writes as they occur.
    return true;
}


void FileStreamWiiNand::StaticNandAsyncCallback(s32 result, NANDCommandBlock* pCommandBlock)
{
    EA_ASSERT(pCommandBlock);

    FileStreamWiiNand* const pFileStreamWiiNand = ((NandFileStreamInfo*)pCommandBlock)->mpFileStreamWiiNand;
    EA_ASSERT(pFileStreamWiiNand);

    pFileStreamWiiNand->mIsBusy      = false;
    pFileStreamWiiNand->mAsyncResult = result;

    if(pFileStreamWiiNand->mCallback)
        pFileStreamWiiNand->mCallback(result, pFileStreamWiiNand, pFileStreamWiiNand->mpContext);
}


} // namespace IO

} // namespace EA



