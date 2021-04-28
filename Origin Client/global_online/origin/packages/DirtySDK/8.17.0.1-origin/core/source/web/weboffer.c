/*H*******************************************************************/
/*!
    \File weboffer.c

    \Description
        This module implements a simple web driven dynamic offer system.
        It downloads a simple script from a web server and uses that to
        display alerts and data entry screens.

    \Copyright
        Copyright (c) Electronic Arts 2004-2005. ALL RIGHTS RESERVED.

    \Version 1.0 03/13/2004 gschaefer
    \Version 2.0 01/11/2005 gschaefer (updated script commands)
*/
/*******************************************************************H*/

/*** Include files ***************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dirtysock.h"
#include "dirtymem.h"
#include "lobbytagfield.h"
#include "protohttp.h"
#include "lobbylang.h"

#include "weboffer.h"

/*** Defines *********************************************************/

#define ALLOW_COLORED_BUTTONS   0
#define WEBOFFER_GOTO_LIMIT     5
#define WEBOFFER_CMD_GOTO       ('goto')    //!< internal command only

#define WEBOFFER_TAGNAME_BUTTON     ("BTN")         //!< this is the tag used for the item in a button list
#define WEBOFFER_TAGNAME_ARTICLE    ("ARTICLE")     //!< this is the tag used for the item in an article list
#define WEBOFFER_TAGNAME_FIXED_IMAGE ("FIXEDIMAGE")  //!< this is the tag used for the item in an auxillary image list

/*** Type Definitions ************************************************/

//! module state
struct WebOfferT
{
    char *pScript;                  //!< script buffer
    int32_t iScript;                //!< size of buffer
    char strResult[256];            //!< script result
    char *pOwner;                   //!< owner memory

    // module memory group
    int32_t iMemGroup;              //!< module mem group id
    void *pMemGroupUserData;        //!< user data associated with mem group

    char strSetup[512];             //!< setup parameters
    WebOfferCommandT Command;       //!< current command record

    char *pExec;                    //!< current execution point
    char *pCmdHead;                 //!< head of current record
    char *pCmdTail;                 //!< tail of current record
    char *pNewsHead;                //!< start of news data
    char *pNewsTail;                //!< end of news data

    ProtoHttpRefT *pHttps;          //!< https module state
    char *pHttpData;                //!< where to append http data
    int32_t iHttpSize;              //!< length of append
    char *pHttpIncl;                //!< http include header
    char *pHttpRsrc;                //!< http resource header
    uint32_t uHttpTime;             //!< timeout for http
    uint32_t uDispTime;             //!< minimum display time
    enum { ST_IDLE, ST_FAKE, ST_SENT, ST_RECVD, ST_DONE, ST_FAIL } eState;
};

/*** Variables ********************************************************************/

const char *_WebOffer_ActionGoto1[] = { "", 
    "BTN1-GOTO", 
    "BTN2-GOTO", 
    "BTN3-GOTO", 
    "BTN4-GOTO", 
    "BTN5-GOTO",  
    "BTN6-GOTO",  
    "BTN7-GOTO",  
    "BTN8-GOTO",
    "ARTICLE0-GOTO", 
    "ARTICLE1-GOTO", 
    "ARTICLE2-GOTO", 
    "ARTICLE3-GOTO", 
    "ARTICLE4-GOTO",  
    "ARTICLE5-GOTO",  
    "ARTICLE6-GOTO",  
    "ARTICLE7-GOTO",
    "ARTICLE8-GOTO",
    "ARTICLE9-GOTO",
    "ARTICLE10-GOTO",
    "ARTICLE11-GOTO",
    "ARTICLE12-GOTO",
    "ARTICLE13-GOTO",
    "ARTICLE14-GOTO",
    "ARTICLE15-GOTO"
};

/*** Private Functions ***********************************************/

// extract a color component
static void _WebOfferGetColor(WebOfferT *pOffer, WebOfferColorT *pColor, const char *pTag)
{
    uint32_t uColor;

    // setup the color
    if(TagFieldFind(pOffer->Command.strParms, pTag))
    {
        uColor = TagFieldGetAddress(TagFieldFind(pOffer->Command.strParms, pTag), 0);
    }
    else
    {
        uColor = TagFieldGetNumber64(TagFieldFind(pOffer->strSetup, "COLOR"), 0xffff0000);
    }

    pColor->uAlpha = (unsigned char)(uColor>>24);
    pColor->uRed = (unsigned char)(uColor>>16);
    pColor->uGreen = (unsigned char)(uColor>>8);
    pColor->uBlue = (unsigned char)(uColor>>0);

    // handle default alpha
    if ((uColor != 0) && (pColor->uAlpha == 0))
    {
        pColor->uAlpha = 255;
    }
}

// setup a button from a tagfield
static void _WebOfferGetButton(WebOfferT *pOffer, const char *pTagName, WebOfferButtonT *pButton, int32_t iItem)
{
    char strFind[32];
    char strButton[64];

    // get the button
    snzprintf(strFind, sizeof(strFind), "%s%d", pTagName, iItem);
    TagFieldGetString(TagFieldFind(pOffer->Command.strParms, strFind), strButton, sizeof(strButton), "");

    // make sure there is a button
    if (strButton[0] != 0)
    {
        // assign the color
        #if ALLOW_COLORED_BUTTONS
        {
            snzprintf(strFind, sizeof(strFind), "%s%d-RGB", pTagName, iItem);
            _WebOfferGetColor(pOffer, &pButton->Color, strFind);
        }
        #endif

        // see if its a submit buffer
        if (strButton[0] == '^')
        {
            // setup default type
            *(pButton->strType) = WEBOFFER_TYPE_SUBMIT;
            TagFieldDupl(pButton->strText, sizeof(pButton->strText), strButton+1);
        }
        else
        {
            // setup default type
            *(pButton->strType) = WEBOFFER_TYPE_NORMAL;
            TagFieldDupl(pButton->strText, sizeof(pButton->strText), strButton);
        }
    }
}

// find a named record
static char *_WebOfferFind(WebOfferT *pOffer, char *pMatch, const char *pFind)
{
    int32_t iSkip;
    int32_t iNext = 0;
    char *pBogus;
    char *pLast = NULL;
    char *pResult = NULL;

    // search for matching label
    while (pMatch != NULL)
    {
        // find section start
        pLast = pMatch;
        pMatch = strstr(pMatch, "%{");
        if (pMatch == NULL)
        {
            pMatch = pLast+strlen(pLast);
        }

        // see if they are searching for next
        if (iNext && (stricmp(pFind, "$next") == 0))
        {
            pResult = pMatch;
            break;
        }

        // see if we ran out of data
        if (*pMatch == 0)
        {
            break;
        }

        // see if they are searching for head
        if (stricmp(pFind, "$head") == 0)
        {
            pResult = pMatch;
            break;
        }

        // see if searching for include header
        if ((stricmp(pFind, "$incl") == 0) && (pMatch[2] == '*'))
        {
            pResult = pMatch;
            break;
        }

        // check for name match
        if ((stristr(pMatch, pFind) == pMatch+2) && ((pMatch+2)[strlen(pFind)] <= ' '))
        {
            pResult = pMatch;
            break;
        }

        // find section end
        pLast = pMatch;
        pMatch = strstr(pMatch, "%}");
        if (pMatch == NULL)
        {
            pMatch = pLast+strlen(pLast);
        }

        // see if they are looking for the tail
        if (stricmp(pFind, "$tail") == 0)
        {
            pResult = pMatch;
            break;
        }

        // advance to next special token
        pLast = pMatch;
        pMatch = strchr(pMatch+1, '%');
        if (pMatch == NULL)
        {
            pMatch = pLast+strlen(pLast);
        }

        // handle binary data marker
        if (pMatch[1] == '$')
        {
            if (pMatch[10] != ':')
            {
                break;
            }

            // get binary data length and ensure its within buffer
            iSkip = strtol(pMatch+2, &pBogus, 16);
            iSkip += 12;
            if (pMatch+iSkip >= pOffer->pScript+pOffer->iScript)
            {
                break;
            }

            // if caller wants a resource, they found it
            if (stricmp(pFind, "$rsrc") == 0)
            {
                pResult = pMatch;
                break;
            }

            // skip past binary record
            pMatch += iSkip;
        }

        // have passed first record
        iNext = 1;
        if (stricmp(pFind, "$rsrc") == 0)
        {
            break;
        }
    }

    // see if they want to find the end
    if (stricmp(pFind, "$last") == 0)
    {
        // we must be at end of last record -- point to terminator
        pResult = pMatch;
    }

    return(pResult);
}

