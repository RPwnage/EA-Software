/*H*************************************************************************************************/
/*!
    \File buddyapi.c

    \Description
        This module implements the client API for the EA.COM buddy server.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2003-2004.  ALL RIGHTS RESERVED

    \Version 1.0 01/20/2002 (GWS) First Version

*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "DirtySDK/proto/protoaries.h"
#include "DirtySDK/proto/protoname.h"
#include "DirtySDK/dirtylang.h"
#include "DirtySDK/util/displist.h"
#include "DirtySDK/util/tagfield.h"
#include "DirtySDK/util/hasher.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/buddy/buddyapi.h"


/*** Defines ***************************************************************************/

// const ID fields for messages, where we dont track sequenced ids.
#define ROSTER_BUDDY 1
#define ROSTER_IGNORE 2
#define USCH_FIXED_ID 3
#define USERFWD_FIXED_ID 4
 

//seconds for waiting to reconnect.. will use random number in range.  todo -- make it configurable?
#define MIN_RECONNECT_TO 15
#define MAX_RECONNECT_TO 100
#define MAX_PACKET 512  

#define BUDDYAPI_STATUS_SIZE    (signed)(sizeof(_BuddyApi_Status)/sizeof(_BuddyApi_Status[0]))

//ATTR flags that  server encodes as chars on Roster or RNOT messages.
#define RNOT_ATTR_FLAG_SENT  (1<<('S'&31))     //!< I invited him to play game
#define RNOT_CHAR_SENT "S" //! the flag as a character string
#define RNOT_ATTR_FLAG_RECEIVED  (1<<('R'&31))  //!< I recd invite to play game
#define RNOT_ATTR_FLAG_TEXT (1<<('T'&31))  //!<  bud does text messaging
#define PGET_ATTR_FLAG_JOINABLE  (1<<('J'&31))  //!< bud can be joined --from presence.

/*** Macros ****************************************************************************/

//To compile out debug print strings in Production release. 
#if DIRTYCODE_LOGGING
#define BuddyApiPrintf(x) _BuddyApiPrintfCode x
#else
#define BuddyApiPrintf(x)
#endif

/*** Type Definitions ******************************************************************/

//! different callback types
enum {
    CB_ADDUSER = 0,
    CB_DELUSER,
    CB_SEND,
    CB_FINDUSER,
    CB_BROADCAST,
    CB_SETFWD,
    CB_BUDRESP,
    CB_GAMERESP,
    CB_GAMEINVITE,
    CB_MAX
};


//! current module state
struct BuddyApiRefT
{
    BuddyMemAllocT *pBuddyAlloc;    //!< buddy memory allocation function.
    BuddyMemFreeT *pBuddyFree;      //!< buddy memory free function.
    HostentT *pHost;                //!< host network
    ProtoAriesRefT *pAries;         //!< network session reference
    int32_t iStat;                  //!< connection status

    // dirtymem memory group used for "front-end" allocationsmodule memory group
    int32_t iMemGroup;              //!< module mem group id (front-end alloc)
    void *pMemGroupUserData;        //!< user data associated with mem group
    
    // dirtymem memory group used for "in-game" allocationsmodule memory group
    int32_t iRefMemGroup;           //!< ref module mem group id (in-game alloc)
    void *pRefMemGroupUserData;     //!< user data associated with ref mem group

    uint32_t uTimeout;              //!< packet timeout
    int32_t iPort;                  //!< save the port number
    int32_t iSeqn;                  //!< sequence number
    int32_t bPeriod;                //!< strip period names (namespace references)

    // save login stuff ..
    char pServer[128];
    int32_t iConnectTimeout;
    char pLKey[128];
    char pRsrc[128];
    int32_t bReconnect;             //!< Flag to tell if we should try to reconnect..
    uint32_t uRandom;               //!< Random number used for re-connect timeout..

    int32_t iProd;                  //!< product identifier token
    char strPresProd[32];           //!< same product identifier
    char strPresSame[256];          //!< presense to same product
    char strPresDiff[256];          //!< presence to different product
    char strPresExtra[128];         //!< additional presence string
    char strPresSend[128];          //!< general presence info
    int32_t bPresJoinable;          //!< Can someone in other (Xbox) game join me?
    int32_t bPresNoText;            //!< true if client does not support messaging?
    uint32_t uPresExFlags;          //!< extra, client specific presence flags, if any.
    uint32_t uPresTick;             //!< time to send presence at
    int32_t iShow;                  //!< current show state

    char strSame[64];               //!< private presence identifier
    uint32_t uLocale;               //!< locale -will give us language.
    char strProduct[64];            //!< product identification string
    char strRelease[64];            //!< product release date
    char strLogin[400];             //!< initial login packet
    char strDomain[16];             //!< the bare domain

    char strEmail [BUDDY_EMAIL_SMS_ADDR_LEN +1 ];   //!< users email, if set
    char strSMS [ BUDDY_EMAIL_SMS_ADDR_LEN +1 ];     //!<  users Sms, if set
    unsigned uForwardFlags ;        //!< user email forwarding setting

    HasherRef *pHash;               //!< hash table for this item
    DispListRef *pDisp;             //!< display list for this item

    void *pMesgData;                //! data for message callback
    BuddyApiCallbackT *pMesgProc;   //! message callback function

    void *pListData;                //! data for buddy list callback
    BuddyApiCallbackT *pListProc;   //! buddy list callback function
    int32_t iRostListCount;         //! number of roster records to be returned

    void *pConnData;                //! data for connection callback
    BuddyApiCallbackT *pConnProc;   //! connection callback function
    BuddyApiDelCallbackT *pBuddyDelProc; //! c/b for caller to free user ref ptr.
    void  *pBuddyDelData;           //! user date for delete user ref callback. 

    BuddyApiBuddyChangeCallbackT *pBuddyChangeProc; //!callback when buddy online after game invite accepted
    void *pBuddyChangeData;         //!data for game invite ready callback
    char pBuddyChangeUser [17];     //!which buddy is ready. TODO --get proper const.

    struct {
        int32_t iType;              //! callback type
        int32_t iSeqn;              //! sequence number of transaction
        uint32_t uTimeout;          //! add user timeout
        void *pCallData;            //! data for add callback
        BuddyApiCallbackT *pCallProc; //! add callback function
    } Callback[CB_MAX];

    void *pDebugData;               //! data for debug callback
    void (*pDebugProc)(void *pData, const char *pText); //! debug callback function
    int32_t iFoundUsersCount;       //! inverse count of users found in search.
    BuddyApiUserT foundUsers[BUDDY_MAX_USERS_FOUND + 1]; //!list of users found.
    int32_t bEndRoster;             //! is initial roster uploade done?    
    char            strTitleName [100];
    char strSavedPresPacket[MAX_PACKET];
    char strSEID [BUDDY_SESSION_ID_LEN+1] ;//!track game sessions for PS2.
};



/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

int32_t _Buddy_CBType[CB_MAX] =
{
    BUDDY_MSG_ADDUSER,          // CB_ADDUSER
    BUDDY_MSG_DELUSER,          // CB_DELUSER
    BUDDY_MSG_SEND,             // CB_SEND
    BUDDY_MSG_USERS,            // CB_FINDUSER
    BUDDY_MSG_BROADCAST,        // CB_BROADCAST
    BUDDY_MSG_SET_FORWARDING,   // CB_SETFWD
    BUDDY_MSG_BUD_INVITE,       // CB_BUDRESP
    BUDDY_MSG_GAME_INVITE,      // CB_GAMERESP
    BUDDY_MSG_GAME_INVITE,      // CB_GAMEINVITE
};

static const char *_BuddyApi_Status[] = { "DISC", "CHAT", "AWAY", "XA", "DND", "PASS", "GAME" };

//maps the Buddy Invite response enum to the actual char code sent to server.
static const char *_MapResponseToAnswer[BUDDY_INVITE_RESPONSE_UNKNOWN_MAX +1] =
{
    "Y",  //accept
    "N",  //reject
    "",   //revoke -- n/a
    "B",  //block
    "R",  //remove
    "z",  //unknown
};

 

struct {
    int16_t iStat;
    char *pState;
} xx;

//  see BUDDY_SERV_XXXX
#if DIRTYCODE_LOGGING
static const char * _BuddyApi_Xlate_Stat [] = { 
     "idle", 
     "connecting",
     "name lookup fail",
     "server timeout",
     "misc connection fail",
     "online",
     "disconnected",
     "reconnect wait",
     "",  //new go here
};
#endif

// Public variables

/*** Private Functions *****************************************************************/
int32_t  _BuddyApiReconnect ( BuddyApiRefT *pState , int32_t bImmediate);


#if DIRTYCODE_LOGGING
/*F*************************************************************************************************/
/*!
    \Function _BuddyApiPrintfCode

    \Description
        Handle debug output

    \Input *pState  - reference pointer
    \Input *pFormat - format string
    \Input ...      - varargs

    \Version 01/20/2002 (GWS)
*/
/*************************************************************************************************F*/
static void _BuddyApiPrintfCode(BuddyApiRefT *pState, const char *pFormat, ...)
{
    va_list args;
    char strText[4096];

    // parse the data
    va_start(args, pFormat);
    ds_vsnprintf(strText, sizeof(strText), pFormat, args);
    va_end(args);

    // pass to caller routine if provied
    if (pState->pDebugProc != NULL)
    {
        (pState->pDebugProc)(pState->pDebugData, strText);
    }
}
#endif

/*F*************************************************************************************************/
/*!
    \Function _BuddyApiDebug

    \Description
        Display packet in debug format

    \Input *pState  - reference pointer
    \Input *pLabel  - output label (send/recv)
    \Input *pMsg    - packet to display

    \Version 01/20/2002 (GWS)
*/
/*************************************************************************************************F*/
static void _BuddyApiDebug(BuddyApiRefT *pState, const char *pLabel, const BuddyApiMsgT *pMsg)
{
#if DIRTYCODE_LOGGING
    int32_t iIdx;
    int32_t iBin;
    int32_t iKind = pMsg->iKind;
    int32_t iCode = pMsg->iCode;
    const char *pData = pMsg->pData;

    // make code printable
    if (iCode == 0)
        iCode = '----';

    // make kind printable
    if (iKind == (signed)0xffffffff)
    {
        iKind = '****';
        if (iCode == (signed)0xffffffff)
            iCode = 'conn';
        else if (iCode == (signed)0xfefefefe)
            iCode = 'disc';
        else
            iCode = '----';
    }

    // show the header
    BuddyApiPrintf((pState, "%s%c%c%c%c/%c%c%c%c\n", pLabel,
        (iKind>>24), (iKind>>16), (iKind>>8), (iKind>>0),
        (iCode>>24), (iCode>>16), (iCode>>8), (iCode>>0)));

    // check for weird characters (non printable)
    for (iIdx = 0, iBin = FALSE; pData[iIdx] != 0; ++iIdx)
    {
        if (pData[iIdx] > 126)
            break;
        if (pData[iIdx] >= ' ')
            continue;
        if ((pData[iIdx] == '\t') || (pData[iIdx] == '\n') || (pData[iIdx] == '\r'))
            continue;
        // contains binary data
        //iBin = TRUE;
        break;
    }

    // show the data
    if (iIdx == 0)
        BuddyApiPrintf((pState, " [null data]\n"));
    else if (iBin == TRUE)
        BuddyApiPrintf((pState, " [binary data]\n"));
    else if (iIdx > 600)
        BuddyApiPrintf((pState, " [long data]\n"));
    else
        BuddyApiPrintf((pState, "%s\n", pData));

#endif  
}

//! convert server ATTR flags to buddy Flags.
static uint32_t _BuddyApiFlagsFromAttr(BuddyApiRefT *pState, BuddyApiMsgT msg)
{
    
    unsigned uFlags =0, uAttrFlags=0;
    uAttrFlags = TagFieldGetFlags(TagFieldFind(msg.pData, "ATTR"), 0);
    uFlags |= (uAttrFlags & RNOT_ATTR_FLAG_SENT )?  BUDDY_FLAG_BUD_REQ_SENT : 0;
    uFlags |= (uAttrFlags & RNOT_ATTR_FLAG_RECEIVED )?  BUDDY_FLAG_BUD_REQ_RECEIVED : 0;
    uFlags |= (uAttrFlags & RNOT_ATTR_FLAG_TEXT )?  BUDDY_FLAG_TEXT : 0;
    //also have 'A'  for accepted at times, but thats like no flags ..
    return(uFlags);
}

/*F*************************************************************************************************/
/*!
    \Function    _BuddyApiTransact

    \Description
        Send a buddy transaction

    \Input pState - module state pointer
    \Input iKind - packet kind
    \Input pData - packet data

    \Version 05/01/03 (GWS)
*/
/*************************************************************************************************F*/
static void _BuddyApiTransact(BuddyApiRefT *pState, int32_t iKind, const char *pData)
{
    // send to server
    ProtoAriesSend(pState->pAries, iKind, 0, pData, -1);

    // show debug output
    if (pState->pDebugProc != NULL)
    {
        BuddyApiMsgT Msg;
        Msg.iKind = iKind;
        Msg.iCode = 0;
        Msg.iType = 0;
        Msg.pData = pData;
        _BuddyApiDebug(pState, "send: ", &Msg);
    }
}

//! Create the list for containing the roster, and a matching hash table.
static void _createBuddyDispList (BuddyApiRefT *pState)
{
    DirtyMemGroupEnter(pState->iMemGroup, pState->pMemGroupUserData);

    // allocate the list and make it active
    if  (pState->pHash == NULL )
    {
        pState->pHash = HasherCreate(1000, 2000);
    }
    if  (pState->pDisp  == NULL)
    {
        pState->pDisp = DispListCreate(100, 100, 0);
    }

    DirtyMemGroupLeave();
}

/*F*************************************************************************************************/
/*!
    \Function _BuddyApiExtractRandomSeed

    \Description
        Grab the random characters in the LKEY for a seed, or, get ticks.

    \Input *pState  - reference pointer

    \Output seed value - uint16_t --only 4 hex chars are truly random.
*/
/*************************************************************************************************F*/

static uint16_t _BuddyApiExtractRandomSeed (BuddyApiRefT *pState)
{
    //There are 4 hex chars on lkey 12..15 that are random -- 
    //grab and convert to binary.
    uint16_t retVal= 0;
    unsigned char bin [17]; //only need 4, but.. 
    char tempStr [17];
    memset (bin,0,sizeof (bin) );
    //sprintf ( pState->pLKey, "308ddc6f393203F0a66fd0b0000283c");
 
    if  ((pState->pLKey  ) && (pState->pLKey[0]!= 0 )&& (pState->pLKey[15] !=0 ) )
    {
        ds_snzprintf (tempStr, sizeof(tempStr), "$%c%c%c%c",   pState->pLKey[12], pState->pLKey[13], 
                                        pState->pLKey[14], pState->pLKey[15]);
        //convert ascii code hex to the 4 binary nibbles i.e. 2 bytes ...
        TagFieldGetBinary  ( tempStr, bin, sizeof ( bin )) ; 
        //Endianess -- is in unaccounted for, here.
        //Could be a problem, in theory, on the PC. However, we are 
        //just getting a random seed, so, ignoring little/big endian conversion.
        retVal = *((uint16_t*) bin);
    }
    else
        retVal = (int16_t) ProtoAriesTick(pState->pAries); //test mode, something that works
    return(retVal);
}


