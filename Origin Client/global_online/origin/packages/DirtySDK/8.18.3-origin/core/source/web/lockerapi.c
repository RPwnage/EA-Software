/*H********************************************************************************/
/*!
    \File lockerapi.c

    \Description
        The Locker API manages a server-based storage location similar in concept
        to a memory card.  Files may be uploaded to and downloaded from this storage
        location.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 11/24/2004 (jbrookes) first version
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
#include "lockerapi.h"

/*** Defines **********************************************************************/

//! verbose debugging
#define LOCKERAPI_VERBOSE   (DIRTYCODE_DEBUG && FALSE)
#define LOCKERAPI_PRINT_XML (FALSE)

// defines for platform and platform mask
#if defined(DIRTYCODE_PC)
#define LOCKERAPI_FILEATTR_PLATFORM     (LOCKERAPI_FILEATTR_PC)
#elif defined(DIRTYCODE_XENON)
#define LOCKERAPI_FILEATTR_PLATFORM     (LOCKERAPI_FILEATTR_XENON)
#elif defined(DIRTYCODE_REVOLUTION)
#define LOCKERAPI_FILEATTR_PLATFORM     (LOCKERAPI_FILEATTR_REV)
#else
#define LOCKERAPI_FILEATTR_PLATFORM     (0)
#endif

#define LOCKERAPI_FILEATTR_PLATMASK     (0xffff < 16)

#define LOCKERAPI_XMLSIZE      (28*1024)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

struct LockerApiRefT
{
    ProtoHttpRefT       *pHttp;                 //!< http module used for web transactions
    ProtoFilterRefT     *pFilter;               //!< used for profanity filtering of description strings.
    
    // module memory group
    int32_t             iMemGroup;              //!< module mem group id
    void                *pMemGroupUserData;     //!< user data associated with mem group
    
    char                strBaseUrl[256];        //!< server name
    char                strLKey[128];           //!< storage for user's LKey, required for web transactions
    char                strPersona[32];         //!< storage for user's persona name
    char                strCurFileOwner[32];    //!< owner of current file
    LockerApiDirectoryT CurrentDir;             //!< current directory
    LockerApiDirectoryT CurrentSearchReult;     //!< current search result

    LockerApiFileT      CurrentFile;            //!< current file being operated on
    int32_t             iCurFileId;             //!< id of current file
    int32_t             iCurFileAttr;           //!< attributes of current file
    int32_t             bCurFileOverwrite;      //!< the overwrite flag for the current file operation (32-bit for byte alignment)

    uint32_t            uFilterType;            //!< filter type to use
    uint32_t            uFilterReqId;           //!< request id for current ProtoFilter request    

    LockerApiCallbackT  *pCallback;             //!< user event callback
    void                *pUserData;             //!< data for user event callback

    int32_t             iSpoolSize;             //!< size of spool buffer

    int32_t             iSendOffset;            //!< send transfer offset
    int32_t             iSendSize;              //!< amount of data being sent
    int32_t             iRecvOffset;            //!< recv transfer offset
    int32_t             iRecvSize;              //!< amount of data being received

    LockerApiCategoryE  eSortCat;               //!< sort category (used by sort callback)
    int32_t             iSortDir;               //!< sort direction (used by sort callback)

    LockerApiEventE     eCurEvent;              //!< current event status
    LockerApiErrorE     eCurError;              //!< current error status

    char                strGameName[LOCKERAPI_FILEGAME_MAXLEN]; //!< store the game name so we can send it to the server 

    enum
    {
        ST_IDLE,                                //!< nothing going on
        ST_DIR,                                 //!< directory fetch operation in progress
        ST_NFO,                                 //!< file info update operation in progress
        ST_NFO_FILTER,                          //!< file info update operation pending response from ProtoFilter
        ST_GET,                                 //!< get operation in progress
        ST_PUT,                                 //!< put operation in progress
        ST_PUT_FILTER,                          //!< put operation pending response from ProtoFilter
        ST_DEL,                                 //!< delete operation in progress
        ST_SEARCH_META                          //!< Search through meta data operation in progress
    } eState;                                   //!< module state

    char strXml[LOCKERAPI_XMLSIZE];             //!< buffer for receiving XML responses
    char strMeta[LOCKERAPI_MAX_META_SIZE];      //!< buffer to store meta data 
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _LockerApiCalcStorage

    \Description
        Calculate and update total storage in current directory.

    \Input *pDirectory  - pointer to directory to update storage for