// do the actual goto transition
static void _WebOfferActionGoto(WebOfferT *pOffer, const char *pTarget)
{
    // handle http link special
    if (stricmp(pTarget, "$http") == 0)
    {
        pOffer->pExec = pOffer->pHttpData;
    }
    // handle http-clear link special
    else if (stricmp(pTarget, "$home") == 0)
    {
        if (pOffer->pHttpData != NULL)
        {
            pOffer->iHttpSize = _WebOfferFind(pOffer, pOffer->pHttpData, "$last")-pOffer->pHttpData;
            memmove(pOffer->pScript, pOffer->pHttpData, pOffer->iHttpSize+1);
        }
        pOffer->pHttpData = NULL;
        // look for default startup link
        pOffer->pExec = _WebOfferFind(pOffer, pOffer->pScript, "startup");
        if (pOffer->pExec == NULL)
        {
    		pOffer->pExec = pOffer->pScript;
        }
    }
    // handle quit special
    else if (stricmp(pTarget, "$quit") == 0)
    {
        pOffer->pExec = NULL;
    }
    // exit is like quit but allows a return code
    else if (strncmp(pTarget, "$exit=", 6) == 0)
    {
        pOffer->pExec = NULL;
        TagFieldDupl(pOffer->strResult, sizeof(pOffer->strResult), pTarget+6);
    }
    // allow next line to be specified (no operation)
    else if (stricmp(pTarget, "$next") == 0)
    {
    }
    // if a goto was included then do it
    else if (pTarget[0] != 0)
    {
        pOffer->pExec = _WebOfferFind(pOffer, pOffer->pScript, pTarget);
    }
}

// handle clear action
static void _WebOfferActionClear(WebOfferT *pOffer, const char *pClear)
{
    char *pFind = NULL;
    char *pNext = NULL;
    char *pLast;

    // let them clear all non-http data
    if (stricmp(pClear, "$all") == 0)
    {
        if (pOffer->pHttpData != NULL)
        {
            memmove(pOffer->pScript, pOffer->pHttpData, pOffer->iHttpSize+1);
            pOffer->pHttpData = NULL;
		    pOffer->pExec = pOffer->pScript;
        }
    }
    else
    {
        pFind = _WebOfferFind(pOffer, pOffer->pScript, pClear);
        pNext = _WebOfferFind(pOffer, pFind, "$next");
        pLast = _WebOfferFind(pOffer, pNext, "$last");

        // make sure all targets were valid
        if ((pFind != NULL) && (pNext != NULL) && (pLast != NULL))
        {
            memmove(pFind, pNext, pLast-pNext+1);
            pOffer->pHttpData = NULL;

            // adjust the execution point if needed
            if (pOffer->pExec > pFind)
            {
                if (pOffer->pExec < pNext)
                {
                    pOffer->pExec = pFind;
                }
                else
                {
                    pOffer->pExec -= (pNext-pFind);
                }
            }
        }
    }
}

// handle exit action
static void _WebOfferActionExit(WebOfferT *pOffer, const char *pParm)
{
    // setup result string
    TagFieldDupl(pOffer->strResult, sizeof(pOffer->strResult), pParm);
    pOffer->pExec = NULL;
}

// handle web-offer action
// note: pAction is modified
static void _WebOfferAction(WebOfferT *pOffer, char *pAction)
{
    char *pNext;
    char *pParm;

    // if no semi, this must be a legacy goto
    if (strchr(pAction, ';') == NULL)
    {
        _WebOfferActionGoto(pOffer, pAction);
        return;
    }

    // parse out the individual commands
    for (pNext = pAction; *pNext != 0; ++pNext)
    {
        if (*pNext == ';')
        {
            // terminate command end
            *pNext = 0;

            // skip past any leading whitespace
            while (*pAction <= ' ')
            {
                ++pAction;
            }

            // convert command to uppercase / find parmeter start
            for (pParm = pAction; (pParm != pNext) && (*pParm > ' '); ++pParm)
                ;
            *pParm = 0;

            // skip whitespace to parm start
            while ((pParm != pNext) && (*pParm <= ' '))
            {
                ++pParm;
            }

            // dispatch to command
            if (stricmp(pAction, "goto") == 0)
            {
                _WebOfferActionGoto(pOffer, pParm);
            }
            else if (stricmp(pAction, "clear") == 0)
            {
                _WebOfferActionClear(pOffer, pParm);
            }
            else if (stricmp(pAction, "exit") == 0)
            {
                _WebOfferActionExit(pOffer, pParm);
            }
            pAction = pNext+1;
        }
    }
}

// handle goto processing
static void _WebOfferGoto(WebOfferT *pOffer, const char *pGoto1, const char *pGoto2, const char *pGoto3)
{
    char strAction[1024];
    WebOfferCommandT *pCmd = &pOffer->Command;

    // setup search string
    TagFieldGetString(TagFieldFind(pCmd->strParms, pGoto3), strAction, sizeof(strAction), "");
    TagFieldGetString(TagFieldFind(pCmd->strParms, pGoto2), strAction, sizeof(strAction), strAction);
    TagFieldGetString(TagFieldFind(pCmd->strParms, pGoto1), strAction, sizeof(strAction), strAction);
    // execute the action
    _WebOfferAction(pOffer, strAction);
}

// setup the url
static void _WebOfferUrl(char *pUrl, int32_t iUrl, const char *pVars)
{
    int32_t iSize;
    char *pHead;
    char *pTail;
    char *pLast;
    char strFind[128];
    char strData[128];

    // do substitution within URL
    while ((pHead = strchr(pUrl, '$')) != NULL)
    {
        if ((pTail = strchr(pHead+1, '$')) == NULL)
        {
            break;
        }

        // extract the tag name to substitute from URL
        iSize = pTail-(pHead+1);
        strsubzcpy(strFind, sizeof(strFind), pHead+1, iSize);

        // get tag data and encode into URL form
        TagFieldGetString(TagFieldFind(pVars, strFind), strData, sizeof(strData), "");
        strFind[0] = '\0';
        ProtoHttpUrlEncodeStrParm(strFind, sizeof(strFind), "", strData);
        iSize = (int32_t)strlen(strFind);
      
        // make sure resulting string does not overflow URL buffer
        for (pLast = pTail; *pLast != '\0'; ++pLast)
            ;
        if ((pLast-pUrl)+iSize >= iUrl)
        {
            break;
        }

        // insert into buffer, removing $ markers in process
        memmove(pHead+iSize, pTail+1, pLast-pTail);
        memcpy(pHead, strFind, iSize);
    }
}

static void _parseTelemetryData(WebOfferT *pOffer)
{
    char strTelemetry[64];
    // first thing make it zero
    memset(&pOffer->Command.Telemetry, 0, sizeof(pOffer->Command.Telemetry));

    TagFieldGetString(TagFieldFind(pOffer->Command.strParms, "TELE"), strTelemetry, sizeof(strTelemetry), "");

    // in the minimum there should be the 2 tokens 
    if(strlen(strTelemetry) >= 8)
    {
        pOffer->Command.Telemetry.uTokenModule = (strTelemetry[0] << 24);
        pOffer->Command.Telemetry.uTokenModule += (strTelemetry[1] << 16);
        pOffer->Command.Telemetry.uTokenModule += (strTelemetry[2] << 8);
        pOffer->Command.Telemetry.uTokenModule += (strTelemetry[3] );

        pOffer->Command.Telemetry.uTokenGroup = (strTelemetry[4] << 24);
        pOffer->Command.Telemetry.uTokenGroup += (strTelemetry[5] << 16);
        pOffer->Command.Telemetry.uTokenGroup += (strTelemetry[6] << 8);
        pOffer->Command.Telemetry.uTokenGroup += (strTelemetry[7] );

        // there is some text to add 
        if(strTelemetry[8] != 0)
        {
            strncpy(pOffer->Command.Telemetry.strTelemetry, (strTelemetry + 8), sizeof(pOffer->Command.Telemetry.strTelemetry));
            pOffer->Command.Telemetry.strTelemetry[sizeof(pOffer->Command.Telemetry.strTelemetry) - 1] = 0;
        }
        
    }
}

/*F********************************************************************************/
/*!
    \Function   _WebOfferGetFixedImage

    \Description
        To be completed

    \Input pOffer       - offer state
    \Input pTagName     - Tag name to find
    \Input pFixedImage  - The picture to show 
    \Input iItem    - item ID

    \Version 09/20/2006 (ozietman)
*/ 
/********************************************************************************F*/
static void _WebOfferGetFixedImage(WebOfferT *pOffer, const char *pTagName, WebOfferFixedImageT *pFixedImage, int32_t iItem)
{
    char strFind[32];
    char strImage[64];

    // get the picture
    snzprintf(strFind, sizeof(strFind), "%s%d", pTagName, iItem);
    TagFieldGetString(TagFieldFind(pOffer->Command.strParms, strFind), strImage, sizeof(strImage), "");

    // make sure there is a picture
    if (strImage[0] != 0)
    {
        TagFieldDupl(pFixedImage->strImage, sizeof(pFixedImage->strImage), strImage);
    }
}    

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function WebOfferCreate2

    \Description
        Create a new web-offer module instance using passed in memory.  Note that
        caller must make sure the buffer is valid until WebOfferDestroy is called.
        Recommand buffer is at least WEBOFFER_SUGGESTED_MEM bytes (based on typical
        usage).

    \Input *pBuffer - pointer to caller supplied buffer, or NULL
    \Input iLength  - length of caller supplied buffer, or size of buffer
                     to allocate if pBuffer==NULL

    \Output WebOfferT reference pointer -- pass as input to all other functions

    \Version 03/13/2004 gschaefer
