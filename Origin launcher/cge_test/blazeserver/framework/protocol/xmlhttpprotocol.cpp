/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class XmlHttpProtocol

    <Describe the responsibility of the class>

    \notes
        <Any additional detail including implementation notes and references.  Delete this
        section if there are no notes.>

        */
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include <stdio.h>
#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/connection/connection.h"
#include "framework/util/shared/blazestring.h"
#include "framework/util/shared/rawbuffer.h"
#include "framework/protocol/xmlhttpprotocol.h"
#include "framework/protocol/shared/httpprotocolutil.h"
#include "framework/protocol/shared/tdfdecoder.h"
#include "framework/util/locales.h"
#include "framework/component/blazerpc.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
\brief XmlHttpProtocol

Constructor.
*/
/*************************************************************************************************/
XmlHttpProtocol::XmlHttpProtocol()
    : mNextSeqno(0),
      mContentLength(-1),
      parseErrorCode(false),
      mErrorCode(0),
      mReply(XML_UNKNOWN),
      mMaxFrameSize(UINT16_MAX+1)
{
}

/*************************************************************************************************/
/*!
\brief ~XmlHttpProtocol

Destructor.
*/
/*************************************************************************************************/
XmlHttpProtocol::~XmlHttpProtocol()
{
}

/*********************************************************************************************/
/*!
\brief receive

Determine if the data in the buffer is a complete XML request.
If not, return the number of bytes still required to complete the frame.  Note that HTTP
requests are not fixed-length, and the only case where the required length will be
known is when parsing the body of a post based on the Content-Length header.  Where the
request is not complete but the amount of data required cannot be determined, some
arbitrary positive number will be returned.

NOTE: By asking for a chunk of data, we may end up requesting more data than the current
request contains.  It is then possible that we could pick off the beginning of a
subsequent pipelined HTTP request from the client.  If we expect pipelined requests,
we will either need to add some ability in our protocol framework to push back the excess
data, or we'll have to be much less efficient and request data in smaller chunks (i.e. just
one or a handful of bytes at a time).

\param[in]  buffer - Buffer to examine.

\return - 0 if frame is complete; number of bytes still required if frame is not complete
*/
/*********************************************************************************************/
bool XmlHttpProtocol::receive(RawBuffer& buffer, size_t& needed)
{
    BLAZE_TRACE_LOG(Log::HTTPXML, "[XmlHttpProtocol].receive: Receiving request");

    size_t available = buffer.datasize();

    if (available <= 0)
    {
        // just started, so trigger read first
        mContentLength = -1;
        needed = 1024;
        return true;
    }

    char8_t* data = reinterpret_cast<char8_t*>(buffer.data());

    if (mContentLength == -1)
    {
        const char8_t header[] = "\r\nContent-Length: ";
        size_t headerLen = strlen(header);
        const char8_t* str = blaze_strnistr(data, header, available);
        if (str == nullptr)
        {
            BLAZE_TRACE_LOG(Log::HTTPXML, "[XmlHttpProtocol].receive: Still need content length header for XML request");
            needed = 1024;
            return true;
        }
        if (blaze_strnstr(str + headerLen, "\r\n", available - (str + headerLen - data)) != nullptr)
        {
            blaze_str2int(str + headerLen, &mContentLength);
            if (mContentLength >= 0 && (uint32_t)mContentLength > mMaxFrameSize)
            {
                BLAZE_WARN_LOG(Log::HTTPXML, "[XmlHttpProtocol].receive: Incoming frame is too large (" << mContentLength << "); "
                    "max frame size allowed is " << mMaxFrameSize);
                needed = 0;
                return false;
            }
        }
        else
        {
            needed = 1024;
            return true;
        }
    }

    if (mContentLength >= 0)
    {
        const char8_t emptyLine[] = "\r\n\r\n";
        const char8_t* str = blaze_strnstr(data, emptyLine, available);

        // If we haven't finished the headers yet, don't know how much more data we need
        // so ask for a chunk
        if (str == nullptr)
        {
            BLAZE_TRACE_LOG(Log::HTTPXML, "[XmlHttpProtocol].receive: Still need more headers for XML request");
            needed = 1024;
            return true;
        }

        size_t desired = mContentLength + (str + strlen(emptyLine) - data);

        // If we have finished at least the headers, we know how much data we want
        if (available >= desired)
        {
            BLAZE_TRACE_LOG(Log::HTTPXML, "[XmlHttpProtocol].receive: Received complete XML request");
            needed = 0;
            return true;
        }
        else
        {
            BLAZE_TRACE_LOG(Log::HTTPXML, "[XmlHttpProtocol].receive: Still need more body for XML request");
            needed = (desired - available);
            return true;
        }
    }

    BLAZE_TRACE_LOG(Log::HTTPXML, "[XmlHttpProtocol].receive: Unable to acquire content length");
    needed = 0;
    return false;
}