    \Version 1.0 12/14/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static void _LockerApiCalcStorage(LockerApiDirectoryT *pDirectory)
{
    int32_t iFileId, iStorage;
    
    for (iFileId = 0, iStorage = 0; iFileId < pDirectory->iNumFiles; iFileId++)
    {
        iStorage += pDirectory->Files[iFileId].Info.iLocSize;
    }
    
    pDirectory->iNumBytes = iStorage;
}

/*F********************************************************************************/
/*!
    \Function _LockerApiFormatBaseUrl

    \Description
        Format string with base url.

    \Input *pLockerApi  - locker object ref
    \Input *pBuffer     - [out] pointer to buffer to format url in
    \Input iBufSize     - size of buffer
    \Input *pCmd        - pointer to command name

    \Version 1.0 12/10/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static void _LockerApiFormatBaseUrl(LockerApiRefT *pLockerApi, char *pBuffer, int32_t iBufSize, const char *pCmd)
{
    snzprintf(pBuffer, iBufSize, "%s?site=easo&cmd=%s&lkey=%s", pLockerApi->strBaseUrl, pCmd, pLockerApi->strLKey);
}

/*F********************************************************************************/
/*!
    \Function _LockerApiDefaultCallback

    \Description
        Default LockerApi user callback.

    \Input *pLockerApi  - locker object ref
    \Input eEvent       - callback event
    \Input *pFile       - file pointer
    \Input *pUserData   - callback user data

    \Version 1.0 11/24/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static void _LockerApiDefaultCallback(LockerApiRefT *pLockerApi, LockerApiEventE eEvent, LockerApiFileT *pFile, void *pUserData)
{
}

/*F********************************************************************************/
/*!
    \Function _LockerSortFunc

    \Description
        Locker file sort function callback.

    \Input *pRef    - lockerapi ref
    \Input *pElemA  - pointer to first element to compare
    \Input *pElemB  - pointer to second element to compare

    \Output
        int32_t         - sign of result determines swap

    \Version 1.0 12/01/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static int32_t _LockerSortFunc(void *pRef, const void *pElemA, const void *pElemB)
{
    LockerApiRefT *pLockerApi = (LockerApiRefT *)pRef;
    LockerApiFileT *pFileA = (LockerApiFileT *)pElemA;
    LockerApiFileT *pFileB = (LockerApiFileT *)pElemB;
    int32_t iResult;

    // perform comparison based on sort category
    switch(pLockerApi->eSortCat)
    {
        case LOCKERAPI_CATEGORY_NAME:
            iResult = ds_stricmp(pFileB->Info.strName, pFileA->Info.strName);
            break;
        case LOCKERAPI_CATEGORY_GAME:
            iResult = ds_stricmp(pFileB->Info.strGame, pFileA->Info.strGame);
            break;
        case LOCKERAPI_CATEGORY_TYPE:
            iResult = ds_stricmp(pFileB->Info.strType, pFileA->Info.strType);
            break;
        case LOCKERAPI_CATEGORY_SIZE:
            iResult = pFileB->Info.iLocSize - pFileA->Info.iLocSize;
            break;
        case LOCKERAPI_CATEGORY_DATE:
            iResult = pFileB->iDate - pFileA->iDate;
            break;
        case LOCKERAPI_CATEGORY_ATTR:
            iResult = (pFileB->Info.iAttr & ~LOCKERAPI_FILEATTR_PLATMASK) - (pFileA->Info.iAttr & ~LOCKERAPI_FILEATTR_PLATMASK);
            break;
        case LOCKERAPI_CATEGORY_IDNT:
            iResult = ds_stricmp(pFileB->Info.strIdnt, pFileA->Info.strIdnt);
            break;
        case LOCKERAPI_CATEGORY_PERM:
            iResult = (pFileA->Info.iPermissions - pFileB->Info.iPermissions);
            break;

        default:
            NetPrintf(("lockerapi: invalid sort type %d\n", pLockerApi->eSortCat));
            iResult = 0;
            break;
    }

    // account for sort direction
    iResult *= pLockerApi->iSortDir;
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _LockerApiReset

    \Description
        Reset transfer state

    \Input *pLockerApi  - locker object ref

    \Version 1.0 05/31/2005 (jbrookes) First Version
*/
/********************************************************************************F*/
static void _LockerApiReset(LockerApiRefT *pLockerApi)
{
    // set to idle state
    pLockerApi->eState = ST_IDLE;

    // clear current file info, if any
    memset(&pLockerApi->CurrentFile, 0, sizeof(pLockerApi->CurrentFile));
    pLockerApi->iCurFileId = -1;

    // reset transfer parameters
    pLockerApi->iSendOffset = 0;
    pLockerApi->iSendSize = 0;
    pLockerApi->iRecvOffset = 0;
    pLockerApi->iRecvSize = 0;
}

/*F********************************************************************************/
/*!
    \Function _LockerApiSend

    \Description
        Send data to server.

    \Input *pLockerApi  - locker object ref
    \Input *pData       - pointer to data to send
    \Input iDataSize    - size of data to send
    
    \Output
        int32_t             - size of data sent

    \Version 1.0 11/29/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static int32_t _LockerApiSend(LockerApiRefT *pLockerApi, const void *pData, int32_t iDataSize)
{
    int32_t iRemainder, iSentSize;

    // calculate amount of data remaining
    if ((iRemainder = (pLockerApi->iSendSize - pLockerApi->iSendOffset)) < iDataSize)
    {
        iDataSize = iRemainder;
    }

    // send data
    if ((iSentSize = ProtoHttpSend(pLockerApi->pHttp, pData, iDataSize)) > 0)
    {
        pLockerApi->iSendOffset += iSentSize;

        #if LOCKERAPI_VERBOSE
        NetPrintf(("lockerapi: sent %d/%d bytes at %d\n", pLockerApi->iSendOffset, pLockerApi->iSendSize, NetTick()));
        #endif
    }
    return(iSentSize);
}

/*F********************************************************************************/
/*!
    \Function _LockerApiRecv

    \Description
        Send data to server.

    \Input *pLockerApi  - locker object ref
    \Input *pData       - pointer to data to send
    \Input iDataSize    - size of data to send

    \Output
        int32_t             - size of data sent

    \Version 1.0 11/29/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static int32_t _LockerApiRecv(LockerApiRefT *pLockerApi, void *pData, int32_t iDataSize)
{
    int32_t iRemainder, iRecvSize;

    // calculate amount of data remaining
    if ((iRemainder = (pLockerApi->iRecvSize - pLockerApi->iRecvOffset)) < iDataSize)
    {
        iDataSize = iRemainder;
    }

    // receive data
    if ((iRecvSize = ProtoHttpRecv(pLockerApi->pHttp, pData, iDataSize, iDataSize)) > 0)
    {
        pLockerApi->iRecvOffset += iRecvSize;
        
        #if LOCKERAPI_VERBOSE
        NetPrintf(("lockerapi: recv %d/%d bytes at %d\n", pLockerApi->iRecvOffset, pLockerApi->iRecvSize, NetTick()));
        #endif
    }
    return(iRecvSize);
}

/*F********************************************************************************/
/*!
    \Function _LockerApiGetFileFromId

    \Description
        Get file based on fileId.

    \Input *pLockerApi  - locker object ref
    \Input iFileId      - file's Id value
    \Input *pFileIdx    - [out] pointer to storage for file index, or NULL
    
    \Output
        LockerApiFileT *    - pointer to file, or NULL

    \Version 1.0 11/30/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static const LockerApiFileT *_LockerApiGetFileFromId(LockerApiRefT *pLockerApi, int32_t iFileId, int32_t *pFileIdx)
{
    int32_t iFileIndex;
    for (iFileIndex = 0; iFileIndex < pLockerApi->CurrentDir.iNumFiles; iFileIndex++)
    {
        if (pLockerApi->CurrentDir.Files[iFileIndex].iId == iFileId)
        {
            if (pFileIdx != NULL)
            {
                *pFileIdx = iFileIndex;
            }
            return(&pLockerApi->CurrentDir.Files[iFileIndex]);
        }
    }
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function _LockerApiXmlResponse

    \Description
        Parse XML response.

    \Input *pLockerApi  - locker object ref
    \Input *ppXml       - [out] optional storage for pointer to XML locker tag
    
    \Output
        int32_t             - xml eerror - LOCKERAPI_ERROR_*
        unsigned char * - if no error, pointer to LOCKER tag, else NULL

    \Version 1.0 11/30/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static int32_t _LockerApiXmlResponse(LockerApiRefT *pLockerApi, const unsigned char **ppXml)
{
    const unsigned char *pXml;
    int32_t iError;

    // parse the XML header
    if ((pXml = XmlFind((unsigned char *)pLockerApi->strXml, (unsigned char *)"LOCKER")) == NULL)
    {
        pLockerApi->eCurError = LOCKERAPI_ERROR_SERVER;
        return(LOCKERAPI_EVENT_ERROR);
    }

    // save xml ref for caller?
    if (ppXml)
    {
        *ppXml = pXml;
    }

    // extract query result
    iError = XmlAttribGetInteger(pXml, (unsigned char *)"error", LOCKERAPI_ERROR_SERVER);
    pLockerApi->eCurError = iError;

    #if DIRTYCODE_LOGGING
    // if an error occurred, output debug info
    if (iError != LOCKERAPI_ERROR_NONE)
    {
        char strError[128];
        XmlAttribGetString(pXml, (unsigned char *)"message", (unsigned char *)strError, sizeof(strError), (unsigned char *)"");
        NetPrintf(("lockerapi: xml response %d: %s\n", iError, strError));
    }
    #endif
    
    // return xml error code to caller
    return(iError);
}

/*F********************************************************************************/
/*!
    \Function _LockerApiWaitResponse

    \Description
        Wait for XML response, and return TRUE if the response was successful.

    \Input *pLockerApi  - locker object ref
    
    \Output
        LockerApiEventE - locker event

    \Version 1.0 12/08/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static int32_t _LockerApiWaitResponse(LockerApiRefT *pLockerApi)
{
    ProtoHttpResponseE eResponse;    
    int32_t iStatus;

    // do we have the body size yet?
    if (pLockerApi->iRecvSize == 0)
    {
        // get size of xml response
        if ((iStatus = ProtoHttpStatus(pLockerApi->pHttp, 'body', NULL, 0)) > (int32_t)sizeof(pLockerApi->strXml))
        {
            // response size is too large
            NetPrintf(("lockerapi: server xml response does not fit in xml buffer\n"));
            pLockerApi->eCurError = LOCKERAPI_ERROR_CLIENT;
            return(LOCKERAPI_EVENT_ERROR);
        }
        else if (iStatus > 0)
        {
#if LOCKERAPI_PRINT_XML
            NetPrintf(("LockerApi : xmlFile = %s", pLockerApi->strXml));
#endif
            pLockerApi->iRecvSize = iStatus;
        }
    }

    // if we need data, try to read it
    if (pLockerApi->iRecvOffset < pLockerApi->iRecvSize)
    {
        if ((iStatus = ProtoHttpRecv(pLockerApi->pHttp, pLockerApi->strXml + pLockerApi->iRecvOffset, 1, pLockerApi->iRecvSize - pLockerApi->iRecvOffset)) > 0)
        {
            pLockerApi->iRecvOffset += iStatus;
        }
    }

    // make sure we're done
    if ((iStatus = ProtoHttpStatus(pLockerApi->pHttp, 'done', NULL, 0)) == 0)
    {
        return(LOCKERAPI_EVENT_NONE);
    }

    // get response
    eResponse = PROTOHTTP_GetResponseClass(ProtoHttpStatus(pLockerApi->pHttp, 'code', NULL, 0));
    if ((iStatus < 0) || (eResponse != PROTOHTTP_RESPONSE_SUCCESSFUL))
    {
        pLockerApi->eCurError = LOCKERAPI_ERROR_SERVER;
        return(LOCKERAPI_EVENT_ERROR);
    }

    // if we have the response, return success
    if ((pLockerApi->iRecvSize > 0) && (pLockerApi->iRecvOffset == pLockerApi->iRecvSize))
    {
        // null-terminate response
        if (pLockerApi->iRecvSize < LOCKERAPI_XMLSIZE)
        {
            pLockerApi->strXml[pLockerApi->iRecvSize] = '\0';
        }
        else
        {
            pLockerApi->strXml[LOCKERAPI_XMLSIZE - 1] = '\0';
        }

        // determine response success or failure
        if (_LockerApiXmlResponse(pLockerApi, NULL) == 0)
        {
            return(LOCKERAPI_EVENT_DONE);
        }
        else
        {
            return(LOCKERAPI_EVENT_ERROR);
        }
    }

    return(LOCKERAPI_EVENT_NONE);
}

/*F********************************************************************************/
/*!
    \Function _LockerApiParseFile

    \Description
        Parse file tag returned by server.

    \Input *pFile   - pointer to file structure to fill in
    \Input *pXml    - pointer to xml to read file info from
    \Input iFileId  - index of file

    \Version 1.0 11/30/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static void _LockerApiParseFile(LockerApiFileT *pFile, const unsigned char *pXml, int32_t iFileId)
{
    pFile->iId = iFileId;
    pFile->iDate = XmlAttribGetInteger(pXml, (unsigned char *)"date", 0);
    XmlAttribGetString(pXml, (unsigned char *)"name", (unsigned char *)pFile->Info.strName, sizeof(pFile->Info.strName), (unsigned char *)"");
    XmlAttribGetString(pXml, (unsigned char *)"desc", (unsigned char *)pFile->Info.strDesc, sizeof(pFile->Info.strDesc), (unsigned char *)"");
    XmlAttribGetString(pXml, (unsigned char *)"game", (unsigned char *)pFile->Info.strGame, sizeof(pFile->Info.strGame), (unsigned char *)"");    
    XmlAttribGetString(pXml, (unsigned char *)"type", (unsigned char *)pFile->Info.strType, sizeof(pFile->Info.strType), (unsigned char *)"");
    XmlAttribGetString(pXml, (unsigned char *)"idnt", (unsigned char *)pFile->Info.strIdnt, sizeof(pFile->Info.strIdnt), (unsigned char *)"");
    pFile->Info.iVersion = XmlAttribGetInteger(pXml, (unsigned char *)"vers", 0);

    pFile->Info.iAttr = XmlAttribGetInteger(pXml, (unsigned char *)"attr", 0);
    pFile->Info.iNetSize = XmlAttribGetInteger(pXml, (unsigned char *)"size", 0);
    pFile->Info.iLocSize = XmlAttribGetInteger(pXml, (unsigned char *)"locs", 0);
    pFile->Info.iPermissions = XmlAttribGetInteger(pXml, (unsigned char *)"perm", 0);
}

/*F********************************************************************************/
/*!
    \Function _LockerApiParseDir

    \Description
        Parse directory returned by dir command.

    \Input *pLockerApi          - locker object ref
    \Input *pDirectory          - [out] storage for parsed directory
    
    \Output
        int32_t                     - zero=success, negative=failure

    \Version 1.0 11/30/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static int32_t _LockerApiParseDir(LockerApiRefT *pLockerApi, LockerApiDirectoryT *pDirectory)
{
    const unsigned char *pXml = NULL;
    int32_t iFile, iNumFiles, iMaxFiles, iNumBytes, iMaxBytes;
    char strOwner[32], strGame[32];
    
    // make sure XML returned success
    if (_LockerApiXmlResponse(pLockerApi, &pXml) != LOCKERAPI_ERROR_NONE)
    {
        return(-1);
    }

    // parse attributes
    iNumFiles = XmlAttribGetInteger(pXml, (unsigned char *)"numFiles", 0);
    iMaxFiles = XmlAttribGetInteger(pXml, (unsigned char *)"maxFiles", 0);
    iNumBytes = XmlAttribGetInteger(pXml, (unsigned char *)"numBytes", 0);
    iMaxBytes = XmlAttribGetInteger(pXml, (unsigned char *)"maxBytes", 0);
    XmlAttribGetString(pXml, (unsigned char *)"game", (unsigned char *)strGame, sizeof(strGame), (unsigned char *)"");
    XmlAttribGetString(pXml, (unsigned char *)"pers", (unsigned char *)strOwner, sizeof(strOwner), (unsigned char *)"");
    
    // clamp to max file count
    if (iNumFiles > LOCKERAPI_MAXFILES)
    {
        NetPrintf(("lockerapi: clamping file count of %d to %d\n", iNumFiles, LOCKERAPI_MAXFILES));
        iNumFiles = LOCKERAPI_MAXFILES;
    }

    // max file count of zero means default max count
    if (iMaxFiles == 0)
    {
        iMaxFiles = LOCKERAPI_MAXFILES;
    }

    // fill in directory header
    pDirectory->iNumFiles = iNumFiles;
    pDirectory->iMaxFiles = iMaxFiles;
    pDirectory->iNumBytes = iNumBytes;
    pDirectory->iMaxBytes = iMaxBytes;
    strnzcpy(pDirectory->strGame, strGame, sizeof(pDirectory->strGame));    
    strnzcpy(pDirectory->strOwner, strOwner, sizeof(pDirectory->strOwner));

    // find first directory entry
    if ((pXml = XmlFind(pXml, (unsigned char *)"LOCKER.FILE")) != NULL)
    {
        // parse directory list
        for (iFile = 0; iFile < iNumFiles; iFile++)
        {
            // parse the file
            _LockerApiParseFile(&pDirectory->Files[iFile], pXml, iFile);

            // index to next file
            if ((pXml = XmlNext(pXml)) == NULL)
            {
                break;
            }
        }

        // make sure we got all the files we were supposed to
        if (iFile != (iNumFiles-1))
        {
            NetPrintf(("lockerapi: unable to parse all files in directory\n"));
            pLockerApi->eCurError = LOCKERAPI_ERROR_CLIENT;
            return(-1);
        }
    }

    // done!
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _LockerApiUpdateDir

    \Description
        Update directory fetch operation.

    \Input *pLockerApi  - locker object ref
    
    \Output
        LockerApiEventE - locker event

    \Version 1.0 11/24/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static LockerApiEventE _LockerApiUpdateDir(LockerApiRefT *pLockerApi)
{
    LockerApiEventE eEvent;

    // wait until we have gotten a response
    if ((eEvent = _LockerApiWaitResponse(pLockerApi)) <= LOCKERAPI_EVENT_NONE)
    {
        return(eEvent);
    }

    // parse directory
    if ((_LockerApiParseDir(pLockerApi, &pLockerApi->CurrentDir)) < 0)
    {
        // clear directory
        memset(&pLockerApi->CurrentDir, 0, sizeof(pLockerApi->CurrentDir));
        return(LOCKERAPI_EVENT_ERROR);
    }

    // return success
    return(LOCKERAPI_EVENT_DIRECTORY);
}

/*F********************************************************************************/
/*!
    \Function _LockerApiSendFileInfoUpdate

    \Description
        Actually sends the UpdateFileInfo request to the server and transitions
        to the ST_NFO state.

    \Input *pLockerApi  - locker object ref
    
    \Output
        LockerApiEventE - locker event

    \Version 1.0 03/15/2006 (tdills) First Version
*/
/********************************************************************************F*/
static LockerApiEventE _LockerApiSendFileInfoUpdate(LockerApiRefT *pLockerApi)
{
    char strUrl[1024];
    int32_t iAttr;

    // get attribute flag to set
    iAttr = pLockerApi->iCurFileAttr;
    if (iAttr == -1)
    {
        // retain previous attribute
        iAttr = pLockerApi->CurrentFile.Info.iAttr;
    }
    else
    {
        // retain previous platform flags, but use new lower attribute bits
        iAttr &= ~LOCKERAPI_FILEATTR_PLATMASK;
        iAttr |= pLockerApi->CurrentFile.Info.iAttr & LOCKERAPI_FILEATTR_PLATMASK;
    }

    // format the URL
    _LockerApiFormatBaseUrl(pLockerApi, strUrl, sizeof(strUrl), "nfo");
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&name=", pLockerApi->CurrentFile.Info.strName);
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&type=", pLockerApi->CurrentFile.Info.strType);
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&game=", pLockerApi->CurrentFile.Info.strGame);
    ProtoHttpUrlEncodeIntParm(strUrl, sizeof(strUrl), "&attr=", iAttr);
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&idnt=", pLockerApi->CurrentFile.Info.strIdnt);
    ProtoHttpUrlEncodeIntParm(strUrl, sizeof(strUrl), "&vers=", pLockerApi->CurrentFile.Info.iVersion);
    ProtoHttpUrlEncodeIntParm(strUrl, sizeof(strUrl), "&perm=", pLockerApi->CurrentFile.Info.iPermissions);

    if (pLockerApi->CurrentFile.Info.strDesc[0] != '\0')
    {
        ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&desc=", pLockerApi->CurrentFile.Info.strDesc);
    }

    // issue the request
    if (ProtoHttpPost(pLockerApi->pHttp, strUrl, NULL, 0, PROTOHTTP_POST) < 0)
    {
        NetPrintf(("lockerapi: error initiating info transaction\n"));
        return(LOCKERAPI_EVENT_ERROR);
    }
    pLockerApi->eState = ST_NFO;
    return(LOCKERAPI_EVENT_NONE);
}

/*F********************************************************************************/
/*!
    \Function _LockerApiSendPutFile

    \Description
        Actually sends the PutFile request to the server and transitions
        to the ST_PUT state.

    \Input *pLockerApi  - locker object ref
    
    \Output
        LockerApiEventE - locker event

    \Version 1.0 03/15/2006 (tdills) First Version
*/
/********************************************************************************F*/
static LockerApiEventE _LockerApiSendPutFile(LockerApiRefT *pLockerApi)
{
    char strUrl[1024 + LOCKERAPI_MAX_META_SIZE];

    // format the URL
    _LockerApiFormatBaseUrl(pLockerApi, strUrl, sizeof(strUrl), "put");
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&name=", pLockerApi->CurrentFile.Info.strName);
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&desc=", pLockerApi->CurrentFile.Info.strDesc);
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&type=", pLockerApi->CurrentFile.Info.strType);
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&idnt=", pLockerApi->CurrentFile.Info.strIdnt);
    ProtoHttpUrlEncodeIntParm(strUrl, sizeof(strUrl), "&perm=", pLockerApi->CurrentFile.Info.iPermissions);
    ProtoHttpUrlEncodeIntParm(strUrl, sizeof(strUrl), "&attr=", pLockerApi->iCurFileAttr);
    ProtoHttpUrlEncodeIntParm(strUrl, sizeof(strUrl), "&locs=", pLockerApi->CurrentFile.Info.iLocSize);
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&pers=", pLockerApi->strCurFileOwner);
    ProtoHttpUrlEncodeIntParm(strUrl, sizeof(strUrl), "&vers=", pLockerApi->CurrentFile.Info.iVersion);
    strnzcat(strUrl, pLockerApi->strMeta, sizeof(strUrl));

    // if overwrite is set, add it to URL
    if (pLockerApi->bCurFileOverwrite != FALSE)
    {
        ProtoHttpUrlEncodeIntParm(strUrl, sizeof(strUrl), "&over=", TRUE);
    }
    if (pLockerApi->strGameName[0] != '\0')
    {
        ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&game=", pLockerApi->strGameName);
    }

    // set upload amount
    pLockerApi->iSendSize = pLockerApi->CurrentFile.Info.iNetSize;

    #if LOCKERAPI_VERBOSE
    NetPrintf(("lockerapi: starting upload of file %s (%d bytes) at %d\n", pLockerApi->CurrentFile.Info.strName, pLockerApi->CurrentFile.Info.iNetSize, NetTick()));
    #endif

    // issue the request
    if (ProtoHttpPost(pLockerApi->pHttp, strUrl, NULL, pLockerApi->iSendSize, PROTOHTTP_POST) < 0)
    {
        NetPrintf(("lockerapi: error initiating post transaction\n"));
        return(LOCKERAPI_EVENT_ERROR);
    }

    // put us in proper state
    pLockerApi->eState = ST_PUT;
    return(LOCKERAPI_EVENT_NONE);
}

/*F********************************************************************************/
/*!
    \Function _LockerApiUpdateFilter

    \Description
        Get filter response and then continue to current operation.

    \Input *pLockerApi  - locker object ref
    
    \Output
        LockerApiEventE - locker event

    \Version 1.0 03/15/2006 (tdills) First Version
*/
/********************************************************************************F*/
static LockerApiEventE _LockerApiUpdateFilter(LockerApiRefT *pLockerApi)
{
    int32_t iFilterResult;

    // get the result from the profanity filter.
    iFilterResult = ProtoFilterResult(pLockerApi->pFilter, pLockerApi->uFilterReqId);
    switch (iFilterResult)
    {
    case PROTOFILTER_VIRTUOUS:
        {
            // nothing to do;
            NetPrintf(("lockerapi: description string is valid\n"));
        }
        break;
    case PROTOFILTER_PROFANE:
        {
            // filter the description string
            ProtoFilterApplyFilter(pLockerApi->pFilter, pLockerApi->uFilterType, pLockerApi->CurrentFile.Info.strDesc);
            NetPrintf(("lockerapi: description string is invalid.\n"));
        }
        break;
    case PROTOFILTER_ERROR_PENDING:
        {
            // the response is still pending so stay in this state for another cycle.
            return(LOCKERAPI_EVENT_NONE);
        }
    case PROTOFILTER_INVALID_ID:
        {
            // this shouldn't happen - we shouldn't be able to get here with an invalid request id.
            NetPrintf(("lockerapi: error retrieving response from profanity filter.\n"));
            return(LOCKERAPI_EVENT_ERROR);
        }
    }

    if (pLockerApi->eState == ST_NFO_FILTER)
    {
        // Send the update request to the server
        return(_LockerApiSendFileInfoUpdate(pLockerApi));
    }
    else if (pLockerApi->eState == ST_PUT_FILTER)
    {
        // Send the put request to the server
        return(_LockerApiSendPutFile(pLockerApi));
    }

    // one of those two states should be true; if not there is an error
    return(LOCKERAPI_EVENT_ERROR);
}

/*F********************************************************************************/
/*!
    \Function _LockerApiUpdateNfo

    \Description
        Update file info set operation.

    \Input *pLockerApi  - locker object ref
    
    \Output
        LockerApiEventE - locker event

    \Version 1.0 11/24/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static LockerApiEventE _LockerApiUpdateNfo(LockerApiRefT *pLockerApi)
{
    LockerApiEventE eResponse;

    // wait until we have a response
    if ((eResponse = _LockerApiWaitResponse(pLockerApi)) == LOCKERAPI_EVENT_NONE)
    {
        return(eResponse);
    }

    // if success, parse the response and update the file info
    if (eResponse == LOCKERAPI_EVENT_DONE)
    {
        const unsigned char *pXml;
        
        if ((pXml = XmlFind((unsigned char *)pLockerApi->strXml, (unsigned char *)"LOCKER.FILE")) != NULL)
        {
            int32_t iLocSize;
            
            // ref current file
            LockerApiFileT *pFile = (LockerApiFileT *)_LockerApiGetFileFromId(pLockerApi, pLockerApi->iCurFileId, NULL);

            // save local size attribute, in case it got changed behind our back
            iLocSize = pFile->Info.iLocSize;

            // update file info
            _LockerApiParseFile(pFile, pXml, pLockerApi->iCurFileId);

            // if the size changed, generate a directory event
            if (pFile->Info.iLocSize != iLocSize)
            {
                eResponse = LOCKERAPI_EVENT_DIRECTORY;
            }
        }
    }

    return(eResponse);
}

/*F********************************************************************************/
/*!
    \Function _LockerApiUpdateGet

    \Description
        Update file get operation.

    \Input *pLockerApi  - locker object ref
    
    \Output
        LockerApiEventE - locker event

    \Version 1.0 11/24/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static LockerApiEventE _LockerApiUpdateGet(LockerApiRefT *pLockerApi)
{
    ProtoHttpResponseE eResponse;
    int32_t iStatus;

    // wait for response
    if ((eResponse = ProtoHttpStatus(pLockerApi->pHttp, 'code', NULL, 0)) != (unsigned)-1)
    {
        if (eResponse != PROTOHTTP_RESPONSE_OK)
        {
            switch(eResponse)
            {
                case PROTOHTTP_RESPONSE_UNAUTHORIZED:
                    pLockerApi->eCurError = LOCKERAPI_ERROR_NOPERMISSION;
                    break;
                case PROTOHTTP_RESPONSE_NOTFOUND:
                    pLockerApi->eCurError = LOCKERAPI_ERROR_FILEMISSING;
                    break;
                default:
                    pLockerApi->eCurError = LOCKERAPI_ERROR_SERVER;
                    break;
            }

            return(LOCKERAPI_EVENT_ERROR);
        }
    }
    else
    {
        return(LOCKERAPI_EVENT_NONE);
    }

    // check for a timeout condition
    if ((iStatus = ProtoHttpStatus(pLockerApi->pHttp, 'time', NULL, 0)) != 0)
    {
        pLockerApi->eCurError = LOCKERAPI_ERROR_SERVER;
        return(LOCKERAPI_EVENT_ERROR);
    }
    
    // if we still need data, trigger read callback
    if (pLockerApi->iRecvOffset < pLockerApi->iRecvSize)
    {
        pLockerApi->pCallback(pLockerApi, LOCKERAPI_EVENT_READDATA, &pLockerApi->CurrentFile, pLockerApi->pUserData);
        return(LOCKERAPI_EVENT_NONE);
    }

    // check for completion
    if ((iStatus = ProtoHttpStatus(pLockerApi->pHttp, 'done', NULL, 0)) != 0)
    {
        if ((iStatus < 0) || (pLockerApi->iRecvOffset != pLockerApi->iRecvSize))
        {
            pLockerApi->eCurError = LOCKERAPI_ERROR_SERVER;
            return(LOCKERAPI_EVENT_ERROR);
        }
        else
        {
            return(LOCKERAPI_EVENT_DONE);
        }
    }

    return(LOCKERAPI_EVENT_NONE);
}

/*F********************************************************************************/
/*!
    \Function _LockerApiUpdatePut

    \Description
        Update file put operation.

    \Input *pLockerApi  - locker object ref
    
    \Output
        LockerApiEventE - locker event

    \Version 1.0 11/24/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static LockerApiEventE _LockerApiUpdatePut(LockerApiRefT *pLockerApi)
{
    LockerApiEventE eEvent;
    int32_t iDataRequired;

    // see if we have an error response
    if ((eEvent = _LockerApiWaitResponse(pLockerApi)) == LOCKERAPI_EVENT_ERROR)
    {
        return(eEvent);
    }

    // get amount of data currently required
    if ((iDataRequired = (pLockerApi->iSendSize - pLockerApi->iSendOffset)) > pLockerApi->iSpoolSize)
    {
        iDataRequired = pLockerApi->iSpoolSize;
    }

    // ask for more data?
    if (iDataRequired > 0)
    {
        pLockerApi->pCallback(pLockerApi, LOCKERAPI_EVENT_WRITEDATA, &pLockerApi->CurrentFile, pLockerApi->pUserData);
    }

    // wait until we have uploaded all of the data
    if (pLockerApi->iSendOffset < pLockerApi->iSendSize)
    {
        return(eEvent);
    }

    // if we're done, add file to directory structure?
    if ((eEvent == LOCKERAPI_EVENT_DONE) && (!strcmp(pLockerApi->strCurFileOwner, pLockerApi->CurrentDir.strOwner)))
    {
        const unsigned char *pXml = NULL;
        int32_t iFileId;

        // get xml response
        _LockerApiXmlResponse(pLockerApi, &pXml);

        // find file entry
        if ((pXml = XmlFind(pXml, (unsigned char *)"LOCKER.FILE")) != NULL)
        {
            LockerApiFileT File;
            
            // parse file response into file structure
            _LockerApiParseFile(&File, pXml, pLockerApi->CurrentDir.iNumFiles);
        
            // see if file is already in directory
            for (iFileId = 0; iFileId < pLockerApi->CurrentDir.iNumFiles; iFileId++)
            {
                LockerApiFileT *pFile = &pLockerApi->CurrentDir.Files[iFileId];

                if (!strcmp(File.Info.strName, pFile->Info.strName) &&
                    !strcmp(File.Info.strType, pFile->Info.strType) &&
                    !strcmp(File.Info.strGame, pFile->Info.strGame) &&                    
                    !ds_stricmp(File.Info.strIdnt, pFile->Info.strIdnt) &&
                    (File.Info.iPermissions == pFile->Info.iPermissions) &&
                    ((File.Info.iAttr & LOCKERAPI_FILEATTR_PLATMASK) == (pFile->Info.iAttr & LOCKERAPI_FILEATTR_PLATMASK)))
                {
                    // since we're updating a pre-existing entry, copy the id
                    File.iId = iFileId;
                    break;
                }
            }
            
            // if we have room for the file
            if (iFileId < pLockerApi->CurrentDir.iMaxFiles)
            {
                // copy file to directory
                memcpy(&pLockerApi->CurrentDir.Files[iFileId], &File, sizeof(File));
                
                // increment file count?
                if (iFileId == pLockerApi->CurrentDir.iNumFiles)
                {
                    pLockerApi->CurrentDir.iNumFiles++;
                }

                // indicate directory was updated
                return(LOCKERAPI_EVENT_DIRECTORY);
            }
        }
    }

    return(eEvent);
}

/*F********************************************************************************/
/*!
    \Function _LockerApiUpdateDel

    \Description
        Update file delete operation.

    \Input *pLockerApi  - locker object ref
    
    \Output
        LockerApiEventE - locker event

    \Version 1.0 11/24/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static LockerApiEventE _LockerApiUpdateDel(LockerApiRefT *pLockerApi)
{
    LockerApiFileT *pDelFile;
    LockerApiEventE eEvent;
    int32_t iFileIndex = 0;

    // wait until we have gotten a response
    if ((eEvent = _LockerApiWaitResponse(pLockerApi)) <= 0)
    {
        return(eEvent);
    }

    // ref file for deletion
    pDelFile = (LockerApiFileT *)_LockerApiGetFileFromId(pLockerApi, pLockerApi->iCurFileId, &iFileIndex);

    // if it isn't the last file in the directory, remove it from the directory
    if (iFileIndex != (pLockerApi->CurrentDir.iNumFiles - 1))
    {
        int32_t iFileId, iNumMoveFiles;

        // move the files to remove the gap
        iNumMoveFiles = (pLockerApi->CurrentDir.iNumFiles - 1) - iFileIndex;
        memmove(pDelFile, pDelFile + 1, iNumMoveFiles * sizeof(LockerApiFileT));

        // rekey the files
        for (iFileId = 0; iFileId < pLockerApi->CurrentDir.iNumFiles-1; iFileId++)
        {
            pLockerApi->CurrentDir.Files[iFileId].iId = iFileId;
        }
    }

    // decrement overall count
    pLockerApi->CurrentDir.iNumFiles--;

    // return success
    return(LOCKERAPI_EVENT_DIRECTORY);
}


/*F********************************************************************************/
/*!
    \Function _LockerApiUpdateSearch

    \Description 
        A response to a search meta data has arrived. Looks the same as a directory 

    \Input *pLockerApi  - locker object ref

    \Output
        LockerApiEventE - locker event

    \Version 02/05/2007 (ozietman)
*/ 
/********************************************************************************F*/
static LockerApiEventE _LockerApiUpdateSearch(LockerApiRefT *pLockerApi)
{
    LockerApiEventE eEvent;

    // see if we have an error response
    if ((eEvent = _LockerApiWaitResponse(pLockerApi)) <= LOCKERAPI_EVENT_NONE)
    {
        return(eEvent);
    }

     // parse directory
    if ((_LockerApiParseDir(pLockerApi, &pLockerApi->CurrentSearchReult)) < 0)
    {
        // clear directory
        memset(&pLockerApi->CurrentSearchReult, 0, sizeof(pLockerApi->CurrentSearchReult));
        return(LOCKERAPI_EVENT_ERROR);
    }
    return(LOCKERAPI_EVENT_DONE);
}

/*F********************************************************************************/
/*!
    \Function _LockerApiUpdate

    \Description
        Update the LockerApi module state.

    \Input *pData   - pointer to module state
    \Input uTick    - current millisecond tick

    \Version 1.0 11/24/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static void _LockerApiUpdate(void *pData, uint32_t uTick)
{
    LockerApiRefT *pLockerApi = (LockerApiRefT *)pData;
    LockerApiEventE eEvent = LOCKERAPI_EVENT_NONE;
    
    // update if not idle
    if (pLockerApi->eState != ST_IDLE)
    {
        // give life to http module
        ProtoHttpUpdate(pLockerApi->pHttp);

        // make sure we haven't timed out
        if (ProtoHttpStatus(pLockerApi->pHttp, 'time', NULL, 0) == FALSE)
        {
            // operate based on state
            switch(pLockerApi->eState)
            {
                case ST_DIR:
                    eEvent = _LockerApiUpdateDir(pLockerApi);
                    break;
                case ST_NFO:
                    eEvent = _LockerApiUpdateNfo(pLockerApi);
                    break;
                case ST_NFO_FILTER:
                    eEvent = _LockerApiUpdateFilter(pLockerApi);
                    break;
                case ST_GET:
                    eEvent = _LockerApiUpdateGet(pLockerApi);
                    break;
                case ST_PUT:
                    eEvent = _LockerApiUpdatePut(pLockerApi);
                    break;
                case ST_PUT_FILTER:
                    eEvent = _LockerApiUpdateFilter(pLockerApi);
                    break;
                case ST_DEL:
                    eEvent = _LockerApiUpdateDel(pLockerApi);
                    break;
                case ST_SEARCH_META:
                    eEvent = _LockerApiUpdateSearch(pLockerApi);
                    break;
                default:
                    NetPrintf(("lockerapi: unknown state %d\n", pLockerApi->eState));
                    break;
            }
        }
        else
        {
            #if LOCKERAPI_VERBOSE
            NetPrintf(("lockerapi: timeout at %d\n", NetTick()));
            #endif
            
            eEvent = LOCKERAPI_EVENT_ERROR;
            pLockerApi->eCurError = LOCKERAPI_ERROR_TIMEOUT;
        }
    }

    // if event isn't none, we're done
    if (eEvent != LOCKERAPI_EVENT_NONE)
    {
        LockerApiFileT *pFile = (pLockerApi->eState != ST_DIR) ? &pLockerApi->CurrentFile : NULL;

        // update storage calculations
        if (eEvent == LOCKERAPI_EVENT_DIRECTORY)
        {
            _LockerApiCalcStorage(&pLockerApi->CurrentDir);
        }

        // trigger user callback
        pLockerApi->pCallback(pLockerApi, (LockerApiEventE)eEvent, pFile, pLockerApi->pUserData);

        // reset state
        _LockerApiReset(pLockerApi);
    }

    // update current event status
    pLockerApi->eCurEvent = eEvent;
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function LockerApiCreate

    \Description
        Create a locker object.

    \Input *pServer     - pointer to locker server name, or NULL to use default
    \Input *pCallback   - pointer to user-supplied event callback
    \Input *pUserData   - pointer to user data to pass to callback

    \Output
        LockerApiRefT * - pointer to new locker object, or NULL

    \Version 1.0 11/24/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
LockerApiRefT *LockerApiCreate(const char *pServer, LockerApiCallbackT *pCallback, void *pUserData)
{
    LockerApiRefT *pLockerApi;
    int32_t iObjectSize;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // make sure server is specified
    if (pServer == NULL)
    {
        NetPrintf(("lockerapi: please spesify a server and don't hard code it\n"));
        return(NULL);
    }

    // calculate object size
    iObjectSize =  sizeof(*pLockerApi);

    // allocate and initialize module state
    if ((pLockerApi = DirtyMemAlloc(iObjectSize, LOCKERAPI_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("lockerapi: unable to allocate module state\n"));
        return(NULL);
    }
    memset(pLockerApi, 0, iObjectSize);
    pLockerApi->iMemGroup = iMemGroup;
    pLockerApi->pMemGroupUserData = pMemGroupUserData;
    pLockerApi->iSpoolSize = LOCKERAPI_SPOOLSIZE;
    pLockerApi->iCurFileId = -1;
    
    // allocate protohttp module
    if ((pLockerApi->pHttp = ProtoHttpCreate(pLockerApi->iSpoolSize)) == NULL)
    {
        NetPrintf(("lockerapi: unable to create protohttp module\n"));
        DirtyMemFree(pLockerApi, LOCKERAPI_MEMID, pLockerApi->iMemGroup, pLockerApi->pMemGroupUserData);
        return(NULL);
    }

    // allocate protofilter module
    if ((pLockerApi->pFilter = ProtoFilterCreate()) == NULL)
    {
        NetPrintf(("lockerapi: unable to create protofilter module\n"));
        DirtyMemFree(pLockerApi, LOCKERAPI_MEMID, pLockerApi->iMemGroup, pLockerApi->pMemGroupUserData);
        DirtyMemFree(pLockerApi, LOCKERAPI_MEMID, pLockerApi->iMemGroup, pLockerApi->pMemGroupUserData);
        return(NULL);
    }
    pLockerApi->uFilterType = PROTOFILTER_TYPE_PROFANE;

    // set http timeout
    ProtoHttpControl(pLockerApi->pHttp, 'time', 60*1000, 0, NULL);
    
    // set user callback
    pLockerApi->pCallback = (pCallback == NULL) ? _LockerApiDefaultCallback : pCallback;
    pLockerApi->pUserData = pUserData;

    // set server
    strnzcpy(pLockerApi->strBaseUrl, pServer, sizeof(pLockerApi->strBaseUrl));

    // install idle callback
    NetConnIdleAdd(_LockerApiUpdate, pLockerApi);

    // return state to caller
    return(pLockerApi);
}

/*F********************************************************************************/
/*!
    \Function LockerApiDestroy

    \Description
        Destroy a locker object.

    \Input *pLockerApi  - locker object ref

    \Version 1.0 11/24/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void LockerApiDestroy(LockerApiRefT *pLockerApi)
{
    // remove idle callback
    NetConnIdleDel(_LockerApiUpdate, pLockerApi);

    // destroy ProtoHttp module
    ProtoHttpDestroy(pLockerApi->pHttp);

    // destroy the ProtoFilter module
    ProtoFilterDestroy(pLockerApi->pFilter);

    // release module memory
    DirtyMemFree(pLockerApi, LOCKERAPI_MEMID, pLockerApi->iMemGroup, pLockerApi->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function LockerApiSetLoginInfo

    \Description
        Set login information

    \Input *pLockerApi  - locker object ref
    \Input *pLKey       - lkey to set
    \Input *pPersona    - persona lkey is associated with

    \Version 1.0 11/24/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void LockerApiSetLoginInfo(LockerApiRefT *pLockerApi, const char *pLKey, const char *pPersona)
{
    strnzcpy(pLockerApi->strLKey, pLKey, sizeof(pLockerApi->strLKey));
    strnzcpy(pLockerApi->strPersona, pPersona, sizeof(pLockerApi->strPersona));
}

/*F********************************************************************************/
/*!
    \Function LockerApiFetchLocker

    \Description
        Get a user's directory (async).

    \Input *pLockerApi  - locker object ref
    \Input *pOwner      - locker owner's persona
    \Input *pGameName   - locker Game name id 

    \Version 11/24/2004 (jbrookes)
*/
/********************************************************************************F*/
void LockerApiFetchLocker(LockerApiRefT *pLockerApi, const char *pOwner, const char *pGameName)
{
    char strUrl[256];
    
    // make sure we're idle
    if (pLockerApi->eState != ST_IDLE)
    {
        NetPrintf(("lockerapi: can't issue locker fetch while not idle\n"));
        return;
    }
    
    // invalidate current directory
    memset(&pLockerApi->CurrentDir, 0, sizeof(pLockerApi->CurrentDir));

    // clear error state
    pLockerApi->eCurError = LOCKERAPI_ERROR_NONE;

    // build url to fetch locker for given user
    _LockerApiFormatBaseUrl(pLockerApi, strUrl, sizeof(strUrl), "dir");
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&pers=", pOwner);
    if (pGameName != NULL)
    {
        strnzcpy(pLockerApi->strGameName, pGameName, sizeof(pLockerApi->strGameName));
        ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&game=", pGameName);
    }

    // issue the request and put us in proper state
    if (ProtoHttpGet(pLockerApi->pHttp, strUrl, PROTOHTTP_HEADBODY) < 0)
    {
        NetPrintf(("lockerapi: error initiating locker fetch\n"));
        return;
    }
    pLockerApi->eState = ST_DIR;
}

/*F********************************************************************************/
/*!
    \Function LockerApiGetDirectory

    \Description
        Get current directory.

    \Input *pLockerApi          - locker object ref

    \Output
        LockerApiDirectoryT *   - pointer to current directory, or NULL if there is none

    \Version 11/24/2004 (jbrookes)
*/
/********************************************************************************F*/
const LockerApiDirectoryT *LockerApiGetDirectory(LockerApiRefT *pLockerApi)
{
    return((pLockerApi->CurrentDir.iMaxFiles > 0) ? &pLockerApi->CurrentDir : NULL);
}

/*F********************************************************************************/
/*!
    \Function LockerApiGetMetaSearch

    \Description
        get the current search result. Appears like directory data

    \Input pLockerApi - locker object ref

    \Version 02/05/2007 (ozietman)
*/ 
/********************************************************************************F*/
const LockerApiDirectoryT *LockerApiGetMetaSearch(LockerApiRefT *pLockerApi)
{
    return((pLockerApi->CurrentSearchReult.iMaxFiles > 0) ? &pLockerApi->CurrentSearchReult : NULL);
}

/*F********************************************************************************/
/*!
    \Function LockerApiSortDirectory

    \Description
        Sort current directory.

    \Input *pLockerApi  - locker object ref
    \Input eCategory    - category to sort by (LOCKERAPI_CATEGORY_*)
    \Input iSortDir     - LOCKERAPI_SORTDIR_ASCENDING or LOCKERAPI_SORTDIR_DESCENDING.

    \Version 1.0 12/01/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void LockerApiSortDirectory(LockerApiRefT *pLockerApi, LockerApiCategoryE eCategory, int32_t iSortDir)
{
    LockerApiDirectoryT *pDirectory = &pLockerApi->CurrentDir;
    int32_t iFileId;
    
    // ref the directory
    if (pDirectory->iNumFiles < 2)
    {
        NetPrintf(("lockerapi: no files to sort\n"));
        return;
    }

    // set variables for sort callback
    pLockerApi->eSortCat = eCategory;
    pLockerApi->iSortDir = iSortDir;
    
    // sort the file list
    LobbyMSort(pLockerApi, pDirectory->Files, pDirectory->iNumFiles, sizeof(pDirectory->Files[0]), _LockerSortFunc);

    // rekey the files
    for (iFileId = 0; iFileId < pLockerApi->CurrentDir.iNumFiles; iFileId++)
    {
        pLockerApi->CurrentDir.Files[iFileId].iId = iFileId;
    }
}

/*F********************************************************************************/
/*!
    \Function LockerApiUpdateFileInfo

    \Description
        Update file attributes.

    \Input *pLockerApi  - locker object ref
    \Input iFileId      - file id of file to update
    \Input *pStrDesc    - new description, or NULL to retain the previous description
    \Input iAttr        - new attributes, or -1 to retain previous attributes
    \Input iPerm        - new permissions, or -1 to retain previous permissions
    \Input *pMetaData   - new metadata for the file, or NULL to not set metadata

    \Version 1.0 11/24/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void LockerApiUpdateFileInfo(LockerApiRefT *pLockerApi, int32_t iFileId, const char *pStrDesc, int32_t iAttr, int32_t iPerm, const char *pMetaData)
{
    const LockerApiFileT *pFile;
    
    // make sure we're idle
    if (pLockerApi->eState != ST_IDLE)
    {
        NetPrintf(("lockerapi: can't update file info while not idle\n"));
        return;
    }
    
    // get file pointer from Id to ensure iFileId is valid
    if ((pFile = _LockerApiGetFileFromId(pLockerApi, iFileId, NULL)) == NULL)
    {
        NetPrintf(("lockerapi: can't update info for file %d\n", iFileId));
        return;
    }

    // save file info
    memcpy(&pLockerApi->CurrentFile, pFile, sizeof(pLockerApi->CurrentFile));
    pLockerApi->iCurFileId = iFileId;

    // make sure the meta data is now zeroed out
    memset(&pLockerApi->strMeta, 0, sizeof(pLockerApi->strMeta));
    // copy meta data if there is data in it 
    if (pMetaData != NULL)
    {
        strnzcpy(pLockerApi->strMeta, pMetaData, sizeof(pLockerApi->strMeta));
    }

    // save the new attribute and permissions flags
    pLockerApi->iCurFileAttr = (iAttr == -1) ? pFile->Info.iAttr : iAttr;
    pLockerApi->CurrentFile.Info.iPermissions = (iPerm == -1) ? pFile->Info.iPermissions : iPerm;

    // clear error state
    pLockerApi->eCurError = LOCKERAPI_ERROR_NONE;

    if (pStrDesc == NULL)
    {
        // empty the description field
        memset(pLockerApi->CurrentFile.Info.strDesc,0,sizeof(pLockerApi->CurrentFile.Info.strDesc));
        // there is no description to filter so send the file update request.
        _LockerApiSendFileInfoUpdate(pLockerApi);
    }
    else
    {
        // save the description string.
        strnzcpy(pLockerApi->CurrentFile.Info.strDesc, pStrDesc, sizeof(pLockerApi->CurrentFile.Info.strDesc));

        // issue the ProtoFilter command
        if (ProtoFilterRequest(pLockerApi->pFilter, pStrDesc, &pLockerApi->uFilterReqId) != PROTOFILTER_ERROR_PENDING)
        {
            NetPrintf(("lockerapi: error issuing profanity filter request\n"));
            return;
        }

        // wait for a response from ProtoFilter.
        pLockerApi->eState = ST_NFO_FILTER;
    }
}

/*F********************************************************************************/
/*!
    \Function LockerApiGetFile

    \Description
        Start a file download (async)

    \Input *pLockerApi  - locker object ref
    \Input iFileId      - file id of file to get

    \Version 1.0 11/24/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void LockerApiGetFile(LockerApiRefT *pLockerApi, int32_t iFileId)
{
    const LockerApiFileT *pFile;
    char strUrl[512];

    // make sure we're idle
    if (pLockerApi->eState != ST_IDLE)
    {
        NetPrintf(("lockerapi: can't get file while not idle\n"));
        return;
    }

    // get file pointer from Id
    if ((pFile = _LockerApiGetFileFromId(pLockerApi, iFileId, NULL)) == NULL)
    {
        NetPrintf(("lockerapi: can't get file %d\n", iFileId));
        return;
    }

    // initialize file info
    memcpy(&pLockerApi->CurrentFile, pFile, sizeof(pLockerApi->CurrentFile));
    pLockerApi->iCurFileId = iFileId;

    // clear error state
    pLockerApi->eCurError = LOCKERAPI_ERROR_NONE;

    // format the URL
    _LockerApiFormatBaseUrl(pLockerApi, strUrl, sizeof(strUrl), "get");
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&pers=", pLockerApi->CurrentDir.strOwner);
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&name=", pFile->Info.strName);
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&type=", pFile->Info.strType);
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&game=", pFile->Info.strGame);
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&idnt=", pFile->Info.strIdnt);
    ProtoHttpUrlEncodeIntParm(strUrl, sizeof(strUrl), "&vers=", pLockerApi->CurrentFile.Info.iVersion);

    // set download amount
    pLockerApi->iRecvSize = pFile->Info.iNetSize;

    #if LOCKERAPI_VERBOSE
    NetPrintf(("lockerapi: starting download of file %s (%d bytes) at %d\n", pFile->Info.strName, pFile->Info.iNetSize, NetTick()));
    #endif

    // issue the request
    if (ProtoHttpGet(pLockerApi->pHttp, strUrl, PROTOHTTP_HEADBODY) < 0)
    {
        NetPrintf(("lockerapi: error initiating get transaction\n"));
        return;
    }
    pLockerApi->eState = ST_GET;
}

/*F********************************************************************************/
/*!
    \Function LockerApiReadData

    \Description
        Read a chunk of data.

    \Input *pLockerApi  - locker object ref
    \Input *pData       - buffer to read into (must be >= iSpoolSize passed to LockerApiCreate())

    \Version 1.0 11/24/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t LockerApiReadData(LockerApiRefT *pLockerApi, void *pData)
{
    // make sure we're in download state
    if (pLockerApi->eState != ST_GET)
    {
        NetPrintf(("lockerapi: can't read data while not downloading\n"));
        return(-1);
    }

    // try reading the data
    return(_LockerApiRecv(pLockerApi, pData, pLockerApi->iSpoolSize));
}

/*F********************************************************************************/
/*!
    \Function LockerApiPutFile

    \Description
        Start a file upload (async).

    \Input *pLockerApi  - locker object ref
    \Input *pFileInfo   - file info
    \Input *pOwner      - locker owner's persona (note: admin use only)
    \Input bOverwrite   - if TRUE, allow overwrite of file
    \Input pMetaData    - pointer to meta data

    \Version 1.0 11/24/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void LockerApiPutFile(LockerApiRefT *pLockerApi, const LockerApiFileInfoT *pFileInfo, const char *pOwner, uint32_t bOverwrite, const char* pMetaData)
{
    int32_t iAttr;

    // make sure we're in idle state
    if (pLockerApi->eState != ST_IDLE)
    {
        NetPrintf(("lockerapi: can't upload file while not idle\n"));
        return;
    }

    // copy to local file descriptor
    memcpy(&pLockerApi->CurrentFile.Info, pFileInfo, sizeof(pLockerApi->CurrentFile.Info));

    // make sure the buffer for meta data is empty
    memset(&pLockerApi->strMeta, 0, sizeof(pLockerApi->strMeta));

    // copy the meta data for this file if any
    if(pMetaData)
    {
        strnzcpy(pLockerApi->strMeta, pMetaData, sizeof(pLockerApi->strMeta));
    }

    // clear error state
    pLockerApi->eCurError = LOCKERAPI_ERROR_NONE;

    // add platform type to attribute if unspecified
    if (((iAttr = pFileInfo->iAttr) & LOCKERAPI_FILEATTR_PLATMASK) == 0)
    {
        iAttr |= LOCKERAPI_FILEATTR_PLATFORM;
    }
    // store the file attributes
    pLockerApi->iCurFileAttr = iAttr;

    // store the owner name (must have admin permission to upload to another user's locker
    if (pOwner != NULL)
    {
        strnzcpy(pLockerApi->strCurFileOwner, pOwner, sizeof(pLockerApi->strCurFileOwner));
    }
    else
    {
        strnzcpy(pLockerApi->strCurFileOwner, pLockerApi->strPersona, sizeof(pLockerApi->strCurFileOwner));
    }

    // store the overwrite flag
    pLockerApi->bCurFileOverwrite = bOverwrite;

    if (pLockerApi->CurrentFile.Info.strDesc[0] == '\0')
    {
        // there is no description to filter so send the put request
        _LockerApiSendPutFile(pLockerApi);
    }
    else
    {
        // issue the ProtoFilter command
        if (PROTOFILTER_ERROR_PENDING != ProtoFilterRequest(pLockerApi->pFilter,pLockerApi->CurrentFile.Info.strDesc,&pLockerApi->uFilterReqId))
        {
            NetPrintf(("lockerapi: error issueing profanity filter request\n"));
            return;
        }

        // wait for a response from ProtoFilter
        pLockerApi->eState = ST_PUT_FILTER;
    }
}

/*F********************************************************************************/
/*!
    \Function LockerApiWriteData

    \Description
        Write a chunk of data.

    \Input *pLockerApi  - locker object ref
    \Input *pData       - pointer to data to write

    \Version 1.0 11/24/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t LockerApiWriteData(LockerApiRefT *pLockerApi, const void *pData)
{
    // make sure we're in upload state
    if (pLockerApi->eState != ST_PUT)
    {
        NetPrintf(("lockerapi: can't write data while not uploading\n"));
        return(-1);
    }

    // try writing the data
    return(_LockerApiSend(pLockerApi, pData, pLockerApi->iSpoolSize));
}

/*F********************************************************************************/
/*!
    \Function LockerApiDeleteFile

    \Description
        Delete a file (async).

    \Input *pLockerApi  - locker object ref
    \Input iFileId      - file id to delete

    \Version 1.0 11/24/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void LockerApiDeleteFile(LockerApiRefT *pLockerApi, int32_t iFileId)
{
    const LockerApiFileT *pFile;
    char strUrl[512];

    // make sure we're in idle state
    if (pLockerApi->eState != ST_IDLE)
    {
        NetPrintf(("lockerapi: can't delete file while not idle\n"));
        return;
    }

    // get file pointer from fileId
    if ((pFile = _LockerApiGetFileFromId(pLockerApi, iFileId, NULL)) == NULL)
    {
        NetPrintf(("lockerapi: can't delete file %d\n", iFileId));
        return;
    }

    // save file info
    memcpy(&pLockerApi->CurrentFile, pFile, sizeof(pLockerApi->CurrentFile));
    pLockerApi->iCurFileId = iFileId;

    // clear error state
    pLockerApi->eCurError = LOCKERAPI_ERROR_NONE;

    // format the URL
    _LockerApiFormatBaseUrl(pLockerApi, strUrl, sizeof(strUrl), "del");
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&name=", pFile->Info.strName);
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&type=", pFile->Info.strType);
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&game=", pFile->Info.strGame);
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&pers=", pLockerApi->CurrentDir.strOwner);
    ProtoHttpUrlEncodeStrParm(strUrl, sizeof(strUrl), "&idnt=", pFile->Info.strIdnt);
    ProtoHttpUrlEncodeIntParm(strUrl, sizeof(strUrl), "&vers=", pLockerApi->CurrentFile.Info.iVersion);

    // issue the request
    if (ProtoHttpPost(pLockerApi->pHttp, strUrl, NULL, 0, PROTOHTTP_POST) < 0)
    {
        NetPrintf(("lockerapi: error initiating del transaction\n"));
        return;
    }
    pLockerApi->eState = ST_DEL;
}

/*F********************************************************************************/
/*!
    \Function LockerApiMetaSearch

    \Description 
        search based on meta data(async)

    \Input pLockerApi - locker object ref
    \Input pMetaData  - meta data to search on it needs to have the & on it

    \Version 02/05/2007 (ozietman)
*/ 
/********************************************************************************F*/
void LockerApiMetaSearch(LockerApiRefT *pLockerApi, const char* pMetaData)
{
    // make sure we're in idle state
    if (pLockerApi->eState != ST_IDLE)
    {
        NetPrintf(("lockerapi: can't search meta data while not idle\n"));
        return;
    }
    // clear error state
    pLockerApi->eCurError = LOCKERAPI_ERROR_NONE;

    // invalidate current directory
    memset(&pLockerApi->CurrentSearchReult, 0, sizeof(pLockerApi->CurrentSearchReult));

    _LockerApiFormatBaseUrl(pLockerApi, pLockerApi->strMeta, sizeof(pLockerApi->strMeta), "src");
    strnzcat(pLockerApi->strMeta, pMetaData, sizeof(pLockerApi->strMeta));
    if (pLockerApi->strGameName[0] != '\0')
    {
        ProtoHttpUrlEncodeStrParm(pLockerApi->strMeta, sizeof(pLockerApi->strMeta), "&game=", pLockerApi->strGameName);
    }

    // issue the request
    if (ProtoHttpGet(pLockerApi->pHttp, pLockerApi->strMeta, PROTOHTTP_HEADBODY) < 0)
    {
        NetPrintf(("lockerapi: error initiating search meta transaction\n"));
        return;
    }

    pLockerApi->eState = ST_SEARCH_META;
}

/*F********************************************************************************/
/*!
    \Function LockerApiAbort

    \Description
        Abort current operation, if any.

    \Input *pLockerApi  - locker object ref

    \Version 12/01/2004 (jbrookes)
*/
/********************************************************************************F*/
void LockerApiAbort(LockerApiRefT *pLockerApi)
{
    // abort current filter transactions if necessary
    if (pLockerApi->eState == ST_PUT_FILTER || pLockerApi->eState == ST_NFO_FILTER)
    {
        ProtoFilterRequestCancel(pLockerApi->pFilter, pLockerApi->uFilterReqId);
    }

    // abort current transaction, if any
    ProtoHttpAbort(pLockerApi->pHttp);

    // reset state
    _LockerApiReset(pLockerApi);
}


/*F********************************************************************************/
/*!
    \Function LockerApiGetLastError

    \Description
        Get most recent error.

    \Input *pLockerApi  - locker object ref

    \Output
        LockerApiErrorE - most recently occurring error

    \Version 1.0 11/24/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
LockerApiErrorE LockerApiGetLastError(LockerApiRefT *pLockerApi)
{
    return(pLockerApi->eCurError);
}

/*F********************************************************************************/
/*!
    \Function LockerApiControl

    \Description
        LockerApi control function.  Different selectors control different behaviors.

    \Input *pLockerApi  - locker object ref
    \Input iControl     - control selector
    \Input iValue       - selector-specific
    \Input iValue2      - selector-specific
    \Input *pValue      - selector-specific
    
    \Output
        int32_t         - selector-specific

    \Notes
        Selectors are:

    \verbatim
        'filt'  - set protofilter filtering type to apply (iValue=type)
        'game'  - set (optional) gamename used in put operations
    \endverbatim

    \Version 1.0 06/12/2007 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t LockerApiControl(LockerApiRefT *pLockerApi, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    // set protofilter filtering type to apply
    if (iControl == 'filt')
    {
        pLockerApi->uFilterType = (unsigned)iValue;
        return(0);
    }
    if ((iControl == 'game') && (pValue != NULL))
    {
        strnzcpy(pLockerApi->strGameName, pValue, (int32_t)strlen(pLockerApi->strGameName));
        return(0);
    }
    // unhandled
    return(-1);
}
