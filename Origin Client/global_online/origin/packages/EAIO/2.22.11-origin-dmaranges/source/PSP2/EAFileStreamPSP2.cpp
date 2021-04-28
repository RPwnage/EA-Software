/////////////////////////////////////////////////////////////////////////////
// EAFileStreamPSP2.cpp
//
// Copyright (c) 2010, Electronic Arts Inc. All rights reserved.
// Created by Stephen Kiazyk
//
/////////////////////////////////////////////////////////////////////////////

/*
Issues: (updated Sept. 28, 2010)
Do not mix 32 and 64-bit values:
- The PSP2 in general expects files sizes/offsets to be 64 bits, however the system itself is 32-bit. 
  As a result, its size_t (and thus, its size_type) for the file sizes is by default configured to be 
  32-bit. Hence, it is very important that one ensures that off_type and size_type are the same bit size.
  This has implications on which of the kernel's 'seek' methods to use (see the method 'wrapper_sceIoSeek' 
  for details).

FileStat lag bug:
- There is currently a bug on PSP2 (as of SDK release 9.25) where the filesystem will not correctly update 
  the stat for files as they are being updated.  Hence, I have written the 'GetSize' method to use 
  the old-fashioned 'seek to the end to find out how long the file is' idiom instead of reading the
  file stat. In order to do so, I am circumventing the constness of this object, however I am hoping that
  this will only be a temporary measure until sony fixes this (we have a bug report with them already).
  This only works when the file is open, and hence 'GetSize' might give an incorrect value if
  called on a non-open file handle, if that same file is opened somewhere else.

*/

#include <EAIO/internal/Config.h>
#include <EAIO/EAFileStream.h>
#include <EAIO/EAFileUtil.h>
#include <coreallocator/icoreallocator.h>
#include EA_ASSERT_HEADER
#include <EAIO/FnEncode.h>
#include <kernel.h>
#include <errno.h>

