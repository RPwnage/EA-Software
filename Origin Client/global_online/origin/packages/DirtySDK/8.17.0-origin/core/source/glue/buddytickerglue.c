/*H********************************************************************************/
/*!
    \File   buddytickerglue.c

    \Description
        buddytickerglue passes events from the buddy module to the ticker module.

    \Notes
        This library is separate to keep from introducing dependencies between the
        modules that would be otherwise required.

        Current functionality includes:

    \verbatim
        Prorogation of Buddy Events to Ticker: buddytickerglue installs an HLBudApi callback
        to track messenger events such as when a buddy logs on or logs off, or when
        a buddy's "extra presence" info is set.  In these cases buddytickerglue adds a
        corresponding event to the Ticker.
    \endverbatim

        Dependencies: HLBudApi, TickerApi

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 04/05/2009 (cvienneau) First Version, adapted from lobbyglue.c
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#ifdef _XBOX
#include <xtl.h>
#endif

#include <string.h>
#include <stdio.h>

#include "dirtysock.h"
#include "dirtymem.h"
#include "lobbytagfield.h"
#include "hlbudapi.h"
#include "tickerapi.h"

#include "buddytickerglue.h"

/*** Defines **********************************************************************/

//! default buddytickerglue buddy ticker score colors
#define BUDDYTICKERGLUE_TICKERWHITE               BUDDYTICKERGLUE_PackColor32(220, 220, 220)
#define BUDDYTICKERGLUE_TICKERGOLD                BUDDYTICKERGLUE_PackColor32(230, 230,  80)
#define BUDDYTICKERGLUE_TICKERBLUE                BUDDYTICKERGLUE_PackColor32(130, 150, 230)

//! size of strExtraPresence buffer
#define BUDDYTICKERGLUE_PRESBUF_SIZE                (64)

//! size of extra-presence duplicate removal cache
#define BUDDYTICKERGLUE_PRESCACHE_SIZE              (8)

//! amount of time before a cached entry is considered stale, in uSec
#define BUDDYTICKERGLUE_PRESCACHE_LIFE              (30 * 1000)

#define BUDDYTICKERGLUE_PERSONA_BUF                 (32)

/*** Macros ***********************************************************************/

//! pack a color
#define BUDDYTICKERGLUE_PackColor32(_r, _g, _b)   ( (uint32_t)(((_r) << 24) | ((_g) << 16) | ((_b) << 8)) )

//! unpack a color
#define BUDDYTICKERGLUE_UnpackColorR(_uColor)     ( ((_uColor) >> 24) & 0xff )
#define BUDDYTICKERGLUE_UnpackColorG(_uColor)     ( ((_uColor) >> 16) & 0xff )
#define BUDDYTICKERGLUE_UnpackColorB(_uColor)     ( ((_uColor) >>  8) & 0xff )

/*** Type Definitions *************************************************************/

//! buddytickerglue buddy/ticker score fields
typedef struct BuddyTickerGlueScoreT
{
    char strHomeUser[BUDDYTICKERGLUE_PERSONA_BUF];
    char strAwayUser[BUDDYTICKERGLUE_PERSONA_BUF];
    int32_t  iHomeScore;
    int32_t  iAwayScore;
    char strStatus[16];
} BuddyTickerGlueScoreT;

//! private module state
struct BuddyTickerGlueRefT
{
    HLBApiRefT              *pHLBRef;               //!< pointer to HLBRef, used to update messenger VoIP flags
    TickerApiT              *pTickerRef;            //!< pointer to TickerRef, used to update Ticker with Buddy events

    // module memory group
    int32_t                 iMemGroup;              //!< module mem group id
    void                    *pMemGroupUserData;     //!< user data associated with mem group
    
    char                    strPresenceCache[BUDDYTICKERGLUE_PRESCACHE_SIZE][BUDDYTICKERGLUE_PRESBUF_SIZE]; //!< buddytickerglue presence cache
    uint32_t                uCacheTime[BUDDYTICKERGLUE_PRESCACHE_SIZE];   //!< cache updated time
    int32_t                 iCacheIndex;            //!< current insert point in cache
    
    BuddyTickerGluePresCallbackT  *pPresCb;               //!< pointer to presence display callback (optional)
    void                    *pPresCbData;           //!< pointer to presence display callback data
    uint32_t                bHomeAway;              //!< if TRUE, display scores in home/away format
    
