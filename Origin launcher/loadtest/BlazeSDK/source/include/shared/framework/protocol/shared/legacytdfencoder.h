/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_LEGACYTDFENCODER_H
#define BLAZE_LEGACYTDFENCODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/
#include "EATDF/tdf.h"
#include "framework/protocol/shared/tdfencoder.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class RawBuffer;

class BLAZESDK_API LegacyTdfEncoder : public TdfEncoder, public EA::TDF::TdfVisitor
{
public:

    LegacyTdfEncoder()
        : mErrorCount(0),
          mBuffer(nullptr)
#ifndef BLAZE_CLIENT_SDK
          ,mFrame(nullptr)
#endif
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
          ,mOnlyEncodeChanged(false)
#endif
    {
    }

    ~LegacyTdfEncoder() override {};
   
    bool encode(RawBuffer& buffer, const EA::TDF::Tdf& tdf
#ifndef BLAZE_CLIENT_SDK
            , const RpcProtocol::Frame* frame = nullptr
#endif
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
            , bool onlyEncodeChanged = false
#endif
        ) override
    {        
#ifndef BLAZE_CLIENT_SDK
        mFrame = frame;
#endif

#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        mOnlyEncodeChanged = onlyEncodeChanged;
#endif
        setBuffer(&buffer);
        bool result = visit(const_cast<EA::TDF::Tdf&>(tdf), const_cast<EA::TDF::Tdf&>(tdf));
        mBuffer = nullptr;
#ifndef BLAZE_CLIENT_SDK
        mFrame = nullptr;
#endif

#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        mOnlyEncodeChanged = false;
#endif

        return result;
    }

    virtual size_t encodeSize() const = 0;

protected:
    virtual void setBuffer(RawBuffer* buffer) { mBuffer = buffer; }
#ifndef BLAZE_CLIENT_SDK
    const RpcProtocol::Frame* getFrame() const { return mFrame; }
#endif

    uint32_t mErrorCount;
    RawBuffer* mBuffer;
#ifndef BLAZE_CLIENT_SDK
    const RpcProtocol::Frame* mFrame;
#endif

#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
    bool preVisitCheck(EA::TDF::Tdf& rootTdf, EA::TDF::TdfMemberIterator& memberIt, EA::TDF::TdfMemberIteratorConst& referenceIt) override { return !mOnlyEncodeChanged || referenceIt.isSet(); }
    bool mOnlyEncodeChanged;
#endif
};

} // Blaze

#endif // BLAZE_LEGACYTDFENCODER_H

