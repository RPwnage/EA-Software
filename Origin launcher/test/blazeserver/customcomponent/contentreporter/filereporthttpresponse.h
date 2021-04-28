/*************************************************************************************************/
/*!
    \file   FileReportHttpResponse.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FILE_REPORT_HTTP_RESPONSE_H
#define BLAZE_FILE_REPORT_HTTP_RESPONSE_H

/*** Include files *******************************************************************************/
#include "framework/connection/outboundhttpconnectionmanager.h"
#include "framework/protocol/outboundhttpresult.h"

#include "EAJson/JsonDomReader.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
	namespace ContentReporter
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

				const char* GetStatus() { return mStatus.c_str(); }
				const char* GetGUID() { return mGUID.c_str(); }
				const char* GetErrorMessage() { return mErrorMessage.c_str(); }
				bool HasErrorMessage() { return mErrorMessage.empty(); }
		private:
			eastl::string mStatus;
			eastl::string mGUID;
			eastl::string mErrorMessage;
			eastl::string mJsonFullPath;
		};
	} // ContentReporter
} // Blaze

#endif // BLAZE_FILE_REPORT_HTTP_RESPONSE_H