*/
/********************************************************************************F*/
WebOfferT *WebOfferCreate(char *pBuffer, int32_t iLength)
{
    WebOfferT *pOffer = NULL;
    void *pOwner = NULL;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate the buffer, if user passed NULL
    if (pBuffer == NULL)
    {
        iLength += sizeof(*pOffer);
        pOwner = pBuffer = DirtyMemAlloc(iLength, WEBOFFER_MEMID, iMemGroup, pMemGroupUserData);
    }

    // just in case-- keep size within reason
    if (iLength > 10000000)
    {
        iLength = 10000000;
    }

    // make sure size is reasonable
    if (iLength > (int32_t)sizeof(*pOffer)+256)
    {
        // allocate module state
        pOffer = (WebOfferT *)pBuffer;
        memset(pOffer, 0, sizeof(*pOffer));
        // setup the script buffer
        pOffer->pScript = pBuffer+sizeof(*pOffer);
        pOffer->iScript = iLength-sizeof(*pOffer);
        // set memory group
        pOffer->iMemGroup = iMemGroup;
        pOffer->pMemGroupUserData = pMemGroupUserData;
        // go to idle state
        pOffer->eState = ST_IDLE;
        // keep https module ready for life of module
        pOffer->pHttps = ProtoHttpCreate(8192);
        if (pOffer->pHttps == NULL)
        {
            WebOfferDestroy(pOffer);
            pOffer = NULL;
        }
        else
        {
            pOffer->pOwner = pOwner;
        }
    }
    else if (pOwner != NULL)
    {
        DirtyMemFree(pOwner, WEBOFFER_MEMID, iMemGroup, pMemGroupUserData);
    }
    
    // reference for other modules
    return(pOffer);
}

/*F*******************************************************************/
/*!
    \Function WebOfferDestroy

    \Description
        Destroy the web-offer module

    \Input *pOffer - the module state from WebOfferCreation

    \Output NULL - to allow inline destroy/variable clear

    \Version 03/13/2004 gschaefer
*/
/*******************************************************************F*/
WebOfferT *WebOfferDestroy(WebOfferT *pOffer)
{
    // make sure offer state is valid
    if (pOffer != NULL)
    {
        // destroy https module if present
        if (pOffer->pHttps != NULL)
        {
            ProtoHttpDestroy(pOffer->pHttps);
        }
        // destroy state buffer if we own it
        if (pOffer->pOwner != NULL)
        {
            DirtyMemFree(pOffer->pOwner, WEBOFFER_MEMID, pOffer->iMemGroup, pOffer->pMemGroupUserData);
        }
    }

    // return NULL to allow inline destroy/variable clear
    return(NULL);
}

/*F*******************************************************************/
/*!
    \Function WebOfferClear

    \Description
        Clear the script and reset to idle

    \Input *pOffer - module state from WebOfferCreate

    \Version 03/13/2004 gschaefer
*/
/*******************************************************************F*/
void WebOfferClear(WebOfferT *pOffer)
{
    pOffer->pScript[0] = 0;
    pOffer->pExec = NULL;
}

/*F*******************************************************************/
/*!
    \Function WebOfferSetup

    \Description
        Setup information about the user/product

    \Input *pOffer - module state from WebOfferCreate
    \Input *pSetup - script to execute

    \Version 03/13/2004 gschaefer
*/
/*******************************************************************F*/
void WebOfferSetup(WebOfferT *pOffer, WebOfferSetupT *pSetup)
{
    // copy over user provided data
    TagFieldClear(pOffer->strSetup, sizeof(pOffer->strSetup));
    TagFieldSetString(pOffer->strSetup, sizeof(pOffer->strSetup), "PERS", pSetup->strPersona);
    TagFieldSetString(pOffer->strSetup, sizeof(pOffer->strSetup), "FAVT", pSetup->strFavTeam);
    TagFieldSetString(pOffer->strSetup, sizeof(pOffer->strSetup), "FAVP", pSetup->strFavPlyr);
    TagFieldSetString(pOffer->strSetup, sizeof(pOffer->strSetup), "PROD", pSetup->strProduct);
    TagFieldSetString(pOffer->strSetup, sizeof(pOffer->strSetup), "PLAT", pSetup->strPlatform);
    TagFieldSetString(pOffer->strSetup, sizeof(pOffer->strSetup), "PRIV", pSetup->strPrivileges);
    TagFieldSetString(pOffer->strSetup, sizeof(pOffer->strSetup), "LANG", pSetup->strLanguage);
    TagFieldSetString(pOffer->strSetup, sizeof(pOffer->strSetup), "FROM", pSetup->strCountry);
	TagFieldSetString(pOffer->strSetup, sizeof(pOffer->strSetup), "REGN", pSetup->strGameRegion);
    TagFieldSetString(pOffer->strSetup, sizeof(pOffer->strSetup), "SLUS", pSetup->strSlusCode);
    TagFieldSetString(pOffer->strSetup, sizeof(pOffer->strSetup), "LKEY", pSetup->strLKey);
    TagFieldSetString(pOffer->strSetup, sizeof(pOffer->strSetup), "SKU", pSetup->strSkuCode);
    TagFieldSetNumber64(pOffer->strSetup, sizeof(pOffer->strSetup), "COLOR", pSetup->uColor);
    TagFieldSetString(pOffer->strSetup, sizeof(pOffer->strSetup), "UID", pSetup->strUID);
    TagFieldSetNumber(pOffer->strSetup, sizeof(pOffer->strSetup), "MEM", pSetup->uMemSize);
}

/*F*******************************************************************/
/*!
    \Function WebOfferExecute

    \Description
        Execute an offer script

    \Input *pOffer - module state from WebOfferCreate
    \Input *pScript - script to execute

    \Version 03/13/2004 gschaefer
*/
/*******************************************************************F*/
void WebOfferExecute(WebOfferT *pOffer, const char *pScript)
{
    int32_t iCount;

    // clear any existing script
    WebOfferClear(pOffer);

    // handle direct web reference special
    if ((strncmp(pScript, "http://", 7) == 0) || (strncmp(pScript, "https://", 8) == 0))
    {
        sprintf(pOffer->pScript, "%%{ CMD=http SUCCESS-GOTO=\"$home\" URL-GET=%s\n%%}", pScript);
        pScript = pOffer->pScript;
    }
    // if its got web-includes, force it through an http cycle
    else if (stristr(pScript, "%{*") != NULL)
    {
        iCount = sprintf(pOffer->pScript, "%%{ CMD=http SUCCESS-GOTO=\"$home\" URL-FAKE=1\n%%}");
        strncpy(pOffer->pScript+iCount+1, pScript, pOffer->iScript-iCount-1);
        pOffer->pScript[pOffer->iScript-1] = 0;
    }
    // else run the script direct
    else if (strlen(pScript) < (unsigned)pOffer->iScript)
    {
        strcpy(pOffer->pScript, pScript);
    }

    // look for optional startup tag
    pOffer->pExec = _WebOfferFind(pOffer, pOffer->pScript, "startup");
    // if no startup tag, execute from first block
    if (pOffer->pExec == NULL)
    {
        pOffer->pExec = pOffer->pScript;
    }

    // reset the result code
    pOffer->strResult[0] = 0;
}

/*F*******************************************************************/
/*!
    \Function WebOfferResultData

    \Description
        Return the result of an offer script

    \Input *pOffer  - module state from WebOfferCreate
    \Input *pBuffer - pointer to return buffer for string values (NULL=ignore)
    \Input  iLength - length of string buffer if supplied

    \Output Script dependent value (0 if not set by script, else atoi of exit string)

    \Version 01/13/2004 gschaefer
*/
/*******************************************************************F*/
int32_t WebOfferResultData(WebOfferT *pOffer, char *pBuffer, int32_t iLength)
{
    if (pBuffer != NULL)
    {
        TagFieldDupl(pBuffer, iLength, pOffer->strResult);
    }
    return(atoi(pOffer->strResult));
}

