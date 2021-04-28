/*************************************************************************************************/
/*!
    \file

    Stream input interface.  Subclasses can choose to override only read(uint8_t data), or
    override all methods for better performance.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_BYTEARRAYOUTPUTSTREAM_H
#define BLAZE_BYTEARRAYOUTPUTSTREAM_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

#include "EASTL/algorithm.h"
#include "framework/util/shared/outputstream.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class BLAZESDK_API ByteArrayOutputStream : public OutputStream
{
public:
    ByteArrayOutputStream(uint8_t* data, uint32_t length) : mData(data), mLength(length), mPosition(0)
    {
    }

    uint32_t write(uint8_t data) override
    {
        if (available() > 0)
        {
            mData[mPosition++] = data;
            return 1;
        }
        return 0;
    }

    uint32_t write(uint8_t* data, uint32_t length) override
    {
        return write(data, 0, length);
    }

    uint32_t write(uint8_t* data, uint32_t offset, uint32_t length) override
    {
        if (data == nullptr)
        {
            return 0;
        }
        uint32_t numToWrite = eastl::min<uint32_t>(available(), length);

        if (numToWrite > 0)
        {
            memcpy(&mData[mPosition], &data[offset], numToWrite);
            mPosition += numToWrite;
        }
        return numToWrite;
    }

private:
    uint32_t available() { return (mLength - mPosition); }

private:
    uint8_t* mData;
    uint32_t mLength;
    uint32_t mPosition;
};

} // namespace Blaze

#endif // BLAZE_BYTEARRAYOUTPUTSTREAM_H

