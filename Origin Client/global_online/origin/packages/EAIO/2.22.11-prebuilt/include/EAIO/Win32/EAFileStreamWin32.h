/////////////////////////////////////////////////////////////////////////////
// EAFileStreamWin32.h
//
// Copyright (c) 2003, Electronic Arts Inc. All rights reserved.
// Created by Paul Pedriana
//
// Provides a file stream for Microsoft-derived platforms. These include
// Win32, Win64, XBox, and XBox2.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef EAIO_EAFILESTREAM_WIN32_H
#define EAIO_EAFILESTREAM_WIN32_H

#ifndef EAIO_EAFILESTREAM_H
	#include <EAIO/EAFileStream.h>
#endif
#ifndef EAIO_EAFILEBASE_H
	#include <EAIO/EAFileBase.h>
#endif
#include <stddef.h>


#if defined(_MSC_VER)
	#pragma warning(push)
	#pragma warning(disable: 4275) // non dll-interface class used as base for DLL-interface classkey 'identifier'
#endif


namespace EA
{
	namespace IO
	{
		/// IOResultCode
		/// File stream-specific errors.
		/// We hard-code the values here because otherwise we'd have to #include Microsoft headers from 
		/// a header file; and that is something we avoid at all costs.
		/// These represent the most common file system errors. However, there are additional errors 
		/// which may be returned by the system APIs which are different from these. You should be 
		/// prepared to receive any value for an error code. 
		enum IOResultCode
		{
			kFSErrorBase              =   0,   /// Error code base for this module
			kFSErrorGeneral           =  -1,   /// Catchall for all other errors
			kFSErrorNotOpen           =  -2,   /// Attempt to read a stream that hasn't been opened. We define this as -2 instead of ERROR_INVALID_HANDLE because we already have another define of our own (kStateNotOpen) which is -2.
			kFSErrorNoMemory          =   8,   /// Insufficient memory to perform operation (ERROR_NOT_ENOUGH_MEMORY)
			kFSErrorInvalidName       = 123,   /// Invalid file name (ERROR_INVALID_NAME)
			kFSErrorNameTooLong       = 111,   /// File name/path is too long (ERROR_BUFFER_OVERFLOW)
			kFSErrorFileNotFound      =   2,   /// Attempt to open a nonexistent file for reading (ERROR_FILE_NOT_FOUND)
			kFSErrorPathNotFound      =   3,   /// Attempt to access a file in a non-existent directory (ERROR_PATH_NOT_FOUND)
			kFSErrorAccessDenied      =   5,   /// Insufficient privileges to open the file (ERROR_ACCESS_DENIED)
			kFSErrorWriteProtect      =  19,   /// Attempt to open a read-only file for writing (ERROR_WRITE_PROTECT)
			kFSErrorSharingViolation  =  16,   /// Attempt to modify a file that is in use (ERROR_CURRENT_DIRECTORY)
			kFSErrorDiskFull          = 112,   /// Out of space on the device (ERROR_DISK_FULL)
			kFSErrorFileAlreadyExists =  80,   /// Attempt to create a new file with the same name as existing file (ERROR_FILE_EXISTS)
			kFSErrorDeviceNotReady    =  21,   /// Attempt to access a hardware device that isn't ready (ERROR_NOT_READY)
			kFSErrorDataCRCError      =  23,   /// The data read off of the disk was corrupted in some way (ERROR_CRC)
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
			#if EA_CHAR16_NATIVE
				FileStream(const char16_t* pPath16);
			#endif
			FileStream(const wchar_t* pPathW);
			FileStream(const char32_t* pPath32);

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
			#if EA_CHAR16_NATIVE
				virtual void      SetPath(const char16_t* pPath16);
			#endif
			virtual void      SetPath(const wchar_t* pPathW);
			virtual void      SetPath(const char32_t* pPath32);
			virtual size_t    GetPath(char8_t* pPath8, size_t nPathLength);
			#if EA_CHAR16_NATIVE
				virtual size_t    GetPath(char16_t* pPath16, size_t nPathLength);
			#endif
			virtual size_t    GetPath(wchar_t* pPathW, size_t nPathLength);
			virtual size_t    GetPath(char32_t* pPath32, size_t nPathLength);

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

			void*             mhFile;                     /// We defined as void* instead of HANDLE in order to simplify header includes. HANDLE is typedef'd to (void *) on all Windows platforms.
			wchar_t           mpPathW[kMaxPathLength];    /// Path for the file.
			int               mnRefCount;                 /// Reference count, which may or may not be in use.
			int               mnAccessFlags;              /// See enum AccessFlags.
			int               mnCD;                       /// See enum CD (creation disposition).
			int               mnSharing;                  /// See enum Share.
			int               mnUsageHints;               /// See enum UsageHints.
			mutable int       mnLastError;                /// Used for error reporting.
			mutable size_type mnSize;                     /// Used for caching the file size, which is sometimes useful. 

		}; // class FileStream

	} // namespace IO

} // namespace EA


#if defined(_MSC_VER)
	#pragma warning(pop)
#endif


#endif  // Header include guard



