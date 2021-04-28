/////////////////////////////////////////////////////////////////////////////
// EAFileStreamWiiHio2.h
//
// Copyright (c) 2007 Criterion Software Limited and its licensors. All Rights Reserved.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#ifndef EAIO_EAFILESTREAMWIIHIO2_H
#define EAIO_EAFILESTREAMWIIHIO2_H


#include <EAIO/EAFileBase.h>
#include <EAIO/EAFileStream.h>
#include <EAIO/Wii/EAFileStreamWiiNand.h>
#include <EAIO/Wii/MCSFile.h>
#include <stddef.h>


namespace EA
{
    namespace IO
    {
        /* To do: Enable this.
        /// IOResultCode
        /// File stream-specific errors.
        /// We hard-code the values here because otherwise we'd have to #include Microsoft headers from 
        /// a header file; and that is something we avoid at all costs.
        /// These represent the most common file system errors. However, there are additional errors 
        /// which may be returned by the system APIs which are different from these. You should be 
        /// prepared to receive any value for an error code. 
        enum IOResultCodeHio2
        {
            kFSErrorBase              =   0,   /// Error code base for this module (NAND_RESULT_OK)
            kFSErrorGeneral           =  -1,   /// Catchall for all other errors
            kFSErrorNotOpen           =  -2,   /// Attempt to read a stream that hasn't been opened. We define this as -2 instead of ERROR_INVALID_HANDLE because we already have another define of our own (kStateNotOpen) which is -2.
            kFSErrorNoMemory          =  -1,   /// Insufficient memory to perform operation (NAND_RESULT_ALLOC_FAILED)
            kFSErrorInvalidName       =  -8,   /// Invalid file name (NAND_RESULT_INVALID)
            kFSErrorNameTooLong       =  -1,   /// File name/path is too long
            kFSErrorFileNotFound      = -12,   /// Attempt to open a nonexistent file for reading (NAND_RESULT_NOEXISTS)
            kFSErrorPathNotFound      =  -1,   /// Attempt to access a file in a non-existent directory
            kFSErrorAccessDenied      = -15,   /// Insufficient privileges to open the file (NAND_RESULT_AUTHENTICATION)
            kFSErrorWriteProtect      =  -1,   /// Attempt to open a read-only file for writing 
            kFSErrorSharingViolation  =  -1,   /// Attempt to modify a file that is in use
            kFSErrorDiskFull          = -11,   /// Out of space on the device (NAND_RESULT_MAXFILES)
            kFSErrorFileAlreadyExists =  -6,   /// Attempt to create a new file with the same name as existing file (NAND_RESULT_EXISTS)
            kFSErrorDeviceNotReady    =  -1,   /// Attempt to access a hardware device that isn't ready
            kFSErrorDataCRCError      =  -1,   /// The data read off of the disk was corrupted in some way
        };
        #define EAIO_RESULT_CODE_DEFINED
        */



        class FileStreamWiiHio2 : public IStream
        {
        public:
            enum { kTypeFileStreamWiiHio2 = 0x0608a770 };

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
            FileStreamWiiHio2(const char8_t* pPath8 = NULL);
            FileStreamWiiHio2(const char16_t* pPath16);

            // FileStreamWiiHio2
            // Does not copy information related to an open file, such as the file handle.
            FileStreamWiiHio2(const FileStreamWiiHio2& fs);

            virtual ~FileStreamWiiHio2();

            // operator=
            // Does not copy information related to an open file, such as the file handle.
            FileStreamWiiHio2& operator=(const FileStreamWiiHio2& fs);

            virtual int       AddRef();
            virtual int       Release();

            virtual void      SetPath(const char8_t* pPath8);
            virtual void      SetPath(const char16_t* pPath16);
            virtual size_t    GetPath(char8_t* pPath8, size_t nPathLength);
            virtual size_t    GetPath(char16_t* pPath16, size_t nPathLength);

            virtual bool      Open(int nAccessFlags = kAccessFlagRead, int nCreationDisposition = kCDDefault, int nSharing = kShareRead, int nUsageHints = kUsageHintNone); 
            virtual bool      Close();
            virtual uint32_t  GetType() const { return kTypeFileStreamWiiHio2; }
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

            ///////////////////////////////////
            // Wii-specific functionality
            ///////////////////////////////////
            // Currently empty

        protected:
            void Reset();

        protected:
            FileInfo        mFileInfo;                  /// The file information structure
            FilePosition    mFilePosition;              /// File read position management
            int             mnRefCount;                 /// Reference count, which may or may not be in use.
            int             mnAccessFlags;              /// See enum AccessFlags.
            int             mnCD;                       /// See enum CD (creation disposition).
            int             mnSharing;                  /// See enum Share.
            int             mnUsageHints;               /// See enum UsageHints.
            mutable int     mnLastError;                /// Used for error reporting.
            char8_t         mpPath8[kMaxPathLength];    /// Path for the file.

        }; // class FileStreamWiiHio2

    } // namespace IO

} // namespace EA


#endif // Header include guard
