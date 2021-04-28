/////////////////////////////////////////////////////////////////////////////
// EAFileStreamKettle.cpp
//
// Copyright (c) 2003, Electronic Arts Inc. All rights reserved.
//
// Provides a file stream for the Kettle platforms.
//
/////////////////////////////////////////////////////////////////////////////


#include <EAIO/EAFileStream.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/FnEncode.h>
#include <coreallocator/icoreallocator.h>
#include EA_ASSERT_HEADER
#include <scebase_common.h>
#include <kernel.h>
#include <sdk_version.h>


namespace EA
{
namespace IO
{

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
	FileStream::SetPath(pPath8); // Note that in a constructor, the virtual function mechanism is inoperable, so we qualify the function call.
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
	FileStream::SetPath(pPath16);
}


FileStream::FileStream(const char32_t* pPath32)
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
	FileStream::SetPath(pPath32);
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
		FileStream::SetPath(pPathW);
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
	mpPath8[0] = 0;
	FileStream::SetPath(fs.mpPath8);
}


FileStream::~FileStream()
{
	FileStream::Close(); // Note that in a destructor, the virtual function mechanism is inoperable, so we qualify the function call.
}


FileStream& FileStream::operator=(const FileStream& fs)
{
	Close();
	SetPath(fs.mpPath8);

	mnFileHandle  = kFileHandleInvalid;
	mnAccessFlags = 0;
	mnCD          = 0;
	mnSharing     = 0;
	mnUsageHints  = fs.mnUsageHints;
	mnLastError   = kStateNotOpen;

	return *this;
}


int FileStream::AddRef()
{
	return ++mnRefCount;
}


int FileStream::Release()
{
	if (mnRefCount > 1)
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
	if (!IsValidHandle() && pPath8)
		EA::StdC::Strlcpy(mpPath8, pPath8, EAArrayCount(mpPath8));
}


void FileStream::SetPath(const char16_t* pPath16)
{
	if (!IsValidHandle() && pPath16)
		EA::StdC::Strlcpy(mpPath8, pPath16, EAArrayCount(mpPath8)); // Convert UCS2 to UTF8.
}


void FileStream::SetPath(const char32_t* pPath32)
{
	if (!IsValidHandle() && pPath32)
		EA::StdC::Strlcpy(mpPath8, pPath32, EAArrayCount(mpPath8)); // Convert UCS4 to UTF8.
}


#if EA_WCHAR_UNIQUE
void FileStream::SetPath(const wchar_t* pPathW)
{
	if (!IsValidHandle() && pPathW)
		EA::StdC::Strlcpy(mpPath8, pPathW, EAArrayCount(mpPath8)); // Convert wchar_t to UTF8.
}
#endif


size_t FileStream::GetPath(char8_t* pPath8, size_t nPathCapacity)
{
	// Return the required strlen of the destination path.
	return EA::StdC::Strlcpy(pPath8, mpPath8, nPathCapacity);
}


size_t FileStream::GetPath(char16_t* pPath16, size_t nPathCapacity)
{
	// Return the required strlen of the destination path.
	return EA::StdC::Strlcpy(pPath16, mpPath8, nPathCapacity);
}


size_t FileStream::GetPath(char32_t* pPath32, size_t nPathCapacity)
{
	// Return the required strlen of the destination path.
	return EA::StdC::Strlcpy(pPath32, mpPath8, nPathCapacity);
}


#if EA_WCHAR_UNIQUE
	size_t FileStream::GetPath(wchar_t* pPathW, size_t nPathCapacity)
	{
		// Return the required strlen of the destination path.
		return EA::StdC::Strlcpy(pPathW, mpPath8, nPathCapacity);
	}
#endif


