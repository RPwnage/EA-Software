/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


#include <EATDF/tdfblob.h>
#include <EATDF/typedescription.h>

#include <memory.h>
#include <EAAssert/eaassert.h>

namespace EA
{
namespace TDF
{

    const char8_t* TdfBlob::DEFAULT_BLOB_ALLOCATION_NAME = "BLOBDATA";
    const TypeDescription& TdfBlob::getTypeDescription() const { return TypeDescriptionSelector<TdfBlob>::get(); }

    TdfBlob::TdfBlob(EA::Allocator::ICoreAllocator& allocator, const char8_t* debugMemName) : 
        TdfObject(),
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        TdfChangeTracker(),
#endif
        mSize ( 0    ),
        mData ( nullptr ),
        mCount( 0    ),
        mOwnsMem(true),
        mAllocator(allocator)        
    {
    }


    TdfBlob::TdfBlob(const TdfBlob& rhs, const char8_t* allocationName) 
        : TdfObject(),
#if EA_TDF_INCLUDE_CHANGE_TRACKING
          TdfChangeTracker(),
#endif
          mSize(rhs.mCount), //perfectly size down to the exact count of items.
          mData(nullptr),
          mCount(rhs.mCount),
          mOwnsMem(true),
          mAllocator(rhs.mAllocator)          
    {
        if (mCount > 0)
        {
            mData = (uint8_t*) CORE_ALLOC(&mAllocator, mCount, allocationName, 0);
            memcpy(mData, rhs.mData, mCount);
        }        
    }

    void TdfBlob::release()
    {
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        markSet();
#endif //EA_TDF_INCLUDE_CHANGE_TRACKING

        // reset the blob
        if (mOwnsMem && mData != nullptr)
        {
            CORE_FREE(&mAllocator, mData);
        }
        
        mData = nullptr;
        mSize = 0;
        mCount = 0;
    }

// Base 64 translation table as described in RFC1113
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Decode translation table
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

/*************************************************************************************************/
/*!
    \brief encodeBase64Block

    Encode 3 8-bit binary bytes as 4 '6-bit' characters
*/
/*************************************************************************************************/
void TdfBlob::encodeBase64Block(uint8_t in[3], char8_t* out, TdfSizeVal len)
{
    *out = cb64[ in[0] >> 2 ];
    *(out + 1) = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xF0) >> 4) ];
    *(out + 2) = (uint8_t) (len > 1 ? cb64[ ((in[1] & 0x0F) << 2) | ((in[2] & 0xC0) >> 6) ] : '=');
    *(out + 3) = (uint8_t) (len > 2 ? cb64[ in[2] & 0x3F ] : '=');
}

/*************************************************************************************************/
/*!
    \brief decodeBase64Block

    Decode 4 6-bit characters into 3 8-bit binary bytes
*/
/*************************************************************************************************/
void TdfBlob::decodeBase64Block(uint8_t in[4], uint8_t* out)
{   
    *out = (uint8_t)(in[0] << 2 | in[1] >> 4);
    *(out + 1) = (uint8_t)(in[1] << 4 | in[2] >> 2);
    *(out + 2) = (uint8_t)(((in[2] << 6) & 0xC0) | in[3]);
}


/*************************************************************************************************/
/*!
    \brief encode

    Encode the given binary data into base 64 format and output the encoded data into the
    char buffer.

    \param[in] the output buffer
    \param[in] the size of output buffer (only work if outputBufSize >= calcBase64EncodedSize() + 1, and the 1 extra space is for \0)
    \return - The number of characters written to the char buffer, not including the null
                character; 0 on empty blob; -1 on error or overflow
*/
/*************************************************************************************************/
int32_t TdfBlob::encodeBase64(char8_t* output, size_t outputBufSize) const
{
    const char8_t* input = (const char8_t*)mData;
    TdfSizeVal inputBufSize = mCount;
    return encodeBase64(input, inputBufSize, output, outputBufSize);
}
int32_t TdfBlob::encodeBase64(const char8_t* input, TdfSizeVal inputBufSize, char8_t* output, size_t outputBufSize)
{
    uint8_t in[3];    
    int32_t blockLen;
    int32_t numEncoded = 0;

    size_t outputLength = calcBase64EncodedSize(inputBufSize);    
    // Check if blob is empty or the size of output buffer is insufficient
    if ((input == nullptr) || (outputLength >= outputBufSize))
    {
        if (outputBufSize > 0)
        {
            *output = '\0';
        }
        return ((input == nullptr) ? 0 : -1);
    }

    size_t paddedLength = inputBufSize;
    size_t remainder = inputBufSize % 3;
    if (remainder > 0)
        paddedLength += 3 - remainder;

    for (TdfSizeVal j = 0; j < paddedLength; j += 3)
    {
        blockLen = 0;
        for (TdfSizeVal i = 0; i < sizeof(in); i++)
        {
            if ((j + i) < inputBufSize)
            {
                in[i] = *input;
                input++;
                blockLen++;
            }
            else
            {
                in[i] = 0;
            }
        }

        if (blockLen)
        {
            encodeBase64Block(in, output+numEncoded, blockLen);
            numEncoded += 4;
        }
    }
    // append \0 to the end of buffer
    *(output + numEncoded) = '\0';
    return numEncoded;
}

