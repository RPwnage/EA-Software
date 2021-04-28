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

#ifndef RAWBUFFER_H
#define RAWBUFFER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

#ifndef BLAZE_CLIENT_SDK
#include "EASTL/intrusive_ptr.h"
#include "coreallocator/detail/macro_impl.h" 
#else
#include "BlazeSDK/blazesdk.h" 
#endif

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class RawBufferPool;

/*
  head()                                    capacity()                                        end()
    |-------------------------------------------------------------------------------------------|
    |                                                                                           |
    |              data()            datasize()              tail()                             |
    |                |------------------------------------------|                               |
    |   headroom()                                                          tailroom()
    |----------------|                                          |-------------------------------|
*/
class BLAZESDK_API RawBuffer
{
    NON_COPYABLE(RawBuffer);
public:
    RawBuffer();
    RawBuffer(uint8_t* buf, size_t len, bool takeOwnership = false);
    RawBuffer(size_t size);
    ~RawBuffer();
    void setBuffer(uint8_t* buf, size_t len);

    // The headroom() is the number of bytes between the beginning of the memory block, and where the data() begins.
    //
    // e.g. { return mData - mHead; }
    size_t headroom() const { return (size_t)(mData - mHead); }

    // The tailroom() is the number of bytes between the last byte of relevant data, and end() of the memory block.
    //
    // e.g. { return mEnd - mTail; }
    size_t tailroom() const { return (size_t)(mEnd - mTail); }

    // Sets data() and tail() equal to head() + len.
    // Note: This does not actually do any memory (re)allocation, or modify the currently allocated memory in any way.
    //
    // e.g. { mData = mTail = mHead + len; }
    void reserve(size_t len) { mData = mTail = mHead + len; }

    // Moves the data() pointer back by 'len' bytes, and then returns the new position of data().
    //
    // e.g. { mData -= len; return mData; }
    uint8_t* push(size_t len) { mData -= len; return mData; }

    // Moves the data() pointer forward by 'len' bytes, and then returns the new position of data().
    //
    // e.g. { mData += len; return mData; }
    uint8_t* pull(size_t len) { mData += len; return mData; }

    // Moves the tail() pointer forward by 'len' bytes, and then returns the new position of tail().
    //
    // e.g. { mTail += len; return mTail; }
    uint8_t* put(size_t len) { mTail += len; return mTail; }

    // Sets data() and tail() equal to the begining of the memory block pointed to by head(), effectively resetting the
    // RawBuffer's members/pointer back to their original positions.  It then returns a reference to itself.
    // Note: This does not actually do any memory (re)allocation, or modify the currently allocated memory in any way.
    //
    // e.g. { mData = mTail = mHead; return *this; }
    RawBuffer& reset() { mData = mTail = mHead; return *this; }

    // Moves the tail() pointer back by 'len' bytes.
    //
    // e.g. { mTail -= len; }
    void trim(size_t len) { mTail -= len; }

    // Eliminates the tailroom().
    // Note: This does not actually do any memory (re)allocation, or modify the currently allocated memory in any way.
    //
    // e.g. { mEnd = mTail; }
    // TODO_MC: This is a strange method to have.  It feels like mHead and mEnd should always point to the beginning and ending of the allocated memory block.
    void trimEnd() { mEnd = mTail; }

    // Returns the total capacity of the RawBuffer. Effectively, the total size of the allocated memory block.
    //
    // e.g. { return mEnd - mHead; }
    size_t capacity() const { return (size_t)(mEnd - mHead); }

    // Returns the number of bytes between the beginning of the memory block (head), and the last byte of relevant data (tail).
    //
    // e.g. { return mTail - mHead; }
    size_t size() const { return (size_t)(mTail - mHead); }

    // Returns the number of bytes that are considered to be the relevant 'data' in the RawBuffer.  When passed to encoders or other
    // transport mechanisms, this is the amount of data that is typically dealt with.
    //
    // e.g. { return mTail - mData; }
    size_t datasize() const { return (size_t)(mTail - mData); }

    // This is the beginning of the relevant data segment within the allocated memory block.  This is not necessarily the same as the
    // first by in the allocated memory block pointed to by head(). The data() member is used differently depending on the context.
    // 
    // Typically, algorithms that read data out of the RawBuffer will use data() as the current read position, and then
    // call pull() to move the current read position (e.g. mData) forward in the RawBuffer.
    //
    // e.g. { return mData; }
    uint8_t* data() const { return mData; }

    // Typically, algorithms that write data in to the RawBuffer will use tail() as the current write position, and then
    // call put() to move the current write position (e.g. mTail) forward in the RawBuffer.
    //
    // e.g. { return mTail; }
    uint8_t* tail() const { return mTail; }

    // Returns the beginning of the allocated memory block. The head() only changes if the memory block is reallocated via a call to
    // acquire() resulting in an internal call to expand().  The memory pointed to by the RawBuffer must be owned for this to ever change.
    //
    // e.g. { return mHead; }
    uint8_t* head() const { return mHead; }

    // Returns the end of the memory block - that is, the last available byte in the RawBuffer +1.
    //
    // e.g. { return mEnd; }
    uint8_t* end() const { return mEnd; }

    // Saves the current value of data(). Use resetToMark() to bring data() back to the saved location.
    //
    // e.g. { mMark = mData; }
    void mark() { mMark = mData; }

    // Sets data() back to the memory location originally saved by a call to mark().
    //
    // e.g. { mData = mMark; }
    void resetToMark() { mData = mMark; }

    // Ensures there is at least 'size' bytes available in the tail() of the RawBuffer. If there is not enough room
    // in the tail() -- that is, tailroom() < size -- then the allocated memory block is reallocated via an internal call to expand(size).
    // Note: If the memory pointed to by this RawBuffer is not owned, and the result of this call requires the memory to be expanded,
    //       then it is a no-op and nullptr is returned.
    //
    // e.g. { return (size <= tailroom()) ? mTail : expand(size); }
    uint8_t* acquire(size_t size) { return (size <= tailroom()) ? mTail : expand(size); }

private:
    static const uint32_t DEFAULT_SIZE = 1024;
    uint8_t* mHead;
    uint8_t* mData;
    uint8_t* mTail;
    uint8_t* mEnd;
    uint8_t* mMark;
    bool mOwnMem;
    uint8_t mBytes[DEFAULT_SIZE+1];

    uint8_t* expand(size_t size);

#ifndef BLAZE_CLIENT_SDK
public:
    void* operator new(size_t size, EA::Allocator::detail::opNonArrayNew, 
        EA::Allocator::ICoreAllocator* allocator, const char*, unsigned int)
    {
        return operator new(size);
    }
    void operator delete(void* p, EA::Allocator::detail::opNonArrayNew, 
        EA::Allocator::ICoreAllocator* allocator, const char*, unsigned int)
    {
        operator delete(p);
    }
    void* operator new(size_t size);
    void operator delete(void *p);

    friend class RawBufferPool;

private:
    //intrusive_ptr stuff
    friend void intrusive_ptr_add_ref(RawBuffer *);
    friend void intrusive_ptr_release(RawBuffer *);
    
    uint32_t mRefCount;
#endif

};

#ifndef BLAZE_CLIENT_SDK

//intrusive_ptr stuff
inline void intrusive_ptr_add_ref(RawBuffer *buf)
{
    buf->mRefCount++;
}

inline void intrusive_ptr_release(RawBuffer *buf)
{
    if (--buf->mRefCount == 0)
    {
        delete buf;
    }
}

typedef eastl::intrusive_ptr<RawBuffer> RawBufferPtr;
#endif


} // Blaze

#endif // RAWBUFFER_H

