/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_TDFDECODER_H
#define BLAZE_TDFDECODER_H
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

#include "framework/util/shared/rawbuffer.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class RawBuffer;

class BLAZESDK_API TdfDecoder : public Decoder
{
public:
    TdfDecoder() {}

    ~TdfDecoder() override {}

#ifdef BLAZE_CLIENT_SDK
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    virtual Blaze::BlazeError decode(RawBuffer& buffer, EA::TDF::Tdf& tdf, bool onlyDecodeChanged = false) = 0;
#else
    virtual Blaze::BlazeError decode(RawBuffer& buffer, EA::TDF::Tdf& tdf) = 0;
#endif //BLAZE_ENABLE_TDF_CHANGE_TRACKING
#else
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    virtual BlazeRpcError DEFINE_ASYNC_RET(decode(RawBuffer& buffer, EA::TDF::Tdf& tdf, bool onlyDecodeChanged = false)) = 0;
#else
    virtual BlazeRpcError DEFINE_ASYNC_RET(decode(RawBuffer& buffer, EA::TDF::Tdf& tdf)) = 0;
#endif //BLAZE_ENABLE_TDF_CHANGE_TRACKING
#endif //BLAZE_CLIENT_SDK

    virtual const char8_t* getErrorMessage() const = 0;
};

} // Blaze

#endif // BLAZE_TDFDECODER_H

