/*************************************************************************************************/
/*!
    \file   FifaStatsHttpResponse.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FIFA_STATS_HTTP_RESPONSE_H
#define BLAZE_FIFA_STATS_HTTP_RESPONSE_H

/*** Include files *******************************************************************************/
#include "framework/connection/outboundhttpconnectionmanager.h"
#include "framework/protocol/outboundhttpresult.h"

#include "fifagroups/tdf/fifagroupstypes.h"
#include "EAJson/JsonDomReader.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
	namespace FifaStats
	{
		class HttpResponse :
			public OutboundHttpResult, 
			public EA::Json::IJsonDomReaderCallback
		{
			public:
				HttpResponse();
				virtual ~HttpResponse();

				virtual BlazeRpcError decodeResult(RawBuffer& response);

				virtual void setHttpError(BlazeRpcError err = ERR_SYSTEM) { }
				virtual bool checkSetError(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) { return false; }
				virtual void setAttributes(const char8_t* fullname, const Blaze::XmlAttribute* attributes, size_t attributeCount) { }
				virtual void setValue(const char8_t* fullname, const char8_t* data, size_t dataLen) { }

				/*** IJsonDomReaderCallback Interface ************************************************************/
				virtual bool BeginObjectValue(EA::Json::JsonDomObject& domObject, const char* pName, size_t nNameLength, EA::Json::JsonDomObjectValue&);	
				virtual bool EndObjectValue(EA::Json::JsonDomObject& domObject, EA::Json::JsonDomObjectValue&);
				virtual bool String(EA::Json::JsonDomString&, const char* pValue, size_t nValueLength, const char* pText, size_t nLength);
				virtual bool Integer(EA::Json::JsonDomInteger&, int64_t value, const char* pText, size_t nLength);

				eastl::string mErrorMessage;
				int64_t mErrorCode;

		private:
			eastl::string mJsonFullPath;
		};
	} // FifaStats
} // Blaze

#endif // BLAZE_FIFA_STATS_HTTP_RESPONSE_H

