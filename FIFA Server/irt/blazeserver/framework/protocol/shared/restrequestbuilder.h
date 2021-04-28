/*************************************************************************************************/
/*!
    \file restrequestbuilder.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_RESTREQUESTBUILDER_H
#define BLAZE_RESTREQUESTBUILDER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "framework/protocol/shared/protocoltypes.h"
#include "framework/protocol/shared/encoder.h"

#include "framework/util/shared/stringbuilder.h"
#include "EASTL/list.h"

#include "framework/eastl_containers.h"

namespace Blaze
{

    class RawBuffer;
    struct HttpParam;

    static const size_t MAX_HEADER_SIZE =  16384;
    static const size_t MAX_URI_PARAM_SIZE = 512;

    class BLAZESDK_API RestRequestBuilder
    {
    public:
        RestRequestBuilder() {}
        virtual ~RestRequestBuilder() {}

        typedef struct RestRequestOptions {
            RestRequestOptions(): encodeEnumsAsStrings(false), encodeBoolsAsTrueFalse(false), encodeListsAsCSV(false) {}

            bool encodeEnumsAsStrings;
            bool encodeBoolsAsTrueFalse;
            bool encodeListsAsCSV;
        } RestRequestOptions;

        typedef BLAZE_EASTL_VECTOR<BLAZE_EASTL_STRING> HeaderVector;
        typedef BLAZE_EASTL_VECTOR<HttpParam> HttpParamVector;
        typedef BLAZE_EASTL_LIST<StringBuilder> StringList;

        static const char8_t* getContentTypeFromEncoderType(const Blaze::Encoder::Type encoderType, bool includeHeader = false);
        static Encoder::Type getEncoderTypeFromContentType(const char8_t* contentType);
        static bool encodePayload(const RestResourceInfo* restInfo, Encoder::Type encoderType, const EA::TDF::Tdf* requestTdf, RawBuffer& payload);

        static void buildCustomHeaderVector(const RestResourceInfo& restInfo, const EA::TDF::Tdf& tdf, HeaderVector& header, bool censor=false);

        static void constructUri(const RestResourceInfo& restInfo, const EA::TDF::Tdf* requestTdf, StringBuilder& uri, const char8_t* uriPrefix = nullptr, bool censor=false);
        static const char8_t** createHeaderCharArray(const HeaderVector& headerVector);
        static void buildCustomParamVector(const RestResourceInfo& restInfo, const EA::TDF::Tdf& tdf, HttpParamVector& httpParams, bool censor=false);
        static void freeCustomParamVector(HttpParamVector& httpParams);

        static bool tdfToStringBuilder(StringBuilder& tdfValue, const EA::TDF::Tdf& tdf, const HttpFieldMapping& fieldInfo, Encoder::SubType encSubType, const RestRequestOptions& options, bool censor=false);
        static bool tdfToStringList(StringList& tdfValues, const EA::TDF::Tdf& tdf, const HttpFieldMapping& fieldInfo, Encoder::SubType encSubType, const RestRequestOptions& options, bool allowMultipleValues=false, bool censor=false);

        // Deprecated
        static bool parseTdfValue(const EA::TDF::Tdf& tdf, const HttpFieldMapping& fieldInfo, StringBuilder& tdfValue, Encoder::SubType encSubType, bool encodeEnumsAsStrings=false);
        static bool parseTdfValue(const EA::TDF::Tdf& tdf, const HttpFieldMapping& fieldInfo, StringList& tdfValues, Encoder::SubType encSubType, bool encodeEnumsAsStrings=false, bool allowMultipleValues=false);

    private:
        static bool tdfValueToString(const EA::TDF::TdfGenericReferenceConst &value, StringBuilder &result, const RestRequestOptions& opts, EA::TDF::TdfMemberInfo::PrintFormat printFormat);

    };
}

#endif

