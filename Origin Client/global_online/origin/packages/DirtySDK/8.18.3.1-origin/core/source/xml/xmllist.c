/*H********************************************************************************/
/*!
    \File xmllist.c

    \Description
        The XmlList API is used to retrieve and parse XML data in the ItemML
        format.  It provides the ability to retrieve the data from web server
        URL.  The iteration-oriented API allows the caller to walk through the
        returned items and properties to access the data.

    \Copyright
        Copyright (c) Electronic Arts 2006. ALL RIGHTS RESERVED.

    \Version 1.0 12/05/2006 (kcarpenter) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dirtysock.h"
#include "dirtymem.h"
#include "protohttp.h"
#include "protofilter.h"
#include "netconn.h"
#include "xmlparse.h"

#include "xmllist.h"

/*** Defines **********************************************************************/

//! Verbose debugging
#define XMLLIST_VERBOSE             (DIRTYCODE_DEBUG && FALSE)

//! Local definitions
#define XMLLIST_FIRST_ITEM_PATH     (unsigned char *)"items.item"
#define XMLLIST_ITEMS_PATH          (unsigned char *)"items"
#define XMLLIST_PROP_PATH           (unsigned char *)".prop"
#define XMLLIST_MAX_INT_STR_LENGTH	30

/*** Macros ***********************************************************************/
#ifndef min
    #define max(a,b)    (((a) > (b)) ? (a) : (b))
    #define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

/*** Type Definitions *************************************************************/
struct XmlListT
{
    ProtoHttpRefT           *pHttp;             //!< HTTP module used for web transactions

    uint8_t                  strUrl[XMLLIST_MAX_URL_LENGTH];            //!< URL from which to retrieve data
    uint8_t                  strLKey[XMLLIST_MAX_LKEY_LENGTH];          //!< User's LKey, required for web transactions
    uint8_t                  strPersona[XMLLIST_MAX_PERSONA_LENGTH];    //!< User's persona name

    XmlListCallbackT        *pCallback;         //!< User event callback
    void                    *pUserData;         //!< Data for user event callback

    uint32_t                 iMaxXmlLength;     //!< Maximum size of XML data that can be read

    // module memory group
    int32_t                 iMemGroup;          //!< module mem group id
    void                    *pMemGroupUserData; //!< user data associated with mem group

    uint8_t                 *pStrXml;           //!< XML data buffer to read into
    const uint8_t           *pStrXmlCurrItem;   //!< Last successfully found item
    const uint8_t           *pStrXmlCurrProp;   //!< Last successfully found property

    XmlListEventE            eLastEvent;        //!< Last event that occurred
    XmlListErrorE            eLastError;        //!< Last error that occurred
	int32_t                  iServerError;		//!< Server error received in the XML (zero == success)

    uint8_t                  bItemFound;        //!< TRUE if we have found an item
    uint8_t                  bPropFound;        //!< TRUE if we have found a property

    enum
    {
        ST_IDLE,                //!< No operation in progress
        ST_GET,                 //!< An HTTP GET operation is in progress
    } eState;
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _XmlListReset

    \Description
        Reset internal state to defaults.

    \Input *pXmlList - XML List object

