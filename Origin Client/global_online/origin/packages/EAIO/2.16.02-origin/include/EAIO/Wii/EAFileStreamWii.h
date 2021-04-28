/////////////////////////////////////////////////////////////////////////////
// EAFileStreamWii.h
//
// Copyright (c) 2003, Electronic Arts Inc. All rights reserved.
// Created by Paul Pedriana
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EAIO_EAFILESTREAM_REVOLUTION_H
#define EAIO_EAFILESTREAM_REVOLUTION_H
#ifdef EA_PLATFORM_REVOLUTION

#include "EAIO/EAFileStream.h"
#include "EAIO/EAFileBase.h"
#include <stddef.h>
#include <revolution/dvd.h>

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
        enum IOResultCodeDvd
        {
            kFSDvdErrorBase              =   0,   /// Error code base for this module (NAND_RESULT_OK)
            kFSDvdErrorGeneral           =  -1,   /// Catchall for all other errors
            kFSDvdErrorNotOpen           =  -2,   /// Attempt to read a stream that hasn't been opened. We define this as -2 instead of ERROR_INVALID_HANDLE because we already have another define of our own (kStateNotOpen) which is -2.
            kFSDvdErrorNoMemory          =  -1,   /// Insufficient memory to perform operation (NAND_RESULT_ALLOC_FAILED)
            kFSDvdErrorInvalidName       =  -8,   /// Invalid file name (NAND_RESULT_INVALID)
            kFSDvdErrorNameTooLong       =  -1,   /// File name/path is too long
            kFSDvdErrorFileNotFound      = -12,   /// Attempt to open a nonexistent file for reading (NAND_RESULT_NOEXISTS)
            kFSDvdErrorPathNotFound      =  -1,   /// Attempt to access a file in a non-existent directory
            kFSDvdErrorAccessDenied      = -15,   /// Insufficient privileges to open the file (NAND_RESULT_AUTHENTICATION)
            kFSDvdErrorWriteProtect      =  -1,   /// Attempt to open a read-only file for writing 
            kFSDvdErrorSharingViolation  =  -1,   /// Attempt to modify a file that is in use
            kFSDvdErrorDiskFull          = -11,   /// Out of space on the device (NAND_RESULT_MAXFILES)
            kFSDvdErrorFileAlreadyExists =  -6,   /// Attempt to create a new file with the same name as existing file (NAND_RESULT_EXISTS)
            kFSDvdErrorDeviceNotReady    =  -1,   /// Attempt to access a hardware device that isn't ready
            kFSDvdErrorDataCRCError      =  -1,   /// The data read off of the disk was corrupted in some way
        };
        #define EAIO_RESULT_CODE_DEFINED
        */


        class FileStream : public IStream
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

            ///////////////////////////////////
            // Wii-specific functionality
            ///////////////////////////////////

            /// SetWindowSize
            /// Size must be an even power of two >= 512.
            /// Must be called while the file is not open.
            void SetWindowSize(uint32_t size);

        protected:
            static const uint32_t kWindowSizeDefault =  512;       /// Size of the window buffer we have on the file.

            bool FillWindow(uint32_t position);
            bool FlushWindow();

        protected:

            DVDFileInfo mFileInfo;                      /// This is somewhat analagous to other systems' file handles.
            uint32_t    mCurrentPosition;               /// The file read/write position as the user sees it.
            int         mnRefCount;                     /// Reference count, which may or may not be in use.
            int         mnAccessFlags;                  /// See enum AccessFlags.
            int         mnCD;                           /// See enum CD (creation disposition).
            int         mnSharing;                      /// See enum Share.
            int         mnUsageHints;                   /// See enum UsageHints.
            mutable int mnLastError;                    /// Used for error reporting.
            char*       mWindow;                        /// Window used for buffering reads and writes.
            uint32_t    mWindowPosition;                /// Which position in the file the window begins at.
            uint32_t    mWindowSize;                    /// Window size for this file. Defaults to kWindowSizeDefault.
            int         mbWindowWasWritten;             /// True if a write occured to our window.
            int         mbSizeWasDecreased;             /// True if a shrinking resize operation was done.
            char8_t     mpPath8[kMaxPathLength];        /// Path for the file.

        }; // class FileStream

    } // namespace IO

} // namespace EA

#endif // #ifdef EA_PLATFORM_REVOLUTION
#endif // #ifndef EAIO_EAFILESTREAM_REVOLUTION_H