/*F*******************************************************************/
/*!
    \Function WebOfferCommand

    \Description
        Return the next command from the script

    \Input *pOffer - module state from WebOfferCreate

    \Version 03/13/2004 gschaefer
*/
/*******************************************************************F*/
WebOfferCommandT *WebOfferCommand(WebOfferT *pOffer)
{
    int32_t iSize;
    int32_t iLimit;
    char *pNews;
    WebOfferCommandT *pCmd = &pOffer->Command;

    // if no module state, return exit code
    if (pOffer == NULL)
    {
        return(NULL);
    }

    // make sure they don't exceed goto limit
    for (iLimit = 0; iLimit < WEBOFFER_GOTO_LIMIT; ++iLimit)
    {
        // see if in quit mode
        if (pOffer->pExec == NULL)
        {
            return(NULL);
        }

        // find start/end of next command
        pOffer->pCmdHead = _WebOfferFind(pOffer, pOffer->pExec, "$head");
        pOffer->pCmdTail = _WebOfferFind(pOffer, pOffer->pCmdHead, "$tail");
        if ((pOffer->pCmdHead == NULL) || (pOffer->pCmdTail == NULL))
        {
            return(NULL);
        }

        // dont include %{ %} in buffer
        pOffer->pCmdHead += 2;
        // make sure it will fit in buffer
        iSize = pOffer->pCmdTail - pOffer->pCmdHead;
        if ((iSize <= 0) || (iSize >= (signed)sizeof(pCmd->strParms)))
        {
            return(NULL);
        }

        // copy to command buffer
        memcpy(pCmd->strParms, pOffer->pCmdHead, iSize);
        pCmd->strParms[iSize] = 0;

        _parseTelemetryData(pOffer);

        // get the command
        pCmd->iCommand = TagFieldGetToken(TagFieldFind(pCmd->strParms, "CMD"), 0);
	    if (pCmd->iCommand == 0)
	    {
		    return(NULL);
	    }

        // move execution pointer ot next command by default
        pOffer->pExec = _WebOfferFind(pOffer, pOffer->pCmdHead-2, "$next");

        // figure out news block if present
        for (pNews = pOffer->pCmdTail+2; (*pNews > 0) && (*pNews <= ' '); ++pNews)
            ;
        pOffer->pNewsHead = pNews;
        pOffer->pNewsTail = pOffer->pExec;

        // see if there is news
        if ((pOffer->pNewsHead != pOffer->pNewsTail) && (pOffer->pNewsTail != NULL))
        {
            // terminate the news data (must replace % character later)
            pOffer->pNewsTail[0] = 0;
        }
        else
        {
            // there is no news data
            pOffer->pNewsHead = pOffer->pNewsTail = NULL;
        }

        // any non-goto command is passed to application
        if (pCmd->iCommand != WEBOFFER_CMD_GOTO)
        {
            // return current command to application
            return(pCmd);
        }

        // handle goto/action sequence
        _WebOfferGoto(pOffer, "GOTO", "", "DEFAULT-GOTO");
    }

    // we looped too many times -- return failure
    return(NULL);
}

/*F*******************************************************************/
/*!
    \Function WebOfferParamList

    \Description
        Extract an item from a parameter list

    \Input *pOffer  - module state from WebOfferCreate
    \Input *pBuffer - pointer to return buffer
    \Input  iLength - length of string buffer
    \Input  pParam  - param list pointer from command structure
    \Input  iIndex  - index into list

    \Output negative=invalid index, 0=success

    \Version 03/25/2005 gschaefer
*/
/*******************************************************************F*/
int32_t WebOfferParamList(WebOfferT *pOffer, char *pBuffer, int32_t iLength, const char *pParam, int32_t iIndex)
{
    int32_t iResult;

    iResult = TagFieldGetDelim(pParam, pBuffer, iLength, NULL, iIndex, ',');
    return(iResult);
}

/*F*******************************************************************/
/*!
    \Function WebOfferAction

    \Description
        Report user input from last command

    \Input *pOffer - module state from WebOfferCreate
    \Input iAction - action

    \Version 03/13/2004 gschaefer
*/
/*******************************************************************F*/
void WebOfferAction(WebOfferT *pOffer, int32_t iAction)
{
    int32_t iSize;
    int32_t iOldSize;
    int32_t iNewSize;

    // handle abort special
    if ((iAction <= WEBOFFER_ACTION_ABORT) || (iAction >= WEBOFFER_ACTION_LAST))
    {
        WebOfferClear(pOffer);
        return;
    }

    // save the button press info
    TagFieldSetNumber(pOffer->Command.strParms, sizeof(pOffer->Command.strParms), "ACTION", iAction);

    // if news, then restore 
    if (pOffer->pNewsHead != pOffer->pNewsTail)
    {
        pOffer->pNewsTail[0] = '%';
        pOffer->pNewsHead = NULL;
        pOffer->pNewsTail = NULL;
    }

    // calc size of old and new command blocks
    iOldSize = pOffer->pCmdTail - pOffer->pCmdHead;
    iNewSize = (int32_t)strlen(pOffer->Command.strParms);

    // see if this will fit in script
    iSize = _WebOfferFind(pOffer, pOffer->pScript, "$last")-pOffer->pScript+1;
    if (iSize-iOldSize+iNewSize > pOffer->iScript)
    {
        WebOfferClear(pOffer);
        return;
    }

    // adjust size and copy back command
    memmove(pOffer->pCmdHead+iNewSize, pOffer->pCmdTail, (pOffer->pScript+iSize)-pOffer->pCmdTail);
    memcpy(pOffer->pCmdHead, pOffer->Command.strParms, iNewSize);

    // handle goto
    _WebOfferGoto(pOffer, _WebOffer_ActionGoto1[iAction], "", "DEFAULT-GOTO");
}

/*F*******************************************************************/
/*!
    \Function WebOfferHttp

    \Description
        Transfer data to web server

    \Input *pOffer - module state from WebOfferCreate

    \Version 03/13/2004 gschaefer
*/
/*******************************************************************F*/
void WebOfferHttp(WebOfferT *pOffer)
{
    int32_t iTime;
    int32_t iIndex;
    char *pHead;
    char *pTail;
    char strUrl[2048];
    char strFind[64];
    char strData[256];
    WebOfferCommandT *pCmd = &pOffer->Command;

    // make sure in HTTP mode else fatal error
    if (pCmd->iCommand != WEBOFFER_CMD_HTTP)
    {
        WebOfferClear(pOffer);
        return;
    }

    // set default values in case setup fails
    // (these get reset later if all goes well)
    pOffer->eState = ST_FAIL;
    pOffer->uDispTime = NetTick()+0000;
    pOffer->uHttpTime = NetTick()+1000;

    // setup target buffer
    pOffer->pHttpData = _WebOfferFind(pOffer, pOffer->pScript, "$last")+1;
    pOffer->iHttpSize = (pOffer->pScript+pOffer->iScript)-pOffer->pHttpData-1;
    pOffer->pHttpIncl = NULL;
    if (pOffer->iHttpSize < 256)
    {
        WebOfferClear(pOffer);
        return;
    }

    // if this is a fake access (data passed in with execute), all done
    if (TagFieldGetNumber(TagFieldFind(pCmd->strParms, "URL-FAKE"), 0) > 0)
    {
        pOffer->eState = ST_FAKE;
        return;
    }

    // empty the download buffer
    pOffer->pHttpData[0] = 0;

    // merge setup data into parms
    TagFieldMerge(pCmd->strParms, sizeof(pCmd->strParms), pOffer->strSetup);
    TagFieldSetNumber(pCmd->strParms, sizeof(pCmd->strParms), "MAXLEN", pOffer->iHttpSize);

    // see if there are additional merge record (like credit card)
    while (TagFieldGetString(TagFieldFind(pCmd->strParms, "INCL"), strFind, sizeof(strFind), "") > 0)
    {
        pHead = _WebOfferFind(pOffer, pOffer->pScript, strFind);
        pTail = _WebOfferFind(pOffer, pHead, "$tail");
        if ((pHead == NULL) || (pTail == NULL))
        {
            break;
        }

        // temp terminate record during merge than restore
        *pTail = 0;
        TagFieldMerge(pCmd->strParms, sizeof(pCmd->strParms), pHead+2+strlen(strFind));
        *pTail = '%';
        break;
    }

    // see if there are special record variables that should be included
    for (iIndex = 1; TagFieldGetString(TagFieldFindIdx(pCmd->strParms, "PARM", iIndex), strFind, sizeof(strFind), "") > 0; ++iIndex)
    {
        // divide the record part
        pTail = strchr(strFind, '.');
        if (pTail == NULL)
            break;
        *pTail = 0;

        // locate start of record
        pHead = _WebOfferFind(pOffer, pOffer->pScript, strFind);
        if (pHead != NULL)
        {
            // grab the data
            TagFieldGetString(TagFieldFind(pHead+2+strlen(strFind), pTail+1), strData, sizeof(strData), "");
            // set the data using PARM identifier as name (record.field)
            *pTail = '.';
            TagFieldSetString(pCmd->strParms, sizeof(pCmd->strParms), strFind, strData);
        }
    }
    // get the URL (note: we assume URLs are properly encoded by the caller, although possibly quoted)
    TagFieldGetRaw(TagFieldFind(pCmd->strParms, "URL"), strUrl, sizeof(strUrl), "");
    TagFieldGetRaw(TagFieldFind(pCmd->strParms, "URL-GET"), strUrl, sizeof(strUrl), strUrl);
    TagFieldGetRaw(TagFieldFind(pCmd->strParms, "URL-POST"), strUrl, sizeof(strUrl), strUrl);
    if (strUrl[0] == 0)
    {
        WebOfferClear(pOffer);
        return;
    }
    
    // strip quotes if present (assume if we have a leading quote, we have a trailing quote; otherwise the URL is invalid anyway)
    if (strUrl[0] == '"')
    {
        int32_t iUrlLen = (int32_t)strlen(strUrl)-1;
        // remove trailing quote
        strUrl[iUrlLen] = '\0';
        // remove leading quote
        memmove(&strUrl[0], &strUrl[1], iUrlLen);
    }

    // get the timeout (clamp to reasonable range)
    iTime = TagFieldGetNumber(TagFieldFind(pCmd->strParms, "TIME"), 30);
    if (iTime < 2)
        iTime = 2;
    if (iTime > 60)
        iTime = 60;
    pOffer->uHttpTime = NetTick() + (iTime*1000);

    // get the minimum display time
    iTime = TagFieldGetNumber(TagFieldFind(pCmd->strParms, "DISP"), 3);
    if (iTime < 0)
        iTime = 0;
    if (iTime > 60)
        iTime = 60;
    pOffer->uDispTime = NetTick() + (iTime*1000);

    // do substitution within URL
    _WebOfferUrl(strUrl, sizeof(strUrl), pCmd->strParms);

    // see if we should post or get
    if (TagFieldFind(pCmd->strParms, "URL-GET") == NULL)
    {
        // post the data
        if (ProtoHttpPost(pOffer->pHttps, strUrl, pCmd->strParms, -1, FALSE) < 0)
        {
            WebOfferClear(pOffer);
            return;
        }
    }
    else
    {
        // get the data
        if (ProtoHttpGet(pOffer->pHttps, strUrl, FALSE) < 0)
        {
            WebOfferClear(pOffer);
            return;
        }
    }

    // process the send
    pOffer->eState = ST_SENT;
}

