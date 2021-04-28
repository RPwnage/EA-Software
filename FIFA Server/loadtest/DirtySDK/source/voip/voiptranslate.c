/*H********************************************************************************/
/*!
    \File voiptranslate.c

    \Description
        Voip translation API wrapping Cloud-based text translation services, supporting
        IBM Watson, Microsoft Speech Service, Google Speech, and Amazon Polly.
        Translation requests may be up to 255 characters in length, and overlapping
        requests are queued in order.

    \Notes
        References:

        IBM Language Translator: https://cloud.ibm.com/apidocs/language-translator
        Microsoft Translator Text: https://docs.microsoft.com/en-us/azure/cognitive-services/translator/
        Google Translate: https://cloud.google.com/translate/docs/reference/translate
        Amazon Translate: https://docs.aws.amazon.com/translate/latest/dg/what-is.html
            https://docs.aws.amazon.com/translate/latest/dg/API_TranslateText.html
        

    \Copyright
        Copyright 2019 Electronic Arts

    \Version 01/28/2019 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/util/aws.h"
#include "DirtySDK/util/base64.h"
#include "DirtySDK/util/jsonformat.h"
#include "DirtySDK/util/jsonparse.h"
#include "DirtySDK/voip/voipdef.h"

#include "DirtySDK/voip/voiptranslate.h"

/*** Defines **********************************************************************/

#define VOIPTRANSLATE_MEMID ('vtrn')

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct VoipTranslateConfigT
{
    VoipSpeechProviderE eProvider;          //!< configured provider
    char strUrl[256];                       //!< URL for text-to-speech request
    char strKey[128];                       //!< API key required for service authentication
} VoipTranslateConfigT;

//! translation request data
typedef struct VoipTranslateRequestT
{
    struct VoipTranslateRequestT *pNext;
    char strText[VOIPTRANSLATE_MAX];
    char strSrcLang[32];
    char strDstLang[32];
    int8_t iUserIndex;
    const void *pUserData;
} VoipTranslateRequestT;

struct VoipTranslateRefT
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // module states
    enum
    {
        ST_FAIL=-1,                     //!< fail
        ST_IDLE,                        //!< idle
//        ST_CONN,                        //!< connecting
//        ST_SEND,                        //!< sending text to translate
        ST_RECV                         //!< receiving translated result
    } eState;

    ProtoHttpRefT *pProtoHttp;              //!< http transport module to handle translate request

    VoipTranslateRequestT *pRequest;        //!< list of queued requests, if any

    VoipSpeechProviderE eProvider;          //!< configured provider
    char strUrl[256];                       //!< URL for text translation request
    char strKey[128];                       //!< API key required for service authentication

    char strHead[256];                      //!< http head for translate request
    char strBody[512];                      //!< http body for translate request
    const char *pBody;                      //!< pointer to start of body (may not match buffer start)

    VoipTextToSpeechMetricsT Metrics;       //!< usage metrics of the translate module
    uint32_t uTtsStartTime;                 //!< time when we sent the request

    uint16_t aJsonParseBuf[512];            //!< buffer to parse JSON results
    char    strResponse[1024];              //!< translation server response
    char    strTranslation[VOIPTRANSLATE_MAX];  //!< translation result

    const void *pUserData;                  //!< user data associated with current request

    uint8_t bStart;                         //!< TRUE if start of stream download, else FALSE
    uint8_t bActive;                        //!< TRUE if stream is active, else FALSE
    int8_t  iUserIndex;                     //!< index of local user current request is being made for
    int8_t  iVerbose;                       //!< verbose debug level (debug only)
};

/*** Variables ********************************************************************/

//! global config state
static VoipTranslateConfigT _VoipTranslate_Config = { VOIPSPEECH_PROVIDER_NONE, "", "" };

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _VoipTranslateCustomHeaderCb

    \Description
        Custom header callback used to sign AWS requests

    \Input *pState          - http module state
    \Input *pHeader         - pointer to http header buffer
    \Input uHeaderSize      - size of http header buffer
    \Input *pUserRef        - voiptranslate ref

    \Output
        int32_t             - output header length

    \Version 12/28/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranslateCustomHeaderCb(ProtoHttpRefT *pState, char *pHeader, uint32_t uHeaderSize, void *pUserRef)
{
    VoipTranslateRefT *pVoipTranslate = (VoipTranslateRefT *)pUserRef;
    int32_t iHdrLen = (int32_t)strlen(pHeader);
    
    // if amazon and we have room, sign the request
    if ((pVoipTranslate->eProvider != VOIPSPEECH_PROVIDER_AMAZON) || (uHeaderSize < (unsigned)iHdrLen))
    {
        return(iHdrLen);
    }

    // sign the request and return the updated size
    return(AWSSignSigV4(pHeader, uHeaderSize, pVoipTranslate->pBody, pVoipTranslate->strKey, "translate", NULL));
}

