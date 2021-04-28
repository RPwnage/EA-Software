/////////////////////////////////////////////////////////////////////////////
// ValidationFile.cpp
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAIO/EAFileStream.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAStreamAdapter.h>
#include <EAIO/EAStreamBuffer.h>
#include <EAIO/PathString.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EAEndian.h>
#include <EAPatchClientTest/ValidationFile.h>
#include <EATest/EATest.h>


namespace EA
{
namespace Patch
{


bool GenerateValidateFile(const char8_t* pFilePath, uint64_t fileSize, bool /*bOverwriteExistingFile*/, eastl::string& sError)
{
    bool result = false;

    EA::IO::FileStream fileStream(pFilePath);
    fileStream.AddRef();
    EA::IO::StreamBuffer streamBuffer(NULL, 0, NULL, 16384, &fileStream);

    // To do: Implement the bOverwriteExistingFile feature. Currently it always overwrites.

    if(fileStream.Open(EA::IO::kAccessFlagReadWrite, EA::IO::kCDCreateAlways, EA::IO::FileStream::kShareRead))
    {
        uint64_t bytesWritten = 0;

        EA::IO::WriteUint64(&streamBuffer, fileSize, EA::IO::kEndianLittle);
        bytesWritten += sizeof(uint64_t);

        for(uint64_t i = bytesWritten; i < fileSize; i++)
        {
            uint8_t value = (i % kValidationFilePrimeNumber);
            streamBuffer.Write(&value, 1);
        }

        // If the file size was very small, we may have overrun it above. So just chop it.
        if(fileSize < sizeof(uint64_t))
            fileStream.SetSize((EA::IO::size_type)fileSize);

        if(fileStream.GetState())
        {
            result = false;
            sError.sprintf("Error writing \"%s\".\n", pFilePath);
        }
    }
    else
    {
        result = false;
        sError.sprintf("Unable to open \"%s\" for writing.\n", pFilePath);
    }

    return result;
}


bool VerifyValidationFile(const char8_t* pFilePath, eastl::string& sError)
{
    bool result = true;

    uint64_t actualFileSize = EA::IO::File::GetSize(pFilePath);

    if(actualFileSize != EA::IO::kSizeTypeError)
    {
        EA::IO::FileStream fileStream(pFilePath);
        fileStream.AddRef();
        EA::IO::StreamBuffer streamBuffer(NULL, 0, NULL, 16384, &fileStream);

        if(fileStream.Open(EA::IO::kAccessFlagRead, EA::IO::kCDOpenExisting, EA::IO::FileStream::kShareRead))
        {
            uint64_t bytesWritten = 0;

            if(actualFileSize < sizeof(uint64_t))
            {
                // We need to have a special case for this.
                // To do.
            }
            else
            {
                uint64_t expectedFileSize;
                EA::IO::ReadUint64(&streamBuffer, expectedFileSize, EA::IO::kEndianLittle);
                bytesWritten += sizeof(uint64_t);

                if(expectedFileSize != actualFileSize)
                {
                    result = false;
                    sError.sprintf("File size mismatch for \"%s\".\n", pFilePath);
                }

                for(uint64_t i = bytesWritten; (i < actualFileSize) && result; i++)
                {
                    uint8_t expectedValue = (i % kValidationFilePrimeNumber);
                    uint8_t actualValue = 0;
                    streamBuffer.Read(&actualValue, 1);

                    if(expectedValue != actualValue)
                    {
                        result = false;
                        sError.sprintf("Byte mismatch for \"%s\".\n", pFilePath);
                    }
                }
            }

            if(fileStream.GetState())
            {
                result = false;
                sError.sprintf("Error reading \"%s\".\n", pFilePath);
            }
        }
        else
        {
            result = false;
            sError.sprintf("Unable to open \"%s\" for reading.\n", pFilePath);
        }
    }
    else
    {
        result = false;
        sError.sprintf("Unable to open \"%s\" for reading.\n", pFilePath);
    }

    return result;
}



///////////////////////////////////////////////////////////////////////////////
// FakeValidationFile
///////////////////////////////////////////////////////////////////////////////

FakeValidationFile::FakeValidationFile()
  : mRefCount(0)
  , mFilePosition(0)
  //mFileSizeTemp()
  , mFileSize(0)
  , mErrorCount(0)
{
    mFileSizeTemp.mFileSize = 0;
}


void FakeValidationFile::Reset(uint64_t newPosition, uint64_t newSize)
{
    // Leave mRefCount alone.
    mFilePosition = newPosition;
    mFileSize     = newSize;
    mErrorCount   = 0;
}


EA::IO::size_type FakeValidationFile::GetSize() const
{
    return (EA::IO::size_type)mFileSize;
}


bool FakeValidationFile::SetSize(EA::IO::size_type size)
{
    mFileSize = size; // This virtual file can be any size.
    return true;
}

// This function exists because EA::IO::size_type might not be 64 bits, but we need it to be.
bool FakeValidationFile::SetSize64(uint64_t size)
{
    mFileSize = size; // This virtual file can be any size.
    return true;
}


EA::IO::off_type FakeValidationFile::GetPosition(EA::IO::PositionType positionType) const
{
    using namespace EA::IO;

    switch(positionType)
    {
        case kPositionTypeBegin:
            return (off_type)(int64_t)mFilePosition;

        case kPositionTypeEnd:
            return (off_type)(int64_t)(mFilePosition - mFileSize);  // This is a <= 0 value.

        case kPositionTypeCurrent:
        default:
            break;
    }

    return 0; // For kPositionTypeCurrent the result is always zero for a 'get' operation.
}


bool FakeValidationFile::SetPosition(EA::IO::off_type position, EA::IO::PositionType positionType)
{
    using namespace EA::IO;

    switch(positionType)
    {
        case kPositionTypeBegin:
            mFilePosition = (uint64_t)(int64_t)position; // We deal with negative positions below.
            break;

        case kPositionTypeCurrent:
            mFilePosition = (uint64_t)(mFilePosition + (int64_t)position);  // We have a signed/unsigned match, but the math should work anyway.
            break;

        case kPositionTypeEnd:
            mFilePosition = (uint64_t)(mFileSize + (int64_t)position); // We deal with invalid resulting positions below.
            break;
    }

    if(mFileSize < mFilePosition)
        mFileSize = mFilePosition;

    return true;
}


EA::IO::size_type FakeValidationFile::GetAvailable() const
{
    return (EA::IO::size_type)(mFileSize - mFilePosition);
}


EA::IO::size_type FakeValidationFile::Read(void* pData, EA::IO::size_type nSize)
{
    uint64_t nSize64 = eastl::min_alt(mFileSize - mFilePosition, (uint64_t)nSize);
    uint8_t* pData8  = static_cast<uint8_t*>(pData);

    for(uint64_t i = 0, f = mFilePosition; i < nSize64; f++, i++)
        pData8[i] = (uint8_t)(f % kValidationFilePrimeNumber);
    
    if(mFilePosition < sizeof(uint64_t)) // The first 8 bytes of a ValidationFile is a uint64_t: the file size in little-endian.
    {
        FileSizeBytes fsb = { EA::StdC::ToLittleEndian(mFileSize) };

        for(uint64_t i = 0, f = mFilePosition, fEnd = eastl::min_alt(mFileSize, (uint64_t)sizeof(uint64_t)); (f < fEnd) && (i < nSize64); f++, i++)
            pData8[i] = fsb.mFileSize8[f];
    }

    mFilePosition += nSize64;

    return (EA::IO::size_type)nSize64;
}


bool FakeValidationFile::Write(const void* pData, EA::IO::size_type nSize)
{
    // We return false if the user attempts to write a wrong byte.
    bool bSuccess = true;
    const uint8_t* pData8 = static_cast<const uint8_t*>(pData);

    if(mFilePosition < sizeof(uint64_t)) // If writing to the beginning of the file, where the uint64_t size is...
    {
        while(nSize && (mFilePosition < sizeof(uint64_t)))
        {
            mFileSizeTemp.mFileSize8[mFilePosition++] = *pData8++;
            nSize--;
        }

        if(mFilePosition == sizeof(uint64_t))
        {
            mFileSizeTemp.mFileSize = EA::StdC::ToLittleEndian(mFileSizeTemp.mFileSize);

            if(mFileSize != 0) // If the expected file size has been pre-set (allowing us to validate it here)...
            {
                if(mFileSize != mFileSizeTemp.mFileSize)
                {
                    bSuccess = false;
                    EA::UnitTest::Report("FakeValidationFile::Write: mFileSize != mFileSizeTemp.mFileSize (%I64u != %I64u)\n", mFileSize, mFileSizeTemp.mFileSize);
                }
            }
        }

        // Fall through and validate any remaining data.
    }

    if(nSize) // If there is anything left to write...
    {
        // If we are here then mFileSizeTemp.mFileSize has been fully written and is usable.
        for(uint64_t i = 0; (i < nSize) && bSuccess; i++, mFilePosition++)
        {
            if(pData8[i] != (mFilePosition % kValidationFilePrimeNumber))
            {
                bSuccess = false;
                EA::UnitTest::Report("FakeValidationFile::Write: pos: %I64u, expected: 0x%x, actual: 0x%x\n", mFilePosition, mFilePosition % kValidationFilePrimeNumber, pData8[i]);
            }
        }

        if(mFileSize < mFilePosition) // If we need to resize the file to accomodate the write...
            mFileSize = mFilePosition; // To consider: Enable a test mode where we don't allow the write to go past the expected mFileSize.
    }

    return bSuccess;
}


} // namespace Patch

} // namespace EA