/*F*******************************************************************/
/*!
    \Function WebOfferHttpComplete

    \Description
        Check on completion status of transfer

    \Input *pOffer - module state from WebOfferCreate

    \Output positive=complete w/success, zero=in progress, negative=complete w/unhandled failure

    \Version 03/13/2004 gschaefer
*/
/*******************************************************************F*/
int32_t WebOfferHttpComplete(WebOfferT *pOffer)
{
    int32_t iComplete = 0;
    WebOfferUpdate(pOffer);

    // handle success -- incorporate new data into script buffer
    if ((pOffer->eState == ST_DONE) && (NetTick() > pOffer->uDispTime))
    {
        pOffer->pHttpData = _WebOfferFind(pOffer, pOffer->pScript, "$last");
        pOffer->pHttpData[0] = '\n';
        _WebOfferGoto(pOffer, "SUCCESS-GOTO", "", "DEFAULT-GOTO");
        pOffer->pHttpData = NULL;
        pOffer->eState = ST_IDLE;
    }

    // handle failure
    if (pOffer->eState == ST_FAIL)
    {
        // see if this was a resource load failure
        if (pOffer->pHttpIncl != NULL)
        {
            char strGoto[64];

            // clear the resource (might have been partially loaded)
            sprintf(pOffer->pHttpIncl-10, "00000000:");
            pOffer->pHttpData = pOffer->pHttpIncl;
            pOffer->pHttpData[0] = '\0';
            pOffer->iHttpSize = (pOffer->pScript+pOffer->iScript)-pOffer->pHttpData-1;

            // get the goto tag
            TagFieldGetString(TagFieldFind(pOffer->pHttpRsrc, "FAILURE-GOTO"), strGoto, sizeof(strGoto), "");

            // if no error specified
            if (strGoto[0] == 0)
            {
                // this will force the next resource to load
                pOffer->eState = ST_DONE;
            }
            else
            {
                // transfer control to the resource tag so goto will execute
                pOffer->pHttpData = _WebOfferFind(pOffer, pOffer->pScript, "$last");
                pOffer->pHttpData[0] = '\n';
                pOffer->pHttpData = NULL;
                pOffer->pExec = pOffer->pHttpRsrc;
                _WebOfferAction(pOffer, strGoto);
                pOffer->eState = ST_IDLE;
            }
        }
        else
        {
            // script download failed -- run error handler
            _WebOfferGoto(pOffer, "FAILURE-GOTO", "", "DEFAULT-GOTO");
            pOffer->eState = ST_IDLE;
        }
    }

    // handle timeout
    if ((pOffer->eState != ST_IDLE) && (NetTick() > pOffer->uHttpTime))
    {
        _WebOfferGoto(pOffer, "TIMEOUT-GOTO", "FAILURE-GOTO", "DEFAULT-GOTO");
        pOffer->eState = ST_IDLE;
    }

    // idle means we were already called -- assume success
    if ((pOffer->eState == ST_IDLE) && (NetTick() > pOffer->uDispTime))
    {
        // default to success
        iComplete = 1;
        // if script execution stops and no result code was generated, treat as error
        if (((pOffer->pExec == NULL) || (pOffer->pExec[0] == 0)) && (pOffer->strResult[0] == 0))
        {
            iComplete = -1;
        }
    }

    return(iComplete);
}

/*F*******************************************************************/
/*!
    \Function WebOfferUpdate

    \Description
        Do period update -- called by HttpResult automaticallt so most
        applications don't need to call direct

    \Input *pOffer - module state from WebOfferCreate

    \Version 03/13/2004 gschaefer
*/
/*******************************************************************F*/
void WebOfferUpdate(WebOfferT *pOffer)
{
    int32_t iResult, iCount;
    int32_t iSrc, iDst;
    char strUrl[2048];
    char *pHead, *pTail;
    int32_t iTime;

    // wait for response from web server
    if (pOffer->eState == ST_SENT)
    {
        ProtoHttpUpdate(pOffer->pHttps);
        iResult = ProtoHttpStatus(pOffer->pHttps, 'data', NULL, 0);
        if (iResult < 0)
        {
            pOffer->eState = ST_FAIL;
        }
        else if (iResult > 0)
        {
            pOffer->eState = (ProtoHttpStatus(pOffer->pHttps, 'code', NULL, 0) == 200 ? ST_RECVD : ST_FAIL);
        }
    }

    // append data to the script buffer
    while (pOffer->eState == ST_RECVD)
    {
        ProtoHttpUpdate(pOffer->pHttps);
        iCount = ProtoHttpRecv(pOffer->pHttps, pOffer->pHttpData, 0, pOffer->iHttpSize);

        // append data to buffer
        if (iCount > 0)
        {
            if (pOffer->pHttpIncl == NULL)
            {
                // strip CR's from data to deal with Windows created files
                for (iSrc = iDst = 0; iSrc < iCount; ++iSrc)
                {
                    if (pOffer->pHttpData[iSrc] != '\r')
                        pOffer->pHttpData[iDst++] = pOffer->pHttpData[iSrc];
                }
                iCount = iDst;
            }
            else
            {
                // update the binary data length
                sprintf(pOffer->pHttpIncl - 10, "%08x:", (int32_t)(pOffer->pHttpData - pOffer->pHttpIncl) + iCount);
            }

            // update data count and terminate buffer
            pOffer->pHttpData += iCount;
            pOffer->iHttpSize -= iCount;
            pOffer->pHttpData[0] = 0;

            // handle buffer overrun as error
            if (pOffer->iHttpSize < 1)
            {
                NetPrintf(("weboffer: out of buffer\n"));
                pOffer->eState = ST_FAIL;
            }
        }
        else
        {
            // handle end of data
            if (iCount == PROTOHTTP_RECVDONE)
            {
                pOffer->eState = ST_DONE;
            }
            // else error condition
            else if (iCount < 0)
            {
                pOffer->eState = ST_FAIL;
            }
            break;
        }
    }

    // handle fake transfer
    if (pOffer->eState == ST_FAKE)
    {
        // pretend data came from server
        iCount = (int32_t)strlen(pOffer->pHttpData);
        pOffer->pHttpData += iCount;
        pOffer->iHttpSize -= iCount;
        pOffer->eState = ST_DONE;
    }

    // transfer just completed
    if (pOffer->eState == ST_DONE)
    {
        // point to start of http data
        pHead = _WebOfferFind(pOffer, pOffer->pScript, "$last")+1;
        // locate include records
        pHead = _WebOfferFind(pOffer, pHead, "$incl");
        pTail = _WebOfferFind(pOffer, pHead, "$next");
        if ((pHead != NULL) && (pTail != NULL) && (pTail-pHead < pOffer->iHttpSize) && (stristr(pHead, "URL=") != NULL))
        {
            // copy record to end of data
            memmove(pOffer->pHttpData, pHead, pTail-pHead);
            memmove(pHead, pTail, pOffer->pHttpData-pHead);
            pOffer->pHttpData[0] = 0;
            pHead = pOffer->pHttpData - (pTail-pHead);

            // extract the parms
            TagFieldGetString(TagFieldFind(pHead, "URL"), strUrl, sizeof(strUrl), "");
            iTime = TagFieldGetNumber(TagFieldFind(pHead, "TIME"), 0);

            // rename URL to mark as already loaded (new tag must be same length as old)
            TagFieldRename(pHead, pTail-pHead, "URL", "UUU");
            // default to failure 
            pOffer->eState = ST_FAIL;

            // validate data
            if ((strUrl[0] != 0) && (pOffer->iHttpSize > 256))
            {
                // add the binary header
                iCount = sprintf(pOffer->pHttpData, "%%$00000000:")+1;
                pOffer->pHttpData += iCount;
                pOffer->iHttpSize -= iCount;
                pOffer->pHttpIncl = pOffer->pHttpData;
                pOffer->pHttpRsrc = pHead;

                // start the download
                _WebOfferUrl(strUrl, sizeof(strUrl), pOffer->strSetup);
                if (ProtoHttpGet(pOffer->pHttps, strUrl, FALSE) >= 0)
                {
                    pOffer->eState = ST_SENT;
                    pOffer->uHttpTime += (iTime*1000);
                }
            }
        }
    }
}