/*F*************************************************************************************************/
/*!
    \Function    _BuddyApiTruncUsername

    \Description
        Usernames come in many flavors.  We're only interested in the actual username part, without
        any domain and resource info.  This function takes as input a full username, and it terminates
        the user name early so as to remove any domain and/or resource information.

    \Input *pUserName    - pointer to username to truncate

    \Version 01/29/03 (JLB)
*/
/*************************************************************************************************F*/
static void _BuddyApiTruncUsername(char *pUserName)
{
    char *pChar;

    if ((pChar = strchr(pUserName,'@')) != NULL)
    {
        *pChar = '\0';
    }

    if ((pChar = strchr(pUserName,'/')) != NULL)
    {
        *pChar = '\0';
    }
    // remove the . too  -- since they support two domains, xbox show as  joe.XBL_SUB
    if ((pChar = strchr(pUserName,'.')) != NULL)
    {
        *pChar = '\0';
    }
}

// Convert a server resource string into Domain and Product strings.
static void _BuddyApiParseResource (char *pResource, int32_t domSize, char * pDomain, int32_t prodSize, char * pProduct )
{
        char strTemp[255];
        char *pDiv;
        ds_strnzcpy(strTemp, pResource, sizeof(strTemp));
        // locate the divider
        if ( ( pDiv = strchr(strTemp, '/'))  == NULL)
        {  //only domain, no product.
            ds_strnzcpy (pDomain, strTemp, domSize );
            *pProduct =0;
        }
        else
        {  // turn into two strings
            *pDiv =0;
            ds_strnzcpy (pDomain, strTemp, domSize );
            ds_strnzcpy ( pProduct, pDiv, prodSize);
        }
}



/*F*************************************************************************************************/
/*!
    \Function    _BuddyApiRecv

    \Description
        Handle receipt of a message and distribution to user callback.

    \Input *pState  - buddy reference pointer
    \Input *pMsg    - buddy message being received

    \Notes
    \verbatim
        This function has to do some rather gross dissecting of the USER field
        of the incoming message, to transform it from Jabber (user@domain/resource)
        form into USER=user DOMN=domain RSRC=resource form.
    \endverbatim

    \Version 01/30/03 (JLB)
*/
/*************************************************************************************************F*/
static void _BuddyApiRecv(BuddyApiRefT *pState, BuddyApiMsgT *pMsg)
{
    char strMesg[2048];
    char strUser[256];
    char strTmpUser[256];
    char *pStr, *pEnd, *pDot;
    
    // make a copy of the message, and extract the input USER record
    TagFieldDupl(strMesg, sizeof(strMesg), pMsg->pData);
    TagFieldGetString(TagFieldFind(strMesg,"USER"), strUser, sizeof(strUser),"");
    TagFieldDelete(strMesg, "USER");

    // isolate the USER string
    pStr = strUser;

    //first grab user -- later  parse the rest.
    //first look for . as in .XBL_SUB too --as not just one domain on server now. 
    if ((pEnd = strchr(pStr, '@')) != NULL)
    {
        *pEnd = '\0';
        strcpy (strTmpUser, pStr);
        if ((pDot = strchr(strTmpUser, '.')) != NULL)
        {
            *pDot = '\0';
        }
        TagFieldSetString(strMesg, sizeof(strMesg), "USER", strTmpUser);
    }

    if (pEnd == NULL) //we are done -- just got a name.
        TagFieldSetString(strMesg, sizeof(strMesg), "USER", pStr);
    else
    {
        // isolate the DOMAIN string
        pStr = pEnd+1;
        if ((pEnd = strchr(pStr, '/')) != NULL)
        {
            *pEnd = '\0';
        }
        TagFieldSetString(strMesg, sizeof(strMesg), "DOMN", pStr);

        if (pEnd != NULL)
        {
            // add the RESOURCE string
            pStr = pEnd+1;
            TagFieldSetString(strMesg, sizeof(strMesg), "RSRC", pStr);
        }
    }

    // point message to new data
    pMsg->pData = strMesg;

    //what time sent -- needed for deleting oldest.
    pMsg->uTimeStamp = TagFieldGetNumber(TagFieldFind(pMsg->pData, "TIME"), 0);// Extract the timestamp

    // forward message to message proc
    pMsg->iType = BUDDY_MSG_CHAT;
    (pState->pMesgProc)(pState, pMsg, pState->pMesgData);
}


/*F*************************************************************************************************/
/*!
    \Function    _BuddyApiPresRecv

    \Description
        Process user presence information.

    \Input *pState  - buddy reference pointer
    \Input *pMsg    - buddy message being received
    \Input *pBuddy  - roster entry pointer

    \Output
        int32_t     - indicate if this a refresh(ie.I came online)or if buddy changed state.

    \Version 03/10/03 (JLB)
*/
/*************************************************************************************************F*/
static int32_t _BuddyApiPresRecv(BuddyApiRefT *pState, BuddyApiMsgT *pMsg, BuddyRosterT *pBuddy)
{
    int32_t iIndex;
    uint32_t uAttrFlags =0; //for parsing ATTR= flags field.
    char strStatTag[256];
    char strShowTag[64];
    const char *pData;
    int32_t bRefresh =0; //did I restart and hence get Pget, or did buddy change state.
    // clear existing presence data
    pBuddy->iShow = 0;
    pBuddy->uLang ='en';
    pBuddy->strPres[0] = 0;

    // set the show status
    TagFieldGetString(TagFieldFind(pMsg->pData, "SHOW"), strShowTag, sizeof( strShowTag ), "");
    for (iIndex = 0; iIndex < BUDDYAPI_STATUS_SIZE; ++iIndex)
    {
        if (strcmp(_BuddyApi_Status[iIndex], strShowTag) == 0)
        {
            pBuddy->iShow = (char)iIndex;
            break;
        }
    }
    //was it refresh (my initialization) or genuine buddy change state   (CHNG=1),  
    bRefresh =(TagFieldGetNumber(TagFieldFind(pMsg->pData, "CHNG"), 0)==0)?1:0;

    //get the games Title Name - to be used in Cross Game Invites.
    TagFieldGetString(TagFieldFind(pMsg->pData, "TITL"), pBuddy->strTitle, sizeof( pBuddy->strTitle), "");

    //see if this is same product--this is server magic, based on presence string on connect.
    pData = TagFieldFind(pMsg->pData, "PROD");
    if (pData != NULL)
    {
        pBuddy->uFlags |= BUDDY_FLAG_SAME;
        //get the extra presence if its there..
        TagFieldGetString(TagFieldFind(pMsg->pData, "EXTR"), 
               pBuddy->strPresExtra , sizeof( pBuddy->strPresExtra ), "");
    }
    else
    { // clear -just in case was set earlier --issues on switch game.
        pBuddy->uFlags &= ~BUDDY_FLAG_SAME;
        pBuddy->strPresExtra[0] =0;
    }

    TagFieldGetString(TagFieldFind(pMsg->pData, "STAT"), strStatTag , sizeof( strStatTag ), "");
    pBuddy->uPresExFlags = TagFieldGetNumber(TagFieldFind( strStatTag , "EX"), 0);
    
    pBuddy->uFlags &=  ~BUDDY_FLAG_JOINABLE; //clear and set if there..
    uAttrFlags = TagFieldGetFlags(TagFieldFind(pMsg->pData, "ATTR"), 0);
    pBuddy->uFlags |= (uAttrFlags & PGET_ATTR_FLAG_JOINABLE )?  BUDDY_FLAG_JOINABLE : 0;
    
    // look for same string
    TagFieldGetString(pData, pBuddy->strPres, sizeof(pBuddy->strPres), "");
    if (pBuddy->strPres[0] != 0)
    {
        TagFieldGetString(pData, pBuddy->strPres, sizeof(pBuddy->strPres), "");
    }
    else
    {
        //lang in Presence string is defunct, but, for backward compatibility
        // get it using "en" as sub-tag. ie. "en=Presence String"
        pData = TagFieldFind(strStatTag, "en"); 
        if ((pData == NULL) && (pBuddy->iShow != BUDDY_STAT_DISC))
        { // presence is null and not disconnected -- bad news?
            BuddyApiPrintf((pState,"Buddy %s is online in state:%d but, missing presence string, \n", pBuddy->strUser, pBuddy->iShow));
        }
        else
        {
            TagFieldGetString(pData, pBuddy->strPres, sizeof(pBuddy->strPres), "");
        }
    }

    return(bRefresh);
}


/*F*************************************************************************************************/
/*!
    \Function _BuddyApiPresSend

    \Description
        Send the presence information to the server

    \Input *pState  - reference pointer

    \Output int32_t - error code (zero=success, negative=error)

    \Version 01/20/2002 (GWS)
*/
/*************************************************************************************************F*/
static void _BuddyApiPresSend(BuddyApiRefT *pState)
{
    char strPacket[MAX_PACKET];
    char strMesg[MAX_PACKET];
    char strStatTag[MAX_PACKET/2];
    char strPresSame[256];
    

    //any extra flags.. goes into the diff field --as embedded tag..
    TagFieldSetNumber (  pState->strPresDiff, sizeof(pState->strPresDiff), "EX", pState->uPresExFlags);
    

    // put product identifier into different-product string
    if (pState->iProd != 0)
    {
        TagFieldSetToken(pState->strPresDiff, sizeof(pState->strPresDiff), "P", pState->iProd);
    }
     
    // setup the packet
    TagFieldDupl(strPacket, sizeof(strPacket), pState->strPresSend);
    if (pState->strPresDiff[0] != 0)
    {
        TagFieldSetString(strPacket, sizeof(strPacket), "STAT", pState->strPresDiff);
    }

    strcpy(strPresSame," "); //use space as default, empty PROD= causes HLBBudIsSameProduct problems.
    if (pState->strPresSame[0] != 0)
    {
        ds_strnzcpy(strPresSame, pState->strPresSame, sizeof (strPresSame) );
    }
    TagFieldSetString(strPacket, sizeof(strPacket), "PROD", strPresSame);

    //extra presence string --as a tag field.
    if (pState->strPresExtra[0]!= 0)
    {
        TagFieldSetString(strPacket, sizeof(strPacket), "EXTR", pState->strPresExtra);
    }

    //send the "no text" attribute "D" if client does not support messages.
    if (pState->bPresNoText == 1)
    {
        TagFieldSetString(strPacket, sizeof(strPacket), "ATTR", "D");
    }

    if (pState->bPresJoinable ) //absence of ATTR J on any PSET will clear..
        TagFieldSetString(strPacket, sizeof(strPacket), "ATTR", "J"); //Joinable in Xbox

    //MAJOR HACK for XBL server -- if send some update after game invite it loses the invite.
    //So, set NXBL (no xbl update) to 1 if just voip changed, as that is what changes prior
    // to game. This is to fix NASCAR issue where if 3 invited, on join, 3rd lost invite due to
    // voip state change on second.
    TagFieldDupl(strMesg, sizeof(strMesg), strPacket);

    //cant mess with STAT in place as EX already encoded, so grab STAT, change and replace
    TagFieldGetString(TagFieldFind(strPacket, "STAT"), strStatTag , sizeof( strStatTag ), "");
    if (strStatTag[0])
    { //zap EX fron STAT as this has VOIP and we dont wanna compare this one..
        TagFieldDelete(   strStatTag, "EX");
    }
    TagFieldDelete( strMesg, "STAT"); 
    TagFieldSetString( strMesg, sizeof(strMesg), "STAT",strStatTag);

    if (strcmp (strMesg, pState->strSavedPresPacket) == 0)
    {//all same, MEANS ONLY  EX --aka voip state CHANGED
        TagFieldSetNumber(strPacket, sizeof(strPacket), "NXBL", 1); //dont update xbl server
    }
    //save for next time..
    TagFieldDupl (pState->strSavedPresPacket, sizeof(pState->strSavedPresPacket), strMesg);

    // send our presence
    _BuddyApiTransact(pState, 'PSET', strPacket);
    pState->uPresTick = 0;

}


/*F*************************************************************************************************/
/*!
    \Function    _BuddyApiStatus

    \Description
        Set the status

    \Input *pState - buddy reference pointer
    \Input iStat   - the updated status

    \Version 03/20/03 (GWS)
*/
/*************************************************************************************************F*/
static void _BuddyApiStatus(BuddyApiRefT *pState, int32_t iStat)
{
    BuddyApiMsgT msg;
    
    #if DIRTYCODE_LOGGING
    const char * strOld = _BuddyApi_Xlate_Stat[  pState->iStat];//.pState;
    const char * strNew = _BuddyApi_Xlate_Stat[  iStat]; //.pState;
    #endif
    
    BuddyApiPrintf( (pState, "STATE TX, ticks %d,  Old State, %s NEW State :%s \n",  ProtoAriesTick(pState->pAries),  strOld, strNew));

    if (pState->iStat == iStat)
        return; //nothing to do; Dont annoy clients by calling twice

    // save the new status
    pState->iStat = iStat;
    
    // do callback if needed 
    // RECN  --reconnect is internal state; dont tell caller if Reconnecting..
    if ((pState->pConnProc != NULL)  &&  ( pState->iStat != BUDDY_SERV_RECN ) )
    {
        msg.iType = BUDDY_MSG_CONNECT;
        msg.iKind = pState->iStat;
        msg.iCode = 0;
        msg.pData = NULL;
        (pState->pConnProc)(pState, &msg, pState->pConnData);
    }
}


// setup for callback
static int32_t _BuddyApiSetCallback(BuddyApiRefT *pState, int32_t iType, BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout)
{
    uint32_t uTimeout = 0;

    // calc the timeout value
    if (iTimeout > 0)
    {
        uTimeout = ProtoAriesTick(pState->pAries)+iTimeout* 1000; 
    }

    // populate the structure
    pState->Callback[iType].iType = _Buddy_CBType[iType];
    pState->Callback[iType].iSeqn = ++(pState->iSeqn);
    pState->Callback[iType].pCallData = pCalldata;
    pState->Callback[iType].pCallProc = pCallback;
    pState->Callback[iType].uTimeout = uTimeout;
    return(pState->Callback[iType].iSeqn);

}

