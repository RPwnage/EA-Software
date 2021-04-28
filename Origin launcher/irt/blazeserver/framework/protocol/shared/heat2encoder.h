/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_HEAT2ENCODER_H
#define BLAZE_HEAT2ENCODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

#include "EATDF/tdfbasetypes.h"
#include "EATDF/tdf.h"
#include "framework/protocol/shared/tdfencoder.h"
#include "framework/protocol/shared/heat2util.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class RawBuffer;

class BLAZESDK_API Heat2Encoder : public TdfEncoder, public EA::TDF::TdfMemberVisitorConst
{
public:
    Heat2Encoder()
        : mBuffer(nullptr), mDefaultDiff(false), mHeat1BackCompat(false)
    {
    }

    ~Heat2Encoder() override {};

    bool encode(RawBuffer& buffer, const EA::TDF::Tdf& tdf
#ifndef BLAZE_CLIENT_SDK
        , const RpcProtocol::Frame* frame = nullptr
#endif
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        , bool onlyEncodeChanged = false
#endif
    ) override;

    const char8_t* getName() const override { return Heat2Encoder::getClassName(); }
    Encoder::Type getType() const override { return Encoder::HEAT2; }
    static const char8_t* getClassName() { return "heat2"; }
    void setUseDefaultDiff(bool useDefaultDiff) { mDefaultDiff = useDefaultDiff; }

    void setHeat1BackCompat(bool backCompat) { mHeat1BackCompat = backCompat;  }

protected:
    bool visitContext(const EA::TDF::TdfVisitContextConst& context) override;

private:
    RawBuffer* mBuffer;
    bool mDefaultDiff;
    bool mHeat1BackCompat;

    bool putHeader(uint32_t tag, Heat2Util::HeatType type);
    void encodeVarsizeInteger(int64_t value);
};

} // Blaze

#endif // BLAZE_HEAT2ENCODER_H

