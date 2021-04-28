/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_HTTPDECODER_H
#define BLAZE_HTTPDECODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

#include <ctype.h>
#include "EASTL/hash_map.h"
#include "framework/protocol/shared/legacytdfdecoder.h"
#include "framework/protocol/shared/protocoltypes.h"
#include "framework/eastl_containers.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class RawBuffer;

typedef BLAZE_EASTL_VECTOR<BLAZE_EASTL_STRING> HttpXmlMapKeyList;

class BLAZESDK_API HttpDecoder : public TdfDecoder
{
public:
    HttpDecoder();
    ~HttpDecoder() override;

#ifdef BLAZE_CLIENT_SDK
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        virtual Blaze::BlazeError decode(RawBuffer& buffer, EA::TDF::Tdf& tdf, bool onlyDecodeChanged = false) override
#else
        virtual Blaze::BlazeError decode(RawBuffer& buffer, EA::TDF::Tdf& tdf)
#endif //BLAZE_ENABLE_TDF_CHANGE_TRACKING
#else
#ifdef BLAZE_ENABLE_TDF_CHANGE_TRACKING
        virtual BlazeRpcError DEFINE_ASYNC_RET(decode(RawBuffer& buffer, EA::TDF::Tdf& tdf, bool onlyDecodeChanged = false) override)
#else
        virtual BlazeRpcError DEFINE_ASYNC_RET(decode(RawBuffer& buffer, EA::TDF::Tdf& tdf))
#endif //BLAZE_ENABLE_TDF_CHANGE_TRACKING
#endif //BLAZE_CLIENT_SDK
    {
        mBuffer = &buffer;    
        bool result = visit(const_cast<EA::TDF::Tdf&>(tdf), const_cast<EA::TDF::Tdf&>(tdf));
        mBuffer = nullptr;

        return (result) ? Blaze::ERR_OK : ((mValidationError == Blaze::ERR_OK) ? Blaze::ERR_SYSTEM : mValidationError);
    }

    virtual bool visit(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &referenceValue);
    void parseTdf(EA::TDF::Tdf &tdf);

    const char8_t* getName() const override { return HttpDecoder::getClassName(); }
    Decoder::Type getType() const override { return Decoder::HTTP; }
    static const char8_t* getClassName() { return "http"; }
    const char8_t* getErrorMessage() const override { return "See prior [HttpDecoder] error log."; }

    virtual bool canUseStringsInBuffer() const { return false; }

protected:
    virtual bool useMemberName() const { return false; }

    virtual const char8_t* getNestDelim() const { return "|"; }

    void getValueByTags(BLAZE_EASTL_LIST<BLAZE_EASTL_STRING>& tags, const char8_t* keyValue, EA::TDF::TdfGenericReference outValue, const EA::TDF::TdfMemberInfo* memberInfo);

    HttpParamMap mParamMap;
    char8_t mUri[4096];

    RawBuffer* mBuffer;
    uint32_t mErrorCount;
#ifdef BLAZE_CLIENT_SDK
    Blaze::BlazeError mValidationError;
#else
    Blaze::BlazeRpcError mValidationError;
#endif

};

} // Blaze

#endif // BLAZE_HTTPDECODER_H
