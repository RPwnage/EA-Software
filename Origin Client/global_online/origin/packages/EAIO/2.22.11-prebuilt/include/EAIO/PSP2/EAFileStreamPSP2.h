/////////////////////////////////////////////////////////////////////////////
// EAFileStreamPSP2.h
//
// Copyright (c) 2010, Electronic Arts Inc. All rights reserved.
// Created by Stephen Kiazyk
//
/////////////////////////////////////////////////////////////////////////////


#ifndef EAIO_EAFILESTREAM_PSP2_H
#define EAIO_EAFILESTREAM_PSP2_H

#ifndef EAIO_EAFILESTREAM_H
	#include <EAIO/EAFileStream.h>
#endif
#ifndef EAIO_EAFILEBASE_H
	#include <EAIO/EAFileBase.h>
#endif
#include <stddef.h>
#include <sceerror.h>
#include <kernel/error.h>

namespace EA
{
	namespace IO
	{
		enum IOResultCode
		{
			kFSErrorBase              = kStateSuccess,              /// No Error
			kFSErrorGeneral           = kStateError,                /// Catch-all for all other errors
			kFSErrorNotOpen           = kStateNotOpen,              /// Attempt to read a stream that hasn't been opened. (Use -2 instead of the SCE error because -2 is an EAIO standard for not open)
			kFSErrorNoMemory          = SCE_ERROR_ERRNO_EMFILE,     /// Insufficient memory to perform operation
			kFSErrorInvalidName       = SCE_ERROR_ERRNO_EINVAL,     /// Invalid file name
			kFSErrorNameTooLong       = SCE_KERNEL_ERROR_IO_NAME_TOO_LONG, /// File name/path is too long
			kFSErrorFileNotFound      = SCE_ERROR_ERRNO_ENOENT,     /// Attempt to open a nonexistent file for reading
			kFSErrorPathNotFound      = SCE_ERROR_ERRNO_ENODEV,     /// Attempt to access a file in a non-existent directory
			kFSErrorAccessDenied      = SCE_ERROR_ERRNO_EPERM,      /// Insufficient privileges to open the file
			kFSErrorWriteProtect      = SCE_ERROR_ERRNO_EROFS,      /// Attempt to open a read-only file for writing
			kFSErrorSharingViolation  = SCE_ERROR_ERRNO_EACCES,     /// Attempt to modify a file that is in use
			kFSErrorDiskFull          = SCE_ERROR_ERRNO_ENOSPC,     /// Out of space on the device
			kFSErrorFileAlreadyExists = SCE_ERROR_ERRNO_EEXIST,     /// Attempt to create a new file with the same name as existing file
			kFSErrorDeviceNotReady    = SCE_ERROR_ERRNO_EBUSY,      /// Attempt to access a hardware device that isn't ready
			kFSErrorDataCRCError      = SCE_ERROR_ERRNO_EIO         /// The data read off of the disk was corrupted in some way
		};

		class EAIO_API FileStream : public IStream
		{
		public:
			enum { kTypeFileStream = 0x34722300 };

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
			FileStream(const char8_t* pPath8 = NULL);
			FileStream(const char16_t* pPath16);

			// FileStream
			// Does not copy information related to an open file, such as the file handle.
			FileStream(const FileStream& fs);

			virtual ~FileStream();

			// operator=
			// Does not copy information related to an open file, such as the file handle.
			FileStream& operator=(const FileStream& fs);

			virtual int       AddRef();
			virtual int       Release();

			virtual void      SetPath(const char8_t* pPath8);
			virtual void      SetPath(const char16_t* pPath16);
			virtual size_t    GetPath(char8_t* pPath8, size_t nPathLength);
			virtual size_t    GetPath(char16_t* pPath16, size_t nPathLength);

			virtual bool      Open(int nAccessFlags = kAccessFlagRead, int nCreationDisposition = kCDDefault, int nSharing = kShareRead, int nUsageHints = kUsageHintNone); 
			virtual bool      Close();
			virtual uint32_t  GetType() const { return kTypeFileStream; }
			virtual int       GetAccessFlags() const;
			virtual int       GetState() const;

			virtual size_type GetSize() const;
			virtual bool      SetSize(size_type size);

			virtual off_type  GetPosition(PositionType positionType = kPositionTypeBegin) const;
			virtual bool      SetPosition(off_type position, PositionType positionType = kPositionTypeBegin);

			virtual size_type GetAvailable() const;

			/// Sets the allocator to use for freeing this object itself when the user calls 
			/// Release for the last time. If SetReleaseAllocator is not used then global delete
			/// is used. By default the global heap is used to free the memory and not the 
			/// allocator set by SetAllocator.
			///
			/// If you copy this object, the new copy will inherit the ReleaseAllocator from the original
			/// MemoryStream if there is one. Otherwise the copy will be freed using global delete
			/// 
			/// Example usage:
			///     MemoryStream* pStream = new(GetMyAllocator()->Alloc(sizeof(MemoryStream))) MemoryStream;
			///     pStream->AddRef();
			///     pStream->SetReleaseAllocator(GetMyAllocator());
			///     pStream->Release(); // Will use GetMyAllocator() to free pStream.
			void              SetReleaseAllocator(EA::Allocator::ICoreAllocator* allocator);

			EA::Allocator::ICoreAllocator* GetReleaseAllocator() const;

			virtual size_type Read(void* pData, size_type nSize);
			virtual bool      Write(const void* pData, size_type nSize);
			virtual bool      Flush();

		protected:
			EA::Allocator::ICoreAllocator* mpReleaseAllocator;
			
			int         mFileHandle;
			char8_t     mpPath8[kMaxPathLength];    /// Path for the file.
			int         mnRefCount;                 /// Reference count, which may or may not be in use.
			int         mnAccessFlags;              /// See enum AccessFlags.
			int         mnCD;                       /// See enum CD (creation disposition).
			int         mnSharing;                  /// See enum Share.
			int         mnUsageHints;               /// See enum UsageHints.
			mutable int mnLastError;                /// Used for error reporting.

		}; // class FileStream

	} // namespace IO

} // namespace EA

#endif  // #ifndef EAIO_EAFILESTREAM_PSP2_H