    char                    strFriends[32];         //!< localized string for "FRIENDS"
};

/*** Variables ********************************************************************/

//!< previously sent extra presence info (used to filter redundant sends)
static char _BuddyTickerGlue_strExtraPresence[BUDDYTICKERGLUE_PRESBUF_SIZE];


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _BuddyTickerGlueCreateUniqueHash

    \Description
        Create a unique 32bit hash of the home and away personas to uniquely tag
        ticker scores with.

    \Input  *pPersonaHome   - pointer to home persona
    \Input  *pPersonaAway   - pointer to away persona

    \Output
        int32_t             - 32bit hash of the concatenation of home and away personas

    \Version 06/16/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _BuddyTickerGlueCreateUniqueHash(const char *pPersonaHome, const char *pPersonaAway)
{
    char strPers[BUDDYTICKERGLUE_PERSONA_BUF*2];
    
    strcpy(strPers, pPersonaHome);
    strcat(strPers, pPersonaAway);
    
    return(NetHash(strPers));
}

/*F********************************************************************************/
/*!
    \Function _BuddyTickerGluePresFormatDefault

    \Description
        Default presence formatting callback for US/English.

    \Input  *pCbData    - unused
    \Input  *pBuffer    - output buffer
    \Input  iBufSize    - size of output buffer
    \Input  *pPersona   - persona to format presence string with
    \Input  bSignedIn   - TRUE if user signed in, FALSE if they signed out

    \Version 06/22/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _BuddyTickerGluePresFormatDefault(void *pCbData, char *pBuffer, int32_t iBufSize, const char *pPersona, uint32_t bSignedIn)
{
    snzprintf(pBuffer, iBufSize, "%s %s", pPersona,
        (bSignedIn) ? "has signed in" : "has signed out");
}

/*F********************************************************************************/
/*!
    \Function _BuddyTickerGlueAddTickerString

    \Description
        Add a string to a ticker event.

    \Input  *pTickerRef - pointer to ticker ref
    \Input  *pRecord    - record to add string to
    \Input  *pString    - string to add
    \Input  uColor      - packed color of string

    \Version 05/11/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _BuddyTickerGlueAddTickerString(TickerApiT *pTickerRef, TickerRecordT *pRecord, const char *pString, uint32_t uColor)
{
    TickerStringT TickerStr;
    
    memset(&TickerStr, 0, sizeof(TickerStr));
    TickerStr.Color.uRed = BUDDYTICKERGLUE_UnpackColorR(uColor);
    TickerStr.Color.uGreen = BUDDYTICKERGLUE_UnpackColorG(uColor);
    TickerStr.Color.uBlue = BUDDYTICKERGLUE_UnpackColorB(uColor);;
    TickerStr.Color.uAlpha = 1;
    strnzcpy((char *)TickerStr.strString, pString, sizeof(TickerStr.strString));

    TickerApiAddString(pTickerRef, pRecord, &TickerStr);
}

/*F********************************************************************************/
/*!
    \Function _BuddyTickerGlueAddInfo

    \Description
        Add a buddy signon/signoff event notification to the ticker.

    \Input  *pTickerRef - pointer to ticker ref
    \Input  *pTickerRec - pointer to ticker record
    \Input  *pInput     - input string from HLBudApi's extra presence info

    \Output
        uint32_t        - TRUE

    \Version 05/25/2004 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _BuddyTickerGlueAddInfo(TickerApiT *pTickerRef, TickerRecordT *pTickerRec, const char *pInput)
{
    // add string
    _BuddyTickerGlueAddTickerString(pTickerRef, pTickerRec, pInput, BUDDYTICKERGLUE_TICKERGOLD);

    // set buddy info to repeat once
    pTickerRec->uRepeatCount = 1;

    // add it
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _BuddyTickerGlueCacheScore

    \Description
        Adds a buddy score to the cache if it is not already in the cache.

    \Input  *pGlueRef       - pointer to ticker ref
    \Input  *pExtraPresence - input string from HLBudApi's extra presence info

    \Output
        uint32_t            - TRUE if it is already in the cache, else FALSE

    \Version 05/11/2004 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _BuddyTickerGlueCacheScore(BuddyTickerGlueRefT *pGlueRef, const char *pExtraPresence)
{
    int32_t iCount, iCache;
    uint32_t uTick = NetTick();

    NetPrintf(("buddytickerglue: scanning cache for string '%s'\n", pExtraPresence));

    // see if it is in the cache
    for (iCount = 0, iCache = pGlueRef->iCacheIndex+1; iCount < BUDDYTICKERGLUE_PRESCACHE_SIZE; iCount++, iCache++)
    {
        // handle wrapping
        if (iCache >= BUDDYTICKERGLUE_PRESCACHE_SIZE)
        {
            iCache = 0;
        }

        // is the element in cache?
        if (!strcmp(pGlueRef->strPresenceCache[iCache], pExtraPresence))
        {
            if (((signed)(uTick - pGlueRef->uCacheTime[iCache])) < 0)
            {
                NetPrintf(("buddytickerglue: cache dup (%d < %d)\n", uTick, pGlueRef->uCacheTime[iCache]));
                return(TRUE);
            }
            else
            {
                NetPrintf(("buddytickerglue: dumping stale cache entry\n"));
            }
        }
    }

    // add it to the cache
    strnzcpy(pGlueRef->strPresenceCache[pGlueRef->iCacheIndex], pExtraPresence, sizeof(pGlueRef->strPresenceCache[0]));
    pGlueRef->uCacheTime[pGlueRef->iCacheIndex] = uTick + BUDDYTICKERGLUE_PRESCACHE_LIFE;

    // move to next cache index
    pGlueRef->iCacheIndex = (pGlueRef->iCacheIndex + 1) % BUDDYTICKERGLUE_PRESCACHE_SIZE;
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function _BuddyTickerGlueAddScore

    \Description
        Add a buddy score to the ticker.

    \Input  *pGlueRef   - pointer to buddytickerglue ref
    \Input  *pTickerRef - pointer to ticker ref
    \Input  *pTickerRec - pointer to ticker record to fill in
    \Input  *pInput     - input string from HLBudApi's extra presence info

    \Output
        uint32_t        - if TRUE, add the score to the ticker

    \Version 05/11/2004 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _BuddyTickerGlueAddScore(BuddyTickerGlueRefT *pGlueRef, TickerApiT *pTickerRef, TickerRecordT *pTickerRec, const char *pInput)
{
    BuddyTickerGlueScoreT ScoreInfo;
    char strHomeScore[32];
    char strAwayScore[32];
    const char *pHu, *pHs, *pAu, *pAs, *pSt;
    unsigned char bFinal;
    uint32_t uStringColors[5] =
    {
        BUDDYTICKERGLUE_TICKERWHITE,
        BUDDYTICKERGLUE_TICKERWHITE,
        BUDDYTICKERGLUE_TICKERWHITE,
        BUDDYTICKERGLUE_TICKERWHITE,
        BUDDYTICKERGLUE_TICKERWHITE
    };

    // pull all required fields
    pHu = TagFieldFind(pInput, "HU");
    pHs = TagFieldFind(pInput, "HS");
    pAu = TagFieldFind(pInput, "AU");
    pAs = TagFieldFind(pInput, "AS");
    pSt = TagFieldFind(pInput, "ST");

    // make sure all fields are present
    if ((pHu && pHs && pAu && pAs && pSt) == 0)
    {
        return(FALSE);
    }

    // extract game info
    TagFieldGetString(pHu, ScoreInfo.strHomeUser, sizeof(ScoreInfo.strHomeUser), "");
    ScoreInfo.iHomeScore = TagFieldGetNumber(pHs, 0);
    TagFieldGetString(pAu, ScoreInfo.strAwayUser, sizeof(ScoreInfo.strAwayUser), "");
    ScoreInfo.iAwayScore = TagFieldGetNumber(pAs, 0);
    TagFieldGetString(pSt, ScoreInfo.strStatus, sizeof(ScoreInfo.strStatus), "");

    // create home/away string scores, figure out if this is a final score or not
    snzprintf(strHomeScore, sizeof(strHomeScore), " %d ", ScoreInfo.iHomeScore);
    snzprintf(strAwayScore, sizeof(strAwayScore), " %d ", ScoreInfo.iAwayScore);
    bFinal = (ScoreInfo.strStatus[0] == 'F') ? TRUE : FALSE;

    // is it a final and not a tie?
    if ((bFinal == TRUE) && (ScoreInfo.iHomeScore != ScoreInfo.iAwayScore))
    {
        if (ScoreInfo.iHomeScore > ScoreInfo.iAwayScore)
        {
            uStringColors[0] = uStringColors[1] = BUDDYTICKERGLUE_TICKERGOLD;
        }
        else
        {
            uStringColors[2] = uStringColors[3] = BUDDYTICKERGLUE_TICKERGOLD;
        }
        
        uStringColors[4] = BUDDYTICKERGLUE_TICKERGOLD;
    }
    else
    {
        // is it final and a tie?
        if (bFinal == TRUE)
        {
            uStringColors[4] = BUDDYTICKERGLUE_TICKERGOLD;
        }
        
        uStringColors[1] = uStringColors[3] = BUDDYTICKERGLUE_TICKERGOLD;
    }

    // add strings
    _BuddyTickerGlueAddTickerString(pTickerRef, pTickerRec, pGlueRef->strFriends, BUDDYTICKERGLUE_TICKERBLUE);
    if (pGlueRef->bHomeAway)
    {
        _BuddyTickerGlueAddTickerString(pTickerRef, pTickerRec, ScoreInfo.strHomeUser, uStringColors[0]);
        _BuddyTickerGlueAddTickerString(pTickerRef, pTickerRec, strHomeScore, uStringColors[1]);
        _BuddyTickerGlueAddTickerString(pTickerRef, pTickerRec, ScoreInfo.strAwayUser, uStringColors[2]);
        _BuddyTickerGlueAddTickerString(pTickerRef, pTickerRec, strAwayScore, uStringColors[3]);
    }
    else
    {
        _BuddyTickerGlueAddTickerString(pTickerRef, pTickerRec, ScoreInfo.strAwayUser, uStringColors[2]);
        _BuddyTickerGlueAddTickerString(pTickerRef, pTickerRec, strAwayScore, uStringColors[3]);
        _BuddyTickerGlueAddTickerString(pTickerRef, pTickerRec, ScoreInfo.strHomeUser, uStringColors[0]);
        _BuddyTickerGlueAddTickerString(pTickerRef, pTickerRec, strHomeScore, uStringColors[1]);
    }
    _BuddyTickerGlueAddTickerString(pTickerRef, pTickerRec, ScoreInfo.strStatus, uStringColors[4]);

    // set ID for this event based on a hash of home/away user, so any subsequent events will replace previous events
    pTickerRec->iId = _BuddyTickerGlueCreateUniqueHash(ScoreInfo.strHomeUser, ScoreInfo.strAwayUser);

    // set scores to repeat three times
    pTickerRec->uRepeatCount = 3;
    
    // add score to ticker
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _BuddyTickerGluePresenceCallback

    \Description
        Updates ticker with buddy events.

    \Input *pRef        - pointer to module state
    \Input *pBuddy      - buddy pointer
    \Input eOldState    - old state
    \Input *pContext    - context pointer

    \Version 04/23/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _BuddyTickerGluePresenceCallback(HLBApiRefT *pRef,  HLBBudT *pBuddy, HLBStatE eOldState, void *pContext)
{
    BuddyTickerGlueRefT *pGlueRef = (BuddyTickerGlueRefT *)pContext;
    uint8_t bExtraInfo = FALSE;
    char strExtraInfo[TICKER_MESSAGE_MAX + 10] = "";
    HLBStatE eNewState;
    
    // ignore temp buddy events
    if (HLBBudIsTemporary(pBuddy))
    {
        return;
    }

    // ignore log-on events
    if (eOldState == HLB_STAT_UNDEFINED)
    {
        NetPrintf(("buddytickerglue: ignoring roster download buddy presence update\n"));
        return;
    }

    // get extra presence info, if any
    HLBBudGetPresenceExtra(pBuddy, strExtraInfo, sizeof(strExtraInfo));
    
    // if state change, let localization callback handle formatting
    if (((eNewState = HLBBudGetState(pBuddy)) != eOldState) && (eNewState != HLB_STAT_PASSIVE))
    {
        // let localization callback format the string
        pGlueRef->pPresCb(pGlueRef->pPresCbData, strExtraInfo, sizeof(strExtraInfo), HLBBudGetName(pBuddy), eNewState == HLB_STAT_CHAT);
    }
    else if (strExtraInfo[0] != '\0')
    {
        // make sure string isn't too long
        if (strlen(strExtraInfo) >= (TICKER_MESSAGE_MAX-1))
        {
            strExtraInfo[TICKER_MESSAGE_MAX-1] = '\0';
        }
        bExtraInfo = TRUE;
    }

    // is there a message to send to the ticker?
    if (strExtraInfo[0] != '\0')
    {
        TickerRecordT TickerRec;
        unsigned char bAddTickerEvent = FALSE;
        
        // init record
        memset(&TickerRec, 0, sizeof(TickerRec));
        TickerRec.iPriority = 8;
        TickerRec.iMsToDisplayFor = 3000;
        TickerApiFilterNone(&TickerRec.Kind);
        TickerApiFilterSet(&TickerRec.Kind, TICKER_FILT_BUDDY);
        
        // init string
        if (bExtraInfo == FALSE)
        {
            // add signon/signoff message
            bAddTickerEvent = _BuddyTickerGlueAddInfo(pGlueRef->pTickerRef, &TickerRec, strExtraInfo);
        }
        else if (_BuddyTickerGlueCacheScore(pGlueRef, strExtraInfo) == FALSE)
        {
            // score update is not in the cache, so add it
            bAddTickerEvent = _BuddyTickerGlueAddScore(pGlueRef, pGlueRef->pTickerRef, &TickerRec, strExtraInfo);
        }

        // insert new ticker event
        if (bAddTickerEvent == TRUE)
        {        
            TickerApiInsert(pGlueRef->pTickerRef, &TickerRec);
        }
    }
}

/*** Public functions *************************************************************/




