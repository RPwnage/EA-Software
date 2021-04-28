/////////////////////////////////////////////////////////////////////////////
// EAFileStreamPS3.h
//
// Copyright (c) 2003, Electronic Arts Inc. All rights reserved.
// Created by Paul Pedriana
//
/////////////////////////////////////////////////////////////////////////////


#ifndef EAIO_EAFILESTREAM_PS3_H
#define EAIO_EAFILESTREAM_PS3_H

#ifndef EAIO_EAFILESTREAM_H
    #include <EAIO/EAFileStream.h>
#endif
#ifndef EAIO_EAFILEBASE_H
    #include <EAIO/EAFileBase.h>
#endif
#include <stddef.h>
#include <cell/cell_fs.h>


namespace EA
{
    namespace IO
    {

        /// IOResultCode
        /// File stream-specific errors.
        /// These represent the most common file system errors. However, there are additional errors 
        /// which may be returned by the system APIs which are different from these. You should be 
        /// prepared to receive any value for an error code. 
        enum IOResultCode
        {
            kFSErrorBase              = 0,                      /// Error code base for this module
            kFSErrorGeneral           = -1,                     /// Catchall for all other errors
            kFSErrorNotOpen           = -2,                     /// Attempt to read a stream that hasn't been opened. We define this as -2 instead of CELL_FS_EBADF because we already have another define of our own (kStateNotOpen) which is -2.
            kFSErrorNoMemory          = CELL_FS_EMFILE,         /// Insufficient memory to perform operation
            kFSErrorInvalidName       = CELL_FS_EINVAL,         /// Invalid file name
            kFSErrorNameTooLong       = CELL_FS_ENAMETOOLONG,   /// File name/path is too long
            kFSErrorFileNotFound      = CELL_FS_ENOENT,         /// Attempt to open a nonexistent file for reading
            kFSErrorPathNotFound      = CELL_FS_ENOTMOUNTED,    /// Attempt to access a file in a non-existent directory
            kFSErrorAccessDenied      = CELL_FS_EPERM,          /// Insufficient privileges to open the file
            kFSErrorWriteProtect      = CELL_FS_EROFS,          /// Attempt to open a read-only file for writing
            kFSErrorSharingViolation  = CELL_FS_EACCES,         /// Attempt to modify a file that is in use
            kFSErrorDiskFull          = CELL_FS_ENOSPC,         /// Out of space on the device
            kFSErrorFileAlreadyExists = CELL_FS_EEXIST,         /// Attempt to create a new file with the same name as existing file
            kFSErrorDeviceNotReady    = CELL_FS_ENODEV,         /// Attempt to access a hardware device that isn't ready
            kFSErrorDataCRCError      = CELL_FS_EIO             /// The data read off of the disk was corrupted in some way
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

            virtual size_type Read(void* pData, size_type nSize);
            virtual bool      Write(const void* pData, size_type nSize);
            virtual bool      Flush();

        protected:
            int         mnFileHandle;               /// System file handle.
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

#endif  // #ifndef EAIO_EAFILESTREAM_PS3_H











