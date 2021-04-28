/*! ************************************************************************************************/
/*!
    \file externalsessionjsondatashared.cpp

    \brief This file contains defines and methods for dealing with the structured format of the
        external session's customizable json based string data, used by BlazeSDK clients and Blaze Server

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#include "framework/blaze.h"

#if (!defined(BLAZE_CLIENT_SDK) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_PS4_CROSSGEN) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS))

#if defined(BLAZE_CLIENT_SDK)
#include "BlazeSDK/component/framework/tdf/externalsessiontypes.h"
#include "BlazeSDK/shared/gamemanager/externalsessionjsondatashared.h"
#else
#include "framework/tdf/externalsessiontypes.h"
#include "framework/logger.h"
#include "component/gamemanager/shared/externalsessionjsondatashared.h"
#endif

#include "framework/util/shared/rawbuffer.h"
#include "framework/util/shared/rawbufferistream.h"
#include "framework/util/shared/bytearrayoutputstream.h"
#include "framework/util/shared/base64.h"
#include "framework/protocol/shared/tdfdecoder.h"
#include "framework/protocol/shared/decoderfactory.h"
#include "EATDF/codec/tdfjsonencoder.h"
#include "EATDF/codec/tdfjsondecoder.h"

namespace Blaze
{
namespace GameManager
{
    /*! ************************************************************************************************/
    /*! \brief Extract Blaze data from the Blaze-specified section of the PSN PlayerSession's customdata.
        \param[in] psnCustomData PSN PlayerSession's customdata to extract and deserialize from
    ***************************************************************************************************/
    bool psnPlayerSessionCustomDataToBlazeData(ExternalSessionBlazeSpecifiedCustomDataPs5& blazeData, const eastl::string& psnCustomData)
    {
        // Sony requires base64
        eastl::string buf;
        if (!fromBase64(buf, psnCustomData))
        {
            return false;
        }
        // Blaze uses JSON
        return fromJson(blazeData, buf);
    }

    /*! ************************************************************************************************/
    /*! \brief Populate the Blaze-specified section of the PSN PlayerSession's customdata, with Blaze data.
        \param[in] blazeData Data to serialize and add to the PSN PlayerSession's customdata
    ***************************************************************************************************/
    bool blazeDataToPsnPlayerSessionCustomData(eastl::string& psnCustomData, const ExternalSessionBlazeSpecifiedCustomDataPs5& blazeData)
    {
        // Blaze uses JSON
        RawBuffer jsonBuf(MAX_PSN_EXTERNALSESSION_CUSTOMDATA_LEN);
        if (!toJson(jsonBuf, blazeData))
        {
            return false;
        }
        // Sony requires base64
        return toBase64(psnCustomData, jsonBuf);
    }


    /*! ************************************************************************************************/
    /*! \brief Helper to serialize tdf to JSON
    ***************************************************************************************************/
    bool toJson(RawBuffer& jsonBuf, const EA::TDF::Tdf& fromTdf)
    {
        EA::TDF::JsonEncoder encoder;
        EA::TDF::JsonEncoder::FormatOptions formatOpts;
        formatOpts.options[EA::Json::JsonWriter::kFormatOptionIndentSpacing] = 0;
        formatOpts.options[EA::Json::JsonWriter::kFormatOptionLineEnd] = 0;
        encoder.setFormatOptions(formatOpts);
        EA::TDF::MemberVisitOptions visitOpt;
        visitOpt.onlyIfSet = true;
        RawBufferIStream istream(jsonBuf);
        bool encodeOk = encoder.encode(istream, fromTdf, nullptr, visitOpt);

#ifndef BLAZE_CLIENT_SDK
        if (IS_LOGGING_ENABLED(Logging::TRACE))
        {
            eastl::string jstr(reinterpret_cast<char8_t*>(jsonBuf.data()), jsonBuf.datasize());
            EA::TDF::TdfPtr reDecodeBuf = fromTdf.getClassInfo().createInstance(EA_TDF_GET_DEFAULT_ALLOCATOR_PTR, "externalsessionjsondatashared::toJson");
            bool reDecodeOk = (encodeOk && fromJson(*reDecodeBuf, jstr));
            TRACE_LOG("[externalsessionjsondatashared].toJson: TDF -> (" << (encodeOk ? jstr.c_str() : "<FAILED>") << (encoder.getErrorMessage() ? encoder.getErrorMessage() : "") << ") -> reDecoded: " << (reDecodeOk ? (StringBuilder() << reDecodeBuf).c_str() : "<FAILED>"));
        }
#endif
        return encodeOk;
    }
    /*! ************************************************************************************************/
    /*! \brief Helper to decode JSON back into tdf
    ***************************************************************************************************/
    bool fromJson(EA::TDF::Tdf& toTdf, const eastl::string& jsonBuf)
    {
        EA::TDF::JsonDecoder decoder;
        RawBuffer buf((uint8_t *)jsonBuf.data(), jsonBuf.size());
        buf.put(jsonBuf.size());
        RawBufferIStream istream(buf);
        bool ok = decoder.decode(istream, toTdf);

#ifndef BLAZE_CLIENT_SDK
        if (!ok)
        {
            ERR_LOG("[externalsessionjsondatashared].fromJson: failed to decode(" << jsonBuf << ") " << (decoder.getErrorMessage() ? decoder.getErrorMessage() : ""));
        }
#endif
        return ok;
    }

    /*! ************************************************************************************************/
    /*! \brief Helper to convert data to base64
    ***************************************************************************************************/
    bool toBase64(eastl::string& base64Buf, const RawBuffer& fromData)
    {
        if (fromData.datasize() == 0)
        {
            return true;
        }
        char8_t tmp[MAX_PSN_EXTERNALSESSION_CUSTOMDATA_LEN];
        //stream class guards vs overflow:
        ByteArrayOutputStream output(reinterpret_cast<uint8_t*>(tmp), sizeof(tmp));
        uint32_t size = Base64::encode((uint8_t *)fromData.data(), (uint32_t)fromData.datasize(), &output);
        tmp[size] = '\0';
        base64Buf = tmp;
        bool encodeOk = (size > 0);

#ifndef BLAZE_CLIENT_SDK
        if (IS_LOGGING_ENABLED(Logging::TRACE))
        {
            eastl::string reDecodeBuf;
            bool reDecodeOk = (encodeOk && fromBase64(reDecodeBuf, base64Buf));
            TRACE_LOG("[externalsessionjsondatashared].toBase64: (" << (eastl::string(reinterpret_cast<char8_t*>(fromData.data()), fromData.datasize())) << ") -> (" << (encodeOk ? base64Buf.c_str() : "<FAILED>") <<") -> reDecoded(" << (reDecodeOk ? reDecodeBuf.c_str() : "<FAILED>") << ")");
        }
#endif
        return encodeOk;
    }
    /*! ************************************************************************************************/
    /*! \brief Helper to convert base64 back to orig data
    ***************************************************************************************************/
    bool fromBase64(eastl::string& toData, const eastl::string& base64buf)
    {
        if (base64buf.empty())
        {
            return true;
        }
        char8_t tmp[MAX_PSN_EXTERNALSESSION_CUSTOMDATA_LEN];
#ifndef BLAZE_CLIENT_SDK
        ASSERT_COND_LOG(base64buf.size() <= MAX_PSN_EXTERNALSESSION_CUSTOMDATA_LEN, "[externalsessionjsondatashared].fromBase64: Warning: PSN data len(" << base64buf.size() << ") is longer than max(" << MAX_PSN_EXTERNALSESSION_CUSTOMDATA_LEN << "). Blaze may clip data.");
#endif
        //stream class guards vs overflow:
        ByteArrayOutputStream output((uint8_t *)tmp, MAX_PSN_EXTERNALSESSION_CUSTOMDATA_LEN);
        ByteArrayInputStream input((const uint8_t *)base64buf.data(), (uint32_t)base64buf.size());
        uint32_t size = Base64::decode(&input, &output);
        tmp[size] = '\0';
        toData = tmp;
        return (size > 0);
    }

} // namespace GameManager
} // namespace Blaze

#endif //(!defined(BLAZE_CLIENT_SDK) || defined(EA_PLATFORM_PS5) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS))