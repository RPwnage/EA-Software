/////////////////////////////////////////////////////////////////////////////
// EAFileStreamPS3.cpp
//
// Copyright (c) 2005, Electronic Arts Inc. All rights reserved.
// Created by Paul Pedriana
//
/////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PS3 device table
//
// Sony Device     rwfilesys    Driver      Read MB/s   Write MB/s      Note
// ------------------------------------------------------------------------------------------------------------------------
// /dev_bdvd       bdvd:        discfs       8.69       N/A             bd-re
// /dev_bdvd       bdvd:        discfs       8.58       N/A             bd-rom
// /dev_hdd0       hdd0:        cfs         51.84       31.77           DECR Game Data
// /dev_hdd1       hdd1:        fat         45.76       15.05           DECR Disc Cache
// /dev_hdd0       hdd0:        cfs         30.51       29.929          DEX 20GB w/ Linux installed
// /dev_hdd1       hdd1:        fat         27.745      27.431          DEX 20GB w/ Linux installed
// /dev_hdd0       hdd0:        cfs         31.228      33.612          DEX 60GB
// /dev_hdd1       hdd1:        fat         22.717      23.352          DEX 60GB
// /app_home       host1:       hostfs       4.48        3.8            
// /dev_bdvd       bdvd:        discfs       8.31       N/A             emulator
// /dev_bdvd       bdvd:        discfs       5.05       N/A             dvd-r inside
// /dev_ms         ms:          fat         12.25        1.33           1 GB Pro; mounts when you put the memory card in.
// /dev_sd         sd:          fat          5.21        0.14           256 MB SD; mounts when you put the memory card in.
// /dev_cf         cf:          fat          1.16        0.3            16 MB
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



#include <EAIO/internal/Config.h>
#include <EAIO/EAFileStream.h>
#include <coreallocator/icoreallocator.h>
#include <EAStdC/EAString.h>
#include <EAIO/FnEncode.h>
#include EA_ASSERT_HEADER

#if defined(EA_PLATFORM_PS3)
	#include <cell/cell_fs.h>
	#include <sdk_version.h>
	#include <sys/timer.h>
	#include <sys/fs_external.h>
#endif


namespace EA
{

namespace IO
{


// Note from Sony regarding retries:
//    Starting with SDK Release 2.1.0 (CELL_SDK_VERSION >= 0x210001), the system will 
//    execute a sufficient number of retries internally and only then will it return CELL_FS_EIO. 
//    It is now unnecessary for the application to execute a retry. If an I/O error returns when 
//    attempting to read data from the disc, the application can now handle this as an unrecoverable 
//    event and stop game progress, and simply wait for the user to trigger the game termination 
//    request event. It is not necessary to notify the user of the error."
//
ErrorResponse PS3DriveWait(const FileErrorInfo& fileErrorInfo)
{
	ErrorResponse response = kErrorResponseRetry;
	int           result   = CELL_FS_ENOTMOUNTED;

	while((response == kErrorResponseRetry) && (result != CELL_FS_SUCCEEDED))
	{
		// Call the user error handler.
		ErrorHandlingFunction pEHF;
		void* pContext;
		GetFileErrorHandler(pEHF, pContext);

		response = (*pEHF)(fileErrorInfo, pContext);

		if(response == kErrorResponseRetry)
		{
			// Wait for a small amount of time.
			sys_timer_sleep(1);

			// Check the status of the drive.
			CellFsStat status;
			result = cellFsStat("/dev_bdvd", &status);
		}
	}

	return response;
}


const int kFileHandleInvalid = -1;


FileStream::FileStream(const char8_t* pPath8)
  : IStream(),
	mpReleaseAllocator(NULL),
	mnFileHandle(kFileHandleInvalid),
	mnRefCount(0),
	mnAccessFlags(0),
	mnCD(0),
	mnSharing(0),
	mnLastError(kStateNotOpen)
{
	mpPath8[0] = 0;
	FileStream::SetPath(pPath8); // Note that in a constructor, the virtual function mechanism is inoperable.
}


FileStream::FileStream(const char16_t* pPath16)
  : IStream(),
	mpReleaseAllocator(NULL),
	mnFileHandle(kFileHandleInvalid),
	mnRefCount(0),
	mnAccessFlags(0),
	mnCD(0),
	mnSharing(0),
	mnLastError(kStateNotOpen)
{
	mpPath8[0] = 0;
	FileStream::SetPath(pPath16); // Note that in a constructor, the virtual function mechanism is inoperable.
}


#if EA_WCHAR_UNIQUE
FileStream::FileStream(const wchar_t* pPathW)
	: IStream(),
	mpReleaseAllocator(NULL),
	mnFileHandle(kFileHandleInvalid),
	mnRefCount(0),
	mnAccessFlags(0),
	mnCD(0),
	mnSharing(0),
	mnLastError(kStateNotOpen)
{
	mpPath8[0] = 0;
	FileStream::SetPath(pPathW); // Note that in a constructor, the virtual function mechanism is inoperable.
}
#endif


FileStream::FileStream(const FileStream& fs)
  : IStream(),
	mpReleaseAllocator(NULL),
	mnFileHandle(kFileHandleInvalid),
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

		mnFileHandle  = kFileHandleInvalid;
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
	if((mnFileHandle == kFileHandleInvalid) && pPath8)
	{
		strncpy(mpPath8, pPath8, kMaxPathLength);
		mpPath8[kMaxPathLength - 1] = 0;
	}  
}


void FileStream::SetPath(const char16_t* pPath16)
{
	if((mnFileHandle == kFileHandleInvalid) && pPath16)
		EA::StdC::Strlcpy(mpPath8, pPath16, kMaxPathLength);
}


#if EA_WCHAR_UNIQUE
void FileStream::SetPath(const wchar_t* pPathW)
{
	if ((mnFileHandle == kFileHandleInvalid) && pPathW)
		EA::StdC::Strlcpy(mpPath8, pPathW, kMaxPathLength);
}
#endif


size_t FileStream::GetPath(char8_t* pPath8, size_t nPathLength)
{
	if(mpPath8 && nPathLength)
	{
		strncpy(pPath8, mpPath8, nPathLength);
		pPath8[nPathLength - 1] = 0;
	}

	// In this case the user wants just the length of the return value.
	return strlen(mpPath8);
}


size_t FileStream::GetPath(char16_t* pPath16, size_t nPathLength)
{
	if(pPath16 && nPathLength)
		return EA::StdC::Strlcpy(pPath16, mpPath8, nPathLength);

	// In this case the user wants just the length of the return value.
	char16_t pathTemp[kMaxPathLength];
	return GetPath(pathTemp, kMaxPathLength);
}


#if EA_WCHAR_UNIQUE
size_t FileStream::GetPath(wchar_t* pPathW, size_t nPathLength)
{
	if (pPathW && nPathLength)
		return EA::StdC::Strlcpy(pPathW, mpPath8, nPathLength);

	// In this case the user wants just the length of the return value.
	wchar_t pathTemp[kMaxPathLength];
	return GetPath(pathTemp, kMaxPathLength);
}
#endif


bool FileStream::Open(int nAccessFlags, int nCreationDisposition, int nSharing, int nUsageHints)
{
	if((mnFileHandle == kFileHandleInvalid) && nAccessFlags) // If not already open and if some kind of access is requested...
	{
		int nOpenFlags(0);

		if(nAccessFlags == kAccessFlagRead)
			nOpenFlags = CELL_FS_O_RDONLY;
		else if(nAccessFlags == kAccessFlagWrite)
			nOpenFlags = CELL_FS_O_WRONLY;
		else if(nAccessFlags == kAccessFlagReadWrite)
			nOpenFlags = CELL_FS_O_RDWR;

		if(nCreationDisposition == kCDDefault)
		{
			// To consider: A proposal is on the table that specifies that the 
			// default disposition for write is kCDCreateAlways and the default
			// disposition for read/write is kCDOpenAlways. However, such a change
			// may break existing code.
			if(nAccessFlags & kAccessFlagWrite)
				nCreationDisposition = kCDOpenAlways;
			else
				nCreationDisposition = kCDOpenExisting;
		}

		switch(nCreationDisposition)
		{
			case kCDCreateNew:
				nOpenFlags |= CELL_FS_O_CREAT;
				nOpenFlags |= CELL_FS_O_EXCL;   // Open only if it doesn't already exist.
				break;

			case kCDCreateAlways:
				nOpenFlags |= CELL_FS_O_CREAT;  // Always make it like the file was just created.
				nOpenFlags |= CELL_FS_O_TRUNC;
				break;

			case kCDOpenExisting:               // Open the file if it exists, else fail.
				break;                          // Nothing to do.

			case kCDOpenAlways:
				nOpenFlags |= CELL_FS_O_CREAT;  // Open the file no matter what.
				break;

			case kCDTruncateExisting:           // Open the file if it exists, and truncate it if so.
				nOpenFlags |= CELL_FS_O_TRUNC;
				break;
		}

		int fd;

		Retry:
		CellFsErrno result = cellFsOpen(mpPath8, nOpenFlags, &fd, NULL, 0);

		EA_COMPILETIME_ASSERT(CELL_FS_SUCCEEDED == 0); // The mnLastError handling below depends on this.

		if(result == CELL_FS_SUCCEEDED) // If it succeeded...
		{
			mnFileHandle  = fd;
			mnAccessFlags = nAccessFlags;
			mnCD          = nCreationDisposition;
			mnSharing     = nSharing;
			mnUsageHints  = nUsageHints;
		}
		else if((result == CELL_FS_ENOTMOUNTED) || (result == CELL_FS_EIO))
		{
			// Sony requires that PS3 DVD media errors prior to SDK 210 be 
			// retried infinitely. We call the user-specified error handler repeatedly. 
			// Note that the error handler may specify that we don't retry but exit this
			// function. However, the caller of this function would then be required to 
			// implement the retries as per Sony's requirements. 
			FileErrorInfo fileErrorInfo = { this, kFileOperationOpen, kDriveTypeUnknown, { 0 } };
			GetPath(fileErrorInfo.mFilePath16, kMaxPathLength);
			fileErrorInfo.mDriveType = kDriveTypeUnknown;

			if(PS3DriveWait(fileErrorInfo) == kErrorResponseRetry)
				goto Retry;
		}

		mnLastError = result;
	}

	return (mnFileHandle != kFileHandleInvalid);
}


bool FileStream::Close()
{
	if((mnFileHandle != kFileHandleInvalid))
	{
		cellFsClose(mnFileHandle);

		mnFileHandle  = kFileHandleInvalid;
		mnAccessFlags = 0;
		mnCD          = 0;
		mnSharing     = 0;
		mnUsageHints  = 0;
		mnLastError   = kStateNotOpen;
	}

	return true;
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
	size_type size = kSizeTypeError;

	if(mnFileHandle != kFileHandleInvalid)
	{
		CellFsStat fsStat;

		const CellFsErrno result = cellFsFstat(mnFileHandle, &fsStat);

		if(result == CELL_FS_SUCCEEDED)
			size = (size_type)fsStat.st_size;
		else
			mnLastError = result;
	}

	return size;
}


bool FileStream::SetSize(size_type size)
{
	bool bReturnValue = false;

	if(mnFileHandle != kFileHandleInvalid)
	{
		//const uint64_t nCurrentSize = GetSize();

		mnLastError = cellFsFtruncate(mnFileHandle, (uint64_t)size);

		if(mnLastError == CELL_FS_SUCCEEDED)
		{
			// The PS3 won't actually increase the size of the file correctly unless
			// you write something to the end of the new size (if it is a size increase). 
			// But we don't want to write anything unless the file is increasing in size.
			// ** Disabled until we can verify that it works as desired. **
			//if(size > nCurrentSize)
			//{
			//    uint64_t nCount = 0;
			//    mnLastError = cellFsLseek(mnFileHandle, (int64_t)size - 1, CELL_FS_SEEK_SET, &nCount);
			//
			//    if(mnLastError == CELL_FS_SUCCEEDED)
			//    {
			//        const char zero = 0;
			//        mnLastError = cellFsWrite(mnFileHandle, &zero, 1, &nCount);
			//
			//        if(mnLastError == CELL_FS_SUCCEEDED)
			//            bReturnValue = true;
			//    }
			//}
			//else
				bReturnValue = true;
		}
	}

	return bReturnValue;
}


off_type FileStream::GetPosition(PositionType positionType) const
{
	off_type position = (off_type)kSizeTypeError;

	if(mnFileHandle != kFileHandleInvalid)
	{
		uint64_t pos = 0;

		if(cellFsLseek(mnFileHandle, 0, CELL_FS_SEEK_CUR, &pos) == CELL_FS_SUCCEEDED)
		{
			if(positionType == kPositionTypeBegin)
				position = (off_type)pos;
			else if(positionType == kPositionTypeEnd)
			{
				position = (off_type)pos;

				if(position != (off_type)-1)
				{
					const size_type nSize = GetSize();
					if(nSize != kSizeTypeError)
						position = (off_type)(position - (off_type)nSize);
				}
			}
			else // Else we have kPositionTypeCurrent
				position = 0; // The position relative to the current position is by definition always zero.
		}
	}

	return position;
}


bool FileStream::SetPosition(off_type position, PositionType positionType)
{
	bool bReturnValue = false;

	if(mnFileHandle != kFileHandleInvalid)
	{
		int nMethod;

		switch(positionType)
		{
			default:
			case kPositionTypeBegin:
				nMethod = CELL_FS_SEEK_SET;
				break;

			case kPositionTypeCurrent:
				nMethod = CELL_FS_SEEK_CUR;
				break;

			case kPositionTypeEnd:
				nMethod = CELL_FS_SEEK_END;
				break;
		}

		uint64_t pos = 0;
		mnLastError = cellFsLseek(mnFileHandle, position, nMethod, &pos);
		
		if(mnLastError == CELL_FS_SUCCEEDED)
			bReturnValue = true;
		else
			mnLastError = errno;
	}

	return bReturnValue;
}


size_type FileStream::GetAvailable() const
{
	const off_type nPosition = GetPosition(kPositionTypeEnd);

	if(nPosition != (off_type)kSizeTypeError)
		return (size_type)-nPosition;

	return kSizeTypeError;
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
	size_type readSize = kSizeTypeError;

	if(mnFileHandle != kFileHandleInvalid)
	{
		uint64_t nCount = 0;

		Retry:
		CellFsErrno result = cellFsRead(mnFileHandle, pData, nSize, &nCount);

		// To consider: write a loop that executes in the case that the 
		// resulting nCount is less than nSize. It turns out that the 
		// PS3 may return an nCount < nSize in the case that the DVD is 
		// being read and the DVD is ejected while there is read-ahead
		// data in the file system buffer. cellFsRead will return the 
		// data in the system buffer even if its size is less than nSize
		// and there is more to read from the file. This is a questionable
		// thing to do on Sony's part, but for the time being we need 
		// to deal with it. As it stands now, the code we have here isn't
		// broken; it merely may return an nCount < nSize even though 
		// there is more data in the file.

		if(mnLastError == CELL_FS_SUCCEEDED)
			readSize = nCount;

		else if((result == CELL_FS_ENOTMOUNTED) || (result == CELL_FS_EIO) || (result == CELL_FS_EBADF)) // Maybe we shouldn't check for CELL_FS_EBADF.
		{
			// Sony requires that PS3 DVD media errors be retried infinitely.
			// We call the user-specified error handler repeatedly. Note that
			// the error handler may specify that we don't retry but exit this
			// function. However, the caller of this function would then be 
			// required to implement the retries as per Sony's requirements. 
			FileErrorInfo fileErrorInfo = { this, kFileOperationOpen, kDriveTypeUnknown, { 0 } };

			GetPath(fileErrorInfo.mFilePath16, kMaxPathLength);
			fileErrorInfo.mDriveType = kDriveTypeUnknown;

			if(PS3DriveWait(fileErrorInfo) == kErrorResponseRetry)
				goto Retry;
		}

		mnLastError = result;
	}

	return readSize;
}


bool FileStream::Write(const void* pData, size_type nSize)
{
	if(mnFileHandle != kFileHandleInvalid)
	{
		uint64_t nCount = 0;
		uint64_t amountWritten = 0;
		int writeResult = 0;

		do
		{
			writeResult = cellFsWrite(mnFileHandle, static_cast<const char*>(pData)+amountWritten, nSize - amountWritten, &nCount);

			if(writeResult == CELL_FS_SUCCEEDED)
				amountWritten += nCount;

		}while(writeResult >= 0 && amountWritten < nSize);
		
		mnLastError = ((writeResult == CELL_FS_SUCCEEDED) && (amountWritten == nSize)) ? kStateSuccess : writeResult;
	}

	return (mnLastError == kStateSuccess);
}


bool FileStream::Flush()
{
	//if(mnFileHandle != kFileHandleInvalid) {
	//    // Nothing to do on this platform.
	//}
	return true;
}


} // namespace IO


} // namespace EA