namespace EA
{

namespace IO
{

namespace
{
// Note: the type returned by sceIoLseek, 'SceOff', is 64-bit. 
// As a result, one must be careful to ensure that both 'off_type' AND 'size_type' are
// 64-bit when using this method (e.g. if size_type is 32 and 'off_type' is 64, then the
// sentinel value 'kSizeTypeError' will expand to 4294967296 instead of -1).
// Hence, when in 32-bit mode, we must use sceIoLseek32 to avoid bad casts.

// The purpose of these methods therefore, is to cascade through to the appropriate seek method
// based on whether we are using 32-bit or 64-bit offsets
#if EAIO_64_BIT_SIZE_ENABLED

off_type wrapper_sceIoSeek(SceUID fd, off_type offset, int whence)
{
	return sceIoLseek(fd, offset, whence);
}

#else // in 32-bit mode

off_type wrapper_sceIoSeek(SceUID fd, off_type offset, int whence)
{
	return sceIoLseek32(fd, offset, whence);
}

#endif

}

const SceUID kFileHandleInvalid = -1;

FileStream::FileStream(const char8_t* pPath8)
  : mpReleaseAllocator(NULL),
	mFileHandle(kFileHandleInvalid),
	mnRefCount(0),
	mnAccessFlags(0),
	mnCD(0),
	mnSharing(0),
	mnUsageHints(kUsageHintNone),
	mnLastError(kStateNotOpen)
{
	FileStream::SetPath(pPath8); // Note that in a constructor, the virtual function mechanism is inoperable.
}


FileStream::FileStream(const char16_t* pPath16)
  : mpReleaseAllocator(NULL),
	mFileHandle(kFileHandleInvalid),
	mnRefCount(0),
	mnAccessFlags(0),
	mnCD(0),
	mnSharing(0),
	mnUsageHints(kUsageHintNone),
	mnLastError(kStateNotOpen)
{
	FileStream::SetPath(pPath16); // Note that in a constructor, the virtual function mechanism is inoperable.
}


FileStream::FileStream(const FileStream& fs)
  : mpReleaseAllocator(NULL),
	mFileHandle(kFileHandleInvalid),
	mnRefCount(0),
	mnAccessFlags(0),
	mnCD(0),
	mnSharing(0),
	mnUsageHints(fs.mnUsageHints),
	mnLastError(kStateNotOpen)
{
	FileStream::SetPath(fs.mpPath8); // Note that in a constructor, the virtual function mechanism is inoperable.
}


FileStream::~FileStream()
{
	FileStream::Close(); // Note that in a destructor, the virtual function mechanism is inoperable.
}


FileStream& FileStream::operator=(const FileStream& fs)
{
	if(&fs != this)
	{
		Close();
		SetPath(fs.mpPath8);

		mnAccessFlags = 0;
		mnCD          = 0;
		mnSharing     = 0;
		mnUsageHints  = fs.mnUsageHints;
		mnLastError   = kStateNotOpen;
	}

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

	if (mpReleaseAllocator)
	{
		this->~FileStream();
		mpReleaseAllocator->Free(this);
	}
	else
	{
		delete this;
	}

	return 0;
}


void FileStream::SetPath(const char8_t* pPath8)
{
	if((mFileHandle == kFileHandleInvalid) && pPath8 != NULL)
	{
		EAIOStrlcpy8(mpPath8, pPath8, kMaxPathLength);
	}  
}


void FileStream::SetPath(const char16_t* pPath16)
{
	if((mFileHandle == kFileHandleInvalid) && pPath16 != NULL)
	{
		StrlcpyUTF16ToUTF8(mpPath8, kMaxPathLength, pPath16);
	}  
}


size_t FileStream::GetPath(char8_t* pPath8, size_t nPathLength)
{
	if (pPath8 != NULL)
	{
		return EAIOStrlcpy8(pPath8, mpPath8, nPathLength);
	}
	return 0;
}


size_t FileStream::GetPath(char16_t* pPath16, size_t nPathLength)
{
	if(pPath16 != NULL)
	{
		StrlcpyUTF8ToUTF16(pPath16, nPathLength, mpPath8);
	}
	return 0;
}


bool FileStream::Open(int nAccessFlags, int nCreationDisposition, int nSharing, int nUsageHints)
{
	if((mFileHandle == kFileHandleInvalid) && nAccessFlags) // If not already open and if some kind of access is requested...
	{
		int flags = 0;

		if(nAccessFlags == kAccessFlagRead)
		{
			flags = SCE_O_RDONLY;
		}
		else if(nAccessFlags == kAccessFlagWrite)
		{
			flags = SCE_O_WRONLY;
		}
		else if(nAccessFlags == kAccessFlagReadWrite)
		{
			flags = SCE_O_RDWR;
		}
		else
		{
			EA_FAIL();
		}

		if(nCreationDisposition == kCDDefault)
		{
			if(nAccessFlags == kAccessFlagWrite)
			{
				nCreationDisposition = kCDOpenAlways;
			}
			else
			{
				nCreationDisposition = kCDOpenExisting;
			}
		}

		switch(nCreationDisposition)
		{
			case kCDCreateNew:
			   {
				// Make sure that the file does not exist
				SceIoStat tempStat;
				if (sceIoGetstat(mpPath8, &tempStat) == 0)
					return false;
				}
				flags |= SCE_O_CREAT;       // Open only if it doesn't already exist.
				break;

			case kCDCreateAlways:
				flags |= SCE_O_CREAT;      // Always make it like the file was just created.
				flags |= SCE_O_TRUNC;
				break;

			case kCDOpenExisting:           // Open the file if it exists, else fail.
				break;                      // Nothing to do.

			case kCDOpenAlways:
				flags |= SCE_O_CREAT;      // Open the file no matter what.
				break;

			case kCDTruncateExisting:       // Open the file if it exists, and truncate it if so.
				flags |= SCE_O_TRUNC;
				break;
		}

		mFileHandle = sceIoOpen(mpPath8, flags, SCE_STM_RWU);

		if(mFileHandle < 0) // If it failed...
		{
			mnLastError = mFileHandle;
			mFileHandle = kFileHandleInvalid;
		}
		else
		{
			mnAccessFlags = nAccessFlags;
			mnCD          = nCreationDisposition;
			mnSharing     = nSharing;
			mnUsageHints  = nUsageHints;
			mnLastError   = kStateSuccess;
		}
	}

	return (mFileHandle != kFileHandleInvalid);
}


bool FileStream::Close()
{
	bool success = false;

	if(mFileHandle != kFileHandleInvalid)
	{
		success = (sceIoClose(mFileHandle) == SCE_OK);
 
		mFileHandle  = kFileHandleInvalid;
		mnAccessFlags = 0;
		mnCD          = 0;
		mnSharing     = 0;
		mnUsageHints  = 0;
		mnLastError   = kStateNotOpen;
	}

	return success;
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
	// HACK HACK HACK
	// Well, sort of, its just that due to a hardware bug, getting the file-size via the
	// stat method in the sce kernel doesn't work while the file is still opened, hence
	// we have to do this the old-fashioned way.  However, this should be removed once
	// the bug is resolved, since the way I'm doing things right now is technically 
	// violating this object's const-ness
	if (mFileHandle != kFileHandleInvalid)
	{
		off_type savedPosition = this->GetPosition();

		// the 'seek to the end of file to get its size' trick
		off_type fileSize = wrapper_sceIoSeek(mFileHandle, 0, SCE_SEEK_END);

		// revert the file pointer back to the start of the file
		off_type reSeek = wrapper_sceIoSeek(mFileHandle, savedPosition, SCE_SEEK_SET);
		
		EA_ASSERT( reSeek == savedPosition );

		return fileSize;
	}
	// HACK HACK HACK

	return File::GetSize(mpPath8);
}


bool FileStream::SetSize(size_type size)
{
	if(mFileHandle != kFileHandleInvalid)
	{
		if (this->GetSize() != size)
		{
			off_type oldPosition = this->GetPosition();

			int closeResult = sceIoClose(mFileHandle);

			EA_ASSERT(closeResult == SCE_OK);

			mFileHandle = kFileHandleInvalid;
			mnLastError = kStateNotOpen;

			SceIoStat newStat;
			memset(static_cast<void*>(&newStat), 0, sizeof(SceIoStat));

			newStat.st_size = static_cast<int64_t>(size);

			int changeResult = sceIoChstat(mpPath8, &newStat, SCE_CST_SIZE);

			int savedCd = mnCD;

			// re-open the file (however, ignore the creation disposition, as it may truncate the file)
			bool opened = this->Open(mnAccessFlags, kCDOpenExisting, mnSharing, mnUsageHints);

			mnCD = savedCd;

			if (opened)
			{
				mnLastError = changeResult;

				if (oldPosition < size)
				{
					this->SetPosition(oldPosition);
				}
			}
		}
	}

	return (mnLastError == kStateSuccess);
}


off_type FileStream::GetPosition(PositionType positionType) const
{
	off_type result = (off_type)kSizeTypeError;

	if(mFileHandle == kFileHandleInvalid)
	{
		return result;
	}

	if (positionType == kPositionTypeCurrent)
	{
		return 0;
	}

	const off_type filePos = wrapper_sceIoSeek(mFileHandle, 0, SCE_SEEK_CUR);
	
	if (filePos >= 0)
	{
		if (positionType == kPositionTypeBegin)
			return filePos;

		if (positionType == kPositionTypeEnd)
		{
			const off_type fileSize = (off_type)this->GetSize();

			if (fileSize >= 0)
			{
				result = filePos - fileSize;
			}
			else
			{
				mnLastError = fileSize;
			}
		}
	}
	else
	{
		mnLastError = filePos;
	}

	return result;
}


bool FileStream::SetPosition(off_type position, PositionType positionType)
{
	if(mFileHandle != kFileHandleInvalid)
	{
		int seekType;

		switch(positionType)
		{
			case kPositionTypeBegin:
				seekType = SCE_SEEK_SET;
				break;

			case kPositionTypeCurrent:
				seekType = SCE_SEEK_CUR;
				break;

			case kPositionTypeEnd:
				seekType = SCE_SEEK_END;
				break;

			default:
				seekType = SCE_SEEK_SET;
		}

		const off_type result = wrapper_sceIoSeek(mFileHandle, position, seekType);

		if(result >= 0)
		{
			mnLastError = kStateSuccess;
		}
		else
		{
			mnLastError = result;
		}
	}

	return (mnLastError == kStateSuccess);
}


size_type FileStream::GetAvailable() const
{
	size_type result = kSizeTypeError;
	const off_type nPosition = GetPosition(kPositionTypeEnd);

	if (nPosition != (off_type)kSizeTypeError)
	{
		result = (size_type)-nPosition;
	}

	return result;
}


void FileStream::SetReleaseAllocator(EA::Allocator::ICoreAllocator* allocator)
{
	mpReleaseAllocator = allocator;
}


EA::Allocator::ICoreAllocator* FileStream::GetReleaseAllocator() const
{
	return mpReleaseAllocator;
}


size_type FileStream::Read(void* pData, size_type nSize)
{
	size_type result = kSizeTypeError;

	if(mFileHandle != kFileHandleInvalid)
	{
		SceSSize amountRead = sceIoRead(mFileHandle, pData, nSize);

		if (amountRead >= 0)
		{
			result = amountRead;
			mnLastError = kStateSuccess;
		}
		else
		{
			mnLastError = amountRead;
		}
	}

	return result;
}


bool FileStream::Write(const void* pData, size_type nSize)
{
	if(mFileHandle != kFileHandleInvalid)
	{
		SceSSize amountWritten = 0;
		SceSSize writeResult = 0;
		
		do
		{
			// I'm assuming that the sce kernel is basically a posix implementation, and hence has the
			// 'write won't neccessarily write everything' problem as well
			writeResult = sceIoWrite(mFileHandle, static_cast<const char*>(pData) + amountWritten, nSize - amountWritten);

			if (writeResult >= 0)
			{
				amountWritten += writeResult;
			}
		}
		while(writeResult >= 0 && amountWritten < nSize);

		mnLastError = ((writeResult >= 0) && (amountWritten == nSize)) ? kStateSuccess : writeResult;
	}

	return (mnLastError == kStateSuccess);
}


bool FileStream::Flush()
{
	if(mFileHandle != kFileHandleInvalid)
	{
		// There doesn't seem to be any reasonable way to do this at the moment
		return true;
	}

	return false;
}


} // namespace IO


} // namespace EA