/*F*******************************************************************/
/*!
    \Function WebOfferResource

    \Description
        Return named resource type field

    \Input pOffer - module state from WebOfferCreate
    \Input pRsrc - resource header to populate
    \Input pName - name of resource

    \Output resource type or zero

    \Version 04/11/2005 gschaefer
*/
/*******************************************************************F*/
int32_t WebOfferResource(WebOfferT *pOffer, WebOfferResourceT *pRsrc, const char *pName)
{
    char *pData;
    char *pFind;
    char *pBogus;
    WebOfferResourceT Temp;

    // let them pass null record to check existance only
    if (pRsrc == NULL)
    {
        pRsrc = &Temp;
    }

    // clear the header
    memset(pRsrc, 0, sizeof(*pRsrc));

    // if there is a news file, we need to restore buffer during the search
    if (pOffer->pNewsHead != pOffer->pNewsTail)
    {
        pOffer->pNewsTail[0] = '%';
    }

    // locate the header & resource
    if ((pName != NULL) && (*pName != 0) && (*pName != '$'))
    {
        pFind = _WebOfferFind(pOffer, pOffer->pScript, pName);
        pData = _WebOfferFind(pOffer, pFind, "$rsrc");

        // setup the header
        if ((pFind != NULL) && (pData != NULL))
        {
            pRsrc->iType = TagFieldGetToken(TagFieldFind(pFind, "TYPE"), 0);
            pRsrc->pParm = pFind;
            pRsrc->iSize = strtol(pData+2, &pBogus, 16);
            pRsrc->pData = (pRsrc->iSize > 0 ? pData+12 : NULL);
        }
    }

    // if there is a news file, restore terminator
    if (pOffer->pNewsHead != pOffer->pNewsTail)
    {
        pOffer->pNewsTail[0] = 0;
    }

    // return type for result
    return(pRsrc->iType);
}

/*F*******************************************************************/
/*!
    \Function WebOfferGetBusy

    \Description
        Extract the busy message for a web transfer

    \Input pOffer - module state from WebOfferCreate
    \Input pInfo - return structure for busy screen data

    \Version 03/13/2004 gschaefer
*/
/*******************************************************************F*/
void WebOfferGetBusy(WebOfferT *pOffer, WebOfferBusyT *pInfo)
{
    const char *pBuf = pOffer->Command.strParms;
    memset(pInfo, 0, sizeof(*pInfo));
    TagFieldGetString(TagFieldFind(pBuf, "TITLE"), pInfo->strTitle, sizeof(pInfo->strTitle), "");
    TagFieldGetString(TagFieldFind(pBuf, "MESG"), pInfo->strMessage, sizeof(pInfo->strMessage), "PLEASE WAIT");
    _WebOfferGetColor(pOffer, &pInfo->MsgColor, "MESG-RGB");
    _WebOfferGetButton(pOffer, WEBOFFER_TAGNAME_BUTTON, &pInfo->Button[0], 1);
}

/*F*******************************************************************/
/*!
    \Function WebOfferGetAlert

    \Description
        Extract the alert data from the current command packet

    \Input pOffer - module state from WebOfferCreate
    \Input pInfo - return structure for alert data

    \Version 03/13/2004 gschaefer
*/
/*******************************************************************F*/
void WebOfferGetAlert(WebOfferT *pOffer, WebOfferAlertT *pInfo)
{
    const char *pBuf = pOffer->Command.strParms;

    // setup base message
    memset(pInfo, 0, sizeof(*pInfo));
    TagFieldGetString(TagFieldFind(pBuf, "TITLE"), pInfo->strTitle, sizeof(pInfo->strTitle), "");
    TagFieldGetString(TagFieldFind(pBuf, "IMAGE"), pInfo->strImage, sizeof(pInfo->strImage), "");
    TagFieldGetString(TagFieldFind(pBuf, "MESG"), pInfo->strMessage, sizeof(pInfo->strMessage), "");
    _WebOfferGetColor(pOffer, &pInfo->MsgColor, "MESG-RGB");

    // setup the button data
    _WebOfferGetButton(pOffer, WEBOFFER_TAGNAME_BUTTON, &pInfo->Button[0], 1);
    _WebOfferGetButton(pOffer, WEBOFFER_TAGNAME_BUTTON, &pInfo->Button[1], 2);
    _WebOfferGetButton(pOffer, WEBOFFER_TAGNAME_BUTTON, &pInfo->Button[2], 3);
    _WebOfferGetButton(pOffer, WEBOFFER_TAGNAME_BUTTON, &pInfo->Button[3], 4);

    // if no buttons defined, default to OK
    if (pInfo->Button[0].strText[0] == 0)
    {
        strcpy(pInfo->Button[0].strText, "OK");
        pInfo->Button[0].strType[0] = WEBOFFER_TYPE_NORMAL;
    }
}

/*F*******************************************************************/
/*!
    \Function WebOfferGetCredit

    \Description
        Extract credit card parms

    \Input pOffer - module state from WebOfferCreate
    \Input pInfo - return structure for credit card data

    \Version 03/13/2004 gschaefer
*/
/*******************************************************************F*/
void WebOfferGetCredit(WebOfferT *pOffer, WebOfferCreditT *pInfo)
{
    char strNum[32];
    int32_t iIdx = 0;
    const char *pBuf = pOffer->Command.strParms;
    memset(pInfo, 0, sizeof(*pInfo));
    TagFieldGetString(TagFieldFind(pBuf, "TITLE"), pInfo->strTitle, sizeof pInfo->strTitle, "");
    TagFieldGetString(TagFieldFind(pBuf, "MESG"), pInfo->strMessage, sizeof pInfo->strMessage, "");
    _WebOfferGetColor(pOffer, &pInfo->MsgColor, "MESG-RGB");
    TagFieldGetString(TagFieldFind(pBuf, "FIRSTNAME"), pInfo->strFirstName, sizeof pInfo->strFirstName, "");
    TagFieldGetString(TagFieldFind(pBuf, "LASTNAME"), pInfo->strLastName, sizeof pInfo->strLastName, "");
    TagFieldGetString(TagFieldFind(pBuf, "ADDRESS1"), pInfo->strAddress[0], sizeof pInfo->strAddress[0], "");
    TagFieldGetString(TagFieldFind(pBuf, "ADDRESS2"), pInfo->strAddress[1], sizeof pInfo->strAddress[1], "");
    TagFieldGetString(TagFieldFind(pBuf, "CITY"), pInfo->strCity, sizeof pInfo->strCity, "");
    TagFieldGetString(TagFieldFind(pBuf, "STATE"), pInfo->strState, sizeof pInfo->strState, "");
    TagFieldGetString(TagFieldFind(pBuf, "POSTCODE"), pInfo->strPostCode, sizeof pInfo->strPostCode, "");
    TagFieldGetString(TagFieldFind(pBuf, "COUNTRY"), pInfo->strCountry, sizeof pInfo->strCountry, "");
    TagFieldGetString(TagFieldFind(pBuf, "EMAIL"), pInfo->strEmail, sizeof pInfo->strEmail, "");
    TagFieldGetString(TagFieldFind(pBuf, "CARDTYPE"), pInfo->strCardType, sizeof pInfo->strCardType, "");
    TagFieldGetString(TagFieldFind(pBuf, "CARDNUMBER"), pInfo->strCardNumber, sizeof pInfo->strCardNumber, "");

    // get country and card type lists
    pInfo->pCardList = TagFieldFind(pBuf, "CARDLIST");
    if (pInfo->pCardList == NULL)
    {
        pInfo->pCardList = "UNKNOWN";
    }
    pInfo->pCountryList = TagFieldFind(pBuf, "COUNTRYLIST");
    if (pInfo->pCountryList == NULL)
    {
        pInfo->pCountryList = "UNKNOWN";
    }

    // read expiry info as strings, but store into structure as numbers
    TagFieldGetString(TagFieldFind(pBuf, "EXPIRYMONTH"), strNum, sizeof(strNum), "1");
    pInfo->iExpiryMonth = atoi(strNum);
    TagFieldGetString(TagFieldFind(pBuf, "EXPIRYYEAR"), strNum, sizeof(strNum), "2004");
    pInfo->iExpiryYear = atoi(strNum);

    // This section reads the information in order to know with what years to 
    //populate the combo box for expiry years
    TagFieldGetString(TagFieldFind(pBuf, "CC_YEAR_START"), strNum, sizeof(strNum), "2006");
    pInfo->iYearStartExpiry = atoi(strNum);
    TagFieldGetString(TagFieldFind(pBuf, "CC_YEAR_END"), strNum, sizeof(strNum), "2015");
    pInfo->iYearEndExpiry = atoi(strNum);

    // setup the button data
    for(iIdx = 0; iIdx < WEBOFFER_CREDIT_NUMBUTTONS ; iIdx++)
    {
        _WebOfferGetButton(pOffer, WEBOFFER_TAGNAME_BUTTON, &pInfo->Button[iIdx], iIdx + 1);
    }

    // setup any auxillary pictures that may or may not be wanted.
    for(iIdx = 0; iIdx < WEBOFFER_CREDIT_NUM_FIXED_IMAGE ; iIdx++)
    {
        _WebOfferGetFixedImage(pOffer, WEBOFFER_TAGNAME_FIXED_IMAGE, &pInfo->FixedImage[iIdx], iIdx + 1);
    }
}