size_t TdfBlob::calcBase64EncodedSize() const
{
    return calcBase64EncodedSize(mCount);
}
size_t TdfBlob::calcBase64EncodedSize(TdfSizeVal inputBufSize)
{
    if (inputBufSize == 0)
        return 0;

    return (inputBufSize / 3 + ((inputBufSize % 3) ? 1 : 0)) * 4;
}

/*************************************************************************************************/
/*!
    \brief decode

    Decode a base64 encoded stream discarding padding, line breaks and noise.

    \param[in] the input buffer
    \param[in] the number of bytes need to be decoded
    \return - The number of characters written to the output buffer, not including the null
    character; -1 on error
*/
/*************************************************************************************************/
int32_t TdfBlob::decodeBase64(const char8_t* input, TdfSizeVal inputBufSize)
{
    release();

    if (inputBufSize % 4 != 0)
    {
        return -1;
    }

    // input buffer size is always larger than decoded buffer size
    resize(inputBufSize);
    mCount = (TdfSizeVal)decodeBase64(input, inputBufSize, (char8_t**)&mData, nullptr);
    return mCount;
}
int32_t TdfBlob::decodeBase64(const char8_t* input, TdfSizeVal inputBufSize, char8_t** output, size_t* outputBufSize)
{
    if (inputBufSize % 4 != 0)
    {
        return -1;
    }

    uint8_t in[4], v = 0;
    TdfSizeVal i = 0;
    TdfSizeVal len = 0;
    TdfSizeVal numDecoded = 0;
    
    size_t availableInputSize = inputBufSize;
    while (availableInputSize > 0)
    {
        for (len = 0, i = 0; i < 4 ; i++)
        {
            v = 0;
            while ((availableInputSize > 0) && (v == 0))
            {
                v = *input;
                v = (uint8_t)((v < 43 || v > 122) ? 0 : cd64[v - 43]);
                if (v)
                {
                    v = (uint8_t)((v == '$') ? 0 : v - 61);
                }
                availableInputSize--;
            }

            if (v)
            {
                len++;
                in[i] = (uint8_t)(v - 1);
                input++;                
            }
            else
            {
                in[i] = 0;
            }
        }

        if (len)
        {
            decodeBase64Block(in, (uint8_t*)((*output) + numDecoded));
            numDecoded += len - 1;
        }
    }

    if (outputBufSize)
        *outputBufSize = numDecoded;
    return (int32_t)numDecoded;
}

    /*! *****************************************************************************************/
    /*! \brief set the given length new data for this blob.

        Caller need to make sure len is the real size of value

        \param[in] value the raw data to be set
        \param[in] len size of the raw data passed in
        \param[in] allocator - New buffer table to use for data.
        \return    true if the init/update blob succeeds, false if alloc in resize failed
    **********************************************************************************************/
    bool TdfBlob::setData(const uint8_t* value, TdfSizeVal len, const char8_t* allocationName)
    {
        if (value == mData) //no-op
        {
            return true;
        }

        if (value == nullptr || len == 0)
        {
            release();

            return true;
        }

        if (mOwnsMem && len < mSize)
        {
            //we don't need to grow, just copy
            memcpy(mData, value, len);
            mCount = len;
        }
        else
        {
            release();
            mData = (uint8_t*) CORE_ALLOC(&mAllocator, len, allocationName, 0);
            memcpy(mData, value, len);
            mSize = mCount = len;
            mOwnsMem = true;
        }

#if EA_TDF_INCLUDE_CHANGE_TRACKING
        markSet();
#endif //EA_TDF_INCLUDE_CHANGE_TRACKING

        return true;
    }

    bool TdfBlob::assignData(uint8_t* value, TdfSizeVal len)
    {
        if (value == mData) //no-op
        {
            return true;
        }

        release();

        mData = value;
        mSize = mCount = len;
        mOwnsMem = false;

#if EA_TDF_INCLUDE_CHANGE_TRACKING
        markSet();
#endif //EA_TDF_INCLUDE_CHANGE_TRACKING

        return true;
    }


    /*! *****************************************************************************************/
    /*! \brief Grow/Shrink the blob using given size.  The contents of the blob may be
          preserved if (newSize >= mCount)

         Caller need to make sure newSize is bigger than mCount if toPreserve flag is set
         If user set the toPreserve flag when (newSize < mCount), resize will still work but the old data won't be copied

         The blob won't be modified if the resize failed in any case (user passes in newSize 0 or alloc fails)

        \param[in] newSize size of the new blob
        \param[in] toPreserve flag to indicate if the old blob data needs to keep when blob grows
        \param[in] allocationName - Name to use for any allocations.
        \return true if the resize succeeds or the new size is same as current size and no need
          to do a resize action, false if alloc failed
    **********************************************************************************************/
    bool TdfBlob::resize(TdfSizeVal newSize, bool toPreserve /*= false */, const char8_t* allocationName /* = DEFAULT_BLOB_ALLOCATION_NAME*/)
    {
        if (toPreserve && newSize < mCount)
        {
            EA_FAIL_MESSAGE("Requested blob size is less than necessary size to preserve data.");
            return false;
        }

        if (newSize == 0)
            return false;

        // No need to do resize
        if (newSize == mSize)
            return true;

        if (!mOwnsMem)
        {
            EA_FAIL_MESSAGE("Attempting to resize a Blob whose memory is not owned. (Call setData to reassign data ownership.)");
            return false;
        }

        uint8_t* newData = (uint8_t*) CORE_ALLOC(&mAllocator, newSize, allocationName, 0);

        if (newData == nullptr)
            return false;

        if (mData != nullptr)
        {
            if(toPreserve)
            {
                memcpy(newData, mData, mCount);
            }
            else
            {
                mCount = 0;
            }

            if (mOwnsMem)
                CORE_FREE(&mAllocator, mData);
        }

        mData = newData;
        mSize = newSize;
        return true;
    }

    TdfBlob* TdfBlob::clone(EA::Allocator::ICoreAllocator& allocator, const char8_t* allocationName) const
    {
        TdfBlob* newBlob = static_cast<TdfBlob*>(createInstance<TdfBlob>(allocator, allocationName));
        copyInto(*newBlob);
        return newBlob;
    }

    // Duplicate this blob into the newBlob, which is already allocated
    void TdfBlob::copyInto(TdfBlob& newBlob, const MemberVisitOptions&, const char8_t* allocationName) const
    {
        if (this == &newBlob)
        {
            return;
        }

#if EA_TDF_INCLUDE_CHANGE_TRACKING
       newBlob.markSet();
#endif
        if (mCount > 0)
        {
            newBlob.resize(mSize, false, allocationName);
            memcpy(newBlob.mData, mData, mCount);
            newBlob.mCount = mCount;
        }
        else if (mCount == 0)
        {
            // Handle clearing the value
            newBlob.setData(nullptr, mCount);
        }
    }

    bool TdfBlob::operator==(const TdfBlob& rhs) const 
    {
        if (rhs.mCount == mCount)
            return memcmp(mData, rhs.mData, mCount) == 0;
        return false;
    }

    bool TdfBlob::operator<(const TdfBlob& rhs) const 
    {
        if (mCount == rhs.mCount)
            return memcmp(mData, rhs.mData, mCount) < 0;
        return mCount < rhs.mCount;
    }

    TdfBlob& TdfBlob::operator=(const TdfBlob& other)
    {
        if (this != &other)
        {
            other.copyInto(*this);
        }
        return *this; 
    }

#if EA_TDF_REGISTER_ALL
    TypeRegistrationNode REGISTRATION_NODE(TypeDescriptionSelector<TdfBlob >::get(), true);
#endif

} //namespace TDF

} //namespace EA

