/*H********************************************************************************/
/*!
    \File voipconcierge.c

    \Description
        This contains the implementation for the Connection
        Concierge mode for the voipserver

    \Copyright
        Copyright (c) 2015 Electronic Arts Inc.

    \Version 09/24/2015 (eesponda)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/prototunnel.h"
#include "DirtySDK/voip/voiptunnel.h"
#include "DirtySDK/util/jsonformat.h"
#include "DirtySDK/util/jsonparse.h"
#include "DirtySDK/dirtyvers.h"

#include "dirtycast.h"
#include "hasher.h"
#include "restapi.h"
#include "conciergeservice.h"
#include "tokenapi.h"
#include "voipserver.h"
#include "voipservercomm.h"
#include "voipserverconfig.h"
#include "udpmetrics.h"
#include "voipconcierge.h"

/*** Defines **********************************************************************/

#define VOIPCONCIERGE_MEMID              ('vpcc')
#define VOIPCONCIERGE_MAXLEN             (128)
#define VOIPCONCIERGE_BODY_SIZE          (1024*256)
#define VOIPCONCIERGE_RESPONSE_BODY_SIZE (1024*32)
#define VOIPCONCIERGE_NAME               ("Connection Concierge")
#define VOIPCONCIERGE_CONTENTTYPE        ("application/json")
#define VOIPCONCIERGE_SCOPE              ("gs_dc_ccs")
#define VOIPCONCIERGE_AUTH_SCHEME        ("NEXUS_S2S")
#define VOIPCONCIERGE_CONSOLE_TAG_SIZE   (64)
#define VOIPCONCIERGE_CCS_CLIENTID_SIZE  (64)

//! precalculation of the max connections for our max client in a game (32)
#define VOIPCONCIERGE_MAXCONNECTIONS     (496)

//! grace period we allow extra past the expiry time for connection set
#define VOIPCONCIERGE_EXPIRY_GRACE      (30*1000)

//! base url for our requests
#define VOIPCONCIERGE_BASE_HOSTEDCONN   ("/v1/hosted_connections")

//! initial size of product map
#define VOIPCONCIERGE_PRODUCTMAP_SIZE   (8)

//! error codes used when handling requests
#define VOIPCONCIERGE_ERR_CANNOT_ADD_CONNECTION     (-1)
#define VOIPCONCIERGE_ERR_CANNOT_ADD_CONNSET        (-2)
#define VOIPCONCIERGE_ERR_INVALID_CONSOLE_INFO      (-3)
#define VOIPCONCIERGE_ERR_CONNSET_INACTIVE          (-4)
#define VOIPCONCIERGE_ERR_FULL                      (-5)
#define VOIPCONCIERGE_ERR_INVALIDPARAM              (-6)
#define VOIPCONCIERGE_ERR_FORBIDDEN                 (-7)
#define VOIPCONCIERGE_ERR_SYSTEM                    (-8)

/*** Type Definitions *************************************************************/

typedef struct ConciergeClientMapEntryT
{
    uint32_t uClientId;
    char     strConsoleTag[VOIPCONCIERGE_CONSOLE_TAG_SIZE];
    char     strClientId[VOIPCONCIERGE_CCS_CLIENTID_SIZE];
    char     strProductId[VOIPCONCIERGE_CCS_PRODUCTID_SIZE];
} ConciergeClientMapEntryT;

//! information about connection set we need to track
typedef struct ConnectionSetT
{
    uint32_t uExpireTime; //!< tick when the connection set expires
} ConnectionSetT;

//! state for the connection concierge mode
typedef struct VoipConciergeRefT
{
    VoipServerRefT *pBase;                      //!< Used to access shared data
    RestApiRefT *pApi;                          //!< used to handle RESTful API requests
    ConciergeServiceRefT *pConciergeService;    //!< used to communicate with the CCS
    uint32_t uHostingServerId;                  //!< ID of the hosting DirtyCast server obtained from CCS in the initial heartbeat
    uint32_t uLastHostNameQueryAttempt;         //!< last time we attempted a hostname query
    HasherRef *pValidatingRequests;             //!< lookup table of requests that are currently validating

    //! allocation configuration
    int32_t iMemGroup;
    void *pMemGroupUserdata;

    //! client id map
    ConciergeClientMapEntryT *pClientMap;
    int32_t iClientMapSize;

    //! product id map
    ConciergeProductMapEntryT *pProductMap;
    int32_t iProductMapSize;

    //! connection set information local to the concierge
    ConnectionSetT *pConnectionSets;
    int32_t iNumConnSets;

    //! time when cpu load under/over threshold detection window was started
    uint32_t uMaxLoadThresholdDetectionStart; 

#if DIRTYCODE_DEBUG
    /* For testing only.
        0      -->  test override disabled
        'norm' -->  Normal.  PCTSERVERLOAD under warn threshold -- client count > 0 (drain needed before exit)
        'warn' -->  Warning. PCTSERVERLOAD over warn threshold  -- client count > 0 (drain needed before exit)
        'erro' -->  Error.   PCTSERVERLOAD over error threshold -- client count > 0 (drain needed before exit)
    */
    uint32_t uFakePctServerLoad;
#endif

    //! various states
    uint8_t bMaxLoad;                           //!< is the server operating at max load (beyond max load it should only be receiving "subsequent" requests from the CCS)?
    uint8_t bMaxLoadDetectionLogged;            //!< used with both detection of 'over max load' and detection 'under max load'
    uint8_t _pad[2];
} VoipConciergeRefT;

//! console information that gets sent for each client
typedef struct GameConsoleT
{
    char strTunnelKey[PROTOTUNNEL_MAXKEYLEN];             //!< ProtoTunnel key information
    char strConsoleTag[VOIPCONCIERGE_CONSOLE_TAG_SIZE];   //!< Game Console Unique Tag
    int32_t iClientIdx;                                   //!< Location in the client id list
} GameConsoleT;

//! indices of the two clients that form a connection
typedef struct HostedConnectionT
{
    int32_t iGameConsoleIndex1;
    int32_t iGameConsoleIndex2;
} HostedConnectionT;

//! status of how far we are in validating a request
typedef enum RequestValidationStateE
{
    REQUEST_STATE_VALIDATING,
    REQUEST_STATE_WAITING,
    REQUEST_STATE_AUTHORIZED,
    REQUEST_STATE_UNAUTHORIZED,
    REQUEST_STATE_FAILURE
} RequestValidationStateE;

//! identifiers of the two clients that form a connection
typedef struct AllocatedResourceT
{
    uint32_t uClientId1;
    uint32_t uClientId2;
} AllocatedResourceT;

//! request data specific POST requests
typedef struct PostDataT
{
    GameConsoleT aConsoles[VOIPTUNNEL_MAXGROUPSIZE];            //!< array of consoles
    int32_t iNumConsoles;                                       //!< number of consoles
    HostedConnectionT aConnections[VOIPCONCIERGE_MAXCONNECTIONS]; //!< List of connections
    int32_t iNumConnections;                                    //!< How many connections?
    uint32_t uLeaseTime;                                        //!< leasetime of the connection set (in milliseconds)
    char strProduct[VOIPCONCIERGE_MAXLEN];                      //!< The identifier that the hosted connection belongs
} PostDataT;

//! request data specific DELETE requests
typedef AllocatedResourceT DeleteDataT;

//! request data specific to the type of request
typedef union RequestDataT
{
    PostDataT   PostData;
    DeleteDataT DeleteData;
} RequestDataT;

//! distinguishes between the type of hosted connection requests
typedef enum RequestTypeE
{
    REQUEST_TYPE_POST = 0,
    REQUEST_TYPE_DELETE
} RequestTypeE;

//! request that we deserialize from json
typedef struct HostedConnectionRequestT
{
    int32_t iRequestId;                                          //!< Unique identifier for debugging
    int32_t iConnSet;                                            //!< (optional) what connection set we are updating
    RequestValidationStateE eState;                              //!< current state of the request validation
    char strClientId[VOIPCONCIERGE_CCS_CLIENTID_SIZE];           //!< ccs client id
    RequestTypeE eRequestType;                                   //!< request type (POST, DELETE, etc)
    RequestDataT RequestData;                                    //!< data specific to the request type
} HostedConnectionRequestT;

//! response information that gets serialized into json
typedef struct HostedConnectionResponseT
{
    int32_t iRequestId;                                     //!< Unique identifier for debugging
    uint32_t uAddress;                                      //!< Front-end Address
    uint16_t uPort;                                         //!< Front-end port
    uint16_t uLeaseTime;                                    //!< leasetime of the connection set (in seconds)
    uint32_t uHostingServerId;                              //!< ID of the hosting DirtyCast server
    int32_t iConnSet;                                       //!< which connection set we allocated to
    uint32_t uCapacity;                                     //!< how much capcity we have left
    int32_t iNumResources;                                  //!< number of allocated connections for post and number of deleted for delete
    AllocatedResourceT aResources[VOIPCONCIERGE_MAXCONNECTIONS]; //!< List of allocated connections
} HostedConnectionResponseT;

//! information that contains the response code and error text
typedef struct GenericErrorResponseT
{
    ProtoHttpResponseE eResponseCode;
    const char *pText;
} GenericErrorResponseT;

//! maps from our concierge error type to the httperror/message we respond with
typedef struct VoipConciergeErrorMapT
{
    int32_t iConciergeError;
    GenericErrorResponseT Error;
} VoipConciergeErrorMapT;

//! function prototypes for handlers for _VoipConcierge_aResources request processes
typedef uint8_t(RequestParamHandlerCbT) (const ProtoHttpServRequestT *pRequest, HostedConnectionRequestT *pHostedRequest);
typedef uint8_t(RequestQueryHandlerCbT) (const ProtoHttpServRequestT *pRequest, HostedConnectionRequestT *pHostedRequest);
typedef int32_t(RequestBodyHandlerCbT)  (VoipConciergeRefT *pVoipConcierge, const ProtoHttpServRequestT *pRequest, HostedConnectionRequestT *pHostedRequest);
typedef int32_t(RequestActionHandlerCbT)(VoipConciergeRefT *pVoipConcierge, const HostedConnectionRequestT *pRequest, HostedConnectionResponseT *pResponse);
typedef void   (PostEncoderCbT)         (const HostedConnectionResponseT *pHostedResponse, ProtoHttpServResponseT *pResponse);

/*** Function Prototypes **********************************************************/

// generic handler for "/v1/hosted_connections" endpoint
static int32_t _VoipConciergeApiConnectionCb(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData, 
                                             RequestTypeE             eRequestType, 
                                             RequestParamHandlerCbT  *pRequestParamHandler, 
                                             RequestQueryHandlerCbT  *pRequestQueryHandler, 
                                             RequestBodyHandlerCbT   *pRequestBodyHandler,
                                             RequestActionHandlerCbT *pRequestActionHandler,
                                             PostEncoderCbT          *pPostEncoderHandler);

// specific handlers for "/v1/hosted_connections" endpoint
static int32_t _VoipConciergeApiCreateConnCb(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData);
static int32_t _VoipConciergeApiUpdateConnCb(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData);
static int32_t _VoipConciergeApiDeleteConnCb(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData);
static int32_t _VoipConciergeApiDeleteAllConnsCb(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData);

/*** Variables ********************************************************************/

//! used to register resources with the RestApi
static const RestApiResourceDataT _VoipConcierge_aResources[] =
{
    // create new connection set or delete based on ccs_client_id
    { "(/v1/hosted_connections(/?)$)",        { NULL, NULL, _VoipConciergeApiCreateConnCb, NULL, _VoipConciergeApiDeleteAllConnsCb, NULL } },
    // update connection set or delete connection set 
    { "(/v1/hosted_connections/[0-9]+(/?)$)", { NULL, NULL, _VoipConciergeApiUpdateConnCb, NULL, _VoipConciergeApiDeleteConnCb,     NULL } }
};

//! used in the handling functions to map our internal errors to the correct http errors
static const VoipConciergeErrorMapT _VoipConcierge_aErrorResponses[] =
{
    { VOIPCONCIERGE_ERR_CANNOT_ADD_CONNECTION,      { PROTOHTTP_RESPONSE_INTERNALSERVERERROR, "cannot create connection" } },
    { VOIPCONCIERGE_ERR_CANNOT_ADD_CONNSET,         { PROTOHTTP_RESPONSE_INTERNALSERVERERROR, "cannot create connection set" } },
    { VOIPCONCIERGE_ERR_INVALID_CONSOLE_INFO,       { PROTOHTTP_RESPONSE_CLIENTERROR, "invalid game console info in request" } },
    { VOIPCONCIERGE_ERR_CONNSET_INACTIVE,           { PROTOHTTP_RESPONSE_NOTFOUND, "connection set does not exist" } },
    { VOIPCONCIERGE_ERR_FULL,                       { PROTOHTTP_RESPONSE_CLIENTERROR, "cannot add anymore connection sets, we are full" } },
    { VOIPCONCIERGE_ERR_INVALIDPARAM,               { PROTOHTTP_RESPONSE_CLIENTERROR, "invalid request parameters" } },
    { VOIPCONCIERGE_ERR_FORBIDDEN,                  { PROTOHTTP_RESPONSE_FORBIDDEN, "forbidden from accessing this resource" } },
    { VOIPCONCIERGE_ERR_SYSTEM,                     { PROTOHTTP_RESPONSE_INTERNALSERVERERROR, "system could not process request" } }
};

