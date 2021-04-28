/*************************************************************************************************/
/*!
    \file 


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include <EATDF/tdfstring.h>
#include <EATDF/tdfblob.h>

#include <EAStdC/EAString.h>

#include <string.h>

namespace EA
{
namespace TDF
{
 

const char8_t* TdfString::DEFAULT_STRING_ALLOCATION_NAME = "STRINGDATA";

static char8_t EMPTY_STR = '\0';

    TdfString::TdfString(EA::Allocator::ICoreAllocator& allocator, const char8_t* allocationName /*= DEFAULT_STRING_ALLOCATION_NAME*/)
    :   mTdfStringPointer(&EMPTY_STR),
        mAllocator(allocator),
        mLengthAndOwnsMem(0)
    {
    }
    
    TdfString::TdfString(const char* value, EA::Allocator::ICoreAllocator& allocator, const char8_t* allocationName /* = DEFAULT_STRING_ALLOCATION_NAME */)
    :   mTdfStringPointer(&EMPTY_STR),
        mAllocator(allocator),
        mLengthAndOwnsMem(0)
    {
        set(value, 0, allocationName);
    }
    
    TdfString::TdfString(const TdfString& rhs, const char8_t* allocationName /* = DEFAULT_STRING_ALLOCATION_NAME */)
    :   mTdfStringPointer(&EMPTY_STR),
        mAllocator(rhs.mAllocator),
        mLengthAndOwnsMem(0)
    {
        set(rhs.mTdfStringPointer, rhs.length(), allocationName);
    }

#if !defined(EA_COMPILER_NO_RVALUE_REFERENCES)
    /*! \brief Move constructor. */
    TdfString::TdfString(TdfString && rhs)
        : mTdfStringPointer(rhs.mTdfStringPointer),
        mAllocator(rhs.mAllocator),
        mLengthAndOwnsMem(rhs.mLengthAndOwnsMem)
    {
#if EA_TDF_STRING_USE_DEFAULT_BUFFER
        // With move semantics, the short string buffer becomes a cost, rather than a savings.  (28 char by default)
        if (rhs.mTdfStringPointer == rhs.mBuffer)
        {
            memcpy(mBuffer, rhs.mBuffer, sizeof(rhs.mBuffer));
            mTdfStringPointer = mBuffer;
        }
#endif
        rhs.mTdfStringPointer = &EMPTY_STR;
        rhs.mLengthAndOwnsMem = 0;
    }

    /*! \brief Move operator. */
    TdfString& TdfString::operator=(TdfString&& rhs)
    {
        release();
#if EA_TDF_STRING_USE_DEFAULT_BUFFER
        // With move semantics, the short string buffer becomes a cost, rather than a savings.  (28 char by default)
        if (rhs.mTdfStringPointer == rhs.mBuffer)
        {
            memcpy(mBuffer, rhs.mBuffer, sizeof(rhs.mBuffer));
            mTdfStringPointer = mBuffer;
            mLengthAndOwnsMem = rhs.mLengthAndOwnsMem;
            rhs.mTdfStringPointer = &EMPTY_STR;
            rhs.mLengthAndOwnsMem = 0;
        }
        else
#endif
        if (&mAllocator == &rhs.mAllocator)
        {
            // take over ownership
            mTdfStringPointer = rhs.mTdfStringPointer;
            mLengthAndOwnsMem = rhs.mLengthAndOwnsMem;
            rhs.mTdfStringPointer = &EMPTY_STR;
            rhs.mLengthAndOwnsMem = 0;
        }
        else
        {
            // can't take over ownership
            set(rhs.mTdfStringPointer, rhs.length());
            rhs.release();
        }

        return *this;
    }