/*********************************************************************************************/
/*!
\brief process

Parse the URL of an http request to determine what component and command the connection should
pass the request along to.

\param[in]  buffer - RawBuffer to read from.
\param[out] component - the component id determined by examining the URL
\param[out] command - the command id determined by examining the URL
\param[out] msgNum - the sequence number assigned to this request
\param[out] type - the type of message
*/
/*********************************************************************************************/
void XmlHttpProtocol::process(RawBuffer& buffer, RpcProtocol::Frame &frame, TdfDecoder& decoder)
{
    BLAZE_TRACE_LOG(Log::HTTPXML, "[XmlHttpProtocol].process: Examining URL.");

    frame.errorCode = ERR_OK;
    frame.context = 0;
    frame.componentId = RPC_COMPONENT_UNKNOWN;
    frame.commandId = RPC_COMMAND_UNKNOWN;
    frame.requestEncodingType = decoder.getType();

    // There should only be one session on a given HTTP connection, so simply set the index to 0
    frame.userIndex = 0;

    // Assign a sequence number to the protocol since the there isn't actually a seqno
    // encoded in the incoming request
    frame.msgNum = getNextSeqno();

    // assume reply first
    frame.messageType = RpcProtocol::REPLY;

    size_t available = buffer.datasize();
    if (available == 0)
    {
        return;
    }

    // Parse header names and values from the full request
    HttpHeaderMap headerMap(BlazeStlAllocator("XmlHttpProtocol::headerMap"));
    HttpProtocolUtil::HttpReturnCode code = HttpProtocolUtil::buildHeaderMap(buffer, headerMap);
    if (code != HttpProtocolUtil::HTTP_OK)
    {
        BLAZE_ERR_LOG(Log::HTTPXML, "[XmlHttpProtocol].process: Problem parsing headers from HTTP request (" << code << ")!");
        return;
    }

    const char8_t * value = nullptr;

    // extract the service name if supplied
    value = HttpProtocolUtil::getHeaderValue(headerMap, "X-BLAZE-ID");
    if (value != nullptr)
    {
        blaze_strnzcpy(frame.serviceName, value, MAX_SERVICENAME_LENGTH);
    }

    // If the BLAZE-SESSION header exists, use it to get the session key
    value = HttpProtocolUtil::getHeaderValue(headerMap, "BLAZE-SESSION");
    if (value != nullptr)
    {
        frame.setSessionKey(value);
    }

    // extract the sequence number if supplied
    value = HttpProtocolUtil::getHeaderValue(headerMap, "X-BLAZE-SEQNO");
    if (value != nullptr)
    {
        uint32_t seqno = 0;
        blaze_str2int(value, &seqno);
        if (seqno != 0)
        {
            frame.msgNum = seqno;
        }
    }

    // extract the component and command
    value = HttpProtocolUtil::getHeaderValue(headerMap, "X-BLAZE-COMPONENT");
    if (value != nullptr)
    {
        frame.componentId = BlazeRpcComponentDb::getComponentIdByName(value);

        value = HttpProtocolUtil::getHeaderValue(headerMap, "X-BLAZE-COMMAND");
        if (value != nullptr)
        {
            frame.commandId = BlazeRpcComponentDb::getCommandIdByName(frame.componentId, value);
        }
    }

    // remove the header, leaving only the XML body
    char8_t* data = reinterpret_cast<char8_t*>(buffer.data());
    const char8_t emptyLine[] = "\r\n\r\n";
    char8_t* str = blaze_strnstr(data, emptyLine, available);

    if (str == nullptr)
    {
        BLAZE_ERR_LOG(Log::HTTPXML, "[XmlHttpProtocol].process: missing header delimiter");
        return;
    }

    str += strlen(emptyLine);
    size_t headerLen = reinterpret_cast<uint8_t*>(str) - buffer.head();
    memcpy(buffer.head(), str, available - headerLen);
    buffer.trim(headerLen);

    // check response type
    XmlReader xmlReader(buffer);
    if (xmlReader.parse(this))
    {
        if (mReply == XML_REPLY_ERROR)
        {
            frame.messageType = RpcProtocol::ERROR_REPLY;
            frame.errorCode = mErrorCode;
        }
        else
        {
            frame.messageType = RpcProtocol::REPLY;
        }
    }
}