/*F*******************************************************************/
/*!
    \Function WebOfferSetCredit

    \Description
        Store updated credit card data form user

    \Input pOffer - module state from WebOfferCreate
    \Input pInfo - updated information from user

    \Version 03/13/2004 gschaefer
*/
/*******************************************************************F*/
void WebOfferSetCredit(WebOfferT *pOffer, WebOfferCreditT *pInfo)
{
    char *pBuf = pOffer->Command.strParms;
    int32_t iLen = sizeof(pOffer->Command.strParms);
    TagFieldSetString(pBuf, iLen, "FIRSTNAME", pInfo->strFirstName);
    TagFieldSetString(pBuf, iLen, "LASTNAME", pInfo->strLastName);
    TagFieldSetString(pBuf, iLen, "ADDRESS1", pInfo->strAddress[0]);
    TagFieldSetString(pBuf, iLen, "ADDRESS2", pInfo->strAddress[1]);
    TagFieldSetString(pBuf, iLen, "CITY", pInfo->strCity);
    TagFieldSetString(pBuf, iLen, "STATE", pInfo->strState);
    TagFieldSetString(pBuf, iLen, "POSTCODE", pInfo->strPostCode);
    TagFieldSetString(pBuf, iLen, "COUNTRY", pInfo->strCountry);
    TagFieldSetString(pBuf, iLen, "EMAIL", pInfo->strEmail);
    TagFieldSetString(pBuf, iLen, "CARDTYPE", pInfo->strCardType);
    TagFieldSetString(pBuf, iLen, "CARDNUMBER", pInfo->strCardNumber);
    TagFieldSetNumber(pBuf, iLen, "EXPIRYMONTH", pInfo->iExpiryMonth);
    TagFieldSetNumber(pBuf, iLen, "EXPIRYYEAR", pInfo->iExpiryYear);
}

/*F*******************************************************************/
/*!
    \Function WebOfferGetPromo

    \Description
        Extract promo screen parms

    \Input pOffer - module state from WebOfferCreate
    \Input pInfo - return structure with promo data

    \Version 03/13/2004 gschaefer
*/
/*******************************************************************F*/
void WebOfferGetPromo(WebOfferT *pOffer, WebOfferPromoT *pInfo)
{
    const char *pBuf = pOffer->Command.strParms;
    int32_t iIdx = 0;
    memset(pInfo, 0, sizeof(*pInfo));
    TagFieldGetString(TagFieldFind(pBuf, "TITLE"), pInfo->strTitle, sizeof(pInfo->strTitle), "");
    TagFieldGetString(TagFieldFind(pBuf, "MESG"), pInfo->strMessage, sizeof(pInfo->strMessage), "");
    _WebOfferGetColor(pOffer, &pInfo->MsgColor, "MESG-RGB");
    TagFieldGetString(TagFieldFind(pBuf, "PROMO"), pInfo->strPromo, sizeof pInfo->strPromo, "");
    TagFieldGetString(TagFieldFind(pBuf, "K-TITLE"), pInfo->strPromoKPopTitle, sizeof(pInfo->strPromoKPopTitle), "");
    pInfo->iStyle = TagFieldGetNumber(TagFieldFind(pBuf, "STYLE"),0);

    // setup the button data
    for(iIdx = 0; iIdx < WEBOFFER_PROMO_NUMBUTTONS ; iIdx++)
    {
        _WebOfferGetButton(pOffer, WEBOFFER_TAGNAME_BUTTON, &pInfo->Button[iIdx], iIdx + 1);
    }

    // setup any auxillary pictures that may or may not be wanted.
    for(iIdx = 0; iIdx < WEBOFFER_PROMO_NUM_FIXED_IMAGE ; iIdx++)
    {
        _WebOfferGetFixedImage(pOffer, WEBOFFER_TAGNAME_FIXED_IMAGE, &pInfo->FixedImage[iIdx], iIdx + 1);
    }

}

/*F*******************************************************************/
/*!
    \Function WebOfferSetPromo

    \Description
        Store updated promo data form user

    \Input pOffer - module state from WebOfferCreate
    \Input pInfo - updated information from user

    \Version 03/13/2004 gschaefer
*/
/*******************************************************************F*/
void WebOfferSetPromo(WebOfferT *pOffer, WebOfferPromoT *pInfo)
{
    char *pBuf = pOffer->Command.strParms;
    int32_t iLen = sizeof(pOffer->Command.strParms);
    TagFieldSetString(pBuf, iLen, "PROMO", pInfo->strPromo);
}

/*F*******************************************************************/
/*!
    \Function WebOfferGetMenu

    \Description
        Extract menu screen parms

    \Input pOffer - module state from WebOfferCreate
    \Input pInfo - return structure with menu data

    \Version 01/07/2005 jfrank
*/
/*******************************************************************F*/
void WebOfferGetMenu(WebOfferT *pOffer, WebOfferMenuT *pInfo)
{
    const char *pBuf = pOffer->Command.strParms;
    const int32_t iNumButtons = ((signed)sizeof(pInfo->Button)) / ((signed)sizeof(WebOfferButtonT));
    int32_t iLoop;

    memset(pInfo, 0, sizeof(*pInfo));
    TagFieldGetString(TagFieldFind(pBuf, "TITLE"), pInfo->strTitle, sizeof(pInfo->strTitle), "");

    // setup the button data
    for(iLoop=0; iLoop < iNumButtons; iLoop++)
    {
        _WebOfferGetButton(pOffer, WEBOFFER_TAGNAME_BUTTON, &pInfo->Button[iLoop], iLoop+1);
    }
}

/*F*******************************************************************/
/*!
    \Function WebOfferGetNews

    \Description
        Extract news screen parms

    \Input pOffer - module state from WebOfferCreate
    \Input pInfo - return structure with news data

    \Output Pointer to news blob

    \Version 03/17/2004 gschaefer
*/
/*******************************************************************F*/
const char *WebOfferGetNews(WebOfferT *pOffer, WebOfferNewsT *pInfo)
{
    int32_t iIdx = 0;
    const char *pBuf = pOffer->Command.strParms;
    memset(pInfo, 0, sizeof(*pInfo));
    TagFieldGetString(TagFieldFind(pBuf, "TITLE"), pInfo->strTitle, sizeof(pInfo->strTitle), "");
    TagFieldGetString(TagFieldFind(pBuf, "IMAGE"), pInfo->strImage, sizeof(pInfo->strImage), "");
    _WebOfferGetColor(pOffer, &pInfo->NewsColor, "NEWS-RGB");
    _WebOfferGetButton(pOffer, WEBOFFER_TAGNAME_BUTTON, &pInfo->Button[0], 1);
    _WebOfferGetButton(pOffer, WEBOFFER_TAGNAME_BUTTON, &pInfo->Button[1], 2);
    _WebOfferGetButton(pOffer, WEBOFFER_TAGNAME_BUTTON, &pInfo->Button[2], 3);
    _WebOfferGetButton(pOffer, WEBOFFER_TAGNAME_BUTTON, &pInfo->Button[3], 4);
    pInfo->pNews = (pOffer->pNewsHead ? pOffer->pNewsHead : "");
    pInfo->iStyle = TagFieldGetNumber(TagFieldFind(pBuf, "STYLE"),0);
    // setup the button data
    for(iIdx = 0; iIdx < WEBOFFER_NEWS_NUMBUTTONS ; iIdx++)
    {
        _WebOfferGetButton(pOffer, WEBOFFER_TAGNAME_BUTTON, &pInfo->Button[iIdx], iIdx + 1);
    }

    // setup any auxillary pictures that may or may not be wanted.
    for(iIdx = 0; iIdx < WEBOFFER_NEWS_NUM_FIXED_IMAGE ; iIdx++)
    {
        _WebOfferGetFixedImage(pOffer, WEBOFFER_TAGNAME_FIXED_IMAGE, &pInfo->FixedImage[iIdx], iIdx + 1);
    }

    return(pInfo->pNews);
}

