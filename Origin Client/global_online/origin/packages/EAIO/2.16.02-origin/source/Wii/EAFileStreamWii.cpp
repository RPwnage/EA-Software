/////////////////////////////////////////////////////////////////////////////
// EAFileStreamWii.cpp
//
// Copyright (c) 2003, Electronic Arts Inc. All rights reserved.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////

#if defined(EA_PLATFORM_REVOLUTION)


#include <EAIO/internal/Config.h>
#include <EAIO/EAFileStream.h>
#include <EAIO/FnEncode.h>
#include <EAStdC/EAString.h>
#include <coreallocator/icoreallocator_interface.h>
#include <revolution/dvd.h>
#include EA_ASSERT_HEADER


static const u32 kFilePositionInvalid = 0;

#define EAFS_MIN(a,b)          ((a) < (b) ? (a) : (b))
#define kWindowMask            (mWindowSize - 1)
#define kWindowPositionInvalid ((uint32_t)(0 - mWindowSize))


namespace EA
{

namespace IO
{

namespace Internal
{

    static bool ShrinkFile(const char* pPath8, DVDFileInfo& dvdFileInfo, uint32_t newLength)
    {
        // Due to the sparseness of the DVD API, we are stuck doing this  
        // somewhat tedious operation to shrink a file:
        //     1 Make a temp file.
        //     2 Copy mFileInfo.length bytes of this file to the temp file.
        //     3 Close and delete this file.
        //     4 Create a new file for this.
        //     5 Copy temp file to the new file for this.
        //     6 Delete the temp file.

        // To do: Implement this. This is not an option; it must be done.
        (void)pPath8; (void)dvdFileInfo; (void)newLength;
        EA_FAIL_MESSAGE("ShrinkFile: Not implemented."); 

        return false;
    }
}


FileStream::FileStream(const char8_t* pPath8)
  : mCurrentPosition(0),
    mnRefCount(0),
    mnAccessFlags(0),
    mnCD(0),
    mnSharing(0),
    mnUsageHints(kUsageHintNone),
    mnLastError(kStateNotOpen),
    mWindowPosition(kWindowPositionInvalid),
    mWindowSize(kWindowSizeDefault),
    mbWindowWasWritten(0),
    mbSizeWasDecreased(0),
    mWindow(NULL)
{
    memset(&mFileInfo, 0, sizeof(mFileInfo));
    FileStream::SetPath(pPath8); // Note that in a constructor, the virtual function mechanism is inoperable.
}


FileStream::FileStream(const char16_t* pPath16)
  : mCurrentPosition(0),
    mnRefCount(0),
    mnAccessFlags(0),
    mnCD(0),
    mnSharing(0),
    mnUsageHints(kUsageHintNone),
    mnLastError(kStateNotOpen),
    mWindowPosition(kWindowPositionInvalid),
    mWindowSize(kWindowSizeDefault),
    mbWindowWasWritten(0),
    mbSizeWasDecreased(0),
    mWindow(NULL)
{
    memset(&mFileInfo, 0, sizeof(mFileInfo));
    FileStream::SetPath(pPath16); // Note that in a constructor, the virtual function mechanism is inoperable.
}


FileStream::FileStream(const FileStream& fs)
  : mCurrentPosition(0),
    mnRefCount(0),
    mnAccessFlags(0),
    mnCD(0),
    mnSharing(0),
    mnUsageHints(fs.mnUsageHints),
    mnLastError(kStateNotOpen),
    mWindowPosition(kWindowPositionInvalid),
    mbWindowWasWritten(0),
    mbSizeWasDecreased(0),
    mWindow(NULL)
{
    memset(&mFileInfo, 0, sizeof(mFileInfo));
    FileStream::SetPath(fs.mpPath8); // Note that in a constructor, the virtual function mechanism is inoperable.
}


FileStream::~FileStream()
{
    Close();
    FileStream::Close(); // Note that in a destructor, the virtual function mechanism is inoperable.
}


FileStream& FileStream::operator=(const FileStream& fs)
{
    Close();

    memset(&mFileInfo, 0, sizeof(mFileInfo));
    mCurrentPosition   = 0;
    mnAccessFlags      = 0;
    mnCD               = 0;
    mnSharing          = 0;
    mnUsageHints       = fs.mnUsageHints;
    mnLastError        = kStateNotOpen;
    mWindowPosition    = kWindowPositionInvalid;
    mbWindowWasWritten = 0;
    mbSizeWasDecreased = 0;
    SetPath(fs.mpPath8);

    return *this;
}


int FileStream::AddRef()
{
    return ++mnRefCount;
}


int FileStream::Release()
{
    if(mnRefCount > 1)
        return --mnRefCount;
    delete this;
    return 0;
}


void FileStream::SetPath(const char8_t* pPath8)
{
    if((mFileInfo.startAddr == kFilePositionInvalid) && pPath8)
        EA::StdC::Strlcpy(mpPath8, pPath8, kMaxPathLength);
}


void FileStream::SetPath(const char16_t* pPath16)
{
    if((mFileInfo.startAddr == kFilePositionInvalid) && pPath16)
        EA::StdC::Strlcpy(mpPath8, pPath16, kMaxPathLength);
}


size_t FileStream::GetPath(char8_t* pPath8, size_t nPathLength)
{
    // Return the length of the path we -tried- to create.
    return EA::StdC::Strlcpy(pPath8, mpPath8, nPathLength);
}


size_t FileStream::GetPath(char16_t* pPath16, size_t nPathLength)
{
    if(pPath16 && mpPath8)
        return EA::StdC::Strlcpy(pPath16, mpPath8, nPathLength);

    // In this case the user wants just the length of the return value.
    char16_t pathTemp[kMaxPathLength];
    return GetPath(pathTemp, kMaxPathLength);
}


bool FileStream::Open(int nAccessFlags, int nCreationDisposition, int nSharing, int nUsageHints)
{
    // The GameCube and Revolution have a somewhat primitive file system 
    // whereby it's more or less assumed that the only device you will
    // be working with is a DVD drive. The concept of file access flags
    // doesn't exist. The 'ethernet' DVD system allows for a network drive
    // emulation of a DVD, including file creation, writing, and removal.

    if ((mFileInfo.startAddr == kFilePositionInvalid) && nAccessFlags) // If not already open and if some kind of access is requested...
    {
        const int32_t entryNum = DVDConvertPathToEntrynum(mpPath8);

        if(nCreationDisposition == kCDDefault)
        {
            if(nAccessFlags & kAccessFlagWrite)
                nCreationDisposition = kCDOpenAlways;
            else
                nCreationDisposition = kCDOpenExisting;
        }

        switch(nCreationDisposition)
        {
            case kCDOpenExisting: // Fails if file doesn't exist, keeps contents.
            case kCDTruncateExisting: // Fails if file doesn't exist, but truncates to 0 if it does.
            {
                if(entryNum == -1) // If the file doesn't exist...
                {
                    mnLastError = kStateError;
                    return false;
                }

                break;
            }

            case kCDCreateNew: // Fails if file already exists.
            {
                EA_ASSERT((nAccessFlags & kAccessFlagWrite) != 0);

                if(entryNum != -1) // If the file already exists...
                {
                    mnLastError = kStateError;
                    return false;
                }

                break;
            }
        }

        // Note that we don't make a distinction between reading and writing. This is because
        // in shipping titles there will only ever be reading. We support writing only for 
        // the case of DVDETH (DVD over ethernet) being supported. And in fact DVDETH is no 
        // longer supported by Nintendo. You need to use EAFileStreamHio2 instead.
        BOOL bResult = DVDOpen(mpPath8, &mFileInfo);

        #ifdef DVDETH
            if(bResult == 0) // If the open attempt failed...
            {
                if((nCreationDisposition == kCDCreateAlways) || 
                   (nCreationDisposition == kCDOpenAlways))
                {
                    bResult = DVDCreate(mpPath8, &mFileInfo);
                }
            }
        #endif

        if(bResult) // If it succeeded...
        {
            mCurrentPosition   = 0;
            mnAccessFlags      = nAccessFlags;
            mnCD               = nCreationDisposition;
            mnSharing          = nSharing;
            mnUsageHints       = nUsageHints;
            mnLastError        = 0;
            mWindowPosition    = kWindowPositionInvalid;
            mbWindowWasWritten = 0;

            if((nAccessFlags & kAccessFlagWrite) &&
                  ((nCreationDisposition == kCDCreateAlways) || (nCreationDisposition == kCDTruncateExisting)))
            {
                // In this case we need to truncate the file.
                SetSize(0);
            }

            EA_ASSERT_MSG(mWindow == NULL, "Filestream buffer is non-null - stream is already open or a previous Close() didn't do a good job!");
            mWindow = (char*)IO::GetAllocator()->Alloc(mWindowSize, "FileStream/mWindow", 0, 32);
        }
        else
        {
            // mCurrentPosition   = 0;
            // mnAccessFlags      = 0; // Not necessary, as these should already be zeroed.
            // mnCD               = 0;
            // mnSharing          = 0;
            // mnUsageHints       = 0;
            // mWindowPosition    = kWindowPositionInvalid;
            // mbWindowWasWritten = 0;
            #ifdef DVDETH
                mnLastError = kStateError;
            #else
                mnLastError = kFSErrorWriteProtect; // The DVD file system cannot be written to.
            #endif
        }
    }

    return (mFileInfo.startAddr != kFilePositionInvalid);
}


bool FileStream::Close()
{
    if (mFileInfo.startAddr != kFilePositionInvalid)
    {
        const BOOL bResult = DVDClose(&mFileInfo);
        EA_ASSERT(bResult != 0);

        if((mnAccessFlags & kAccessFlagWrite) && mbSizeWasDecreased)
        {
            DVDFileInfo dvdFileInfo;
 
            if(DVDOpen(mpPath8, &dvdFileInfo))
            {
                EA_ASSERT(mFileInfo.length <= dvdFileInfo.length);

                if(mFileInfo.length < dvdFileInfo.length) // If we need to revise the system's view of the size...
                    Internal::ShrinkFile(mpPath8, dvdFileInfo, mFileInfo.length);

                DVDClose(&dvdFileInfo);
            }
        }

        memset(&mFileInfo, 0, sizeof(mFileInfo));
        mnAccessFlags       = 0;
        mnCD                = 0;
        mnSharing           = 0;
        mnUsageHints        = 0;
        mnLastError         = kStateNotOpen;
        mWindowPosition     = kWindowPositionInvalid;
        mbWindowWasWritten  = 0;
        mbSizeWasDecreased  = 0;

        if (mWindow)
        {
            IO::GetAllocator()->Free(mWindow);
            mWindow = NULL;
        }
    }

    return true;
}


void FileStream::SetWindowSize(uint32_t size)
{
    EA_ASSERT((size >= 512) && ((size & (size - 1)) == 0));
    mWindowSize = size;
}

int FileStream::GetAccessFlags() const
{
    return mnAccessFlags;
}


int FileStream::GetState() const
{
    return mnLastError;
}


size_type FileStream::GetSize() const
{
    if(mFileInfo.startAddr != kFilePositionInvalid)
    {
        mnLastError = kStateSuccess;

        return (size_type)mFileInfo.length;
    }

    return kSizeTypeError;
}


bool FileStream::SetSize(size_type size)
{
    if(mFileInfo.startAddr != kFilePositionInvalid)
    {
        EA_ASSERT((mnAccessFlags & kAccessFlagWrite) != 0);

        if((mnAccessFlags & kAccessFlagWrite) != 0)
        {
            // There isn't a way to do this, at least not in any direct way.
            // If the size is an increase, then we can attempt to write data
            // until the desired size. If the size is a decrease, we can 
            // shrink the file via a copy to a new file then delete this file
            // and copy the old file to this file. That is a pretty undesirable
            // solution, needless to say. Luckily, it would not happen in a 
            // shipping application, as all DVD files are then read-only.

            if(size > (size_type)mFileInfo.length) // If increasing in size...
            {
                const off_type savedPosition = GetPosition();

                if(SetPosition((off_type)mFileInfo.length)) // Go to the end of the file.
                {
                    char buffer[512]; // No need to initialize this with anything.

                    #ifdef EA_DEBUG
                        memset(buffer, 0xfe, sizeof(buffer));
                    #endif

                    while(mFileInfo.length < size)
                    {
                        const size_type writeSize = EAFS_MIN(size - mFileInfo.length, sizeof(buffer));

                        if(Write(buffer, writeSize))
                            mFileInfo.length += writeSize;
                        else
                            break;
                    }
                }

                SetPosition(savedPosition);

                return (mFileInfo.length == size);
            }
            else if(size < (size_type)mFileInfo.length) // If decreasing in size...
            {
                // What we do here is flush the current window and then 
                // change mFileInfo.length to be the shorter size. However, 
                // the file system driver still thinks that it's some other size.
                // So before closing this file we will need to update the system's
                // view of what the file size is. Since there isn't a file size
                // setting function available, we'll need to do something like 
                // copy the file to a temp file and shorten it to our desired size.

                if(FlushWindow())
                {
                    mbSizeWasDecreased = 1;
                    mFileInfo.length   = size;

                    if(mCurrentPosition > size)
                        mCurrentPosition = size;
                }
            }
        }
    }

    return false;
}


off_type FileStream::GetPosition(PositionType positionType) const
{
    off_type result;

    EA_ASSERT(mFileInfo.startAddr != kFilePositionInvalid);

    if(positionType == kPositionTypeBegin)
        result = (off_type)mCurrentPosition;
    else if(positionType == kPositionTypeEnd)
        result = (off_type)mCurrentPosition - (off_type)mFileInfo.length;
    else // Else the position relative to current, which is by definition zero.
        result = 0;

    return result;
}


bool FileStream::SetPosition(off_type position, PositionType positionType)
{
    if(mFileInfo.startAddr != kFilePositionInvalid)
    {
        size_type newPosition;

        switch(positionType)
        {
            default:
            case kPositionTypeBegin:
                newPosition = (size_type)position;
                break;

            case kPositionTypeCurrent:
                newPosition = (size_type)((off_type)mCurrentPosition + position);
                break;

            case kPositionTypeEnd:
                newPosition = (size_type)((off_type)mFileInfo.length + position);
                break;
        }

        if(newPosition <= (size_type)mFileInfo.length)
        {
            mCurrentPosition = newPosition;
            return true;
        }

        mnLastError = kStateError;
    }

    return false;
}


size_type FileStream::GetAvailable() const
{
    EA_ASSERT(mnAccessFlags != 0);

    return (size_type)(mFileInfo.length - mCurrentPosition);
}


bool FileStream::FillWindow(uint32_t position)
{
    EA_ASSERT((mWindowPosition & kWindowMask) == 0);

    const uint32_t nNewWindowPosition = (position & (~kWindowMask));

    if (nNewWindowPosition != mWindowPosition) // If we need to fill a new window...
    {
        if(FlushWindow()) // Write data to disk if necessary.
        {
            s32 fillSize = mWindowSize;
            if ((nNewWindowPosition + fillSize) > mFileInfo.length)
                  fillSize = (mFileInfo.length - nNewWindowPosition);

            s32 result = DVDRead(&mFileInfo, mWindow, OSRoundUp32B(fillSize), (s32)nNewWindowPosition);
            if(result >= 0)
                mWindowPosition = nNewWindowPosition;

            return (result >= 0);
        }
    }

    return true;
}


bool FileStream::FlushWindow()
{
#ifdef DVDETH
    EA_ASSERT((mWindowPosition & kWindowMask) == 0);

    if(mbWindowWasWritten)
    {
        EA_ASSERT((mnAccessFlags & kAccessFlagWrite) != 0);

        const BOOL bResult = DVDWrite(&mFileInfo, (char *)mWindow, (s32)mWindowSize, (s32)mWindowPosition);

        if(bResult != 0)
            mbWindowWasWritten = 0;

        return (bResult != 0);
    }

    return true;

#else

    return true;

#endif
}


size_type FileStream::Read(void* pData, size_type nSize)
{
    size_type returnValue = kSizeTypeError;

    if(mFileInfo.startAddr != kFilePositionInvalid)
    {
        EA_ASSERT((mnAccessFlags & kAccessFlagRead) != 0);

        const uint32_t endPosition = EAFS_MIN(mFileInfo.length, mCurrentPosition + nSize);

        if(endPosition > mCurrentPosition) // If there is anything to read...
        {
            char*    pTempData    = (char*)pData;
            uint32_t tempPosition = mCurrentPosition;
            bool     result       = true;

            while(result && (tempPosition < endPosition))
            {
                result = FillWindow(tempPosition);

                if(result)
                {
                    const uint32_t copySize = EAFS_MIN(endPosition, mWindowPosition + mWindowSize) - tempPosition;

                    memcpy(pTempData, mWindow + (tempPosition - mWindowPosition), copySize);

                    pTempData    += copySize;
                    tempPosition += copySize;
                }
            }

            if(tempPosition == endPosition) // If the entire read was successful...
            {
                returnValue      = (size_type)(endPosition - mCurrentPosition);
                mCurrentPosition = endPosition;
            }
        }
    }

    return returnValue;
}


bool FileStream::Write(const void* pData, size_type nSize)
{
#ifdef DVDETH
    if(mFileInfo.startAddr != kFilePositionInvalid)
    {
        EA_ASSERT((mnAccessFlags & kAccessFlagWrite) != 0);

        const uint32_t endPosition  = mCurrentPosition + nSize;
        const char*    pTempData    = (const char*)pData;
        uint32_t       tempPosition = mCurrentPosition;
        bool           result       = true;

        while(result && (tempPosition < endPosition))
        {
            // If we have an existing dirty window and it it's at a 
            // position outside where we want to write now, flush it.
            if(mbWindowWasWritten && ((mCurrentPosition & kWindowMask) != mWindowPosition))
                result = FlushWindow();

            if(result)
            {
                const uint32_t copySize = EAFS_MIN(endPosition, mWindowPosition + mWindowSize) - tempPosition;

                if(copySize == mWindowSize) // If we are writing an entire window... bypass our window buffer and directly write it.
                {
                    // For maximal efficiency, we want to avoid reading a window if all
                    // we're going to do is turn around and write the entire window.
                    if(DVDWrite(&mFileInfo, (char*)pTempData, (s32)mWindowSize, (s32)mWindowPosition) == 0)
                        result = false;
                }
                else
                {
                    result = FillWindow(tempPosition);
                    
                    if(result)
                    {
                        memcpy(((char *)mWindow) + (tempPosition - mWindowPosition), pTempData, copySize);
                        mbWindowWasWritten = true;
                    }
                }

                if(result)
                {
                    pTempData        += copySize;
                    tempPosition     += copySize;
                    mCurrentPosition += copySize;
                }
            }
        }

        if(mFileInfo.length < mCurrentPosition)
            mFileInfo.length = mCurrentPosition;

        if(tempPosition == endPosition) // If the entire write was successful...
            return true;
    }
#endif

    return false;
}


bool FileStream::Flush()
{
    EA_ASSERT(mnAccessFlags != 0);

    return FlushWindow();
}


} // namespace IO


} // namespace EA


#undef EAFS_MIN
#undef kWindowMask
#undef kWindowPositionInvalid


#endif // (platform check)