/*F********************************************************************************/
/*!
    \Function _VoipTranslateBasicAuth

    \Description
        Encode Basic HTTP authorization header as per https://tools.ietf.org/html/rfc7617

    \Input *pBuffer     - [out] output buffer for encoded base64 string
    \Input iBufSize     - size of output buffer
    \Input *pUser       - user identifer
    \Input *pPass       - user password

    \Output
        const char *    - pointer to output buffer

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipTranslateBasicAuth(char *pBuffer, int32_t iBufSize, const char *pUser, const char *pPass)
{
    char strAuth[128];
    ds_snzprintf(strAuth, sizeof(strAuth), "%s:%s", pUser, pPass);
    Base64Encode2(strAuth, (int32_t)strlen(strAuth), pBuffer, iBufSize);
    return(pBuffer);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranslateRequestAdd

    \Description
        Queue request for later sending

    \Input *pVoipTranslate  - pointer to module state
    \Input iUserIndex       - local user index of user who is requesting speech synthesis
    \Input *pSrcLang        - src language to translate from (empty string if unknown)
    \Input *pDstLang        - dst language to translate to
    \Input *pText           - text to be translated
    \Input *pUserData       - user data to be returned along with translation result

    \Output
        int32_t             - negative=failure, else success

    \Version 11/09/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranslateRequestAdd(VoipTranslateRefT *pVoipTranslate, int32_t iUserIndex, const char *pSrcLang, const char *pDstLang, const char *pText, const void *pUserData)
{
    VoipTranslateRequestT *pRequest;

    // allocate and clear the request
    if ((pRequest = DirtyMemAlloc(sizeof(*pRequest), VOIPTRANSLATE_MEMID, pVoipTranslate->iMemGroup, pVoipTranslate->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voipTranslate: could not allocate request\n"));
        pVoipTranslate->Metrics.uErrorCount += 1;
        return(-1);
    }
    ds_memclr(pRequest, sizeof(*pRequest));

    // copy the request data
    ds_strnzcpy(pRequest->strText, pText, sizeof(pRequest->strText));
    ds_strnzcpy(pRequest->strSrcLang, pSrcLang, sizeof(pRequest->strSrcLang));
    ds_strnzcpy(pRequest->strDstLang, pDstLang, sizeof(pRequest->strDstLang));
    pRequest->iUserIndex = iUserIndex;
    pRequest->pUserData = pUserData;

    // add to queue
    pRequest->pNext = pVoipTranslate->pRequest;
    pVoipTranslate->pRequest = pRequest;

    // return success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranslateRequestGet

    \Description
        Get queued request

    \Input *pVoipTranslate  - pointer to module state
    \Input *pRequest        - [out] storage for request (may be null)

    \Version 11/09/2018 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipTranslateRequestGet(VoipTranslateRefT *pVoipTranslate, VoipTranslateRequestT *pRequest)
{
    VoipTranslateRequestT **ppRequest;
    // get oldest request (we add to head, so get from tail)
    for (ppRequest = &pVoipTranslate->pRequest; (*ppRequest)->pNext != NULL; ppRequest = &((*ppRequest)->pNext))
        ;
    // copy request
    if (pRequest != NULL)
    {
        ds_memcpy_s(pRequest, sizeof(*pRequest), *ppRequest, sizeof(**ppRequest));
    }
    // free request
    DirtyMemFree(*ppRequest, VOIPTRANSLATE_MEMID, pVoipTranslate->iMemGroup, pVoipTranslate->pMemGroupUserData);
    // remove from list
    *ppRequest = NULL;
}

/*F********************************************************************************/
/*!
    \Function _VoipTranslateFormatHeadWatson

    \Description
        Format head and url of Watson translation request

    \Input *pVoipTranslate  - pointer to module state
    \Input *pUrl            - [out] buffer for formatted url
    \Input iUrlLen          - length of url buffer
    \Input *pHead           - [out] buffer for formatted request header
    \Input iHeadLen         - length of header buffer

    \Output
        int32_t             - negative=failure, else success

    \Version 10/18/2019 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipTranslateFormatHeadWatson(VoipTranslateRefT *pVoipTranslate, char *pUrl, int32_t iUrlLen, char *pHead, int32_t iHeadLen)
{
    char strAuth[128];
    int32_t iOffset=0;

    // encode Basic authorization string with string apikey:<key>
    _VoipTranslateBasicAuth(strAuth, sizeof(strAuth), "apikey", pVoipTranslate->strKey);

    // format request header
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "Content-Type: application/json\r\n");
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "Authorization: Basic %s\r\n", strAuth);

    // format url
    ds_snzprintf(pUrl, iUrlLen, "%s/v3/translate?version=2018-05-01", pVoipTranslate->strUrl);
    return(pUrl);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranslateFormatBodyWatson

    \Description
        Format body of Watson translation request

    \Input *pVoipTranslate  - pointer to module state
    \Input *pBody           - [out] buffer to hold request body
    \Input iBodyLen         - buffer length
    \Input *pSrcLang        - language to translate from
    \Input *pDstLang        - language to translate to
    \Input *pText           - pointer to text request

    \Output
        int32_t             - negative=failure, else success

    \Version 10/16/2019 (jbrookes)
*/
/********************************************************************************F*/
static char *_VoipTranslateFormatBodyWatson(VoipTranslateRefT *pVoipTranslate, char *pBody, int32_t iBodyLen, const char *pSrcLang, const char *pDstLang, const char *pText)
{
    char strModel[8];
    JsonInit(pBody, iBodyLen, 0);
    JsonArrayStart(pBody, "text");
    JsonAddStr(pBody, NULL, pText);
    JsonArrayEnd(pBody);
    ds_snzprintf(strModel, sizeof(strModel), "%s-%s", pSrcLang, pDstLang);
    JsonAddStr(pBody, "model_id", strModel);
    pBody = JsonFinish(pBody);
    return(pBody);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranslateFormatHeadMicrosoft

    \Description
        Format head and url of Microsoft translation request

    \Input *pVoipTranslate  - pointer to module state
    \Input *pUrl            - [out] buffer for formatted url
    \Input iUrlLen          - length of url buffer
    \Input *pHead           - [out] buffer for formatted request header
    \Input iHeadLen         - length of header buffer
    \Input *pSrcLang        - language to translate from
    \Input *pDstLang        - language to translate to

    \Output
        int32_t             - negative=failure, else success

    \Version 11/06/2019 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipTranslateFormatHeadMicrosoft(VoipTranslateRefT *pVoipTranslate, char *pUrl, int32_t iUrlLen, char *pHead, int32_t iHeadLen, const char *pSrcLang, const char *pDstLang)
{
    int32_t iOffset=0;

    // format request header
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "Content-Type: application/json; charset=UTF-8\r\n");
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "Ocp-Apim-Subscription-Key: %s\r\n", pVoipTranslate->strKey);

    // format url
    ds_snzprintf(pUrl, iUrlLen, "%s&from=%s&to=%s", pVoipTranslate->strUrl, pSrcLang, pDstLang);
    return(pUrl);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranslateFormatBodyMicrosoft

    \Description
        Format body of Microsoft translation request

    \Input *pVoipTranslate  - pointer to module state
    \Input *pBody           - [out] buffer to hold request body
    \Input iBodyLen         - buffer length
    \Input *pText           - pointer to text request

    \Output
        int32_t             - negative=failure, else success

    \Version 11/06/2019 (jbrookes)