/*F*******************************************************************/
/*!
    \Function WebOfferGetArticles

    \Description
        Extract article list structure

    \Input pOffer - module state from WebOfferCreate
    \Input pInfo - return structure with article list data

    \Version 11/03/2005 gschaefer
*/
/*******************************************************************F*/
void WebOfferGetArticles(WebOfferT *pOffer, WebOfferArticlesT *pInfo)
{
    const char *pBuf = pOffer->Command.strParms;
    const int32_t iNumArticles = ((signed)sizeof(pInfo->Article)) / ((signed)sizeof(pInfo->Article[0]));
    const int32_t iNumButtons  = ((signed)sizeof(pInfo->Button))  / ((signed)sizeof(pInfo->Button[0]));
    int32_t iLoop;

    memset(pInfo, 0, sizeof(*pInfo));
    TagFieldGetString(TagFieldFind(pBuf, "TITLE"), pInfo->strTitle, sizeof(pInfo->strTitle), "");

    // setup the button data
    for (iLoop = 0; iLoop < iNumArticles; iLoop++)
    {
        TagFieldGetString(TagFieldFindIdx(pBuf, WEBOFFER_TAGNAME_ARTICLE, iLoop), pInfo->Article[iLoop].strArticle, sizeof(pInfo->Article[iLoop].strArticle), "");
        TagFieldGetString(TagFieldFindIdx(pBuf, "IMAGE", iLoop), pInfo->Article[iLoop].strImage, sizeof(pInfo->Article[iLoop].strImage), "");
        TagFieldGetString(TagFieldFindIdx(pBuf, "DESC", iLoop), pInfo->Article[iLoop].strDesc, sizeof(pInfo->Article[iLoop].strDesc), "");
        TagFieldGetString(TagFieldFindIdx(pBuf, "TYPE", iLoop), pInfo->Article[iLoop].strType, sizeof(pInfo->Article[iLoop].strType), "");
    }
    // setup the button data
    for(iLoop=0; iLoop < iNumButtons; iLoop++)
    {
        _WebOfferGetButton(pOffer, WEBOFFER_TAGNAME_BUTTON, &pInfo->Button[iLoop], iLoop+1);
    }

    // setup any auxillary pictures that may or may not be wanted.
    for(iLoop = 0; iLoop < WEBOFFER_ARTICLES_NUM_FIXED_IMAGE ; iLoop++)
    {
        _WebOfferGetFixedImage(pOffer, WEBOFFER_TAGNAME_FIXED_IMAGE, &pInfo->FixedImage[iLoop], iLoop + 1);
    }

}

/*F*******************************************************************/
/*!
    \Function WebOfferGetStory

    \Description
        Extract text story screen info

    \Input pOffer - module state from WebOfferCreate
    \Input pInfo - return structure with story data

    \Output Pointer to story text

    \Version 11/03/2005 gschaefer
*/
/*******************************************************************F*/
const char *WebOfferGetStory(WebOfferT *pOffer, WebOfferStoryT *pInfo)
{
    const char *pBuf = pOffer->Command.strParms;
    memset(pInfo, 0, sizeof(*pInfo));
    TagFieldGetString(TagFieldFind(pBuf, "TITLE"), pInfo->strTitle, sizeof(pInfo->strTitle), "");
    TagFieldGetString(TagFieldFind(pBuf, "IMAGE"), pInfo->strImage, sizeof(pInfo->strImage), "");
    pInfo->pText = pOffer->pNewsHead;
    return(pInfo->pText);
}

/*F*******************************************************************/
/*!
    \Function WebOfferGetMedia

    \Description
        Extract media player screen info

    \Input pOffer - module state from WebOfferCreate
    \Input pInfo - return structure with media data

    \Output Pointer to media list

    \Version 11/03/2005 gschaefer
*/
/*******************************************************************F*/
const char *WebOfferGetMedia(WebOfferT *pOffer, WebOfferMediaT *pInfo)
{
    const char *pBuf = pOffer->Command.strParms;
    memset(pInfo, 0, sizeof(*pInfo));
    TagFieldGetString(TagFieldFind(pBuf, "TITLE"), pInfo->strTitle, sizeof(pInfo->strTitle), "");
    TagFieldGetString(TagFieldFind(pBuf, "IMAGE"), pInfo->strImage, sizeof(pInfo->strImage), "");
    pInfo->pMedia = pOffer->pNewsHead;
    return(pInfo->pMedia);
}

/*F*******************************************************************/
/*!
    \Function WebOfferGetForm

    \Description
        Extract web offer form parms

    \Input pOffer - module state from WebOfferCreate
    \Input pInfo - return structure for web form data

    \Version 05/23/2006 (ozietman)
*/
/*******************************************************************F*/
void WebOfferGetForm(WebOfferT *pOffer, WebOfferFormT *pInfo)
{
    const int32_t iNumFields  = WEBOFFER_FORM_NUMFIELDS;
    const char *pBuf = pOffer->Command.strParms;
    
    int32_t iLoop;


    memset(pInfo, 0, sizeof(*pInfo));

    // The iNumFields used within the WebOfferFormT structure should be set by the application, however
    // we will default it to the maximum number of fields supported here.
    pInfo->iNumFields = WEBOFFER_FORM_NUMFIELDS;

    TagFieldGetString(TagFieldFind(pBuf, "TITLE"), pInfo->strTitle, sizeof(pInfo->strTitle), "");
    TagFieldGetString(TagFieldFind(pBuf, "MESG"), pInfo->strMessage, sizeof(pInfo->strMessage), "");
    pInfo->iNumRows = TagFieldGetNumber(TagFieldFind(pBuf, "NUMROW"), 1);

    // setup the fields, their default values, and their list of choices if applicable.
    for (iLoop = 0; iLoop < iNumFields; iLoop++)
    {
        // Tags are one based vs. zero based arrays, so we need to account for this when generating the tag string.
        TagFieldGetString(TagFieldFindIdx(pBuf, "FLD-NAME", iLoop + 1), pInfo->Fields[iLoop].strName, sizeof(pInfo->Fields[iLoop].strName), "");
        TagFieldGetString(TagFieldFindIdx(pBuf, "FLD-USERVALUE", iLoop + 1), pInfo->Fields[iLoop].strValue, sizeof(pInfo->Fields[iLoop].strValue), "");

        // Retrieve the list of values that the user can cycle through in the field data entry box
        pInfo->Fields[iLoop].pValues = TagFieldFindIdx(pBuf, "FLD-VALUES", iLoop + 1);
        if(pInfo->Fields[iLoop].pValues == NULL)
        {
            pInfo->Fields[iLoop].pValues = "";
        }

        pInfo->Fields[iLoop].iMaxWidth = TagFieldGetNumber(TagFieldFindIdx(pBuf, "FLD-MAXWIDTH", iLoop + 1), 0);
        pInfo->Fields[iLoop].iLabelWidth = TagFieldGetNumber(TagFieldFindIdx(pBuf, "FLD-LABELWIDTH", iLoop + 1), 0);
        pInfo->Fields[iLoop].iStyle = TagFieldGetNumber(TagFieldFindIdx(pBuf, "FLD-STYLE", iLoop + 1), 0);
        pInfo->Fields[iLoop].bMandatory = TagFieldGetNumber(TagFieldFindIdx(pBuf, "FLD-MANDATORY", iLoop + 1), 0);
    }


    // setup the button data
    for(iLoop = 0; iLoop < WEBOFFER_FORM_NUMBUTTONS ; iLoop++)
    {
        _WebOfferGetButton(pOffer, WEBOFFER_TAGNAME_BUTTON, &pInfo->Button[iLoop], iLoop + 1);
    }

    // setup any auxillary pictures that may or may not be wanted.
    for(iLoop = 0; iLoop < WEBOFFER_FORM_NUM_FIXED_IMAGE ; iLoop++)
    {
        _WebOfferGetFixedImage(pOffer, WEBOFFER_TAGNAME_FIXED_IMAGE, &pInfo->FixedImage[iLoop], iLoop + 1);
    }
}

/*F*******************************************************************/
/*!
    \Function WebOfferGetMarketplace

    \Description
        Extract web offer marketplace parms

    \Input pOffer - module state from WebOfferCreate
    \Input pInfo - return structure for web form data

    \Version 06/13/2005 (tdills)
*/
/*******************************************************************F*/
void WebOfferGetMarketplace(WebOfferT* pOffer, WebOfferMarketplaceT* pInfo)
{
    const char *pBuf = pOffer->Command.strParms;

    memset(pInfo, 0, sizeof(*pInfo));

    // get the default marketplace offer id.
    pInfo->uOfferId = TagFieldGetNumber(TagFieldFind(pBuf, "ID"), 0);
}

/*F*******************************************************************/
/*!
    \Function WebOfferSetForm

    \Description
        Store updated web offer form data from user

    \Input pOffer - module state from WebOfferCreate
    \Input pInfo - updated information from user

    \Version 05/23/2006 (ozietman)
*/
/*******************************************************************F*/
void WebOfferSetForm(WebOfferT *pOffer, WebOfferFormT *pInfo)
{
    char *pBuf = pOffer->Command.strParms;
    int32_t iLen = sizeof(pOffer->Command.strParms);
    int32_t iLoop;

    for(iLoop=0; iLoop < pInfo->iNumFields; iLoop++)
    {
        char strTag[128];
        snzprintf(strTag, sizeof(strTag), "FLD-USERVALUE%d", iLoop + 1);

        TagFieldSetString(pBuf, iLen, strTag, pInfo->Fields[iLoop].strValue);
    }
}
// setup a picture from a tagfield

