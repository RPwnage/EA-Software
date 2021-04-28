#ifndef __ORIGIN_CONTENT_H__
#define __ORIGIN_CONTENT_H__

#include "OriginTypes.h"
#include "OriginEnumeration.h"

#ifdef __cplusplus
extern "C"
{
#endif
/// \defgroup content Content
/// \brief This module contains the functions for getting information about content in Origin.

/// \ingroup content
/// \brief Get the content information for the current content state in Origin.
/// In order to know what the state of the content is in Origin you can call OriginQueryContent to get the baseline of the current state of the content.
/// Any subsequent updates to the content will be provided through the ORIGIN_EVENT_CONTENT events.
/// \param[in] user The user for whom the content is requested for.
/// \param[in] gameIds A filter option, to only get updates for content that belongs to a certain gameId(s). This is usually a list of masterTitleIds, but if you need to specify offerIds, or contentIds, please prefix value with 'offerId:', 'contentId:'. (other valid prefixes are: 'masterTitleId:', 'multiplayerId:'). If NULL all content will be returned.
/// \param[in] gameIdCount The number of items in the gameIds array.
/// \param[in] multiplayerId The optional multiplayer Id to filter the content on. Specifying a multiplayer Id will cause contentType to be replaced by ORIGIN_QUERY_CONTENT_BASE_GAME. Specify NULL if this parameter is not required.
/// \param[in] contentType Specify what kind of content you want returned. contentType is a concatenation of the enumContentType enumeration. (ORIGIN_QUERY_CONTENT_DLC | ORIGIN_QUERY_CONTENT_ULC). Specifying 0 is the same as specifying ORIGIN_QUERY_CONTENT_ALL.
/// \param[in] callback The function to call when the query is done/error occurred.
/// \param[in] context An game provided context for this query. The context will be passed back into the callback. In general this is pointing to the game side handler class.
/// \param[in] timeout A timeout value used to indicate how long the SDK will wait for a return from Origin. Value in milliseconds.
/// \param[out] handle The query handle to the enumeration object. You can pass in NULL if you specify a callback.
/// \return An error code indicating whether the function executed successfully. This will only return an error of the request couldn't be posted, and doesn't indicate whether the request can be completed.
OriginErrorT ORIGIN_SDK_API OriginQueryContent(OriginUserT user, const OriginCharT *gameIds[], OriginSizeT gameIdCount, const OriginCharT * multiplayerId, uint32_t contentType, OriginEnumerationCallbackFunc callback, void* context, uint32_t timeout, OriginHandleT *handle);

/// \ingroup content
/// \brief Get the content information for the current content state in Origin.
/// In order to know what the state of the content is in Origin you can call OriginQueryContent to get the baseline of the current state of the content.
/// Any subsequent updates to the content will be provided through the ORIGIN_EVENT_CONTENT events.
/// \param[in] user The user for whom the content is requested for.
/// \param[in] gameIds A filter option, to only get updates for content that belongs to a certain gameId(s). This is usually a list of masterTitleIds, but if you need to specify offerIds, or contentIds, please prefix value with 'offerId:', 'contentId:'. (other valid prefixes are: 'masterTitleId:', 'multiplayerId:'). If NULL all content will be returned.
/// \param[in] gameIdCount The number of items in the gameIds array.
/// \param[in] multiplayerId The optional multiplayer Id to filter the content on. Specifying a multiplayer Id will cause contentType to be replaced by ORIGIN_QUERY_CONTENT_BASE_GAME. Specify NULL if this parameter is not required.
/// \param[in] contentType Specify what kind of content you want returned. contentType is a concatenation of the enumContentType enumeration. (ORIGIN_QUERY_CONTENT_DLC | ORIGIN_QUERY_CONTENT_ULC). Specifying 0 is the same as specifying ORIGIN_QUERY_CONTENT_ALL.
/// \param[out] total Returns the number of OriginContentT records are going to be returned in the OriginReadEnumeration function.
/// \param[in] timeout A timeout value used to indicate how long the SDK will wait for a return from Origin. Value in milliseconds.
/// \param[out] handle The query handle to the enumeration object. You can pass in NULL if you specify a callback.
/// \return An error code indicating whether the function executed successfully. This will only return an error of the request couldn't be posted, and doesn't indicate whether the request can be completed.
OriginErrorT ORIGIN_SDK_API OriginQueryContentSync(OriginUserT user, const OriginCharT *gameIds[], OriginSizeT gameIdCount, const OriginCharT * multiplayerId, uint32_t contentType, OriginSizeT *total, uint32_t timeout, OriginHandleT *handle);


/// \ingroup content
/// \brief Restart the game after the current game has terminated.
/// In order for some games to perform an update flow, they may need to be restarted. This function will allow the game to request a restart from Origin.
/// \param[in] user The user for whom the restart is requested for.
/// \param[in] option Specify how the the game should be restarted. Specify ORIGINRESTARTGAME_NORMAL if you want the Origin logic to dictate how the title is updated.
/// \param[in] callback The function to call when the request is done/error occurred.
/// \param[in] context An game provided context for this query. The context will be passed back into the callback. In general this is pointing to the game side handler class.
/// \param[in] timeout A timeout value used to indicate how long the SDK will wait for a return from Origin. Value in milliseconds.
/// \return An error code indicating whether the function executed successfully.
OriginErrorT ORIGIN_SDK_API OriginRestartGame(OriginUserT user, enumRestartGameOptions option, OriginErrorSuccessCallback callback, void *context, uint32_t timeout);

/// \ingroup content
/// \brief Restart the game after the current game has terminated.
/// In order for some games to perform an update flow, they may need to be restarted. This function will allow the game to request a restart from Origin.
/// \param[in] user The user for whom the restart is requested for.
/// \param[in] option Specify how the the game should be restarted. Specify ORIGINRESTARTGAME_NORMAL if you want the Origin logic to dictate how the title is updated.
/// \param[in] timeout A timeout value used to indicate how long the SDK will wait for a return from Origin. Value in milliseconds.
/// \return An error code indicating whether the function executed successfully.
OriginErrorT ORIGIN_SDK_API OriginRestartGameSync(OriginUserT user, enumRestartGameOptions option, uint32_t timeout);

/// \ingroup content
/// \brief Start a game
/// \param[in] gameId The gameId identifies the game to start. Use either the masterTitleId, offerId or contentId. Please prefix the gameId with 'offerId:' or 'contentId:', when not using a masterTitleId. The 'masterTitleId:' prefix is optional.
/// \param[in] multiplayerId An optional parameter to further narrow down what game to start. The multiplayerId is usually set to the MDM item Id of the standard edition of the game.
/// \param[in] commandLineArgs Additional command line arguments for the game. These arguments will be added to the Origin configured command line arguments.
/// \param[in] callback The function to call when the request is done/error occurred.
/// \param[in] context An game provided context for this query. The context will be passed back into the callback. In general this is pointing to the game side handler class.
/// \param[in] timeout A timeout value used to indicate how long the SDK will wait for a return from Origin. Value in milliseconds.
/// \return An error code indicating whether the function executed successfully.
OriginErrorT ORIGIN_SDK_API OriginStartGame(const OriginCharT * gameId, const OriginCharT * multiplayerId, const OriginCharT * commandLineArgs, OriginErrorSuccessCallback callback, void *context, uint32_t timeout);

/// \ingroup content
/// \brief Start a game
/// \param[in] gameId The gameId identifies the game to start. Use either the masterTitleId, offerId or contentId. Please prefix the gameId with 'offerId:' or 'contentId:', when not using a masterTitleId. The 'masterTitleId:' prefix is optional.
/// \param[in] multiplayerId An optional parameter to further narrow down what game to start. The multiplayerId is usually set to the MDM item Id of the standard edition of the game.
/// \param[in] commandLineArgs Additional command line arguments for the game. These arguments will be added to the Origin configured command line arguments.
/// \param[in] timeout A timeout value used to indicate how long the SDK will wait for a return from Origin. Value in milliseconds.
/// \return An error code indicating whether the function executed successfully.
OriginErrorT ORIGIN_SDK_API OriginStartGameSync(const OriginCharT * gameId, const OriginCharT * multiplayerId, const OriginCharT * commandLineArgs, uint32_t timeout);

#ifdef __cplusplus
}
#endif


#endif //__ORIGIN_CONTENT_H__