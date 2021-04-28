/////////////////////////////////////////////////////////////////////////////
// EAFileStreamWiiHio2.cpp
//
// Copyright (c) 2007 Criterion Software Limited and its licensors. All Rights Reserved.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#include <EAIO/internal/Config.h>
#include <EAIO/Wii/EAFileStreamWiiHio2.h>
#include <EAIO/Wii/MCSFile.h>
#include <EAStdC/EAString.h>
#include <coreallocator/icoreallocator_interface.h>
#include EA_ASSERT_HEADER




namespace EA
{

namespace IO
{

namespace Internal
{

}


FileStreamWiiHio2::FileStreamWiiHio2(const char8_t* pPath8)
{
    Reset();
    FileStreamWiiHio2::SetPath(pPath8); // Note that in a constructor, the virtual function mechanism is inoperable.
}


FileStreamWiiHio2::FileStreamWiiHio2(const char16_t* pPath16)
{
    Reset();
    FileStreamWiiHio2::SetPath(pPath16); // Note that in a constructor, the virtual function mechanism is inoperable.
}


FileStreamWiiHio2::FileStreamWiiHio2(const FileStreamWiiHio2& fs)
{
    Reset();
    FileStreamWiiHio2::SetPath(fs.mpPath8); // Note that in a constructor, the virtual function mechanism is inoperable.
}


FileStreamWiiHio2::~FileStreamWiiHio2()
{
    FileStreamWiiHio2::Close(); // Note that in a destructor, the virtual function mechanism is inoperable.
}


FileStreamWiiHio2& FileStreamWiiHio2::operator=(const FileStreamWiiHio2& fs)
{
    Close();
    // Assignments here
    SetPath(fs.mpPath8);

    return *this;
}


int FileStreamWiiHio2::AddRef()
{
    return ++mnRefCount;
}


int FileStreamWiiHio2::Release()
{
    if(mnRefCount > 1)
        return --mnRefCount;
    delete this;
    return 0;
}


void FileStreamWiiHio2::Reset()
{
    memset(&mFileInfo, 0, sizeof(mFileInfo));
    mnRefCount       = 0;
    mnAccessFlags    = 0;
    mnCD             = 0;
    mnSharing        = 0;
    mnUsageHints     = kUsageHintNone;
    mnLastError      = kStateNotOpen;
}


void FileStreamWiiHio2::SetPath(const char8_t* pPath8)
{
    if(!mnAccessFlags && pPath8)
        EA::StdC::Strlcpy(mpPath8, pPath8, kMaxPathLength);
}


void FileStreamWiiHio2::SetPath(const char16_t* pPath16)
{
    if(!mnAccessFlags && pPath16)
        EA::StdC::Strlcpy(mpPath8, pPath16, kMaxPathLength);
}


size_t FileStreamWiiHio2::GetPath(char8_t* pPath8, size_t nPathLength)
{
    // Return the length of the path we -tried- to create.
    return EA::StdC::Strlcpy(pPath8, mpPath8, nPathLength);
}


size_t FileStreamWiiHio2::GetPath(char16_t* pPath16, size_t nPathLength)
{
    if(pPath16 && mpPath8)
        return EA::StdC::Strlcpy(pPath16, mpPath8, nPathLength);

    // In this case the user wants just the length of the return value.
    char16_t pathTemp[kMaxPathLength];
    return GetPath(pathTemp, kMaxPathLength);
}


bool FileStreamWiiHio2::Open(int nAccessFlags, int nCreationDisposition, int nSharing, int /*nUsageHints*/)
{
    if(!mnAccessFlags && nAccessFlags) // If not already open and if some kind of access is requested...
    {
        uint32_t openFlags = 0;

        // It is convenient that the Wii read and write flags are identical to the EAIO read and write flags.
        EA_COMPILETIME_ASSERT((kAccessFlagRead  == FILEIO_FLAG_READ) && (kAccessFlagWrite == FILEIO_FLAG_WRITE));

        if(nAccessFlags & kAccessFlagRead)
            openFlags |= FILEIO_FLAG_READ;
        if(nAccessFlags & kAccessFlagWrite)
            openFlags |= FILEIO_FLAG_WRITE;

        if(nSharing & kShareRead)
            openFlags |= FILEIO_FLAG_SHAREREAD;
        if(nSharing & kShareWrite)
            openFlags |= FILEIO_FLAG_SHAREWRITE;

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
                openFlags |= FILEIO_FLAG_NOCREATE;
                break;
            }

            case kCDCreateNew: // Fails if file already exists.
            {
                EA_ASSERT((mnAccessFlags & kAccessFlagWrite) != 0);

                // This doesn't actually work, at least not with the current MCSServer:
                openFlags |= FILEIO_FLAG_NOOVERWRITE;

                // Since there is no way to get this to work via Open flags, we manually test for the file's existence synchronously here.
                FileInfo fileInfo;
                FindData findData;

                const u32 result = FileIO_FindFirst(&fileInfo, &findData, mpPath8);

                if(result == 0)  // If found...
                {
                    FileIO_FindClose(&fileInfo);
                    return false; // The file already exists, so fail the operation.
                }

                break;
            }
        }

