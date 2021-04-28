/*************************************************************************************************/
/*!
    \file

    Stream output interface.  Subclasses can choose to override only write(uint8_t data), or
    override all methods for better performance.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_OUTPUTSTREAM_H
#define BLAZE_OUTPUTSTREAM_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class BLAZESDK_API OutputStream
{
public:
    virtual ~OutputStream() {}

    virtual uint32_t write(uint8_t data) = 0;

    virtual uint32_t write(uint8_t* data, uint32_t length)
    {
        return write(data, 0, length);
    }

    virtual uint32_t write(uint8_t* data, uint32_t offset, uint32_t length)
    {
        int numWritten = 0;

        for (uint32_t i = offset; i < length; i++)
        {
            if (!write(data[i]))
            {
                break;
            }

            numWritten++;
        }

        return numWritten;
    }
};

} // namespace Blaze

#endif // BLAZE_OUTPUTSTREAM_H

