/*************************************************************************************************/
/*!
    \file   xmshd_createmedia.cpp

    $Header: //gosdev/games/FIFA/2014/Gen3/DEV/blazeserver/13.x/customcomponent/xmshd/xmshd_httputil.h $

    \attention
    (c) Electronic Arts Inc. 2013
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"

// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"
#include "framework/controller/controller.h"

// easfc includes
#include "xmshdslaveimpl.h"
#include "xmshd/tdf/xmshdtypes.h"

#include "xmshdconnection.h"

namespace Blaze
{
    namespace XmsHd
    {
		static const char8_t* RETURN_NEW_LINE = "\r\n";

		static const char8_t* HTTP_MULTIPART_BOUNDARY_DELIMITER = "--";

		static const char8_t* HTTP_MULTIPART_BLOCK_HEAD = "\r\nContent-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\nContent-Type: text/plain; charset=ISO-8859-1\r\n\r\n";
		static const char8_t* HTTP_MULTIPART_BOUNDARY_SECTION_META = "Content-Disposition: form-data; name=\"metadata\"\r\nContent-Type: application/json;charset=ISO-8859-1\r\n";

		static const char8_t* HTTP_SECTION_META_JSON_START = "{\r\n\t\"metadata\":\r\n\t[\r\n";
		static const char8_t* HTTP_SECTION_META_JSON_END = "\t]\r\n}\r\n";

		//===========================================================================================================================
		void WriteNewLine(char8_t* buffer, size_t& bufferPos, size_t bufferSize)
		{
			static size_t RETURN_NEW_LINE_SIZE = strlen(RETURN_NEW_LINE);

			if (buffer != nullptr)
			{
				blaze_strnzcat(buffer, RETURN_NEW_LINE, bufferSize);
			}

			bufferPos += RETURN_NEW_LINE_SIZE;
		}

		//===========================================================================================================================
		void WriteMultiPartBoundaryDelimiter(char8_t* buffer, size_t& bufferPos, size_t bufferSize)
		{
			static size_t HTTP_MULTIPART_BOUNDARY_DELIMITER_SIZE = strlen(HTTP_MULTIPART_BOUNDARY_DELIMITER);

			if (buffer != nullptr)
			{
				blaze_strnzcat(buffer, HTTP_MULTIPART_BOUNDARY_DELIMITER, bufferSize);
			}

			bufferPos += HTTP_MULTIPART_BOUNDARY_DELIMITER_SIZE;
		}


		//===========================================================================================================================
		void WriteMultiPartBoundary(char8_t* buffer, size_t& bufferPos, size_t bufferSize, const char8_t* boundaryStr)
		{
			static size_t MULTIPART_BOUNDARY_SIZE = strlen(boundaryStr);
			
			WriteMultiPartBoundaryDelimiter(buffer, bufferPos, bufferSize);

			if (buffer != nullptr)
			{
				blaze_strnzcat(buffer, boundaryStr, bufferSize);
			}

			bufferPos += MULTIPART_BOUNDARY_SIZE;
		}

		//===========================================================================================================================
		void WriteMultiPartBlockHead(char8_t* buffer, size_t& bufferPos, size_t bufferSize, const char8_t* fileName)
		{
			eastl::string multiPartBlockHeadStr;
			multiPartBlockHeadStr.sprintf(HTTP_MULTIPART_BLOCK_HEAD, fileName, fileName);
			size_t multiPartBlockHeadStrSize = multiPartBlockHeadStr.size();

			if (buffer != nullptr)
			{
				blaze_strnzcat(buffer, multiPartBlockHeadStr.c_str(), bufferSize);
			}

			bufferPos += multiPartBlockHeadStrSize;
		}

		//===========================================================================================================================
		void WriteMetaData_Json_Start(char8_t* buffer, size_t& bufferPos, size_t bufferSize)
		{
			static size_t HTTP_SECTION_META_JSON_START_SIZE = strlen(HTTP_SECTION_META_JSON_START);

			if (buffer != nullptr)
			{
				blaze_strnzcat(buffer, HTTP_SECTION_META_JSON_START, bufferSize);
			}

			bufferPos += HTTP_SECTION_META_JSON_START_SIZE;
		}

		//===========================================================================================================================
		void WriteMetaData_Json_End(char8_t* buffer, size_t& bufferPos, size_t bufferSize)
		{
			static size_t HTTP_SECTION_META_JSON_END_SIZE = strlen(HTTP_SECTION_META_JSON_END);

			if (buffer != nullptr)
			{
				blaze_strnzcat(buffer, HTTP_SECTION_META_JSON_END, bufferSize);
			}

			bufferPos += HTTP_SECTION_META_JSON_END_SIZE;
		}

		//===========================================================================================================================
		/*
			Sample Json Metadata structure

			{
			"name":"VProTitle",
			"type":"string",
			"value":"FIFA HD Roar!",
			"searchable":1
			}

		*/
		//===========================================================================================================================
		void WriteMetaData_Json_Element(char8_t* buffer, size_t& bufferPos, size_t bufferSize, Blaze::XmsHd::XmsAttributes* metaDataElement)
		{
			eastl::string metaDataElementStr;

			metaDataElementStr.sprintf("\t\t{\r\n");

			// Add the Name field
			if (strlen(metaDataElement->getName()) > 0)
			{
				metaDataElementStr.append_sprintf("\t\t\t\"name\":\"%s\",\r\n", metaDataElement->getName());
			}

			// Add the Type field
			if (strlen(metaDataElement->getValue()) > 0)
			{
				metaDataElementStr.append_sprintf("\t\t\t\"value\":\"%s\",\r\n", metaDataElement->getValue());
			}

			// Add the Type field
			if (strlen(metaDataElement->getType()) > 0)
			{
				metaDataElementStr.append_sprintf("\t\t\t\"type\":\"%s\",\r\n", metaDataElement->getType());
			}

			// Add the Match field
			if (strlen(metaDataElement->getMatch()) > 0)
			{
				metaDataElementStr.append_sprintf("\t\t\t\"type\":\"%s\",\r\n", metaDataElement->getMatch());
			}

			// Add the To field
			if (strlen(metaDataElement->getTo()) > 0)
			{
				metaDataElementStr.append_sprintf("\t\t\t\"to\":\"%s\",\r\n", metaDataElement->getTo());
			}

			// Add the From field
			if (strlen(metaDataElement->getFrom()) > 0)
			{
				metaDataElementStr.append_sprintf("\t\t\t\"from\":\"%s\",\r\n", metaDataElement->getFrom());
			}

			// Add the Searchable field
			metaDataElementStr.append_sprintf("\t\t\t\"searchable\":%d\r\n", metaDataElement->getSearchable() ? 1 : 0);

			metaDataElementStr.append_sprintf("\t\t}\r\n");


			size_t metaDataElementSize = metaDataElementStr.size();

			if (buffer != nullptr)
			{
				blaze_strnzcat(buffer, metaDataElementStr.c_str(), bufferSize);
			}

			bufferPos += metaDataElementSize;
		}

		//===========================================================================================================================
		/*
		META DATA Format

		HTTP_MULTIPART_BOUNDARY_DELIMITER
		DEFAULT_MULTIPART_BOUNDARY
		HTTP_MULTIPART_BOUNDARY_SECTION_META

		JSON format Metadata

		RETURN_NEW_LINE
		HTTP_MULTIPART_BOUNDARY_DELIMITER
		DEFAULT_MULTIPART_BOUNDARY
		*/
		//===========================================================================================================================
		void WriteMetaDataHeader(char8_t* buffer, size_t& bufferPos, size_t bufferSize, const char8_t* boundaryStr)
		{
			static size_t HTTP_MULTIPART_BOUNDARY_SECTION_META_SIZE = strlen(HTTP_MULTIPART_BOUNDARY_SECTION_META);

			// ==== Write MetaData Header ====

			WriteMultiPartBoundary(buffer, bufferPos, bufferSize, boundaryStr);
			WriteNewLine(buffer, bufferPos, bufferSize);

			if (buffer != nullptr)
			{
				blaze_strnzcat(buffer, HTTP_MULTIPART_BOUNDARY_SECTION_META, bufferSize);
			}

			bufferPos += HTTP_MULTIPART_BOUNDARY_SECTION_META_SIZE;

			WriteNewLine(buffer, bufferPos, bufferSize);
		}

		//===========================================================================================================================
		void WriteMetaData(char8_t* buffer, size_t& bufferPos, size_t bufferSize, PublishDataRequest& request)
		{
			// ==== Write MetaData Content ====
			WriteMetaData_Json_Start(buffer, bufferPos, bufferSize);

			PublishDataRequest::XmsAttributesList& metaDataList = request.getAttributes();

			PublishDataRequest::XmsAttributesList::iterator metaDataIdx = metaDataList.begin();
			PublishDataRequest::XmsAttributesList::iterator metaDataEnd = metaDataList.end();

			bool notFirstElement = false;
			for (; metaDataIdx != metaDataEnd; ++metaDataIdx)
			{
				if (notFirstElement)
				{
					if (buffer != nullptr)
					{
						blaze_strnzcat(buffer, ",", bufferSize);
					}

					bufferPos += 1;
				}

				WriteMetaData_Json_Element(buffer, bufferPos, bufferSize, (*metaDataIdx));

				notFirstElement = true;
			}

			WriteMetaData_Json_End(buffer, bufferPos, bufferSize);
		}

		//===========================================================================================================================
		/*
		 For each file

		 HTTP_MULTIPART_BLOCK_HEAD

		 <FileData>

		 HTTP_MULTIPART_BOUNDARY_DELIMITER = "--"
		 DEFAULT_MULTIPART_BOUNDARY = "-31415926535897932384626"

		*/
		//===========================================================================================================================
		void WriteContentData(char8_t* buffer, size_t& bufferPos, size_t bufferSize, const char8_t* boundaryStr, Blaze::XmsHd::XmsBinary* binary)
		{

			WriteMultiPartBlockHead(buffer, bufferPos, bufferSize, binary->getName());

			if (buffer != nullptr)
			{
				memcpy(buffer + bufferPos, binary->getData(), binary->getDataSize());
			}

			bufferPos += binary->getDataSize();

			WriteNewLine(buffer, bufferPos, bufferSize);
			WriteMultiPartBoundary(buffer, bufferPos, bufferSize, boundaryStr);
		}

		//===========================================================================================================================
		void WriteContentDatas(char8_t* buffer, size_t& bufferPos, size_t bufferSize, const char8_t* boundaryStr, PublishDataRequest& request)
		{
			PublishDataRequest::XmsBinaryList::iterator binaryIdx = request.getBinaries().begin();
			PublishDataRequest::XmsBinaryList::iterator binaryEnd = request.getBinaries().end();

			for (; binaryIdx != binaryEnd; ++binaryIdx)
			{
				Blaze::XmsHd::XmsBinary* xmsBinary = (*binaryIdx);

				WriteContentData(buffer, bufferPos, bufferSize, boundaryStr, xmsBinary);
			}
		}

		//===========================================================================================================================
    } // XmsHd
} // Blaze