/*F********************************************************************************/
/*!
    \Function BuddyTickerGlueCreate

    \Description
        Start up buddy ticker glue code.  This glue code provides functionality that
        ties together independent modules.

    \Input *pHLBRef     - pointer to hlbapi module state (may be NULL)
    \Input *pTickerRef  - pointer to tickerapi module state (may be NULL)

    \Version 03/08/2004 (jbrookes)
*/
/********************************************************************************F*/
BuddyTickerGlueRefT *BuddyTickerGlueCreate(HLBApiRefT *pHLBRef, TickerApiT *pTickerRef)
{
    BuddyTickerGlueRefT *pGlueRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    
    // allocate module state memory
    if ((pGlueRef = (BuddyTickerGlueRefT *)DirtyMemAlloc(sizeof(*pGlueRef), BUDDYTICKERGLUE_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        return(NULL);
    }
    memset(pGlueRef, 0, sizeof(*pGlueRef));
    pGlueRef->iMemGroup = iMemGroup;
    pGlueRef->pMemGroupUserData =pMemGroupUserData;
    
    // clear strExtraPresence static buffer
    memset(_BuddyTickerGlue_strExtraPresence, 0, sizeof(_BuddyTickerGlue_strExtraPresence));
    
    // store module refs
    pGlueRef->pHLBRef = pHLBRef;
    pGlueRef->pTickerRef = pTickerRef;
    
    // set up to receive buddyapi presence events
    if ((pGlueRef->pHLBRef != NULL) && (pGlueRef->pTickerRef != NULL))
    {
        HLBApiRegisterBuddyPresenceCallback(pGlueRef->pHLBRef, _BuddyTickerGluePresenceCallback, pGlueRef);
        HLBApiPresenceVOIPSend(pGlueRef->pHLBRef, HLB_VOIP_NONE);
    }
    
    // set up default localization info
    strnzcpy(pGlueRef->strFriends, "FRIENDS ", sizeof(pGlueRef->strFriends));
    pGlueRef->pPresCb = _BuddyTickerGluePresFormatDefault;
    
    // return ref to caller
    return(pGlueRef);
}

/*F********************************************************************************/
/*!
    \Function BuddyTickerGlueDestroy

    \Description
        Shut down buddy ticker glue module.

    \Input *pGlueRef    - module state

    \Version 03/08/2004 (jbrookes)
*/
/********************************************************************************F*/
void BuddyTickerGlueDestroy(BuddyTickerGlueRefT *pGlueRef)
{
    // unregister buddy callback
    if (pGlueRef->pHLBRef != NULL)
    {
        HLBApiRegisterBuddyPresenceCallback(pGlueRef->pHLBRef, NULL, NULL);
    }
    
    // dispose of ref
    DirtyMemFree(pGlueRef, BUDDYTICKERGLUE_MEMID, pGlueRef->iMemGroup, pGlueRef->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function BuddyTickerGlueSetLocalization

    \Description
        Set localization info.

    \Input  *pGlueRef       - pointer to module ref
    \Input  *pPresCb        - pointer to a callback to format presence information for display
    \Input  *pPresCbData    - pointer to user data for callback
    \Input  bHomeAway       - if TRUE, display ticker scores in home/away format (default is away/home)    
    \Input  *pFriendsStr    - pointer to use for localized display of "FRIENDS " string (note - requires trailing space)

    \Version 06/22/2004 (jbrookes)
*/
/********************************************************************************F*/
void BuddyTickerGlueSetLocalization(BuddyTickerGlueRefT *pGlueRef, BuddyTickerGluePresCallbackT *pPresCb, void *pPresCbData, uint32_t bHomeAway, const char *pFriendsStr)
{
    pGlueRef->pPresCb = pPresCb;
    pGlueRef->pPresCbData = pPresCbData;
    pGlueRef->bHomeAway = bHomeAway;
    strnzcpy(pGlueRef->strFriends, pFriendsStr, sizeof(pGlueRef->strFriends));
}

/*F********************************************************************************/
/*!
    \Function BuddyTickerGlueSetScore

    \Description
        Set a score to be broadcast via messenger and automatically be added to
        the tickers of buddies.

        As this function is intended to be called in-game (when BuddyTickerGlue is
        presumably shut down) this function takes an HLBud ref directly, instead
        of a BuddyTickerGlue ref.

    \Input  *pHLBRef        - pointer to HLBud ref
    \Input  *pHomeUser      - pointer to name of home user
    \Input  iHomeScore      - score of home user
    \Input  pAwayUser       - pointer to away user
    \Input  iAwayScore      - score of away user
    \Input  *pStatus        - game status string (see notes) eg 1Q, 2Q, OT, F, etc.
    \Input  *pPresSame      - if non-null, points to the string to update pressame with

    \Output
        int32_t             - HLB result code (HLB_ERROR_*)

    \Notes
        The status string is game-dependent with one exception - the first
        character must be 'F' if the score is a final score, as this changes the
        way the text color formatting is performed.

    \Version 05/11/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t BuddyTickerGlueSetScore(HLBApiRefT *pHLBRef, const char *pHomeUser, int32_t iHomeScore, const char *pAwayUser, int32_t iAwayScore, const char *pStatus, const char *pPresSame)
{
    char strExtraPresence[BUDDYTICKERGLUE_PRESBUF_SIZE];
    int32_t iResult = HLB_ERROR_NONE;
    
    // create tagfield
    TagFieldClear(strExtraPresence, sizeof(strExtraPresence));
    TagFieldSetString(strExtraPresence, sizeof(strExtraPresence), "HU", pHomeUser);
    TagFieldSetNumber(strExtraPresence, sizeof(strExtraPresence), "HS", iHomeScore);
    TagFieldSetString(strExtraPresence, sizeof(strExtraPresence), "AU", pAwayUser);
    TagFieldSetNumber(strExtraPresence, sizeof(strExtraPresence), "AS", iAwayScore);
    TagFieldSetString(strExtraPresence, sizeof(strExtraPresence), "ST", pStatus);
    
    // only send it if it is different
    if (strcmp(_BuddyTickerGlue_strExtraPresence, strExtraPresence))
    {
        HLBApiPresenceExtra(pHLBRef, strExtraPresence);
        strnzcpy(_BuddyTickerGlue_strExtraPresence, strExtraPresence, sizeof(_BuddyTickerGlue_strExtraPresence));
        
        // if they want us to update normal presence, do so
        if (pPresSame != NULL)
        {
            iResult = HLBApiPresenceSend(pHLBRef, HLB_STAT_PASSIVE, -1, (char *)pPresSame, 0);
        }
        else
        {
            // just send extra presence update
            iResult = HLBApiPresenceSendSetPresence(pHLBRef);
        }
    }

    // return result code to caller
    return(iResult);
}