#endif

    /*! \brief Release the storage buffer by deallocating it if owned.
                The buffer pointer is set to nullptr. */
    void TdfString::release()
    {
        if (mTdfStringPointer == &EMPTY_STR)
        {
            // avoid touching memory when string is empty
            return;
        }

        if (getOwnsMem() && mTdfStringPointer != nullptr 
#if EA_TDF_STRING_USE_DEFAULT_BUFFER            
            && mTdfStringPointer != mBuffer
#endif            
            )
        {
            CORE_FREE(&mAllocator, mTdfStringPointer);
        }
        mTdfStringPointer = &EMPTY_STR;
        setLength(0, false);
    }

    bool TdfString::operator<(const TdfString& rhs) const
    {
        return (EA::StdC::Strcmp(mTdfStringPointer, rhs.mTdfStringPointer) < 0);
    }

    bool TdfString::operator==(const TdfString& rhs) const
    {
        return (EA::StdC::Strcmp(mTdfStringPointer, rhs.mTdfStringPointer) == 0);
    }
    bool TdfString::operator!=(const TdfString& rhs) const
    {
        return (EA::StdC::Strcmp(mTdfStringPointer, rhs.mTdfStringPointer) != 0);
    }

    bool TdfString::operator<(const char8_t* rhs) const
    {
        return (EA::StdC::Strcmp(mTdfStringPointer, rhs) < 0);
    }

    bool TdfString::operator==(const char8_t* rhs) const
    {
        return (EA::StdC::Strcmp(mTdfStringPointer, rhs) == 0);
    }
    bool TdfString::operator!=(const char8_t* rhs) const
    {
        return (EA::StdC::Strcmp(mTdfStringPointer, rhs) != 0);
    }

    void TdfString::set(const char8_t* value, TdfStringLength len, const char8_t* allocationName /* = DEFAULT_STRING_ALLOCATION_NAME */ )
    {
        if (value != mTdfStringPointer) //no-op on same string set
        {
            if (value != nullptr && value[0] != '\0')
            {
                if (len == 0)
                {
                    len = (TdfStringLength) EA_TDF_STRLEN(value) + 1;
                }
                else
                {
                    len = len + 1; //the contract we have states you pass in char size, not the nullptr char.  "len" is used internally to represent whole
                              //buffer size, so correct here.  Later on we store the actual string length minus the nullptr.
                }

                if (getOwnsMem() && mTdfStringPointer != &EMPTY_STR && len - 1 <= length())
                {
                    //just copy and be done with it
                    memcpy(mTdfStringPointer, value, len);
                }
                else
                {
                    resize(len, allocationName);
                    memcpy(mTdfStringPointer, value, len);
                }

                setLength(len - 1, getOwnsMem());
                mTdfStringPointer[length()] = 0; // null terminate the string
            }
            else if (*mTdfStringPointer != EMPTY_STR)
            {
                release();
            }
        }
    }  

    void TdfString::resize(TdfStringLength len, const char8_t* allocationName)
    {
        release();

        setOwnsMem();
#if EA_TDF_STRING_USE_DEFAULT_BUFFER
        if (len <= DEFAULT_BUFFER_LEN)
        {
            mTdfStringPointer = mBuffer;
        }
        else
#endif
        {
            mTdfStringPointer = (char8_t*)CORE_ALLOC(&mAllocator, len, allocationName, 0);
        }
    }

    void TdfString::assignBuffer(const char8_t* value)
    {
        release();
        mTdfStringPointer = const_cast<char8_t*>(value);
        setLength((TdfStringLength) EA_TDF_STRLEN(value), false);
    }

    bool TdfStringCompareIgnoreCase::operator()(const TdfString& lhs, const TdfString& rhs) const
    {
        return (EA::StdC::Stricmp(lhs.mTdfStringPointer, rhs.mTdfStringPointer) < 0);
    }
    size_t TdfStringHash::operator() (const TdfString& str) const
    {
        eastl::hash<const char8_t*> h;
        return h(str.mTdfStringPointer);
    }

    size_t TdfString::calcBase64EncodedSize() const
    {
        return TdfBlob::calcBase64EncodedSize(length());
    }
    int32_t TdfString::encodeBase64(char8_t* output, size_t outputBufSize) const
    {
        return TdfBlob::encodeBase64(mTdfStringPointer, length(), output, outputBufSize);
    }
    int32_t TdfString::decodeBase64(const char8_t* input, TdfSizeVal inputBufSize)
    {
        release();

        if (inputBufSize % 4 != 0)
        {
            return -1;
        }

        // input buffer size is always larger than decoded buffer size
        resize(inputBufSize);
        size_t tempLength;
        TdfBlob::decodeBase64(input, inputBufSize, &mTdfStringPointer, &tempLength);
        setLength((TdfSizeVal)tempLength, true);
        return (int32_t)tempLength;
    }

    bool TdfString::isValidUtf8() const
    {
        if (mTdfStringPointer == nullptr)
            return true;

        const char8_t* bytes = mTdfStringPointer;
        uint32_t cp;
        int32_t num;

        while (*bytes != 0x00)
        {
            if ((*bytes & 0x80) == 0x00)
            {
                // U+0000 to U+007F 
                cp = (*bytes & 0x7F);
                num = 1;
            }
            else if ((*bytes & 0xE0) == 0xC0)
            {
                // U+0080 to U+07FF 
                cp = (*bytes & 0x1F);
                num = 2;
            }
            else if ((*bytes & 0xF0) == 0xE0)
            {
                // U+0800 to U+FFFF 
                cp = (*bytes & 0x0F);
                num = 3;
            }
            else if ((*bytes & 0xF8) == 0xF0)
            {
                // U+10000 to U+10FFFF 
                cp = (*bytes & 0x07);
                num = 4;
            }
            else
                return false;

            bytes += 1;
            for (int32_t i = 1; i < num; ++i)
            {
                if ((*bytes & 0xC0) != 0x80)
                    return false;
                cp = (cp << 6) | (*bytes & 0x3F);
                bytes += 1;
            }

            if ((cp > 0x10FFFF) ||
                ((cp >= 0xD800) && (cp <= 0xDFFF)) ||
                ((cp <= 0x007F) && (num != 1)) ||
                ((cp >= 0x0080) && (cp <= 0x07FF) && (num != 2)) ||
                ((cp >= 0x0800) && (cp <= 0xFFFF) && (num != 3)) ||
                ((cp >= 0x10000) && (cp <= 0x1FFFFF) && (num != 4)))
                return false;
        }

        return true;
    }


} //namespace TDF
} //namespace EA

