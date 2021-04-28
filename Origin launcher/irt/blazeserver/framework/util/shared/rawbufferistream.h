/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RawBufferIStream

    Simple iStream to manage a uint8_ byte buffer.

*/
/*************************************************************************************************/

#ifndef RAWBUFFERISTREAM_H
#define RAWBUFFERISTREAM_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/
#include "EAIO/EAStream.h"
#include "framework/util/shared/rawbuffer.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class BLAZESDK_API RawBufferIStream : public EA::IO::IStream
{

public:
    RawBufferIStream(RawBuffer &buf) : mBuf(buf) {}

    ~RawBufferIStream() override {};

    // Use as a IStream
    bool Write(const void* pData, EA::IO::size_type nSize) override;
    EA::IO::size_type Read(void* pData, EA::IO::size_type nSize) override;

    EA::IO::size_type GetSize() const override { return mBuf.datasize(); }

private:
    RawBufferIStream& operator=(const RawBufferIStream& otherObj);
private:
    // NO-OP APIs from EA::IO::IStream interface
    int AddRef() override { return 0; };
    int Release() override { return 0; };
    virtual void SetReleaseAllocator(EA::Allocator::ICoreAllocator*) {} 
    uint32_t GetType() const override { return 0; }
    int GetAccessFlags() const override { return 0; }
    int GetState() const override { return 0; }
    bool Close() override { return true; }    
    bool SetSize(EA::IO::size_type size) override { return true; }
    EA::IO::off_type GetPosition(EA::IO::PositionType positionType = EA::IO::kPositionTypeEnd) const override { return mBuf.size(); }
    bool SetPosition(EA::IO::off_type position, EA::IO::PositionType positionType = EA::IO::kPositionTypeEnd) override
    {
        mBuf.mark(); // Save off the data pointer before we reset
        mBuf.reset();
        mBuf.resetToMark(); // Restore the data pointer
        mBuf.put(position); // Set the tail to the new position relative to the head
        return true;
    }
    EA::IO::size_type GetAvailable() const override { return 0; }
    bool Flush() override { return true; }

    RawBuffer& mBuf;
};

} // Blaze

#endif // RAWBUFFERISTREAM_H