//! add to list based on roster resp (init), and return Id of the message.
static int32_t  _BuddyApiHandleROSTMsg(BuddyApiRefT *pState, BuddyApiMsgT msg)
{
    int32_t iId;
    int32_t iIdx;
    char strTemp[32];
    BuddyRosterT *pBuddy = NULL;
    DispListT *pRec;

    //can get this, internally, before init, so, make sure lists are created..
    _createBuddyDispList (pState);

    // figure appropriate buddy/ignore flag
    iIdx = 0;
    iId = TagFieldGetNumber(TagFieldFind(msg.pData, "ID"), 0);
    if (iId == ROSTER_BUDDY)
    {
        iIdx = BUDDY_FLAG_BUDDY;
    }
    if (iId == ROSTER_IGNORE)
    {
        iIdx = BUDDY_FLAG_IGNORE;
    }

    

    // extract name and look for matching record
    TagFieldGetString(TagFieldFind(msg.pData, "USER"), strTemp, sizeof(strTemp), "");
    // remove domain and resource
    _BuddyApiTruncUsername(strTemp);

    // make sure username is valid

    if (strlen (strTemp) <= 0 )
    { //bad bud -- protect from bad server..
        return(iId);
    }
    //BuddyApiPrintf((pState, " BUDDY ROST MSG %s ", strTemp));
    if (pState->bPeriod || (strchr(strTemp, '.') == NULL))
    {
        // find user record
        pRec = HashStrFind(pState->pHash, strTemp);
        if ((pRec == NULL) && (strlen(strTemp) < sizeof(pBuddy->strUser)))
        {
            // new one, so alloc and add it
            //BuddyApiPrintf((pState, " BUDDY ROST MSG-- adding bud %s ", strTemp));
            pBuddy = pState->pBuddyAlloc(sizeof(*pBuddy), BUDDYAPI_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            if (pBuddy == NULL)
            {
                return(-1); //out of mem...
            }
             
            memset(pBuddy, 0, sizeof(*pBuddy));
            pBuddy->uLang = LOBBYAPI_LANGUAGE_ENGLISH; //until server returns it
            strcpy(pBuddy->strUser, strTemp);
            pRec = DispListAdd(pState->pDisp, pBuddy);
            HashStrAdd(pState->pHash, pBuddy->strUser, pRec);
        }

        // update existing or new buddy  record
        if ((pRec != NULL) && ((pBuddy = DispListGet(pState->pDisp, pRec)) != NULL))
        {

            // If refreshing the roster and buddy already had pending and 
            // accepted while we were passive then we need to show that.
            pBuddy->uFlags &= ~BUDDY_FLAG_BUD_REQ_SENT;
            pBuddy->uFlags &= ~BUDDY_FLAG_BUD_REQ_RECEIVED;

            //TagFieldGetString(TagFieldFind(msg.pData, "GROUP"), pBuddy->strGroups, sizeof(pBuddy->strGroups), "");
            if (iIdx != 0)
            {
                pBuddy->uFlags |= iIdx;
                //pBuddy->uFlags &= ~BUDDY_FLAG_TEMP; //DONT--can still be a temp bud.
            }
            //get and also set flags from the "ATTR" field, if present.
            pBuddy->uFlags |=  _BuddyApiFlagsFromAttr(pState, msg);

            //special case -- if sent or received a wanna be bud request, he is NOT my buddy.
            if  ( ((pBuddy->uFlags & BUDDY_FLAG_BUD_REQ_SENT)  > 0)   ||
                  ((pBuddy->uFlags & BUDDY_FLAG_BUD_REQ_RECEIVED)  > 0)   )
            {
                pBuddy->uFlags &= ~(BUDDY_FLAG_BUDDY); //clear it --Not a  true bud.
            }
        }
    }
    return(iId);
}

//! Parse and process a Presence Message.
static void _BuddyApiHandlePGETMsg ( BuddyApiRefT *pState, BuddyApiMsgT msg)
{
    char strTemp[32];
    DispListT *pRec;
    int32_t bRefresh =0;
    int32_t iPrevShow =0; //buddy chat/offline state prior to this msg.
    // extract name and look for matching record
    TagFieldGetString(TagFieldFind(msg.pData, "USER"), strTemp, sizeof(strTemp), "");

    // remove domain and resource
    _BuddyApiTruncUsername(strTemp);

    pRec = HashStrFind(pState->pHash, strTemp);
    if (pRec != NULL)
    {
        BuddyRosterT PrevBuddy;

        // retrieve buddy record
        BuddyRosterT *pBuddy = DispListGet(pState->pDisp, pRec);

        // make a copy
        ds_memcpy_s(&PrevBuddy,sizeof(PrevBuddy),pBuddy,sizeof(*pBuddy));

        // update the presence
        bRefresh = _BuddyApiPresRecv(pState, &msg, pBuddy);
        
        //If a full state refresh, previous buddy state is meaninless, set to -1.
        //LobbyGlue/ticker depend on this to not show extraneous "buddy online" msgs on restart.
        iPrevShow = (bRefresh==1)? BUDDY_STAT_UNDEFINED : PrevBuddy.iShow;

        // if buddy has changed, update display list, and call change c/b.
        if (memcmp(&PrevBuddy,pBuddy,sizeof(PrevBuddy)))
        {
            DispListDirty(pState->pDisp,TRUE);
            BuddyApiPrintf((pState, "PRES: buddy=%s, show=%d,  pres=%s, lang=%d, exFlags=%d\n",
                pBuddy->strUser, pBuddy->iShow,  pBuddy->strPres, pBuddy->uLang, pBuddy->uPresExFlags));
            //call change proc unconditionally, 
            if (pState->pBuddyChangeProc  )
                (pState->pBuddyChangeProc)(pState, pBuddy, BUDDY_MSG_PRESENCE,  iPrevShow, pState->pBuddyChangeData);

        }
        // if this buddy had a game invite pending, then, call the cb
        {
            if (pState->pBuddyChangeUser[0] != 0) 
            { //have outstanding gameinvite ready c/b.
                if (strcmp (pState->pBuddyChangeUser, strTemp)==0)
                {  //now  set the flag, as ready AND logged in to EA.
                    BuddyApiPrintf((pState, "PGET setting INVITE_READY flag for buddy %s, flag=%x\n", pBuddy->strUser,pBuddy->uFlags));
                    pBuddy->uFlags |= BUDDY_FLAG_GAME_INVITE_READY;
                    if (pState->pBuddyChangeProc) 
                        (pState->pBuddyChangeProc)(pState, pBuddy, BUDDY_MSG_GAME_INVITE, BUDDY_FLAG_GAME_INVITE_READY, pState->pBuddyChangeData);
                    pState->pBuddyChangeUser[0]=0;
                }
            }
        }
    }
}

//! Handle a server acknowlegement to a msg/command sent.  
static void _processAck (BuddyApiRefT *pState, BuddyApiMsgT msg, int32_t cbType )
{
    // check   response  acknowledgement and call callback
    if  (pState->Callback[ cbType].pCallProc != NULL)
    {
        if (TagFieldGetNumber(TagFieldFind(msg.pData, "ID"), 0) 
            == pState->Callback[cbType].iSeqn)
        {
            (pState->Callback[cbType].pCallProc)(pState, &msg, 
                pState->Callback[cbType].pCallData);
            pState->Callback[cbType].pCallProc = NULL;
            pState->Callback[cbType].uTimeout = 0;
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiDisconnect

    \Description
        Disconnect from the buddy server

    \Input *pState      - reference pointer

    \Output int32_t - error code (zero=success, negative=error)

    \Version 01/20/2002 (GWS)
*/
/*************************************************************************************************F*/
int32_t BuddyApiDisconnect(BuddyApiRefT *pState )
{
    //return(0); //XXXX
    // done with dns
    if (pState->pHost != NULL)
    {
        pState->pHost->Free(pState->pHost);
        pState->pHost = NULL;
    }

    // tell peer we are gone
    _BuddyApiTransact(pState, 'DISC', "");

    // unconnect and idle
    ProtoAriesUnconnect(pState->pAries);
    if (pState->iStat == BUDDY_SERV_ONLN)
    {
        // disconnect before going idle
        _BuddyApiStatus(pState, BUDDY_SERV_DISC);
    }
    _BuddyApiStatus(pState, BUDDY_SERV_IDLE);
    
    // flush the list
    BuddyApiFlush(pState);

    if (  pState->bReconnect ==0 )
    {
        pState->pConnProc = NULL; //--otherwise, needed for re-connect..
    }
    return(BUDDY_ERROR_NONE);
}

static int32_t _BuddyApiSendMsg(BuddyApiRefT *pState,int32_t msgType, const char *pMesg, BuddyApiCallbackT *pCallback, 
                     void *pCalldata, int32_t iTimeout)
{
    // see if the budy length exceeds the maximum supported
    // (this is to prevent applications for accidentally overruning each others limits)
    if (TagFieldGetString(TagFieldFind(pMesg, "BODY"), NULL, 0, "") > BUDDY_EASO_BODYLEN)
    {
        return(BUDDY_ERROR_BADPARM);
    }
    
    // see if the subject length exceeds the maximum supported
    // (this is to prevent applications for accidentally overruning each others limits)
    if (TagFieldGetString(TagFieldFind(pMesg, "SUBJ"), NULL, 0, "") > BUDDY_EASO_SUBJLEN)
    {
        return(BUDDY_ERROR_BADPARM);
    }
    if (msgType == 'SEND')
        _BuddyApiSetCallback(pState, CB_SEND, pCallback, pCalldata, iTimeout);
    // save the callback info
    if (msgType == 'BRDC')
        _BuddyApiSetCallback(pState, CB_BROADCAST, pCallback, pCalldata, iTimeout);


    // send the request
    _BuddyApiTransact(pState,  msgType , pMesg);

    return(BUDDY_ERROR_NONE);
}

//! send an Invite Response.
static int32_t  _BuddyApiSendResponse ( BuddyApiRefT *pState , BuddyRosterT * pBuddy,
                                        int32_t  iKind, const char * strAnswer, int32_t bPres, int32_t iId)
{
    char strPacket[250];
    TagFieldClear(strPacket, sizeof(strPacket));
    TagFieldSetString(strPacket, sizeof(strPacket), "LRSC",   pState->strDomain);
    if ( iKind =='GRSP' )
    { //server does not like RSRC on buddy invite stuff..needed for game invite only
        TagFieldSetString(strPacket, sizeof(strPacket), "RSRC", pState->pRsrc); // pState->strDomain);
    }
    TagFieldSetNumber(strPacket, sizeof(strPacket), "ID", iId);
    TagFieldSetString(strPacket, sizeof(strPacket), "USER", pBuddy->strUser);
    if (strlen (strAnswer) > 0  )
    {
        TagFieldSetString(strPacket, sizeof(strPacket), "ANSW", strAnswer);
    }
    if ((bPres) == 1  )
    { //asked for presence so set .. it defaults to No
        TagFieldSetString(strPacket, sizeof(strPacket), "PRES", "Y");
    }
    _BuddyApiTransact ( pState, iKind, strPacket);
    return(0);
}


static BuddyRosterT *_BuddyApiAddOrSetFlags (BuddyApiRefT *pState, BuddyRosterT *pBuddy)
{
    BuddyRosterT *pOwned = NULL;
    DispListT *pRec;
    // make sure there is a list
    if (pState->pDisp != NULL)
    {
        // try and locate an existing record
        pRec = HashStrFind(pState->pHash, pBuddy->strUser);
        if (pRec == NULL)
        {
            // allocate a copy
            pOwned = pState->pBuddyAlloc(sizeof(*pOwned), BUDDYAPI_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            if (pOwned == NULL)
            {
                return(NULL);
            }
            pOwned->uLang = LOBBYAPI_LANGUAGE_ENGLISH; //until server returns it.
            ds_memcpy_s(pOwned, sizeof(*pOwned), pBuddy, sizeof(*pBuddy));
            // add record to roster
            pRec = DispListAdd(pState->pDisp, pOwned);
            HashStrAdd(pState->pHash, pOwned->strUser, pRec);
        }
        else
        { //its alread there
            pOwned = DispListGet(pState->pDisp, pRec);
        }
        // update the (set) new flags  //Wont CLEAR unset flags, client has to do.
        pOwned->uFlags |= pBuddy->uFlags;
        // DONT clear the transaction number by default
        // XXX cause problems with Race condition on RNOT and RADM resp.
        //pOwned->iPend = 0;
    }
    return(pOwned);
}

static uint32_t _BuddyApiDeleteIfNeeded (BuddyApiRefT *pState, char * pName, unsigned uClearFlags)
{
    uint32_t uClearedFlags =0;
    DispListT *pRec;
    BuddyRosterT *pOwned;
    // try and locate an existing record
    pRec = HashStrFind(pState->pHash, pName);
    if (pRec != NULL)
    {
        // remove the flags
        pOwned = DispListGet(pState->pDisp, pRec);
        uClearedFlags = (pOwned->uFlags & uClearFlags);
        pOwned->uFlags ^= uClearedFlags;

        // see if all the real buddy flags were cleared
        //if ((pOwned->uFlags & (BUDDY_FLAG_BUDDY|BUDDY_FLAG_IGNORE|BUDDY_FLAG_TEMP)) == 0)
        if ((pOwned->uFlags & BUDDY_MASK_KEEP_BUDDY ) == 0)            
        {
            // yep -- delete the record
            HashStrDel(pState->pHash, pName);
            pOwned = DispListDel(pState->pDisp, pRec);
            //give caller a chance to free mem, if needed.
            if (pState->pBuddyDelProc )
            {
                pState->pBuddyDelProc ( pState,pOwned, pState->pBuddyDelData);
            }
            pState->pBuddyFree(pOwned, BUDDYAPI_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        }
    }
    return(uClearedFlags);
}

static int32_t _BuddyApiHandleGNOTMsg ( BuddyApiRefT *pState, BuddyApiMsgT msg)
{
    /*
    GNOT comes from server, telling if someone invited me or revoked, or, 
     if someone I invited, accepted or declined.. 
    */
    

    int32_t rc = BUDDY_ERROR_NOTUSER;
    char strUser[65];
    char strTemp[32];
    char strSEID[ BUDDY_SESSION_ID_LEN +1];
    int32_t eAction = BUDDY_INVITE_RESPONSE_UNKNOWN_MAX;
    BuddyRosterT *pBuddy ;
    char cAction;

    TagFieldGetString(TagFieldFind(msg.pData, "TYPE"), strTemp , sizeof( strTemp)-1, "");
    cAction = strTemp[0];
    if (( cAction =='I') || (cAction =='R')) //host invited or revoked
    {
        TagFieldGetString(TagFieldFind(msg.pData, "HOST"), strUser, sizeof(strUser), "");
        //grab ps2 game session id too..particularly for invites..not relevant for revoke?
        TagFieldGetString(TagFieldFind(msg.pData, "GSTR"), strSEID, sizeof(strSEID), "");
    }
    else if ((cAction =='A') || (cAction =='D')) //bud accepted or declined
        TagFieldGetString(TagFieldFind(msg.pData, "USER"), strUser, sizeof(strUser), "");
    else
        return(rc);

    _BuddyApiTruncUsername(strUser);

    if ( ( pBuddy = BuddyApiFind ( pState, strUser)) == NULL )
        return(rc);

    if (cAction =='I') //invite
    {
        eAction  = BUDDY_FLAG_GAME_INVITE_RECEIVED;
        pBuddy->uFlags |= eAction;
        ds_strnzcpy(pBuddy->strSEID,strSEID,sizeof(pBuddy->strSEID));
    }
    else if (cAction =='A') //invite
    {
        eAction  = BUDDY_FLAG_GAME_INVITE_ACCEPTED;
        pBuddy->uFlags |= eAction;
    }
    else if (cAction =='D') // decline aka reject
    {
        eAction  = BUDDY_FLAG_GAME_INVITE_REJECTED;
        //pBuddy->uFlags |= eAction; --no more, reject means it goes away.
        pBuddy->uFlags &= ~BUDDY_FLAG_GAME_INVITE_SENT; 
    }
    if (cAction =='R') //revoke or delete... lets clear the fact that it came..
    {   
        eAction  = BUDDY_FLAG_GAME_INVITE_REVOKE;
        pBuddy->uFlags &= ~BUDDY_FLAG_GAME_INVITE_RECEIVED;
        pBuddy->strSEID[0] =0; //Game session no longer  makes sense, so clear to be safe..
    }   
    if (cAction =='G') //game ready state -- back online in my game..
    {   
        eAction  = BUDDY_FLAG_GAME_INVITE_READY;
        pBuddy->uFlags |=  eAction;
    }   

    if (pState->pBuddyChangeProc != NULL)  
    { // tell caller of game notification state change.
        (pState->pBuddyChangeProc)(pState, pBuddy, BUDDY_MSG_GAME_INVITE, eAction, pState->pBuddyChangeData);
    }
    rc = BUDDY_ERROR_NONE;
    return(rc);
}

static int32_t _BuddyApiHandleRNOTMsg(BuddyApiRefT *pState, BuddyApiMsgT msg)
{
    /*
    RNOT comes from server, telling if one invites, accepts, or rejects A buddy.
    If I, fiso001 requests fiso002 to be my buddy, 
    I get:        RNOT USER=fiso002 CHNG=A, ATTR=S(ent)
    fiso002 gets: RNOT USER=fiso001 CHNG=A, ATTR=R(eceived)
    If he accepts, I get RNOT  CHNG=X with no attributes, and so does he.
    If he rejects, I get RNOT CHANG=D --to delete, so does he.
    If I revoke the invite, I get  RNOT CHNG=D , and so does he.
    If he is a fully committed buddy, and removes me, I get RNOT CHNG=D, and so does he.
    */
    char strTemp[255];
    int32_t rc = 0;
    char cCHNGFlag;
    BuddyRosterT *pOwned;
    BuddyRosterT buddy ;
    memset (&buddy, 0, sizeof (buddy));
    
    // NOTE: Because server was too slow with RNOT, we did setting of local flags in 
    // BuddyApiRespondBuddy when response was sent. This does not hurt that, but, for
    // "accept"/reject/block op, is kind of redundant here.

    TagFieldGetString(TagFieldFind(msg.pData, "USER"), strTemp, sizeof(strTemp), "");
    // extract name and  resource --which is domain/sub-domain
    _BuddyApiTruncUsername(strTemp);
    
    strncpy(buddy.strUser, strTemp, BUDDY_NAME_LEN);
    buddy.strUser[BUDDY_NAME_LEN] = 0;

    TagFieldGetString(TagFieldFind(msg.pData, "CHNG"), strTemp, sizeof(strTemp), "Z");
    cCHNGFlag = strTemp[0];
    if (cCHNGFlag == 'D')
    { 
        //RACE:check for a Delete followed by rapid Add on same bud. RNOT comes after RADM sent.
        if (   ((pOwned = BuddyApiFind ( pState, buddy.strUser )) != NULL )
            && ( (pOwned->uFlags & BUDDY_FLAG_PEND) && (pOwned->uFlags & BUDDY_FLAG_BUD_REQ_SENT )) )
        {//race condition, got RNOT while RADM is pending.. DONT zap.
                BuddyApiPrintf(( pState, "RNOT/RADM--Race Cond? Have RDAM outstanding so ignore RNOT (D)\n"));
        }
        else
        {
            //clear the buddy related flags, and he will get zapped if all flags are cleared.
            unsigned uFlags = BUDDY_FLAG_BUDDY | BUDDY_FLAG_BUD_REQ_SENT | BUDDY_FLAG_BUD_REQ_RECEIVED;
            _BuddyApiDeleteIfNeeded ( pState, buddy.strUser, uFlags);
        }
    }
    else 
    { //X or A
        //Get the attribute flags, set em and add/change buddy in list       
        buddy.uFlags |=  _BuddyApiFlagsFromAttr(pState, msg);

        //NOTE: X is change case, but on xbox, due their server not seeing  my 
        //my invited buddies, we get an A when bud accepted me--treat it like  an X.
        if (( cCHNGFlag == 'X') ||  ( cCHNGFlag == 'A') )
        { //change case
            //  check special case where invite sent/recd set, and now gone --means my bud now.
            if ( (pOwned = BuddyApiFind ( pState, buddy.strUser )) != NULL )
            {
                if  (  (((pOwned->uFlags & BUDDY_FLAG_BUD_REQ_SENT )  > 0) &&
                         ((buddy.uFlags & BUDDY_FLAG_BUD_REQ_SENT )  == 0)   )
                    ||
                       (((pOwned->uFlags & BUDDY_FLAG_BUD_REQ_RECEIVED )  > 0) &&
                        ((buddy.uFlags & BUDDY_FLAG_BUD_REQ_RECEIVED )  == 0)   ) )
                { //now a true buddy..
                    buddy.uFlags |= BUDDY_FLAG_BUDDY; 
                    BuddyApiPrintf((pState, "BUDDY BUDDY: RNOT, flag:%x MAKING:%s  Buddy", pOwned->uFlags,  buddy.strUser));
                    //clear existing pending flags, as AddOrSetFlags will only set new flags.
                    pOwned->uFlags &= ~BUDDY_FLAG_BUD_REQ_RECEIVED; //clear both wannabes..
                    pOwned->uFlags &= ~BUDDY_FLAG_BUD_REQ_SENT; //clear both wannabes..
                }
            }
        }
        //now, set the new flags and add to list if needed.
        if ( (pOwned = _BuddyApiAddOrSetFlags (pState, &buddy))== NULL)
            rc= BUDDY_ERROR_NOTUSER; //buddy not there, and not added -- a bug.
    } //X or A
    return(rc);
}

//! Deal with response to finduser --which is  one "USER"  msg for each user found.
static int32_t  _BuddyApiHandleSearchedUserMsg(BuddyApiRefT *pState, BuddyApiMsgT msg)
{
    int32_t rc =-1;
    char strTemp[100];

    if ( TagFieldGetNumber(TagFieldFind(msg.pData, "ID"), 0)  == USCH_FIXED_ID)
    {
        int32_t i;
        rc =0; //assume ok if process the msg, as it has to count ALL responses..
        
        // extract name and  resource --which is domain/sub-domain
        TagFieldGetString(TagFieldFind(msg.pData, "USER"), strTemp, sizeof(strTemp), "");
        _BuddyApiTruncUsername(strTemp);
        for ( i=0;  i <= BUDDY_MAX_USERS_FOUND ; i++ )
        {
            if (pState->foundUsers[i].name[0] == 0 )
                break; //found empty slot.
        }
        //  iFoundUsers = i+1; //how many found so far..
        if ( i < BUDDY_MAX_USERS_FOUND )
        {
            ds_strnzcpy (pState->foundUsers[i].name, strTemp, sizeof  (pState->foundUsers[i].name )  );
            TagFieldGetString(TagFieldFind(msg.pData, "RSRC"), strTemp, sizeof(strTemp), "");
            //convert resource in domain and product.
            _BuddyApiParseResource ( strTemp,BUDDY_DOMAIN_LEN,  pState->foundUsers[i].domain,
                BUDDY_PRODUCT_LEN, pState->foundUsers[i].product); 

        }
    }
    return(rc);
}



/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function BuddyApiCreate

    \Description
        Create a new buddy interface client

    \Input *pAlloc                  - alloc function to use (optional)
    \Input *pFree                   - free function to use (optional)
    \Input *pProduct                - product id (product-platform-year)
    \Input *pRelease                - product release (year4-month2-day2)
    \Input iRefMemGroup             - mem group to use for ref (zero=default)
    \Input *pRefMemGroupUserData    - pointer to user data associated with mem group

    \Output BuddyApiRefT *          - state pointer (pass to other functions) (NULL=failed)

    \Version 01/20/2002 (GWS)
*/
/*************************************************************************************************F*/
BuddyApiRefT *BuddyApiCreate2(BuddyMemAllocT *pAlloc,  BuddyMemFreeT *pFree,
                              const char *pProduct, const char *pRelease, 
                              int32_t iRefMemGroup, void *pRefMemGroupUserData)
{
    BuddyApiRefT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // if refmemgroup is unspecified, use current
    if (iRefMemGroup == 0)
    {
        iRefMemGroup = iMemGroup;
        pRefMemGroupUserData = pMemGroupUserData;
    }

    // if alloc/free are unspecified, use defaults
    if ((pAlloc == NULL ) || (pFree == NULL))
    { 
        pAlloc = DirtyMemAlloc;
        pFree = DirtyMemFree;
    }
    
    // allocate a new state record
    pState = DirtyMemAlloc(sizeof(*pState), BUDDYAPI_MEMID, iRefMemGroup, pRefMemGroupUserData);

    if (pState == NULL)
    { //alloc failed, not much we can do..
        return(NULL);
    }

    memset(pState, 0, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    pState->iRefMemGroup = iRefMemGroup;
    pState->pRefMemGroupUserData = pRefMemGroupUserData;
    pState->pBuddyAlloc = pAlloc;
    pState->pBuddyFree = pFree;

    // save the product
    TagFieldDupl(pState->strProduct, sizeof(pState->strProduct), pProduct);
    // save the version
    TagFieldDupl(pState->strRelease, sizeof(pState->strRelease), pRelease);

    // mark persistent modules with ref memgroup
    DirtyMemGroupEnter(iRefMemGroup, pRefMemGroupUserData);

    // create virtual session handler
    pState->pAries = ProtoAriesCreate(8192);
    if (pState->pAries == NULL)
    { //mem alloc failed, return now or will blow up on Update..
        BuddyApiPrintf( ( pState, "ProtoAriesCreate return NULL, mem alloc issue?\n"));
        return(NULL);
    }

    ProtoAriesSecure(pState->pAries, 0);
    // reset the sequence number
    pState->iSeqn = 100;  //low numbers are reserved.
    pState->uForwardFlags |= BUDDY_FORWARD_FLAG_UNKNOWN;

    // done with ref memgroup label
    DirtyMemGroupLeave();

    // return reference pointer
    return(pState);
}


/*F*************************************************************************************************/
/*!
    \Function BuddyApiDestroy

    \Description
        Destroy an existing client

    \Input *pState  - reference pointer

    \Version 01/20/2002 (GWS)
*/
/*************************************************************************************************F*/
void BuddyApiDestroy(BuddyApiRefT *pState)
{
    // done with dns
    if (pState->pHost != NULL)
    {
        pState->pHost->Free(pState->pHost);
        pState->pHost = NULL;
    }
    BuddyApiFlush(pState); //just in case disp list still around.

    // disconnect
    BuddyApiDisconnect(pState); 

    // flush the buddy list
    BuddyApiFlush(pState);
    // done with protocol
    ProtoAriesDestroy(pState->pAries);
    // done with memory
    DirtyMemFree(pState, BUDDYAPI_MEMID, pState->iRefMemGroup, pState->pRefMemGroupUserData);
}


/*F*************************************************************************************************/
/*!
    \Function BuddyApiStatus

    \Description
        Return the current connection status

    \Input *pState     - reference pointer

    \Output BUDDY_SERV_xxxx

    \Version 03/20/2002 (GWS)
*/
/*************************************************************************************************F*/
int32_t BuddyApiStatus(BuddyApiRefT *pState)
{
    return(pState->iStat);
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiResource

    \Description
         Return the resource name set in  this api.

    \Input *pState     - reference pointer

    \Output char * - Resource String .
*/
/*************************************************************************************************F*/
char * BuddyApiResource(BuddyApiRefT *pState)
{
    return(pState->pRsrc );
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiDebug

    \Description
        Assign the debug callback

    \Input *pState     - reference pointer
    \Input *pDebugData - user data for callback
    \Input *pDebugProc - user callback

    \Version 01/20/2002 (GWS)
*/
/*************************************************************************************************F*/
void BuddyApiDebug(BuddyApiRefT *pState, void *pDebugData, void (*pDebugProc)(void *pData, const char *pText))
{
    // save the debug code
    pState->pDebugData = pDebugData;
    pState->pDebugProc = pDebugProc;
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiConfig

    \Description
         // Config the language, initial same product identifier and locale for translating title name.

    \Input *pState      - reference pointer
    \Input *pSame       - "same product" identifier string
    \Input  uLocality   - This users locale, used for translating title names.

    \Output int32_t     - error code (zero=success, negative=error)

    \Version 01/20/2002 (GWS)
*/
/*************************************************************************************************F*/
int32_t BuddyApiConfig(BuddyApiRefT *pState, const char *pSame, uint32_t uLocality)
{
    // save same-product string
    TagFieldDupl(pState->strSame, sizeof(pState->strSame), pSame);

    // save product identifier
    pState->iProd = 0; // not used iProd;
    pState->uLocale = uLocality; // locale will give us language..
    pState->bReconnect = 1;
    // save namespace flag
    pState->bPeriod = 1;
    return(BUDDY_ERROR_NONE);
}


/*F*************************************************************************************************/
/*!
    \Function BuddyApiConnect

    \Description
        Connect to the buddy server

    \Input *pState      - reference pointer
    \Input *pServer     - buddy server hostname
    \Input iPort        - buddy server port
    \Input iTimeout     - connection timeout in seconds; cannot be zero if reconnect required.
    \Input *pLKey       - session key (or username:password for dev)
    \Input *pRsrc       - Resource (domain/sub-domain) uniquely identified product.
    \Input *pCallback   - completion callback function
    \Input *pCalldata   - callback data

    \Output int32_t - error code (zero=success, negative=error)

    \Version 01/20/2002 (GWS)
*/
/*************************************************************************************************F*/
int32_t BuddyApiConnect(BuddyApiRefT *pState, const char *pServer, int32_t iPort, int32_t iTimeout, const char *pLKey, const char *pRsrc, BuddyApiCallbackT *pCallback, void *pCalldata)
{
    char *pDiv;
    char strUser[256];
    char strTemp[256];
    
    BuddyApiDisconnect(pState);
    pState->pConnData = pCalldata;
    pState->pConnProc = pCallback;
    //save key server and port info if need to re-connect.
    pState->iPort = iPort;
    pState->iConnectTimeout = iTimeout;
    ds_strnzcpy (pState->pServer , pServer, sizeof (pState->pServer ));
    ds_strnzcpy (pState->pLKey, pLKey, sizeof (pState->pLKey ));
    ds_strnzcpy (pState->pRsrc, pRsrc, sizeof (pState->pRsrc ));
    // get a seed from lkey or other..
    if (pState->uRandom ==0 )
        pState->uRandom =  _BuddyApiExtractRandomSeed (pState);
    // authenticate the user
    TagFieldClear(pState->strLogin, sizeof(pState->strLogin));
    TagFieldSetString(pState->strLogin, sizeof(pState->strLogin), "PROD", pState->strProduct);
    TagFieldSetString(pState->strLogin, sizeof(pState->strLogin), "VERS", pState->strRelease);
    TagFieldSetString(pState->strLogin, sizeof(pState->strLogin), "PRES", pState->strSame);

    LOBBYAPI_LocalizerTokenCreateLocalityString(strTemp, pState->uLocale); 
    TagFieldSetString(pState->strLogin, sizeof(pState->strLogin), "LOCL", strTemp);    
    BuddyApiPrintf((pState, "*****Locale  :%s: \n", strTemp));  

    // pass lkey or user:pass combo
    if (strchr(pLKey, ':') == NULL)
    {
        TagFieldSetString(pState->strLogin, sizeof(pState->strLogin), "USER", pRsrc);
        TagFieldSetString(pState->strLogin, sizeof(pState->strLogin), "LKEY", pLKey);
        
    }
    else
    {
        
        // extract the user:pass combo
        ds_strnzcpy(strTemp, pLKey, sizeof(strTemp));

        // locate the divider
        pDiv = strchr(strTemp, ':');
        if (pDiv != NULL)
        {
            // turn into two strings
            *pDiv++ = 0;

            // make private copy of username
            TagFieldDupl(strUser, sizeof(strUser), strTemp);
            // add the resource if it will fit
            if (strlen(strUser)+strlen(pRsrc) < sizeof(strTemp))
            {
                strcat(strUser, pRsrc);
            }
            // assign the username
            TagFieldSetString(pState->strLogin, sizeof(pState->strLogin), "USER", strUser);
            // use the password or lkey
            if (pDiv[0] == '$')
            {
                TagFieldSetString(pState->strLogin, sizeof(pState->strLogin), "LKEY", pDiv+1);
            }
            else
            {
                TagFieldSetString(pState->strLogin, sizeof(pState->strLogin), "PASS", pDiv);
            }
        }
    }

    // save the domain
    TagFieldDupl(pState->strDomain, sizeof(pState->strDomain), pRsrc);
    if (pState->strDomain[0] == '/')
    {
        strcpy(pState->strDomain, pState->strDomain+1);
    }
    pDiv = strchr(pState->strDomain, '/');
    if (pDiv != NULL)
    {
        *pDiv = 0;
    }

    // To support SSL connections the hostname has to be passed
    // to ProtoAriesConnect in order to do certificate validation.
    // Consequently the ProtoNameAsync call here is now redundant.
    // However it and all its supporting code has been left untouched
    // in order to to minimize the size of the required to add SSL support.
    // At some point pState->pHost and all references to it should be removed.
    //
    if (pServer[0] == '*')
        pServer += 1;

    pState->pHost = ProtoNameAsync(pServer, iTimeout*1000);
    _BuddyApiStatus(pState, BUDDY_SERV_CONN);
     if (iTimeout > 0)
         pState->uTimeout = ProtoAriesTick(pState->pAries)+iTimeout*1000;
     else
        pState->uTimeout =0;

    // make next presence update instant
    pState->iShow = -1;

    return(BUDDY_ERROR_NONE);
}

/*F*************************************************************************************************/
/*!
    \Function _BuddyApiReconnect

    \Description
        Re-Connect to the buddy server, usually after a connect failure or, server shutdown scenario.

    \Input *pState      - reference pointer
    \Input  bImmediate  - If 0, shedule reconnect for later, if true try to connect now.

    \Output int32_t - error code (zero=success, negative=error)
*/
/*************************************************************************************************F*/

int32_t  _BuddyApiReconnect ( BuddyApiRefT *pState, int32_t bImmediate )
{
    int32_t timeout;
    if ( ! pState->bReconnect ) //dont do it.
        return(0);

    
    //16 bit random -- need uniform in range of min .. max
    //not the best for uniform random #, but will do for now..
    timeout = MIN_RECONNECT_TO + (pState->uRandom & 0xFFFF ) % ( MAX_RECONNECT_TO - MIN_RECONNECT_TO );
    // update the random number generator
    pState->uRandom  = (pState->uRandom * 1664525) + 1013904223;
    BuddyApiPrintf( ( pState, "Reconnecting, timer is %d  \n", timeout));
    if (bImmediate )
    {  //time to connect 
        //For timeout--use some randomness in timing out, but, also ensure we wait for server..
        //BuddyApiPrintf( ( pState, "(re-) Connecting to buddy server ... \n" ) ;
        return(BuddyApiConnect( pState, pState->pServer, pState->iPort, 
                  (pState->iConnectTimeout/2 ) + timeout, pState->pLKey, pState->pRsrc, 
                   pState->pConnProc , pState->pConnData));

    }
    else
    {  //schedule it..
        pState->uTimeout = ProtoAriesTick(pState->pAries)+ timeout* 1000; 
        _BuddyApiStatus (pState, BUDDY_SERV_RECN);
    }
    return(0);
}


/*F*************************************************************************************************/
/*!
    \Function BuddyApiFlush

    \Description
        Flush the roster from memory

    \Input *pState  - reference pointer

    \Version 01/20/2002 (GWS)
*/
/*************************************************************************************************F*/
void BuddyApiFlush(BuddyApiRefT *pState)
{
    DispListT *pRec;
    BuddyApiMsgT msg;
    BuddyRosterT *pBuddy;

    // make sure flush is enabled
    if (pState->pDisp != NULL)
    {
        // free the resources
        while ((pRec = HasherFlush(pState->pHash)) != NULL)
        {
            pBuddy =(DispListDel(pState->pDisp, pRec));
            //give caller a chance to free mem, if needed.
            if (pState->pBuddyDelProc )
            {
                pState->pBuddyDelProc (pState, pBuddy, pState->pBuddyDelData);
            }
            pState->pBuddyFree(pBuddy, BUDDYAPI_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        }

        // do callback for any final update
        if (pState->pListProc != NULL)
        {
            // setup the message
            msg.iType = BUDDY_MSG_ROSTER;
            msg.iKind = 0;
            msg.iCode = 0;
            msg.pData = (char *)pState->pDisp;
            // perform the callback
            (pState->pListProc)(pState, &msg, pState->pListData);
        }

        // done with resources
        HasherDestroy(pState->pHash);
        pState->pHash = NULL;
        DispListDestroy(pState->pDisp);
        pState->pDisp = NULL;

        // clear the callback
        pState->pListData = NULL;
        pState->pListProc = NULL;
    }
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiRoster

    \Description
        Get the roster from the server

    \Input *pState      - reference pointer
    \Input *pCallback   - callback
    \Input *pCalldata   - callback data

    \Output Returns a displist reference

    \Version 01/20/2002 (GWS)
*/
/*************************************************************************************************F*/
DispListRef *BuddyApiRoster(BuddyApiRefT *pState, BuddyApiCallbackT *pCallback, void *pCalldata)
{
    char strPacket[256];

    //-- Dont check for dispList null, as dispList can be created if there are invited buddies.
    //--TODO change name to "..LoadRoster"..

    // set the callback
    pState->pListProc = pCallback;
    pState->pListData = pCalldata;

    // allocate the list and make it active
    _createBuddyDispList (pState);

    // request the user forwarding settings..
    TagFieldClear(strPacket, sizeof(strPacket));
    TagFieldSetString(strPacket, sizeof(strPacket), "LRSC", pState->strDomain);
    TagFieldSetNumber(strPacket, sizeof(strPacket), "ID", USERFWD_FIXED_ID);
    _BuddyApiTransact(pState, 'EPGT', strPacket); //email user property get..


    // request the buddy list
    pState->bEndRoster =0; //roster not yet uploaded, so clear till done.
    TagFieldClear(strPacket, sizeof(strPacket));
    TagFieldSetString(strPacket, sizeof(strPacket), "LRSC", pState->strDomain);
    TagFieldSetString(strPacket, sizeof(strPacket), "LIST", "B");
    TagFieldSetString(strPacket, sizeof(strPacket), "PRES", "Y");
    TagFieldSetString(strPacket, sizeof(strPacket), "PEND", "Y"); // new invitees
    TagFieldSetNumber(strPacket, sizeof(strPacket), "ID", ROSTER_BUDDY);
    _BuddyApiTransact(pState, 'RGET', strPacket);

    // request the ignore list
    TagFieldClear(strPacket, sizeof(strPacket));
    TagFieldSetString(strPacket, sizeof(strPacket), "LRSC", pState->strDomain);
    TagFieldSetString(strPacket, sizeof(strPacket), "LIST", "I");
    TagFieldSetString(strPacket, sizeof(strPacket), "PRES", "Y");
    TagFieldSetString(strPacket, sizeof(strPacket), "PEND", "Y"); // new invitees
    TagFieldSetNumber(strPacket, sizeof(strPacket), "ID", ROSTER_IGNORE);
    _BuddyApiTransact(pState, 'RGET', strPacket);

    // mark display list as dirty to get update
    DispListDirty(pState->pDisp, 1);

    return(pState->pDisp);
}


/*F*************************************************************************************************/
/*!
    \Function BuddyApiRosterList 

    \Description
        Return the current roster display list

    \Input *pState  - reference pointer

    \Output Returns a displist reference (NULL=nothing allocated)

    \Version 01/20/2002 (GWS)
*/
/*************************************************************************************************F*/
DispListRef *BuddyApiRosterList(BuddyApiRefT *pState)
{
    return(pState->pDisp);
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiDomain

    \Description
         Return the current Domain, as set in  this api. Domain is the resource minus extra stuff at end 
          that distinguishes servers.

    \Input *pState     - reference pointer

    \Output char * - Domain String .
*/
/*************************************************************************************************F*/
char * BuddyApiDomain(BuddyApiRefT *pState)
{
    return(pState->strDomain);
}

// Ensure the name set conforms to specs.
static int32_t _BuddyApiValidateName ( BuddyApiRefT *pState, char * strName )
{
    //if any of bad is true, return an error.
    return( (
            (strName == NULL )
        ||  (strName[0] == '\0' )
        ||  (strchr( strName, '/') != NULL)
        ||  (strchr( strName, '@') != NULL)
        ||  (!pState->bPeriod && (strchr( strName, '.') != NULL))   
        ) ? BUDDY_ERROR_BADPARM : BUDDY_ERROR_NONE) ;
}


/*F*************************************************************************************************/
/*!
    \Function BuddyApiAdd

    \Description
        Add a user to the buddy roster

    \Input *pState      - reference pointer
    \Input *pBuddy      - buddy record
    \Input *pCallback   - callback
    \Input *pCalldata   - callback data
    \Input iTimeout     - time out

    \Output int32_t     - zero=success, negative=error

    \Version 01/20/2002 (GWS)
*/
/*************************************************************************************************F*/
int32_t BuddyApiAdd(BuddyApiRefT *pState, BuddyRosterT *pBuddy, BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout)
{
    //DispListT *pRec;
    char strPacket[256];
    BuddyRosterT *pOwned;
    int32_t rc = BUDDY_ERROR_MEMORY, iSeq =0;

    // make sure flags are set and username is non-null
    if (((pBuddy == NULL) || (pBuddy->uFlags == 0))  ||
        ( _BuddyApiValidateName ( pState, pBuddy->strUser ) != 0 ) )
    {
        return(BUDDY_ERROR_BADPARM);
    }

    if ((pBuddy->uFlags & BUDDY_FLAG_TEMP) ==0 )            
    {
    // save the callback info --only if adding a perma bud. Do not do so
    // for adding a temp bud -- as can hit race and overide an actual outstanding ADDUSER transaction.
        iSeq = _BuddyApiSetCallback(pState, CB_ADDUSER, pCallback, pCalldata, iTimeout);
    }

    if ( (pOwned = _BuddyApiAddOrSetFlags (pState, pBuddy))!= NULL)
    { //its now in my list ...
        // send request to server if needed

        TagFieldClear(strPacket, sizeof(strPacket));
        TagFieldSetString(strPacket, sizeof(strPacket), "LRSC", pState->strDomain);

            //TagFieldSetString(strPacket, sizeof(strPacket), "LIST", "I");
        TagFieldSetString(strPacket, sizeof(strPacket), "USER", pBuddy->strUser);

        if ( (pBuddy->uFlags  & (BUDDY_FLAG_BUDDY |BUDDY_FLAG_BUD_REQ_SENT |BUDDY_FLAG_IGNORE) )>0)
        {        // by default,  we want a response
            TagFieldSetNumber(strPacket, sizeof(strPacket), "ID",  iSeq);
            pOwned->uFlags |= BUDDY_FLAG_PEND;
            pOwned->iPend = iSeq;
        }
        if (pBuddy->uFlags & BUDDY_FLAG_BUDDY)
        {
            // add to buddy list
            TagFieldSetString(strPacket, sizeof(strPacket), "LIST", "B");
            TagFieldSetString(strPacket, sizeof(strPacket), "PRES", "Y");
            //TagFieldSetString(strPacket, sizeof(strPacket), "GROUP", pBuddy->strGroups);
            _BuddyApiTransact(pState, 'RADD', strPacket);
        }
        if (pBuddy->uFlags & BUDDY_FLAG_IGNORE)
        {
            // add to ignore list
            TagFieldSetString(strPacket, sizeof(strPacket), "LIST", "I");
            _BuddyApiTransact(pState, 'RADD', strPacket);
        }

        if (pBuddy->uFlags & BUDDY_FLAG_BUD_REQ_SENT )
        {
            // add to my invite list...
            TagFieldSetString(strPacket, sizeof(strPacket), "PRES", "Y");
            _BuddyApiTransact(pState, 'RADM', strPacket);  //add-mutual
        }
        
        if (pBuddy->uFlags & BUDDY_FLAG_WATCH)
        {
            // request their presence --by itself, or add of a temp buddy --no ack needed.
            _BuddyApiTransact(pState, 'PADD', strPacket);
            if (   ((pBuddy->uFlags & BUDDY_FLAG_BUDDY) ==0  ) 
                && ((pBuddy->uFlags & BUDDY_FLAG_IGNORE) ==0 ) )
            {//just a temp buddy; Wont get a RADD response, so lets clear t/o.
                //pState->Callback[CB_ADDUSER].uTimeout=0;
            }
        }
        rc = BUDDY_ERROR_NONE;
    } //its there
    return(rc);
}

//! Invite a user to be my buddy.
int32_t BuddyApiBuddyInvite(BuddyApiRefT *pState, BuddyRosterT *pBuddy, BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout)
{
    // send an Invite  -- essentially replaces the old  "add buddy",
    // ideally, should not add buddy to list,  till we get BNOT.  
    pBuddy->uFlags |= BUDDY_FLAG_BUD_REQ_SENT;
    return(BuddyApiAdd(pState,pBuddy,pCallback,pCalldata,iTimeout));
}
//! Invite a user to play Cross Game --Xbox Only!
int32_t BuddyApiGameInvite(BuddyApiRefT *pState, BuddyRosterT *pBuddy, BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout,
                       int32_t iSessionId, const char *pGameInviteState )
{
    int32_t rc=0;
    int32_t iSeq=0;

    // save the callback info, and inc sequence number ... 
    char strPacket[250];
    iSeq = _BuddyApiSetCallback(pState, CB_GAMEINVITE, pCallback, pCalldata, iTimeout);
    pBuddy->uFlags |= BUDDY_FLAG_GAME_INVITE_SENT;

    TagFieldClear(strPacket, sizeof(strPacket));
    TagFieldSetString(strPacket, sizeof(strPacket), "RSRC", pState->pRsrc);
    //TagFieldSetString(pState->strPresSend, sizeof(pState->strPresSend), "RSRC", );
    TagFieldSetNumber(strPacket, sizeof(strPacket), "ID", iSeq);
    TagFieldSetString(strPacket, sizeof(strPacket), "USER", pBuddy->strUser);
    TagFieldSetString(strPacket, sizeof(strPacket), "TITL", pState->strTitleName);
    TagFieldSetNumber(strPacket, sizeof(strPacket), "SESS", iSessionId);
    TagFieldSetString(strPacket, sizeof(strPacket), "GSTR", pState->strSEID);
    _BuddyApiTransact ( pState, 'GINV', strPacket);

    if (rc ==0 )
    {
        pBuddy->uFlags |= BUDDY_FLAG_GAME_INVITE_SENT;
    }
    return(rc);
}

// Join a buddy in another game.
int32_t BuddyApiJoinGame(BuddyApiRefT *pState, BuddyRosterT *pBuddy, 
                    BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout);


//! Join my buddy, wherever he is --he must be in another game and joinable.  
int32_t BuddyApiJoinGame(BuddyApiRefT *pState, BuddyRosterT *pBuddy, BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout)
{
    int32_t rc= BUDDY_ERROR_UNSUPPORTED; //XBox only
    return(rc);
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiDel

    \Description
        Add a user to the buddy roster

    \Input *pState      - reference pointer
    \Input *pBuddy      - buddy record
    \Input *pCallback   - callback
    \Input *pCalldata   - callback data
    \Input iTimeout     - time out

    \Output int32_t     - zero=success, negative=error

    \Version 01/20/2002 (GWS)
*/
/*************************************************************************************************F*/
int32_t BuddyApiDel(BuddyApiRefT *pState, BuddyRosterT *pBuddy, BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout)
{
    int32_t iResult = BUDDY_ERROR_NOTUSER;
    //DispListT *pRec;
    char strPacket[256];
    //BuddyRosterT *pOwned;
    uint32_t uFlags;
    int32_t iSeq;

    // make sure there is a list
    if ((pBuddy != NULL) && (pState->pDisp != NULL))
    {

        if  ( _BuddyApiValidateName ( pState, pBuddy->strUser ) != 0 )
            return(BUDDY_ERROR_BADPARM);

        // save the callback info
        iSeq = _BuddyApiSetCallback(pState, CB_DELUSER, pCallback, pCalldata, iTimeout);
        uFlags = _BuddyApiDeleteIfNeeded( pState, pBuddy->strUser, pBuddy->uFlags);
        if (uFlags != 0 )
        {

            // send request to server if needed
            if (uFlags & BUDDY_FLAG_BUDDY)
            {
                // remove from buddy list
                TagFieldClear(strPacket, sizeof(strPacket));
                TagFieldSetNumber(strPacket, sizeof(strPacket), "ID", iSeq);
                TagFieldSetString(strPacket, sizeof(strPacket), "LRSC", pState->strDomain);
                TagFieldSetString(strPacket, sizeof(strPacket), "LIST", "B");
                TagFieldSetString(strPacket, sizeof(strPacket), "PRES", "Y");
                TagFieldSetString(strPacket, sizeof(strPacket), "USER", pBuddy->strUser);
                //TagFieldSetString(strPacket, sizeof(strPacket), "GROUP", pBuddy->strGroups);              
                _BuddyApiTransact(pState, 'RDEM', strPacket); //new protocal -- delete mutual.

                TagFieldSetNumber(strPacket, sizeof(strPacket), "ID", -1);
                _BuddyApiTransact(pState, 'RDEL', strPacket); //send this too -- for ole server  
            }
            if (uFlags & BUDDY_FLAG_IGNORE)
            {
                // remove from ignore list
                TagFieldClear(strPacket, sizeof(strPacket));
                TagFieldSetNumber(strPacket, sizeof(strPacket), "ID", iSeq);
                TagFieldSetString(strPacket, sizeof(strPacket), "LRSC", pState->strDomain);
                TagFieldSetString(strPacket, sizeof(strPacket), "LIST", "I");
                TagFieldSetString(strPacket, sizeof(strPacket), "USER", pBuddy->strUser);
                _BuddyApiTransact(pState, 'RDEL', strPacket);
            }

            if (uFlags & BUDDY_FLAG_WATCH)
            {
                // request their presence
                TagFieldClear(strPacket, sizeof(strPacket));
                TagFieldSetString(strPacket, sizeof(strPacket), "LRSC", pState->strDomain);
                TagFieldSetString(strPacket, sizeof(strPacket), "USER", pBuddy->strUser);
                _BuddyApiTransact(pState, 'PDEL', strPacket);
            }
            // success
            iResult = BUDDY_ERROR_NONE;
        }
    }

    return(iResult);
}


/*F*************************************************************************************************/
/*!
    \Function BuddyApiFind

    \Description
        Find a user in the buddy roster

    \Input *pState - reference pointer
    \Input *pUser  - name of buddy

    \Output BuddyRosterT * - matching record (NULL=no match)

    \Version 01/20/2002 (GWS)
*/
/*************************************************************************************************F*/
BuddyRosterT *BuddyApiFind(BuddyApiRefT *pState, const char *pUser)
{
    DispListT *pRec;
    BuddyRosterT *pMatch = NULL;

    // find the user
    if ((pState->pHash != NULL) && (pUser != NULL))
    {
        pRec = HashStrFind(pState->pHash, pUser);
        if (pRec != NULL)
        {
            pMatch = DispListGet(pState->pDisp, pRec);
        }
    }
    return(pMatch);
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiFindUsers

    \Description
         Request a search of users, of names starting with the pUserPattern. Min 3 chars.

    \Input *pState          - reference pointer
    \Input *pUserPattern    -  User name pattern to search. Min 3 characters, server add the wildcard.
    \Input *pCallback       - Callback function to be called when search is completed.
    \Input *pCalldata       - User data or context that will be passed back to caller in the callback.
    \Input iTimeout         - timeout in seconds for command to completed. Ignored (no t/o) if 0.

    \Output int32_t - BUDDY_ERROR_NONE if request was sent to server.
*/
/*************************************************************************************************F*/


int32_t  BuddyApiFindUsers(BuddyApiRefT *pState, const char *pUserPattern,
                             BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout)
{
    char strPacket[255];
    if (pUserPattern[0] ==0)
        return(BUDDY_ERROR_BADPARM);

    // search for users  in the whole domain 
    // server responds with USCH (summary)packet and USER   packet for each user.
    memset ( pState->foundUsers,0, sizeof (pState->foundUsers )); //clear results. 
    // save the callback info
     _BuddyApiSetCallback(pState, CB_FINDUSER, pCallback, pCalldata, iTimeout);


     TagFieldClear(strPacket, sizeof(strPacket));
     TagFieldSetNumber(strPacket, sizeof(strPacket), "ID",  USCH_FIXED_ID);  
     TagFieldSetString(strPacket, sizeof(strPacket), "RSRC", pState->strDomain); 
     //note can specify product too -- if only looking for product budds..

     TagFieldSetString(strPacket, sizeof(strPacket), "USER",  pUserPattern); 
     TagFieldSetNumber(strPacket, sizeof(strPacket), "MAXR",  BUDDY_MAX_USERS_FOUND);
     //optional namespace NSPC field. We seek personas..CSO is synonymous with persona

     //set this, so callback does not hit while waiting for USCH response, with actual count.
     pState->iFoundUsersCount =9999;  
     _BuddyApiTransact(pState, 'USCH', strPacket);
     return(0);
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiSetForwarding

    \Description
        Turn email forwarding on or off, and set email address.

    \Input *pState          - pointer to API State.
    \Input uForwardingFlags - turn forwarding on or off  by presence of bits in flag.
    \Input *strEmail        - the email address to set it to. Will be set even if bEnable is off, due to server issue.
    \Input *pCallback *     - callback to get status of operation.
    \Input *pCalldata       - user context for callback.
    \Input iTimeout         - time out

    \Output - int32_t  - Return code-- HLB_ERROR_NONE if ok, HLB_ERROR_BADPARAM if params not there.  
*/
/*********************************************************************************************F*/
int32_t BuddyApiSetForwarding(BuddyApiRefT *pState, unsigned uForwardingFlags, const char * strEmail,
                             BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout)
{
    char strPacket[255];
    int32_t iSeq;
    if ( ( uForwardingFlags &  BUDDY_FORWARD_FLAG_EMAIL ) && 
            (   ( strEmail == NULL ) || strEmail[0] ==0 ) )
        return(BUDDY_ERROR_BADPARM);

    if (strEmail != NULL )
    {
        ds_strnzcpy ( pState->strEmail, strEmail, sizeof (pState->strEmail) );
    }

    pState->uForwardFlags = uForwardingFlags;

    // save the callback info
     iSeq = _BuddyApiSetCallback(pState, CB_SETFWD, pCallback, pCalldata, iTimeout);
     TagFieldClear(strPacket, sizeof(strPacket));
     TagFieldSetNumber(strPacket, sizeof(strPacket), "ID",  iSeq); 
     TagFieldSetString(strPacket, sizeof(strPacket), "RSRC", pState->strDomain); 
     //note can specify product too -- if only looking for product budds..

     if ( (uForwardingFlags &  BUDDY_FORWARD_FLAG_EMAIL )!=0 )
     {
             TagFieldSetString(strPacket, sizeof(strPacket), "ADDR", strEmail); 
             TagFieldSetString(strPacket, sizeof(strPacket), "ENAB", "T"); 
     }
     else //it is cleared ..
     {
             TagFieldSetString(strPacket, sizeof(strPacket), "ENAB", "F");

             //TODO server should not insist for email addr when disabling forwarding.
             TagFieldSetString(strPacket, sizeof(strPacket), "ADDR", pState->strEmail); 
     }
     _BuddyApiTransact(pState, 'EPST', strPacket);
     return(0);
}

/*F*************************************************************************************************/
/*!
    \Function HLBApiGetForwarding

    \Description
        Get  email and other forwarding states, if already loaded from server. 

    \Input *pState              - ptr to API State.
    \Input *puForwardingFlags   - Returned enable flags. Ignore if return code is "pending".
    \Input *strEmail            - The email address will be copied to this string, if set.

    \Output - int32_t HLB_ERROR_NONE if ok, HLB_ERROR_PEND if not loaded yet.
*/
/*********************************************************************************************F*/
int32_t BuddyApiGetForwarding(BuddyApiRefT *pState, unsigned *puForwardingFlags, char * strEmail)
{
    if (strEmail != NULL )
        ds_strnzcpy (strEmail, pState->strEmail, sizeof (pState->strEmail) );
    if ( puForwardingFlags != NULL)
        *puForwardingFlags =  pState->uForwardFlags ;

    if ( ( pState->uForwardFlags & BUDDY_FORWARD_FLAG_UNKNOWN ) != 0 )
        return(BUDDY_ERROR_PEND); //not loaded yet
    else 
        return(BUDDY_ERROR_NONE);
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiUserFound

    \Description
        Return users found from FindUser request. Allows client to iterate list of returned users.  
        Should be called after callback from BuddyApiFindUsers has returned with a succes status.

    \Input *pState  - reference pointer
    \Input iItem  -int32_t -  Zero based item number in list.

    \Output return code -- *BuddyApiUserT  -user name and domain, null if  not responed yet, or item not there.
*/
/*************************************************************************************************F*/
BuddyApiUserT  *BuddyApiUserFound(BuddyApiRefT *pState, int32_t iItem)
{
    BuddyApiUserT *pUser = NULL;
    //if out of limit, or record is cleared, return null...
    if (    (iItem < BUDDY_MAX_USERS_FOUND )
         && (pState->foundUsers[iItem].name[0] != 0) )
         pUser = &(pState->foundUsers[iItem]);
    return(pUser);
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiPresInit

    \Description
        Prepare to send our presence information to the server

    \Input *pState - reference pointer

    \Version 03/10/2002 (GWS)
*/
/*************************************************************************************************F*/
void BuddyApiPresInit(BuddyApiRefT *pState)
{
    pState->strPresSame[0] = 0;
    pState->strPresDiff[0] = 0;
    pState->uPresExFlags =0;
}


/*F*************************************************************************************************/
/*!
    \Function BuddyApiPresSame

    \Description
        Set the presence information intended for the same product

    \Input *pState  - reference pointer
    \Input *pSame   - string indended for same product

    \Version 03/10/2002 (GWS)
*/
/*************************************************************************************************F*/
void BuddyApiPresSame(BuddyApiRefT *pState, const char *pSame)
{
    if (pState->strSame[0] != 0)
    {
        TagFieldDupl(pState->strPresSame, sizeof(pState->strPresSame), pSame);
    }
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiPresJoinable

    \Description
        Set/Reset the I am joinable bit for Xbox --so other games can join/meet me.

    \Input *pState - reference pointer
    \Input *bOn    - 1 to set it on, 0 to turn it off.
*/
/*************************************************************************************************F*/
void BuddyApiPresJoinable(BuddyApiRefT *pState, int32_t bOn)
{
    pState->bPresJoinable = bOn;
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiPresDiff

    \Description
        Setup information for different products

    \Input *pState - reference pointer
    \Input *pDiff  - string intended for different products
    \Input *pLang  - language of string

    \Version 03/10/2002 (GWS)
*/
/*************************************************************************************************F*/
void BuddyApiPresDiff(BuddyApiRefT *pState, const char *pDiff, const char *pLang)
{
    // add to list of different product strings
    if ((pLang[0] != 0) && (pLang[1] != 0) && (pLang[2] == 0) && (pDiff[0] != 0))
    {
        TagFieldSetString(pState->strPresDiff, sizeof(pState->strPresDiff), pLang, pDiff);
    }
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiPresExFlags

    \Description
        Add extra, client specific,  info to the Presence.  

    \Input *pState - reference pointer
    \Input uFlags  - Flags to set in presence. Will show up in Buddies presence.  

    \Version 11/18/2003 (TE)
*/
/*************************************************************************************************F*/
void BuddyApiPresExFlags (BuddyApiRefT *pState,  uint32_t uFlags)
{
    pState->uPresExFlags = uFlags;
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiPresExtra

    \Description
        Set addition string to users presence, will be sent to buddies when
        BuddyApiPresSend is called.

    \Input *pState - reference pointer
    \Input strPresenceExtra  - String to set in presence. Will show up in Buddies presence.  
*/
/*************************************************************************************************F*/
void BuddyApiPresExtra(BuddyApiRefT *pState, const char * strPresenceExtra)
{
    TagFieldDupl(pState->strPresExtra, sizeof(pState->strPresExtra), strPresenceExtra);
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiPresNoText

    \Description
        Set flag to set no text capability to server -if dont support text messages.
        Will be sent to server on next send (e.g. BuddyApiPresSend).

    \Input *pState - reference pointer
*/
/*************************************************************************************************F*/
void BuddyApiPresNoText(BuddyApiRefT *pState)
{
    pState->bPresNoText =1;
}

void BuddyApiSuspendXDK( BuddyApiRefT *pState)
{

}

// Force into state where it  will listen to Xbox callbacks etc. regardless of Presence state
void BuddyApiResumeXDK( BuddyApiRefT *pState)
{
 
}
/*F*************************************************************************************************/
/*!
    \Function BuddyApiPresSend

    \Description
        Prepare to send our presence information to the server

    \Input *pState - reference pointer
    \Input *pRsrc  - (optional) resource
    \Input iShow   - show state (BUDDY_STAT_CHAT ... BUDDY_STAT_PASS)
    \Input iDelay  - milliseconds to delay before sending (0=send now)
    \Input bDontAutoRefreshRoster --Stop refresh of roster, even if going CHAT from PASSive.

    \Output int32_t - error code (zero=success, negative=error)

    \Version 01/20/2002 (GWS)
*/
/*************************************************************************************************F*/
int32_t BuddyApiPresSend(BuddyApiRefT *pState, const char *pRsrc, int32_t iShow, int32_t iDelay, int32_t bDontAutoRefreshRoster)
{
    const char *pShow;
    uint32_t uTick = ProtoAriesTick(pState->pAries);

    // translate the status field
    pShow = ((iShow >= 0) && (iShow < BUDDYAPI_STATUS_SIZE)) ? _BuddyApi_Status[iShow] : _BuddyApi_Status[0];

    // save the final send info
    TagFieldClear(pState->strPresSend, sizeof(pState->strPresSend));
    TagFieldSetString(pState->strPresSend, sizeof(pState->strPresSend), "RSRC", pRsrc);
    TagFieldSetString(pState->strPresSend, sizeof(pState->strPresSend), "SHOW", pShow);

    // set the send time
    if (pState->uPresTick == 0)
    {
        pState->uPresTick = uTick+(uint32_t)iDelay;
    }

    // see if we should force immediate send (for status change)
    if (pState->iShow != iShow)
    {
        // if was passive, and came online we still need to refresh, as updates while passive lost..
        //bDontAutoRefreshRoster is 1 if client REALLY does not want to refresh, cause client doing it anyways e.g. Resume
        if ((  (pState->iShow == BUDDY_STAT_PASS) || (pState->iShow ==BUDDY_STAT_GAME) )
            && (iShow == BUDDY_STAT_CHAT )  && (bDontAutoRefreshRoster == 0) )
        { //need to refresh, leave client callbacks untouched-so set again..
            BuddyApiRoster(pState,  pState->pListProc,  pState->pListData);  
        }

        pState->iShow = iShow;
        pState->uPresTick = uTick;
    }

    // check for immediate presence send
    if (uTick >= pState->uPresTick)
    {
        _BuddyApiPresSend(pState);
    }

    return(BUDDY_ERROR_NONE);
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiSend

    \Description
        Send a message to a remote user

    \Input *pState  - reference pointer
    \Input *pMesg   - message packet
    \Input *pCallback   - callback
    \Input *pCalldata   - callback data
    \Input iTimeout     - time out

    \Output int32_t - error code (zero=success, negative=error)

    \Version 01/20/2002 (GWS)
*/
/*************************************************************************************************F*/
int32_t BuddyApiSend(BuddyApiRefT *pState, const char *pMesg, BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout)
{
    return(_BuddyApiSendMsg ( pState, 'SEND',  pMesg,  pCallback,  pCalldata,  iTimeout));
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiBroadcast
    \Description
        Send a message to a  list of users, or to all in product or even domain.

    \Input *pState      - reference pointer
    \Input *pMesg       - message packet
    \Input *pCallback   - callback 
    \Input *pCalldata   - callback data
    \Input iTimeout     - time out

    \Output int32_t - error code (zero=success, negative=error)
*/
/*************************************************************************************************F*/
int32_t BuddyApiBroadcast(BuddyApiRefT *pState, const char *pMesg, BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout)
{
    return(_BuddyApiSendMsg ( pState, 'BRDC',  pMesg,  pCallback,  pCalldata,  iTimeout));
}


/*F*************************************************************************************************/
/*!
    \Function BuddyApiRecv

    \Description
        Set the receive callback

    \Input *pState      - reference pointer
    \Input *pCallback   - receive callback
    \Input *pCalldata   - callback user data

    \Version 01/20/2002 (GWS)
*/
/*************************************************************************************************F*/
void BuddyApiRecv(BuddyApiRefT *pState, BuddyApiCallbackT *pCallback, void *pCalldata)
{
    pState->pMesgProc = pCallback;
    pState->pMesgData = pCalldata;
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiRegisterBuddyChangeCallback

    \Description
        Register a callback when buddy changes state -- this 
         include presence change or game notifications.

    \Input *pState      - reference pointer
    \Input *pCallback   - callback function for when change occur.
    \Input *pCalldata   - callback user data
*/
/*************************************************************************************************F*/
void BuddyApiRegisterBuddyChangeCallback(BuddyApiRefT *pState, BuddyApiBuddyChangeCallbackT *pCallback, void *pCalldata)
{
    pState->pBuddyChangeProc = pCallback;
    pState->pBuddyChangeData = pCalldata;
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiRegisterBuddyChangeCallback

    \Description
        Register optional callback for client to free user ref ptr, when buddy is being deleted 
        from mem. BuddyApi will call this, just prior to freeing 
        the buddy memory.

    \Input *pState      - reference pointer
    \Input *pDelCallback   - callback function for when change occur.
    \Input *pCalldata   - callback user data
*/
/*************************************************************************************************F*/
void BuddyApiRegisterBuddyDelCallback(BuddyApiRefT *pState, BuddyApiDelCallbackT *pDelCallback, void *pCalldata)
{
    pState->pBuddyDelProc = pDelCallback;
    pState->pBuddyDelData = pCalldata;
}

/*F*************************************************************************************************/
/*!
    \Function BuddyApiUpdate

    \Description
        Handle periodic update, and return the aries game tick.

    \Input *pState      - reference pointer

    \Output
        unsigned        - aries game tick.

    \Version 01/20/2002 (GWS)
*/
/*************************************************************************************************F*/
unsigned BuddyApiUpdate(BuddyApiRefT *pState)
{
    int32_t iId;
    int32_t iIdx;
    char *pData;
    char strTemp[32];
    DispListT *pRec;
    BuddyApiMsgT msg;
    uint32_t uTick;
    BuddyRosterT *pBuddy;
    int32_t bOldEndRoster = pState->bEndRoster; //remember, in case it completed in this cycle

    // idle time tasks
    uTick = ProtoAriesTick(pState->pAries);

    // give update time
    ProtoAriesUpdate(pState->pAries);

    // clear the message packet
    memset(&msg, 0, sizeof(msg));

    // check for host resolve
    if ((pState->pConnProc != NULL) && (pState->pHost != NULL) && ((iIdx = pState->pHost->Done(pState->pHost)) != 0))
    {
        if (iIdx > 0)
        {
            // start a new connection
            const char *pServer = (pState->pServer[0] == '*') 
                ? pState->pServer + 1
                : pState->pServer;
            ProtoAriesConnect(pState->pAries, pServer, pState->pHost->addr, pState->iPort);
            if (pServer != pState->pServer)
                ProtoAriesSecure(pState->pAries, PROTOARIES_SECURE_SSL);
            BuddyApiPrintf((pState, "connecting to %s:%d\n", pState->pServer, pState->iPort));
            // send to server
            _BuddyApiTransact(pState, 'AUTH', pState->strLogin);
        }
        if (iIdx < 0)
        {
            // got a name lookup error
            _BuddyApiStatus(pState, BUDDY_SERV_NAME);
            if ( pState->bReconnect ==0 )
                pState->pConnProc = NULL; //--otherwise, needed for re-connect..
        }

        // release the dns record
        pState->pHost->Free(pState->pHost);
        pState->pHost = NULL;
    }

    // check for timeout
    if ((pState->uTimeout != 0) && (uTick > pState->uTimeout))
    {
        // got a timeout
        if ( pState->iStat == BUDDY_SERV_RECN )
        { //was  scheduled for reconnect; timer expired, so lets try to connect now
            BuddyApiPrintf( (pState, "Reconnect timer expired --will connect now ..\n"));
            _BuddyApiReconnect( pState, 1); //immediate=1, try  now. 
        }
        else
        {
            _BuddyApiStatus(pState, BUDDY_SERV_TIME);
            if ( pState->bReconnect ==0 )
                pState->pConnProc = NULL; //--otherwise, needed for re-connect..
        }
    }

    // see if we need to send a presence packet
    if ((pState->iStat == BUDDY_SERV_ONLN) && (pState->uPresTick != 0) && (uTick >= pState->uPresTick))
    {
        _BuddyApiPresSend(pState);
    }

    // check for transaction timeout
    if (pState->iStat == BUDDY_SERV_ONLN)
    {
        for (iIdx = 0; iIdx < CB_MAX; ++iIdx)
        {
            if ((pState->Callback[iIdx].pCallProc != NULL) && (pState->Callback[iIdx].uTimeout != 0) && (uTick > pState->Callback[iIdx].uTimeout))
            { //did not hear from server, so reset..
                pState->Callback[iIdx].uTimeout = 0; //clear in case reconnect hits it
                ProtoAriesUnconnect(pState->pAries);
                BuddyApiPrintf( (pState, "Transaction eCB: %d timed out...\n", iIdx));
                _BuddyApiStatus(pState, BUDDY_SERV_TIME);
                //_BuddyApiReconnect (pState, 0); //try reconnect later if needed..
                break;
            }
        }
    }
    

    // see if need to cancel outstanding transcations
    if ((pState->iStat == BUDDY_SERV_DISC) || (pState->iStat == BUDDY_SERV_TIME))
    {
        // handle disconnect
        for (iIdx = 0; iIdx < CB_MAX; ++iIdx)
        {
            if (pState->Callback[iIdx].pCallProc != NULL)
            {
                msg.iType = pState->Callback[iIdx].iType;
                msg.iCode = BUDDY_ERROR_OFFLINE;
                (pState->Callback[iIdx].pCallProc)(pState, &msg, pState->Callback[iIdx].pCallData);
                pState->Callback[iIdx].pCallProc = NULL;
            }
        }

        //dont reconnect if just got a disc -- could be bad password etc. However, if connect timed out, reconnect.
        if (pState->iStat == BUDDY_SERV_TIME)  
        {
            _BuddyApiReconnect ( pState, 1 ); //immediate =1; try now 
        }
    }

    // process all pending packets
    for (; ProtoAriesPeek(pState->pAries, &msg.iKind, &msg.iCode, &pData) >= 0; ProtoAriesRecv(pState->pAries, NULL, NULL, NULL, 0))
    { //pending packets
        msg.pData = pData;

        // show the packet
        if (pState->pDebugProc != NULL)
        {
            _BuddyApiDebug(pState, "recv: ", &msg);
        }

        // handle incoming chat messages
        if ((msg.iKind == 'RECV') && (pState->pMesgProc != NULL))
        {
            _BuddyApiRecv(pState, &msg);
        }

        // check for send acknowledgement
        if ((msg.iKind == 'SEND') && (pState->Callback[CB_SEND].pCallProc != NULL))
        {
            msg.iType = BUDDY_MSG_SEND;
            (pState->Callback[CB_SEND].pCallProc)(pState, &msg, pState->Callback[CB_SEND].pCallData);
            pState->Callback[CB_SEND].pCallProc = NULL;
        }

        // check for broadcast message  acknowledgement
        if ((msg.iKind == 'BRDC' ) && (pState->Callback[CB_BROADCAST].pCallProc != NULL))
        {
            msg.iType = BUDDY_MSG_BROADCAST;
            (pState->Callback[CB_BROADCAST].pCallProc)(pState, &msg, pState->Callback[CB_BROADCAST].pCallData);
            pState->Callback[CB_BROADCAST ].pCallProc = NULL;
        }

        // check for connect
        if ((msg.iKind == (signed)0xffffffff) && (msg.iCode == (signed)0xffffffff))
        {
            _BuddyApiStatus(pState, BUDDY_SERV_ONLN);
        }

        // respond to ping
        if (msg.iKind == 'PING')
        {
            ProtoAriesSend(pState->pAries, msg.iKind, msg.iCode, msg.pData, -1);
        }

        // check for disconnect
        if ((msg.iKind == (signed)0xffffffff) && (msg.iCode == (signed)0xfefefefe))
        {
            int32_t oldState = pState->iStat;
            _BuddyApiStatus(pState, BUDDY_SERV_DISC); 
            if ( oldState  == BUDDY_SERV_ONLN )
            {
                BuddyApiPrintf( ( pState, "Got Disconnect while connected, will req reconnect ...\n"));
                _BuddyApiReconnect (pState , 0);
            }
        }

        // handle authorization message
        if (msg.iKind == 'AUTH')
        {   
            pState->uTimeout = 0;
            if (msg.iCode != 0)
            { //auth error, 
                //first call callback --was this way, dont wanna disturb
                msg.iType = BUDDY_MSG_AUTHORIZE;
                (pState->pConnProc)(pState, &msg, pState->pConnData);

                // authorization error 
                // AUTH/netw-- mean IMServ is down, so do retry later if needed..
                if ((msg.iCode == 'netw' ) || (msg.iCode == 'bsod') ) 
                {
                    BuddyApiDisconnect(pState );
                    BuddyApiPrintf( (pState, "netw  or bsod error on Auth --will req Reconnect ..\n"));
                    _BuddyApiReconnect ( pState, 0 ); //immed=0, retry later..
                }
            }
            else
            {
                //server has connected,

                //for ps2, get title name from server. Xbox has to lookup from id.
                TagFieldGetString(TagFieldFind(msg.pData, "TITL"), pState->strTitleName, 
                                                       sizeof(pState->strTitleName), "");

                //call callback after title etc. is obtained.
                 msg.iType = BUDDY_MSG_AUTHORIZE;
                (pState->pConnProc)(pState, &msg, pState->pConnData);
            }
        }

        // handle incoming admin messages
        if ( msg.iKind == 'ADMN')  
        {  
            //look for special admin type=DUPL msg -- means someone else is now using my id, I am logged off.
            //Dont want to re-connect in this case, as it will simply oscillate between users, 
            //so lets change state, and when disconnect msg comes, it will be ignored.
            TagFieldGetString(TagFieldFind(msg.pData, "TYPE"), strTemp , sizeof( strTemp)-1, "");
            if (ds_stricmp (strTemp, "DUPL" ) == 0 ) 
                _BuddyApiStatus (pState, BUDDY_SERV_DISC);

            if  (pState->pMesgProc != NULL)
            { //inform client of msg
                msg.iType = BUDDY_MSG_ADMIN;
                (pState->pMesgProc)(pState, &msg, pState->pMesgData);
            }
        }

        // check for roster-get acknolwedgement
        if (msg.iKind == 'RGET')
        {
            if (TagFieldGetNumber(TagFieldFind(msg.pData, "ID"), 0) == ROSTER_IGNORE)
            {
                pState->iRostListCount = TagFieldGetNumber(TagFieldFind(msg.pData, "SIZE"), 0);
                if (pState->iRostListCount == 0)
                {
                    pState->bEndRoster = 1;
                }
            }
        }

        // handle roster record --uploads of initial roster
        if ((msg.iKind == 'ROST') && (pState->pDisp != NULL))
        {
            int32_t iId2 = _BuddyApiHandleROSTMsg ( pState, msg);
            // see if we need end-of-roster callback --count the ignore list.
            if ((iId2 == ROSTER_IGNORE) && (--(pState->iRostListCount) == 0))
            {
                pState->bEndRoster = 1;
            }
        }
        // handle new Roster  Notification  -- buddy invites, rejects etc. 
        if ((msg.iKind == 'RNOT') && (pState->pDisp != NULL))
        {
            if ( _BuddyApiHandleRNOTMsg(pState, msg) == 0)
            {
                DispListDirty(pState->pDisp, 1);
            }
        }

        // handle presence update
        if ((msg.iKind == 'PGET') && (pState->pDisp != NULL))
        {
            _BuddyApiHandlePGETMsg ( pState, msg);
        }

        // check for roster add  or roster invite result
        if (   ((msg.iKind == 'RADD') || (msg.iKind == 'RADM')) 
            && (pState->pDisp != NULL))
        {
            msg.iType = BUDDY_MSG_ADDUSER;
            iId = TagFieldGetNumber(TagFieldFind(msg.pData, "ID"), 0);
            //clear timer unconditionally -- implies only one outstanding ADD tx
            pState->Callback[CB_ADDUSER].uTimeout=0;

            // locate any pending transactions
            for (iIdx = 0; (pBuddy = DispListIndex(pState->pDisp, iIdx)) != NULL; ++iIdx)
            {
                // found a match
                if ((pBuddy->uFlags & BUDDY_FLAG_PEND) && (pBuddy->iPend == iId))
                {

                    pBuddy->uFlags &= ~BUDDY_FLAG_PEND ;
                    if (msg.iCode != 0)
                    { //failure
                        int32_t zap=1;
                        if (msg.iCode == 'blck')
                        { //iam blocked, cannot invite...
                             msg.iCode = 0;  //dont give user error -- simply delete as a "reject".
                        }
                        else if (msg.iCode == 'full' )
                        {
                             msg.iCode = BUDDY_ERROR_FULL;       
                        }
                        else
                             msg.iCode = BUDDY_ERROR_NOTUSER;
                        
                        if (msg.iKind == 'RADD')
                        { //error when  trying to block  someone..
                                 if (  ((pBuddy->uFlags & BUDDY_FLAG_BUDDY)>0)
                                     ||((pBuddy->uFlags & BUDDY_FLAG_BUD_REQ_SENT)>0)
                                      ||((pBuddy->uFlags & BUDDY_FLAG_WATCH)>0)
                                     ||((pBuddy->uFlags & BUDDY_FLAG_BUD_REQ_RECEIVED)>0))
                                 { //its a bud, or a wannabe -- so dont delete
                                     zap =0;
                                     pBuddy->uFlags &= ~BUDDY_FLAG_IGNORE;
                                 }
                        }
                        if (msg.iKind == 'RADM')
                        { //error when  trying to add bud..
                                 if (  ((pBuddy->uFlags & BUDDY_FLAG_IGNORE)>0) 
                                     ||((pBuddy->uFlags & BUDDY_FLAG_BUD_REQ_RECEIVED)>0)
                                     ||((pBuddy->uFlags & BUDDY_FLAG_WATCH)>0)  ) 
                                 { //its a blocked or temp or wanna be my bud, -- so dont delete
                                     zap =0;
                                     pBuddy->uFlags &= ~BUDDY_FLAG_BUD_REQ_SENT;
                                 }
                        }
                        // perform callback with status
                        if (pState->Callback[CB_ADDUSER].pCallProc != NULL)
                        {
                            msg.pData = pBuddy->strUser;
                            (pState->Callback[CB_ADDUSER].pCallProc)(pState, &msg, pState->Callback[CB_ADDUSER].pCallData);
                        }
                        if (zap)
                        {
                            //no such user or blocked from inviting/adding -- delete from list
                            pRec = HashStrDel(pState->pHash, pBuddy->strUser);
                            DispListDel(pState->pDisp, pRec);
                            //give caller a chance to free mem, if needed.
                            if (pState->pBuddyDelProc )
                            {
                                pState->pBuddyDelProc (pState, pBuddy, pState->pBuddyDelData);
                            }
                            pState->pBuddyFree(pBuddy, BUDDYAPI_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
                        } //zap

                         
                    } //failure
                    else
                    { //server sez, add succeeded...
                        // extract the formatted username
                        TagFieldGetString(TagFieldFind(msg.pData, "FUSR"), strTemp, sizeof(strTemp), "");
                        // remove domain and resource
                        _BuddyApiTruncUsername(strTemp);

                        // if we got one, replace the copy in the list
                        if ((strTemp[0] != 0) && (strlen(strTemp) < sizeof(pBuddy->strUser)))
                        {
                            pRec = HashStrDel(pState->pHash, pBuddy->strUser);
                            TagFieldDupl(pBuddy->strUser, sizeof(pBuddy->strUser), strTemp);
                            HashStrAdd(pState->pHash, pBuddy->strUser, pRec);
                        }

                        // perform callback with success
                        if (pState->Callback[CB_ADDUSER].pCallProc != NULL)
                        {
                            msg.iCode = BUDDY_ERROR_NONE;
                            msg.pData = pBuddy->strUser;
                            (pState->Callback[CB_ADDUSER].pCallProc)(pState, &msg, pState->Callback[CB_ADDUSER].pCallData);
                        }
                        // no longer pending -- we are real
                        pBuddy->iPend = 0;
                        pBuddy->uFlags &= ~BUDDY_FLAG_PEND ;
                         //dont clear temp flag, in case he reject me later, and I have msg from him.  
                    }
                    break;
                }
            }

            // always update after this
            DispListDirty(pState->pDisp, 1);
        }

         //check for presence addition response.
        if ((msg.iKind == 'PADD') && (pState->Callback[CB_ADDUSER].pCallProc != NULL))
        {
            //if the ID is in the msg, then will get a RADD too, so let it handle it.
            if ((msg.pData == NULL ) || (msg.pData[0] ==0 ) )
            {
                //means resp to temp add --clear timer as a RADD wont be coming in.
                  pState->Callback[CB_ADDUSER].uTimeout=0;
            }
        }

        //check for presence deletion response.
        if ((msg.iKind == 'PDEL') && (pState->Callback[CB_DELUSER].pCallProc != NULL))
        {
            //if the ID is in the msg, then will get a RADD too, so let it handle it.
            if ((msg.pData == NULL ) || (msg.pData[0] ==0 ) )
            {
                //means resp to temp delete --clear timer as a RDEL wont be coming in.
                  pState->Callback[CB_DELUSER].uTimeout=0;
            }
        }

        // check for roster delete  or roster delete mutual acknowledgement
        if ((msg.iKind == 'RDEL') || (msg.iKind == 'RDEM') ) 
        {
            msg.iType = BUDDY_MSG_DELUSER;
            _processAck (pState, msg, CB_DELUSER);
        }

        // check for roster(invite)  response  acknowledgement
        if (msg.iKind == 'RRSP')  
        {
            msg.iType = BUDDY_MSG_BUD_INVITE;
            _processAck (pState, msg, CB_BUDRESP);
        }

        // check   game invite response or revoke  acknowledgement
        if  ((msg.iKind == 'GRSP')  ||  (msg.iKind == 'GRVK') )
        {
            msg.iType = BUDDY_MSG_GAME_INVITE;
            _processAck (pState, msg, CB_GAMERESP);
        }

            // check for Game (invite)   acknowledgement
        if  (msg.iKind == 'GINV')
        {
            msg.iType = BUDDY_MSG_GAME_INVITE; //TODO change this?
            _processAck ( pState, msg, CB_GAMEINVITE);
        }

            // check for Game Invite Notification
        if  (msg.iKind == 'GNOT')
        {  
            if (( _BuddyApiHandleGNOTMsg(pState, msg) == 0) && (pState->pDisp != NULL))
                DispListDirty(pState->pDisp, 1);
        }

        // user forwarding settings ack..
        if (msg.iKind == 'EPST') 
        { 
            msg.iType = BUDDY_MSG_SET_FORWARDING;
            _processAck (pState, msg, CB_SETFWD);
        }  

        if (msg.iKind == 'EPGT')
        {  //get user settings/forwarding
            char strTemp2[100];
            if (TagFieldGetNumber(TagFieldFind(msg.pData, "ID"), 0) == USERFWD_FIXED_ID)
            {
                if ( msg.iCode == 0)
                {
                    // no longer unknown, so clear the bit..
                    pState->uForwardFlags &= ~BUDDY_FORWARD_FLAG_UNKNOWN; 
                    TagFieldGetString(TagFieldFind(msg.pData, "ADDR"), pState->strEmail, 
                                                                sizeof(pState->strEmail), "");
                    TagFieldGetString(TagFieldFind(msg.pData, "ENAB"), strTemp2, sizeof(strTemp2),"" ); 
                    if (ds_stricmp (strTemp2,"T") == 0 )
                            pState->uForwardFlags |= BUDDY_FORWARD_FLAG_EMAIL; //set email flag
                }      
            }
        } //get user settings/forwarding 

        // check for user search - acknolwedgement /summary.
        if (msg.iKind == 'USCH')
        { //user settings/forwarding
            if (TagFieldGetNumber(TagFieldFind(msg.pData, "ID"), 0) == USCH_FIXED_ID)
            {
                if ( msg.iCode != 0) 
                {
                    if (msg.iCode == 'susr')
                    { //bad search string -- treat it like  nothing returned.
                        pState->iFoundUsersCount =0; 
                    }
                    else
                    {
                        msg.iType = BUDDY_MSG_USERS;
                        if (pState->Callback[CB_FINDUSER].pCallProc) 
                            (pState->Callback[CB_FINDUSER].pCallProc)(pState, &msg, pState->Callback[CB_FINDUSER].pCallData);
                        pState->Callback[CB_FINDUSER].pCallProc = NULL;
                    }
                }
                else
                {
                    pState->iFoundUsersCount = TagFieldGetNumber(TagFieldFind(msg.pData, "SIZE"), 0);
                }
            }
        }

        if ((msg.iKind == 'USER')  &&  (_BuddyApiHandleSearchedUserMsg ( pState, msg)  ==0 ))
        { //if found a user, decrement count..
            pState->iFoundUsersCount--;
        }

        //if counted down to 0 and have c/b, then found all users; call c/b.
        if (   ( pState->iFoundUsersCount ==0 ) 
             &&( pState->Callback[CB_FINDUSER].pCallProc != NULL     )   )
        {
            msg.iType = BUDDY_MSG_USERS;
            msg.iKind = 0;  
            msg.iCode = 0;
            msg.pData = NULL; //todo return count  as tag?
           (pState->Callback[CB_FINDUSER].pCallProc)(pState, &msg, pState->Callback[CB_FINDUSER].pCallData);
            pState->Callback[CB_FINDUSER].pCallProc = NULL;
        }

        // ignore other message types
    } //for ...pending packets

    if ((pState->bEndRoster) &&  (bOldEndRoster==0 ) &&(pState->pDisp != NULL))
    {
        // also do list callback with special msg hlbudapi knows that Roster was loaded..
        if (pState->pListProc != NULL)
        {
            // setup the message
            memset(&msg,0,sizeof(msg));
            msg.iType = BUDDY_MSG_ENDROSTER;
            // perform the callback
            (pState->pListProc)(pState, &msg, pState->pListData);
        }

        DispListDirty(pState->pDisp, 1); //roster loading done, mark as dirty.
    }

    // see if displist callback needed
    if ((pState->pListProc != NULL) && (DispListOrder(pState->pDisp) > 0))
    {
        // setup the message
        msg.iType = BUDDY_MSG_ROSTER;
        msg.iKind = 0;
        msg.iCode = 0;
        msg.pData = (char *)pState->pDisp;
        // perform the callback
        (pState->pListProc)(pState, &msg, pState->pListData);
    }

    return(uTick);
}


// Respond to user request to be my buddy. Just send the msg and change state locally.
// Note: Need to show action immediately, instead of waiting for server-RNOT msg, as it was too slow
// Hence, this will also set/clear needed flags, just as RNOT does.
int32_t BuddyApiRespondBuddy(BuddyApiRefT *pState, BuddyRosterT *pBuddy, BuddyInviteResponseTypeE eResponse,
                         BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout)
{
    const char * strAnswer;
    int32_t iKind ='RRSP';  //roster response
    int32_t bPres = 0; //by default, dont subscribe to presence.
    int32_t iSeq,rc;

    strAnswer = _MapResponseToAnswer [eResponse];
    // save the callback info and increment Id sequence number..
    if (eResponse == BUDDY_INVITE_RESPONSE_REVOKE )
    { // like  a delete..
        iSeq = _BuddyApiSetCallback(pState, CB_DELUSER, pCallback, pCalldata, iTimeout);
    }
    else
        iSeq = _BuddyApiSetCallback(pState, CB_BUDRESP, pCallback, pCalldata, iTimeout);

    switch (eResponse)
    {   
    case BUDDY_INVITE_RESPONSE_REJECT:
        break;
    case BUDDY_INVITE_RESPONSE_ACCEPT:
        bPres=1;  //on accept buddy, I subscribe to his presence too.
        break;
    case BUDDY_INVITE_RESPONSE_BLOCK:
        break;
    case BUDDY_INVITE_RESPONSE_REVOKE:
        iKind ='RDEM';  //Roster DElete Mutual..
        break;
    default:
        return(BUDDY_ERROR_BADPARM);
    }

    //! send the  Invite Response.
    rc =   _BuddyApiSendResponse (  pState ,  pBuddy,
        iKind,  strAnswer, bPres, iSeq) ;

    //Now set/clear the flags in local copy, just like RNOT response would..
    if (rc == BUDDY_ERROR_NONE )
    {
        switch (eResponse) 
        {   
        case BUDDY_INVITE_RESPONSE_REJECT:
            _BuddyApiDeleteIfNeeded( pState, pBuddy->strUser,  BUDDY_FLAG_BUD_REQ_RECEIVED) ;
            break;
        case BUDDY_INVITE_RESPONSE_ACCEPT:
            if ( (pBuddy->uFlags & BUDDY_FLAG_BUD_REQ_RECEIVED) >0 )
            { //is true bud now..
                BuddyApiPrintf((pState, "BUDDY ACCEPT:  flag:%x MAKING:%s  Buddy", pBuddy->uFlags,  pBuddy->strUser));
                pBuddy->uFlags &= ~BUDDY_FLAG_BUD_REQ_RECEIVED ;
                pBuddy->uFlags &= ~BUDDY_FLAG_BUD_REQ_SENT; //in case of a race cond.
                pBuddy->uFlags |= BUDDY_FLAG_BUDDY ;
            }
            break;
        case BUDDY_INVITE_RESPONSE_BLOCK:
            if ( (pBuddy->uFlags & BUDDY_FLAG_BUD_REQ_RECEIVED) >0 )
            {
                pBuddy->uFlags &= ~BUDDY_FLAG_BUD_REQ_RECEIVED ;
                pBuddy->uFlags |= BUDDY_FLAG_IGNORE ;
            }
            break;
        case BUDDY_INVITE_RESPONSE_REVOKE:
            _BuddyApiDeleteIfNeeded( pState, pBuddy->strUser, BUDDY_FLAG_BUD_REQ_SENT);
            break;
        default:
            break;
        }
    }
    return(rc);
}

//!Respond to  game invite requests. pBuddy is null for a cancel of ALL invites sent.
int32_t BuddyApiRespondGame(BuddyApiRefT *pState, BuddyRosterT *pBuddy, BuddyInviteResponseTypeE eResponse,
                    BuddyApiCallbackT *pCallback, void *pCalldata, int32_t iTimeout, int32_t iSessionId)
{
    const char * strAnswer;
    int32_t iKind =  'GRSP';
    int32_t iSeq =0;
    int32_t rc = BUDDY_ERROR_BADPARM;
    strAnswer = _MapResponseToAnswer [ eResponse];
    
    switch (eResponse) 
    {   
    case BUDDY_INVITE_RESPONSE_REJECT:
        pBuddy->uFlags &= ~BUDDY_MASK_GAME_REJECT_ONLY;
        break;
    case BUDDY_INVITE_RESPONSE_ACCEPT:
        pBuddy->uFlags |= BUDDY_FLAG_GAME_INVITE_ACCEPTED;
        break;
    case BUDDY_INVITE_RESPONSE_REMOVE: //remove, but no block
        pBuddy->uFlags &= ~BUDDY_MASK_GAME_INVITE;
        break;
    case BUDDY_INVITE_RESPONSE_REVOKE:
        iKind ='GRVK';
        if (pBuddy)
        {
            pBuddy->uFlags &= ~BUDDY_MASK_GAME_REVOKE_ONLY;
        }
        break;
    default:
        return(BUDDY_ERROR_BADPARM);
    }

    //YAMS -- dont talk to Xbox Server for game invites .
    // send the  Game Invite Response -- PS2 goes thru our server 

    // save the callback info and increment Id sequence number..
    iSeq = _BuddyApiSetCallback(pState, CB_GAMERESP, pCallback, pCalldata, iTimeout);
    if ( iKind !='GRVK' )
        rc =  _BuddyApiSendResponse (  pState ,  pBuddy, iKind,  strAnswer,0, iSeq) ;
    else
    {
        char strPacket[250];
        TagFieldClear(strPacket, sizeof(strPacket));
         //if no buddy, cancel by revoking all -- dont set user..
        if (pBuddy)
        {
            TagFieldSetString(strPacket, sizeof(strPacket), "USER", pBuddy->strUser);
        }

        TagFieldSetString(strPacket, sizeof(strPacket), "RSRC", pState->pRsrc);  
        TagFieldSetNumber(strPacket, sizeof(strPacket), "ID", iSeq);

        TagFieldSetString(strPacket, sizeof(strPacket), "TITL", pState->strTitleName);
        TagFieldSetNumber(strPacket, sizeof(strPacket), "SESS",  iSessionId); 
        _BuddyApiTransact ( pState, 'GRVK', strPacket);

        rc=0;
    }

    return(rc);
}

//!temp test method prior to Server side Xbox integration.. 
void _BuddyApiSetTalkToXbox (BuddyApiRefT *pState, int32_t bOn)
{

}

//!Get the " Official" Xbox title name for a given title, based on my Locale-language
int32_t BuddyApiGetTitleName ( BuddyApiRefT  * pState, const uint32_t uTitleId, const int32_t iBufSize, char * pTitleName)
{
    return(BUDDY_ERROR_MISSING);
}

//!Get the " Official" Xbox title name for a given title. pLang ignored for now -- defaults to English.
int32_t BuddyApiGetMyTitleName ( BuddyApiRefT  * pState,   const char * pLang, const int32_t iBufSize, char * pTitleName)
{
    int32_t rc=0; 

    ds_strnzcpy ( pTitleName, pState->strTitleName, iBufSize);
    if (strlen (pTitleName ) == 0)
        rc = BUDDY_ERROR_MISSING;
    return(rc);
}


// Set the SessionID that is passed to the game invitation
int32_t BuddyApiSetGameInviteSessionID(BuddyApiRefT *pState, const char *sSessionID)
{
    //PS2 just save it for when send the invite etc.
    ds_strnzcpy(pState->strSEID, sSessionID, sizeof(pState->strSEID));

     return(BUDDY_ERROR_NONE);
}


// Get the SessionID for a particular user
int32_t BuddyApiGetUserSessionID(BuddyApiRefT *pState, const char * strGamertag, char *sSessionID, const int32_t iBufSize)
{
    // PS2- get it from the user -- buddy rec..
    // Possible mem optimization -- keep this in list of received game invites ONLY.
    BuddyRosterT *pBuddy = BuddyApiFind(pState, strGamertag);  
    if ( pBuddy == NULL)
    {
        return(BUDDY_ERROR_MISSING);
    }
    else
    {
        ds_strnzcpy(sSessionID, pBuddy->strSEID, iBufSize); 
        return(BUDDY_ERROR_NONE);
    }
}


// If xbox user goes "appearOffline" and then sends a game invite, sever does not know title
// but, we can get it from the XDK. Client need title, if not they say "invited to play: <blank>"
void BuddyApiRefreshTitle ( BuddyApiRefT * pBudRef,  BuddyRosterT *pBuddy)
{

}

