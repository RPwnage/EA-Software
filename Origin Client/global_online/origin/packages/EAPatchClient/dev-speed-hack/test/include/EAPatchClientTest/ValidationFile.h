/////////////////////////////////////////////////////////////////////////////
// ValidationFile.h
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// ValidationFile
// -----------------
// A ValidationFile is a file whose contents are used solely to validate the 
// integrity of the file. Whereas a checksum or hash is a single number used 
// to validate the contents of an entire file, every byte of a ValidationFile's
// contents verify the file's contents, without needing to see the rest of 
// the file. It specifies that the 375'th byte of a given file is always a
// predetermined value.
//
// FakeValidationFile
// -----------------
// A FakeValidationFile is a virtual ValidationFile that doesn't actually
// exist on disk. A web server could send you a fakeValidationFile by simply
// fabricating its contents as it sends you the data, as opposed to reading
// the data from a physical disk file.
//
// ValidationFile format
// -----------------
// <uint64_t><uint8_t><uint8_t><uint8_t>....
//
// The first uint64_t (first 8 bytes) is the size of the file, little-endian encoded.
// Each uint8_t is the sbsolute position of the uint8_t in the file mod 251, and thus
// will be a value in the range of [0, 250].
// If the file is less than 8 bytes in size, then the uint64_t bytes are chopped.
//     This happens to work because the file is little endian and so if the file is 
//     1 byte in size then the file consists of just the first byte of the 8 uint64_t
//     bytes, and it has a value of 1.
//
// Example 10 byte file:
//     0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x09
//
// Example 2 byte file:
//     0x02, 0x00
//
// Example 500 byte file:                                                    (0xfa == 250)          (pos 499) 
//     0xf4, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x0a, .... 0xfa, 0x00, ... 0xf7, 0xf8
//
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCH_VALIDATIONFILE_H
#define EAPATCH_VALIDATIONFILE_H


#include <EAPatchClient/Base.h>
#include <EAIO/EAFileStream.h>


/// ValidationFiles by convention use the following file name extension. 
/// This isn't required, but it makes it easy to identify such files, 
/// the same as with other file formats and the name extensions they use.
#define kValidationFileExtension         ".validationFile"
#define kValidationFileExtensionPattern "*.validationFile"

/// FakeValidationFiles by convention use the following file name extension.
#define kFakeValidationFileExtension         ".fakeValidationFile"
#define kFakeValidationFileExtensionPattern "*.fakeValidationFile"

// This is the largest prime under 256. It's not critical that this be a prime number, 
// though it's important that it's not a very even number like a power of 2 or multiple
// of something. This reduces the chance of some bug coincidentally making the written
// file look correct when it isn't.
static const uint64_t kValidationFilePrimeNumber = 251;


namespace EA
{
    namespace Patch
    {
        /// GenerateValidateFile
        /// Writes a ValidationFile to the given file path.
        bool GenerateValidateFile(const char8_t* pFilePath, uint64_t fileSize, bool bOverwriteExistingFile, eastl::string& sError);

        /// VerifyValidationFile
        /// Verifies the correctness of a ValidationFile at the given file path.
        bool VerifyValidationFile(const char8_t* pFilePath, eastl::string& sError);



        /// FakeValidationFile
        /// Implements a virtual ValidationFile which isn't actually backed by contents on disk.
        /// It supports reading from and writing to. When being read from, it generates proper
        /// ValidationFile contents. When being written to, it verifies that it is being given
        /// proper ValidationFile contents as it goes.
        /// 
        class FakeValidationFile : public EA::IO::IStream
        {
        public:
            enum { kTypeFakeValidationFile = 0x0e5cc420 };

            FakeValidationFile();

            int AddRef()
                { return ++mRefCount; }

            int Release()
                { return --mRefCount; }

            uint32_t GetType() const
                { return kTypeFakeValidationFile; }

            int GetAccessFlags() const
                { return EA::IO::kAccessFlagReadWrite; }

            int GetState() const
                { return mErrorCount; }

            bool Close()
                { return true; }

            bool Flush()
                { return true; }

            void Reset(uint64_t newPosition = 0, uint64_t newSize = 0);

            EA::IO::size_type GetSize() const;
            bool              SetSize(EA::IO::size_type size);
            bool              SetSize64(uint64_t size);

            EA::IO::off_type GetPosition(EA::IO::PositionType positionType = EA::IO::kPositionTypeBegin) const;
            bool             SetPosition(EA::IO::off_type position, EA::IO::PositionType positionType = EA::IO::kPositionTypeBegin);

            EA::IO::size_type GetAvailable() const;

            EA::IO::size_type Read(void* pData, EA::IO::size_type nSize);

            bool Write(const void* pData, EA::IO::size_type nSize);

        protected:
            union FileSizeBytes {
                uint64_t mFileSize;
                uint8_t  mFileSize8[sizeof(uint64_t)];
            };

        protected:
            int           mRefCount;        /// 
            uint64_t      mFilePosition;    /// Virtual current file position.
            FileSizeBytes mFileSizeTemp;    /// Buffer for incoming bytes from the first 8 bytes of the file.
            uint64_t      mFileSize;        /// Starts out at zero, gets filled in when all 8 bytes of mFileSizeTemp are written.
            int           mErrorCount;      /// 
        };

    } // namespace Patch

} // namespace EA




#endif // Header include guard
