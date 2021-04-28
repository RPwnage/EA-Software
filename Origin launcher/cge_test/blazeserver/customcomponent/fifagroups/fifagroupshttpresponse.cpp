/*************************************************************************************************/
/*!
    \file   FifaGroupsHttpResponse.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FifaGroupsHttpResponse

    Creates an HTTP Response.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "fifagroupshttpresponse.h"

namespace Blaze
{
	namespace FifaGroups
	{
		HttpResponse::HttpResponse() :
			OutboundHttpResult(),
			EA::Json::IJsonDomReaderCallback(),
			mStatus(),
			mGUID(),
			mErrorMessage(),
			mErrorCode(0),
			mJsonFullPath()
		{
			mAttributes.clear();
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
				BLAZE_ERR_LOG(BlazeRpcLog::fifagroups, ("[HttpResponse].EndObjectValue: unexpected EndObjectValue."));
			}
			return true;
		}

		bool HttpResponse::String(EA::Json::JsonDomString&, const char* pValue, size_t nValueLength, const char* pText, size_t nLength)
		{
			if (blaze_stricmp(mJsonFullPath.c_str(), "/error/name") == 0)
			{
				mErrorMessage.sprintf("%s", pValue);
			}
			else if (blaze_stricmp(mJsonFullPath.c_str(), "/_id") == 0 || blaze_stricmp(mJsonFullPath.c_str(), "/group/_id") == 0)
			{
				mGUID.sprintf("%s", pValue);
			}
			else if (blaze_stricmp(mJsonFullPath.c_str(), "/status") == 0)
			{
				mStatus.sprintf("%s", pValue);
			}
			else if (blaze_strncmp(mJsonFullPath.c_str(), "/attributes/", 12) == 0 || blaze_strncmp(mJsonFullPath.c_str(), "/group/attributes/", 18) == 0)
			{

				const char* tempPath = mJsonFullPath.c_str();
				tempPath++;
				const char* key = strchr(tempPath, '/');
				key++;

				const char* tempKey = blaze_strnstr(key, "attributes/", 11);
				if (tempKey != NULL)
				{
					key = strchr(key, '/');
					key++;
				}

				int currentSize = mAttributes.size();
				mAttributes.resize(currentSize + 1, false);

				mAttributes[currentSize] = mAttributes.allocate_element();
				mAttributes[currentSize]->setType(TYPE_STRING);
				mAttributes[currentSize]->setSValue(pValue);
				mAttributes[currentSize]->setKey(key);
			}

			return true;
		}

		bool HttpResponse::Integer(EA::Json::JsonDomInteger&, int64_t value, const char* pText, size_t nLength)
		{
			if (blaze_stricmp(mJsonFullPath.c_str(), "/error/code") == 0)
			{
				mErrorCode = value;
			}
			else if (blaze_strncmp(mJsonFullPath.c_str(), "/attributes/", 12) == 0)
			{
				const char* tempPath = mJsonFullPath.c_str();
				tempPath++;
				const char* key = strchr(tempPath, '/');
				key++;

				int currentSize = mAttributes.size();
				mAttributes.resize(currentSize + 1, false);

				mAttributes[currentSize] = mAttributes.allocate_element();
				mAttributes[currentSize]->setType(TYPE_INT);
				mAttributes[currentSize]->setIValue(value);
				mAttributes[currentSize]->setKey(key);
			}
			return true;
		}

	} // FifaGroups
} // Blaze