/*********************************************************************************************/
/*!
\brief framePayload

Setup the frame header in the given buffer using the provided request object.  The
buffer's data point on input will be set to point to the frame's payload and the payload
will be present.

On exit, the given buffer's data pointer must be pointing to the head of the frame.

\param[out] buffer - the buffer containing the response data into which header is written
\param[in] component - the component id
\param[in] command - the command id
\param[in] errorCode - the error code
\param[in] msgNum - the message num
\param[in] type - the message type
\param[in] payload - the buffer containing the payload
*/
/*********************************************************************************************/
void XmlHttpProtocol::framePayload(RawBuffer& buffer, const RpcProtocol::Frame &frame, int32_t dataSize, TdfEncoder& encoder, const EA::TDF::Tdf* tdf /*= nullptr*/)
{
    BLAZE_TRACE_LOG(Log::HTTPXML, "[XmlHttpProtocol].framePayload: Writing HTTP header.");

    if (tdf != nullptr)
    {
        encoder.encode(buffer, *tdf, &frame);
        dataSize = buffer.datasize();
        // Just do following to silence Coverity warning. Don't want to remove setting this in case someone uses it in future in the code below.
        // The above code is similar to other implementations of RpcProtocol (so a bit consistent).
        (void)dataSize; 
    }

    char8_t header[HEADER_SIZE];

    // build the request message (which starts with the method -- always GET for now)
    size_t len = blaze_snzprintf(header, sizeof(header), "GET /%s/%s",
        BlazeRpcComponentDb::getComponentNameById(frame.componentId),
        BlazeRpcComponentDb::getCommandNameById(frame.componentId, frame.commandId));

    if (frame.getSessionKey() != nullptr)
    {
        len += blaze_snzprintf(header + len, sizeof(header) - len, "/%s", frame.getSessionKey());
    }

    if (buffer.datasize() > 0)
    {
        len += blaze_strnzcat(header + len, "?", sizeof(header) - len);
    }

    memcpy(buffer.push(len), header, len);

    // ... and followed by the version and any header fields
    len = blaze_snzprintf(header, sizeof(header), " HTTP/1.1\r\n");
    len += blaze_snzprintf(header + len, sizeof(header) - len, "X-BLAZE-SEQNO: %u\r\n", frame.msgNum);
    // add any other header fields, e.g. Host ...

    // terminate with another CRLF (note that 2 CRLFs are needed to terminate)
    len += blaze_strnzcat(header + len, "\r\n", sizeof(header) - len);
    memcpy(buffer.tail(), header, len);
    buffer.put(len);
}

/*************************************************************************************************/
/*!
    brief Start Document event.

    This function gets called when the first XML element is encountered. Any whitespace,
    comments and the <?xml tag at the start of the document are skipped before this
    function is called.
*/
/*************************************************************************************************/
void XmlHttpProtocol::startDocument()
{
    mReply = XML_UNKNOWN;
}

/*************************************************************************************************/
/*!
    \brief End Document event.

    This function gets called when a close element matching the first open element
    is encountered. This marks the end of the XML document. Any data following this
    close element is ignored by the parser.
*/
/*************************************************************************************************/
void XmlHttpProtocol::endDocument()
{
}

/*************************************************************************************************/
/*!
    \brief Start Element event.

    This function gets called when an open element is encountered. (\<element\>)

    \param name name of the element. (Not null-terminated)
    \param nameLen length of the name of the element.
    \param attributes list of attributes for this element (can be nullptr if attributeCount = 0)
    \param attributeCount number of attributes.
    \note The name pointers of the element and any attributes passed to this function
    will point directly to the actual XML data. This is why the strings are not
    null-terminated and the length is provided instead.
*/
/*************************************************************************************************/
void XmlHttpProtocol::startElement(const char *name, size_t nameLen, const XmlAttribute *attributes, size_t attributeCount)
{
    if (mReply == XML_UNKNOWN)
    {
        if (nameLen == 5 && blaze_strnicmp(name, "error", 5) == 0)
        {
            mReply = XML_REPLY_ERROR;
        }
        else
        {
            mReply = XML_REPLY;
        }
    }
    else if (mReply == XML_REPLY_ERROR)
    {
        if (nameLen == 9 && blaze_strnicmp(name, "errorCode", 9) == 0)
        {
            parseErrorCode = true;
        }
    }
}

/*************************************************************************************************/
/*!
    \brief End Element event.

    This function gets called when a close element is encountered. (\</element\>)

    \param name name of the element. (Not null-terminated)
    \param nameLen length of the name of the element.
    \note The name pointer passed to this function will point directly to the
    name of the element in the actual XML data. This is why the string is not
    null-terminated and the length is provided instead.
*/
/*************************************************************************************************/
void XmlHttpProtocol::endElement(const char *name, size_t nameLen)
{
}

/*************************************************************************************************/
/*!
    \brief Characters event.

    This function gets called when XML characters are encountered.

    \param characters pointer to character data.
    \param charLen length of character data.
    \note The character pointer passed to this function will point directly to the
    character data in the actual XML data. This is why the data is not
    null-terminated and the length is provided instead.
*/
/*************************************************************************************************/
void XmlHttpProtocol::characters(const char *characters, size_t charLen)
{
    if (parseErrorCode)
    {
        blaze_str2int(characters, &mErrorCode);
        parseErrorCode = false; // done
    }
}

/*************************************************************************************************/
/*!
    \brief CDATA event.

    This function gets called when a CDATA section is encountered.

    \param data pointer to the data.
    \param dataLen length of the data.
*/
/*************************************************************************************************/
void XmlHttpProtocol::cdata(const char *data, size_t dataLen)
{
}

void XmlHttpProtocol::setMaxFrameSize(uint32_t maxFrameSize)
{
    mMaxFrameSize = maxFrameSize;
}

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

} // Blaze

