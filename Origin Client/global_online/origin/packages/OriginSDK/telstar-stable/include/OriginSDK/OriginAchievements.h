#ifndef __ORIGIN_ACHIEVEMENTS_H__
#define __ORIGIN_ACHIEVEMENTS_H__

#include "OriginTypes.h"

#ifdef __cplusplus
extern "C" 
{
#endif

    /// \defgroup achievements Achievements
    /// \brief This module contains the functions that provides Origin's Achievement feature.

    /// \ingroup achievements
    /// \brief Grant an achievement to the user.
    /// 
    /// \param[in] user The local user who gets the Achievement granted. Use #OriginGetDefaultUser().
    /// \param[in] persona The origin or game persona who gets the Achievement granted. Get the persona from OriginGetProfile for the Origin Persona.
    /// \param[in] achievementId The id of the achievement.
    /// \param[in] progress The amount of progress to add onto the achievement (use 1 for regular achievement) .
    /// \param[in] achievementCode The secret key defined in the operations configuration for the achievement product.
    /// \param[in] timeout The amount of time in milli-seconds that you want to wait for this function to succeed.
    /// \param[in] callback A callback to a handler function that will receive a notification when the achievement is sent, or an error occurred.
    /// \param[in] pContext A user-provided context for the callback.
    /// \return ORIGIN_SUCCESS when succeeded, ORIGIN_PENDING when the achievement is cached for offline posting. ORIGIN_ERROR_PROXY_UNAUTHORIZED when the achievementCode doesn't match the code configured in the GOSCC tool. ORIGIN_ERROR_PROXY_NOT_FOUND when trying to post an achievement for a game that is not configured for achievements.
    OriginErrorT ORIGIN_SDK_API OriginGrantAchievement(OriginUserT user, OriginPersonaT persona, int achievementId, int progress, const OriginCharT * achievementCode, uint32_t timeout, OriginAchievementCallback callback, void *pContext);

    /// \ingroup achievements
    /// \brief Grant an achievement to the user.
    /// 
    /// \param[in] user The local user who gets the Achievement granted. Use #OriginGetDefaultUser().
    /// \param[in] persona The origin or game persona who gets the Achievement granted. Get the persona from OriginGetProfile for the Origin Persona.
    /// \param[in] achievementId The id of the achievement.
    /// \param[in] progress The amount of progress to add onto the achievement (use 1 for regular achievement) .
    /// \param[in] achievementCode The secret key defined in the operations configuration for the achievement product.
    /// \param[in] timeout The amount of time in milli-seconds that you want to wait for this function to succeed.
    /// \param[out] achievement The awarded achievement.
    /// \param[out] handle A handle that manages the resources associated with the achievement record. Use #OriginDestroyHandle to release the resources.
    /// \return ORIGIN_SUCCESS when succeeded, ORIGIN_PENDING when the achievement is cached for offline posting. ORIGIN_ERROR_PROXY_UNAUTHORIZED when the achievementCode doesn't match the code configured in the GOSCC tool. ORIGIN_ERROR_PROXY_NOT_FOUND when trying to post an achievement for a game that is not configured for achievements.
    OriginErrorT ORIGIN_SDK_API OriginGrantAchievementSync(OriginUserT user, OriginPersonaT persona, int achievementId, int progress, const OriginCharT * achievementCode, uint32_t timeout, OriginAchievementT *achievement, OriginHandleT *handle);


    /// \ingroup achievements
    /// \brief Create an achievement event document. Use \ref OriginDestroyHandle to release the associated resources after the post.
    /// \param[out] eventHandle The created event record.
    /// \return ORIGIN_SUCCESS when the record is successfully created, ORIGIN_ERROR_INVALID_PARAMETER if eventHandle == NULL.
    OriginErrorT ORIGIN_SDK_API OriginCreateEventRecord(OriginHandleT * eventHandle);

    /// \ingroup achievements
    /// \brief Add an event to the event record.
    /// \param[in] eventHandle The event record you want to add an event to.
    /// \param[in] eventName The name of the event to add.
    /// \return ORIGIN_SUCCESS when the event is added. ORIGIN_ERROR_INVALID_HANDLE if the handle is not for an event. ORIGIN_ERROR_ALREADY_EXISTS when trying to add an event, when an event without parameters is already in the event record.
    OriginErrorT ORIGIN_SDK_API OriginAddEvent(OriginHandleT eventHandle, const OriginCharT * eventName);

    /// \ingroup achievements
    /// \brief Add a parameter to the current event
    /// \param[in] eventHandle The event record you want to add an event parameter to.
    /// \param[in] name The name of the parameter.
    /// \param[in] value The value of the parameter.
    /// \return ORIGIN_SUCCESS when the parameter is properly added.
    OriginErrorT ORIGIN_SDK_API OriginAddEventParameter(OriginHandleT eventHandle, const OriginCharT *name, const OriginCharT *value);

    /// \ingroup achievements
    /// \brief Send achievement event.
    /// 
    /// \param[in] user The local user who gets the Achievement granted. Use #OriginGetDefaultUser().
    /// \param[in] persona The origin or game persona who gets the Achievement granted. Get the persona from OriginGetProfile for the Origin Persona.
    /// \param[in] eventHandle A handle to an achievement Event Document.
    /// \param[in] timeout The amount of time in milli-seconds that you want to wait for this function to succeed.
    /// \param[in] callback A callback to a handler function that will receive a notification when the achievement is sent, or an error occurred.
    /// \param[in] pContext A user-provided context for the callback.
    OriginErrorT ORIGIN_SDK_API OriginPostAchievementEvents(OriginUserT user, OriginPersonaT persona, OriginHandleT eventHandle, uint32_t timeout, OriginErrorSuccessCallback callback, void *pContext);

    /// \ingroup achievements
    /// \brief Grant an achievement to the user.
    /// 
    /// \param[in] user The local user who gets the Achievement granted. Use #OriginGetDefaultUser().
    /// \param[in] persona The origin or game persona who gets the Achievement granted. Get the persona from OriginGetProfile for the Origin Persona.
    /// \param[in] eventHandle A handle to an achievement Event Document.
    /// \param[in] timeout The amount of time in milli-seconds that you want to wait for this function to succeed.
    /// \param[out] achievement The awarded achievement.
    /// \param[out] achievementHandle A handle that manages the resources associated with the achievement record. Use #OriginDestroyHandle to release the resources.
    OriginErrorT ORIGIN_SDK_API OriginPostAchievementEventsSync(OriginUserT user, OriginPersonaT persona, OriginHandleT eventHandle, uint32_t timeout/*, OriginAchievementT *achievement, OriginHandleT *achievementHandle*/);


#ifdef ORIGIN_BACKWARDS_COMPATIBILITY_CHECKING

    /// \ingroup achievements
    /// \brief post wincodes.
    /// 
    /// \param[in] user The local user for whom to post the wincodes for. Use #OriginGetDefaultUser().
    /// \param[in] persona The origin or game persona who you post wincodes for. Get the persona from OriginGetProfile for the Origin Persona.
    /// \param[in] authcode An authorization code for identifying the wincode source.
    /// \param[in] keys The wincode keys to post.
    /// \param[in] values The wincode values to post.
    /// \param[in] count The number of wincodes to post.
    /// \param[in] timeout The amount of time in milli-seconds that you want to wait for this function to succeed.
    /// \param[in] callback A callback to a handler function that will receive a notification when the wincodes are sent, or an error occurred.
    /// \param[in] pContext A user-provided context for the callback.
    OriginErrorT ORIGIN_SDK_API ORIGIN_DEPRECATED("The wincode system was never completed. Dropping support. Use OriginPostAchievementEvents instead") OriginPostWincodes(OriginUserT user, OriginPersonaT persona, const OriginCharT *authcode, const OriginCharT ** keys, const OriginCharT ** values, OriginSizeT count, uint32_t timeout, OriginErrorSuccessCallback callback, void *pContext);

    /// \ingroup achievements
    /// \brief Grant an achievement to the user.
    /// 
    /// \param[in] user The local user for whom to post the wincodes for. Use #OriginGetDefaultUser().
    /// \param[in] persona The origin or game persona who you post wincodes for. Get the persona from OriginGetProfile for the Origin Persona.
    /// \param[in] authcode An authorization code for identifying the wincode source.
    /// \param[in] keys The wincode keys to post.
    /// \param[in] values The wincode values to post.
    /// \param[in] count The number of wincodes to post.
    /// \param[in] timeout The amount of time in milli-seconds that you want to wait for this function to succeed.
    OriginErrorT ORIGIN_SDK_API ORIGIN_DEPRECATED("The wincode system was never completed. Dropping support. use OriginPostAchievementEventsSync instead") OriginPostWincodesSync(OriginUserT user, OriginPersonaT persona, const OriginCharT *authcode, const OriginCharT ** keys, const OriginCharT ** values, OriginSizeT count, uint32_t timeout);

#endif //ORIGIN_BACKWARDS_COMPATIBILITY_CHECKING

    /// \ingroup achievements
    /// \brief Queries the achievements a user owns.
    ///
    /// Query the achievements the user owns including from other titles.
    /// \param[in] user The local user.
    /// \param[in] persona The origin or game persona for who you want to get the Achievements for. Get the persona from OriginGetProfile for the Origin Persona.
    /// \param[in] pGameIdList[] A list of game ids to query the achievements for. If no id is specified the default AchievementSet for the game is returned.
    /// \param[in] gameIdCount The number of game Ids in the list (0=query achievements for the current game only).
    /// \param[in] mode This determines whether the QueryAchievements returns only the active, or all achievements for the provided gameIds.
    /// \param[in] callback A callback function that will be called from \ref OriginUpdate as data arrives.
    /// \param[in] pContext A data pointer for the callback function.
    /// \param[in] timeout The amount of time the SDK will wait for a response.
    /// \param[out] pHandle A handle for \ref OriginReadEnumeration functions to return \ref OriginAchievementSetT structures.
    OriginErrorT ORIGIN_SDK_API OriginQueryAchievements(OriginUserT user, OriginPersonaT persona, const OriginCharT *pGameIdList[], OriginSizeT gameIdCount, enumAchievementMode mode, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);

    /// \ingroup achievements
    /// \brief Queries the achievements a user owns.
    ///
    /// Query the achievements the user owns including from other titles.
    /// \param[in] user The local user.
    /// \param[in] persona The origin or game persona for who you want to get the Achievements for. Get the persona from OriginGetProfile for the Origin Persona.
    /// \param[in] pGameIdList[] A list of game ids to query the achievements for.  If no id is specified the default AchievementSet for the game is returned.
    /// \param[in] gameIdCount The number of game Ids in the list (0=query achievements for the current game only).
    /// \param[in] mode This determines whether the QueryAchievements returns only the active, or all achievements for the provided gameIds.
    /// \param[out] pTotalCount The number of achievement Sets returned. 
    /// \param[in] timeout The amount of time the SDK will wait for a response.
    /// \param[out] pHandle A handle for \ref OriginReadEnumeration functions to return \ref OriginAchievementSetT structures.
    OriginErrorT ORIGIN_SDK_API OriginQueryAchievementsSync(OriginUserT user, OriginPersonaT persona, const OriginCharT *pGameIdList[], OriginSizeT gameIdCount, enumAchievementMode mode, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle);


#ifdef __cplusplus
}
#endif



#endif //__ORIGIN_ACHIEVEMENTS_H__