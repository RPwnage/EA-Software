/*************************************************************************************************/
/*!
    \file   FifaStatsHttpResponse.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FifaStatsHttpResponse

    Creates an HTTP Response.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "fifastatshttpresponse.h"

namespace Blaze
{
	namespace FifaStats
	{
		HttpResponse::HttpResponse() :
			OutboundHttpResult(),
			EA::Json::IJsonDomReaderCallback(),
			mErrorMessage(),
			mErrorCode(0),
			mJsonFullPath()
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

			if (reader.SetString(data, response.datasize(), false) == true)
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
			if (NULL != pName && 0 != nNameLength && '\0' != pName[0])
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
			else
			{
				BLAZE_ERR_LOG(BlazeRpcLog::fifastats, ("[HttpResponse].EndObjectValue: unexpected EndObjectValue."));
			}
			return true;
		}

		bool HttpResponse::String(EA::Json::JsonDomString&, const char* pValue, size_t nValueLength, const char* pText, size_t nLength)
		{
			if (blaze_stricmp(mJsonFullPath.c_str(), "/error/name") == 0)
			{
				mErrorMessage.sprintf("%s", pValue);
			}

			return true;
		}

		bool HttpResponse::Integer(EA::Json::JsonDomInteger&, int64_t value, const char* pText, size_t nLength)
		{
			if (blaze_stricmp(mJsonFullPath.c_str(), "/error/code") == 0)
			{
				mErrorCode = value;
			}
			return true;
		}

	} // FifaStats
} // Blaze
