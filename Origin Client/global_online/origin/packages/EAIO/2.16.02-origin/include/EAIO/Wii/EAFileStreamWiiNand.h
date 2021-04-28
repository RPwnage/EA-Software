/////////////////////////////////////////////////////////////////////////////
// EAFileStreamWiiNand.h
//
// Copyright (c) 2007 Criterion Software Limited and its licensors. All Rights Reserved.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#ifndef EAIO_EAFILESTREAMWIINAND_H
#define EAIO_EAFILESTREAMWIINAND_H


#include <EAIO/EAFileStream.h>
#include <EAIO/EAFileBase.h>
#include <revolution/nand.h>
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
        enum IOResultCodeNand
        {
            kFSNandErrorBase              =   0,   /// Error code base for this module (NAND_RESULT_OK)
            kFSNandErrorGeneral           =  -1,   /// Catchall for all other errors
            kFSNandErrorNotOpen           =  -2,   /// Attempt to read a stream that hasn't been opened. We define this as -2 instead of ERROR_INVALID_HANDLE because we already have another define of our own (kStateNotOpen) which is -2.
            kFSNandErrorNoMemory          =  -1,   /// Insufficient memory to perform operation (NAND_RESULT_ALLOC_FAILED)
            kFSNandErrorInvalidName       =  -8,   /// Invalid file name (NAND_RESULT_INVALID)
            kFSNandErrorNameTooLong       =  -1,   /// File name/path is too long
            kFSNandErrorFileNotFound      = -12,   /// Attempt to open a nonexistent file for reading (NAND_RESULT_NOEXISTS)
            kFSNandErrorPathNotFound      =  -1,   /// Attempt to access a file in a non-existent directory
            kFSNandErrorAccessDenied      = -15,   /// Insufficient privileges to open the file (NAND_RESULT_AUTHENTICATION)
            kFSNandErrorWriteProtect      =  -1,   /// Attempt to open a read-only file for writing 
            kFSNandErrorSharingViolation  =  -1,   /// Attempt to modify a file that is in use
            kFSNandErrorDiskFull          = -11,   /// Out of space on the device (NAND_RESULT_MAXFILES)
            kFSNandErrorFileAlreadyExists =  -6,   /// Attempt to create a new file with the same name as existing file (NAND_RESULT_EXISTS)
            kFSNandErrorDeviceNotReady    =  -1,   /// Attempt to access a hardware device that isn't ready
            kFSNandErrorDataCRCError      =  -1,   /// The data read off of the disk was corrupted in some way
        };
        #define EAIO_RESULT_CODE_DEFINED
        */


        // FilePosition
        //
        // Implements a file position management utility.
        //
        class FilePosition
        {
        public:
            FilePosition() : mFileSize(0), mPosition(0) { }

            void      SetFileSize(size_type fileSize) { mFileSize = fileSize; }
            size_type GetFileSize() const             { return mFileSize; }
            
            size_type Skip(off_type offset);
            size_type Append(off_type offset);
            void      Seek(off_type offset, int origin);
            size_type Tell() const { return mPosition; }

        protected:
            size_type  mFileSize;
            size_type  mPosition;
        };


        ///////////////////////////////////////////////////////////////////////
        // NandFileSystem
        //
        // This is the high level file system manager. It initializes and 
        // shuts down the Nand-based file system. 
        // 
        // The NAND library provides functions to access the NAND flash memory built 
        // into the Wii console. NAND flash memory has the following characteristics:
        //    - Nonvolatile memory
        //    - Fast read, but slightly slow write
        //    - Random access
        //    - Concept of bit error
        //    - Concept of bad blocks
        //    - Concept of wear and tear because of writing 
        // 
        // The NAND memory capacity of the Wii console is 512 MB. However, not all of 
        // the 512 MB space can be used by the application program, as some of the space 
        // will be taken by the system files written at the factory or for permanently 
        // reserved system needs.
        // 
        // The Wii NAND file system provides a file system with the following characteristics.
        //    - Similar architecture to UNIX file systems.
        //    - Hierarchical file system.
        //    - Access control with three permission levels (Owner/Group/Other). (Group is a permission that corresponds to a company. It can be used when you want to allow access for titles from the same company.)
        //    - Single-bit error correction is done in the file system.
        //    - Bad block processing is done in the file system.
        //    - Ability of the file system to withstand sudden power shutdown. (The entire file system will not be damaged even if power is shut down while writing data or updating the FAT.)
        //    - Up to twelve characters for the file name/directory name.
        //    - Supports both absolute path specification and relative path specification.
        //    - The path delimiter text character is "/".
        //    - The maximum path length is 64 characters (including the NULL character).
        //    - A directory structure up to eight levels deep can be created.
        //    - The size of the file system block (FS block) is 16 KB. (In other words, even a 1-byte file takes a region of 16 KB.)
        //    - Maximum number of files/directories to be created is approximately 6000. 
        ///////////////////////////////////////////////////////////////////////

        class NandFileSystem
        {
        public:
            NandFileSystem() { }
           ~NandFileSystem() { }

            bool Init()      { return true; }
            void Shutdown()  { }
        };



        // FileStreamWiiNand
        //
        // Implements a file stream based on the NAND (flash memory) file system.
        // Starting with RevolutionSDK 2.1, patch 1, the NANDInit function is called 
        // automatically by the system. Thus you don't need to call NANDInit in your
        // application before using this file functionality.
        // 
        class FileStreamWiiNand : public IStream
        {
        public:
            enum { kTypeFileStreamWiiNand = 0x0608a765 };

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
            FileStreamWiiNand(const char8_t* pPath8 = NULL);
            FileStreamWiiNand(const char16_t* pPath16);

            // FileStreamWiiNand
            // Does not copy information related to an open file, such as the file handle.
            FileStreamWiiNand(const FileStreamWiiNand& fs);

            virtual ~FileStreamWiiNand();

            // operator=
            // Does not copy information related to an open file, such as the file handle.
            FileStreamWiiNand& operator=(const FileStreamWiiNand& fs);

            virtual int       AddRef();
            virtual int       Release();

            virtual void      SetPath(const char8_t* pPath8);
            virtual void      SetPath(const char16_t* pPath16);
            virtual size_t    GetPath(char8_t* pPath8, size_t nPathLength);
            virtual size_t    GetPath(char16_t* pPath16, size_t nPathLength);

            virtual bool      Open(int nAccessFlags = kAccessFlagRead, int nCreationDisposition = kCDDefault, int nSharing = kShareRead, int nUsageHints = kUsageHintNone); 
            virtual bool      Close();
            virtual uint32_t  GetType() const { return kTypeFileStreamWiiNand; }
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

            typedef void (*IStreamCallback)(int32_t result, IStream* pStream, void* pContext);

            bool      ReadAsync(       void* pData, size_type nSize, IStreamCallback callback, void* pContext);
            bool      WriteAsync(const void* pData, size_type nSize, IStreamCallback callback, void* pContext);
            int32_t   WaitAsync() const;
            bool      IsBusy() const;

        protected:
            struct NandFileStreamInfo
            {
                NANDCommandBlock    mCommandBlock;
                NANDFileInfo        mNandInfo;
                FileStreamWiiNand*  mpFileStreamWiiNand;
            };

        protected:
            void Reset();

            static void StaticNandAsyncCallback(s32 result, NANDCommandBlock* pCommandBlock);

        protected:
            NandFileStreamInfo  mFileInfo;                      /// 
            FilePosition        mFilePosition;                  /// file read position information
            int                 mnRefCount;                     /// Reference count, which may or may not be in use.
            int                 mnAccessFlags;                  /// See enum AccessFlags.
            int                 mnCD;                           /// See enum CD (creation disposition).
            int                 mnSharing;                      /// See enum Share.
            int                 mnUsageHints;                   /// See enum UsageHints.
            mutable int         mnLastError;                    /// Used for error reporting.
            char8_t             mpPath8[kMaxPathLength];        /// Path for the file.

            // Async processing data
            int32_t             mAsyncResult;   // Store results of asynchronous processing
            IStreamCallback     mCallback;      // Asynchronous process callback
            void*               mpContext;      // Callback argument
            volatile bool       mIsBusy;        // Tells if we are busy doing an async operation.

            /*
            bool                mCanRead;             // Can read flag
            bool                mCanWrite;            // Can write flag
            volatile bool       mIsBusy;              // busy flag
            bool                mCloseOnDestroyFlg;   // flag indicating whether the file is to be closed when the destructor executes
            bool                mCloseEnableFlg;      // Flag indicating whether a file close is permitted in the FileStream
            */

        }; // class FileStreamWiiNand

    } // namespace IO

} // namespace EA


#endif // Header include guard

