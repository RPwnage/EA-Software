/*************************************************************************************************/
/*!
    \file   XmsHdHttpUtil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_XMSHD_HTTPUTIL_H
#define BLAZE_XMSHD_HTTPUTIL_H

/*** Include files *******************************************************************************/
#include "xmshd/tdf/xmshdtypes.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
	namespace XmsHd
	{
		class PublishDataRequest;

		static const char* HTTP_HEADER_FIELD_CONTENT_TYPE_JSON = "Content-Type: text/json";
		static const char* HTTP_HEADER_FIELD_CONTENT_TYPE_MULTIPART = "Content-Type: multipart/mixed; boundary=%s";
		static const char* HTTP_HEADER_FIELD_CONTENT_DISPOSITION = "Content-Disposition: name=%s; filename=%s";

		void WriteNewLine(char8_t* buffer, size_t& bufferPos, size_t bufferSize);

		void WriteMultiPartBoundaryDelimiter(char8_t* buffer, size_t& bufferPos, size_t bufferSize);
		void WriteMultiPartBoundary(char8_t* buffer, size_t& bufferPos, size_t bufferSize, const char8_t* boundaryStr);

		void WriteMultiPartBlockHead(char8_t* buffer, size_t& bufferPos, size_t bufferSize, const char8_t* fileName);

		void WriteMetaData_Json_Start(char8_t* buffer, size_t& bufferPos, size_t bufferSize);
		void WriteMetaData_Json_End(char8_t* buffer, size_t& bufferPos, size_t bufferSize);
		void WriteMetaData_Json_Element(char8_t* buffer, size_t& bufferPos, size_t bufferSize, Blaze::XmsHd::XmsAttributes* metaDataElement);

		void WriteMetaDataHeader(char8_t* buffer, size_t& bufferPos, size_t bufferSize, const char8_t* boundaryStr);
		void WriteMetaData(char8_t* buffer, size_t& bufferPos, size_t bufferSize, PublishDataRequest& request);

		void WriteContentData(char8_t* buffer, size_t& bufferPos, size_t bufferSize, const char8_t* boundaryStr, Blaze::XmsHd::XmsBinary* binary);
		void WriteContentDatas(char8_t* buffer, size_t& bufferPos, size_t bufferSize, const char8_t* boundaryStr, PublishDataRequest& request);
	} // XmsHd
} // Blaze

#endif // BLAZE_XMSHD_HTTPUTIL_H

