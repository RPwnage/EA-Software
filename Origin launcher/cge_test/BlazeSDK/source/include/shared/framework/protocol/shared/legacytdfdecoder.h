/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_LEGACYTDFDECODER_H
#define BLAZE_LEGACYTDFDECODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/
#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/blazeerrors.h"
#else
#include "blazerpcerrors.h"
#endif

#include "EATDF/tdf.h"
#include "framework/protocol/shared/decoder.h"
#include "framework/protocol/shared/tdfdecoder.h"

#include "framework/util/shared/rawbuffer.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#ifdef LEGACY_DECODE_ASSERT_ON_ERROR
    #ifdef BLAZE_CLIENT_SDK
        #define INC_ERROR_COUNT BlazeAssert(false);
    #else
        #define INC_ERROR_COUNT EA_FAIL();
    #endif
#else
    #define INC_ERROR_COUNT ++mErrorCount;
#endif

namespace Blaze
{
class RawBuffer;

class BLAZESDK_API LegacyTdfDecoder : public TdfDecoder, public EA::TDF::TdfVisitor
{
public:
    LegacyTdfDecoder() 
    :   mBuffer(nullptr), 
        mErrorCount(0), 
        mAtTopLevel(true),
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        mOnlyDecodeChanged(false),
        mCurrentMemberIndex((uint32_t)-1),
#endif
        mValidationError(Blaze::ERR_OK)
    {}

    ~LegacyTdfDecoder() override {}

#ifdef BLAZE_CLIENT_SDK
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        virtual Blaze::BlazeError decode(RawBuffer& buffer, EA::TDF::Tdf& tdf, bool onlyDecodeChanged = false) override
#else
        virtual Blaze::BlazeError decode(RawBuffer& buffer, EA::TDF::Tdf& tdf)
#endif //BLAZE_ENABLE_TDF_CHANGE_TRACKING
#else
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        BlazeRpcError DEFINE_ASYNC_RET(decode(RawBuffer& buffer, EA::TDF::Tdf& tdf, bool onlyDecodeChanged = false) override)
#else
        virtual BlazeRpcError DEFINE_ASYNC_RET(decode(RawBuffer& buffer, EA::TDF::Tdf& tdf))
#endif //BLAZE_ENABLE_TDF_CHANGE_TRACKING
#endif //BLAZE_CLIENT_SDK
    {
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        mOnlyDecodeChanged = onlyDecodeChanged;
#endif //BLAZE_ENABLE_TDF_CHANGE_TRACKING
        setBuffer(&buffer);    
        bool result = visit(const_cast<EA::TDF::Tdf&>(tdf), const_cast<EA::TDF::Tdf&>(tdf));
        mBuffer = nullptr;

#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        mOnlyDecodeChanged = false;
        mCurrentMemberIndex = (uint32_t)-1;
#endif //BLAZE_ENABLE_TDF_CHANGE_TRACKING
        return (result) ? Blaze::ERR_OK : ((mValidationError == Blaze::ERR_OK) ? Blaze::ERR_SYSTEM : mValidationError);
    }

    size_t getBufferSize() { return mBuffer->size(); }


    virtual void setBuffer(RawBuffer* buffer) { mBuffer = buffer; }

protected:
    RawBuffer* mBuffer;
    uint32_t mErrorCount;
    bool mAtTopLevel;       // True when decoding is at the top level (not a nested struct)
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    bool mOnlyDecodeChanged;
    uint32_t mCurrentMemberIndex;
    bool preVisitCheck(EA::TDF::Tdf& rootTdf, EA::TDF::TdfMemberIterator& memberIt, EA::TDF::TdfMemberIteratorConst& referenceIt) override { mCurrentMemberIndex = (uint32_t)referenceIt.getIndex(); return true; }
    void markCurrentMemberSet(EA::TDF::Tdf& tdf) { tdf.markMemberSet(mCurrentMemberIndex); }
#endif
#ifdef BLAZE_CLIENT_SDK
    Blaze::BlazeError mValidationError;
#else
    Blaze::BlazeRpcError mValidationError;
#endif
};

#ifdef ENABLE_LEGACY_CODECS
/*************************************************************************************************/
/*!
    \brief compareMapKeysInteger

    A sort function used to sort map keys (as numeric keys) before decoding.
    Maps must be sorted in order for the find() function to work correctly.

    \return - true if a < b, false if a >= b

*/
/*************************************************************************************************/
bool compareMapKeysInteger(const BLAZE_EASTL_STRING& a, const BLAZE_EASTL_STRING& b)
{
    const char8_t* pa = a.c_str();
    const char8_t* pb = b.c_str();
    bool isNegative = false;    //true iff a and b are negative
    size_t alen = a.size();
    size_t blen = b.size();

    if (*pa == '-')
    {
        ++pa;
        --alen;
        isNegative = true;
    }
    if (*pb == '-')
    {
        ++pb;
        --blen;
        if (!isNegative)
        {
            return false;   //shortcut: b < 0, a >= 0
        }
    }
    else
    {
        if (isNegative)
        {
            return true;    //shortcut: a < 0, b >= 0
        }
    }

    if (alen < blen)
    {
        return !isNegative;
    }
    else if (alen > blen)
    {
        return isNegative;
    }

    while (*pa && *pb && *pa == *pb)
    {
        ++pa;
        ++pb;
    }

    return (isNegative) ? *pa > *pb : *pa < *pb;
}

/*************************************************************************************************/
/*!
    \brief compareMapKeysString

    A sort function used to sort map keys (as string keys) before decoding.
    Maps must be sorted in order for the find() function to work correctly.

    \return - true if a < b, false if a >= b

*/
/*************************************************************************************************/
bool compareMapKeysString(const BLAZE_EASTL_STRING& a, const BLAZE_EASTL_STRING& b)
{
    return (a.compare(b) < 0);
}

/*************************************************************************************************/
/*!
    \brief compareMapKeysStringIgnoreCase

    A sort function used to sort map keys (as string keys) before decoding.
    Maps must be sorted in order for the find() function to work correctly.

    \return - true if a < b, false if a >= b

*/
/*************************************************************************************************/
bool compareMapKeysStringIgnoreCase(const BLAZE_EASTL_STRING& a, const BLAZE_EASTL_STRING& b)
{
    return (a.comparei(b) < 0);
}

#endif

} // Blaze

#endif // BLAZE_LEGACYTDFDECODER_H

