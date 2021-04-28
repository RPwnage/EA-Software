/*************************************************************************************************/
/*!
    \file

    Stream input interface.  Subclasses can choose to override only read(uint8_t data), or
    override all methods for better performance.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_INPUTSTREAM_H
#define BLAZE_INPUTSTREAM_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class BLAZESDK_API InputStream
{
public:
    virtual ~InputStream() {};
    virtual uint32_t available() const = 0;
    virtual bool isRewindable() const = 0;
    virtual void rewind() = 0;

    virtual uint32_t skip(uint32_t n) = 0;

    // Read a single byte of data.  Returns the number of bytes read.  Zero indicates an error.
    virtual uint32_t read(uint8_t* data) = 0;

    // Read bytes into the given buffer
    virtual uint32_t read(uint8_t* data, uint32_t length)
    {
        return read(data, 0, length);
    }

    // Read bytes into the given buffer starting at the specified offset
    virtual uint32_t read(uint8_t* data, uint32_t offset, uint32_t length)
    {
        uint32_t numRead = 0;

        while ((numRead < length) && (read(&data[numRead + offset]) == 1))
        {
            numRead++;
        }

        return numRead;
    }
};

} // namespace Blaze

#endif // BLAZE_INPUTSTREAM_H

