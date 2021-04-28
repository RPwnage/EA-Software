/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_HEAT2DECODER_H
#define BLAZE_HEAT2DECODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

#include "EATDF/tdfbasetypes.h"
#include "EATDF/tdf.h"
#include "framework/protocol/shared/tdfdecoder.h"
#include "framework/protocol/shared/heat2util.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class RawBuffer;

class BLAZESDK_API Heat2Decoder : public TdfDecoder, public EA::TDF::TdfMemberVisitor
{
public:
    Heat2Decoder();
    ~Heat2Decoder() override;

#ifndef BLAZE_CLIENT_SDK
    Blaze::BlazeRpcError 
#else
    Blaze::BlazeError
#endif
        decode(RawBuffer& buffer, EA::TDF::Tdf& tdf
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
            , bool onlyDecodeChanged = false
#endif
        ) override;

    void setBuffer(RawBuffer* buffer);

    const char8_t* getName() const override { return Heat2Decoder::getClassName(); }
    Decoder::Type getType() const override { return Decoder::HEAT2; }
    static const char8_t* getClassName() { return "heat2"; }
    const char8_t* getErrorMessage() const override { return "See prior [Heat2Decoder] error log.";  }

    void setHeat1BackCompat(bool backCompat) { mHeat1BackCompat = backCompat; }

protected:
    bool visitContext(const EA::TDF::TdfVisitContext& context) override;

private:
    RawBuffer* mBuffer;
#ifndef BLAZE_CLIENT_SDK
    Blaze::BlazeRpcError mValidationResult;
#endif
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    bool mOnlyDecodeChanged;
#endif

    bool getHeader(uint32_t tag, Heat2Util::HeatType type);
    bool decodeVarsizeInteger(int64_t& value);
    bool getStructTerminator();
    bool skipElement(Heat2Util::HeatType type);

private:
    static const uint32_t MAX_SKIP_ELEMENT_DEPTH;
    uint32_t mCurrentSkipDepth;
    bool mFatalError;
    bool mHeat1BackCompat;
};

} // Blaze

#endif // BLAZE_HEAT2DECODER_H