        // We don't specify FILEIO_FLAG_CREATEDIR, as that would be inconsistent with 
        // other platforms/file systems, though it is convenient.

        const u32 result = FileIO_Open(&mFileInfo, mpPath8, openFlags);

        if(result == FILEIO_ERROR_SUCCESS)
        {
            mnAccessFlags = nAccessFlags;

            const u32 size = FileIO_GetOpenFileSize(&mFileInfo);
            mFilePosition.SetFileSize(size);

            if((nAccessFlags & kAccessFlagWrite) &&
               ((nCreationDisposition == kCDCreateAlways) || (nCreationDisposition == kCDTruncateExisting)))
            {
                // In this case we need to truncate the file.
                SetSize(0);
            }

            return true;
        }
        else
            mnLastError = (int)(unsigned)result;
    }

    return false;
}


bool FileStreamWiiHio2::Close()
{
    if(mnAccessFlags)
    {
        FileIO_Close(&mFileInfo);
        memset(&mFileInfo, 0, sizeof(mFileInfo));
        mnAccessFlags = 0;
        mnLastError   = kStateNotOpen;
    }

    return true;
}


int FileStreamWiiHio2::GetAccessFlags() const
{
    return mnAccessFlags;
}


int FileStreamWiiHio2::GetState() const
{
    return mnLastError;
}


size_type FileStreamWiiHio2::GetSize() const
{
    EA_ASSERT(mnAccessFlags != 0);
    return mFilePosition.GetFileSize();
}


bool FileStreamWiiHio2::SetSize(size_type size)
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
        // Currently the MCS file system doesn't appear to allow truncating.
        // The best we can do is create a new truncated file and write our file
        // data to it, but even that isn't directly supported by MCS.
        // EA_FAIL();
        SetPosition((off_type)size);
    }
    else
        bReturnValue = true;

    return bReturnValue;
}


off_type FileStreamWiiHio2::GetPosition(PositionType positionType) const
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


bool FileStreamWiiHio2::SetPosition(off_type position, PositionType positionType)
{
    EA_COMPILETIME_ASSERT((kPositionTypeBegin   == FILEIO_SEEK_BEGIN)   && 
                          (kPositionTypeCurrent == FILEIO_SEEK_CURRENT) && 
                          (kPositionTypeEnd     == FILEIO_SEEK_END));
    EA_ASSERT(mnAccessFlags != 0);

    const u32 result = FileIO_Seek(&mFileInfo, position, positionType, NULL);

    if(result == FILEIO_ERROR_SUCCESS)
    {
        mFilePosition.Seek(position, positionType);
        return true;
    }

    mnLastError = (int)(unsigned)result;
    return false;
}


size_type FileStreamWiiHio2::GetAvailable() const
{
    EA_ASSERT(mnAccessFlags != 0);
    return GetSize() - GetPosition();
}


size_type FileStreamWiiHio2::Read(void* pData, size_type nSize)
{
    EA_ASSERT(pData && (mnAccessFlags & kAccessFlagRead));

    size_type returnValue = kSizeTypeError;
    u32       readBytes   = 0;
    const u32 result      = FileIO_Read(&mFileInfo, pData, nSize, &readBytes);
    
    if(result == FILEIO_ERROR_SUCCESS)
    {
        mFilePosition.Skip((off_type)readBytes);
        returnValue = readBytes;
    }
    else
        mnLastError = (int)(unsigned)result;

    return returnValue;
}


bool FileStreamWiiHio2::Write(const void* pData, size_type nSize)
{
    EA_ASSERT(pData && (mnAccessFlags & kAccessFlagWrite));
    
    const u32 result = FileIO_Write(&mFileInfo, pData, nSize);
    
    if(result == FILEIO_ERROR_SUCCESS)
    {
        mFilePosition.Append((off_type)nSize);
        return true;
    }
    
    mnLastError = (int)(unsigned)result;
    return false;

}


bool FileStreamWiiHio2::Flush()
{
    EA_ASSERT(mnAccessFlags != 0);

    // This is not currently directly implemented by the MCS server.
    // However, we may be able to fake it via forced polling.
    //
    // for(int i = 0; (i < 8) && (Mcs_Polling() == 0); ++i)
    //    OSSleepMilliseconds(8);

    return false;
}


} // namespace IO

} // namespace EA



