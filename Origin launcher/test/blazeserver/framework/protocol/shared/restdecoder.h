/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_RESTDECODER_H
#define BLAZE_RESTDECODER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

#include "framework/protocol/shared/httpdecoder.h"
#include "framework/protocol/shared/httpprotocolutil.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class RawBuffer;

class BLAZESDK_API RestDecoder : public HttpDecoder
{
public:
    RestDecoder();
    ~RestDecoder() override;

    bool visit(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &referenceValue) override;


    const char8_t* getName() const override { return RestDecoder::getClassName(); }
    Decoder::Type getType() const override { return Decoder::REST; }
    static const char8_t* getClassName() { return "rest"; }

    void decodeRestResponse(bool isRestResponse, bool hasError = false, int32_t httpErrorCode = -1) { mIsRestResponse = isRestResponse; mHasError = hasError; mHttpErrorCode = httpErrorCode; }
    void setRestResourceInfo(const RestResourceInfo* restInfo) { mRestResourceInfo = restInfo; }
    void setHttpResponseHeaders(const HttpHeaderMap *headerMap);

protected:

    const char8_t* getNestDelim() const override { return "()[]."; }
    bool useMemberName() const override { return true; } 

private:
    HttpHeaderMap mHeaderMap;
    HttpProtocolUtil::HttpMethod mMethod;
    bool mHeaderMapManuallySet;
    Decoder::Type mDecodingType;
    const char8_t* mRecordPayload;
    size_t mContentLength;
    const RestResourceInfo* mRestResourceInfo;
    bool mIsRestResponse;
    bool mHasError;
    int32_t mHttpErrorCode;

    void initializeState();
    bool parseRequest();
    bool parseResponse(const char8_t * tdfName);
    bool parseCommonHeaders(const char8_t * tdfName = nullptr);
    bool parseTemplateParams();
    void mapUrlParams();
    void parseHeaderMap(const HttpFieldMapping *headers, size_t arrayCount);
};

} // Blaze

#endif // BLAZE_RESTDECODER_H
