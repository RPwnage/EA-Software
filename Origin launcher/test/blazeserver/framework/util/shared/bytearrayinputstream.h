/*************************************************************************************************/
/*!
    \file

    Stream input interface.  Subclasses can choose to override only read(uint8_t data), or
    override all methods for better performance.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_BYTEARRAYINPUTSTREAM_H
#define BLAZE_BYTEARRAYINPUTSTREAM_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

#include "EASTL/algorithm.h"
#include "framework/util/shared/inputstream.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class BLAZESDK_API ByteArrayInputStream : public InputStream
{
public:
    ByteArrayInputStream(const uint8_t* data, uint32_t length) : mData(data), mLength(length), mPosition(0)
    {
    }

    uint32_t available() const override { return (mLength - mPosition); }
    bool isRewindable() const override { return true; }
    void rewind() override { mPosition = 0; }

    // Skip n bytes if possible.  Return the number of bytes actually skipped.
    uint32_t skip(uint32_t n) override
    {
        uint32_t numToSkip = eastl::max<uint32_t>(available(), n);
        mPosition += numToSkip;
        return numToSkip;
    }

    // Read a single byte of data.  Returns the number of bytes read.  Zero indicates an error.
    uint32_t read(uint8_t* data) override
    {
        if (data == nullptr)
        {
            return 0;
        }
        if (available() > 0)
        {
            *data = mData[mPosition];
            ++mPosition;
            return 1;
        }
        else
        {
            return 0;
        }
    }

    uint32_t read(uint8_t* data, uint32_t length) override
    {
        return read(data, 0, length);
    }

    // Read bytes into the given buffer starting at the specified offset
    uint32_t read(uint8_t* data, uint32_t offset, uint32_t length) override
    {
        if (data == nullptr)
        {
            return 0;
        }
        uint32_t numToRead = eastl::max<uint32_t>(available(), length);

        if (numToRead > 0)
        {
            memcpy(&data[offset], &mData[mPosition], numToRead);
            mPosition += numToRead;
            return numToRead;
        }
        else
        {
            return 0;
        }
    }

private:
    const uint8_t* mData;
    uint32_t mLength;
    uint32_t mPosition;
};

} // namespace Blaze

#endif // BLAZE_BYTEARRAYINPUTSTREAM_H

