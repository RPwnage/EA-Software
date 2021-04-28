/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RawBuffer

    Simple iStream to manage a uint8_ byte buffer.

*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/util/shared/rawbufferistream.h"

namespace Blaze
{

    /*** Public Methods ******************************************************************************/

    // Use as a IStream
    bool RawBufferIStream::Write(const void* pData, EA::IO::size_type nSize)
    {
        char8_t* t = (char8_t*)mBuf.acquire(nSize+1);
        if (t != nullptr)
        {
            if (pData != nullptr)
                memcpy(t, pData, nSize);
            t[nSize] = '\0';
            mBuf.put(nSize);
        }
        else
            return false;

        return true;
    }

    EA::IO::size_type RawBufferIStream::Read(void* pData, EA::IO::size_type nSize)
    {
        EA_ASSERT(nSize > 0);
        const EA::IO::size_type bytesAvailable = (EA::IO::size_type)mBuf.datasize();
        if (bytesAvailable > 0)
        {
            if (nSize > bytesAvailable)
                nSize = bytesAvailable;

            const void* bufData = mBuf.data();
            memcpy(pData, bufData, nSize);
            mBuf.pull(nSize);

            return nSize;
        }
        return 0; // EOF
    }
} // namespace Blaze
