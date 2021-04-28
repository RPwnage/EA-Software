/*! *********************************************************************************************/
/*!
    \file   tdfstring.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef EA_TDF_TDF_STRING_H
#define EA_TDF_TDF_STRING_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include <EATDF/internal/config.h>
#include <EATDF/tdfobject.h>

/************* Include files ***************************************************************************/

/************ Defines/Macros/Constants/Typedefs *******************************************************/



namespace EA
{
namespace TDF
{
 

struct TdfStringCompareIgnoreCase;
struct TdfStringHash;

typedef TdfSizeVal TdfStringLength;

/*! ***********************************************************************/
/*! \class TdfString
    \brief simple string wrapper class used by the tdf types
***************************************************************************/
class EATDF_API TdfString
{
public:
    static const char8_t* DEFAULT_STRING_ALLOCATION_NAME;

    /*! \brief Construct an empty tdf string */
    TdfString(EA::Allocator::ICoreAllocator& allocator EA_TDF_DEFAULT_ALLOCATOR_ASSIGN, const char8_t* allocationName = DEFAULT_STRING_ALLOCATION_NAME);
    /*! \brief Construct a tdf string */
    TdfString(const char* value, EA::Allocator::ICoreAllocator& allocator EA_TDF_DEFAULT_ALLOCATOR_ASSIGN, const char8_t* allocationName = DEFAULT_STRING_ALLOCATION_NAME);
    /*! \brief Copy constructor. */
    TdfString(const TdfString& rhs, const char8_t* allocationName = DEFAULT_STRING_ALLOCATION_NAME);
#if !defined(EA_COMPILER_NO_RVALUE_REFERENCES)
    /*! \brief Move constructor. */
    TdfString(TdfString&& rhs);
    /*! \brief Move operator. */
    TdfString& operator=(TdfString&& rhs);
#endif

    /*! \brief Assignment operator. */
    TdfString& operator=(const TdfString& rhs)
    {
        set(rhs.mTdfStringPointer, rhs.length());
        return *this;
    }

    /*! \brief Assignment operator from character array. */
    TdfString& operator=(const char8_t* rhs)
    {
        set(rhs);
        return *this;
    }

    /*! \brief Release the storage buffer by deallocating it if owned.
            The buffer pointer is set to nullptr. */
    void release();

    ~TdfString()
    {
        release();
    }

    /*! \brief Set the new value for this string. NO-OP if value aliases internal pointer. */
    void set(const char8_t* value, TdfStringLength len = 0, const char8_t* allocationName = DEFAULT_STRING_ALLOCATION_NAME);
    void assignBuffer(const char8_t* value);

    /*! \brief Return the current length of this string. */
    TdfStringLength length() const { return (mLengthAndOwnsMem >> 1); }

    /*! \brief Returns true if the string is empty, false otherwise */
    bool empty() const { return (length() == 0); }

    /*! \brief Return a pointer to the wrapped character array. */
    const char8_t* c_str() const
    {
        return mTdfStringPointer;
    }

    /*! \brief Cast operator to convert to a character array */
    operator const char8_t*() const
    {
        return mTdfStringPointer;
    }

    bool operator<(const TdfString& rhs) const;
    bool operator<(const char8_t* rhs) const;

    bool operator==(const TdfString& rhs) const;
    bool operator==(const char8_t* rhs) const;
    bool operator!=(const TdfString& rhs) const;
    bool operator!=(const char8_t* rhs) const;

    // Check if the TDF string is a valid utf8 encoding
    bool isValidUtf8() const;

    size_t calcBase64EncodedSize() const;
    int32_t encodeBase64(char8_t* output, size_t outputBufSize) const;
    int32_t decodeBase64(const char8_t* input, TdfSizeVal inputBufSize);

    EA::Allocator::ICoreAllocator& getAllocator() const { return mAllocator; }
private:
    void setLength(TdfStringLength length, bool ownsMem) { mLengthAndOwnsMem = (length << 1) | (ownsMem ? 1 : 0); }

    // destructive resize - releases the existing string
    void resize(TdfStringLength len, const char8_t* allocationName = DEFAULT_STRING_ALLOCATION_NAME);

    bool getOwnsMem()   { return (bool)(mLengthAndOwnsMem & 1); }
    void setOwnsMem()   { mLengthAndOwnsMem |= 1; }

    friend struct TdfStringCompareIgnoreCase;
    friend struct TdfStringHash;
    char8_t* mTdfStringPointer;  // Storage pointer for the wrapped string it could be internal and object managed or external
    EA::Allocator::ICoreAllocator& mAllocator;
    TdfStringLength mLengthAndOwnsMem;     // This includes the length and Owns Memory bit.
    

#if EA_TDF_STRING_USE_DEFAULT_BUFFER
    static const uint32_t DEFAULT_BUFFER_LEN = 27;
    char8_t mBuffer[DEFAULT_BUFFER_LEN+1]; // with this buffer sizeof(TdfString)==48 on linux which is well packed
#endif
};

struct EATDF_API TdfStringCompareIgnoreCase
{
    bool operator() (const TdfString& lhs, const TdfString& rhs) const;
    static bool isCaseSensitive() { return false; }
};
struct EATDF_API TdfStringHash
{
    size_t operator() (const TdfString& str) const;
    static bool isCaseSensitive() { return true; }
};

// Helper functions to create a copy of the given value, using the correct allocator for TdfString. 
template <>
struct TdfPrimitiveAllocHelper<TdfString>
{
    TdfString operator()(const TdfString& value, EA::Allocator::ICoreAllocator& allocator) const
    {
        if (&value.getAllocator() == &allocator)
            return value;

        TdfString stringWithAllocator(allocator);
        stringWithAllocator = value;
        return stringWithAllocator;
    }
    TdfString operator()(EA::Allocator::ICoreAllocator& allocator) const
    {
        return TdfString(allocator);
    }
};



} //namespace TDF
} //namespace EA


#endif // EA_TDF_TDF_STRING_H
