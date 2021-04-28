/*************************************************************************************************/
/*!
    \file restrequestbuilder.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/protocol/shared/restrequestbuilder.h"
#include "framework/protocol/shared/jsonencoder.h"
#include "framework/protocol/shared/encoderfactory.h"
#include "framework/protocol/shared/httpparam.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/protocol/shared/httpprotocolutil.h"
#ifdef BLAZE_CLIENT_SDK
#include "DirtySDK/crypt/cryptsha2.h"
#else
#include "framework/util/hashutil.h"
#endif

namespace Blaze
{

    bool RestRequestBuilder::encodePayload(const RestResourceInfo* restInfo, Encoder::Type encoderType, const EA::TDF::Tdf* requestTdf, RawBuffer& payload)
    {
        bool success = false;

        if (requestTdf != nullptr)
        {
            if (restInfo != nullptr && restInfo->requestPayloadBlobFunc != nullptr)
            {
                // inject the binary data to the request body
                EA::TDF::TdfBlob& requestPayloadBlob = (*restInfo->requestPayloadBlobFunc)(const_cast<EA::TDF::Tdf&>(*requestTdf));
                if (requestPayloadBlob.getSize() > 0)
                {
                    uint8_t* outputBuffer = payload.acquire(requestPayloadBlob.getSize() + 1);
                    if (outputBuffer != nullptr)
                    {
                        memcpy(outputBuffer, requestPayloadBlob.getData(), requestPayloadBlob.getSize());
                        outputBuffer[requestPayloadBlob.getSize()] = '\0';
                        payload.put(requestPayloadBlob.getSize());
                    }
                    else
                    {
                        return false;
                    }
                }

                return true;
            }

            TdfEncoder* encoder = static_cast<TdfEncoder*>(EncoderFactory::create(encoderType, restInfo->payloadEncoderSubType));
            if (encoder == nullptr)
            {
                return false;
            }

            if (restInfo != nullptr && restInfo->requestPayloadMemberTags != nullptr)
            {
                if (encoderType == Encoder::JSON)
                {
                    JsonEncoder* jsonEncoder = static_cast<JsonEncoder*>(encoder);
                    if (restInfo->requestPayloadMemberTags)
                    {
                        jsonEncoder->setSubField(*restInfo->requestPayloadMemberTags);
                        jsonEncoder->setEncodeVariableGenericTdfInfo(!restInfo->encodeVariableGenericWithoutTdfInfo);
                    }
                }
            }

#ifdef BLAZE_CLIENT_SDK
            success = encoder->encode(payload, *requestTdf);
#else
            RpcProtocol::Frame frame;
            // On the server we intentionally use the onlyIfSet flag by default (the NORMAL case).
            // This allows proxy components to avoid sending non-set data by default.  
            // If you need to send a default value (like an empty list), either set the value directly, or use the markSet() function on the TDFObject.
            success = encoder->encode(payload, *requestTdf, &frame, (restInfo->payloadEncoderSubType == Encoder::NORMAL));
#endif
            BLAZE_DELETE_MGID(DEFAULT_BLAZE_MEMGROUP, encoder);
        }

        return success;
    }

    const char8_t* RestRequestBuilder::getContentTypeFromEncoderType(const Encoder::Type encoderType, bool includeHeader)
    {
        switch (encoderType)
        {
        case Encoder::XML2:
            return (includeHeader) ? HttpProtocolUtil::HTTP_XML_CONTENTTYPE : XML_CONTENTTYPE;
        case Encoder::HEAT2:
            return (includeHeader) ? HttpProtocolUtil::HTTP_HEAT_CONTENTTYPE : HEAT_CONTENTTYPE;
        case Encoder::JSON:
            return (includeHeader) ? HttpProtocolUtil::HTTP_JSON_CONTENTTYPE : JSON_CONTENTTYPE;
        default:
            return "";
        }
    }

    Encoder::Type RestRequestBuilder::getEncoderTypeFromContentType(const char8_t* contentType)
    {
        if (contentType != nullptr)
        {
            if (blaze_stricmp(contentType, XML_CONTENTTYPE) == 0 || blaze_strnicmp(contentType, XML_CONTENTTYPE ";", 16) == 0)
            {
                return Encoder::XML2;
            }
            else if (blaze_stricmp(contentType, HEAT_CONTENTTYPE) == 0 || blaze_strnicmp(contentType, HEAT_CONTENTTYPE ";", 23) == 0)
            {
                return Encoder::HEAT2;
            }
            else if (blaze_stricmp(contentType, JSON_CONTENTTYPE) == 0 || blaze_strnicmp(contentType, JSON_CONTENTTYPE ";", 17) == 0)
            {
                return Encoder::JSON;
            }
        }

        return Encoder::INVALID;
    }


    void RestRequestBuilder::buildCustomHeaderVector(const RestResourceInfo& restInfo, const EA::TDF::Tdf& tdf, HeaderVector& header, bool censor)
    {
        const HttpFieldMapping* headerTdfPair = restInfo.customRequestHeaders;
        header.reserve((HeaderVector::size_type)restInfo.restRequestHeaderArrayCount);

        StringBuilder headerValue;
        for (size_t counter = 0; counter < restInfo.restRequestHeaderArrayCount; ++counter)
        {   
            headerValue.reset();
            RestRequestOptions options;
            options.encodeEnumsAsStrings = restInfo.encodeEnumsAsStrings;
            options.encodeBoolsAsTrueFalse = restInfo.encodeBoolsAsTrueFalse;
            if (tdfToStringBuilder(headerValue, tdf, headerTdfPair[counter], restInfo.headerEncoderSubType, options, censor))
                header.push_back(BLAZE_EASTL_STRING(headerTdfPair[counter].header) + ": " + headerValue.get());
        }
    }


    void RestRequestBuilder::constructUri(const RestResourceInfo& restInfo, const EA::TDF::Tdf* requestTdf, StringBuilder& uri, const char8_t* uriPrefix, bool censor)
    {
        const HttpFieldMapping* resourcePathComponents = restInfo.resourcePathComponents;
        uri.reset();

        if (uriPrefix != nullptr)
        {
            uri.append("%s", uriPrefix);
        }

        if (restInfo.resourcePathComponentSize == 0)
            return; //nothing to do

        // Check if the resourcePath starts with a '/' and include one in the uri if it doesn't
        if (resourcePathComponents[0].header == nullptr || *(resourcePathComponents[0].header) != '/')
            uri.append("/");

        // Replace template parameters with tdf values

        for (size_t counter = 0; counter < restInfo.resourcePathComponentSize; ++counter)
        {
            if (resourcePathComponents[counter].header != nullptr)
            {
                uri << resourcePathComponents[counter].header;
            }

            if (requestTdf != nullptr && resourcePathComponents[counter].tagArray != nullptr)
            {
                StringBuilder paramValue;
                RestRequestOptions options;
                options.encodeEnumsAsStrings = restInfo.encodeEnumsAsStrings;
                options.encodeBoolsAsTrueFalse = restInfo.encodeBoolsAsTrueFalse;
                tdfToStringBuilder(paramValue, *requestTdf, resourcePathComponents[counter], Encoder::NORMAL, options, censor);
                uri.append("%s", paramValue.get());             
            }
        }
    }


    const char8_t** RestRequestBuilder::createHeaderCharArray(const HeaderVector& headerVector)
    {
        if (headerVector.empty())
            return nullptr;

#ifdef BLAZE_CLIENT_SDK
        const char8_t** headerArray = BLAZE_NEW_ARRAY(const char8_t*, headerVector.size(), MEM_GROUP_FRAMEWORK, "HttpHeaderArray");
#else
        const char8_t** headerArray = BLAZE_NEW_ARRAY(const char8_t*, headerVector.size());
#endif

        HeaderVector::const_iterator iter = headerVector.begin(), end = headerVector.end();
        for (size_t counter = 0; iter != end; ++iter, ++counter)
        {
            headerArray[counter] = iter->c_str();
        }

        return headerArray;
    }


    void RestRequestBuilder::buildCustomParamVector(const RestResourceInfo& restInfo, const EA::TDF::Tdf& tdf, HttpParamVector& httpParams, bool censor)
    {
        const HttpFieldMapping* headerTdfPair = restInfo.urlParams;
        for (size_t counter = 0; counter < restInfo.urlParamsArrayCount; ++counter)
        {
            RestRequestOptions options;
            options.encodeBoolsAsTrueFalse = restInfo.encodeBoolsAsTrueFalse;
            options.encodeEnumsAsStrings = restInfo.encodeEnumsAsStrings;
            options.encodeListsAsCSV = restInfo.encodeListsAsCSV;

            // The EA::TDF::Tdf member we are looking for might be a list, so we use a StringList to get each
            // value, and then add them individually to the httpParams vector.
            StringList BLAZE_SHARED_EASTL_VAR(headerValues, MEM_GROUP_FRAMEWORK_TEMP, "buildCustomParamVector::headerValues");
            if (tdfToStringList(headerValues, tdf, headerTdfPair[counter], restInfo.urlParamEncoderSubType, options, true, censor))
            {
                StringList::iterator it = headerValues.begin();
                StringList::iterator end = headerValues.end();
                for ( ; it != end; ++it)
                {
                    HttpParamVector::reference param = httpParams.push_back();
                    param.name = headerTdfPair[counter].header;
                    param.value = blaze_strdup(it->get());
                    param.encodeValue = true;
                }
            }
        }
    }


    void RestRequestBuilder::freeCustomParamVector(HttpParamVector& httpParams)
    {
        HttpParamVector::const_iterator itr = httpParams.begin();
        HttpParamVector::const_iterator end = httpParams.end();
        for(; itr != end; itr++)
        {
            // clean up memory allocated in buildCustomParamVector via strdup
#ifndef BLAZE_CLIENT_SDK
            BLAZE_FREE((void*) (*itr).value);
#else
            BLAZE_FREE(MEM_GROUP_DEFAULT, (void*) (*itr).value);
#endif
        }
    }

    bool RestRequestBuilder::tdfToStringBuilder(StringBuilder& tdfValue, const EA::TDF::Tdf& tdf, const HttpFieldMapping& fieldInfo, Encoder::SubType encSubType, const RestRequestOptions& options, bool censor)
    {
        // Callers of this function expect a single value at the given EA::TDF::Tdf member.
        // So, call tdfToStringList, then ensure the EA::TDF::Tdf member
        // represents only one value.  If it does, then pass that value back to the caller.
        StringList BLAZE_SHARED_EASTL_VAR(tdfValues, MEM_GROUP_FRAMEWORK_TEMP, "tdfToStringList::tdfValues");
        if (tdfToStringList(tdfValues, tdf, fieldInfo, encSubType, options, false, censor))
        {
            if (!tdfValues.empty())
                tdfValue.append("%s", tdfValues.front().get());
            else
                tdfValue.reset();
            return true;
        }
        return false;
    }

    bool RestRequestBuilder::tdfToStringList(StringList& tdfValues, const EA::TDF::Tdf& tdf, const HttpFieldMapping& fieldInfo, Encoder::SubType encSubType, const RestRequestOptions& options, bool allowMultipleValues, bool censor)
    {
        EA::TDF::TdfGenericReferenceConst tdfResult;
        const EA::TDF::TdfMemberInfo* memberInfo;
        bool memberSet = true;
        if (tdf.getValueByTags(fieldInfo.tagArray, fieldInfo.tagCount, tdfResult, &memberInfo, &memberSet))
        {
            switch (encSubType)
            {
                case Encoder::DEFAULTDIFFERNCE:
                {
                    // only encode this tdf if value is not equal to default
                    if(memberInfo != nullptr && memberInfo->equalsDefaultValue(tdfResult))
                        return false;
                    break;
                }
                case Encoder::NORMAL:
                {
                    // When this EA::TDF::Tdf has an encoderSubType of NORMAL, it might also have trackChanges enabled.
                    // tdf.getValueByTags() told us if the member has been set, so bail if the member wasn't set.
                    if (!memberSet)
                        return false;
                    break;
                }
                default:
                    return false;
            }

            EA::TDF::TdfMemberInfo::PrintFormat printFormat = EA::TDF::TdfMemberInfo::NORMAL;
#if EA_TDF_INCLUDE_EXTENDED_TAG_INFO
            if (censor && memberInfo != nullptr && (memberInfo->printFormat == EA::TDF::TdfMemberInfo::CENSOR || memberInfo->printFormat == EA::TDF::TdfMemberInfo::HASH))
                printFormat = memberInfo->printFormat;
#endif

            StringBuilder sb;
            if (tdfResult.getTypeDescription().isIntegral() || (tdfResult.getTypeDescription().isFloat()) || tdfResult.getTypeDescription().isString() || (tdfResult.getType() == EA::TDF::TDF_ACTUAL_TYPE_TIMEVALUE))
            {
                if (!tdfValueToString(tdfResult, sb, options, printFormat))
                    return false;
                tdfValues.push_back().append("%s", sb.get());
            }
            else if (tdfResult.getTypeDescription().getTdfType() == EA::TDF::TDF_ACTUAL_TYPE_LIST)
            {
                // If the caller wasn't expecting this member to be a list, then bail.
                if (!allowMultipleValues)
                    return false;

                if (tdfResult.asList().vectorSize() == 0)
                    return true;

                StringBuilder& nextSb = tdfValues.push_back();

                // Try to convert each member in the list to a string
                for (size_t a = 0; a < tdfResult.asList().vectorSize(); ++a)
                {
                    EA::TDF::TdfGenericReferenceConst itemValue;
                    tdfResult.asList().getValueByIndex(a, itemValue);
                    if (!tdfValueToString(itemValue, sb, options, printFormat))
                        return false;

                    if (a == 0)
                        nextSb.append("%s", sb.get());
                    else if (options.encodeListsAsCSV)
                        nextSb.append(",%s", sb.get());
                    else
                        tdfValues.push_back().append("%s", sb.get());

                    sb.reset();
                }
            }
            else
            {
                return false;
            }
        }
        else
        {
            tdfValues.clear();
        }
        return true;
    }

    bool RestRequestBuilder::tdfValueToString(const EA::TDF::TdfGenericReferenceConst &value, StringBuilder &result, const RestRequestOptions& opts, EA::TDF::TdfMemberInfo::PrintFormat printFormat)
    {
        if (printFormat == EA::TDF::TdfMemberInfo::CENSOR)
        {
            result.append(HttpProtocolUtil::CENSORED_STR);
            return true;
        }

        StringBuilder tempResult;
        StringBuilder* sb = &result;
        if (printFormat == EA::TDF::TdfMemberInfo::HASH)
            sb = &tempResult;

        switch (value.getType())
        {
            case EA::TDF::TDF_ACTUAL_TYPE_BOOL:
                 if(opts.encodeBoolsAsTrueFalse)
                 {
                    if(value.asBool())
                        sb->append("true");
                    else
                        sb->append("false");
                }
                else
                {
                    sb->append("%" PRIu64, value.asBool() ? 1 : 0);
                }
                break;
            case EA::TDF::TDF_ACTUAL_TYPE_UINT8:
                sb->append("%" PRIu8, value.asUInt8());
                break;
            case EA::TDF::TDF_ACTUAL_TYPE_UINT16:
                sb->append("%" PRIu16, value.asUInt16());
                break;
            case EA::TDF::TDF_ACTUAL_TYPE_UINT32:
                sb->append("%" PRIu32, value.asUInt32());
                break;
            case EA::TDF::TDF_ACTUAL_TYPE_UINT64:
                sb->append("%" PRIu64, value.asUInt64());
                break;
            case EA::TDF::TDF_ACTUAL_TYPE_INT8:
                sb->append("%" PRIi8, value.asInt8());
                break;
            case EA::TDF::TDF_ACTUAL_TYPE_INT16:
                sb->append("%" PRIi16, value.asInt16());
                break;
            case EA::TDF::TDF_ACTUAL_TYPE_INT32:
                sb->append("%" PRIi32, value.asInt32());
                break;
            case EA::TDF::TDF_ACTUAL_TYPE_INT64:
                sb->append("%" PRIi64, value.asInt64());
                break;
            case EA::TDF::TDF_ACTUAL_TYPE_ENUM:
                if (opts.encodeEnumsAsStrings && value.getTypeDescription().asEnumMap() != nullptr)
                {
                    const char8_t *identifier = nullptr;
                    value.getTypeDescription().asEnumMap()->findByValue(value.asEnum(), &identifier);
                    sb->append("%s", identifier);
                }
                else
                {
                    sb->append("%" PRIi32, value.asEnum());  // asEnum returns an int32 
                }
                break;
            case EA::TDF::TDF_ACTUAL_TYPE_FLOAT:
            {
                sb->append("%f", value.asFloat());
                break;
            }
            case EA::TDF::TDF_ACTUAL_TYPE_STRING:        
            {
                // Don't send non-utf8 strings around.
                if (!value.asString().isValidUtf8())
                    return false;

                sb->append("%s", value.asString().c_str());
                break;
            }
            case EA::TDF::TDF_ACTUAL_TYPE_TIMEVALUE:
            {
                sb->append("%" PRIi64, value.asTimeValue().getMicroSeconds());
                break;
            }
            default:
                return false;
        }

        if (printFormat != EA::TDF::TdfMemberInfo::HASH)
            return true;

#ifdef BLAZE_CLIENT_SDK
        uint8_t hashed[CRYPTSHA256_HASHSIZE];
        CryptSha2T sha256;
        CryptSha2Init(&sha256, CRYPTSHA256_HASHSIZE);
        CryptSha2Update(&sha256, (uint8_t*)tempResult.get(), (uint32_t)tempResult.length());
        CryptSha2Final(&sha256, hashed, CRYPTSHA256_HASHSIZE);
        result.append((char8_t*)hashed);
#else
        char8_t hashed[HashUtil::SHA256_STRING_OUT];
        HashUtil::generateSHA256Hash(tempResult.get(), tempResult.length(), hashed, HashUtil::SHA256_STRING_OUT);
        result.append(hashed);
#endif

        return true;
    }

    bool RestRequestBuilder::parseTdfValue(const EA::TDF::Tdf& tdf, const HttpFieldMapping& fieldInfo, StringBuilder& tdfValue, Encoder::SubType encSubType, bool encodeEnumsAsStrings)
    {
        RestRequestOptions options;
        options.encodeEnumsAsStrings = encodeEnumsAsStrings;
        return tdfToStringBuilder(tdfValue, tdf, fieldInfo, encSubType, options, false);
    }

    bool RestRequestBuilder::parseTdfValue(const EA::TDF::Tdf& tdf, const HttpFieldMapping& fieldInfo, StringList& tdfValues, Encoder::SubType encSubType,  bool encodeEnumsAsStrings, bool allowMultipleValues)
    {
        RestRequestOptions options;
        options.encodeEnumsAsStrings = encodeEnumsAsStrings;
        return tdfToStringList(tdfValues, tdf, fieldInfo, encSubType, options, allowMultipleValues, false);
    }
}

