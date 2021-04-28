/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_TDFENCODER_H
#define BLAZE_TDFENCODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/
#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/blazesdk.h"
#endif
#include "EATDF/tdf.h"
#include "framework/protocol/shared/encoder.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class RawBuffer;

class BLAZESDK_API TdfEncoder : public Encoder
{
public:

    // A max map key length for encoders if encoding key/value pairs. Encoders may truncate longer strings.
    const static int32_t MAX_MAP_ELEMENT_KEY_LENGTH = 128;

    TdfEncoder() {}

    ~TdfEncoder() override {};
   
    virtual bool encode(RawBuffer& buffer, const EA::TDF::Tdf& tdf
#ifndef BLAZE_CLIENT_SDK
            , const RpcProtocol::Frame* frame = nullptr
#endif
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
            , bool onlyEncodeChanged = false
#endif
        ) = 0;
};

} // Blaze

#endif // BLAZE_TDFENCODER_H