    \Version 12/04/2006 (kcarpenter)
*/
/********************************************************************************F*/
static void _XmlListReset(XmlListT *pXmlList)
{
    // Reset URL and login information
    pXmlList->strUrl[0] = '\0';
    pXmlList->strLKey[0] = '\0';
    pXmlList->strPersona[0] = '\0';

    // Reset XML parsing
    pXmlList->pStrXml[0] = '\0';
    pXmlList->pStrXmlCurrItem = pXmlList->pStrXml;
    pXmlList->pStrXmlCurrProp = pXmlList->pStrXml;
    pXmlList->bItemFound = FALSE;
    pXmlList->bPropFound = FALSE;

    // Reset event and error info
    pXmlList->eLastEvent = XMLLIST_EVENT_NONE;
    pXmlList->eLastError = XMLLIST_ERROR_OK;
    pXmlList->eState     = ST_IDLE;
}

/*F********************************************************************************/
/*!
    \Function _XmlListFindFirst

    \Description
    \verbatim
        Find the first <item> tag and <prop> tags
    \endverbatim

    \Input *pXmlList    - XML List object

    \Output
        XmlListEventE   - XML List event

    \Version 12/13/2006 (kcarpenter)
*/
/********************************************************************************F*/
static XmlListEventE _XmlListFindFirst(XmlListT *pXmlList)
{
    int32_t iResultSize;
    const uint8_t *pXml;
    char strBuffer[XMLLIST_MAX_INT_STR_LENGTH];

    // Assume neither will be found
    pXmlList->bItemFound = FALSE;
    pXmlList->bPropFound = FALSE;

    // Find the main body tag: <items>
    if ((pXml = XmlFind(pXmlList->pStrXml, XMLLIST_ITEMS_PATH)) != NULL)
    {
        // See if the "err" attribute is present -- if not assume success
        iResultSize = XmlAttribGetString(pXml, (unsigned char *)XMLLIST_ERROR_ATTRIB, (unsigned char *)strBuffer, XMLLIST_MAX_INT_STR_LENGTH, NULL);
        if (iResultSize == -1)
        {
            // Zero is success
            pXmlList->iServerError = 0;
        }

        pXmlList->iServerError = atoi(strBuffer);
    }

    // Find the first item
    if ((pXml = XmlFind(pXmlList->pStrXml, XMLLIST_FIRST_ITEM_PATH)) == NULL)
    {
        // Unable to find the main body tag
        pXmlList->eLastError = XMLLIST_ERROR_PARSER;
        pXmlList->eLastEvent = XMLLIST_EVENT_REQ_COMPLETE_ERROR;
        return(XMLLIST_EVENT_REQ_COMPLETE_ERROR);
    }

    // Found the first item, so save it
    pXmlList->pStrXmlCurrItem = pXml;
    pXmlList->bItemFound = TRUE;

    // Find the first item's first property
    if ((pXml = XmlFind(pXmlList->pStrXmlCurrItem, XMLLIST_PROP_PATH)) == NULL)
    {
        // Unable to find the main body tag
        pXmlList->eLastError = XMLLIST_ERROR_PARSER;
        pXmlList->eLastEvent = XMLLIST_EVENT_REQ_COMPLETE_ERROR;
        return(XMLLIST_EVENT_REQ_COMPLETE_ERROR);
    }

    // Found the first property, so save it
    pXmlList->pStrXmlCurrProp = pXml;
    pXmlList->bPropFound = TRUE;

    pXmlList->eLastError = XMLLIST_ERROR_OK;
    pXmlList->eLastEvent = XMLLIST_EVENT_REQ_COMPLETE_OK;
    return(XMLLIST_EVENT_REQ_COMPLETE_OK);
}

/*F********************************************************************************/
/*!
    \Function _XmlListUpdateGet

    \Description
        Update HTTP get operation.

    \Input *pXmlList - XML List object
    
    \Output
        XmlListEventE - XML List event

    \Version 03/21/2007 (jbrookes)
*/
/********************************************************************************F*/
static XmlListEventE _XmlListUpdateGet(XmlListT *pXmlList)
{
    ProtoHttpResponseE eResponse;
    XmlListEventE eEvent = XMLLIST_EVENT_NONE;
    int32_t iRecvResult;
    
    // Update the HTTP module
    ProtoHttpUpdate(pXmlList->pHttp);

    // Read all data
    if ((iRecvResult = ProtoHttpRecvAll(pXmlList->pHttp, (char *)pXmlList->pStrXml, pXmlList->iMaxXmlLength)) >= 0)
    {
        NetPrintf(("xmllist: recv %d bytes at time %d\n", iRecvResult, NetTick()));
        if ((eResponse = ProtoHttpStatus(pXmlList->pHttp, 'code', NULL, 0)) != PROTOHTTP_RESPONSE_OK)
        {
            switch(eResponse)
            {
                case PROTOHTTP_RESPONSE_UNAUTHORIZED:
                    pXmlList->eLastError = XMLLIST_ERROR_UNAUTHORIZED;
                    break;
                case PROTOHTTP_RESPONSE_NOTFOUND:
                    pXmlList->eLastError = XMLLIST_ERROR_NOT_FOUND;
                    break;
                default:
                    pXmlList->eLastError = XMLLIST_ERROR_SERVER;
                    break;
            }

            eEvent = XMLLIST_EVENT_ERROR;
        }
        else
        {
            eEvent = _XmlListFindFirst(pXmlList);
        }

        pXmlList->eState = ST_IDLE;
    }
    else if ((iRecvResult < 0) && (iRecvResult != PROTOHTTP_RECVWAIT))
    {
        if (ProtoHttpStatus(pXmlList->pHttp, 'time', NULL, 0) == TRUE)
        {
            #if XMLLIST_VERBOSE
            NetPrintf(("xmllist: timeout at %d\n", NetTick()));
            #endif
            pXmlList->eLastError = XMLLIST_ERROR_TIMEOUT;
        }
        else
        {
            pXmlList->eLastError = XMLLIST_ERROR_SERVER;
        }
        
        eEvent = XMLLIST_EVENT_ERROR;
        pXmlList->eState = ST_IDLE;
    }

    return(eEvent);
}

/*F********************************************************************************/
/*!
    \Function _XmlListUpdate

    \Description
        Update the XmlList module state.

    \Input *pData   - pointer to module state
    \Input uTick    - current millisecond tick

    \Version 12/04/2006 (kcarpenter)
*/
/********************************************************************************F*/
static void _XmlListUpdate(void *pData, uint32_t uTick)
{
    XmlListT *pXmlList = (XmlListT *)pData;
    XmlListEventE eEvent = XMLLIST_EVENT_NONE;
    
    // Update if not idle
    if (pXmlList->eState == ST_GET)
    {
        eEvent = _XmlListUpdateGet(pXmlList);
    }

    // See if any event occured and if so, pass it on via the callback
    if (eEvent != XMLLIST_EVENT_NONE)
    {
        // Update current event status
        pXmlList->eLastEvent = eEvent;

        // Trigger user callback, if provided
        if (pXmlList->pCallback != NULL)
        {
            pXmlList->pCallback(pXmlList, eEvent, 0, pXmlList->pUserData);
        }
    }
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function XmlListCreate

    \Description
       Create a new XmlListT object with the given parameters.

    \Input iMaxXmlLength - Maximum size of XML file that can be received.
    \Input *pCallback    - User callback to call when events occur. 
    \Input pUserData     - User-specified data that will be passed into the callback.
    
    \Output
        XmlListT*          - A new XmlListT object

    \Version 1.0 11/23/2006 (kcarpenter) First Version
*/
/********************************************************************************F*/
XmlListT *XmlListCreate(uint32_t iMaxXmlLength, XmlListCallbackT *pCallback, void *pUserData)
{
    XmlListT *pXmlList = NULL;
    int32_t iObjectSize = sizeof(*pXmlList);
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // Allocate and initialize module state
    if ((pXmlList = DirtyMemAlloc(iObjectSize, XMLLIST_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("xmllist: unable to allocate module state\n"));
        return(NULL);
    }
    memset(pXmlList, 0, iObjectSize);
    pXmlList->iMemGroup = iMemGroup;
    pXmlList->pMemGroupUserData = pMemGroupUserData;

    // Allocate memory for XML data based on size requested by the caller
    if ((pXmlList->pStrXml = DirtyMemAlloc(iMaxXmlLength, XMLLIST_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("xmllist: unable to allocate XML buffer\n"));
        DirtyMemFree(pXmlList, XMLLIST_MEMID, pXmlList->iMemGroup, pXmlList->pMemGroupUserData);
        return(NULL);
    }

    // Copy configuration parameters
    pXmlList->iMaxXmlLength = iMaxXmlLength;
    pXmlList->pCallback     = pCallback;
    pXmlList->pUserData     = pUserData;

    // Allocate protohttp module
    if ((pXmlList->pHttp = ProtoHttpCreate(pXmlList->iMaxXmlLength)) == NULL)
    {
        NetPrintf(("xmllist: unable to create protohttp module\n"));
        DirtyMemFree(pXmlList->pStrXml, XMLLIST_MEMID, pXmlList->iMemGroup, pXmlList->pMemGroupUserData);
        DirtyMemFree(pXmlList, XMLLIST_MEMID, pXmlList->iMemGroup, pXmlList->pMemGroupUserData);
        return(NULL);
    }

    // Set HTTP timeout
    ProtoHttpControl(pXmlList->pHttp, 'time', 60*1000, 0, NULL);
 
    // Reset the rest of the structure's fields to defaults
    _XmlListReset(pXmlList);

    // Install idle callback
    NetConnIdleAdd(_XmlListUpdate, pXmlList);

    // Return state to caller
    return(pXmlList);
}

///*F********************************************************************************/
/*!
    \Function XmlListDestroy

    \Description
       Destroy a previously created XmlListT.  Release all associated resources. 

    \Input *pXmlList - XML List object

    \Version 11/23/2006 (kcarpenter)
*/
/********************************************************************************F*/
void XmlListDestroy(XmlListT *pXmlList)
{
    // Remove idle callback
    NetConnIdleDel(_XmlListUpdate, pXmlList);

    // Destroy ProtoHttp module
    ProtoHttpDestroy(pXmlList->pHttp);

    // Release XML string memory
    DirtyMemFree(pXmlList->pStrXml, XMLLIST_MEMID, pXmlList->iMemGroup, pXmlList->pMemGroupUserData);

    // Release module memory
    DirtyMemFree(pXmlList, XMLLIST_MEMID, pXmlList->iMemGroup, pXmlList->pMemGroupUserData);
}

///*F********************************************************************************/
/*!
    \Function XmlListReset

    \Description
       Reset the XmlList state to the defaults. 

    \Input *pXmlList - XML List object

    \Version 12/04/2006 (kcarpenter)
*/
/********************************************************************************F*/
void XmlListReset(XmlListT *pXmlList)
{
    _XmlListReset(pXmlList);
}

/*F********************************************************************************/
/*!
    \Function XmlListFetch

    \Description
       Initiate an HTTP request to the web server to read the given URL (async).
       When the complete result has been receivd, the caller's XmlListCallback
       will be called (it will also be called if the operation fails or is
       cancelled.  Alternatively, you may poll for completion by calling
       XmlListGetLastEvent().

    \Input *pXmlList - XML List object
    \Input *pStrUrl  - URL to issue the request to

    \Version 12/04/2006 (kcarpenter)
*/
/********************************************************************************F*/
void XmlListFetch(XmlListT *pXmlList, const uint8_t *pStrUrl)
{
	char* pSiteStr;

	// See if there are any parameters in the string
	if (strchr((char *)pStrUrl, '?') == 0)
	{
		// No parameters on the URL -- append the site parameter with a "?" character
	    pSiteStr = "?site=easo";
	}
	else
	{
		// At least one parameter on the URL -- append the site parameter with an "&" character
	    pSiteStr = "&site=easo";
	}

	// Format the lkey onto the end if given
    if (pXmlList->strLKey[0] != 0)
    {
		// Include the lkey
        snzprintf((char *)pXmlList->strUrl, XMLLIST_MAX_URL_LENGTH, "%s%s&lkey=%s", pStrUrl, pSiteStr, pXmlList->strLKey);
    }
    else
    {
		// Do not include the lkey
        snzprintf((char *)pXmlList->strUrl, XMLLIST_MAX_URL_LENGTH, "%s%s", pStrUrl, pSiteStr);
    }

    // Clear out the XML memory before each fetch, including the first time
    memset(pXmlList->pStrXml, 0, pXmlList->iMaxXmlLength);

	// Reset the server error
	pXmlList->iServerError = 0;

	// Send the GET request 
    ProtoHttpGet(pXmlList->pHttp, (char *)pXmlList->strUrl, FALSE);

    // Transition to the GET state
    pXmlList->eState = ST_GET;

    pXmlList->eLastError = XMLLIST_ERROR_OK;
    pXmlList->eLastEvent = XMLLIST_EVENT_REQ_SENT;

    pXmlList->bItemFound = FALSE;
    pXmlList->bPropFound = FALSE;
}

/*F********************************************************************************/
/*!
    \Function XmlListSetLoginInfo

    \Description
        Set login information

    \Input *pXmlList - XML List object
    \Input *pLKey    - lkey to set
    \Input *pPersona - persona lkey is associated with

    \Version 12/04/2006 (kcarpenter)
*/
/********************************************************************************F*/
void XmlListSetLoginInfo(XmlListT *pXmlList, const char *pLKey, const char *pPersona)
{
    if (pLKey)
    {
        strnzcpy((char *)pXmlList->strLKey, pLKey, sizeof(pXmlList->strLKey));
    }

    if (pPersona)
    {
        strnzcpy((char *)pXmlList->strPersona, pPersona, sizeof(pXmlList->strPersona));
    }
}

/*F********************************************************************************/
/*!
    \Function XmlListAbortFetch

    \Description
        Abort current Fetch operation, if any.

    \Input *pXmlList  - XML List object

    \Version 12/04/2006 (kcarpenter)
*/
/********************************************************************************F*/
void XmlListAbortFetch(XmlListT *pXmlList)
{
    // Abort current transaction, if any
    ProtoHttpAbort(pXmlList->pHttp);

    // Reset state
    _XmlListReset(pXmlList);

    pXmlList->eLastEvent = XMLLIST_EVENT_REQ_ABORTED;
}

/*F********************************************************************************/
/*!
    \Function XmlListGetLastEvent

    \Description
        Get most recent event (useful if not using callbacks).

    \Input *pXmlList  - XML List object
    
    \Output
        XmlListEventE - XML List event

    \Version 1.0 12/04/2006 (kcarpenter) First Version
*/
/********************************************************************************F*/
XmlListEventE XmlListGetLastEvent(XmlListT *pXmlList)
{
    return(pXmlList->eLastEvent);
}

/*F********************************************************************************/
/*!
    \Function XmlListGetLastError

    \Description
        Get most recent error.

    \Input *pXmlList  - XML List object
    
    \Output
        XmlListErrorE - most recently occurring error

    \Version 1.0 12/04/2006 (kcarpenter) First Version
*/
/********************************************************************************F*/
XmlListErrorE XmlListGetLastError(XmlListT *pXmlList)
{
    return(pXmlList->eLastError);
}

/*F********************************************************************************/
/*!
    \Function XmlListGetLastError

    \Description
    \verbatim
        Get the last error code reported by the web server.
        This is the value of the "err" attribute in the <items> tag.
        NOTE: This method should only be called after a completed call
              to XmlListFetch().
    \endverbatim

    \Input *pXmlList  - XML List object

    \Output
        XmlListErrorE - most recently occurring error

    \Version 1.0 12/04/2006 (kcarpenter) First Version
*/
/********************************************************************************F*/
int32_t XmlListGetServerError(XmlListT *pXmlList)
{
    return(pXmlList->iServerError);
}

/*F********************************************************************************/
/*!
    \Function XmlListRewind

    \Description
        Return the parser to the beginning of the XML data.  After this call,
        the parser will be positioned at the first item and the first property.

    \Input *pXmlList  - XML List object

    \Version 12/04/2006 (kcarpenter)
*/
/********************************************************************************F*/
void XmlListRewind(XmlListT *pXmlList)
{
    pXmlList->pStrXmlCurrItem = pXmlList->pStrXml;
    pXmlList->pStrXmlCurrProp = pXmlList->pStrXml;

    _XmlListFindFirst(pXmlList);
}

/*F********************************************************************************/
/*!
    \Function XmlListNextItem

    \Description
    \verbatim
        Advance to the next <item> tag.  If found, then return TRUE, else FALSE.
        Call XmlListGetLastError() for a more detailed error code.  
    \endverbatim

    \Input *pXmlList  - XML List object
    
    \Output
        int32_t  - TRUE if found, FALSE if not

    \Version 1.0 12/04/2006 (kcarpenter) First Version
*/
/********************************************************************************F*/
int32_t XmlListNextItem(XmlListT *pXmlList)
{
    const uint8_t *pXml;
    if ((pXml = XmlNext(pXmlList->pStrXmlCurrItem)) == NULL)
    {
        pXmlList->eLastError = XMLLIST_ERROR_PARSER;
        return(FALSE);
    }

    // Update current location for next search
    pXmlList->pStrXmlCurrItem = pXml;
    pXmlList->bItemFound = TRUE;

    // Find the next item
    if ((pXml = XmlFind(pXmlList->pStrXmlCurrItem, XMLLIST_PROP_PATH)) == NULL)
    {
        // Unable to find the main body tag
        pXmlList->eLastError = XMLLIST_ERROR_PARSER;
        return(FALSE);
    }

    // Found the next item
    pXmlList->pStrXmlCurrProp = pXml;
    pXmlList->bPropFound = TRUE;

    pXmlList->eLastError = XMLLIST_ERROR_OK;
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function XmlListNextProperty

    \Description
    \verbatim
        Advance to the next <prop> tag.  If found, then return TRUE, else FALSE.
        Call XmlListGetLastError() for a more detailed error code.  
    \endverbatim

    \Input *pXmlList  - XML List object
    
    \Output
        int32_t  - TRUE if found, FALSE if not

    \Version 1.0 12/04/2006 (kcarpenter) First Version
*/
/********************************************************************************F*/
int32_t XmlListNextProperty(XmlListT *pXmlList)
{
    const uint8_t *pXml;

    // Find the next property
    if ((pXml = XmlNext(pXmlList->pStrXmlCurrProp)) == NULL)
    {
        pXmlList->eLastError = XMLLIST_ERROR_PARSER;
        return(FALSE);
    }

    // Found the next property
    pXmlList->pStrXmlCurrProp = pXml;
    pXmlList->bPropFound = TRUE;

    pXmlList->eLastError = XMLLIST_ERROR_OK;
    return(TRUE);
}

//*F********************************************************************************/
/*!
    \Function XmlListGetItemAttrib

    \Description
    \verbatim
        Get the named attribute from the current <item> tag.
    \endverbatim

    \Input *pXmlList    - XML List object
    \Input *pAttribName - Attribute name
    \Input *pBuffer     - Output buffer for attribute value
    \Input  iLength     - Length of pBuffer

    \Output
        int32_t  - TRUE if found, FALSE if not

    \Version 1.0 12/04/2006 (kcarpenter) First Version
*/
/********************************************************************************F*/
int32_t XmlListGetItemAttrib(XmlListT *pXmlList, uint8_t *pAttribName, uint8_t *pBuffer, int32_t iLength)
{
    int32_t iResultSize;

    if (!pXmlList->bItemFound)
    {
        pXmlList->eLastError = XMLLIST_ERROR_PARSER;
        return(FALSE);
    }

    iResultSize = XmlAttribGetString(pXmlList->pStrXmlCurrItem, pAttribName, pBuffer, iLength, NULL);
    if (iResultSize == -1)
    {
        pXmlList->eLastError = XMLLIST_ERROR_PARSER;
        return(FALSE);
    }

    pXmlList->eLastError = XMLLIST_ERROR_OK;
    return(TRUE);
}


/*F********************************************************************************/
/*!
    \Function XmlListGetPropertyName

    \Description
    \verbatim
        Get the "name" attribute of the current <prop> tag.
    \endverbatim

    \Input *pXmlList - XML List object
    \Input *pBuffer  - Output buffer for attribute value
    \Input  iLength  - Length of pBuffer
    
    \Output
        TRUE if found, FALSE if not

    \Version 1.0 12/04/2006 (kcarpenter) First Version
*/
/********************************************************************************F*/
int32_t XmlListGetPropertyName(XmlListT *pXmlList, uint8_t *pBuffer, int32_t iLength)
{
    int32_t iResultSize;

    if (!pXmlList->bPropFound)
    {
        pXmlList->eLastError = XMLLIST_ERROR_PARSER;
        return(FALSE);
    }

    iResultSize = XmlAttribGetString(pXmlList->pStrXmlCurrProp, (unsigned char *)XMLLIST_NAME_ATTRIB, pBuffer, iLength, NULL);
    if (iResultSize == -1)
    {
        pXmlList->eLastError = XMLLIST_ERROR_PARSER;
        return(FALSE);
    }

    pXmlList->eLastError = XMLLIST_ERROR_OK;
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function XmlListGetPropertyAttrib

    \Description
    \verbatim
        Get the named attribute from the current <prop> tag.
    \endverbatim

    \Input *pXmlList    - XML List object
    \Input *pAttribName - Attribute name
    \Input *pBuffer     - Output buffer for attribute value
    \Input  iLength     - Length of pBuffer

    \Output
        TRUE if found, FALSE if not

    \Version 1.0 12/04/2006 (kcarpenter) First Version
*/
/********************************************************************************F*/
int32_t XmlListGetPropertyAttrib(XmlListT *pXmlList, uint8_t *pAttribName, uint8_t *pBuffer, int32_t iLength)
{
    int32_t iResultSize;

    if (!pXmlList->bPropFound)
    {
        pXmlList->eLastError = XMLLIST_ERROR_PARSER;
        return(FALSE);
    }

    iResultSize = XmlAttribGetString(pXmlList->pStrXmlCurrProp, pAttribName, pBuffer, iLength, NULL);
    if (iResultSize == -1)
    {
        pXmlList->eLastError = XMLLIST_ERROR_PARSER;
        return(FALSE);
    }

    pXmlList->eLastError = XMLLIST_ERROR_OK;
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function XmlListGetPropertyContent

    \Description
    \verbatim
        Get the content data contained in the current <prop> tag.
    \endverbatim

    \Input *pXmlList - XML List object
    \Input *pBuffer  - Output buffer for content value
    \Input  iLength  - Length of pBuffer

    \Output
        TRUE if found, FALSE if not

    \Version 1.0 12/04/2006 (kcarpenter) First Version
*/
/********************************************************************************F*/
int32_t XmlListGetPropertyContent(XmlListT *pXmlList, uint8_t *pBuffer, int32_t iLength)
{
    int32_t iResultSize;

    if (!pXmlList->bPropFound)
    {
        pXmlList->eLastError = XMLLIST_ERROR_PARSER;
        return(FALSE);
    }

    iResultSize = XmlContentGetString(pXmlList->pStrXmlCurrProp, pBuffer, iLength, NULL);
    if (iResultSize == -1)
    {
        pXmlList->eLastError = XMLLIST_ERROR_PARSER;
        return(FALSE);
    }

    pXmlList->eLastError = XMLLIST_ERROR_OK;
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function XmlListGetRawXml

    \Description
        Return a pointer to the internal XML buffer.

    \Input *pXmlList   - XML List object

    \Output
        uint8_t     - Pointer to the raw internal XML string

    \Version 1.0 12/04/2006 (kcarpenter) First Version
*/
/********************************************************************************F*/
uint8_t *XmlListGetRawXml(XmlListT *pXmlList)
{
    return(pXmlList->pStrXml);
}