*/
/********************************************************************************F*/
static char *_VoipTranslateFormatBodyMicrosoft(VoipTranslateRefT *pVoipTranslate, char *pBody, int32_t iBodyLen, const char *pText)
{
    JsonInit(pBody, iBodyLen, JSON_FL_NOOUTERBRACE);
    JsonArrayStart(pBody, NULL);
    JsonObjectStart(pBody, NULL);
    JsonAddStr(pBody, "Text", pText);
    JsonObjectEnd(pBody);
    JsonArrayEnd(pBody);
    pBody = JsonFinish(pBody);
    return(pBody);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranslateFormatHeadGoogle

    \Description
        Format connection header for Google Translate

    \Input *pVoipTranslate  - pointer to module state
    \Input *pUrl            - [out] buffer for formatted url
    \Input iUrlLen          - length of url buffer
    \Input *pHead           - [out] buffer for formatted request header
    \Input iHeadLen         - length of header buffer
    \Input *pSrcLang        - src language to translate from
    \Input *pDstLang        - dst language to translate to
    \Input *pText           - text to translate

    \Output
        int32_t             - negative=failure, else success

    \Notes
        Ref: https://cloud.google.com/translate/docs/reference/translate

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipTranslateFormatHeadGoogle(VoipTranslateRefT *pVoipTranslate, char *pUrl, int32_t iUrlLen, char *pHead, int32_t iHeadLen, const char *pSrcLang, const char *pDstLang, const char *pText)
{
    char strUrlEncode[512]="";
    // format request header
    *pHead = '\0';
    // url-encode request text
    ProtoHttpUrlEncodeStrParm(strUrlEncode, sizeof(strUrlEncode), "", pText);
    // format request url
    ds_snzprintf(pUrl, iUrlLen, "%s?q=%s&source=%s&target=%s&format=text&key=%s", pVoipTranslate->strUrl, strUrlEncode, pSrcLang, pDstLang, pVoipTranslate->strKey);
    // return url
    return(pUrl);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranslateFormatBodyGoogle

    \Description
        Format request body for Google Translate request

    \Input *pVoipTranslate  - pointer to module state
    \Input *pBody           - [out] buffer to hold request body
    \Input iBodyLen         - buffer length
    \Input *pText           - pointer to text request

    \Output
        int32_t             - negative=failure, else success

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
static char * _VoipTranslateFormatBodyGoogle(VoipTranslateRefT *pVoipTranslate, char *pBody, int32_t iBodyLen, const char *pText)
{
    *pBody = '\0';
    return(pBody);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranslateFormatHeadAmazon

    \Description
        Format connection header for Amazon Translate

    \Input *pVoipTranslate  - pointer to module state
    \Input *pUrl            - [out] buffer for formatted url
    \Input iUrlLen          - length of url buffer
    \Input *pHead           - [out] buffer for formatted request header
    \Input iHeadLen         - length of header buffer

    \Output
        int32_t             - negative=failure, else success

    \Version 11/18/2019 (jbrookes)
*/
/********************************************************************************F*/
static const char *_VoipTranslateFormatHeadAmazon(VoipTranslateRefT *pVoipTranslate, char *pUrl, int32_t iUrlLen, char *pHead, int32_t iHeadLen)
{
    int32_t iOffset=0;
    // format request header
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "Content-Type: application/x-amz-json-1.1\r\n");
    iOffset += ds_snzprintf(pHead+iOffset, iHeadLen-iOffset, "X-Amz-Target: AWSShineFrontendService_20170701.TranslateText\r\n");
    // copy url
    ds_strnzcpy(pUrl, pVoipTranslate->strUrl, iUrlLen);
    return(pUrl);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranslateFormatBodyAmazon

    \Description
        Format request body for Amazon Translate

    \Input *pVoipTranslate  - pointer to module state
    \Input *pBody           - [out] buffer to hold request body
    \Input iBodyLen         - buffer length
    \Input *pSrcLang        - language to translate from
    \Input *pDstLang        - language to translate to
    \Input *pText           - pointer to text request

    \Output
        int32_t             - negative=failure, else success

    \Version 11/18/2019 (jbrookes)
*/
/********************************************************************************F*/
static char *_VoipTranslateFormatBodyAmazon(VoipTranslateRefT *pVoipTranslate, char *pBody, int32_t iBodyLen, const char *pSrcLang, const char *pDstLang, const char *pText)
{
    JsonInit(pBody, iBodyLen, 0);
    JsonAddStr(pBody, "SourceLanguageCode", pSrcLang);
    JsonAddStr(pBody, "TargetLanguageCode", pDstLang);
    JsonAddStr(pBody, "Text", pText);
    pBody = JsonFinish(pBody);
    return(pBody);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranslateStart

    \Description
        Start translation request

    \Input *pVoipTranslate  - pointer to module state
    \Input iUserIndex       - local user index of user who is requesting speech synthesis
    \Input *pSrcLang        - src language to translate from (empty string if unknown)
    \Input *pDstLang        - dst language to translate to
    \Input *pText           - pointer to text request
    \Input *pUserData       - user data associated with request

    \Output
        int32_t             - ProtoHttp request result

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranslateStart(VoipTranslateRefT *pVoipTranslate, int32_t iUserIndex, const char *pSrcLang, const char *pDstLang, const char *pText, const void *pUserData)
{
    const char *pUrl, *pReq;
    char strUrl[1024], strSrcLang[3], strDstLang[3];
    int32_t iResult;

    // copy language (and discard extraneous stuff e.g. -US from en-US)
    ds_strnzcpy(strSrcLang, pSrcLang, sizeof(strSrcLang));
    ds_strnzcpy(strDstLang, pDstLang, sizeof(strDstLang));

    // format header/url and request body
    if (pVoipTranslate->eProvider == VOIPSPEECH_PROVIDER_MICROSOFT)
    {
        pUrl = _VoipTranslateFormatHeadMicrosoft(pVoipTranslate, strUrl, sizeof(strUrl), pVoipTranslate->strHead, sizeof(pVoipTranslate->strHead), strSrcLang, strDstLang);
        pReq = _VoipTranslateFormatBodyMicrosoft(pVoipTranslate, pVoipTranslate->strBody, sizeof(pVoipTranslate->strBody), pText);
    }
    else if (pVoipTranslate->eProvider == VOIPSPEECH_PROVIDER_GOOGLE)
    {
        pUrl = _VoipTranslateFormatHeadGoogle(pVoipTranslate, strUrl, sizeof(strUrl), pVoipTranslate->strHead, sizeof(pVoipTranslate->strHead), strSrcLang, strDstLang, pText);
        pReq = _VoipTranslateFormatBodyGoogle(pVoipTranslate, pVoipTranslate->strBody, sizeof(pVoipTranslate->strBody), pText);
    }
    else if (pVoipTranslate->eProvider == VOIPSPEECH_PROVIDER_IBMWATSON)
    {
        pUrl = _VoipTranslateFormatHeadWatson(pVoipTranslate, strUrl, sizeof(strUrl), pVoipTranslate->strHead, sizeof(pVoipTranslate->strHead));
        pReq = _VoipTranslateFormatBodyWatson(pVoipTranslate, pVoipTranslate->strBody, sizeof(pVoipTranslate->strBody), strSrcLang, strDstLang, pText);
    }
    else if (pVoipTranslate->eProvider == VOIPSPEECH_PROVIDER_AMAZON)
    {
        pUrl = _VoipTranslateFormatHeadAmazon(pVoipTranslate, strUrl, sizeof(strUrl), pVoipTranslate->strHead, sizeof(pVoipTranslate->strHead));
        pReq = _VoipTranslateFormatBodyAmazon(pVoipTranslate, pVoipTranslate->strBody, sizeof(pVoipTranslate->strBody), strSrcLang, strDstLang, pText);
    }
    else
    {
        NetPrintf(("voiptranslate: undefined provider\n"));
        return(-1);
    }
    NetPrintfVerbose((pVoipTranslate->iVerbose, 1, "voiptranslate: request body\n%s\n", pReq));
    pVoipTranslate->pBody = pReq;

    // set header
    ProtoHttpControl(pVoipTranslate->pProtoHttp, 'apnd', 0, 0, pVoipTranslate->strHead);

    pVoipTranslate->Metrics.uEventCount += 1;
    pVoipTranslate->Metrics.uCharCountSent += (uint32_t)strlen(pText);
    pVoipTranslate->uTtsStartTime = NetTick();

    // make the request
    if ((iResult = ProtoHttpPost(pVoipTranslate->pProtoHttp, pUrl, pReq, -1, FALSE)) >= 0)
    {
        NetPrintfVerbose((pVoipTranslate->iVerbose, 0, "voiptranslate: translating %s->%s: %s\n", strSrcLang, strDstLang, pText));
        pVoipTranslate->bActive = TRUE;
        pVoipTranslate->eState = ST_RECV;
    }
    else
    {
        NetPrintf(("voiptranslate: failed request\n"));
        pVoipTranslate->Metrics.uErrorCount += 1;
        pVoipTranslate->eState = ST_FAIL;
    }

    // save user data associated with this request
    pVoipTranslate->pUserData = pUserData;

    // return to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranslateParseResponseWatson

    \Description
        Parse response from Watson translation service

    \Input *pVoipTranslate  - pointer to module state
    \Input *pResponse       - server response
    \Input *pResult         - parse result buffer
    \Input iResultSize      - length of result buffer

    \Output
        int32_t             - negative=failure, else success

    \Version 10/16/2019 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranslateParseResponseWatson(VoipTranslateRefT *pVoipTranslate, const char *pResponse, char *pResult, int32_t iResultSize)
{
    const char *pCurrent;
    uint16_t *pJsonParseBuf;
    int32_t iResult = -1;

    // parse the response
    if (JsonParse(pVoipTranslate->aJsonParseBuf, sizeof(pVoipTranslate->aJsonParseBuf)/sizeof(pVoipTranslate->aJsonParseBuf[0]), pResponse, -1) == 0)
    {
        NetPrintf(("voiptranslate: warning: parse results truncated\n"));
    }
    pJsonParseBuf = pVoipTranslate->aJsonParseBuf;

    // check for translation result
    if ((pCurrent = JsonFind2(pJsonParseBuf, NULL, "translations[", 0)) != NULL)
    {
        JsonGetString(JsonFind2(pJsonParseBuf, pCurrent, ".translation", 0), pResult, iResultSize, "");
        iResult = 1;
    }
    // process error, if there is one
    else if ((pCurrent = JsonFind2(pJsonParseBuf, NULL, "error", 0)) != NULL)
    {
        char strText[128];
        int32_t iCode = JsonGetInteger(JsonFind2(pJsonParseBuf, NULL, "code", 0), 0);
        JsonGetString(pCurrent, strText, sizeof(strText), "");
        ds_snzprintf(pResult, iResultSize, "error %d (%s)", iCode, strText);
    }

    // return result to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranslateParseResponseGoogle

    \Description
        Parse response from Google Cloud translation service

    \Input *pVoipTranslate  - pointer to module state
    \Input *pResponse       - server response
    \Input *pResult         - parse result buffer
    \Input iResultSize      - length of result buffer

    \Output
        int32_t             - negative=failure, else success

    \Version 09/27/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranslateParseResponseGoogle(VoipTranslateRefT *pVoipTranslate, const char *pResponse, char *pResult, int32_t iResultSize)
{
    const char *pCurrent;
    uint16_t *pJsonParseBuf;
    int32_t iResult = -1;

    // parse the response
    if (JsonParse(pVoipTranslate->aJsonParseBuf, sizeof(pVoipTranslate->aJsonParseBuf)/sizeof(pVoipTranslate->aJsonParseBuf[0]), pResponse, -1) == 0)
    {
        NetPrintf(("voiptranslate: warning: parse results truncated\n"));
    }
    pJsonParseBuf = pVoipTranslate->aJsonParseBuf;

    // check for translation result
    if ((pCurrent = JsonFind2(pJsonParseBuf, NULL, "data.translations[", 0)) != NULL)
    {
        JsonGetString(JsonFind2(pJsonParseBuf, pCurrent, ".translatedText", 0), pResult, iResultSize, "");
        iResult = 1;
    }
    // process error, if there is one
    else if ((pCurrent = JsonFind2(pJsonParseBuf, NULL, "error", 0)) != NULL)
    {
        char strText[128];
        int32_t iCode = JsonGetInteger(JsonFind2(pJsonParseBuf, pCurrent, ".code", 0), 0);
        JsonGetString(JsonFind2(pJsonParseBuf, pCurrent, ".message", 0), strText, sizeof(strText), "");
        ds_snzprintf(pResult, iResultSize, "error %d (%s)", iCode, strText);
    }

    // return result to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranslateParseResponseMicrosoft

    \Description
        Parse response from Microsoft translation service

    \Input *pVoipTranslate  - pointer to module state
    \Input *pResponse       - server response
    \Input *pResult         - parse result buffer
    \Input iResultSize      - length of result buffer

    \Output
        int32_t             - negative=failure, else success

    \Version 11/06/2019 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranslateParseResponseMicrosoft(VoipTranslateRefT *pVoipTranslate, const char *pResponse, char *pResult, int32_t iResultSize)
{
    const char *pCurrent;
    uint16_t *pJsonParseBuf;
    int32_t iResult = -1;

    // parse the response
    if (JsonParse(pVoipTranslate->aJsonParseBuf, sizeof(pVoipTranslate->aJsonParseBuf)/sizeof(pVoipTranslate->aJsonParseBuf[0]), pResponse, -1) == 0)
    {
        NetPrintf(("voiptranslate: warning: parse results truncated\n"));
    }
    pJsonParseBuf = pVoipTranslate->aJsonParseBuf;

    // check for translation result
    if ((pCurrent = JsonFind2(pJsonParseBuf, NULL, "[", 0)) != NULL)
    {
        if ((pCurrent = JsonFind2(pJsonParseBuf, pCurrent, ".translations[", 0)) != NULL)
        {
            JsonGetString(JsonFind2(pJsonParseBuf, pCurrent, ".text", 0), pResult, iResultSize, "");
            iResult = 1;
        }
    }
    // process error, if there is one
    else if ((pCurrent = JsonFind2(pJsonParseBuf, NULL, "error", 0)) != NULL)
    {
        char strText[128];
        int32_t iCode = JsonGetInteger(JsonFind2(pJsonParseBuf, pCurrent, ".code", 0), 0);
        JsonGetString(JsonFind2(pJsonParseBuf, pCurrent, ".message", 0), strText, sizeof(strText), "");
        ds_snzprintf(pResult, iResultSize, "error %d (%s)", iCode, strText);
    }

    // return result to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranslateParseResponseAmazon

    \Description
        Parse response from Amazon Translation service

    \Input *pVoipTranslate  - pointer to module state
    \Input *pResponse       - server response
    \Input *pResult         - parse result buffer
    \Input iResultSize      - length of result buffer

    \Output
        int32_t             - negative=failure, else success

    \Version 11/18/2019 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranslateParseResponseAmazon(VoipTranslateRefT *pVoipTranslate, const char *pResponse, char *pResult, int32_t iResultSize)
{
    const char *pCurrent;
    uint16_t *pJsonParseBuf;
    int32_t iResult = -1;

    // parse the response
    if (JsonParse(pVoipTranslate->aJsonParseBuf, sizeof(pVoipTranslate->aJsonParseBuf)/sizeof(pVoipTranslate->aJsonParseBuf[0]), pResponse, -1) == 0)
    {
        NetPrintf(("voiptranslate: warning: parse results truncated\n"));
    }
    pJsonParseBuf = pVoipTranslate->aJsonParseBuf;

    // check for translation result
    if ((pCurrent = JsonFind2(pJsonParseBuf, NULL, "TranslatedText", 0)) != NULL)
    {
        JsonGetString(pCurrent, pResult, iResultSize, "");
        iResult = 1;
    }
    /* process error, if there is one; note that the "__type" response does not appear in translate documentation, but does show up in
       sample code, e.g. https://docs.aws.amazon.com/code-samples/latest/catalog/python-apigateway-aws_service-aws_service.py.html */
    else if ((pCurrent = JsonFind2(pJsonParseBuf, NULL, "__type", 0)) != NULL)
    {
        char strExcept[32], strMessage[128];
        JsonGetString(pCurrent, strExcept, sizeof(strExcept), "");
        JsonGetString(JsonFind2(pJsonParseBuf, pCurrent, "Message", 0), strMessage, sizeof(strMessage), "");
        ds_snzprintf(pResult, iResultSize, "%s (%s)", strExcept, strMessage);
    }

    // return result to caller
    return(iResult);
}


/*F********************************************************************************/
/*!
    \Function _VoipTranslateParseResponse

    \Description
        Parse response from translation service

    \Input *pVoipTranslate  - pointer to module state
    \Input *pResponse       - server response
    \Input *pResult         - parse result buffer
    \Input iResultSize      - length of result buffer

    \Output
        int32_t             - negative=failure, zero=continue receiving, else success

    \Version 08/30/2018 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTranslateParseResponse(VoipTranslateRefT *pVoipTranslate, const char *pResponse, char *pResult, int32_t iResultSize)
{
    int32_t iResult = -1;
    if (pVoipTranslate->eProvider == VOIPSPEECH_PROVIDER_IBMWATSON)
    {
        iResult = _VoipTranslateParseResponseWatson(pVoipTranslate, pResponse, pResult, iResultSize);
    }
    else if (pVoipTranslate->eProvider == VOIPSPEECH_PROVIDER_MICROSOFT)
    {
        iResult = _VoipTranslateParseResponseMicrosoft(pVoipTranslate, pResponse, pResult, iResultSize);
    }
    else if (pVoipTranslate->eProvider == VOIPSPEECH_PROVIDER_GOOGLE)
    {
        iResult = _VoipTranslateParseResponseGoogle(pVoipTranslate, pResponse, pResult, iResultSize);
    }
    else if (pVoipTranslate->eProvider == VOIPSPEECH_PROVIDER_AMAZON)
    {
        iResult = _VoipTranslateParseResponseAmazon(pVoipTranslate, pResponse, pResult, iResultSize);
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranslateUpdateRecv

    \Description
        Update receiving of translation response.

    \Input *pVoipTranslate  - module state

    \Output
        int32_t             - updated state

    \Version 12/14/2018 (jbrookes) Split from VoipTranscribeUpdate()
*/
/********************************************************************************F*/
static int32_t _VoipTranslateUpdateRecv(VoipTranslateRefT *pVoipTranslate)
{
    int32_t iResult;

    // see if there's anything to receive
    if ((iResult = ProtoHttpRecvAll(pVoipTranslate->pProtoHttp, pVoipTranslate->strResponse, sizeof(pVoipTranslate->strResponse))) >= 0)
    {
        // null terminate and log response
        pVoipTranslate->strResponse[iResult] = '\0';
        NetPrintfVerbose((pVoipTranslate->iVerbose, 2, "voiptranslate: response (%d bytes)\n%s\n", iResult, pVoipTranslate->strResponse));

        // parse the result
        if ((iResult = _VoipTranslateParseResponse(pVoipTranslate, pVoipTranslate->strResponse, pVoipTranslate->strTranslation, sizeof(pVoipTranslate->strTranslation))) > 0)
        {
#if 0
            // update transcription length metric
            uint32_t uTranscriptionLength = (uint32_t)strnlen(pVoipTranscribe->strTranscription, sizeof(pVoipTranscribe->strTranscription));
            pVoipTranscribe->Metrics.uCharCountRecv += uTranscriptionLength;
            pVoipTranscribe->Metrics.uDelay += NetTickDiff(NetTick(), pVoipTranscribe->uSttStartTime);
            if (uTranscriptionLength == 0)
            {
                // keep track of number of consecutive empty results
                pVoipTranscribe->iConsecEmptyCt += 1;
                // update overall empty result count
                pVoipTranscribe->Metrics.uEmptyResultCount += 1;
                // set backoff if appropriate
                _VoipTranscribeBackoffSet(pVoipTranscribe);
            }
            else
            {
                // reset consecutive empty result tracker
                pVoipTranscribe->iConsecEmptyCt = 0;
            }
            // reset consecutive error count metric
            pVoipTranscribe->iConsecErrorCt = 0;
#endif
            // log transcription and transition back to idle state
            NetPrintfVerbose((pVoipTranslate->iVerbose, 0, "voiptranslate: translation=%s\n", pVoipTranslate->strTranslation));
            pVoipTranslate->eState = ST_IDLE;
            pVoipTranslate->bActive = FALSE;
        }
        else if (iResult < 0)
        {
            NetPrintf(("voiptranslate: service error: %s\n", pVoipTranslate->strTranslation));
            pVoipTranslate->strTranslation[0] = '\0';
            pVoipTranslate->eState = ST_FAIL;
        }
    }
    else if ((iResult < 0) && (iResult != PROTOHTTP_RECVWAIT))
    {
        NetPrintf(("voiptranslate: recv() returned %d\n", iResult));
        pVoipTranslate->eState = ST_FAIL;
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipTranslateConfig

    \Description
        Configure the VoipTranslate module

    \Input *pVoipTranslate  - pointer to module state
    \Input eProvider        - VOIPSPEECH_PROVIDER_* (VOIPSPEECH_PROVIDER_NONE to disable)
    \Input *pUrl            - pointer to url to use for tts requests
    \Input *pKey            - pointer to authentication key to use for tts requests

    \Output
        uint32_t            - TRUE if configured successfully

    \Version 11/07/2018 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _VoipTranslateConfig(VoipTranslateRefT *pVoipTranslate, VoipSpeechProviderE eProvider, const char *pUrl, const char *pKey)
{
    uint8_t uRet = TRUE;
    NetCritEnter(NULL);
    if (eProvider != VOIPSPEECH_PROVIDER_NONE)
    {
        pVoipTranslate->eProvider = eProvider;
        ds_strnzcpy(pVoipTranslate->strUrl, pUrl, sizeof(pVoipTranslate->strUrl));
        ds_strnzcpy(pVoipTranslate->strKey, pKey, sizeof(pVoipTranslate->strKey));
    }
    else
    {
        NetPrintfVerbose((pVoipTranslate->iVerbose, 0, "voiptranslate: translation disabled\n"));
        pVoipTranslate->eProvider = eProvider;
        ds_memclr(pVoipTranslate->strUrl, sizeof(pVoipTranslate->strUrl));
        ds_memclr(pVoipTranslate->strKey, sizeof(pVoipTranslate->strKey));
        uRet = FALSE;
    }
    NetCritLeave(NULL);
    return(uRet);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipTranslateCreate

    \Description
        Create the translation module

    \Output
        VoipTranslateRefT *       - new module state, or NULL

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
VoipTranslateRefT *VoipTranslateCreate(void)
{
    VoipTranslateRefT *pVoipTranslate;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pVoipTranslate = DirtyMemAlloc(sizeof(*pVoipTranslate), VOIPTRANSLATE_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voiptranslate: could not allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pVoipTranslate, sizeof(*pVoipTranslate));
    pVoipTranslate->iMemGroup = iMemGroup;
    pVoipTranslate->pMemGroupUserData = pMemGroupUserData;
    pVoipTranslate->iVerbose = 1;

    // allocate http ref with default buffer size
    if ((pVoipTranslate->pProtoHttp = ProtoHttpCreate(0)) == NULL)
    {
        VoipTranslateDestroy(pVoipTranslate);
        return(NULL);
    }
    // set protohttp debug level
    ProtoHttpControl(pVoipTranslate->pProtoHttp, 'spam', 1, 0, NULL);
    // set to use keep-alive
    ProtoHttpControl(pVoipTranslate->pProtoHttp, 'keep', 1, 0, NULL);
    // enable reuse on post
    ProtoHttpControl(pVoipTranslate->pProtoHttp, 'rput', 1, 0, NULL);
    // set protohttp custom header callback, used to sign AWS requests
    ProtoHttpCallback(pVoipTranslate->pProtoHttp, _VoipTranslateCustomHeaderCb, NULL, pVoipTranslate);

    // configure for particular provider
    if (!_VoipTranslateConfig(pVoipTranslate, _VoipTranslate_Config.eProvider, _VoipTranslate_Config.strUrl, _VoipTranslate_Config.strKey))
    {
        NetPrintf(("voipTranslate: could not configure for provider\n"));
        VoipTranslateDestroy(pVoipTranslate);
        return(NULL);
    }

    // return ref to caller
    return(pVoipTranslate);
}

/*F********************************************************************************/
/*!
    \Function VoipTranslateConfig

    \Description
        Set global state to configure the VoipTranslate modules

    \Input eProvider        - VOIPSPEECH_PROVIDER_* (VOIPSPEECH_PROVIDER_NONE to disable)
    \Input *pUrl            - pointer to url to use for tts requests
    \Input *pKey            - pointer to authentication key to use for tts requests

    \Version 11/07/2018 (jbrookes)
*/
/********************************************************************************F*/
void VoipTranslateConfig(VoipSpeechProviderE eProvider, const char *pUrl, const char *pKey)
{
    NetCritEnter(NULL);
    _VoipTranslate_Config.eProvider = eProvider;
    ds_strnzcpy(_VoipTranslate_Config.strUrl, pUrl, sizeof(_VoipTranslate_Config.strUrl));
    ds_strnzcpy(_VoipTranslate_Config.strKey, pKey, sizeof(_VoipTranslate_Config.strKey));
    NetCritLeave(NULL);
}

/*F********************************************************************************/
/*!
    \Function VoipTranslateDestroy

    \Description
        Destroy the VoipTranslate module

    \Input *pVoipTranslate    - pointer to module state

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
void VoipTranslateDestroy(VoipTranslateRefT *pVoipTranslate)
{
    // destroy protostream module, if allocated
    if (pVoipTranslate->pProtoHttp != NULL)
    {
        ProtoHttpDestroy(pVoipTranslate->pProtoHttp);
    }
    // release any queued requests
    while (pVoipTranslate->pRequest!= NULL)
    {
        _VoipTranslateRequestGet(pVoipTranslate, NULL);
    }
    // dispose of module memory
    DirtyMemFree(pVoipTranslate, VOIPTRANSLATE_MEMID, pVoipTranslate->iMemGroup, pVoipTranslate->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function VoipTranslateInput

    \Description
        Input text to be translated

    \Input *pVoipTranslate  - pointer to module state
    \Input iUserIndex       - local user index of user who is requesting speech synthesis
    \Input *pSrcLang        - src language to translate from (empty string if unknown)
    \Input *pDstLang        - dst language to translate to
    \Input *pText           - text to be converted
    \Input *pUserData       - user data to be returned with translate result

    \Output
        int32_t             - zero=success, otherwise=failure

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipTranslateInput(VoipTranslateRefT *pVoipTranslate, int32_t iUserIndex, const char *pSrcLang, const char *pDstLang, const char *pText, const void *pUserData)
{
    // make sure a provider is configured
    if (pVoipTranslate->eProvider == VOIPSPEECH_PROVIDER_NONE)
    {
        NetPrintfVerbose((pVoipTranslate->iVerbose, 0, "voiptranslate: no provider configured\n"));
        return(-1);
    }
    // handle if there is already narration ongoing
    if (pVoipTranslate->bActive)
    {
        NetPrintfVerbose((pVoipTranslate->iVerbose, /*1*/0, "voiptranslate: queueing request '%s'\n", pText));
        return(_VoipTranslateRequestAdd(pVoipTranslate, iUserIndex, pSrcLang, pDstLang, pText, pUserData));
    }
    // if ready, start the request
    return(_VoipTranslateStart(pVoipTranslate, iUserIndex, pSrcLang, pDstLang, pText, pUserData));
}

/*F********************************************************************************/
/*!
    \Function VoipTranslateGet

    \Description
        Get translation if available; if a translation is available, this
        call copies it and clears it.

    \Input *pVoipTranslate  - pointer to module state
    \Input *pBuffer         - [out] output buffer
    \Input iBufLen          - size of output buffer
    \Input **ppUserData     - [out] stroage for user data associated with request; may be null

    \Output
        int32_t             - zero=no translation, else translation copied

    \Version 09/07/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipTranslateGet(VoipTranslateRefT *pVoipTranslate, char *pBuffer, int32_t iBufLen, void **ppUserData)
{
    if (pVoipTranslate->strTranslation[0] == '\0')
    {
        return(0);
    }
    ds_strnzcpy((char *)pBuffer, pVoipTranslate->strTranslation, iBufLen);
    pVoipTranslate->strTranslation[0] = '\0';
    if (ppUserData != NULL)
    {
        *ppUserData = (void *)pVoipTranslate->pUserData;
    }
    pVoipTranslate->pUserData = NULL;
    return(1);
}

/*F********************************************************************************/
/*!
    \Function VoipTranslateStatus

    \Description
        Get module status.

    \Input *pVoipTranslate  - pointer to module state
    \Input iStatus          - status selector
    \Input iValue           - selector specific
    \Input *pBuffer         - selector specific
    \Input iBufSize         - selector specific

    \Output
        int32_t             - selector specific

    \Notes
        Other status codes are passed down to the stream transport handler.

    \verbatim
        'ttsm' - get the VoipTextToSpeechMetricsT via pBuffer
    \endverbatim

    \Version 11/15/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipTranslateStatus(VoipTranslateRefT *pVoipTranslate, int32_t iStatus, int32_t iValue, void *pBuffer, int32_t iBufSize)
{
    if (iStatus == 'ttsm')
    {
        if ((pBuffer != NULL) && (iBufSize >= (int32_t)sizeof(VoipTextToSpeechMetricsT)))
        {
            ds_memcpy_s(pBuffer, iBufSize, &pVoipTranslate->Metrics, sizeof(VoipTextToSpeechMetricsT));
            return(0);
        }
        return(-1);
    }
    return(ProtoHttpStatus(pVoipTranslate->pProtoHttp, iStatus, pBuffer, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function VoipTranslateControl

    \Description
        Set control options

    \Input *pVoipTranslate  - pointer to module state
    \Input iControl         - control selector
    \Input iValue           - selector specific
    \Input iValue2          - selector specific
    \Input *pValue          - selector specific

    \Output
        int32_t             - selector specific

    \Notes
        iStatus can be one of the following:

        \verbatim
            'conf' - refresh config
            'spam' - set verbose debug level (debug only)
        \endverbatim

        Unhandled codes are passed through to the stream transport handler

    \Version 08/30/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipTranslateControl(VoipTranslateRefT *pVoipTranslate, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    // refresh config
    if (iControl == 'conf')
    {
        return(_VoipTranslateConfig(pVoipTranslate, _VoipTranslate_Config.eProvider, _VoipTranslate_Config.strUrl, _VoipTranslate_Config.strKey));
    }
    #if DIRTYCODE_LOGGING
    // set verbosity for us and pass through to stream transport handler
    if (iControl == 'spam')
    {
        pVoipTranslate->iVerbose = iValue;
    }
    #endif
    // if not handled, let transport handler take a stab at it
    return(ProtoHttpControl(pVoipTranslate->pProtoHttp, iControl, iValue, iValue2, pValue));
}

/*F********************************************************************************/
/*!
    \Function VoipTranslateUpdate

    \Description
        Update the narration module

    \Input *pVoipTranslate    - pointer to module state

    \Version 10/25/2018 (jbrookes)
*/
/********************************************************************************F*/
void VoipTranslateUpdate(VoipTranslateRefT *pVoipTranslate)
{
    // see if we need to start a queued translation request
    if ((pVoipTranslate->pRequest != NULL) && !pVoipTranslate->bActive)
    {
        VoipTranslateRequestT Request;
        _VoipTranslateRequestGet(pVoipTranslate, &Request);
        _VoipTranslateStart(pVoipTranslate, Request.iUserIndex, Request.strSrcLang, Request.strDstLang, Request.strText, Request.pUserData);
    }

    // give life to http
    ProtoHttpUpdate(pVoipTranslate->pProtoHttp);

    // process responses
    if (pVoipTranslate->eState == ST_RECV)
    {
        _VoipTranslateUpdateRecv(pVoipTranslate);
    }

    // process fail state
    if (pVoipTranslate->eState == ST_FAIL)
    {
        //$$todo - failure stuff
        pVoipTranslate->eState = ST_IDLE;
        pVoipTranslate->bActive = FALSE;
    }
}
