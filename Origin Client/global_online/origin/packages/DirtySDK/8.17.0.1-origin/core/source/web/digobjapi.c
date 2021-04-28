/*H********************************************************************************/
/*!
    \File digobjapi.c

    \Description
        expose Digital Object BESL services through this API

    \Copyright
        Copyright (c) Electronic Arts 2006. ALL RIGHTS RESERVED.

    \Version 1.0 05/30/2006 (tchen) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

#include "dirtysock.h"
#include "dirtymem.h"
#include "protohttp.h"
#include "protofilter.h"
#include "netconn.h"
#include "lobbysort.h"
#include "xmlparse.h"

#include "digobjapi.h"

/*** Defines **********************************************************************/

//! verbose debugging
#define DIGOBJAPI_VERBOSE   (DIRTYCODE_DEBUG && FALSE)

//! path of digobj jsp
#define DIGOBJAPI_JSPURL    "easo/editorial/common/2007/digobj/digobj.jsp"

#define DIGOBJ_XMLSIZE      (28*1024)
/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

struct DigObjApiRefT
{
    ProtoHttpRefT       *pHttp;         //!< http module used for web transactions
    
    // module memory group
    int32_t             iMemGroup;      //!< module mem group id
    void                *pMemGroupUserData;  //!< user data associated with mem group
    
    char                strServer[256]; //!< server name
    char                strLKey[128];   //!< storage for user's LKey, required for web transactions
    char                strPersona[32]; //!< storage for user's persona name
    DigObjApiCallbackT  *pCallback;     //!< user event callback
    void                *pUserData;     //!< data for user event callback

    int32_t             iSpoolSize;     //!< size of spool buffer
    int32_t             iRecvSize;      //!< amount of data being received
    int32_t             iRecvOffset;    //!< recv transfer offset

    DigObjApiEventE     eCurEvent;      //!< current event status
    DigObjApiErrorE     eCurError;      //!< current error status
    DigObjApiIdT        aDigitalObjectIDs[DIGOBJAPI_MAXOBJ];       //!< current list of digital object ids
    int32_t             iObjCount;       //!< number of object IDs currently stored in aDigitalObjectIDs

    enum
    {
        ST_IDLE,                        //!< nothing going on
        ST_RDM,                         //!< redeem code operation in progress
        ST_FET                          //!< get digital object list operation in progress
    } eState;                           //!< module state

    char                strXml[DIGOBJ_XMLSIZE];  //!< buffer for receiving XML responses
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _DigObjApiFormatBaseUrl

    \Description
        Format string with base url.

    \Input *pDigObjApi  - digobj object ref
    \Input *pBuffer     - [out] pointer to buffer to format url in
    \Input iBufSize     - size of buffer
    \Input *pCmd        - pointer to command name

