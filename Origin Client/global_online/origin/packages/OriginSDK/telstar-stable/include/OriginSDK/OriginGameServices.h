#ifndef __ORIGIN_GAME_SERVICES_H__
#define __ORIGIN_GAME_SERVICES_H__

#include "OriginTypes.h"
#include "OriginEnumeration.h"

#ifdef __cplusplus
extern "C"
{
#endif
/// \defgroup gameservices Game Services
/// \brief This module contains the functions for game service facilities in Origin.

/// \ingroup gameservices
/// \brief  Start Game Services Support
/// This will load a DLL into the process that has the support for the game services.
/// \param reserved In the future we probably want to be able to pass in some flags. Pass in 0 for now.
/// \param callback A game defined function that receives the result of the operation.
/// \param pContext A game defined context for the callback, this context will be passed back into the callback function.
/// \param timeout The time in milliseconds the game is prepared to wait for the result.
    
OriginErrorT OriginStartupGameServices(int reserved, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout);

/// \ingroup gameservices
/// \brief  Shutdown Game Services Support
/// This will disconnect from the game server, and releases all resources for game services support
/// \param callback A game defined function that receives the result of the operation.
/// \param pContext A game defined context for the callback, this context will be passed back into the callback function.
/// \param timeout The time in milliseconds the game is prepared to wait for the result.

OriginErrorT OriginShutdownGameServices(OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout);


enum enumServerType
{
    ORIGIN_SERVER_TYPE_LOCAL,       ///< Servers that are in the local subnet.
    ORIGIN_SERVER_TYPE_NEAR,        ///< Servers that are near to the user geographically.
    ORIGIN_SERVER_TYPE_CONTINENT,   ///< Servers that are on the same continent approximately 
    ORIGIN_SERVER_TYPE_GLOBAL       ///< Servers that are anywhere in the world.
};

struct OriginServerInfoT
{
    const OriginCharT * ServerName;         ///< A user readable name for the server
    const OriginCharT * ServerAttributes;   ///< A game defined set of attributes for the server. Can be game modes etc.
    const OriginCharT * ServerId;           ///< An id that identifies the server, and can be used to log into an instance.
    const OriginCharT * ServerImageId;      ///< An id to the image of the server. Use \ref OriginQueryImage(ServerImageId, ...) to retrieve.
    const OriginCharT * LocationName;       ///< The location of the server, eg. Amsterdam NH, The Netherlands.
    const OriginCharT * LocationCountry;    ///< The country code for the location of the server. NLD, USA, CAN, etc   (ISO 3166-1 alpha-3)
    int                 Ping;               ///< The average ping to the server.
    int                 NumberOfPlayers;    ///< The number of players currently on the server.
    int                 MaxNumberOfPlayers; ///< The max number of players for that server.
    int                 ExpertLevel;        ///< Indication on the average level of players on the server. [0-beginner, 10-expert]
    char                bPasswordProtected; ///< Indication that the server requires a password.          

    // TBD
};

/// \brief Query for game servers
///
/// \param serverType Instruct the query what to look for. 
///	\param[in] callback The callback function to be called when information arrives. The returned data will be \ref OriginServerInfoT
///	\param[in] pContext A user-defined context to be used in the callback.
/// \param[in] timeout A timeout in milliseconds. 
/// \param[out] pHandle The handle to identify this enumeration. Use OriginDestroyHandle() to release the resources associated with this enumeration. pass in NULL for this value.
OriginErrorT OriginQueryServers(enumServerType serverType, OriginEnumerationCallbackFunc callback, void *pContext, uint32_t timeout, OriginHandleT *pHandle);

struct OriginServerJoinInfoT
{
    OriginServerInfoT ServerInfo;       ///< A copy of the server information.
    const OriginCharT * SessionId;      ///< A unique id that identifies the users session on that server. 
    const OriginCharT * LobbyId;        ///< The lobby the user is currently placed in.
    /// TBD
};

typedef OriginErrorT (*OriginServerJoinInfoCallbackFunc)(void *pContext, OriginServerJoinInfoT *joinInfo, OriginSizeT size, OriginErrorT err);

/// \ingroup gameservices
/// \brief Try to join a server. This may fail if the server is full, or no longer available.
/// \param[in] userId The userId of the user who is trying to log in.
/// \param[in] authCode The auth code to allow access to the server. Use \ref OriginRequestAuthCode to obtain the proper authorization code.
/// \param[in] serverId The serverId of the server the user is trying to join.
/// \param[in] password An optional password.
///	\param[in] callback The callback function to be called when information arrives. The returned data will be \ref OriginServerJoinInfoT
///	\param[in] pContext A user-defined context to be used in the callback.
/// \param[in] timeout A timeout in milliseconds. 
OriginErrorT OriginJoinServer(OriginUserT userId, const OriginCharT * authCode, const OriginCharT * serverId, const OriginCharT * password, OriginServerJoinInfoCallbackFunc callback, void * pContext, uint32_t timeout);


/// \ingroup gameservices
/// \brief  Leave the server the user is currently connected to.
/// \param userId The userId for the user to disconnect.
/// \param sessionId The session id that identifies the user on the server.
/// \param callback A game defined function that receives the result of the operation.
/// \param pContext A game defined context for the callback, this context will be passed back into the callback function.
/// \param timeout The time in milliseconds the game is prepared to wait for the result.
OriginErrorT OriginLeaveServer(OriginUserT userId, const OriginCharT * sessionId, OriginErrorSuccessCallback callback, void * pContext, uint32_t timeout);


#ifdef __cplusplus
}
#endif


#endif //__ORIGIN_GAME_SERVICES_H__