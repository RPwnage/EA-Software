/*************************************************************************************************/
/*!
    \file   FileReportHttpResponse.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FileReportHttpResponse

    Creates an HTTP Response.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "filereporthttpresponse.h"

namespace Blaze
{
	namespace ContentReporter
	{
		HttpResponse::HttpResponse() :
			OutboundHttpResult(),
			EA::Json::IJsonDomReaderCallback()
		{
		}

		//=======================================================================================
		HttpResponse::~HttpResponse()
		{
		}

		//=======================================================================================
		BlazeRpcError HttpResponse::decodeResult(RawBuffer& response)
		{
			BlazeRpcError retval = Blaze::ERR_OK;

			// Use default Blaze allocator
			Allocator* allocator = Allocator::getAllocator();

			EA::Json::JsonDomReader reader(allocator);
			const char8_t* data = reinterpret_cast<const char8_t*>(response.data());

			if (data != nullptr && reader.SetString(data, response.datasize(), false) == true)
			{
				EA::Json::JsonDomDocument node(allocator);

				if (reader.Build(node) == EA::Json::kSuccess)
				{
					node.Iterate(this);
				}
				else
				{
					WARN_LOG("[HttpResponse:" << this << "].decodeResult: Failed to build Json dom object.");

					// Copy data to temporary buffer in case data doesn't have null-terminator.
					eastl::string rawData(data, response.datasize());
					TRACE_LOG("[HttpResponse:" << this << "].decodeResult: Received data: " << rawData.c_str());
				}
			}

			return retval;
		}

		bool HttpResponse::BeginObjectValue(EA::Json::JsonDomObject& domObject, const char* pName, size_t nNameLength, EA::Json::JsonDomObjectValue&)
		{
			mJsonFullPath += '/';
			if (pName != nullptr && nNameLength != 0 && pName[0] != '\0')
			{
				mJsonFullPath += pName;
			}
			return true;
		}

		bool HttpResponse::EndObjectValue(EA::Json::JsonDomObject& domObject, EA::Json::JsonDomObjectValue&)
		{
			// Remove last added name
			eastl::string::size_type pos = mJsonFullPath.rfind('/');
			if (eastl::string::npos != pos)
			{
				mJsonFullPath.erase(pos);
			}

			return true;
		}

		bool HttpResponse::String(EA::Json::JsonDomString&, const char* pValue, size_t nValueLength, const char* pText, size_t nLength)
		{
			if (blaze_stricmp(mJsonFullPath.c_str(), "/message") == 0)
			{
				mErrorMessage += pValue;
			}
			else if (blaze_stricmp(mJsonFullPath.c_str(), "/uuid") == 0)
			{
				mGUID += pValue;
			}
			else if (blaze_stricmp(mJsonFullPath.c_str(), "/status") == 0)
			{
				mStatus += pValue;
			}
			
			return true;
		}

	} // ContentReporter
} // Blaze