bool FileStream::Open(int nAccessFlags, int nCreationDisposition, int nSharing, int nUsageHints)
{
	if (!IsValidHandle() && nAccessFlags) // If not already open and if some kind of access is requested...
	{
		int nOpenFlags = 0;

		if (nAccessFlags == kAccessFlagRead)
			nOpenFlags = SCE_KERNEL_O_RDONLY;
		else if (nAccessFlags == kAccessFlagWrite)
			nOpenFlags = SCE_KERNEL_O_WRONLY;
		else if (nAccessFlags == kAccessFlagReadWrite)
			nOpenFlags = SCE_KERNEL_O_RDWR;

		if (nCreationDisposition == kCDDefault)
		{
			if (nAccessFlags & kAccessFlagWrite)
				nCreationDisposition = kCDOpenAlways;
			else
				nCreationDisposition = kCDOpenExisting;
		}

		switch (nCreationDisposition)
		{
			case kCDCreateNew:
				nOpenFlags |= SCE_KERNEL_O_CREAT;
				nOpenFlags |= SCE_KERNEL_O_EXCL;       // Open only if it doesn't already exist.
				break;

			case kCDCreateAlways:
				nOpenFlags |= SCE_KERNEL_O_CREAT;      // Always make it like the file was just created.
				nOpenFlags |= SCE_KERNEL_O_TRUNC;
				break;

			case kCDOpenExisting:           // Open the file if it exists, else fail.
				break;                      // Nothing to do.

			case kCDOpenAlways:
				nOpenFlags |= SCE_KERNEL_O_CREAT;      // Open the file no matter what.
				break;

			case kCDTruncateExisting:       // Open the file if it exists, and truncate it if so.
				nOpenFlags |= SCE_KERNEL_O_TRUNC;
				break;
		}

		const size_t length = strlen(mpPath8); // We know the length is at least 1 from our check above.

		if (length > 0)
		{
			// We explicitly disable the ability to open a directory.
			const bool pathIsDirectory = ((mpPath8[length - 1] == '/') || Directory::Exists(mpPath8));
			if (!pathIsDirectory)
			{
				#if (SCE_ORBIS_SDK_VERSION >= 0x00910000u)
					SceKernelMode mode = (SCE_KERNEL_S_IRU | SCE_KERNEL_S_IRWU);
				#else
					SceKernelMode mode = (SCE_KERNEL_S_IRUSR | SCE_KERNEL_S_IWUSR);
				#endif

				mnFileHandle = sceKernelOpen(mpPath8, nOpenFlags, mode);

				if (mnFileHandle < 0) // If it failed...
				{
					// mnAccessFlags = 0; // Not necessary, as these should already be zeroed.
					// mnCD          = 0;
					// mnSharing     = 0;
					// mnUsageHints  = 0;
					mnLastError      = mnFileHandle;
					mnFileHandle     = kFileHandleInvalid;
				}
				else
				{
					mnAccessFlags = nAccessFlags;
					mnCD          = nCreationDisposition;
					mnSharing     = nSharing;
					mnUsageHints  = nUsageHints;
					mnLastError   = 0;
				}
			}
		}
	}

	return IsValidHandle();
}


bool FileStream::Close()
{
	if (IsValidHandle())
	{
		const int result = sceKernelClose(mnFileHandle);
		EA_ASSERT(result == SCE_OK); EA_UNUSED(result);

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
	if (IsValidHandle())
	{
		SceKernelStat st;
		const int result = sceKernelFstat(mnFileHandle, &st);

		if (result == SCE_OK)
			return st.st_size;

		mnLastError = result;
	}

	return kSizeTypeError;
}


bool FileStream::SetSize(size_type size)
{
	if (IsValidHandle())
	{
		 const int result = sceKernelFtruncate(mnFileHandle, static_cast<off_t>(size));
		 
		 if (result == SCE_OK)
			 return true;

		 mnLastError = result;
	}

	return false;
}


off_type FileStream::GetPosition(PositionType positionType) const
{
	off_t result = 0;

	if (IsValidHandle())
	{
		if (positionType == kPositionTypeBegin)
		{
			result = sceKernelLseek(mnFileHandle, 0, SCE_KERNEL_SEEK_CUR);
		}
		else if (positionType == kPositionTypeEnd)
		{
			result = sceKernelLseek(mnFileHandle, 0, SCE_KERNEL_SEEK_CUR);

			if (result >= 0)
			{
				const size_type nSize = GetSize();
				if (nSize != kSizeTypeError)
					result -= static_cast<off_t>(nSize);
			}
		}
	}

	return result;
}


bool FileStream::SetPosition(off_type position, PositionType positionType)
{
	if (IsValidHandle())
	{
		int nMethod;

		switch(positionType)
		{
			default:
			case kPositionTypeBegin:
				nMethod = SCE_KERNEL_SEEK_SET;
				break;

			case kPositionTypeCurrent:
				nMethod = SCE_KERNEL_SEEK_CUR;
				break;

			case kPositionTypeEnd:
				nMethod = SCE_KERNEL_SEEK_END;
				break;
		}

		const off_t nResult = sceKernelLseek(mnFileHandle, static_cast<off_t>(position), nMethod);

		if (nResult >= 0)
			return true;

		mnLastError = static_cast<int>(nResult);
	}

	return false;
}


size_type FileStream::GetAvailable() const
{
	if (IsValidHandle())
	{
		const off_type nPosition = GetPosition(kPositionTypeEnd);

		if (nPosition != static_cast<off_type>(kSizeTypeError))
			return static_cast<size_type>(-nPosition);
	}

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
	if (IsValidHandle())
	{
		const ssize_t nCount = sceKernelRead(mnFileHandle, pData, static_cast<size_t>(nSize));

		if (nCount >= 0)
			return static_cast<size_type>(nCount);
	}

	return kSizeTypeError;
}


bool FileStream::Write(const void* pData, size_type nSize)
{
	const char* pData8 = static_cast<const char*>(pData);

	if (IsValidHandle())
	{
		ssize_t  nCount = 0;
		size_type amountWritten = 0;

		do
		{
			nCount = sceKernelWrite(mnFileHandle, pData8 + amountWritten, nSize - amountWritten);

			if (nCount >= 0)
				amountWritten += static_cast<size_type>(nCount);

		} while (nCount >= 0 && amountWritten < nSize);
		
		mnLastError = (nCount >= 0 && amountWritten == nSize) ? kStateSuccess : static_cast<int>(nCount);
	}

	return (mnLastError == kStateSuccess);
}


bool FileStream::Flush()
{
	if (IsValidHandle())
	{
		if (sceKernelFsync(mnFileHandle) != SCE_OK)
			return false;
	}

	return true;
}
} // namespace IO
} // namespace EA