/*** Private functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _VoipConciergeGetErrorResponse

    \Description
        Based on the result, find the HTTP Response Code and message

    \Input iResult  - the result we are looking for in our array

    \Output
        const GenericErrorResponseT *   - pointer to the response we will use

    \Version 10/02/2015 (eesponda)
*/
/********************************************************************************F*/
static const GenericErrorResponseT *_VoipConciergeGetErrorResponse(int32_t iResult)
{
    uint32_t uResponse;
    for (uResponse = 0; uResponse < (sizeof(_VoipConcierge_aErrorResponses) / sizeof(VoipConciergeErrorMapT)); uResponse += 1)
    {
        const VoipConciergeErrorMapT *pData = &_VoipConcierge_aErrorResponses[uResponse];
        if (pData->iConciergeError == iResult)
        {
            break;
        }
    }
    return(&_VoipConcierge_aErrorResponses[uResponse].Error);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeVoipTunnelEventCallback

    \Description
        VoipTunnel event callback handler.

    \Input *pVoipTunnel     - voiptunnel module state
    \Input *pEventData      - event data
    \Input *pUserData       - user callback ref (voipserver module state)

    \Version 09/25/2015 (eesponda)
*/
/********************************************************************************F*/
static void _VoipConciergeVoipTunnelEventCallback(VoipTunnelRefT *pVoipTunnel, const VoipTunnelEventDataT *pEventData, void *pUserData)
{
    VoipConciergeRefT *pVoipConcierge = (VoipConciergeRefT *)pUserData;
    ProtoTunnelRefT *pProtoTunnel = VoipServerGetProtoTunnel(pVoipConcierge->pBase);
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipConcierge->pBase);
    VoipTunnelClientT *pClient = pEventData->pClient;
    #if DIRTYVERS > DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
    VoipTunnelGameMappingT* pGameMapping;
    #endif

    if (pClient == NULL)
    {
        // client is unavailable, can this happen?
        return;
    }
    switch (pEventData->eEvent)
    {
        // add client event
    case VOIPTUNNEL_EVENT_ADDCLIENT:
    {
        ProtoTunnelInfoT TunnelInfo;

        ds_memcpy(&TunnelInfo, &pConfig->TunnelInfo, sizeof(TunnelInfo));
        TunnelInfo.uRemoteClientId = pClient->uClientId;

        #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
        /*  if the tunnel is already allocated make sure we are not using a key that is suspended

            prototunnel refcounts the tunnel by checking if the uRemoteClientId exists in its
            list of tunnels. it expects you to use a different key when allocating each
            time, if you do not use a different key it will print out a warning but continue
            on. this additional refcounted tunnel can cause some issues with connections
            between client and server.

            for these reasons we want to check if this same user is using the same key and
            block its allocation.

            when the tunnel gets refcounted down we will just completely destroy the tunnel
            and new requests will just allocate it as a new tunnel
        */
        if (pClient->uTunnelId != 0)
        {
            int32_t iGame;
            for (iGame = 0; iGame < pClient->iNumSuspended; iGame += 1)
            {
                if (strcmp(pClient->aSuspendedData[iGame].strTunnelKey, pClient->strTunnelKey) == 0)
                {
                    break;
                }
            }
            if (iGame != pClient->iNumSuspended)
            {
                DirtyCastLogPrintf("voipconcierge: skipping tunnel alloc for clientId=0x%08x as it already has tunnel=0x%08x with key=%s\n",
                    pClient->uClientId, pClient->uTunnelId, pClient->strTunnelKey);
                break;
            }
        }

        // allocate the tunnel
        if ((pClient->uTunnelId = ProtoTunnelAlloc(pProtoTunnel, &TunnelInfo, pClient->strTunnelKey)) == (unsigned)-1)
        {
            DirtyCastLogPrintf("voipconcierge: tunnel alloc for clientId=0x%08x failed\n", pClient->uClientId);
        }
        #else
        if ((pGameMapping = VoipTunnelClientMatchGameIdx(pClient, pEventData->iGameIdx)) == NULL)
        {
            DirtyCastLogPrintf("voipconcierge: tunnel event error for clientId=0x%08x - doest not belong to game idx %d\n", pClient->uClientId, pEventData->iGameIdx);
            break;
        }

        /*  if the tunnel is already allocated make sure we are not using a key that is already
            associated with another game

            prototunnel refcounts the tunnel by checking if the uRemoteClientId exists in its
            list of tunnels. it expects you to use a different key when allocating each
            time, if you do not use a different key it will print out a warning but continue
            on. this additional refcounted tunnel can cause some issues with connections
            between client and server.

            for these reasons we want to check if this same user is using the same key and
            block its allocation.

            when the tunnel gets refcounted down we will just completely destroy the tunnel
            and new requests will just allocate it as a new tunnel
        */
        if (pClient->uTunnelId != 0)
        {
            int32_t iGameMappingIndex;
            for (iGameMappingIndex = 0; iGameMappingIndex < VOIPTUNNEL_MAX_GAMES_PER_CLIENT; iGameMappingIndex += 1)
            {
                if (pGameMapping != &pClient->aGameMappings[iGameMappingIndex])
                {
                    if (strcmp(pClient->aGameMappings[iGameMappingIndex].strTunnelKey, pGameMapping->strTunnelKey) == 0)
                    {

                        break;
                    }
                }
            }
            if (iGameMappingIndex != VOIPTUNNEL_MAX_GAMES_PER_CLIENT)
            {
                DirtyCastLogPrintf("voipconcierge: skipping tunnel alloc for clientId=0x%08x as they already have tunnel=0x%08x with key=%s\n",
                    pClient->uClientId, pClient->uTunnelId, pGameMapping->strTunnelKey);
                break;
            }
        }

        // allocate the tunnel (if tunnel already exist, refcounting will be taken care of internally)
        if ((pClient->uTunnelId = ProtoTunnelAlloc(pProtoTunnel, &TunnelInfo, pGameMapping->strTunnelKey)) == (unsigned)-1)
        {
            DirtyCastLogPrintf("voipconcierge: tunnel alloc for clientId=0x%08x failed\n", pClient->uClientId);
        }
        #endif

        // at least survive for the configured timeout value
        pClient->uLastUpdate = NetTick();

        break;
    }

    // del client event
    case VOIPTUNNEL_EVENT_DELCLIENT:
    {
        #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
        const char *pTunnelKey;
        int32_t iGameSlot;

        if (pClient->uTunnelId == 0)
        {
            DirtyCastLogPrintf("voipconcierge: voiptunnel delclient event for clientId=0x%08x with no prototunnel tunnel allocated\n", pClient->uClientId);
            break;
        }

        // reference the tunnel key for the connection we are removing
        if ((iGameSlot = pEventData->iDataSize) == 0)
        {
            // this is the case we are removing for the active game
            pTunnelKey = pClient->strTunnelKey;
        }
        else
        {
            // this is the case where we are removing a suspended game
            pTunnelKey = pClient->aSuspendedData[iGameSlot - 1].strTunnelKey;
        }

        if (ProtoTunnelFree(pProtoTunnel, pClient->uTunnelId, pTunnelKey) == 0)
        {
            // refcount has hit zero so we can clear the tunnel completely
            pClient->uTunnelId = 0;
        }
        #else
        if (pClient->uTunnelId == 0)
        {
            DirtyCastLogPrintf("voipconcierge: voiptunnel delclient event for clientId=0x%08x with no prototunnel tunnel allocated\n", pClient->uClientId);
            break;
        }

        if ((pGameMapping = VoipTunnelClientMatchGameIdx(pClient, pEventData->iGameIdx)) == NULL)
        {
            DirtyCastLogPrintf("voipconcierge: tunnel event error for clientId=0x%08x - doest not belong to game idx %d\n", pClient->uClientId, pEventData->iGameIdx);
            break;
        }

        if (ProtoTunnelFree(pProtoTunnel, pClient->uTunnelId, pGameMapping->strTunnelKey) == 0)
        {
            // refcount has hit zero so we can clear the tunnel completely
            pClient->uTunnelId = 0;
        }
        #endif

        // at least survive for the configured timeout value
        pClient->uLastUpdate = NetTick();

        break;
    }

    default:
    {
        break;
    }
    } // end of switch()
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeSplitUrl

    \Description
        Splits data on a delimiter

    \Input *pSource     - the string we are splitting
    \Input iSrcSize     - size of string
    \Input *pDest       - where we are writing the data split
    \Input iDstSize     - size of dest string
    \Input **pNewSource - used to save position of iteration

    \Note
        This can be used to split any string but normally used for url in the
        context of this module. Assumes that when parsing '/' the leading slash
        is parsed out before calling this function

    \Output
        uint8_t         - success of splitting

    \Version 10/05/2015 (eesponda)
*/
/********************************************************************************F*/
static uint8_t _VoipConciergeSplitUrl(const char *pSource, int32_t iSrcSize, char cDelimiter, char *pDest, int32_t iDstSize, const char **pNewSource)
{
    const char *pLocation;

    // if we start with delimiter skip it
    // this happens when you do multiple parses
    if (pSource[0] == cDelimiter)
    {
        pSource += 1;
        (*pNewSource) += 1;
    }

    // terminate the destination
    if ((pDest != NULL) && (iDstSize > 0))
    {
        pDest[0] = '\0';
    }

    // check validity of inputs
    if ((pSource == NULL) || (pSource[0] == '\0') || (iSrcSize <= 0))
    {
        return(FALSE);
    }

    if ((pLocation = strchr(pSource, cDelimiter)) != NULL)
    {
        const int32_t iCount = pLocation - pSource;
        (*pNewSource) += iCount;
        if (pDest != NULL)
        {
            ds_strnzcpy(pDest, pSource, DS_MIN(iDstSize, iCount));
        }
    }
    else
    {
        (*pNewSource) += iSrcSize;
        if (pDest != NULL)
        {
            ds_strnzcpy(pDest, pSource, DS_MIN(iDstSize, iSrcSize));
        }
    }

    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeGenerateClientId

    \Description
        Generates a new client id

    \Output
        uint32_t - new generated client id

    \Version 09/25/2015 (eesponda)
*/
/********************************************************************************F*/
static uint32_t _VoipConciergeGenerateClientId(void)
{
    // the valid range for DirtyCast client ids is 0x80000000 - 0xBFFFFFFF
    static uint32_t uClientId = 0x80000000;
    uint32_t uNewClientId = uClientId;

    if (uClientId == 0xBFFFFFFF)
    {
        uClientId = 0x80000000;
    }
    else
    {
        uClientId += 1;
    }

    return(uNewClientId);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeGenerateGameId

    \Description
        Generates a new game id

    \Output
        uint32_t - new generated game id (0x00000001-0xffffffff)

    \Version 12/01/2015 (eesponda)
*/
/********************************************************************************F*/
static uint32_t _VoipConciergeGenerateGameId(void)
{
    // zero is an invalid game id
    uint32_t uNewGameId;
    static uint32_t uGameId = 1;

    // make sure we dont use zero
    if (uGameId == 0)
    {
        uGameId = 1;
    }
    uNewGameId = uGameId;
    uGameId += 1;

    return(uNewGameId);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeAddClientToProduct

    \Description
        Add client to the product map entry. Create the product if it does not yet exist.

    \Input *pVoipConcierge      - ref to the voipconcierge module
    \Input *pProductID          - id of the product to which the client belongs

    \Output
        int32_t                 - zero=success, negative=failure

    \Version 02/20/2017 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeAddClientToProduct(VoipConciergeRefT *pVoipConcierge, const char *pProductId)
{
    int32_t iResult = 0;            // default to "success"
    int32_t iProductIndex;
    int32_t iFreeEntryIndex = -1;   // default to "no free entry"

    // look for the product id
    for (iProductIndex = 0; iProductIndex < pVoipConcierge->iProductMapSize; iProductIndex += 1)
    {
        if (pVoipConcierge->pProductMap[iProductIndex].strProductId[0] == '\0')
        {
            if (iFreeEntryIndex == -1)
            {
                // remember where first free entry is
                iFreeEntryIndex = iProductIndex;
            }
        }
        else if (strncmp(pVoipConcierge->pProductMap[iProductIndex].strProductId, pProductId, sizeof(pVoipConcierge->pProductMap[iProductIndex].strProductId)) == 0)
        {
            pVoipConcierge->pProductMap[iProductIndex].iClientCount++;
            break;
        }
    }

    // if product does not already exist, add it to the map
    if (iProductIndex == pVoipConcierge->iProductMapSize)
    {
        // if the product map is full, create a larger one
        if (iFreeEntryIndex == -1)
        {
            int32_t iNewProductMapSize = pVoipConcierge->iProductMapSize + VOIPCONCIERGE_PRODUCTMAP_SIZE;
            ConciergeProductMapEntryT *pNewProductMap;

            // create the elements of the new product map
            if ((pNewProductMap = (ConciergeProductMapEntryT *)DirtyMemAlloc(sizeof(ConciergeProductMapEntryT) * iNewProductMapSize, VOIPCONCIERGE_MEMID, pVoipConcierge->iMemGroup, pVoipConcierge->pMemGroupUserdata)) != NULL)
            {
                DirtyCastLogPrintf("voipconcierge: expanding concierge mode module's product map from %d entries to %d entries\n", pVoipConcierge->iProductMapSize, iNewProductMapSize);

                ds_memclr(pNewProductMap, sizeof(ConciergeProductMapEntryT) * iNewProductMapSize);

                // initialize new map with content of old map
                ds_memcpy_s(pNewProductMap, sizeof(ConciergeProductMapEntryT) * iNewProductMapSize, pVoipConcierge->pProductMap, sizeof(ConciergeProductMapEntryT) * pVoipConcierge->iProductMapSize);

                // delete old map
                DirtyMemFree(pVoipConcierge->pProductMap, VOIPCONCIERGE_MEMID, pVoipConcierge->iMemGroup, pVoipConcierge->pMemGroupUserdata);

                // save location of new free entry
                iFreeEntryIndex = pVoipConcierge->iProductMapSize;

                // reset internal variables used to manage product map
                pVoipConcierge->iProductMapSize = iNewProductMapSize;
                pVoipConcierge->pProductMap = pNewProductMap;
            }
            else
            {
                DirtyCastLogPrintf("voipconcierge: unable to expand concierge mode module's product map from %d entries to %d entries\n", pVoipConcierge->iProductMapSize, iNewProductMapSize);
            }
        }

        // if a free spot was found, use it to add the new product
        if (iFreeEntryIndex != -1)
        {
            ds_strnzcpy(pVoipConcierge->pProductMap[iFreeEntryIndex].strProductId, pProductId, sizeof(pVoipConcierge->pProductMap[iFreeEntryIndex].strProductId));
            pVoipConcierge->pProductMap[iFreeEntryIndex].iClientCount++;
            DirtyCastLogPrintf("voipconcierge: new product map entry --> %s\n", pVoipConcierge->pProductMap[iFreeEntryIndex].strProductId);
        }
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeRemoveClientFromProduct

    \Description
        Remove client from the product map entry.

    \Input *pVoipConcierge      - ref to the voipconcierge module
    \Input *pProductId          - product to which the connection belongs

    \Notes
        We do not clear the product when we don't have any clients as we want to
        continue to report zero for our metrics. It doesn't make sense to remove
        the product as we will likely see it again.

    \Version 02/20/2017 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipConciergeRemoveClientFromProduct(VoipConciergeRefT *pVoipConcierge, const char *pProductId)
{
    int32_t iProductIndex;

    // look for the product id
    for (iProductIndex = 0; iProductIndex < pVoipConcierge->iProductMapSize; iProductIndex += 1)
    {
        if (strncmp(pVoipConcierge->pProductMap[iProductIndex].strProductId, pProductId, sizeof(pVoipConcierge->pProductMap[iProductIndex].strProductId)) == 0)
        {
            pVoipConcierge->pProductMap[iProductIndex].iClientCount--;

            if (pVoipConcierge->pProductMap[iProductIndex].iClientCount < 0)
            {
                DirtyCastLogPrintf("voipconcierge: critical error - product map entry %s has a negative client count\n", pVoipConcierge->pProductMap[iProductIndex].strProductId);
                pVoipConcierge->pProductMap[iProductIndex].iClientCount = 0;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeAddClient

    \Description
        Function to insert a key / value pair into the client map

    \Input *pVoipConcierge - ref to the voipconcierge module
    \Input iRequestId      - identifies the request for logging purposes
    \Input *pCcsClientId   - ccs client id
    \Input *pConsoleTag    - unique console tag
    \Input *pProductId     - id of product to which this client belongs

    \Output
        uint32_t           - the DirtyCast client id, or 0 if failed

    \Version 01/15/2016 (amakoukji)
*/
/********************************************************************************F*/
static uint32_t _VoipConciergeAddClient(VoipConciergeRefT *pVoipConcierge, int32_t iRequestId, const char *pCcsClientId, const char *pConsoleTag, const char *pProductId)
{
    int32_t iIndex = 0;

    // look for the first open slot
    for (; iIndex < pVoipConcierge->iClientMapSize; ++iIndex)
    {
        ConciergeClientMapEntryT *pEntry = &pVoipConcierge->pClientMap[iIndex];
        if (pEntry->uClientId == 0)
        {
            if (_VoipConciergeAddClientToProduct(pVoipConcierge, pProductId) < 0)
            {
                DirtyCastLogPrintf("voipconcierge: error, client map full, unable to add product reference for request with client_id = %s and tag = %s (req=%d)\n", pCcsClientId, pConsoleTag, iRequestId);
                return(0);
            }

            pEntry->uClientId = _VoipConciergeGenerateClientId();
            // add both keys to the entry
            ds_strnzcpy(pEntry->strClientId, pCcsClientId, sizeof(pEntry->strClientId));
            ds_strnzcpy(pEntry->strConsoleTag, pConsoleTag, sizeof(pEntry->strConsoleTag));
            ds_strnzcpy(pEntry->strProductId, pProductId, sizeof(pEntry->strProductId));

            DirtyCastLogPrintf("voipconcierge: added new client 0x%08x to map at index %d\n", pEntry->uClientId, iIndex);
            return(pEntry->uClientId);
        }
    }

    // failed to find an empty slot
    DirtyCastLogPrintf("voipconcierge: error, client map full, unable to insert new client from client_id = %s and tag = %s (req=%d)\n", pCcsClientId, pConsoleTag, iRequestId);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeGetClientId

    \Description
        Function that takes the string ids as keys to locate the DirtyCast client
        id or generate a new one if required

    \Input *pVoipConcierge - ref to the voipconcierge module
    \Input iRequestId      - identifies the request for logging purposes
    \Input *pCcsClientId   - ccs client id
    \Input *pConsoleTag    - unique console tag
    \Input *pProductId     - id of product to which this client belongs
    \Input bGen            - generate a new client id if not found

    \Output
        uint32_t           - the DirtyCast client id, or 0 is not found

    \Version 01/15/2016 (amakoukji)
*/
/********************************************************************************F*/
static uint32_t _VoipConciergeGetClientId(VoipConciergeRefT *pVoipConcierge, int32_t iRequestId, const char *pCcsClientId, const char *pConsoleTag, const char *pProductId, uint8_t bGen)
{
    int32_t iIndex = 0;

    for (; iIndex < pVoipConcierge->iClientMapSize; ++iIndex)
    {
        const ConciergeClientMapEntryT *pEntry = &pVoipConcierge->pClientMap[iIndex];
        if ((strncmp(pEntry->strClientId, pCcsClientId, sizeof(pEntry->strClientId)) == 0) &&
            (strncmp(pEntry->strConsoleTag, pConsoleTag, sizeof(pEntry->strConsoleTag)) == 0))
        {
            return(pVoipConcierge->pClientMap[iIndex].uClientId);
        }
    }

    return((bGen == TRUE) ? _VoipConciergeAddClient(pVoipConcierge, iRequestId, pCcsClientId, pConsoleTag, pProductId) : 0);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeRemoveClient

    \Description
        Function to insert a key / value pair into the client map

    \Input *pVoipConcierge - ref to the voipconcierge module
    \Input uClientId       - client id

    \Output
        uint32_t           - 0 if successful, or negative if failed

    \Version 01/15/2016 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeRemoveClient(VoipConciergeRefT *pVoipConcierge, uint32_t uClientId)
{
    int32_t iIndex = 0;

    // look for the client id
    for (; iIndex < pVoipConcierge->iClientMapSize; ++iIndex)
    {
        if (pVoipConcierge->pClientMap[iIndex].uClientId == uClientId)
        {
            _VoipConciergeRemoveClientFromProduct(pVoipConcierge, pVoipConcierge->pClientMap[iIndex].strProductId);

            // remove the entry
            ds_memclr(&pVoipConcierge->pClientMap[iIndex], sizeof(ConciergeClientMapEntryT));
            DirtyCastLogPrintf("voipconcierge: removed client 0x%08x from map at index %d\n", uClientId, iIndex);
            return(0);
        }
    }

    // failed to find the client id
    DirtyCastLogPrintf("voipconcierge: warning, client 0x%08x was not found while trying to delete it\n", uClientId);
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeAddVoipClient

    \Description
        Creates a new VoipTunnelClientT and adds it to the voiptunnel

    \Input *pVoipTunnel    - ref to the voiptunnel module
    \Input iGameIdx        - the voiptunnel game index to add the client to (usually game specific)
    \Input iRequestId      - identifies the request for logging purposes
    \Input uClientId       - id assigned to client we are adding
    \Input *pGameConsole   - console information used for creation (tunnel key and index)
    \Input **ppNewClient   - the new client that we are creating

    \Output
        int32_t            - the new clientid or negative

    \Version 09/25/2015 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeAddVoipClient(VoipTunnelRefT *pVoipTunnel, int32_t iGameIdx, int32_t iRequestId, uint32_t uClientId, const GameConsoleT *pGameConsole, VoipTunnelClientT **ppNewClient)
#if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
{
    int32_t iResult;
    VoipTunnelClientT Client;

    // format client
    ds_memclr(&Client, sizeof(Client));
    Client.uClientId = uClientId;
    Client.iGameIdx = iGameIdx;

    // aClientIds shall be updated later in the flow
    ds_strnzcpy(Client.strTunnelKey, pGameConsole->strTunnelKey, sizeof(Client.strTunnelKey));

    // attempt to add the client to the connection set
    if ((iResult = VoipTunnelClientListAdd2(pVoipTunnel, &Client, ppNewClient, pGameConsole->iClientIdx)) != 0)
    {
        DirtyCastLogPrintf("voipconcierge: could not add clientid 0x%08x to connection set index %04d (req=%d)\n", Client.uClientId, iGameIdx, iRequestId);
    }

    return(iResult);
}
#else
{
    int32_t iResult;

    // attempt to add the client to the connection set
    if ((iResult = VoipTunnelClientListAdd(pVoipTunnel, uClientId, iGameIdx, pGameConsole->strTunnelKey, pGameConsole->iClientIdx, ppNewClient)) != 0)
    {
        DirtyCastLogPrintf("voipconcierge: could not add clientid 0x%08x to voiptunnel game index %04d (req=%d)\n", uClientId, iGameIdx, iRequestId);
    }

    return(iResult);
}
#endif

/*F********************************************************************************/
/*!
    \Function _VoipConciergeAddConnection

    \Description
        Takes in the request of hosted connections and allocates them

    \Input *pVoipConcierge      - ref to the voipconcierge module
    \Input iGameIdx             - the voiptunnel game index matching the connection set we are checking/adding to
    \Input iRequestId           - identifies the request for logging purposes
    \Input *pClientInstanceId   - the client instance id
    \Input *pPostData           - request data decoded from json payload
    \Input *pAllocatedResource  - [out] the connections allocated (same size of pConnections)

    \Output
        int32_t                 - number of clients created or negative=failure

    \Version 09/25/2015 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeAddConnection(VoipConciergeRefT *pVoipConcierge, int32_t iGameIdx, int32_t iRequestId, const char *pClientInstanceId, const PostDataT *pPostData, AllocatedResourceT *pAllocatedResource)
{
    int32_t iConnIdx;
    int32_t iClientsCreated = 0;
    uint32_t uClient1Id, uClient2Id;
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipConcierge->pBase);
    #if DIRTYVERS > DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
    VoipTunnelGameT *pGame = VoipTunnelGameListMatchIndex(pVoipTunnel, iGameIdx);
    VoipTunnelGameMappingT *pGameMapping;
    #endif

    for (iConnIdx = 0; iConnIdx < pPostData->iNumConnections; iConnIdx += 1)
    {
        VoipTunnelClientT *pClient1, *pClient2;
        const GameConsoleT *pGameConsole1, *pGameConsole2;
        const HostedConnectionT *pCurrentIn = &pPostData->aConnections[iConnIdx];
        AllocatedResourceT *pCurrentOut = &pAllocatedResource[iConnIdx];

        // assign data based on index in connection
        pGameConsole1 = &pPostData->aConsoles[pCurrentIn->iGameConsoleIndex1];
        pGameConsole2 = &pPostData->aConsoles[pCurrentIn->iGameConsoleIndex2];

        /* lookup the clientid in our cache then use to query the voiptunnel for our client
           if the client doesn't exist or belongs to a different game then create (when the client exists it will be refcounted) */
        if ((uClient1Id = _VoipConciergeGetClientId(pVoipConcierge, iRequestId, pClientInstanceId, pGameConsole1->strConsoleTag, pPostData->strProduct, TRUE)) == 0)
        {
            return(VOIPCONCIERGE_ERR_CANNOT_ADD_CONNECTION);
        }

        #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
        if (((pClient1 = VoipTunnelClientListMatchId(pVoipTunnel, uClient1Id)) == NULL) || (pClient1->iGameIdx != iGameIdx))
        {
            if (_VoipConciergeAddVoipClient(pVoipTunnel, iGameIdx, iRequestId, uClient1Id, pGameConsole1, &pClient1) < 0)
            {
                _VoipConciergeRemoveClient(pVoipConcierge, uClient1Id);
                return(VOIPCONCIERGE_ERR_CANNOT_ADD_CONNECTION);
            }
            iClientsCreated += 1;
        }
        #else
        if (((pClient1 = VoipTunnelClientListMatchId(pVoipTunnel, uClient1Id)) == NULL) || (VoipTunnelClientMatchGameId(pVoipTunnel, pClient1, pGame->uGameId) == NULL))
        {
            if (_VoipConciergeAddVoipClient(pVoipTunnel, iGameIdx, iRequestId, uClient1Id, pGameConsole1, &pClient1) < 0)
            {
                _VoipConciergeRemoveClient(pVoipConcierge, uClient1Id);
                return(VOIPCONCIERGE_ERR_CANNOT_ADD_CONNECTION);
            }
            iClientsCreated += 1;
        }
        #endif

        if ((uClient2Id = _VoipConciergeGetClientId(pVoipConcierge, iRequestId, pClientInstanceId, pGameConsole2->strConsoleTag, pPostData->strProduct, TRUE)) == 0)
        {
            return(VOIPCONCIERGE_ERR_CANNOT_ADD_CONNECTION);
        }

        #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
        if (((pClient2 = VoipTunnelClientListMatchId(pVoipTunnel, uClient2Id)) == NULL) || (pClient2->iGameIdx != iGameIdx))
        {
            if (_VoipConciergeAddVoipClient(pVoipTunnel, iGameIdx, iRequestId, uClient2Id, pGameConsole2, &pClient2) < 0)
            {
                _VoipConciergeRemoveClient(pVoipConcierge, uClient2Id);
                return(VOIPCONCIERGE_ERR_CANNOT_ADD_CONNECTION);
            }
            iClientsCreated += 1;
        }
        #else
        if (((pClient2 = VoipTunnelClientListMatchId(pVoipTunnel, uClient2Id)) == NULL) || (VoipTunnelClientMatchGameId(pVoipTunnel, pClient2, pGame->uGameId) == NULL))
        {
            if (_VoipConciergeAddVoipClient(pVoipTunnel, iGameIdx, iRequestId, uClient2Id, pGameConsole2, &pClient2) < 0)
            {
                _VoipConciergeRemoveClient(pVoipConcierge, uClient2Id);
                return(VOIPCONCIERGE_ERR_CANNOT_ADD_CONNECTION);
            }
            iClientsCreated += 1;
        }
        #endif

        DirtyCastLogPrintf("voipconcierge: adding connection between clients 0x%08x (tag:%s) and 0x%08x (tag:%s) (req=%d)\n",
            uClient1Id, pGameConsole1->strConsoleTag, uClient2Id, pGameConsole2->strConsoleTag, iRequestId);

        #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
        // update the number of clients
        pClient1->iNumClients += 1;
        pClient2->iNumClients += 1;

        // update the clientid list
        pClient1->aClientIds[pGameConsole1->iClientIdx] = pClient1->uClientId;
        pClient1->aClientIds[pGameConsole2->iClientIdx] = pClient2->uClientId;
        VoipTunnelClientRefreshSendMask(pVoipTunnel, pClient1);

        pClient2->aClientIds[pGameConsole1->iClientIdx] = pClient1->uClientId;
        pClient2->aClientIds[pGameConsole2->iClientIdx] = pClient2->uClientId;
        VoipTunnelClientRefreshSendMask(pVoipTunnel, pClient2);
        #else
        // for both clients involved in the connection, update respective list of clientids for that specific game
        pGameMapping = VoipTunnelClientMatchGameId(pVoipTunnel, pClient1, pGame->uGameId);
        pGameMapping->iNumClients += 1;
        pGameMapping->aClientIds[pGameConsole2->iClientIdx] = pClient2->uClientId;
        VoipTunnelClientRefreshSendMask(pVoipTunnel, pClient1, iGameIdx);

        pGameMapping = VoipTunnelClientMatchGameId(pVoipTunnel, pClient2, pGame->uGameId);
        pGameMapping->iNumClients += 1;
        pGameMapping->aClientIds[pGameConsole1->iClientIdx] = pClient1->uClientId;
        VoipTunnelClientRefreshSendMask(pVoipTunnel, pClient2, iGameIdx);
        #endif

        // update response
        pCurrentOut->uClientId1 = pClient1->uClientId;
        pCurrentOut->uClientId2 = pClient2->uClientId;
    }

    return(iClientsCreated);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeRemoveFromClientList

    \Description
        Deletes each client from the other clients respective client list

    \Input *pVoipTunnel     - instance to the voiptunnel
    \Input iRequestId       - identifier of the request for logging purposes
    \Input uClient1Id       - identifier to the first client
    \Input uClient2Id       - identifier to the second client
    \Input *pClientIds1     - [out] the client list we are updating for client 1
    \Input *pNumClients1    - [out] number of clients in the client list for client 1
    \Input *pClientIds2     - [out] the client list we are updating for client 2
    \Input *pNumClients2    - [out] number of clients in the client list for client 2

    \Output
        int32_t             - zero=success,negative=error

    \Version 01/25/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeRemoveFromClientList(VoipTunnelRefT *pVoipTunnel, int32_t iRequestId, uint32_t uClient1Id, uint32_t uClient2Id, int32_t *pClientIds1, int32_t *pNumClients1, int32_t *pClientIds2, int32_t *pNumClients2)
{
    int32_t iIndex;
    int32_t iClientIdx1 = -1, iClientIdx2 = -1;

    // search the client ids for each of the clients in connection
    for (iIndex = 0; iIndex < VOIPTUNNEL_MAXGROUPSIZE; iIndex += 1)
    {
        if (pClientIds1[iIndex] == 0)
        {
            continue;
        }
        if ((uint32_t)pClientIds1[iIndex] == uClient2Id)
        {
            iClientIdx2 = iIndex;
            break;
        }
    }
    // client 1 is not connected to client 2
    if (iIndex == VOIPTUNNEL_MAXGROUPSIZE)
    {
        DirtyCastLogPrintf("voipconcierge: client id 0x%0x not found in client list for client id 0x%08x (req=%d)\n", uClient2Id, uClient1Id, iRequestId);
        return(VOIPCONCIERGE_ERR_INVALID_CONSOLE_INFO);
    }

    for (iIndex = 0; iIndex < VOIPTUNNEL_MAXGROUPSIZE; iIndex += 1)
    {
        if (pClientIds2[iIndex] == 0)
        {
            continue;
        }
        if ((uint32_t)pClientIds2[iIndex] == uClient1Id)
        {
            iClientIdx1 = iIndex;
            break;
        }
    }
    // client 2 is not connected to client 1
    if (iIndex == VOIPTUNNEL_MAXGROUPSIZE)
    {
        DirtyCastLogPrintf("voipconcierge: client id 0x%0x not found in client list for client id 0x%08x (req=%d)\n", uClient1Id, uClient2Id, iRequestId);
        return(VOIPCONCIERGE_ERR_INVALID_CONSOLE_INFO);
    }

    // remove the other client from the list of client ids
    pClientIds1[iClientIdx2] = 0;
    pClientIds2[iClientIdx1] = 0;
    // update the number of clients
    *pNumClients1 -= 1;
    *pNumClients2 -= 1;

    return(0);
}

#if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
/*F********************************************************************************/
/*!
    \Function _VoipConciergeSuspendInfoMatchId

    \Description
        Tries to find VoipTunnelSuspendInfoT * given a game index

    \Input *pClient             - instance of the Client we are searching in
    \Input iGameIdx             - identifier of the Game we are using to search

    \Output
        VoipTunnelSuspendInfoT * - instance of suspend info or NULL if not found

    \Version 01/25/2016 (eesponda)
*/
/********************************************************************************F*/
static VoipTunnelSuspendInfoT* _VoipConciergeSuspendInfoMatchId(VoipTunnelClientT* pClient, int32_t iGameIdx)
{
    int32_t iGame;
    VoipTunnelSuspendInfoT* pSuspendInfo = NULL;

    // iterator through games and look for a match
    for (iGame = 0; iGame < pClient->iNumSuspended; iGame += 1)
    {
        if (pClient->aSuspendedData[iGame].iGameIdx == iGameIdx)
        {
            pSuspendInfo = &pClient->aSuspendedData[iGame];
            break;
        }
    }

    // return suspend info to caller, or NULL if no match
    return(pSuspendInfo);
}


/*F********************************************************************************/
/*!
    \Function _VoipConciergeRemoveConnection

    \Description
        Takes in the request of hosted connections and deallocates them

    \Input *pVoipConcierge  - ref to the VoipConcierge module
    \Input iGameIdx         - the voiptunnel game index associated with the connection set the connection belongs to
    \Input *pRequestData    - the data for the connections we no longer need to host

    \Output
        int32_t             - number of clients removed or negative if failure

    \Version 01/14/2016 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeRemoveConnection(VoipConciergeRefT *pVoipConcierge, int32_t iGameIdx, int32_t iRequestId, const DeleteDataT *pRequestData)
{
    int32_t iClientsRemoved = 0;
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipConcierge->pBase);
    VoipTunnelClientT *pClient1;
    VoipTunnelClientT *pClient2;

    // validate inputs
    if ((pClient1 = VoipTunnelClientListMatchId(pVoipTunnel, pRequestData->uClientId1)) == NULL)
    {
        DirtyCastLogPrintf("voipconcierge: client id 0x%08x does not exist in the voiptunnel (req=%d)\n", pRequestData->uClientId1, iRequestId);
        return(VOIPCONCIERGE_ERR_INVALID_CONSOLE_INFO);
    }
    if ((pClient2 = VoipTunnelClientListMatchId(pVoipTunnel, pRequestData->uClientId2)) == NULL)
    {
        DirtyCastLogPrintf("voipconcierge: client id 0x%08x does not exist in the voiptunnel (req=%d)\n", pRequestData->uClientId2, iRequestId);
        return(VOIPCONCIERGE_ERR_INVALID_CONSOLE_INFO);
    }

    DirtyCastLogPrintf("voipconcierge: removing connection between clients 0x%08x and 0x%08x (req=%d)\n", pRequestData->uClientId1, pRequestData->uClientId2, iRequestId);

    /*  Either remove the clients from their active or suspended game:

        We will check to make sure each client in the connection is in the same active game
        otherwise we assume that they are removing a suspended game connection.

        We will search for the suspended information using the iGameIdx as a key.

        Removal of client from connection set will only happen in both cases if there are no longer
        any connections left in the context of that game.

        We do not try removing from the cache in the case of removing suspended
        connections because the clients still exist in an active game.  */

        // if both clients are in the active game then follow same logic
    if (pClient1->iGameIdx == iGameIdx && pClient2->iGameIdx == iGameIdx)
    {
        int32_t iResult;

        // try to remove from the client list
        if ((iResult = _VoipConciergeRemoveFromClientList(pVoipTunnel, iRequestId, pRequestData->uClientId1, pRequestData->uClientId2, pClient1->aClientIds, &pClient1->iNumClients, pClient2->aClientIds, &pClient2->iNumClients)) < 0)
        {
            return(iResult);
        }

        // if the client is still marked for deletion, remove it from the tunnel
        if (pClient1->iNumClients == 0)
        {
            VoipTunnelClientListDel(pVoipTunnel, pClient1, -1);     // delete the client from the tunnel
            iClientsRemoved += 1;                                   // increment the clients removed counter

            if (VoipTunnelClientListMatchId(pVoipTunnel, pRequestData->uClientId1) == NULL)
            {
                _VoipConciergeRemoveClient(pVoipConcierge, pRequestData->uClientId1); // delete client ids from the ClientMap

                // need to refresh the pointer to client 2 as deletes invalidate the pointers
                pClient2 = VoipTunnelClientListMatchId(pVoipTunnel, pRequestData->uClientId2);
            }
        }

        if (pClient2->iNumClients == 0)
        {
            VoipTunnelClientListDel(pVoipTunnel, pClient2, -1);     // delete the client from the tunnel
            iClientsRemoved += 1;                                   // increment the clients removed counter

            if (VoipTunnelClientListMatchId(pVoipTunnel, pRequestData->uClientId2) == NULL)
            {
                _VoipConciergeRemoveClient(pVoipConcierge, pRequestData->uClientId2); // delete client ids from the ClientMap
            }
        }

        // no clients were removed from the voiptunnel, we want to refresh the sendmasks (deletes will refresh send masks for us)
        if (iClientsRemoved == 0)
        {
            VoipTunnelClientRefreshSendMask(pVoipTunnel, pClient1);
            VoipTunnelClientRefreshSendMask(pVoipTunnel, pClient2);
        }
    }
    // otherwise one or both clients are not in the active game
    // in the case that only one client is active then this operation is not supported
    // in the case both are suspended it will look for the game and clear the client list
    else
    {
        int32_t iResult;
        VoipTunnelSuspendInfoT *pSuspendInfo1, *pSuspendInfo2;

        if ((pSuspendInfo1 = _VoipConciergeSuspendInfoMatchId(pClient1, iGameIdx)) == NULL)
        {
            DirtyCastLogPrintf("voipconcierge: client id 0x%08x does not belong to suspended connection set %04d (possible mismatch?) (req=%d)\n", pClient1->uClientId, iGameIdx, iRequestId);
            return(VOIPCONCIERGE_ERR_INVALID_CONSOLE_INFO);
        }
        if ((pSuspendInfo2 = _VoipConciergeSuspendInfoMatchId(pClient2, iGameIdx)) == NULL)
        {
            DirtyCastLogPrintf("voipconcierge: client id 0x%08x does not belong to suspended connection set %04d (possible mismatch?) (req=%d)\n", pClient2->uClientId, iGameIdx, iRequestId);
            return(VOIPCONCIERGE_ERR_INVALID_CONSOLE_INFO);
        }

        // try to remove from the client list
        if ((iResult = _VoipConciergeRemoveFromClientList(pVoipTunnel, iRequestId, pRequestData->uClientId1, pRequestData->uClientId2, pSuspendInfo1->aClientIds, &pSuspendInfo1->iNumClients, pSuspendInfo2->aClientIds, &pSuspendInfo2->iNumClients)) < 0)
        {
            return(iResult);
        }

        // if not connected with anyone else in the suspended game just remove
        if (pSuspendInfo1->iNumClients == 0)
        {
            VoipTunnelClientListDel(pVoipTunnel, pClient1, iGameIdx);
        }
        if (pSuspendInfo2->iNumClients == 0)
        {
            VoipTunnelClientListDel(pVoipTunnel, pClient2, iGameIdx);
        }
    }

    return(iClientsRemoved);
}
#else
/*F********************************************************************************/
/*!
    \Function _VoipConciergeRemoveConnection

    \Description
        Takes in the request of hosted connections and deallocates them

    \Input *pVoipConcierge  - ref to the VoipConcierge module
    \Input uConnSetId       - connection set id
    \Input iGameIdx         - the voiptunnel game index associated with the connection set the connection belongs to
    \Input *pRequestData    - the data for the connections we no longer need to host

    \Output
        int32_t             -  number of clients removed or negative if failure

    \Version 01/14/2016 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeRemoveConnection(VoipConciergeRefT* pVoipConcierge, uint32_t uConnSetId, int32_t iGameIdx, int32_t iRequestId, const DeleteDataT* pRequestData)
{
    int32_t iClientsRemoved = 0;
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipConcierge->pBase);
    VoipTunnelClientT *pClient1, *pClient2;
    VoipTunnelGameMappingT *pGameMapping1, *pGameMapping2;
    int32_t iResult;

    // validate inputs
    if ((pClient1 = VoipTunnelClientListMatchId(pVoipTunnel, pRequestData->uClientId1)) == NULL)
    {
        DirtyCastLogPrintf("voipconcierge: rem conn err - client id 0x%08x does not exist in voiptunnel (req=%d)\n", pRequestData->uClientId1, iRequestId);
        return(VOIPCONCIERGE_ERR_INVALID_CONSOLE_INFO);
    }
    if ((pClient2 = VoipTunnelClientListMatchId(pVoipTunnel, pRequestData->uClientId2)) == NULL)
    {
        DirtyCastLogPrintf("voipconcierge: rem conn err - client id 0x%08x does not exist in voiptunnel (req=%d)\n", pRequestData->uClientId2, iRequestId);
        return(VOIPCONCIERGE_ERR_INVALID_CONSOLE_INFO);
    }

    if ((pGameMapping1 = VoipTunnelClientMatchGameId(pVoipTunnel, pClient1, uConnSetId)) == NULL)
    {
        DirtyCastLogPrintf("voipconcierge: rem conn err - client id 0x%08x is not in the client list of conn set (id=0x%08x/index=%04d)  (req=%d)\n",
            pRequestData->uClientId1, uConnSetId, iGameIdx, iRequestId);
        return(VOIPCONCIERGE_ERR_INVALID_CONSOLE_INFO);
    }

    if ((pGameMapping2 = VoipTunnelClientMatchGameId(pVoipTunnel, pClient2, uConnSetId)) == NULL)
    {
        DirtyCastLogPrintf("voipconcierge: rem conn err - client id 0x%08x is not in the client list of conn set (id=0x%08x/index=%04d)  (req=%d)\n",
            pRequestData->uClientId2, uConnSetId, iGameIdx, iRequestId);
        return(VOIPCONCIERGE_ERR_INVALID_CONSOLE_INFO);
    }

    DirtyCastLogPrintf("voipconcierge: removing connection between clients 0x%08x and 0x%08x for conn set (id=0x%08x/index=%04d) (req=%d)\n",
        pRequestData->uClientId1, pRequestData->uClientId2, uConnSetId, iGameIdx, iRequestId);

    // try to remove from the client list
    if ((iResult = _VoipConciergeRemoveFromClientList(pVoipTunnel, iRequestId, pRequestData->uClientId1, pRequestData->uClientId2, pGameMapping1->aClientIds, &pGameMapping1->iNumClients, pGameMapping2->aClientIds, &pGameMapping2->iNumClients)) < 0)
    {
        return(iResult);
    }

    // if the client is no longer connected to anybody in that game, remove it from that voiptunnel game
    if (pGameMapping1->iNumClients == 0)
    {
        VoipTunnelClientListDel(pVoipTunnel, pClient1, iGameIdx); // delete the client from the tunnel

        // if the client is no longer known to voiptunnel, then delete the client id from the voipconcierge client map
        if (VoipTunnelClientListMatchId(pVoipTunnel, pRequestData->uClientId1) == NULL)
        {
            _VoipConciergeRemoveClient(pVoipConcierge, pRequestData->uClientId1); // delete client ids from the ClientMap

            /* need to refresh pClient2 and pGameMapping2 because they were potentially invalidated 
               by the implicit memmove in VoipTunnelClientListDel() */
            pClient2 = VoipTunnelClientListMatchId(pVoipTunnel, pRequestData->uClientId2);
            pGameMapping2 = VoipTunnelClientMatchGameId(pVoipTunnel, pClient2, uConnSetId);
        }

        pClient1 = NULL;
        iClientsRemoved += 1;                                     // increment the clients removed counter
    }

    // if the client is no longer connected to anybody in that game, remove it from that voiptunnel game
    if (pGameMapping2->iNumClients == 0)
    {
        VoipTunnelClientListDel(pVoipTunnel, pClient2, iGameIdx); // delete the client from the tunnel

        // if the client is no longer known to voiptunnel, then delete the client id from the voipconcierge client map
        if (VoipTunnelClientListMatchId(pVoipTunnel, pRequestData->uClientId2) == NULL)
        {
            _VoipConciergeRemoveClient(pVoipConcierge, pRequestData->uClientId2); // delete client ids from the ClientMap
        }

        pClient2 = NULL;
        iClientsRemoved += 1;                                     // increment the clients removed counter
    }

    if (pClient1 != NULL)
    {
        VoipTunnelClientRefreshSendMask(pVoipTunnel, pClient1, iGameIdx);
    }
    if (pClient2 != NULL)
    {
        VoipTunnelClientRefreshSendMask(pVoipTunnel, pClient2, iGameIdx);
    }
 
    return(iClientsRemoved);
}
#endif


/*F********************************************************************************/
/*!
    \Function _VoipConciergeDecodeConsoleJson

    \Description
        Decodes the console information portion of the json

    \Input *pJson           - parsing information
    \Input *pConsoleInfo    - section containing the information to parse
    \Input iIndex           - index into which connection we are parsing
    \Input *pGameConsole    - [in/out] array of consoles
    \Input *pNumConnsoles   - [in/out] number of game consoles

    \Output
        int32_t             - success=index into game console array, error=negative

    \Version 09/28/2015 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeDecodeConsoleJson(const uint16_t *pJson, const char *pConsoleInfo, int32_t iIndex, GameConsoleT *pGameConsoles, int32_t *pNumConsoles)
{
    GameConsoleT GameConsole;
    int32_t iGameConsole;

    // memclear GameConsole to avoid uninitialized bytes negatively impacting the result of the memcmp() performed further down in this function
    ds_memclr(&GameConsole, sizeof(GameConsole));

    // pull the data out of json, if the field is not found NULL will be returned which will fall into the default case
    JsonGetString(JsonFind2(pJson, pConsoleInfo, ".prototunnel_key", iIndex), GameConsole.strTunnelKey, sizeof(GameConsole.strTunnelKey), "");
    GameConsole.iClientIdx = JsonGetInteger(JsonFind2(pJson, pConsoleInfo, ".game_console_index", iIndex), -1);
    JsonGetString(JsonFind2(pJson, pConsoleInfo, ".game_console_unique_tag", iIndex), GameConsole.strConsoleTag, sizeof(GameConsole.strConsoleTag), "");

    // validate console info
    if ((*GameConsole.strTunnelKey == '\0') || (GameConsole.iClientIdx < 0) || (GameConsole.iClientIdx > VOIPTUNNEL_MAXGROUPSIZE) || (*GameConsole.strConsoleTag == '\0'))
    {
        DirtyCastLogPrintf("voipconcierge: invalid console info found when decoding json (key=%s, idx=%d, tag=%s)\n",
            GameConsole.strTunnelKey, GameConsole.iClientIdx, GameConsole.strConsoleTag);
        return(-1);
    }

    for (iGameConsole = 0; iGameConsole < *pNumConsoles; iGameConsole += 1)
    {
        const GameConsoleT *pExistingConsole = &pGameConsoles[iGameConsole];

        // check to see if we already have the console tracked in the list
        if (memcmp(pExistingConsole, &GameConsole, sizeof(GameConsole)) == 0)
        {
            break;
        }
        /* if it is not an exact match, make sure there are no collision that exists.
           in the case that the client index matches but the console tag differs it would end up generating a different client id.
           in the case that the tunnel key differs it would end up refcounting which is not desired in this context. */
        if (GameConsole.iClientIdx == pExistingConsole->iClientIdx)
        {
            DirtyCastLogPrintf("voipconcierge: trying to add a console with matching client index %d but mismatching console info new (key=%s, tag=%s) / existing (key=%s, tag=%s)\n",
                GameConsole.iClientIdx, GameConsole.strTunnelKey, GameConsole.strConsoleTag, pExistingConsole->strTunnelKey, pExistingConsole->strConsoleTag);
            iGameConsole = -1;
            break;
        }
    }
    // if not found add to the array
    if (iGameConsole == *pNumConsoles)
    {
        if (*pNumConsoles < VOIPTUNNEL_MAXGROUPSIZE)
        {
            iGameConsole = (*pNumConsoles)++;
            ds_memcpy(&pGameConsoles[iGameConsole], &GameConsole, sizeof(GameConsole));
        }
        else
        {
            DirtyCastLogPrintf("voipconcierge: tried to add a console that would exceed the total number supported\n");
            // exceeded the allowed of number of consoles
            iGameConsole = -1;
        }
    }
    return(iGameConsole);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeDecodeJson

    \Description
        Decodes in the request from the CCS for hosted connection

    \Input *pBuffer     - raw json buffer
    \Input iBufSize     - size of the buffer
    \Input *pRequest    - [out] output for the parsed json

    \Version 09/28/2015 (eesponda)
*/
/********************************************************************************F*/
static void _VoipConciergeDecodeJson(const char *pBuffer, int32_t iBufSize, HostedConnectionRequestT *pRequest)
{
    uint16_t *pJson;
    const char *pCurrent;
    int32_t iMemGroup, iTableSize;
    void *pMemUserData;

    // check that there's something to parse
    if ((pBuffer == NULL) || (iBufSize <= 0))
    {
        return;
    }

    // query for the required table size
    if ((iTableSize = JsonParse(NULL, 0, pBuffer, iBufSize)) == 0)
    {
        DirtyCastLogPrintf("voipconcierge: failed to obtain table size from parser\n");
        return;
    }

    // allocate some memory for parsing
    DirtyMemGroupQuery(&iMemGroup, &pMemUserData);
    if ((pJson = (uint16_t *)DirtyMemAlloc(iTableSize, VOIPCONCIERGE_MEMID, iMemGroup, pMemUserData)) == NULL)
    {
        DirtyCastLogPrintf("voipconcierge: failed to allocate space to parse json\n");
        return;
    }
    ds_memclr(pJson, iTableSize);

    // start the parsing
    if (JsonParse(pJson, iTableSize, pBuffer, iBufSize) == 0)
    {
        DirtyCastLogPrintf("voipconcierge: buffer too small for parsing\n");
        DirtyMemFree(pJson, VOIPCONCIERGE_MEMID, iMemGroup, pMemUserData);
        return;
    }

    // parse out the request id, which is used for debugging
    if ((pCurrent = JsonFind(pJson, "request_id")) != NULL)
    {
        pRequest->iRequestId = JsonGetInteger(pCurrent, 0);
    }

    // parse out the product the request is from
    if (pRequest->eRequestType == REQUEST_TYPE_POST)
    {
        if ((pCurrent = JsonFind(pJson, "product")) != NULL)
        {
            JsonGetString(pCurrent, pRequest->RequestData.PostData.strProduct, sizeof(pRequest->RequestData.PostData.strProduct), "");
        }

        // parse out the leasetime (in minutes) of the connection set and convert to milliseconds
        if ((pCurrent = JsonFind(pJson, "leasetime")) != NULL)
        {
            pRequest->RequestData.PostData.uLeaseTime = (uint32_t)JsonGetInteger(pCurrent, 0);
        }
    }

    /*  only parse the ccs_client_id when it wasn't already parsed from the URI
        this happens in DELETE all cases */
    if (pRequest->strClientId[0] == '\0')
    {
        // parse out the ccs client id the request is from
        if ((pCurrent = JsonFind(pJson, "ccs_client_id")) != NULL)
        {
            JsonGetString(pCurrent, pRequest->strClientId, sizeof(pRequest->strClientId), "");
        }
    }

    if (pRequest->eRequestType == REQUEST_TYPE_POST)
    {
        PostDataT *pPostData = &pRequest->RequestData.PostData;

        // parse out the connections
        while ((pCurrent = JsonFind2(pJson, NULL, "connections[", pPostData->iNumConnections)) != NULL)
        {
            const char *pConsoleInfo;
            HostedConnectionT *pHostedConnection = &pPostData->aConnections[pPostData->iNumConnections];
            if ((pConsoleInfo = JsonFind2(pJson, pCurrent, ".game_console_1", pPostData->iNumConnections)) != NULL)
            {
                pHostedConnection->iGameConsoleIndex1 = _VoipConciergeDecodeConsoleJson(pJson, pConsoleInfo, pPostData->iNumConnections, pPostData->aConsoles, &pPostData->iNumConsoles);
            }
            if ((pConsoleInfo = JsonFind2(pJson, pCurrent, ".game_console_2", pPostData->iNumConnections)) != NULL)
            {
                pHostedConnection->iGameConsoleIndex2 = _VoipConciergeDecodeConsoleJson(pJson, pConsoleInfo, pPostData->iNumConnections, pPostData->aConsoles, &pPostData->iNumConsoles);
            }
            pPostData->iNumConnections += 1;

            // sanity check in case we reach our maximum
            if (pPostData->iNumConnections == VOIPCONCIERGE_MAXCONNECTIONS)
            {
                DirtyCastLogPrintf("voipconcierge: exiting out of connections json parsing as we have reached our maximum allowed number (req=%d)\n", pRequest->iRequestId);
                break;
            }
        }
    }

    DirtyMemFree(pJson, VOIPCONCIERGE_MEMID, iMemGroup, pMemUserData);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeEncodePostJson

    \Description
        Encodes the response for the POST into a json buffer

    \Input *pBuffer     - location to write json to
    \Input *pResponse   - the data we are formatting into json

    \Version 09/28/2015 (eesponda)
*/
/********************************************************************************F*/
static void _VoipConciergeEncodePostJson(char *pBuffer, const HostedConnectionResponseT *pResponse)
{
    int32_t iIndex;
    char strAddrText[20];

    // if any failure happens with any of these formatting functions just return out to finish the formatting early
    // this is to prevent a condition where we overrun the buffer

    // only write the request id back when valid
    if ((pResponse->iRequestId > 0) && (JsonAddInt(pBuffer, "request_id", pResponse->iRequestId) != JSON_ERR_NONE))
    {
        return;
    }

    // only write the json if we allocated any resources, in cases where we are only extending the
    // lease this will not happen
    if (pResponse->iNumResources > 0)
    {
        if (JsonArrayStart(pBuffer, "allocated_resources") != JSON_ERR_NONE)
        {
            return;
        }
        for (iIndex = 0; iIndex < pResponse->iNumResources; iIndex += 1)
        {
            const AllocatedResourceT *pResource = &pResponse->aResources[iIndex];

            if (JsonObjectStart(pBuffer, NULL) != JSON_ERR_NONE)
            {
                return;
            }
            if (JsonAddInt(pBuffer, "game_console_id_1", pResource->uClientId1) != JSON_ERR_NONE)
            {
                return;
            }
            if (JsonAddInt(pBuffer, "game_console_id_2", pResource->uClientId2) != JSON_ERR_NONE)
            {
                return;
            }
            if (JsonObjectEnd(pBuffer) != JSON_ERR_NONE)
            {
                return;
            }
        }
        if (JsonArrayEnd(pBuffer) != JSON_ERR_NONE)
        {
            return;
        }
    }

    if (JsonAddStr(pBuffer, "ip", SocketInAddrGetText(pResponse->uAddress, strAddrText, sizeof(strAddrText))) != JSON_ERR_NONE)
    {
        return;
    }
    if (JsonAddInt(pBuffer, "port", pResponse->uPort) != JSON_ERR_NONE)
    {
        return;
    }
    if (JsonAddInt(pBuffer, "hosting_server_id", pResponse->uHostingServerId) != JSON_ERR_NONE)
    {
        return;
    }
    if ((pResponse->iConnSet > 0) && (JsonAddInt(pBuffer, "conn_set_id", pResponse->iConnSet) != JSON_ERR_NONE))
    {
        return;
    }
    if (JsonAddInt(pBuffer, "capacity", pResponse->uCapacity) != JSON_ERR_NONE)
    {
        return;
    }
    if (JsonAddInt(pBuffer, "leasetime", pResponse->uLeaseTime) != JSON_ERR_NONE)
    {
        return;
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeEncodeDeleteJson

    \Description
        Encodes the response for the DELETE query into a json buffer

    \Input *pBuffer     - location to write json to
    \Input *pResponse   - the data we are formatting into json

    \Version 09/28/2015 (eesponda)
*/
/********************************************************************************F*/
static void _VoipConciergeEncodeDeleteJson(char *pBuffer, const HostedConnectionResponseT *pResponse)
{
    // if any failure happens with any of these formatting functions just return out to finish the formatting early
    // this is to prevent a condition where we overrun the buffer

    // only write the request id back when valid
    if ((pResponse->iRequestId > 0) && (JsonAddInt(pBuffer, "request_id", pResponse->iRequestId) != JSON_ERR_NONE))
    {
        return;
    }

    if (JsonAddInt(pBuffer, "capacity", pResponse->uCapacity) != JSON_ERR_NONE)
    {
        return;
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeEncodeJsonErr

    \Description
        Encodes the error response into a json buffer

    \Input *pBuffer      - location to write json to
    \Input *pResponse    - the data we are formatting into json
    \Input iRequestId    - the id sent in the request

    \Version 10/02/2015 (eesponda)
*/
/********************************************************************************F*/
static void _VoipConciergeEncodeJsonErr(char *pBuffer, const GenericErrorResponseT *pResponse, int32_t iRequestId)
{
    // if any failure happens with any of these formatting functions just return out to finish the formatting early
    // this is to prevent a condition where we overrun the buffer

    // only write the request id back when valid
    if ((iRequestId > 0) && (JsonAddInt(pBuffer, "request_id", iRequestId) != JSON_ERR_NONE))
    {
        return;
    }
    if (JsonAddStr(pBuffer, "message", pResponse->pText) != JSON_ERR_NONE)
    {
        return;
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeEncodePostSuccess

    \Description
        Utility to take our POST response and encode it correctly

    \Input *pHostedResponse - internal response data we use to encode
    \Input *pResponse       - response data used to create http response

    \Version 10/05/2015 (eesponda)
*/
/********************************************************************************F*/
static void _VoipConciergeEncodePostSuccess(const HostedConnectionResponseT *pHostedResponse, ProtoHttpServResponseT *pResponse)
{
    char *pBuffer;
    char strBuffer[VOIPCONCIERGE_RESPONSE_BODY_SIZE];
    RestApiResponseDataT *pResponseData = RestApiGetResponseData(pResponse);

    // init the json formatter
    JsonInit(strBuffer, sizeof(strBuffer), FALSE);

    // format the response body
    _VoipConciergeEncodePostJson(strBuffer, pHostedResponse);

    // finish the formatting
    pBuffer = JsonFinish(strBuffer);
    ds_strnzcpy(pResponseData->pBody, pBuffer, sizeof(strBuffer));
    pResponse->iContentLength = strlen(pResponseData->pBody);

    // set the http code, additional headers based on if we are created or updating
    if (pHostedResponse->iConnSet > 0)
    {
        pResponse->eResponseCode = PROTOHTTP_RESPONSE_CREATED;
        pResponse->iHeaderLen += ds_snzprintf(pResponse->strHeader, sizeof(pResponse->strHeader)-pResponse->iHeaderLen,
            "Location: %s/%d\r\n", VOIPCONCIERGE_BASE_HOSTEDCONN, pHostedResponse->iConnSet);
    }
    else
    {
        pResponse->eResponseCode = PROTOHTTP_RESPONSE_OK;
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeEncodeDeleteSuccess

    \Description
        Utility to take our DELETE response and encode it correctly

    \Input *pHostedResponse - internal response data we use to encode
    \Input *pResponse       - response data used to create http response

    \Version 10/05/2015 (eesponda)
*/
/********************************************************************************F*/
static void _VoipConciergeEncodeDeleteSuccess(const HostedConnectionResponseT *pHostedResponse, ProtoHttpServResponseT *pResponse)
{
    char *pBuffer;
    char strBuffer[VOIPCONCIERGE_RESPONSE_BODY_SIZE];
    RestApiResponseDataT *pResponseData = RestApiGetResponseData(pResponse);

    // if we actually deleted anything return data back, otherwise just return 204 (no content)
    if (pHostedResponse->iNumResources > 0)
    {
        // init the json formatter
        JsonInit(strBuffer, sizeof(strBuffer), FALSE);

        // format the response body
        _VoipConciergeEncodeDeleteJson(strBuffer, pHostedResponse);

        // finish the formatting
        pBuffer = JsonFinish(strBuffer);

        ds_strnzcpy(pResponseData->pBody, pBuffer, sizeof(strBuffer));
        pResponse->iContentLength = strlen(pResponseData->pBody);
        pResponse->eResponseCode = PROTOHTTP_RESPONSE_OK;
    }
    else
    {
        pResponse->eResponseCode = PROTOHTTP_RESPONSE_NOCONTENT;
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeEncodeError

    \Description
        Encodes the error response into a json buffer

    \Input *pError    - error data we use to encode
    \Input iRequestId - id sent with the request
    \Input *pResponse - [out] response data used to create http response

    \Version 10/05/2015 (eesponda)
*/
/********************************************************************************F*/
static void _VoipConciergeEncodeError(const GenericErrorResponseT *pError, int32_t iRequestId, ProtoHttpServResponseT *pResponse)
{
    char *pBuffer;
    char strBuffer[VOIPCONCIERGE_RESPONSE_BODY_SIZE];
    RestApiResponseDataT *pResponseData = RestApiGetResponseData(pResponse);

    // init the json formatter
    JsonInit(strBuffer, sizeof(strBuffer), FALSE);

    // format the error json
    _VoipConciergeEncodeJsonErr(strBuffer, pError, iRequestId);

    // finish the formatting
    pBuffer = JsonFinish(strBuffer);

    ds_strnzcpy(pResponseData->pBody, pBuffer, sizeof(strBuffer));
    pResponse->iContentLength = strlen(pResponseData->pBody);
    pResponse->eResponseCode = pError->eResponseCode;
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeCreateConnectionSet

    \Description
        Creates a new connection set and adds connections to it

    \Input *pVoipConcierge  - module state
    \Input *pRequest        - data used to create connection set / connections
    \Input *pResponse       - data used to respond back to clients

    \Output
        int32_t             - zero=success,negative=error

    \Version 09/28/2015 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeCreateConnectionSet(VoipConciergeRefT *pVoipConcierge, const HostedConnectionRequestT *pRequest, HostedConnectionResponseT *pResponse)
{
    int32_t iIndex;
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipConcierge->pBase);
    const int32_t iMaxGames = VoipTunnelStatus(pVoipTunnel, 'mgam', 0, NULL, 0);
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipConcierge->pBase);

    for (iIndex = 0; iIndex < iMaxGames; iIndex += 1)
    {
        int32_t iResult;
        uint32_t uConnSet;
        char strTime[32];
        const VoipTunnelGameT *pGame;

        /* Make sure that the connection set is not active
           Currently 'nusr' will just return 0 for an uninit game because
           of the use of VoipTunnelRefT.ClientList to count the games that
           match instead of VoipTunnelRefT.GameList[iValue].iNumClients
           If we can change that behaviour that would be a preferable way to query */
        if (VoipTunnelStatus(pVoipTunnel, 'nusr', iIndex, NULL, 0) != -1)
        {
            continue;
        }
        uConnSet = _VoipConciergeGenerateGameId();

        // add the game to the voiptunnel
        if ((iResult = VoipTunnelGameListAdd2(pVoipTunnel, iIndex, uConnSet)) < 0)
        {
            DirtyCastLogPrintf("voipconcierge: could not add connection set (id=0x%08x/index=%04d) to voiptunnel (req=%d error=%d)\n", uConnSet, iIndex, pRequest->iRequestId, iResult);
            return(VOIPCONCIERGE_ERR_CANNOT_ADD_CONNSET);
        }
        pGame = VoipTunnelGameListMatchIndex(pVoipTunnel, iIndex);

        // convert the lease time into milliseconds so it can be used with NetTickDiff
        pVoipConcierge->pConnectionSets[iIndex].uExpireTime = NetTick() + (pRequest->RequestData.PostData.uLeaseTime * (60*1000)) + VOIPCONCIERGE_EXPIRY_GRACE;
        // convert the lease time into seconds and print out the time it expires
        DirtyCastLogPrintf("voipconcierge: connection set (id=0x%08x/index=%04d) lease extended until %s (req=%d)\n",
            uConnSet, iIndex, DirtyCastCtime(strTime, sizeof(strTime), time(NULL) + (pRequest->RequestData.PostData.uLeaseTime*60)), pRequest->iRequestId);

        // add the clients to the voiptunnel
        if ((iResult = _VoipConciergeAddConnection(pVoipConcierge, iIndex, pRequest->iRequestId, pRequest->strClientId, &pRequest->RequestData.PostData, pResponse->aResources)) < 0)
        {
            int32_t iGameClient;
            uint32_t aClientList[VOIPTUNNEL_MAXGROUPSIZE];

            // pointer will be valid as it was created in the context of the this function
            ds_memcpy(aClientList, pGame->aClientList, sizeof(aClientList));

            DirtyCastLogPrintf("voipconcierge: could not add connections to set (id=0x%08x/index=%04d) (req=%d error=%d)\n", uConnSet, iIndex, pRequest->iRequestId, iResult);
            VoipTunnelGameListDel(pVoipTunnel, iIndex);

            // remove clients that are no longer active
            for (iGameClient = 0; iGameClient < VOIPTUNNEL_MAXGROUPSIZE; iGameClient += 1)
            {
                uint32_t uClientId;
                if ((uClientId = aClientList[iGameClient]) == 0)
                {
                    continue;
                }
                if (VoipTunnelClientListMatchId(pVoipTunnel, uClientId) != NULL)
                {
                    continue;
                }

                _VoipConciergeRemoveClient(pVoipConcierge, uClientId);
            }

            return(iResult);
        }

        // update the response data
        pResponse->iRequestId = pRequest->iRequestId;
        pResponse->uLeaseTime = pRequest->RequestData.PostData.uLeaseTime;
        pResponse->uAddress = pConfig->uFrontAddr;
        pResponse->uPort = pConfig->uTunnelPort;
        pResponse->uHostingServerId = pVoipConcierge->uHostingServerId;
        pResponse->iConnSet = (int32_t)uConnSet;
        pResponse->iNumResources = pRequest->RequestData.PostData.iNumConnections;
        pResponse->uCapacity = ((VoipServerStatus(pVoipConcierge->pBase, 'psta', 0, NULL, 0) != VOIPSERVER_PROCESS_STATE_RUNNING) || pVoipConcierge->bMaxLoad) ? 0 :
            (VoipTunnelStatus(pVoipTunnel, 'musr', 0, NULL, 0) - VoipTunnelStatus(pVoipTunnel, 'nusr', -1, NULL, 0));

        // success
        DirtyCastLogPrintf("voipconcierge: added new connection set (id=0x%08x/index=%04d/clients=%d) (req=%d)\n", uConnSet, iIndex, pGame->iNumClients, pRequest->iRequestId);
        return(0);
    }

    // full
    DirtyCastLogPrintf("voipconcierge: we are full (req=%d)\n", pRequest->iRequestId);
    return(VOIPCONCIERGE_ERR_FULL);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeUpdateConnectionSet

    \Description
        Updates the connection set with no connections

    \Input *pVoipConcierge  - module state
    \Input *pRequest        - data used to create connection set / connections
    \Input *pResponse       - data used to respond back to clients

    \Output
        int32_t             - zero=success,negative=error

    \Version 09/28/2015 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeUpdateConnectionSet(VoipConciergeRefT *pVoipConcierge, const HostedConnectionRequestT *pRequest, HostedConnectionResponseT *pResponse)
{
    int32_t iResult;
    int32_t iConnSet;
    char strTime[32];
    const VoipTunnelGameT *pGame;
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipConcierge->pBase);
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipConcierge->pBase);

    // try to match a game to this connection set
    if ((pGame = VoipTunnelGameListMatchId(pVoipTunnel, (uint32_t)pRequest->iConnSet, &iConnSet)) == NULL)
    {
        DirtyCastLogPrintf("voipconcierge: connection set 0x%08x is inactive (req=%d)\n", pRequest->iConnSet, pRequest->iRequestId);
        return(VOIPCONCIERGE_ERR_CONNSET_INACTIVE);
    }

    // add clients to the voiptunnel
    if (pRequest->RequestData.PostData.iNumConnections > 0)
    {
        if ((iResult = _VoipConciergeAddConnection(pVoipConcierge, iConnSet, pRequest->iRequestId, pRequest->strClientId, &pRequest->RequestData.PostData, pResponse->aResources)) < 0)
        {
            DirtyCastLogPrintf("voipconcierge: could not add connections to set 0x%08x (req=%d error=%d)\n", pRequest->iConnSet, pRequest->iRequestId, iResult);
            return(VOIPCONCIERGE_ERR_CANNOT_ADD_CONNECTION);
        }
    }
    // convert the lease time into milliseconds so it can be used with NetTickDiff
    pVoipConcierge->pConnectionSets[iConnSet].uExpireTime = NetTick() + (pRequest->RequestData.PostData.uLeaseTime * (60*1000)) + VOIPCONCIERGE_EXPIRY_GRACE;
    // convert the lease time into seconds and print out the time it expires
    DirtyCastLogPrintf("voipconcierge: connection set (id=0x%08x/index=%04d) lease extended until %s (req=%d)\n",
        pRequest->iConnSet, iConnSet, DirtyCastCtime(strTime, sizeof(strTime), time(NULL) + (pRequest->RequestData.PostData.uLeaseTime*60)), pRequest->iRequestId);

    // update the response data
    pResponse->iRequestId = pRequest->iRequestId;
    pResponse->uLeaseTime = pRequest->RequestData.PostData.uLeaseTime;
    pResponse->uAddress = pConfig->uFrontAddr;
    pResponse->uPort = pConfig->uTunnelPort;
    pResponse->uHostingServerId = pVoipConcierge->uHostingServerId;
    pResponse->iNumResources = pRequest->RequestData.PostData.iNumConnections;
    pResponse->uCapacity = ((VoipServerStatus(pVoipConcierge->pBase, 'psta', 0, NULL, 0) != VOIPSERVER_PROCESS_STATE_RUNNING) || pVoipConcierge->bMaxLoad) ? 0 :
        (VoipTunnelStatus(pVoipTunnel, 'musr', 0, NULL, 0) - VoipTunnelStatus(pVoipTunnel, 'nusr', -1, NULL, 0));

    // success
    DirtyCastLogPrintf("voipconcierge: updated connection set (id=0x%08x/index=%04d/clients=%d) (req=%d)\n", pRequest->iConnSet, iConnSet, pGame->iNumClients, pRequest->iRequestId);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeValidateToken

    \Description
        Checks to make sure the token is not expired and has correct scope

    \Input *pTokenInfo  - the information we are using to validate

    \Output
        uint8_t         - TRUE when valid, FALSE otherwise

    \Version 10/20/2015 (eesponda)
*/
/********************************************************************************F*/
static uint8_t _VoipConciergeValidateToken(const TokenApiInfoDataT *pTokenInfo)
{
    if ((NetTickDiff(pTokenInfo->iExpiresIn, NetTick()) > 0) &&
        (ds_stristr(pTokenInfo->strScopes, VOIPCONCIERGE_SCOPE) != NULL))
    {
        return(TRUE);
    }
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeTokenCb

    \Description
        Callback that is fired when the tokeninfo request is complete

    \Input *pAccessToken - the access token the response is for
    \Input *pData        - the response information (union based on request)
    \Input *pUserData    - contains our hosted request data to update

    \Version 10/20/2015 (eesponda)
*/
/********************************************************************************F*/
static void _VoipConciergeTokenCb(const char *pAccessToken, const TokenApiInfoDataT *pData, void *pUserData)
{
    VoipConciergeRefT *pVoipConcierge = (VoipConciergeRefT *)pUserData;
    HostedConnectionRequestT *pHostedRequest;

    // find and delete the hosted request
    if ((pHostedRequest = HashStrDel(pVoipConcierge->pValidatingRequests, pAccessToken)) == NULL)
    {
        DirtyCastLogPrintf("voipconcierge: could not find the validating request for the newly validated token, probably deleted earlier via the RestApi complete callback\n");
        return;
    }

    /* if the request failed, the voipserver itself is at fault
       or the backend is having trouble
       ie: failure to connect to backend or server returns 500

       in both these cases these are not a fault of the caller */
    if (pData == NULL)
    {
        pHostedRequest->eState = REQUEST_STATE_FAILURE;
    }
    /* if the request succeeded but resulted in an error
       this is the fault of the caller. in this case we
       would send back an empty response that would be
       evaluated as a forbidden (403).

       note: if the voipserver is configured with an invalid
       cert we might get an error from the backend but it is
       impossible for this code path to be hit since registration
       with the CCS would fail */
    else
    {
        pHostedRequest->eState = _VoipConciergeValidateToken(pData) ?
            REQUEST_STATE_AUTHORIZED : REQUEST_STATE_UNAUTHORIZED;
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeProcessToken

    \Description
        Either pulls data from the cache or fires a new request for token info

    \Input *pVoipConcierge  - module state
    \Input *pRequestData    - http request data, contains our access token
    \Input *pHostedRequest  - resource request data, stores the current state of validation

    \Output
        int32_t             - zero=success,negative=error

    \Version 10/20/2015 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeProcessToken(VoipConciergeRefT *pVoipConcierge, const RestApiRequestDataT *pRequestData, HostedConnectionRequestT *pHostedRequest)
{
    const TokenApiInfoDataT *pTokenInfo;
    TokenApiRefT *pTokenRef = VoipServerGetTokenApi(pVoipConcierge->pBase);

    // try to get cached token
    if ((pTokenInfo = TokenApiInfoQuery(pTokenRef, pRequestData->strAccessToken)) != NULL)
    {
        /* if there are any requests validating, set this request to wait
           given that we do not know if the incoming requests are dependent on the validating requests we want to ensure
           no problems due to dependencies between requests. this means that we never process requests if there are
           current request validating */
        if (HasherCount(pVoipConcierge->pValidatingRequests) > 0)
        {
            pHostedRequest->eState = REQUEST_STATE_WAITING;
        }
        // otherwise we can just validate the token
        else if (_VoipConciergeValidateToken(pTokenInfo) == TRUE)
        {
            pHostedRequest->eState = REQUEST_STATE_AUTHORIZED;
        }
        else
        {
            return(VOIPCONCIERGE_ERR_FORBIDDEN);
        }
    }
    // need to request a token?
    else if (pTokenInfo == NULL)
    {
        // check for an existing request, if found just set the request to waiting
        if (HashStrFind(pVoipConcierge->pValidatingRequests, pRequestData->strAccessToken) != NULL)
        {
            pHostedRequest->eState = REQUEST_STATE_WAITING;
        }
        // otherwise we can kick off the new request
        else
        {
            if (HashStrAdd(pVoipConcierge->pValidatingRequests, pRequestData->strAccessToken, pHostedRequest) < 0)
            {
                return(VOIPCONCIERGE_ERR_SYSTEM);
            }
            else if (TokenApiInfoQueryAsync(pTokenRef, pRequestData->strAccessToken, _VoipConciergeTokenCb, pVoipConcierge) < 0)
            {
                HashStrDel(pVoipConcierge->pValidatingRequests, pRequestData->strAccessToken);
                return(VOIPCONCIERGE_ERR_SYSTEM);
            }
            pHostedRequest->eState = REQUEST_STATE_VALIDATING;
        }
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeValidateJson

    \Description
        Validate the decoded request is the values we expect

    \Input *pVoipConcierge - module state
    \Input *pHostedRequest - hosted connection request data

    \Output
        int32_t            - 0=success, negative=error

    \Version 02/11/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeValidateJson(VoipConciergeRefT *pVoipConcierge, const HostedConnectionRequestT *pRequest)
{
    int32_t iConn, iGameIdx = -1;
    const PostDataT *pPostData;
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipConcierge->pBase);

    if (pRequest->eRequestType != REQUEST_TYPE_POST)
    {
        // we only validate in the case of json data
        return(0);
    }
    pPostData = &pRequest->RequestData.PostData;

    // we only require the connection data when it is a create hosted connection
    // request. in cases where we dont get this data for update we just dont add new connections
    if (pRequest->iConnSet <= 0)
    {
        if (pRequest->RequestData.PostData.iNumConnections <= 0)
        {
            DirtyCastLogPrintf("voipconcierge: request validation failed no connections present\n");
            return(VOIPCONCIERGE_ERR_INVALIDPARAM);
        }
    }
    // otherwise we translate what game index we are talking to in voiptunnel terms
    else
    {
        VoipTunnelGameListMatchId(pVoipTunnel, pRequest->iConnSet, &iGameIdx);
    }

    if (pPostData->strProduct[0] == '\0')
    {
        DirtyCastLogPrintf("voipconcierge: request validation failed no product information\n");
        return(VOIPCONCIERGE_ERR_INVALIDPARAM);
    }
    else if (pRequest->strClientId[0] == '\0')
    {
        DirtyCastLogPrintf("voipconcierge: request validation failed missing client id information\n");
        return(VOIPCONCIERGE_ERR_INVALIDPARAM);
    }
    else if (pPostData->uLeaseTime == 0)
    {
        DirtyCastLogPrintf("voipconcierge: request validation failed invalid leasetime information\n");
        return(VOIPCONCIERGE_ERR_INVALIDPARAM);
    }

    // if we have any connection data then we should validate it
    for (iConn = 0; iConn < pPostData->iNumConnections; iConn += 1)
    {
        VoipTunnelClientT *pClient;
        uint32_t uClientId1, uClientId2;
        const GameConsoleT *pConsole1, *pConsole2;
        const HostedConnectionT *pConnection = &pPostData->aConnections[iConn];
        #if DIRTYVERS > DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
        VoipTunnelGameMappingT *pGameMapping;
        #endif

        if ((pConnection->iGameConsoleIndex1 < 0) || (pConnection->iGameConsoleIndex2 < 0))
        {
            // no need to log here, we log at the time where the game console index is generated
            return(VOIPCONCIERGE_ERR_INVALID_CONSOLE_INFO);
        }
        pConsole1 = &pPostData->aConsoles[pConnection->iGameConsoleIndex1];
        pConsole2 = &pPostData->aConsoles[pConnection->iGameConsoleIndex2];

        if ((pConsole1->iClientIdx == pConsole2->iClientIdx) || (strcmp(pConsole1->strTunnelKey, pConsole2->strTunnelKey) == 0))
        {
            DirtyCastLogPrintf("voipconcierge: cannot create connection using same console info\n");
            return(VOIPCONCIERGE_ERR_INVALID_CONSOLE_INFO);
        }

        /* check to make sure that we don't already have other clients occupying the slots already
           for the game that we are currently _updating_
           we pass FALSE to the gen slot since we only want to lookup and not generate if one doesn't exist */

        uClientId1 = _VoipConciergeGetClientId(pVoipConcierge, pRequest->iRequestId, pRequest->strClientId, pConsole1->strConsoleTag, pPostData->strProduct, FALSE);
        uClientId2 = _VoipConciergeGetClientId(pVoipConcierge, pRequest->iRequestId, pRequest->strClientId, pConsole2->strConsoleTag, pPostData->strProduct, FALSE);

        #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
        if (((pClient = VoipTunnelClientListMatchId(pVoipTunnel, uClientId1)) != NULL) && (pClient->aClientIds[pConsole2->iClientIdx] != 0) && (pClient->iGameIdx == iGameIdx))
        {
            DirtyCastLogPrintf("voipconcierge: cannot update client 0x%08x's clientlist with client id 0x%08x because slot index %d already occupied by client id 0x%08x\n",
                pClient->uClientId, uClientId2, pConsole2->iClientIdx, pClient->aClientIds[pConsole2->iClientIdx]);
            return(VOIPCONCIERGE_ERR_INVALID_CONSOLE_INFO);
        }

        if (((pClient = VoipTunnelClientListMatchId(pVoipTunnel, uClientId2)) != NULL) && (pClient->aClientIds[pConsole1->iClientIdx] != 0) && (pClient->iGameIdx == iGameIdx))
        {
            DirtyCastLogPrintf("voipconcierge: cannot update client 0x%08x's clientlist with client id 0x%08x because slot index %d already occupied by client id 0x%08x\n",
                pClient->uClientId, uClientId1, pConsole1->iClientIdx, pClient->aClientIds[pConsole1->iClientIdx]);
            return(VOIPCONCIERGE_ERR_INVALID_CONSOLE_INFO);
        }
        #else
        if ((pClient = VoipTunnelClientListMatchId(pVoipTunnel, uClientId1)) != NULL)
        {
            if ((pGameMapping = VoipTunnelClientMatchGameIdx(pClient, iGameIdx)) != NULL)
            {
                if ((pGameMapping->aClientIds[pConsole2->iClientIdx] != 0) && (VoipTunnelClientMatchGameId(pVoipTunnel, pClient, (uint32_t)pRequest->iConnSet) != NULL))
                {
                    DirtyCastLogPrintf("voipconcierge: cannot update client 0x%08x's clientlist with client id 0x%08x because slot index %d already occupied by client id 0x%08x\n",
                        pClient->uClientId, uClientId2, pConsole2->iClientIdx, pGameMapping->aClientIds[pConsole2->iClientIdx]);
                        return(VOIPCONCIERGE_ERR_INVALID_CONSOLE_INFO);
                }
            }
        }

        if ((pClient = VoipTunnelClientListMatchId(pVoipTunnel, uClientId2)) != NULL)
        {
            if ((pGameMapping = VoipTunnelClientMatchGameIdx(pClient, iGameIdx)) != NULL)
            {
                if ((pGameMapping->aClientIds[pConsole1->iClientIdx] != 0) && (VoipTunnelClientMatchGameId(pVoipTunnel, pClient, (uint32_t)pRequest->iConnSet) != NULL))
                {
                    DirtyCastLogPrintf("voipconcierge: cannot update client 0x%08x's clientlist with client id 0x%08x because slot index %d already occupied by client id 0x%08x\n",
                        pClient->uClientId, uClientId1, pConsole1->iClientIdx, pGameMapping->aClientIds[pConsole1->iClientIdx]);
                        return(VOIPCONCIERGE_ERR_INVALID_CONSOLE_INFO);
                }
            }
        }
        #endif
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeValidateConnSetParameters

    \Description
        Pulls connset from the http parameters

    \Input *pRequest        - http request data
    \Input *pHostedRequest  - [in/out] hosted connection request data

    \Output
        uint8_t             - TRUE if success, FALSE otherwise

    \Version 04/11/2018 (amakoukji)
*/
/********************************************************************************F*/
static uint8_t _VoipConciergeValidateConnSetParameters(const ProtoHttpServRequestT *pRequest, HostedConnectionRequestT *pHostedRequest)
{
    char strConnSet[11];
    const char* pUrl = pRequest->strUrl;
    uint8_t bSuccess = TRUE;

    ds_memclr(strConnSet, sizeof(strConnSet));

    // skip the version and resource in the url
    _VoipConciergeSplitUrl(pUrl, (int32_t)strlen(pUrl), '/', NULL, 0, &pUrl);
    _VoipConciergeSplitUrl(pUrl, (int32_t)strlen(pUrl), '/', NULL, 0, &pUrl);

    if (pUrl[0] != '\0')
    {
        if (_VoipConciergeSplitUrl(pUrl, (int32_t)strlen(pUrl), '/', strConnSet, sizeof(strConnSet), &pUrl) == TRUE)
        {
            pHostedRequest->iConnSet = (strConnSet[0] != '\0') ? (int32_t)strtoul(strConnSet, NULL, 10) : 0;
        }
        else
        {
            DirtyCastLogPrintf("voipconcierge: request validaton failed uri contained invalid resource\n");
            bSuccess = FALSE;
        }
    }

    return(bSuccess);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeProcessConnMessageBody

    \Description
        Validated JSON message in HTTP body for create and update conn set

    \Input *pVoipConcierge  - module state
    \Input *pRequest        - http request data
    \Input *pHostedRequest  - [in/out] hosted connection request data

    \Output
        int32_t             - 0 if success, < 0 otherwise

    \Version 04/11/2018 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeProcessConnMessageBody(VoipConciergeRefT *pVoipConcierge, const ProtoHttpServRequestT *pRequest, HostedConnectionRequestT *pHostedRequest)
{
    RestApiRequestDataT *pRequestData = RestApiGetRequestData(pRequest);
    int32_t iResult = 0;

    _VoipConciergeDecodeJson(pRequestData->pBody, (int32_t)pRequest->iContentLength, pHostedRequest);
    iResult = _VoipConciergeValidateJson(pVoipConcierge, pHostedRequest);

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeValidateDeleteQueryGameClientIds

    \Description
        Pulls a pair of user client ID from the query string

    \Input *pRequest        - http request data
    \Input *pHostedRequest  - [in/out] hosted connection request data

    \Output
        uint8_t             - TRUE if success, FALSE otherwise

    \Version 04/11/2018 (amakoukji)
*/
/********************************************************************************F*/
static uint8_t _VoipConciergeValidateDeleteQueryGameClientIds(const ProtoHttpServRequestT *pRequest, HostedConnectionRequestT *pHostedRequest)
{
    const char* pQuery = pRequest->strQuery;
    uint8_t bSuccess = TRUE;

    if (pHostedRequest->iConnSet <= 0)
    {
        // this check is redundant in practice but is checked here for consistency
        bSuccess = FALSE;
    }
    else if (pHostedRequest->eRequestType != REQUEST_TYPE_DELETE)
    {
        // double check that this is a delete request
        bSuccess = FALSE;
    }
    else if (pQuery[0] != '\0')
    {
        char strName[64], strValue[64];
        DeleteDataT *pDeleteData = &pHostedRequest->RequestData.DeleteData;

        // note: if game_console_id_1 AND game_console_id_2 are not specified the request will be treated as
        //       a DELETE ALL connections in the specified connection set
        while (ProtoHttpGetNextParam(NULL, pQuery, strName, sizeof(strName), strValue, sizeof(strValue), &pQuery) == 0)
        {
            if (ds_stricmp(strName, "game_console_id_1") == 0)
            {
                pDeleteData->uClientId1 = strlen(strValue) > 0 ? (int32_t)strtoul(strValue, NULL, 10) : 0;
            }
            if (ds_stricmp(strName, "game_console_id_2") == 0)
            {
                pDeleteData->uClientId2 = strlen(strValue) > 0 ? (int32_t)strtoul(strValue, NULL, 10) : 0;
            }
        }

        // validate the data from the query parameters
        if ((pDeleteData->uClientId1 == 0) || (pDeleteData->uClientId2 == 0))
        {
            DirtyCastLogPrintf("voipconcierge: invalid client id info on remote connection\n");
            bSuccess = FALSE;
        }
        if (pDeleteData->uClientId1 == pDeleteData->uClientId2)
        {
            DirtyCastLogPrintf("voipconcierge: cannot remove connection from itself\n");
            bSuccess = FALSE;
        }
    }

    return(bSuccess);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeValidateQueryCcsClientId

    \Description
        Pulls a CCS client ID from the query string

    \Input *pRequest        - http request data
    \Input *pHostedRequest  - [in/out] hosted connection request data

    \Output
        uint8_t             - TRUE if success, FALSE otherwise

    \Version 04/11/2018 (amakoukji)
*/
/********************************************************************************F*/
static uint8_t _VoipConciergeValidateQueryCcsClientId(const ProtoHttpServRequestT *pRequest, HostedConnectionRequestT *pHostedRequest)
{
    const char* pQuery = pRequest->strQuery;
    uint8_t bSuccess = TRUE;

    if (pQuery[0] != '\0')
    {
        char strName[64], strValue[64];

        while (ProtoHttpGetNextParam(NULL, pQuery, strName, sizeof(strName), strValue, sizeof(strValue), &pQuery) == 0)
        {
            if (ds_stricmp(strName, "ccs_client_id") == 0)
            {
                ProtoHttpUrlDecodeStrParm(strValue, pHostedRequest->strClientId, sizeof(pHostedRequest->strClientId));
            }
        }

        if (pHostedRequest->strClientId[0] == '\0')
        {
            DirtyCastLogPrintf("voipconcierge: cannot perform a delete all without a filter\n");
            bSuccess = FALSE;
        }
    }

    return(bSuccess);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeHandleDeleteRequest

    \Description
        Calls the function to delete resources based on request

    \Input *pVoipConcierge  - module state
    \Input *pRequest        - hosted connection request data
    \Input *pResponse       - [out] hosted connection response data

    \Output
        int32_t             - zero=success,negative=error

    \Notes
        If there are no connections remaining in the connection set we do not
        do an implicit delete. Instead a delete of the connection set without
        game console filters are required to delete the connection set in
        its entirety.

    \Version 01/08/2016 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeHandleDeleteRequest(VoipConciergeRefT *pVoipConcierge, const HostedConnectionRequestT *pRequest, HostedConnectionResponseT *pResponse)
{
    int32_t iGameIdx, iResult;
    VoipTunnelGameT *pGame;
    const DeleteDataT *pDeleteData = &pRequest->RequestData.DeleteData;
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipConcierge->pBase);

    // update the response data
    pResponse->iRequestId = pRequest->iRequestId;

    if (pRequest->iConnSet > 0)
    {
        // if a connection set was specified try to match a game to it
        if ((pGame = VoipTunnelGameListMatchId(pVoipTunnel, (uint32_t)pRequest->iConnSet, &iGameIdx)) == NULL)
        {
            #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
            DirtyCastLogPrintf("voipconcierge: connection set 0x%08x is inactive (req=%d)\n", pRequest->iConnSet, pRequest->iRequestId);
            #else
            DirtyCastLogPrintf("voipconcierge: connection set 0x%08x does not exist (req=%d [delete])\n", pRequest->iConnSet, pRequest->iRequestId);
            #endif
            return(VOIPCONCIERGE_ERR_CONNSET_INACTIVE);
        }

        if ((pDeleteData->uClientId1 == 0) && (pDeleteData->uClientId2 == 0))
        {
            int32_t iGameClient;
            uint32_t aClientList[VOIPTUNNEL_MAXGROUPSIZE];
            ds_memcpy(aClientList, pGame->aClientList, sizeof(aClientList));

            // if this is a DELETE request for the entire connection set, just delete from the voip tunnel
            VoipTunnelGameListDel(pVoipTunnel, iGameIdx);

            // delete now inactive client ids
            for (iGameClient = 0; iGameClient < VOIPTUNNEL_MAXGROUPSIZE; iGameClient += 1)
            {
                uint32_t uClientId;
                if ((uClientId = aClientList[iGameClient]) == 0)
                {
                    continue;
                }
                if (VoipTunnelClientListMatchId(pVoipTunnel, uClientId) != NULL)
                {
                    continue;
                }

                // remove client from the map
                _VoipConciergeRemoveClient(pVoipConcierge, uClientId);
                pResponse->iNumResources += 1;
            }

            DirtyCastLogPrintf("voipconcierge: deleted connection set (id=0x%08x/index=%04d) (req=%d)\n", pRequest->iConnSet, iGameIdx, pRequest->iRequestId);
        }
        else
        {
            // remove clients from the voiptunnel
            #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
            if ((iResult = _VoipConciergeRemoveConnection(pVoipConcierge, iGameIdx, pRequest->iRequestId, pDeleteData)) < 0)
            #else
            if ((iResult = _VoipConciergeRemoveConnection(pVoipConcierge, pRequest->iConnSet, iGameIdx, pRequest->iRequestId, pDeleteData)) < 0)
            #endif
            {
                DirtyCastLogPrintf("voipconcierge: could not remove connections to set (id=0x%08x/index=%04d) (req=%d error=%d)\n", pRequest->iConnSet, iGameIdx, pRequest->iRequestId, iResult);
                return(iResult);
            }
            pResponse->iNumResources = iResult;

            // print out how many clients are remaining in the game
            DirtyCastLogPrintf("voipconcierge: connection set (id=0x%08x/index=%04d) now has %d clients registered (req=%d)\n", pGame->uGameId, iGameIdx, pGame->iNumClients, pRequest->iRequestId);
        }
    }
    else if (pRequest->strClientId[0] != '\0')
    {
        int32_t iClientIdx;

        // loop through every client and find the clients that match and delete them
        for (iClientIdx = 0; iClientIdx < pVoipConcierge->iClientMapSize; iClientIdx += 1)
        {
            VoipTunnelClientT *pClient;
            ConciergeClientMapEntryT *pEntry = &pVoipConcierge->pClientMap[iClientIdx];

            if (strncmp(pEntry->strClientId, pRequest->strClientId, sizeof(pEntry->strClientId)) != 0)
            {
                continue;
            }

            // incase the client is in multiple connection sets we need to keep deleting them
            while ((pClient = VoipTunnelClientListMatchId(pVoipTunnel, pEntry->uClientId)) != NULL)
            {
                #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
                VoipTunnelGameListDel(pVoipTunnel, pClient->iGameIdx);
                #else
                VoipTunnelGameListMatchId(pVoipTunnel, pRequest->iConnSet, &iGameIdx);

                VoipTunnelGameListDel(pVoipTunnel, iGameIdx);
                #endif
            }

            DirtyCastLogPrintf("voipconcierge: removed client 0x%08x from map at index %d\n", pEntry->uClientId, iClientIdx);
            ds_memclr(pEntry, sizeof(*pEntry));
            pResponse->iNumResources += 1;
        }
        DirtyCastLogPrintf("voipconcierge: found %d clients to delete by ccs_client_id: %s filter\n", pResponse->iNumResources, pRequest->strClientId);
    }
    else
    {
        // no connection set was specified
        DirtyCastLogPrintf("voipconcierge: connection set or client id not specified was not specified for delete (req=%d)\n", pRequest->iRequestId);
        return (VOIPCONCIERGE_ERR_INVALIDPARAM);
    }

    // update the capacity data with new changes
    pResponse->uCapacity = ((VoipServerStatus(pVoipConcierge->pBase, 'psta', 0, NULL, 0) != VOIPSERVER_PROCESS_STATE_RUNNING) || pVoipConcierge->bMaxLoad) ? 0 :
        (VoipTunnelStatus(pVoipTunnel, 'musr', 0, NULL, 0) - VoipTunnelStatus(pVoipTunnel, 'nusr', -1, NULL, 0));

    // success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeInitializeRequest

    \Description
        Allocates a request object, initializes it and kicks off token validation

    \Input *pVoipConcierge         - module state
    \Input *pRequest               - http request data
    \Input  eRequestType           - indicates if the type of request (POST, DELETE)
    \Input *pResponse              - [out] http response filled out if any failure

    \Output
        HostedConnectionRequestT * - pointer to saved hosted connection request object
                                     otherwise NULL if any failure

    \Version 10/19/2015 (eesponda)
*/
/********************************************************************************F*/
static HostedConnectionRequestT *_VoipConciergeInitializeRequest(VoipConciergeRefT *pVoipConcierge, const ProtoHttpServRequestT *pRequest, RequestTypeE eRequestType, ProtoHttpServResponseT *pResponse)
{
    int32_t iResult;
    HostedConnectionRequestT *pHostedRequest;
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipConcierge->pBase);
    RestApiRequestDataT *pRequestData = RestApiGetRequestData(pRequest);

    if (pRequestData->pResource != NULL)
    {
        pHostedRequest = pRequestData->pResource;

        // if we are in the waiting state, attempt another validate. don't check authvalidate since we assume it if we are in waiting
        if ((pHostedRequest->eState == REQUEST_STATE_WAITING) && ((iResult = _VoipConciergeProcessToken(pVoipConcierge, pRequestData, pHostedRequest)) < 0))
        {
            DirtyCastLogPrintf("voipconcierge: validation of token failed (req=%d) while req is in 'waiting' state\n", pHostedRequest->iRequestId);
            _VoipConciergeEncodeError(_VoipConciergeGetErrorResponse(iResult), pHostedRequest->iRequestId, pResponse);

            DirtyMemFree(pHostedRequest, VOIPCONCIERGE_MEMID, pVoipConcierge->iMemGroup, pVoipConcierge->pMemGroupUserdata);
            pRequestData->pResource = NULL;
            return(NULL);
        }
        return(pHostedRequest);
    }

    // allocate the request data
    if ((pHostedRequest = (HostedConnectionRequestT *)DirtyMemAlloc(sizeof(*pHostedRequest), VOIPCONCIERGE_MEMID, pVoipConcierge->iMemGroup, pVoipConcierge->pMemGroupUserdata)) == NULL)
    {
        DirtyCastLogPrintf("voipconcierge: failed to allocate hosted connection request for handling\n");
        return(NULL);
    }
    ds_memclr(pHostedRequest, sizeof(*pHostedRequest));
    pHostedRequest->eRequestType = eRequestType;
    // save the request for later
    pRequestData->pResource = pHostedRequest;

    if (pConfig->bAuthValidate == TRUE)
    {
        // validate the token either by getting the cached token data or starting a request for a new token
        if ((iResult = _VoipConciergeProcessToken(pVoipConcierge, pRequestData, pHostedRequest)) < 0)
        {
            DirtyCastLogPrintf("voipconcierge: validation of token failed (req=%d)\n", pHostedRequest->iRequestId);
            _VoipConciergeEncodeError(_VoipConciergeGetErrorResponse(iResult), pHostedRequest->iRequestId, pResponse);

            DirtyMemFree(pHostedRequest, VOIPCONCIERGE_MEMID, pVoipConcierge->iMemGroup, pVoipConcierge->pMemGroupUserdata);
            return(NULL);
        }
    }
    else
    {
        pHostedRequest->eState = REQUEST_STATE_AUTHORIZED;
    }

    return(pHostedRequest);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeValidateRequest

    \Description
        Validates the request after have confirmed the request is trusted

    \Input *pVoipConcierge          - module state
    \Input *pRequest                - http request data
    \Input *pRequestParamHandler    - pointer to handler for request parameters
    \Input *pRequestQueryHandler    - pointer to handler for query parameters
    \Input *pResponseBodyHandler    - pointer to handler for request body

    \Output
        int32_t                     - result of validation (zero=success, negative=failure)

    \Version 07/25/2018 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeValidateRequest(VoipConciergeRefT *pVoipConcierge, const ProtoHttpServRequestT *pRequest, RequestParamHandlerCbT *pRequestParamHandler, RequestQueryHandlerCbT *pRequestQueryHandler, RequestBodyHandlerCbT *pRequestBodyHandler)
{
    int32_t iResult;
    RestApiRequestDataT *pRequestData = RestApiGetRequestData(pRequest);
    HostedConnectionRequestT *pHostedRequest = (HostedConnectionRequestT *)pRequestData->pResource;

    // validate parameters
    if ((pRequestParamHandler != NULL) && (pRequestParamHandler(pRequest, pHostedRequest) == FALSE))
    {
        DirtyCastLogPrintf("voipconcierge: validation of parameters failed (req=%d)\n", pHostedRequest->iRequestId);
        return(VOIPCONCIERGE_ERR_INVALIDPARAM);
    }

    // validate query string
    if ((pRequestQueryHandler != NULL) && (pRequestQueryHandler(pRequest, pHostedRequest) == FALSE))
    {
        DirtyCastLogPrintf("voipconcierge: validation of query string failed (req=%d)\n", pHostedRequest->iRequestId);
        return(VOIPCONCIERGE_ERR_INVALIDPARAM);
    }

    // decode json into our request structure
    if ((pRequestBodyHandler != NULL) && ((iResult = pRequestBodyHandler(pVoipConcierge, pRequest, pHostedRequest)) < 0))
    {
        DirtyCastLogPrintf("voipconcierge: validation of request body failed (req=%d)\n", pHostedRequest->iRequestId);
        return(iResult);
    }

    // print the validated resources
    if (pHostedRequest->eRequestType == REQUEST_TYPE_POST)
    {
        const PostDataT *pPostData = &pHostedRequest->RequestData.PostData;
        if (pPostData->iNumConnections > 0)
        {
            DirtyCastLogPrintf("voipconcierge: add hosted connection request validated [ request_id = %d, product = %s, ccs_client_id = %s, leasetime = %um, num_conns = %d, conn_set = %d ]\n",
                pHostedRequest->iRequestId, pPostData->strProduct, pHostedRequest->strClientId, pPostData->uLeaseTime, pPostData->iNumConnections, pHostedRequest->iConnSet);
        }
        else
        {
            DirtyCastLogPrintf("voipconcierge: lease time extension request validated [ request_id = %d, product = %s, ccs_client_id = %s, leasetime = %um, conn_set = %d ]\n",
                pHostedRequest->iRequestId, pPostData->strProduct, pHostedRequest->strClientId, pPostData->uLeaseTime, pHostedRequest->iConnSet);
        }
    }
    else if (pHostedRequest->eRequestType == REQUEST_TYPE_DELETE)
    {
        const DeleteDataT *pDeleteData = &pHostedRequest->RequestData.DeleteData;
        if (pHostedRequest->iConnSet > 0)
        {
            if ((pDeleteData->uClientId1 == 0) && (pDeleteData->uClientId2 == 0))
            {
                DirtyCastLogPrintf("voipconcierge: remove connection set request validated [ request_id = %d, ccs_client_id = %s, conn_set = %d ]\n",
                    pHostedRequest->iRequestId, pHostedRequest->strClientId, pHostedRequest->iConnSet);
            }
            else
            {
                DirtyCastLogPrintf("voipconcierge: remove hosted connection request validated [ request_id = %d, ccs_client_id = %s, game_console_id_1 = 0x%08x, game_console_id_2 = 0x%08x, conn_set = %d ]\n",
                    pHostedRequest->iRequestId, pHostedRequest->strClientId, pDeleteData->uClientId1, pDeleteData->uClientId2, pHostedRequest->iConnSet);
            }
        }
        else
        {
            DirtyCastLogPrintf("voipconcierge: remove hosted connections by filter request validated [ request_id = %d, ccs_client_id = %s ]\n",
                pHostedRequest->iRequestId, pHostedRequest->strClientId);
        }
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeApiConnectionCb

    \Description
        General callback for handling REST request for connections

    \Input *pRequest              - request data
    \Input *pResponse             - response data
    \Input *pUserData             - pointer to module state
    \Input  eRequestType          - http request type
    \Input *pRequestParamHandler  - handler for the http parameters
    \Input *pRequestQueryHandler  - handler for the http query string
    \Input *pRequestBodyHandler   - handler for the http message body
    \Input *pRequestActionHandler - handler to process the action
    \Input *pPostEncoderHandler   - handler for the return message encoder

    \Output
        int32_t                   - zero=success,positive=in-progress,negative=error

    \Version 04/11/2018 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeApiConnectionCb(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData,
    RequestTypeE             eRequestType,
    RequestParamHandlerCbT  *pRequestParamHandler,
    RequestQueryHandlerCbT  *pRequestQueryHandler,
    RequestBodyHandlerCbT   *pRequestBodyHandler,
    RequestActionHandlerCbT *pRequestActionHandler,
    PostEncoderCbT          *pPostEncoderHandler)
{
    int32_t iResult, iRequestId = 0;
    HostedConnectionRequestT *pHostedRequest;
    HostedConnectionResponseT HostedResponse;
    VoipConciergeRefT *pVoipConcierge = (VoipConciergeRefT *)pUserData;
    RestApiRequestDataT *pRequestData = RestApiGetRequestData(pRequest);

    // check that the required handlers have a value
    if (pPostEncoderHandler == NULL)
    {
        return(-1);
    }

    // if we don't have the request object allocated this is a new request so do
    // the standard initialization
    if ((pHostedRequest = _VoipConciergeInitializeRequest(pVoipConcierge, pRequest, eRequestType, pResponse)) == NULL)
    {
        return(-1);
    }
    iRequestId = pHostedRequest->iRequestId;
    ds_memclr(&HostedResponse, sizeof(HostedResponse));

    // if still validating or waiting allow for more processing time
    if ((pHostedRequest->eState == REQUEST_STATE_VALIDATING) || (pHostedRequest->eState == REQUEST_STATE_WAITING))
    {
        return(1);
    }
    // make sure the request did not fail
    else if (pHostedRequest->eState == REQUEST_STATE_FAILURE)
    {
        iResult = VOIPCONCIERGE_ERR_SYSTEM;
    }
    // make sure we are authorized
    else if (pHostedRequest->eState == REQUEST_STATE_UNAUTHORIZED)
    {
        iResult = VOIPCONCIERGE_ERR_FORBIDDEN;
    }
    else if ((iResult = _VoipConciergeValidateRequest(pVoipConcierge, pRequest, pRequestParamHandler, pRequestQueryHandler, pRequestBodyHandler)) == 0)
    {
        // try to handle the request and encode the failure if needed
        iResult = pRequestActionHandler(pVoipConcierge, pHostedRequest, &HostedResponse);
    }

    // before doing processing clean up the request data
    DirtyMemFree(pRequestData->pResource, VOIPCONCIERGE_MEMID, pVoipConcierge->iMemGroup, pVoipConcierge->pMemGroupUserdata);
    pRequestData->pResource = pHostedRequest = NULL;

    // now handle error if needed
    if (iResult < 0)
    {
        _VoipConciergeEncodeError(_VoipConciergeGetErrorResponse(iResult), iRequestId, pResponse);
        return(-1);
    }

    // encode success and return
    pPostEncoderHandler(&HostedResponse, pResponse);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeApiCreateConnCb

    \Description
        Callback for handling create REST request

    \Input *pRequest        - request data
    \Input *pResponse       - response data
    \Input *pUserData       - pointer to module state

    \Output
        int32_t             - zero=success,positive=in-progress,negative=error

    \Version 04/11/2018 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeApiCreateConnCb(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData)
{
    return(_VoipConciergeApiConnectionCb(pRequest, pResponse, pUserData,
                                         REQUEST_TYPE_POST,
                                         NULL,
                                         NULL,
                                         &_VoipConciergeProcessConnMessageBody,
                                         &_VoipConciergeCreateConnectionSet,
                                         &_VoipConciergeEncodePostSuccess));
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeApiUpdateConnCb

    \Description
        Callback for handling update REST request

    \Input *pRequest        - request data
    \Input *pResponse       - response data
    \Input *pUserData       - pointer to module state

    \Output
        int32_t             - zero=success,positive=in-progress,negative=error

    \Version 04/11/2018 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeApiUpdateConnCb(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData)
{
    return(_VoipConciergeApiConnectionCb(pRequest, pResponse, pUserData,
                                         REQUEST_TYPE_POST,
                                         &_VoipConciergeValidateConnSetParameters,
                                         NULL,
                                         &_VoipConciergeProcessConnMessageBody,
                                         &_VoipConciergeUpdateConnectionSet,
                                         &_VoipConciergeEncodePostSuccess));
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeApiDeleteConnCb

    \Description
        Callback for handling delete REST request

    \Input *pRequest        - request data
    \Input *pResponse       - response data
    \Input *pUserData       - pointer to module state

    \Output
        int32_t             - zero=success,positive=in-progress,negative=error

    \Version 04/11/2018 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeApiDeleteConnCb(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData)
{
    return(_VoipConciergeApiConnectionCb(pRequest, pResponse, pUserData,
                                         REQUEST_TYPE_DELETE,
                                         &_VoipConciergeValidateConnSetParameters,
                                         &_VoipConciergeValidateDeleteQueryGameClientIds,
                                         NULL,
                                         &_VoipConciergeHandleDeleteRequest,
                                         &_VoipConciergeEncodeDeleteSuccess));
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeApiDeleteAllConnsCb

    \Description
        Callback for handling delete all REST request

    \Input *pRequest        - request data
    \Input *pResponse       - response data
    \Input *pUserData       - pointer to module state

    \Output
        int32_t             - zero=success,positive=in-progress,negative=error

    \Version 04/11/2018 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeApiDeleteAllConnsCb(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData)
{
    return(_VoipConciergeApiConnectionCb(pRequest, pResponse, pUserData,
                                         REQUEST_TYPE_DELETE,
                                         NULL,
                                         &_VoipConciergeValidateQueryCcsClientId,
                                         NULL,
                                         &_VoipConciergeHandleDeleteRequest,
                                         &_VoipConciergeEncodeDeleteSuccess));
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergePollHostingId

    \Description
        Poll for the hosting server id provided by CCS

    \Input *pVoipConcierge  - module state

    \Version 01/12/2016 (amakoukji)
*/
/********************************************************************************F*/
static void _VoipConciergePollHostingId(VoipConciergeRefT *pVoipConcierge)
{
    uint32_t uOldHostingServerId = pVoipConcierge->uHostingServerId;
    uint32_t uNewHostingServerId = (uint32_t)ConciergeServiceStatus(pVoipConcierge->pConciergeService, 'clid', 0, NULL, 0);
    if (uOldHostingServerId != uNewHostingServerId)
    {
        pVoipConcierge->uHostingServerId = uNewHostingServerId;
        ProtoTunnelControl(VoipServerGetProtoTunnel(pVoipConcierge->pBase), 'clid', uNewHostingServerId, 0, NULL);
        DirtyCastLogPrintf("voipconcierge: setting hosting server id to 0x%08x, was 0x%08x\n", uNewHostingServerId, uOldHostingServerId);
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeCheckExpired

    \Description
        Check for expired connections, delete from the voiptunnel if found

    \Input *pVoipConcierge  - module state
    \Input uCurTick         - current time to check timeout

    \Version 10/06/2015 (eesponda)
*/
/********************************************************************************F*/
static void _VoipConciergeCheckExpired(VoipConciergeRefT *pVoipConcierge, uint32_t uCurTick)
{
    int32_t iGameIdx;
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipConcierge->pBase);
    const int32_t iMaxGames = VoipTunnelStatus(pVoipTunnel, 'mgam', 0, NULL, 0);

    for (iGameIdx = 0; iGameIdx < iMaxGames; iGameIdx += 1)
    {
        VoipTunnelGameT *pGame;
        uint32_t aClientList[VOIPTUNNEL_MAXGROUPSIZE];
        int32_t iClientIdx;
        int32_t iClientsDeleted = 0;

        // check to see if the connection set is active
        if (VoipTunnelStatus(pVoipTunnel, 'nusr', iGameIdx, NULL, 0) < 0)
        {
            continue;
        }

        // check to see if any connection set's lease is up
        if (NetTickDiff(uCurTick, pVoipConcierge->pConnectionSets[iGameIdx].uExpireTime) <= 0)
        {
            continue;
        }

        // ref game to get the clientids
        if ((pGame = VoipTunnelGameListMatchIndex(pVoipTunnel, iGameIdx)) == NULL)
        {
            continue;
        }
        ds_memcpy(aClientList, pGame->aClientList, sizeof(aClientList));

        // remove the connection set that has its lease expired
        DirtyCastLogPrintf("voipconcierge: removing connection set (id=0x%08x/index=%04d) as connection set lease expired\n", pGame->uGameId, iGameIdx);
        VoipTunnelGameListDel(pVoipTunnel, iGameIdx);

        // loop through all the clients that were in the game and remove any from the
        // cache that no longer exist in the voiptunnel
        for (iClientIdx = 0; iClientIdx < VOIPTUNNEL_MAXGROUPSIZE; iClientIdx += 1)
        {
            uint32_t uClientId;
            if ((uClientId = aClientList[iClientIdx]) == 0)
            {
                continue;
            }
            if (VoipTunnelClientListMatchId(pVoipTunnel, uClientId) != NULL)
            {
                continue;
            }

            _VoipConciergeRemoveClient(pVoipConcierge, uClientId);
            iClientsDeleted += 1;
        }

        // notify the CCS if any clients were removed (happens on next update)
        if (iClientsDeleted > 0)
        {
            ConciergeServiceControl(pVoipConcierge->pConciergeService, 'hrbt', 0, 0, NULL);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeGameEventCb

    \Description
        Callback when we receive game data (on game port vs voip port)

    \Input *pEventData  - information about the recv'd data
    \Input *pUserData   - pointer to module ref

    \Output
        int32_t         - zero (packet not consumed)

    \Version 10/05/2015 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeGameEventCb(const GameEventDataT *pEventData, void *pUserData)
{
    VoipConciergeRefT *pVoipConcierge = (VoipConciergeRefT *)pUserData;
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipConcierge->pBase);

    if (pEventData->eEvent == VOIPSERVER_EVENT_RECVGAME)
    {
        uint32_t uLatency;
        if (VoipServerExtractNetGameLinkLate((const CommUDPPacketHeadT *)pEventData->pData, (uint32_t)pEventData->iDataSize, &uLatency) > 0)
        {
            GameServerStatInfoT StatInfo;
            ds_memclr(&StatInfo, sizeof(StatInfo));

            // accumulate latency information
            GameServerPacketUpdateLateStatsSingle(&StatInfo.E2eLate, uLatency);
            // accumulate late bin information
            GameServerPacketUpdateLateBin(&pConfig->LateBinCfg, &StatInfo.LateBin, uLatency);

            // send information to the base module for tracking
            VoipServerControl(pVoipConcierge->pBase, 'e2el', 0, 0, &StatInfo.E2eLate);
            VoipServerControl(pVoipConcierge->pBase, 'lbin', 0, 0, &StatInfo.LateBin);
        }
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeRestApiCompleteCb

    \Description
        Complete callback registered with RestApi.

    \Input *pRequest    - request data (can be NULL)
    \Input *pResponse   - response data (can be NULL)
    \Input *pUserData   - pointer to module state

    \Version 03/08/2021 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipConciergeRestApiCompleteCb(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData)
{
    VoipConciergeRefT *pVoipConcierge = (VoipConciergeRefT *)pUserData;

    if (pRequest != NULL)
    {
        RestApiRequestDataT *pRequestData = RestApiGetRequestData(pRequest);

        if (pRequestData != NULL)
        {
            // abandon token validation if in progress
            HashStrDel(pVoipConcierge->pValidatingRequests, pRequestData->strAccessToken);

            if (pRequestData->pResource != NULL)
            {
                DirtyMemFree(pRequestData->pResource, VOIPCONCIERGE_MEMID, pVoipConcierge->iMemGroup, pVoipConcierge->pMemGroupUserdata);
                pRequestData->pResource = NULL;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeUpdateMonitor

    \Description
        Update the warnings/errors based on the configured thresholds

    \Input *pVoipConcierge  - module state
    \Input  uCurTick        - current tick for this frame

    \Version 12/09/2015 (eesponda)
*/
/********************************************************************************F*/
static void _VoipConciergeUpdateMonitor(VoipConciergeRefT *pVoipConcierge, uint32_t uCurTick)
{
    uint32_t uValue, uMonitorFlagWarnings, uMonitorFlagErrors;
    uint8_t bClearOnly;
    VoipServerStatT VoipServerStats;
    UdpMetricsT UdpMetrics;

    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipConcierge->pBase);
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipConcierge->pBase);
    const ServerMonitorConfigT *pWrn = &pConfig->MonitorWarnings;
    const ServerMonitorConfigT *pErr = &pConfig->MonitorErrors;
    const int32_t iMaxGames = VoipTunnelStatus(pVoipTunnel, 'mgam', 0, NULL, 0);
    const int32_t iMaxClients = VoipTunnelStatus(pVoipTunnel, 'musr', 0, NULL, 0);
    const uint32_t uPrevFlagWarnings = (uint32_t)VoipServerStatus(pVoipConcierge->pBase, 'mwrn', 0, NULL, 0);
    const uint32_t uPrevFlagErrors = (uint32_t)VoipServerStatus(pVoipConcierge->pBase, 'merr', 0, NULL, 0);

    // get voipserver stats
    VoipServerStatus(pVoipConcierge->pBase, 'stat', 0, &VoipServerStats, sizeof(VoipServerStats));
    VoipServerStatus(pVoipConcierge->pBase, 'udpm', 0, &UdpMetrics, sizeof(UdpMetrics));

    // check game slot percentage available
    uValue = ((iMaxGames - VoipTunnelStatus(pVoipTunnel, 'ngam', 0, NULL, 0)) * 100) / iMaxGames;
    VoipServerUpdateMonitorFlag(pVoipConcierge->pBase, uValue < pWrn->uPctGamesAvail, uValue < pErr->uPctGamesAvail, VOIPSERVER_MONITORFLAG_PCTGAMESAVAIL, FALSE);

    // check client slot percentage available
    uValue = ((iMaxClients - VoipTunnelStatus(pVoipTunnel, 'nusr', -1, NULL, 0)) * 100) / iMaxClients;
    VoipServerUpdateMonitorFlag(pVoipConcierge->pBase, uValue < pWrn->uPctClientSlot, uValue < pErr->uPctClientSlot, VOIPSERVER_MONITORFLAG_PCTCLIENTSLOT, FALSE);

    // check average latency, if system load is 1/2 of warning threshold or greater
    bClearOnly = (VOIPSERVER_LoadInt(VoipServerStats.ServerInfo.aLoadAvg[1]) < (pWrn->uAvgSystemLoad/2)) ? TRUE : FALSE;
    uValue = (VoipServerStats.StatInfo.E2eLate.uNumStat != 0) ? VoipServerStats.StatInfo.E2eLate.uSumLate / VoipServerStats.StatInfo.E2eLate.uNumStat : 0;
    VoipServerUpdateMonitorFlag(pVoipConcierge->pBase, uValue > pWrn->uAvgClientLate, uValue > pErr->uAvgClientLate, VOIPSERVER_MONITORFLAG_AVGCLIENTLATE, bClearOnly);

    // check pct server load
    uValue = (uint32_t)VoipServerStats.ServerInfo.fCpu;
    VoipServerUpdateMonitorFlag(pVoipConcierge->pBase, uValue > pWrn->uPctServerLoad, uValue > pErr->uPctServerLoad, VOIPSERVER_MONITORFLAG_PCTSERVERLOAD, FALSE);

    // check pct system load
    uValue = VOIPSERVER_LoadInt(VoipServerStats.ServerInfo.aLoadAvg[1]);
    VoipServerUpdateMonitorFlag(pVoipConcierge->pBase, uValue > pWrn->uAvgSystemLoad, uValue > pErr->uAvgSystemLoad, VOIPSERVER_MONITORFLAG_AVGSYSTEMLOAD, FALSE);

    // check the UDP stats
    VoipServerUpdateMonitorFlag(pVoipConcierge->pBase, UdpMetrics.uRecvQueueLen > pWrn->uUdpRecvQueueLen, UdpMetrics.uRecvQueueLen > pErr->uUdpRecvQueueLen, VOIPSERVER_MONITORFLAG_UDPRECVQULEN, FALSE);
    VoipServerUpdateMonitorFlag(pVoipConcierge->pBase, UdpMetrics.uSendQueueLen > pWrn->uUdpSendQueueLen, UdpMetrics.uSendQueueLen > pErr->uUdpSendQueueLen, VOIPSERVER_MONITORFLAG_UDPSENDQULEN, FALSE);

    // $todo$ check to see if clients are having trouble connecting to the server

    // debug output if monitor state was changed
    uMonitorFlagWarnings = (uint32_t)VoipServerStatus(pVoipConcierge->pBase, 'mwrn', 0, NULL, 0);
    uMonitorFlagErrors = (uint32_t)VoipServerStatus(pVoipConcierge->pBase, 'merr', 0, NULL, 0);

    if (uMonitorFlagWarnings != uPrevFlagWarnings)
    {
        DirtyCastLogPrintf("voipserverdiagnostic: monitor warning state change: 0x%08x->0x%08x\n", uPrevFlagWarnings, uMonitorFlagWarnings);
    }
    if (uMonitorFlagErrors != uPrevFlagErrors)
    {
        DirtyCastLogPrintf("voipserverdiagnostic: monitor error state change: 0x%08x->0x%08x\n", uPrevFlagErrors, uMonitorFlagErrors);
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeMaxLoadUpdate

    \Description
        Max load detection logic.

    \Input *pVoipConcierge  - mode state

    \Notes
        The 'max load' is the point at which we want to tell the CCS that we
        are operating under heavy usage already and we save what is left of
        our capacity to service 'subsequent' requests only, i.e. connection
        requests for already existing connection sets. We communicate that info
        to the CCS by announcing a capacity of 0. 

    \Version 11/07/2018 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipConciergeMaxLoadUpdate(VoipConciergeRefT *pVoipConcierge)
{
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipConcierge->pBase);
    VoipServerRefT *pVoipServer = VoipConciergeGetBase(pVoipConcierge);
    uint32_t uCurrTick = NetTick();
    uint32_t uMonitorFlagWarnings;

    // get warning monitoring info from voipserver
    uMonitorFlagWarnings = VoipServerStatus(pVoipServer, 'mwrn', 0, NULL, 0);

#if DIRTYCODE_DEBUG
    if (pVoipConcierge->uFakePctServerLoad == 'warn' || pVoipConcierge->uFakePctServerLoad == 'erro')
    {
        uMonitorFlagWarnings |= VOIPSERVER_MONITORFLAG_PCTSERVERLOAD;
    }
#endif

    if (pVoipConcierge->bMaxLoad)
    {
        if ((uMonitorFlagWarnings & VOIPSERVER_MONITORFLAG_PCTSERVERLOAD) == 0)
        {
            if (pVoipConcierge->bMaxLoadDetectionLogged == FALSE)
            {
                DirtyCastLogPrintf("voipconcierge: 'under max load' detected (cpu <= %d)\n", pConfig->MonitorWarnings.uPctServerLoad);
                pVoipConcierge->bMaxLoadDetectionLogged = TRUE;
            }

            // must stay long enough under max load threshold to trigger a sub-state transition back to 'active' sub-state
            if (NetTickDiff(uCurrTick, pVoipConcierge->uMaxLoadThresholdDetectionStart) > pConfig->iCpuThresholdDetectionDelay)
            {
                DirtyCastLogPrintf("voipconcierge: 'under max load' sustained long enough (cpu <= %d for at least %d s)\n",
                    pConfig->MonitorWarnings.uPctServerLoad, pConfig->iCpuThresholdDetectionDelay / 1000);

                pVoipConcierge->bMaxLoad = FALSE;
                pVoipConcierge->bMaxLoadDetectionLogged = FALSE;
                pVoipConcierge->uMaxLoadThresholdDetectionStart = uCurrTick;
            }
        }
        else
        {
            pVoipConcierge->bMaxLoadDetectionLogged = FALSE;
            pVoipConcierge->uMaxLoadThresholdDetectionStart = uCurrTick;
        }
    }
    else
    {
        if (uMonitorFlagWarnings & VOIPSERVER_MONITORFLAG_PCTSERVERLOAD)
        {
            if (pVoipConcierge->bMaxLoadDetectionLogged == FALSE)
            {
                DirtyCastLogPrintf("voipconcierge: 'over max load' detected (cpu > %d)\n", pConfig->MonitorWarnings.uPctServerLoad);
                pVoipConcierge->bMaxLoadDetectionLogged = TRUE;
            }

            // must stay long enough over max load threshold to trigger a sub-state transition to 'full' sub-state
            if (NetTickDiff(uCurrTick, pVoipConcierge->uMaxLoadThresholdDetectionStart) > pConfig->iCpuThresholdDetectionDelay)
            {
                DirtyCastLogPrintf("voipconcierge: 'over max load' sustained long enough (cpu > %d for at least %d s)\n",
                    pConfig->MonitorWarnings.uPctServerLoad, pConfig->iCpuThresholdDetectionDelay / 1000);
                pVoipConcierge->bMaxLoad = TRUE;
                pVoipConcierge->bMaxLoadDetectionLogged = FALSE;
                pVoipConcierge->uMaxLoadThresholdDetectionStart = uCurrTick;
            }
        }
        else
        {
            pVoipConcierge->bMaxLoadDetectionLogged = FALSE;
            pVoipConcierge->uMaxLoadThresholdDetectionStart = uCurrTick;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeUpdateCCS

    \Description
        Give life to module in charge of interfacing the CCS

    \Input *pVoipConcierge  - mode state
    \Input uCapacity        - capacity to be announced to CCS

    \Version 20/09/2019 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipConciergeUpdateCCS(VoipConciergeRefT *pVoipConcierge, uint32_t uCapacity)
{
    // call ConciergeServiceRegister() if it was never called before
    if (ConciergeServiceStatus(pVoipConcierge->pConciergeService, 'regi', 0, NULL, 0) == FALSE)
    {
        const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipConcierge->pBase);
        VoipServerRefT *pVoipServer = VoipConciergeGetBase(pVoipConcierge);
        uint32_t uCurrTick = NetTick();

        // only re-attempt hostname query every 10 sec
        if ((pVoipConcierge->uLastHostNameQueryAttempt == 0) ||
            NetTickDiff(uCurrTick, pVoipConcierge->uLastHostNameQueryAttempt) > 10000)
        {
            char strHostname[256];

            pVoipConcierge->uLastHostNameQueryAttempt = uCurrTick;

            // if uCurrTick is 0, then alter it because 0 means 'never attempted do it now'
            if (pVoipConcierge->uLastHostNameQueryAttempt == 0)
            {
                pVoipConcierge->uLastHostNameQueryAttempt = 1;
            }

            VoipServerStatus(pVoipServer, 'host', 0, strHostname, sizeof(strHostname));

            if (strHostname[0] != '\0')
            {
                char strHost[256], strVersions[64];

                // set the versions
                ProtoTunnelStatus(VoipServerGetProtoTunnel(pVoipServer), 'vset', 0, strVersions, sizeof(strVersions));

                // set the host to tell the CCS where to contact us
                ds_snzprintf(strHost, sizeof(strHost), "%s://%s:%d", (pConfig->bOffloadInboundSSL ? "https" : "http"), strHostname, pConfig->uApiPort);

                // register ourselves with the CCS
                ConciergeServiceRegister(pVoipConcierge->pConciergeService, pConfig->strPingSite, pConfig->strPool, strHost, strVersions);
            }
            else
            {
                DirtyCastLogPrintf("voipconcierge: delaying call to ConciergeServiceRegister because hostname query not yet successful\n");
            }
        }
    }

    ConciergeServiceUpdate(pVoipConcierge->pConciergeService, uCapacity);
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipConciergeInitialize

    \Description
        Allocates the state for the mode and does any additional setup or
        configuration needed

    \Input *pVoipServer     - pointer to the base module (shared data / functionality)
    \Input *pCommandTags    - configuration data (from command line)
    \Input *pConfigTags     - configuration data (from configuration file)

    \Output
        VoipConciergeRefT * - pointer to the mode state

    \Version 09/24/2015 (eesponda)
*/
/********************************************************************************F*/
VoipConciergeRefT *VoipConciergeInitialize(VoipServerRefT *pVoipServer, const char *pCommandTags, const char *pConfigTags)
{
    char strVersions[64];
    VoipConciergeRefT *pVoipConcierge;
    int32_t iMemGroup, iIndex;
    void *pMemGroupUserdata;
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);
    ProtoTunnelRefT *pProtoTunnel = VoipServerGetProtoTunnel(pVoipServer);
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipServer);
    TokenApiRefT *pTokenApi = VoipServerGetTokenApi(pVoipServer);
    const int32_t iNumResources = sizeof(_VoipConcierge_aResources) / sizeof(RestApiResourceDataT);
    uint32_t uRestApiFlags = 0;

    // get allocation information
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserdata);

    // ensure that we have a valid pingsite for registration
    if (pConfig->strPingSite[0] == '\0')
    {
        DirtyCastLogPrintf("voipconcierge: ping site is required to register with the CCS. please add a ping site to the configuration\n");
        return(NULL);
    }

    // set the versions
    ProtoTunnelStatus(pProtoTunnel, 'vset', 0, strVersions, sizeof(strVersions));

    // if we are offloading SSL to stunnel then set the flag to bind to localhost which stunnel uses for communication
    if (pConfig->bOffloadInboundSSL)
    {
        uRestApiFlags |= PROTOHTTPSERV_FLAG_LOOPBACK;
    }

    // allocate module state
    if ((pVoipConcierge = (VoipConciergeRefT *)DirtyMemAlloc(sizeof(VoipConciergeRefT), VOIPCONCIERGE_MEMID, iMemGroup, pMemGroupUserdata)) == NULL)
    {
        DirtyCastLogPrintf("voipconcierge: unable to allocate concierge mode module\n");
        return(NULL);
    }
    ds_memclr(pVoipConcierge, sizeof(VoipConciergeRefT));
    pVoipConcierge->pBase = pVoipServer;
    pVoipConcierge->iMemGroup = iMemGroup;
    pVoipConcierge->pMemGroupUserdata = pMemGroupUserdata;

    // allocate the api module
    if ((pVoipConcierge->pApi = RestApiCreate(pConfig->uApiPort, VOIPCONCIERGE_NAME, iNumResources, VOIPCONCIERGE_BODY_SIZE, FALSE, uRestApiFlags)) == NULL)
    {
        DirtyCastLogPrintf("voipconcierge: unable to create the restapi module\n");
        VoipConciergeShutdown(pVoipConcierge);
        return(NULL);
    }
    RestApiCallback(pVoipConcierge->pApi, _VoipConciergeRestApiCompleteCb, pVoipConcierge);
    RestApiControl(pVoipConcierge->pApi, 'cont', 0, 0, (void *)VOIPCONCIERGE_CONTENTTYPE);
    if (pConfig->bAuthValidate == TRUE)
    {
        RestApiControl(pVoipConcierge->pApi, 'asch', 0, 0, (void *)VOIPCONCIERGE_AUTH_SCHEME);
    }
    RestApiControl(pVoipConcierge->pApi, 'spam', pConfig->uDebugLevel, 0, NULL);

    // allocate the ccs module
    if ((pVoipConcierge->pConciergeService = ConciergeServiceCreate(pTokenApi, pConfig->strCertFilename, pConfig->strKeyFilename)) == NULL)
    {
        DirtyCastLogPrintf("voipconcierge: unable to create the conciergeservice module\n");
        VoipConciergeShutdown(pVoipConcierge);
        return(NULL);
    }
    ConciergeServiceControl(pVoipConcierge->pConciergeService, 'spam', pConfig->uDebugLevel, 0, NULL);
    ConciergeServiceControl(pVoipConcierge->pConciergeService, 'ccsa', 0, 0, (void*)pConfig->strCoordinatorAddr);

    // create the elements of the client map
    if ((pVoipConcierge->pClientMap = (ConciergeClientMapEntryT *)DirtyMemAlloc(sizeof(ConciergeClientMapEntryT) * pConfig->iMaxClients, VOIPCONCIERGE_MEMID, iMemGroup, pMemGroupUserdata)) == NULL)
    {
        DirtyCastLogPrintf("voipconcierge: unable to allocate concierge mode module's client map\n");
        VoipConciergeShutdown(pVoipConcierge);
        return(NULL);
    }
    ds_memclr(pVoipConcierge->pClientMap, sizeof(ConciergeClientMapEntryT) * pConfig->iMaxClients);
    pVoipConcierge->iClientMapSize = pConfig->iMaxClients;

    // create the elements of the product map
    if ((pVoipConcierge->pProductMap = (ConciergeProductMapEntryT *)DirtyMemAlloc(sizeof(ConciergeProductMapEntryT) * VOIPCONCIERGE_PRODUCTMAP_SIZE, VOIPCONCIERGE_MEMID, iMemGroup, pMemGroupUserdata)) == NULL)
    {
        DirtyCastLogPrintf("voipconcierge: unable to allocate concierge mode module's product map\n");
        VoipConciergeShutdown(pVoipConcierge);
        return(NULL);
    }
    ds_memclr(pVoipConcierge->pProductMap, sizeof(ConciergeProductMapEntryT) * VOIPCONCIERGE_PRODUCTMAP_SIZE);
    pVoipConcierge->iProductMapSize = VOIPCONCIERGE_PRODUCTMAP_SIZE;

    // create the connection set tracking
    pVoipConcierge->iNumConnSets = VoipTunnelStatus(pVoipTunnel, 'mgam', 0, NULL, 0);
    if ((pVoipConcierge->pConnectionSets = (ConnectionSetT *)DirtyMemAlloc(sizeof(*pVoipConcierge->pConnectionSets) * pVoipConcierge->iNumConnSets, VOIPCONCIERGE_MEMID, iMemGroup, pMemGroupUserdata)) == NULL)
    {
        DirtyCastLogPrintf("voipconcierge: unable to allocate concierge mode module's connection sets data\n");
        VoipConciergeShutdown(pVoipConcierge);
        return(NULL);
    }
    ds_memclr(pVoipConcierge->pConnectionSets, sizeof(*pVoipConcierge->pConnectionSets) * pVoipConcierge->iNumConnSets);

    // create the hasher, we don't expect many of these so just use the defaults
    if ((pVoipConcierge->pValidatingRequests = HasherCreate(0, 0)) == NULL)
    {
        DirtyCastLogPrintf("voipconcierge: unable to allocate validating requests hash table\n");
        VoipConciergeShutdown(pVoipConcierge);
        return(NULL);
    }

    // add the resources we handle
    for (iIndex = 0; iIndex < iNumResources; iIndex += 1)
    {
        RestApiAddResource(pVoipConcierge->pApi, &_VoipConcierge_aResources[iIndex], pVoipConcierge);
    }

    // configure game/voip callback
    VoipServerCallback(pVoipServer, _VoipConciergeGameEventCb, _VoipConciergeVoipTunnelEventCallback, pVoipConcierge);

    // initialize max load detection window
    pVoipConcierge->uMaxLoadThresholdDetectionStart = NetTick();

    return(pVoipConcierge);
}

/*F********************************************************************************/
/*!
    \Function VoipConciergeProcess

    \Description
        Process loop for Connection Concierge mode of the voipserver

    \Input *pVoipConcierge  - mode state
    \Input *pSignalFlags    - VOIPSERVER_SIGNAL_* flags passing signal state to process.
    \Input uCurTick         - current tick for this frame

    \Output
        uint8_t             - TRUE to continue processing, FALSE to exit

    \Version 09/24/2015 (eesponda)
*/
/********************************************************************************F*/
uint8_t VoipConciergeProcess(VoipConciergeRefT *pVoipConcierge, uint32_t *pSignalFlags, uint32_t uCurTick)
{
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipConcierge->pBase);
    uint32_t uCapacity;

    // are we operating above or under the configured max load threshold?
    _VoipConciergeMaxLoadUpdate(pVoipConcierge);

    // check signal flags
    if (*pSignalFlags & VOIPSERVER_SIGNAL_SHUTDOWN_IFEMPTY)
    {
        // if we aren't already draining, start draining
        if (VoipServerStatus(pVoipConcierge->pBase, 'psta', 0, NULL, 0) < VOIPSERVER_PROCESS_STATE_DRAINING)
        {
            VoipServerProcessMoveToNextState(pVoipConcierge->pBase, VOIPSERVER_PROCESS_STATE_DRAINING);

            // immediately heartbeat the CCS such that it knows quick that we no longer accept subsequent conn requests
            ConciergeServiceControl(pVoipConcierge->pConciergeService, 'hrbt', 0, 0, NULL);
        }
    }

    if (VoipServerStatus(pVoipConcierge->pBase, 'psta', 0, NULL, 0) == VOIPSERVER_PROCESS_STATE_EXITED)
    {
        return(FALSE);
    }

    // pump the API, to allow us to handle requests
    RestApiUpdate(pVoipConcierge->pApi);

    uCapacity = ((VoipServerStatus(pVoipConcierge->pBase, 'psta', 0, NULL, 0) != VOIPSERVER_PROCESS_STATE_RUNNING) || pVoipConcierge->bMaxLoad) ?
        0 : (VoipTunnelStatus(pVoipTunnel, 'musr', 0, NULL, 0) - VoipTunnelStatus(pVoipTunnel, 'nusr', -1, NULL, 0));

    // give life to module in charge of interfacing CC
    _VoipConciergeUpdateCCS(pVoipConcierge, uCapacity);

    // poll for a server id
    _VoipConciergePollHostingId(pVoipConcierge);

    // check for expired connections
    _VoipConciergeCheckExpired(pVoipConcierge, uCurTick);

    // update monitor flags
    _VoipConciergeUpdateMonitor(pVoipConcierge, uCurTick);

    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function VoipConciergeStatus

    \Description
        Get module status

    \Input *pVoipConcierge  - pointer to module state
    \Input iSelect          - status selector
    \Input iValue           - selector specific
    \Input *pBuf            - [out] - selector specific
    \Input iBufSize         - size of output buffer

    \Output
        int32_t             - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'actv'  - returns number of active clients (per product, per tunnel vers)
            'done'  - returns if the module has finished its time critical operations
            'fail'  - returns TRUE if the server has entered the "max load" state  (can't accept more clients)
            'offl'  - returns whether we are offline (unknown to CCS) or online (known to CCS)
            'prod'  - returns pointer to product map
            'read'  - returns TRUE if kubernetes readiness probe can succeed, FALSE otherwise
        \endverbatim

    \Version 12/09/2015 (eesponda)
*/
/********************************************************************************F*/
int32_t VoipConciergeStatus(VoipConciergeRefT *pVoipConcierge, int32_t iStatus, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iStatus == 'actv')
    {
        int32_t iIndex;
        int32_t iNumClients = 0;
        uint32_t uTunnelVer = (uint32_t)iValue;
        const char *pProductId = (const char*)pBuf;

        for (iIndex = 0; iIndex < pVoipConcierge->iClientMapSize; iIndex++)
        {
            const ConciergeClientMapEntryT *pEntry = &pVoipConcierge->pClientMap[iIndex];
            VoipTunnelClientT *pVoipTunnelClient = VoipTunnelClientListMatchId(VoipServerGetVoipTunnel(pVoipConcierge->pBase), pEntry->uClientId);

            // make sure client is valid
            if (pVoipTunnelClient != NULL)
            {
                uint32_t bTunnelVerMatch = FALSE;
                uint32_t bProductIdMatch = FALSE;

                // do we have a match on the specified product (if applicable)
                if ((pProductId == NULL) || (strncmp(pEntry->strProductId, pProductId, sizeof(pEntry->strProductId)) == 0))
                {
                    bProductIdMatch = TRUE;
                }

                // do we have a match on the tunnel version (if applicable)
                if ((uTunnelVer == 0) || (uTunnelVer == (uint32_t)ProtoTunnelStatus(VoipServerGetProtoTunnel(pVoipConcierge->pBase), 'vers', pVoipTunnelClient->uTunnelId, NULL, 0)))
                {
                    bTunnelVerMatch = TRUE;
                }

                if (bProductIdMatch && bTunnelVerMatch)
                {
                    iNumClients++;
                }
            }
        }
        return(iNumClients);
    }
    if (iStatus == 'done')
    {
        uint8_t bDone;
        TokenApiRefT *pTokenRef = VoipServerGetTokenApi(pVoipConcierge->pBase);

        // we need to finish all nucleus operations or other RSA operations in a timely manner
        bDone  = TokenApiStatus(pTokenRef, 'done', NULL, 0);
        bDone &= ConciergeServiceStatus(pVoipConcierge->pConciergeService, 'done', 0, NULL, 0);

        return(bDone);
    }
    if (iStatus == 'offl')
    {
        // pass through to ConciergeService module
        return(ConciergeServiceStatus(pVoipConcierge->pConciergeService, iStatus, 0, NULL, 0) != 0);
    }
    if (iStatus == 'prod')
    {
        *(ConciergeProductMapEntryT **)pBuf = pVoipConcierge->pProductMap;
        return(pVoipConcierge->iProductMapSize);
    }
    if (iStatus == 'read')
    {
        // we return 'ready' as soon as successfully registered with CCS (i.e. hosting server id is not 0)
        return(ConciergeServiceStatus(pVoipConcierge->pConciergeService, 'clid', 0, NULL, 0) != 0);
    }

    // selector unsupported
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function VoipConciergeControl

    \Description
        Module control function

    \Input *pVoipConcierge - pointer to module state
    \Input iControl        - control selector
    \Input iValue          - selector specific
    \Input iValue2         - selector specific
    \Input *pValue         - selector specific

    \Output
        int32_t            - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'spam' - update debug level via iValue
        \endverbatim

    \Version 12/09/2015 (eesponda)
*/
/********************************************************************************F*/
int32_t VoipConciergeControl(VoipConciergeRefT *pVoipConcierge, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'spam')
    {
        RestApiControl(pVoipConcierge->pApi, iControl, iValue, iValue2, pValue);
        ConciergeServiceControl(pVoipConcierge->pConciergeService, iControl, iValue, iValue2, pValue);
        return(0);
    }

#if DIRTYCODE_DEBUG
    if (iControl == 'fpct')
    {
        DirtyCastLogPrintf("voipconcierge: [debug] FAKE PCTSERVERLOAD changed to --> '%C'\n", iValue);
        pVoipConcierge->uFakePctServerLoad = (uint32_t)iValue;
        return(0);
    }
#endif

    // control unsupported
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function VoipConciergeDrain

    \Description
        Perform drain actions.

    \Input *pVoipConcierge  - module state

    \Output
        uint32_t            - TRUE if drain complete, FALSE otherwise

    \Version 01/07/2020 (mclouatre)
*/
/********************************************************************************F*/
uint32_t VoipConciergeDrain(VoipConciergeRefT *pVoipConcierge)
{
    if (VoipTunnelStatus(VoipServerGetVoipTunnel(pVoipConcierge->pBase), 'ngam', 0, NULL, 0) == 0)
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

/*F********************************************************************************/
/*!
    \Function VoipConciergeShutdown

    \Description
        Cleans up the mode state

    \Input *pVoipConcierge   - mode state

    \Version 09/24/2015 (eesponda)
*/
/********************************************************************************F*/
void VoipConciergeShutdown(VoipConciergeRefT *pVoipConcierge)
{
    // destroy the validating requests hash table
    if (pVoipConcierge->pValidatingRequests != NULL)
    {
        HasherDestroy(pVoipConcierge->pValidatingRequests);
        pVoipConcierge->pValidatingRequests = NULL;
    }

    // destroy the connection set info
    if (pVoipConcierge->pConnectionSets != NULL)
    {
        DirtyMemFree(pVoipConcierge->pConnectionSets, VOIPCONCIERGE_MEMID, pVoipConcierge->iMemGroup, pVoipConcierge->pMemGroupUserdata);
        pVoipConcierge->pConnectionSets = NULL;
    }

    // destroy the product map
    if (pVoipConcierge->pProductMap != NULL)
    {
        DirtyMemFree(pVoipConcierge->pProductMap, VOIPCONCIERGE_MEMID, pVoipConcierge->iMemGroup, pVoipConcierge->pMemGroupUserdata);
        pVoipConcierge->pProductMap = NULL;
    }

    // destroy the client map
    if (pVoipConcierge->pClientMap != NULL)
    {
        DirtyMemFree(pVoipConcierge->pClientMap, VOIPCONCIERGE_MEMID, pVoipConcierge->iMemGroup, pVoipConcierge->pMemGroupUserdata);
        pVoipConcierge->pClientMap = NULL;
    }

    // destroy conciergeservice module
    if (pVoipConcierge->pConciergeService != NULL)
    {
        ConciergeServiceDestroy(pVoipConcierge->pConciergeService);
        pVoipConcierge->pConciergeService = NULL;
    }

    // destroy api module
    if (pVoipConcierge->pApi != NULL)
    {
        RestApiDestroy(pVoipConcierge->pApi);
        pVoipConcierge->pApi = NULL;
    }

    // cleanup module state
    DirtyMemFree(pVoipConcierge, VOIPCONCIERGE_MEMID, pVoipConcierge->iMemGroup, pVoipConcierge->pMemGroupUserdata);
}

/*F********************************************************************************/
/*!
    \Function VoipConciergeGetBase

    \Description
        Gets the base module for getting shared functionality

    \Input *pVoipConcierge   - module state

    \Output
        VoipServerRefT *     - module state to our shared functionality between modes

    \Version 12/02/2015 (eesponda)
*/
/********************************************************************************F*/
VoipServerRefT *VoipConciergeGetBase(VoipConciergeRefT *pVoipConcierge)
{
    return(pVoipConcierge->pBase);
}
