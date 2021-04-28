/*H********************************************************************************/
/*!
    \File protohttputil.c

    \Description
        This module implements HTTP support routines such as URL processing, header
        parsing, etc.

    \Copyright
        Copyright (c) Electronic Arts 2000-2012.

    \Version 1.0 12/18/2012 (jbrookes)  Refactored into separate module from protohttp.
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/proto/protohttputil.h"

//$$DEPRECATE - remove call from this module, require direct call instead - API IMPACTING
int32_t ProtoHttpGetLocationHeader(void *pState, const char *pInpBuf, char *pBuffer, int32_t iBufSize, const char **pHdrEnd);

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private variables

// Public variables


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ProtoHttpGetHeaderFieldName

    \Description
        Extract header value from the designated header field.  Passing pOutBuf=null
        and iOutLen=0 results in returning the size required to store the field,
        null-inclusive.

    \Input *pInpBuf - pointer to header text
    \Input *pOutBuf - [out] output for header field name
    \Input iOutLen  - size of output buffer

    \Output
        int32_t     - negative=error, positive=success

    \Version 03/17/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpGetHeaderFieldName(const char *pInpBuf, char *pOutBuf, int32_t iOutLen)
{
    int32_t iOutSize;

    for (iOutSize = 0; (iOutSize < iOutLen) && (pInpBuf[iOutSize] != ':') && (pInpBuf[iOutSize] != '\0'); iOutSize += 1)
    {
        pOutBuf[iOutSize] = pInpBuf[iOutSize];
    }
    if (iOutSize == iOutLen)
    {
        return(-1);
    }
    pOutBuf[iOutSize] = '\0';
    return(iOutSize);
}

/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoHttpExtractHeaderValue

    \Description
        Extract header value from the designated header field.  Passing pOutBuf=null
        and iOutLen=0 results in returning the size required to store the field,
        null-inclusive.

    \Input *pInpBuf - pointer to header text
    \Input *pOutBuf - [out] output for header field value, null for size query
    \Input iOutLen  - size of output buffer or zero for size query
    \Input **ppHdrEnd- [out] pointer past end of parsed header (optional)

    \Output
        int32_t     - negative=not found or not enough space, zero=success, positive=size query result

    \Version 03/17/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpExtractHeaderValue(const char *pInpBuf, char *pOutBuf, int32_t iOutLen, const char **ppHdrEnd)
{
    int32_t iValLen;

    // no input, nothing to look for
    if (pInpBuf == NULL)
    {
        return(-1);
    }

    // calc size and copy to buffer, if specified
    for (iValLen = 0; *pInpBuf != '\0' ; pInpBuf += 1, iValLen += 1)
    {
        // check for EOL (CRLF)
        if ((pInpBuf[0] == '\r') && (pInpBuf[1] == '\n'))
        {
            // check next character -- if whitespace we have a line continuation
            if ((pInpBuf[2] == ' ') || (pInpBuf[2] == '\t'))
            {
                // skip CRLF and LWS
                for (pInpBuf += 3; (*pInpBuf == ' ') || (*pInpBuf == '\t'); pInpBuf += 1)
                    ;
            }
            else // done
            {
                break;
            }
        }

        // copy to output buffer
        if (pOutBuf != NULL)
        {
            pOutBuf[iValLen] = *pInpBuf;
            if ((iValLen+1) >= iOutLen)
            {
                *pOutBuf = '\0';
                return(-1);
            }
        }
    }
    // set header end pointer, if requested
    if (ppHdrEnd != NULL)
    {
        *ppHdrEnd = pInpBuf;
    }
    // if a copy request, null-terminate and return success
    if (pOutBuf != NULL)
    {
        pOutBuf[iValLen] = '\0';
        return(0);
    }
    // if a size inquiry, return value length (null-inclusive)
    return(iValLen+1);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpParseHeaderCode

    \Description
        Parse header response code from HTTP header.

    \Input *pHdrBuf     - pointer to first line of HTTP header

    \Output
        int32_t         - parsed header code

    \Version 01/12/2010 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpParseHeaderCode(const char *pHdrBuf)
{
    int32_t iHdrCode;

    // skip to result code
    while ((*pHdrBuf != '\r') && (*pHdrBuf > ' '))
    {
        pHdrBuf += 1;
    }
    while ((*pHdrBuf != '\r') && (*pHdrBuf <= ' '))
    {
        pHdrBuf += 1;
    }
    // grab the result code
    for (iHdrCode = 0; (*pHdrBuf >= '0') && (*pHdrBuf <= '9'); pHdrBuf += 1)
    {
        iHdrCode = (iHdrCode * 10) + (*pHdrBuf & 15);
    }
    return(iHdrCode);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpFindHeaderValue

    \Description
        Finds the header field value for the designated header field.  The input
        header text to find should not include formatting (leading \n or trailing
        colon).

    \Input *pHdrBuf     - input buffer
    \Input *pHeaderText - header field to find

    \Output
        char *          - pointer to header value text or null if not found

    \Version 03/17/2009 (jbrookes)
*/
/********************************************************************************F*/
const char *ProtoHttpFindHeaderValue(const char *pHdrBuf, const char *pHeaderText)
{
    char strSearchText[64];
    const char *pFoundText;

    ds_snzprintf(strSearchText, sizeof(strSearchText), "\n%s:", pHeaderText);
    if ((pFoundText = ds_stristr(pHdrBuf, strSearchText)) != NULL)
    {
        // skip header field name
        pFoundText += strlen(strSearchText);

        // skip any leading white-space
        while ((*pFoundText != '\0') && (*pFoundText != '\r') && (*pFoundText <= ' '))
        {
            pFoundText += 1;
        }
    }

    return(pFoundText);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpGetHeaderValue

    \Description
        Extract header value for the header field specified by pName from pHdrBuf.
        Passing pOutBuf=null and iOutLen=0 results in returning the size required
        to store the field, null-inclusive.

    \Input *pState  - module state
    \Input *pHdrBuf - header text
    \Input *pName   - header field name (case-insensitive)
    \Input *pBuffer - [out] pointer to buffer to store extracted header value (null for size query)
    \Input iBufSize - size of output buffer (zero for size query)
    \Input **pHdrEnd- [out] pointer past end of parsed header (optional)

    \Output
        int32_t     - negative=not found or not enough space, zero=success, positive=size query result

    \Version 03/24/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpGetHeaderValue(void *pState, const char *pHdrBuf, const char *pName, char *pBuffer, int32_t iBufSize, const char **pHdrEnd)
{
    int32_t iResult = -1;

    // check for location header request which is specially handled
    if ((pState != NULL) && (!ds_stricmp(pName, "location")))
    {
        iResult = ProtoHttpGetLocationHeader(pState, pHdrBuf, pBuffer, iBufSize, pHdrEnd);
    }
    else if ((pHdrBuf = ProtoHttpFindHeaderValue(pHdrBuf, pName)) != NULL)
    {
        // extract the header
        iResult = ProtoHttpExtractHeaderValue(pHdrBuf, pBuffer, iBufSize, pHdrEnd);
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpGetNextHeader

    \Description
        Get the next header name/value pair from the header buffer.  pHdrBuf should
        initially be pointing to the start of the header text buffer ("HTTP...") and
        this function can then be iteratively called to extract the individual header
        fields from the header one by one.  pHdrBuf could also start as the result
        of a call to ProtoHttpGetHeaderValue() if desired.  If the start of the
        buffer is not the end of line marker, it is assumed to be the header to
        retrieve.

    \Input *pState  - module state
    \Input *pHdrBuf - header text
    \Input *pName   - [out] header field name
    \Input iNameSize- size of header field name buffer
    \Input *pValue  - [out] pointer to buffer to store extracted header value (null for size query)
    \Input iValSize - size of output buffer (zero for size query)
    \Input **pHdrEnd- [out] pointer past end of parsed header

    \Output
        int32_t     - negative=not found or not enough space, zero=success, positive=size query result

    \Version 03/24/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpGetNextHeader(void *pState, const char *pHdrBuf, char *pName, int32_t iNameSize, char *pValue, int32_t iValSize, const char **pHdrEnd)
{
    int32_t iNameLen, iResult;

    // is this a first call?
    if (!strncmp(pHdrBuf, "HTTP", 4))
    {
        // skip http header
        for ( ; *pHdrBuf != '\r' && *pHdrBuf != '\0'; pHdrBuf += 1)
            ;
        if (*pHdrBuf == '\0')
        {
            return(-1);
        }
    }

    // skip crlf from previous line?
    if ((pHdrBuf[0] == '\r') && (pHdrBuf[1] == '\n'))
    {
        pHdrBuf += 2;
    }

    // get header name
    if ((iNameLen = _ProtoHttpGetHeaderFieldName(pHdrBuf, pName, iNameSize)) <= 0)
    {
        return(-1);
    }
    // skip header name and leading whitespace
    for (pHdrBuf += iNameLen + 1; (*pHdrBuf != '\0') && (*pHdrBuf != '\r') && (*pHdrBuf <= ' '); pHdrBuf += 1)
        ;

    // get header value
    iResult = ProtoHttpExtractHeaderValue(pHdrBuf, pValue, iValSize, pHdrEnd);
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpUrlEncodeIntParm

    \Description
        Url-encode an integer parameter.

    \Input *pBuffer - [out] pointer to buffer to append parameter to
    \Input iLength  - length of buffer
    \Input *pParm   - pointer to parameter (not encoded)
    \Input iValue   - integer to encode

    \Output
        int32_t     - negative=failure, zero=success

    \Version 11/30/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpUrlEncodeIntParm(char *pBuffer, int32_t iLength, const char *pParm, int32_t iValue)
{
    char strValue[32];
    char *pData;

    // format the value
    ds_snzprintf(strValue, sizeof(strValue), "%d", iValue);
    pData = strValue;

    // save room for null terminator
    iLength -= 1;

    // locate the append point
    for (; (*pBuffer != 0) && (iLength > 0); iLength--)
    {
        pBuffer++;
    }

    // add in the parameter (non encoded)
    for (; (*pParm != 0) && (iLength > 0); iLength--)
    {
        *pBuffer++ = *pParm++;
    }

    // add in the number
    for (; (*pData != 0) && (iLength > 0); iLength--)
    {
        *pBuffer++ = *pData++;
    }

    // null-terminate string
    *pBuffer = '\0';

    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpUrlEncodeStrParm

    \Description
        Url-encode a string parameter.

    \Input *pBuffer - [out] pointer to buffer to append parameter to
    \Input iLength  - length of buffer
    \Input *pParm   - pointer to url parameter (not encoded)
    \Input *pData   - string to encode

    \Output
        int32_t     - negative=failure, zero=success

    \Version 11/30/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpUrlEncodeStrParm(char *pBuffer, int32_t iLength, const char *pParm, const char *pData)
{
    // table of "safe" characters
    // 0=hex encode, non-zero=direct encode, @-O=valid hex digit (&15 to get value)
    static char _strSafe[256] =
        "0000000000000000" "0000000000000000"   // 0x00 - 0x1f
        "0000000000000000" "@ABCDEFGHI000000"   // 0x20 - 0x3f (allow digits)
        "0JKLMNO111111111" "1111111111100000"   // 0x40 - 0x5f (allow uppercase)
        "0JKLMNO111111111" "1111111111100000"   // 0x60 - 0x7f (allow lowercase)
        "0000000000000000" "0000000000000000"   // 0x80 - 0x9f
        "0000000000000000" "0000000000000000"   // 0xa0 - 0xbf
        "0000000000000000" "0000000000000000"   // 0xc0 - 0xdf
        "0000000000000000" "0000000000000000";  // 0xe0 - 0xff}

    return(ProtoHttpUrlEncodeStrParm2(pBuffer, iLength, pParm, pData, _strSafe));
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpUrlEncodeStrParm2

    \Description
        Url-encode a string parameter.

    \Input *pBuffer - [out] pointer to buffer to append parameter to
    \Input iLength  - length of buffer
    \Input *pParm   - pointer to url parameter (not encoded)
    \Input *pData   - string to encode
    \Input *pStrSafe- safe string table

    \Output
        int32_t     - negative=failure, zero=success

    \Version 11/30/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpUrlEncodeStrParm2(char *pBuffer, int32_t iLength, const char *pParm, const char *pData, const char *pStrSafe)
{
    // hex translation table
    static char _strHex[16] = "0123456789ABCDEF";

    // save room for null terminator
    iLength -= 1;

    // locate the append point
    for (; (*pBuffer != 0) && (iLength > 0); iLength--)
    {
        pBuffer++;
    }

    // add in the parameter (non encoded)
    for (; (*pParm != 0) && (iLength > 0); iLength--)
    {
        *pBuffer++ = *pParm++;
    }

    // add in the encoded data
    for (; (*pData != 0) && (iLength > 2); )
    {
        // see how we need to encode this
        if (pStrSafe[*(const uint8_t *)pData] == '0')
        {
            uint8_t ch = *pData++;
            *pBuffer++ = '%';
            *pBuffer++ = _strHex[ch>>4];
            *pBuffer++ = _strHex[ch&15];
            iLength -= 3;
        }
        else
        {
            *pBuffer++ = *pData++;
            iLength--;
        }
    }

    // make sure to add the extra data if a hex encode isn't necessary for the last bytes of pData
    for (; (*pData != 0) && (iLength > 0) && (pStrSafe[*(const uint8_t *)pData] != '0'); iLength--)
    {
        *pBuffer++ = *pData++;
    }

    // null-terminate string
    *pBuffer = '\0';

    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpUrlDecodeStrParm

    \Description
        Url-decode a string parameter.

    \Input *pBuffer - pointer to the buffer with the url-encoded string
    \Input *pData   - [out] location to write the url-decoded string
    \Input iDataSize- length of pData

    \Output
        int32_t     - negative=failure, zero=success
*/
/********************************************************************************F*/
int32_t ProtoHttpUrlDecodeStrParm(const char *pBuffer, char* pData, int32_t iDataSize)
{
    int32_t iPos = 0, iCount = 0;
    for (; pBuffer[iPos] != '\0' && iCount < iDataSize; iPos += 1)
    {
        const char ch = pBuffer[iPos];
        if (ch == '%')
        {
            // Save the base 16 number in a string to be converted
            // to an character
            char aTemp[3] = { '\0' }; //!< 2 character + null term
            aTemp[0] = pBuffer[iPos+1];
            aTemp[1] = pBuffer[iPos+2];
            iPos += 2;

            // Convert the base 16 number to a character
            // and write it to the output string
            pData[iCount++] = (char)strtol(aTemp, NULL, 16 /* Base */);
        }
        else
        {
            pData[iCount++] = ch;
        }
    }

    // Null-Terminate
    pData[iCount] = '\0';

    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpUrlParse

    \Description
        Parses a Url into component parts.

    \Input *pUrl        - Url to parse
    \Input *pKind       - [out] http request kind
    \Input iKindSize    - size of kind output buffer
    \Input *pHost       - [out] host name
    \Input iHostSize    - size of host output buffer
    \Input *pPort       - [out] request port
    \Input *pSecure     - [out] request security

    \Output
        const char *    - pointer past end of parsed Url

    \Version 07/01/2009 (jbrookes) First Version
*/
/********************************************************************************F*/
const char *ProtoHttpUrlParse(const char *pUrl, char *pKind, int32_t iKindSize, char *pHost, int32_t iHostSize, int32_t *pPort, int32_t *pSecure)
{
    uint8_t bPortSpecified;
    // parse the url into component parts
    return(ProtoHttpUrlParse2(pUrl, pKind, iKindSize, pHost, iHostSize, pPort, pSecure, &bPortSpecified));
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpUrlParse2

    \Description
        Parse information from given URL.

    \Input *pUrl        - pointer to input URL to parse
    \Input *pKind       - [out] storage for parsed URL kind (eg "http", "https")
    \Input iKindSize    - size of pKind buffer
    \Input *pHost       - [out] storage for parsed URL host
    \Input iHostSize    - size of pHost buffer
    \Input *pPort       - [out] storage for parsed port
    \Input *pSecure     - [out] storage for parsed security (0 or 1)
    \Input *pPortSpecified - [out] storage for whether port was specified

    \Output
        const char *    - pointer to URL arguments

    \Notes
        URI RFC http://www.ietf.org/rfc/rfc2396.txt 

    \Version 11/10/2004 (jbrookes) Split/combined from ProtoHttpGet() and ProtoHttpPost()
*/
/********************************************************************************F*/
const char *ProtoHttpUrlParse2(const char *pUrl, char *pKind, int32_t iKindSize, char *pHost, int32_t iHostSize, int32_t *pPort, int32_t *pSecure, uint8_t *pPortSpecified)
{
    char strKind[PROTOHTTPUTIL_SCHEME_MAX] = "";
    const char *s;
    int32_t i, iPort;

    // skip any leading white-space
    for ( ; (*pUrl != 0) && (*pUrl <= ' '); pUrl += 1)
        ;

    // see if there is a protocol reference (cf rfc2396 sec 3.1)
    for (s = pUrl; isalnum(*s) || (*s == '+') || (*s == '-') || (*s == '.'); s++)
        ;
    if (*s == ':')
    {
        // copy over the protocol kind
        ds_strsubzcpy(strKind, sizeof(strKind), pUrl, (int32_t)(s-pUrl));
        pUrl += (s-pUrl)+1;
    }
    ds_strnzcpy(pKind, strKind, iKindSize);

    // determine if secure connection
    *pSecure = (ds_stricmp(pKind, "https") == 0);

    // skip past any white space
    for ( ; (*pUrl != 0) && (*pUrl <= ' '); pUrl += 1)
        ;

    // skip optional //
    if ((pUrl[0] == '/') && (pUrl[1] == '/'))
    {
        pUrl += 2;
    }

    // extract the server name
    for (i = 0; i < (iHostSize-1); i++)
    {
        // make sure the data is valid
        if ((*pUrl <= ' ') || (*pUrl == '/') || (*pUrl == '?') || (*pUrl == ':'))
        {
            break;
        }
        pHost[i] = *pUrl++;
    }
    pHost[i] = '\0';

    // extract the port if included
    iPort = 0;
    if (*pUrl == ':')
    {
        for (pUrl++, iPort = 0; (*pUrl >= '0') && (*pUrl <= '9'); pUrl++)
        {
            iPort = (iPort * 10) + (*pUrl & 15);
        }
    }
    // if port is unspecified, set it here to a standard port
    if (iPort == 0)
    {
        iPort = *pSecure ? 443 : 80;
        *pPortSpecified = FALSE;
    }
    else
    {
        *pPortSpecified = TRUE;
    }
    *pPort = iPort;

    // skip past white space to arguments
    for ( ; (*pUrl != 0) && (*pUrl <= ' '); pUrl += 1)
        ;

    // return pointer to arguments
    return(pUrl);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpGetNextParam

    \Description
        Get the next query param name/value pair from the uri buffer. This function
        can be iteratively called to extra the individual params one by one.

    \Input *pState  - module state
    \Input *pUriBuf - path text
    \Input *pName   - [out] query key name
    \Input iNameSize- size of query key name buffer
    \Input *pValue  - [out] pointer to buffer to store extracted query value
    \Input iValSize - size of output buffer
    \Input **pUriEnd- [out] pointer past end of parsed query

    \Output
        int32_t     - negative=not found or not enough space, zero=success
*/
/********************************************************************************F*/
int32_t ProtoHttpGetNextParam(void *pState, const char *pUriBuf, char *pName, int32_t iNameSize, char *pValue, int32_t iValSize, const char **pUriEnd)
{
    const char* pEnd = NULL;

    // check if there is anything left to parse
    if (pUriBuf[0] == '\0')
    {
        return(-1);
    }

    // try to find the '?' character denoting the beginning
    // of the query string
    // if found set the new beginning one character after
    // otherwise we parse as it has already been removed
    if ((pEnd = strchr(pUriBuf, '?')) != NULL)
    {
        pUriBuf = pEnd + 1;
    }

    // try to find the end of the key
    if ((pEnd = strchr(pUriBuf, '=')) == NULL)
    {
        return(-1);
    }

    // copy the value and set the new position
    // we are skipping an extra character for '='
    ds_strsubzcpy(pName, iNameSize, pUriBuf, (int32_t)(pEnd-pUriBuf));
    pUriBuf = pEnd + 1;

    // try to find the end of the value
    if ((pEnd = strchr(pUriBuf, '&')) == NULL)
    {
        // go to the end of the string
        if ((pEnd = strchr(pUriBuf, '\0')) == NULL)
        {
            return(-1);
        }

        // copy the value and set the new position
        // we are not skipping an extra character as we have reached
        // the end of the string
        ds_strsubzcpy(pValue, iValSize, pUriBuf, (int32_t)(pEnd-pUriBuf));
        pUriBuf = pEnd;
    }
    else
    {
        // copy the value and set the new position
        // we are skipping an extra character for '&'
        ds_strsubzcpy(pValue, iValSize, pUriBuf, (int32_t)(pEnd-pUriBuf));
        pUriBuf = pEnd + 1;
    }

    // save the location
    if (pUriEnd != NULL)
    {
        *pUriEnd = pUriBuf;
    }

    return(0);
}