    \Version 12/10/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _DigObjApiFormatBaseUrl(DigObjApiRefT *pDigObjApi, char *pBuffer, int32_t iBufSize, const char *pCmd)
{
    if (!strncmp(pDigObjApi->strServer, "http", 4))
    {
        snzprintf(pBuffer, iBufSize, "%s?site=easo&cmd=%s&lkey=%s", pDigObjApi->strServer, pCmd, pDigObjApi->strLKey);
    }
    else
    {
        snzprintf(pBuffer, iBufSize, "http://%s/%s?site=easo&cmd=%s&lkey=%s", pDigObjApi->strServer, DIGOBJAPI_JSPURL, pCmd, pDigObjApi->strLKey);
    }
}

/*F********************************************************************************/
/*!
    \Function _DigObjApiDefaultCallback

    \Description
        Default DigObjApi user callback.

    \Input *pDigObjApi  - digobj object ref
    \Input eEvent       - callback event
    \Input *pUserData   - callback user data

    \Version 11/24/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _DigObjApiDefaultCallback(DigObjApiRefT *pDigObjApi, DigObjApiEventE eEvent, void *pUserData)
{
}


/*F********************************************************************************/
/*!
    \Function _DigObjApiXmlResponse

    \Description
        Parse XML response.

    \Input *pDigObjApi  - digobj object ref
    \Input *ppXml       - [out] optional storage for pointer to XML DigObj tag

    \Output
        int32_t         - xml eerror - DIGOBJAPI_ERROR_*
        unsigned char * - if no error, pointer to DIGOBJ tag, else NULL

    \Version 05/30/2006 (tchen)
*/
/********************************************************************************F*/
static int32_t _DigObjApiXmlResponse(DigObjApiRefT *pDigObjApi, const unsigned char **ppXml)
{
    const unsigned char *pXml;
    int32_t iError;

    // parse the XML header
    if ((pXml = XmlFind((unsigned char *)pDigObjApi->strXml, (unsigned char *)"DIGOBJ")) == NULL)
    {
        pDigObjApi->eCurError = DIGOBJAPI_ERROR_SERVER;
        return(DIGOBJAPI_ERROR_SERVER);
    }

    // save xml ref for caller?
    if (ppXml)
    {
        *ppXml = pXml;
    }

    // extract query result
    iError = XmlAttribGetInteger(pXml, (unsigned char *)"error", DIGOBJAPI_ERROR_NONE);
    pDigObjApi->eCurError = iError;

    #if DIRTYCODE_LOGGING
    // if an error occurred, output debug info
    if (iError != DIGOBJAPI_ERROR_NONE)
    {
        char strError[128];
        XmlAttribGetString(pXml, (unsigned char *)"message", (unsigned char *)strError, sizeof(strError), (unsigned char *)"");
        NetPrintf(("digobjapi: xml response %d: %s\n", iError, strError));
    }
    #endif
    
    // return xml error code to caller
    return(iError);
}

/*F********************************************************************************/
/*!
    \Function _DigObjApiWaitResponse

    \Description
        Wait for XML response, and return TRUE if the response was successful.

    \Input *pDigObjApi  - DigObj object ref

    \Output
        DigObjApiEventE - DigObj event

    \Version 12/08/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _DigObjApiWaitResponse(DigObjApiRefT *pDigObjApi)
{
    ProtoHttpResponseE eResponse;
    int32_t iStatus;

    // do we have the body size yet?
    if (pDigObjApi->iRecvSize == 0)
    {
        // get size of xml response
        if ((iStatus = ProtoHttpStatus(pDigObjApi->pHttp, 'body', NULL, 0)) > (int32_t)sizeof(pDigObjApi->strXml))
        {
            // response size is too large
            NetPrintf(("digobjapi: server xml response does not fit in xml buffer\n"));
            pDigObjApi->eCurError = DIGOBJAPI_ERROR_CLIENT;
            return(DIGOBJAPI_EVENT_ERROR);
        }
        else if (iStatus > 0)
        {
            pDigObjApi->iRecvSize = iStatus;
        }
    }

    // if we need data, try to read it
    if (pDigObjApi->iRecvOffset < pDigObjApi->iRecvSize)
    {
        if ((iStatus = ProtoHttpRecv(pDigObjApi->pHttp, pDigObjApi->strXml + pDigObjApi->iRecvOffset, 1, pDigObjApi->iRecvSize - pDigObjApi->iRecvOffset)) > 0)
        {
            pDigObjApi->iRecvOffset += iStatus;
        }
    }

    // make sure we're done
    if ((iStatus = ProtoHttpStatus(pDigObjApi->pHttp, 'done', NULL, 0)) == 0)
    {
        return(DIGOBJAPI_EVENT_NONE);
    }

    // get response
    eResponse = PROTOHTTP_GetResponseClass(ProtoHttpStatus(pDigObjApi->pHttp, 'code', NULL, 0));
    if ((iStatus < 0) || (eResponse != PROTOHTTP_RESPONSE_SUCCESSFUL))
    {
        pDigObjApi->eCurError = DIGOBJAPI_ERROR_SERVER;
        return(DIGOBJAPI_EVENT_ERROR);
    }

    // if we have the response, return success
    if (pDigObjApi->iRecvOffset == pDigObjApi->iRecvSize)
    {
        // null-terminate response
        if (pDigObjApi->iRecvSize < DIGOBJ_XMLSIZE)
        {
            pDigObjApi->strXml[pDigObjApi->iRecvSize] = '\0';
        }
        else
        {
            pDigObjApi->strXml[DIGOBJ_XMLSIZE - 1] = '\0';
        }
        
        // determine response success or failure
        if (_DigObjApiXmlResponse(pDigObjApi, NULL) == 0)
        {
            return(DIGOBJAPI_EVENT_DONE);
        }
        else
        {
            return(DIGOBJAPI_EVENT_ERROR);
        }
    }

    return(DIGOBJAPI_EVENT_NONE);
}

/*F********************************************************************************/
/*!
    \Function _DigObjApiParseObj

    \Description
        Parse list of digital object IDs returned by rdm and get commands.

    \Input *pDigObjApi  - locker object ref

    \Output
        int32_t         - zero=success, negative=failure

    \Version 06/01/2006 (tchen)
*/
/********************************************************************************F*/
static int32_t _DigObjApiParseObj(DigObjApiRefT *pDigObjApi)
{
    const unsigned char *pXml = NULL;
    int32_t i;

    // make sure XML returned success
    if (_DigObjApiXmlResponse(pDigObjApi, &pXml) != DIGOBJAPI_ERROR_NONE)
    {
        return(-1);
    }

    // parse attributes
    pDigObjApi->iObjCount = XmlAttribGetInteger(pXml, (unsigned char *)"count", 0);
    
    // find first id entry
    if ((pXml = XmlFind(pXml, (unsigned char *)"DIGOBJ.OBJID")) != NULL)
    {
        // parse id list
        for (i = 0; i < pDigObjApi->iObjCount; i++)
        {
            // parse the id
            XmlAttribGetString(pXml, (unsigned char *)"domain", (unsigned char *)pDigObjApi->aDigitalObjectIDs[i].strDomain, sizeof(pDigObjApi->aDigitalObjectIDs[i].strDomain), (unsigned char *)"");
            XmlAttribGetString(pXml, (unsigned char *)"id", (unsigned char *)pDigObjApi->aDigitalObjectIDs[i].strID, sizeof(pDigObjApi->aDigitalObjectIDs[i].strID), (unsigned char *)"");

            // index to next id
            if ((pXml = XmlNext(pXml)) == NULL)
            {
                break;
            }
        }

        // make sure we got all the files we were supposed to
        if (i != (pDigObjApi->iObjCount-1))
        {
            NetPrintf(("digobjapi: unable to parse all ids in list\n"));
            pDigObjApi->eCurError = DIGOBJAPI_ERROR_CLIENT;
            return(-1);
        }
    }

    // done!
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _DigObjApiUpdateRdm

    \Description
        Update digital object redeem operation.

    \Input *pDigObjApi  - digobj object ref

    \Output
        DigObjApiEventE - digobj event

    \Version 06/01/2006 (tchen)
*/
/********************************************************************************F*/
static DigObjApiEventE _DigObjApiUpdateRdm(DigObjApiRefT *pDigObjApi)
{
    DigObjApiEventE eEvent;

    // wait until we have gotten a response
    if ((eEvent = _DigObjApiWaitResponse(pDigObjApi)) <= DIGOBJAPI_EVENT_NONE)
    {
        return(eEvent);
    }

    _DigObjApiParseObj(pDigObjApi);

    // return success
    return(DIGOBJAPI_EVENT_RDM);
}

/*F********************************************************************************/
/*!
    \Function _DigObjApiUpdateGet

    \Description
        Update digital object get operation.

    \Input *pDigObjApi  - digobj object ref

    \Output
        DigObjApiEventE - digobj event

    \Version 06/01/2006 (tchen)
*/
/********************************************************************************F*/
static DigObjApiEventE _DigObjApiUpdateGet(DigObjApiRefT *pDigObjApi)
{
    DigObjApiEventE eEvent;

    // wait until we have gotten a response
    if ((eEvent = _DigObjApiWaitResponse(pDigObjApi)) <= DIGOBJAPI_EVENT_NONE)
    {
        return(eEvent);
    }

    _DigObjApiParseObj(pDigObjApi);

    // return success
    return(DIGOBJAPI_EVENT_FET);
}

/*F********************************************************************************/
/*!
    \Function _DigObjApiUpdate

    \Description
        Update the DigObjApi module state.

    \Input *pData   - pointer to module state
    \Input uTick    - current millisecond tick

    \Version 06/01/2006 (tchen)
*/
/********************************************************************************F*/
static void _DigObjApiUpdate(void *pData, uint32_t uTick)
{
    DigObjApiRefT *pDigObjApi = (DigObjApiRefT *)pData;
    DigObjApiEventE eEvent = DIGOBJAPI_EVENT_NONE;

    // update if not idle
    if (pDigObjApi->eState != ST_IDLE)
    {
        // give life to http module
        ProtoHttpUpdate(pDigObjApi->pHttp);

        // make sure we haven't timed out
        if (ProtoHttpStatus(pDigObjApi->pHttp, 'time', NULL, 0) == FALSE)
        {
            // operate based on state
            switch(pDigObjApi->eState)
            {
                case ST_RDM:
                    eEvent = _DigObjApiUpdateRdm(pDigObjApi);
                    break;
                case ST_FET:
                    eEvent = _DigObjApiUpdateGet(pDigObjApi);
                    break;
                default:
                    NetPrintf(("digobjapi: unknown state %d\n", pDigObjApi->eState));
                    break;
            }
        }
        else
        {
            #if DIGOBJAPI_VERBOSE
            NetPrintf(("digobjapi: timeout at %d\n", NetTick()));
            #endif
            
            eEvent = DIGOBJAPI_EVENT_ERROR;
            pDigObjApi->eCurError = DIGOBJAPI_ERROR_TIMEOUT;
        }
    }

    // if event isn't none, we're done
    if (eEvent != DIGOBJAPI_EVENT_NONE)
    {
        // trigger user callback
        pDigObjApi->pCallback(pDigObjApi, (DigObjApiEventE)eEvent, pDigObjApi->pUserData);

        // reset state
        pDigObjApi->eState = ST_IDLE;
        pDigObjApi->iRecvOffset = 0;
        pDigObjApi->iRecvSize = 0;

    }

    // update current event status
    pDigObjApi->eCurEvent = eEvent;
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function DigObjApiCreate

    \Description
        Create a digobj object.

    \Input *pServer     - pointer to digobj server name, or NULL to use default
    \Input *pCallback   - pointer to user-supplied event callback
    \Input *pUserData   - pointer to user data to pass to callback

    \Output
        DigObjApiRefT * - pointer to new digobj object, or NULL

    \Version 06/01/2006 (tchen)
*/
/********************************************************************************F*/
DigObjApiRefT *DigObjApiCreate(const char *pServer, DigObjApiCallbackT *pCallback, void *pUserData)
{
    DigObjApiRefT *pDigObjApi;
    int32_t iObjectSize;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // make sure server is specified
    if (pServer == NULL)
    {
        NetPrintf(("digobjapi: can't create module without a server name\n"));
        return(NULL);
    }

    // calculate object size
    iObjectSize =  sizeof(*pDigObjApi);

    // allocate and initialize module state
    if ((pDigObjApi = DirtyMemAlloc(iObjectSize, DIGOBJAPI_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("digobjapi: unable to allocate module state\n"));
        return(NULL);
    }
    memset(pDigObjApi, 0, iObjectSize);
    pDigObjApi->iMemGroup = iMemGroup;
    pDigObjApi->pMemGroupUserData = pMemGroupUserData;
    pDigObjApi->iSpoolSize = DIGOBJAPI_SPOOLSIZE;

    // allocate protohttp module
    if ((pDigObjApi->pHttp = ProtoHttpCreate(pDigObjApi->iSpoolSize)) == NULL)
    {
        NetPrintf(("digobjapi: unable to create protohttp module\n"));
        DirtyMemFree(pDigObjApi, DIGOBJAPI_MEMID, pDigObjApi->iMemGroup, pDigObjApi->pMemGroupUserData);
        return(NULL);
    }

    // set http timeout
    ProtoHttpControl(pDigObjApi->pHttp, 'time', 60*1000, 0, NULL);

    // set user callback
    pDigObjApi->pCallback = (pCallback == NULL) ? _DigObjApiDefaultCallback : pCallback;
    pDigObjApi->pUserData = pUserData;

    // set server
    strnzcpy(pDigObjApi->strServer, pServer, sizeof(pDigObjApi->strServer));

    // install idle callback
    NetConnIdleAdd(_DigObjApiUpdate, pDigObjApi);

    // return state to caller
    return(pDigObjApi);
}

/*F********************************************************************************/
/*!
    \Function DigObjApiDestroy

    \Description
        Destroy a digobj object.

    \Input *pDigObjApi  - digobj object ref

    \Version 06/01/2006 (tchen)
*/
/********************************************************************************F*/
void DigObjApiDestroy(DigObjApiRefT *pDigObjApi)
{
    // remove idle callback
    NetConnIdleDel(_DigObjApiUpdate, pDigObjApi);

    // destroy ProtoHttp module
    ProtoHttpDestroy(pDigObjApi->pHttp);

    // release module memory
    DirtyMemFree(pDigObjApi, DIGOBJAPI_MEMID, pDigObjApi->iMemGroup, pDigObjApi->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function DigObjApiSetLoginInfo

    \Description
        Set login information

    \Input *pDigObjApi  - digobj object ref
    \Input *pLKey       - lkey to set
    \Input *pPersona    - persona lkey is associated with

    \Version 11/24/2004 (jbrookes)
*/
/********************************************************************************F*/
void DigObjApiSetLoginInfo(DigObjApiRefT *pDigObjApi, const char *pLKey, const char *pPersona)
{
    strnzcpy(pDigObjApi->strLKey, pLKey, sizeof(pDigObjApi->strLKey));
    strnzcpy(pDigObjApi->strPersona, pPersona, sizeof(pDigObjApi->strPersona));
}

/*F********************************************************************************/
/*!
    \Function DigObjApiRedeemCode

    \Description
        Get a user's directory (async).

    \Input *pDigObjApi  - digobj object ref
    \Input *pRegCode    - digobj reg code

    \Version 06/01/2006 (tchen)
*/
/********************************************************************************F*/
void DigObjApiRedeemCode(DigObjApiRefT *pDigObjApi, const char *pRegCode)
{
    char strUrl[256];

    // make sure we're idle
    if (pDigObjApi->eState != ST_IDLE)
    {
        NetPrintf(("digobjapi: can't issue digobj redeem while not idle\n"));
        return;
    }

    // clear error state
    pDigObjApi->eCurError = DIGOBJAPI_ERROR_NONE;

    // build url to fetch digobj for given user
    _DigObjApiFormatBaseUrl(pDigObjApi, strUrl, sizeof(strUrl), "rdm");
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&code=", pRegCode);
    
    // issue the request and put us in proper state
    if (ProtoHttpGet(pDigObjApi->pHttp, strUrl, PROTOHTTP_HEADBODY) < 0)
    {
        NetPrintf(("digobjapi: error initiating digobj redeem\n"));
        return;
    }
    pDigObjApi->eState = ST_RDM;
}

/*F********************************************************************************/
/*!
    \Function DigObjApiFetchObjectListForUser

    \Description
        Get a user's directory (async).

    \Input *pDigObjApi  - digobj object ref
    \Input *pOwner      - digobj owner's persona

    \Version 06/01/2006 (tchen)
*/
/********************************************************************************F*/
void DigObjApiFetchObjectListForUser(DigObjApiRefT *pDigObjApi, const char *pOwner)
{
    char strUrl[256];

    // make sure we're idle
    if (pDigObjApi->eState != ST_IDLE)
    {
        NetPrintf(("digobjapi: can't issue digobj get while not idle\n"));
        return;
    }

    // clear error state
    pDigObjApi->eCurError = DIGOBJAPI_ERROR_NONE;

    // build url to fetch digobj for given user
    _DigObjApiFormatBaseUrl(pDigObjApi, strUrl, sizeof(strUrl), "get");
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&pers=", pOwner);

    // issue the request and put us in proper state
    if (ProtoHttpGet(pDigObjApi->pHttp, strUrl, PROTOHTTP_HEADBODY) < 0)
    {
        NetPrintf(("digobjapi: error initiating digobj get\n"));
        return;
    }
    pDigObjApi->eState = ST_FET;
}

/*F********************************************************************************/
/*!
    \Function DigObjApiGetObjectCount

    \Description
        Get currently count of objects

    \Input *pDigObjApi  - digobj object ref

    \Version 06/14/2006 (tchen)
*/
/********************************************************************************F*/
int32_t DigObjApiGetObjectCount(DigObjApiRefT *pDigObjApi)
{
    return(pDigObjApi->iObjCount);
}

/*F********************************************************************************/
/*!
    \Function DigObjApiGetObjectAtIndex

    \Description
        Get a digital object from the array

    \Input *pDigObjApi  - digobj object ref
    \Input uIdx         - index of the item in the array

    \Version 06/01/2006 (tchen)
*/
/********************************************************************************F*/
const DigObjApiIdT* DigObjApiGetObjectAtIndex(DigObjApiRefT *pDigObjApi, uint32_t uIdx)
{
    if ( uIdx+1 > (uint32_t)pDigObjApi->iObjCount)
    {
        return(NULL);
    }

    return(&(pDigObjApi->aDigitalObjectIDs[uIdx]));
}

/*F********************************************************************************/
/*!
    \Function DigObjApiGetLastEvent

    \Description
        Get most recent event (useful if not using callbacks).

    \Input *pDigObjApi  - digobj object ref

    \Output
        DigObjApiEventE - most recently occurring event

    \Version 11/24/2004 (jbrookes)
*/
/********************************************************************************F*/
DigObjApiEventE DigObjApiGetLastEvent(DigObjApiRefT *pDigObjApi)
{
    return(pDigObjApi->eCurEvent);
}

/*F********************************************************************************/
/*!
    \Function DigObjApiGetLastError

    \Description
        Get most recent error.

    \Input *pDigObjApi  - digobj object ref

    \Output
        DigObjApiErrorE - most recently occurring error

    \Version 11/24/2004 (jbrookes)
*/
/********************************************************************************F*/
DigObjApiErrorE DigObjApiGetLastError(DigObjApiRefT *pDigObjApi)
{
    return(pDigObjApi->eCurError);
}

