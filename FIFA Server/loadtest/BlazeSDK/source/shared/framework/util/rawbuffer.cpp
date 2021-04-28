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

    Simple class to manage a uint8_ byte buffer.

*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/util/shared/rawbuffer.h"
#include "EAStdC/EAString.h"

namespace Blaze
{

/*** Public Methods ******************************************************************************/

RawBuffer::RawBuffer()
    : mOwnMem(false)
#ifndef BLAZE_CLIENT_SDK
    , mRefCount(0)
#endif
{
    // Always set the 'hidden' last byte to '\0'
    mBytes[DEFAULT_SIZE] = '\0';
    setBuffer(nullptr, 0);
}

RawBuffer::RawBuffer(uint8_t* buf, size_t size, bool takeOwnership /* = false */)
    : mOwnMem(takeOwnership)
#ifndef BLAZE_CLIENT_SDK
    , mRefCount(0)
#endif
{
    // Always set the 'hidden' last byte to '\0'
    mBytes[DEFAULT_SIZE] = '\0';
    setBuffer(buf, size);
}

RawBuffer::RawBuffer(size_t size)
    : mOwnMem(true)
#ifndef BLAZE_CLIENT_SDK
    , mRefCount(0)
#endif
{
    uint8_t* mem = nullptr;

    if (size <= DEFAULT_SIZE)
    {
        mem = mBytes;
        size = DEFAULT_SIZE;
    }
    else
    {

        // RawBuffer is sometimes used for storing text data, and cast to char*
        // to perform string operations on said text data; therefore, we add a
        // 'hidden' byte that ensures that the buffer is always '\0' terminated, 
        // so operations like strstr((char*)buffer.data(), "somestring") 
        // are guaranteed to work correctly without reading memory out of bounds.
        mem = (uint8_t*) BLAZE_ALLOC_MGID(size + 1, MEM_GROUP_FRAMEWORK_RAWBUF, "RAWBUFFER_ALLOC");
    }

    // Always set the 'hidden' last byte to '\0'
    mem[size] = '\0';
    setBuffer(mem, size);
}

RawBuffer::~RawBuffer()
{
    if (mOwnMem && mHead != mBytes)
    {
        BLAZE_FREE_MGID(MEM_GROUP_FRAMEWORK_RAWBUF, mHead);
    }
}

void RawBuffer::setBuffer(uint8_t* buf, size_t len)
{
    mHead = buf;
    mData = buf;
    mTail = buf;
    mEnd = buf != nullptr ? buf + len : nullptr;
    mMark = buf;
}

/*** Private Methods ******************************************************************************/

/*! \brief Try and make space for the requested number of bytes
 *
 * \return Return pointer to the tail on success; nullptr on failure
 */
uint8_t* RawBuffer::expand(size_t size)
{
    if (EA_UNLIKELY(!mOwnMem))
    {
        // We don't own the memory so we can't accomodate the request
        return nullptr;
    }

    size_t newSize = 0;
    if (mEnd != nullptr)
    {
        newSize = static_cast<size_t>(mEnd - mHead); 
    }
    if (newSize == 0)
    {
        // Buffer is empty, reserve the requested size
        newSize = size;
    }
    else
    {
        // Grow the buffer in increments to try and avoid unnecessary subsequent allocations
        size_t available = 0;
        if (mEnd != nullptr)
        {
            available = static_cast<size_t>(mEnd - mTail); 
        }

        do
        {
            available += newSize;
            newSize *= 2;
        } while (size > available);
    }
    
    // RawBuffer is sometimes used for storing text data, and cast to char*
    // to perform string operations on said text data; therefore, we add a
    // 'hidden' byte that ensures that the buffer is always '\0' terminated, 
    // so operations like strstr((char*)buffer.data(), "somestring") 
    // are guaranteed to work correctly without reading memory out of bounds.
    uint8_t* newBuf = (uint8_t*) BLAZE_ALLOC_MGID(newSize + 1, MEM_GROUP_FRAMEWORK_RAWBUF, "RAWBUFFER_REALLOC");
    if (EA_UNLIKELY(newBuf == nullptr))
        return nullptr;

    // Copy the existing data to the new buffer
    if (mHead != nullptr && mTail != nullptr)
    {
        memcpy(newBuf, mHead, static_cast<size_t>(mTail - mHead));
    }

    // Always set the 'hidden' last byte to '\0'
    newBuf[newSize] = '\0';

    //toss the old memory
    if (mHead != mBytes)
    {
        BLAZE_FREE_MGID(MEM_GROUP_FRAMEWORK_RAWBUF, mHead);
    }

    // Fixup all the pointers
    if (mData != nullptr && mHead != nullptr && mTail != nullptr && mMark != nullptr)
    {
        mData = newBuf + (mData - mHead);
        mTail = newBuf + (mTail - mHead);
        mMark = newBuf + (mMark - mHead);
    }
    else
    {
        mData = newBuf;
        mTail = newBuf;
        mMark = newBuf;
    }

    //set the start and end
    mHead = newBuf;
    mEnd = newBuf + newSize;   

    return mTail;
}



} // namespace Blaze
