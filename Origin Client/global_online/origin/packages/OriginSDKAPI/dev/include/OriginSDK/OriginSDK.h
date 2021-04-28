#ifndef __ORIGINSDK_H__
#define __ORIGINSDK_H__

#include "Origin.h"

/// \mainpage Origin SDK Documentation
///
/// \section intro Introduction to Integrating Origin
///
/// The Origin SDK provides social networking and e-commerce capabilities for EA games. It provides user profiles, the
/// ability to find and connect with friends, the ability to chat with friends, invite them to join the user in game
/// play, and it provides access to an EA online store, from which users can puchase, or pre-purchase, a variety of EA
/// games and add-ons, as well as being able to try out certain games for a limited period of time.
///
/// Origin's APIs are written in standard C and they avoid any operating system-specific concepts. They are designed
/// to be cross-platform to allow straightforward integration into a variety of EA games and applications running on a
/// variety of platforms.
///
/// This Origin SDK documentation is divided into two main parts: the <i>Origin SDK Integrator's Guide</i> and the
/// <i>Origin SDK API Reference</i>, the pages of which are described in the following.
///
/// \section intguide Origin SDK Integrator's Guide
///
/// The <em>Origin SDK Integrator's Guide</em> provides an introduction to the use of the Origin SDK. It provides a
/// high-level procedural guide to the steps required to integrate Origin's features into your game or product.
///
/// Of the pages in this section, the first four pages discuss the basics of integrating Origin into your product, while
/// the remaining pages discuss the integration of specific features, which may be optional for your integration
/// effort. The first four pages, however, cover essential issues in Origin integration.
///
/// <ul>
///   <li><a href="aa-intguide-general.html"><b>Integrating Origin</b></a>: This page discusses the basics of coding the
///     startup of Origin, setting up event callbacks, and updating the SDK so that those events can be processed, as
///     well as shutting Origin down.</li>
///   <li><a href="aa-intguideapi-general.html"><b>Integrating Origin IGO API</b></a>: This page discusses the basics of coding the
///     startup of Origin IGO API, setting up event callbacks, and using the OIG API so that UI & events can be processed, as
///     well as shutting Origin IGO API down.</li>
///   <li><a href="ba-intguide-designpatterns.html"><b>Design Patterns</b></a>: This page discusses Origin design
///     patterns. The topics discussed include "Using Synchronous vs. Asynchronous Queries" and Origin's "Enumerator
///     Pattern".</li>
///   <li><a href="ca-intguide-eventhandling.html"><b>Event Handling</b></a>: This page describes Origin's event handling,
///     which includes demonstrating how to register for events and how to respond to those events by setting the
///     required data type for each different callback function.</li>
///   <li><a href="da-intguide-platforms.html"><b>Developing for Different Platforms</b></a>: This page describes the Origin SDK's handling
///     of development for differnent platforms, which includes accessing the DIME Ecommerce API via the new Services
///     Layer's Ecommerce API, handling multiple players on consoles using controllerIndex(), and displaying the
///     In-Game Overlay on either PCs and Macs or via the IGO API, or displaying them on either X-Boxes or PS3s via
///     the SDK's Services Layer API's allocateSurface() function.</li>
///   <li><a href="ea-intguide-singlesignon.html"><b>Integrating Single Sign-on</b></a>: This page discusses how to
///     get a Nucleus Authtoken through the OriginSDK.</li>
///   <li><a href="fa-intguide-ingameoverlay.html"><b>Integrating the In-Game Overlay</b></a>: This page discusses the
///     integration of the In-Game Overlay, which allows Origin displays to be overlayed on top of the game
///     display.</li>
///   <li><a href="ga-intguide-profile.html"><b>Integrating the User Profile</b></a>: This page discusses the integration
///     of the methods used to retrieve the local user's profile.</li>
///   <li><a href="ha-intguide-friends.html"><b>Integrating the Friends Feature</b></a>: This page discusses the
///     integration of the Origin Friends feature, which includes functions that identify a user's friends and that
///     query for information on those friends.</li>
///   <li><a href="ia-intguide-invites.html"><b>Integrating the Invite Feature</b></a>: This page discusses the integration of the
///     Origin Invite feature, which allows the game to invite other players to join in the default user's game in
///     progress.</li>
///   <li><a href="ja-intguide-ablockeduser.html"><b>Integrating the Block List</b></a>: This page discusses the
///     integration of the Origin Block User feature, which provides methods used to retrieve the Origin block list.</li>
///   <li><a href="ka-intguide-recentplayers.html"><b>Integrating the Recent Players Feature</b></a>: This page discusses
///     the integration of the Origin Recent Players feature, which allows the game to provide a list of users that the
///     local user has recently played with to Origin.</li>
///   <li><a href="la-intguide-presence.html"><b>Integrating the Presence Feature</b></a>: This page discusses the
///     integration of the Origin Presence feature, which sets and gets the local user's presence status, gets the
///     presence status of the local user's friends, and allows the local user to subscribe and unsubscribe to others'
///     presence status.</li>
///   <li><a href="ma-intguide-chat.html"><b>Integrating the Chat Feature</b></a>: This page discusses the integration of
///     the Ebisu Chat feature, which provides instant messaging capabilities between the local user and his or her
///     friends.</li>
///   <li><a href="na-intguide-commerce.html"><b>Integrating Origin Commerce</b></a>: This page discusses the integration
///     of the Origin Commerce feature, which allows the local user to browse store offerings, purchase games and
///     add-ons, and query and redeem the local user's product entitlements.</li>
///   <li><a href="oa-intguide-_f_a_q.html"><b>Origin Integration FAQ</b></a>: This page provides answers to some of the most
///     Frequently Asked Questions about integrating the Origin SDK.</li>
///   <li><a href="za-intguide-appendix1.html"><b>Appendix 1: Multiple Instance Testing</b></a>: This page provides answers to some of the most
///     Frequently Asked Questions about integrating the Origin SDK.</li>
/// </ul>
///
/// \section mods Origin SDK API Reference
///
/// For greater clarity in understanding the Origin Client SDK's API, this reference has been organized into modules
/// that discuss the functions and data types for one particular subject area of the API. The Modules are listed
/// alphabetically in the left navigation pane, but an alternative order that follows the organization of the
/// <i>Integrator's Guide</i> more closely is shown below.
///
/// <ul>
///   <li><a href="group__sdk.html"><b>General</b></a>: This module contains the functions for starting up
///     Origin, for several general operations used by a variety of other functions, and for shutting Origin down.</li>
///   <li><a href="group__igoapi.html"><b>IGO API</b></a>: This module contains the functions for using the
///     Origin IGO API.</li>
///   <li><a href="group__profile.html"><b>Profile</b></a>: This module contains the functions for getting the profile
///     of the default user.</li>
///   <li><a href="group__lang.html"><b>Language</b></a>: This module contains defines that identify the available
///     localized languages.</li>
///   <li><a href="group__igo.html"><b>In-Game Overlay</b></a>: This module contains the functions that provide the
///     Origin In-Game Overlay.</li>
///   <li><a href="group__friends.html"><b>Friends</b></a>: This module contains the functions for Origin's basic social
///     networking features.</li>
///   <li><a href="group__pres.html"><b>Presence</b></a>: This module contains the functions for providing Origin's
///     Presence awareness feature.</li>
///   <li><a href="group__recplay.html"><b>Recent Players</b></a>: This module contains a function that allows Origin to
///     get a list of Recent Players, users that the local user has recently played with from his or her known
///     games.</li>
///   <li><a href="group__invite.html"><b>Invite</b></a>: This module contains the functions that allow the game to
///     Invite other players to join in the default user's game in progress.</li>
///   <li><a href="group__blocked.html"><b>Block List</b></a>: This module contains the functions that allow the game to
///     access the default user's block list.</li>
///   <li><a href="group__chat.html"><b>Chat</b></a>: This module contains the function that provides Ebisu's instant
///     messaging feature.</li>
///   <li><a href="group__ecommerce.html"><b>Commerce</b></a>: This module contains the functions that provide Origin's
///     Commerce features.</li>
///   <li><a href="group__mem.html"><b>Memory</b></a>: This module contains the typedefs and functions that allow Origin
///     SDK integrators to provide their own memory management framework.</li>
///   <li><a href="group__enumerate.html"><b>Enumeration</b></a>: This module contains a typedef and three functions
///     that provide Origin's enumeration capabilities.</li>
///   <li><a href="group__error.html"><b>Errors</b></a>: This module contains the defines that provide Origin's error
///     message capabilities.</li>
///   <li><a href="group__defs.html"><b>Defines</b></a>: This module contains a wide variety of defines used by a
///     variety of Origin functions.</li>
///   <li><a href="group__enums.html"><b>Enums</b></a>: This module contains a set of enums that are used by a variety
///     of Origin functions.</li>
///   <li><a href="group__types.html"><b>Types</b></a>: This module contains a number of structs, defines, and typedefs
///     used by a variety of Origin functions.</li>
///   <li><a href="group__inttypes.html"><b>Integer Types</b></a>: This module contains common type definitions provided
///     for pre-Visual Studio 2010 releases.</li>
/// </ul>
///
/// \htmlonly <p style="text-align: center">&#8212;&nbsp;&#167;&nbsp;&#8212;</p> \endhtmlonly

#include "OriginVersion.h"
#include "OriginTypes.h"
#include "OriginDefines.h"
#include "OriginEnums.h"
#include "OriginError.h"
#include "OriginMemory.h"
#include "OriginProfile.h"
#include "OriginFriends.h"
#include "OriginPresence.h"
#include "OriginEvents.h"
#include "OriginEnumeration.h"
#include "OriginIGO.h"
#include "OriginRecentPlayers.h"
#include "OriginChat.h"
#include "OriginCommerce.h"
#include "OriginInvite.h"
#include "OriginBlocked.h"
#include "OriginAchievements.h"
#include "OriginProgressiveInstallation.h"
#include "OriginContent.h"
#include "OriginInGame.h"

#ifdef __cplusplus
extern "C"
{
#endif

/// \defgroup sdk General
/// \brief This module contains the functions for starting up Origin, for several general operations used by a variety
/// of other functions, and for shutting Origin down.
///
/// There are sychronous and asynchronous versions of several of these functions. For an explanation of why, see
/// \ref syncvsasync in the <em>Integrator's Guide</em>.
///
/// For more information on the integration of this feature, see \ref aa-intguide-general in the <em>Integrator's Guide</em>.

/// \ingroup sdk
/// \brief Gets the default userId.
/// \return The unique user id for the current user.
OriginUserT ORIGIN_SDK_API OriginGetDefaultUser();

/// \ingroup sdk
/// \brief Gets the default userId.
/// \return The unique user id for the current user.
OriginPersonaT ORIGIN_SDK_API OriginGetDefaultPersona();

/// \ingroup sdk
/// \brief Initializes the Origin SDK.
///
/// Must be called prior to any other Origin functions to initialize the SDK for use. May be called multiple times, but
/// each call to startup must later be matched with a call to shutdown. Only the first call to OriginStartup will
/// initialize the SDK using the pInfo parameter. All subsequent calls simply increment an internal reference counter.
/// Thus, the last call to \ref OriginShutdown is what actually shuts down the SDK (when the internal reference counter
/// reaches zero). This allows multiple middleware layers to co-exist gracefully. Origin will startup its own worker
/// threads in the background. This function will also check to see if the Origin desktop client is running (since it
/// does all the actual work). If not, it will attempt to launch it into the background to allow the SDK functions to
/// operate. If the Origin desktop application is not already running or OriginStartup cannot launch it, this function
/// will return an error and all subsequent function calls with return \ref ORIGIN_ERROR_CORE_NOTLOADED.
/// \param[in] flags Zero, or a bitwise OR combination of ORIGIN_FLAG_* values.
/// \param[in] lsxPort Zero to use the default port, non-zero values are used for debugging purposes only.
/// \param[in] pInput The structure is populated with game info.
/// \param[out] pOutput Optional structure to be filled with Origin info (string data remains valid until \ref OriginShutdown).
/// \return Returns \ref ORIGIN_SUCCESS if the OriginSDK is properly initialized, or an error when the SDK couldn't initialize
/// properly (e.g., \ref ORIGIN_ERROR_CORE_NOTLOADED when the Origin couldn't be started, \ref ORIGIN_WARNING_IGO_NOTLOADED when the
/// IGO could not be confirmed to be running).
/// \note If you want IGO to work during development scenarios, where the game is not started through Origin, it will be necessary for
/// some of the graphics API's (DX8, DX9, OpenGL) to start the Origin SDK before initialization of the Graphics API. This will 
/// allow the IGO to be loaded and be properly hooked in.
OriginErrorT ORIGIN_SDK_API OriginStartup(int32_t flags, uint16_t lsxPort, OriginStartupInputT *pInput, OriginStartupOutputT *pOutput);

/// \ingroup sdk
/// \brief select the active OriginSDK instance.
/// \param[in] instance The instance handle for the SDK previously returned in the OriginStartupOutputT structure.
OriginErrorT ORIGIN_SDK_API OriginSelectInstance(OriginInstanceT instance);

/// \ingroup sdk
/// \brief Shuts down the Origin SDK.
///
/// You need to call OriginShutdown as many times as you have called \ref OriginStartup for the SDK to shutdown properly.
/// \return Returns \ref ORIGIN_SUCCESS if the OriginSDK shutdown without errors.
OriginErrorT ORIGIN_SDK_API OriginShutdown();

/// \ingroup sdk
/// \brief Updates callbacks and messages.
///
/// In order for the callbacks to happen in the games main thread context, this function needs to get called.
/// \return Returns \ref ORIGIN_SUCCESS if the OriginSDK update occurred without errors.  ORIGIN_ERROR_CORE_NOTLOADED when the connection with origin is severed.
OriginErrorT ORIGIN_SDK_API OriginUpdate();

#ifdef ORIGIN_BACKWARDS_COMPATIBILITY_CHECKING

/// \ingroup sdk
/// \brief Requests a ticket synchronously.
/// \deprecated Please use \ref OriginRequestAuthCodeSync instead
///
/// Requests a ticket from the Origin Server identifying the user. This function will block until the ticket is
/// received.
///
/// The \ref OriginHandleT can be cast to a pointer, and together with the length information, can be sent to other
/// servers as a means for authentication.
/// \param user This should be the default user, because we cannot issue tickets for 'other' users.
/// \param ticket Receives an \ref OriginHandleT handle when the ticket is received.
/// \param length The size of the ticket.
/// \note For Blaze integrators: DIGITAL goods project ID should be used when setting up titledefines.cfg. Do NOT use the packaged goods project Id.
/// https://developer.origin.com/documentation/display/blaze/Login+And+Account+Creation+Flows#LoginAndAccountCreationFlows-ConfiguringPROJECTIDandManagingONLINEACCESSEntitlements
OriginErrorT ORIGIN_SDK_API ORIGIN_DEPRECATED("This function is deprecated please use OriginRequestAuthCodeSync instead.") OriginRequestTicketSync(OriginUserT user, OriginHandleT *ticket, OriginSizeT *length);

/// \ingroup sdk
/// \brief Requests a ticket asynchronously.
/// \deprecated Please use \ref OriginRequestAuthCode instead
///
/// Requests a ticket from the Origin Server identifying the user. The reply will be provided through the callback function.
/// \param user This should be the default user, because we cannot issue tickets for 'other' users.
/// \param callback The callback function that will be called when the Ticket information is received.
/// \param pcontext The user provided context to the callback system. This context can contain anything. Most of the time it will be a pointer to a handler class.
/// \note For Blaze integrators: DIGITAL goods project ID should be used when setting up titledefines.cfg. Do NOT use the packaged goods project Id.
/// https://developer.origin.com/documentation/display/blaze/Login+And+Account+Creation+Flows#LoginAndAccountCreationFlows-ConfiguringPROJECTIDandManagingONLINEACCESSEntitlements
OriginErrorT ORIGIN_SDK_API ORIGIN_DEPRECATED("This function is deprecated please use OriginRequestAuthCode instead.") OriginRequestTicket(OriginUserT user, OriginTicketCallback callback, void * pcontext);

#endif

/// \ingroup sdk
/// \brief Requests an authcode synchronously.
///
/// Requests an authcode from the Origin Server identifying the user. This function will block until the authcode is
/// received.
///
/// The \ref OriginHandleT can be cast to a pointer, and together with the length information, can be sent to other
/// servers as a means for authentication.
/// \param user This should be the default user, because we cannot issue tickets for 'other' users.
/// \param clientId The client Id the AuthCode should be generated for.
/// \param authcode Receives an \ref OriginHandleT handle when the ticket is received.
/// \param length The size of the authcode.
/// \param scope [optional] Scope used to generate the authcode. This optional param is NULL if not specified and will use the default scope.
/// \param appendAuthSource [optional] Whether to append the Authentication source to the AuthCode request or not. This optional param is false if not specified.
/// \note For Blaze integrators: DIGITAL goods project ID should be used when setting up titledefines.cfg. Do NOT use the packaged goods project Id.
/// https://developer.origin.com/documentation/display/blaze/Login+And+Account+Creation+Flows#LoginAndAccountCreationFlows-ConfiguringPROJECTIDandManagingONLINEACCESSEntitlements
OriginErrorT ORIGIN_SDK_API OriginRequestAuthCodeSync(OriginUserT user, const OriginCharT *clientId, OriginHandleT *authcode, OriginSizeT *length, const OriginCharT *scope = NULL, const bool appendAuthSource = false);

/// \ingroup sdk
/// \brief Requests an authcode asynchronously.
///
/// Requests an authcode from the Origin Server identifying the user. The reply will be provided through the callback function.
/// \param user This should be the default user, because we cannot issue tickets for 'other' users.
/// \param clientId The client Id the AuthCode should be generated for.
/// \param callback The callback function that will be called when the authcode information is received.
/// \param pcontext The user provided context to the callback system. This context can contain anything. Most of the time it will be a pointer to a handler class.
/// \param[in] timeout How long you want to wait for the reply to come back. Time in milliseconds.
/// \param scope [optional] Scope used to generate the authcode. This optional param is NULL if not specified and will use the default scope.
/// \param appendAuthSource [optional] Whether to append the Authentication source to the AuthCode request or not. This optional param is false if not specified.
/// \note For Blaze integrators: DIGITAL goods project ID should be used when setting up titledefines.cfg. Do NOT use the packaged goods project Id.
/// https://developer.origin.com/documentation/display/blaze/Login+And+Account+Creation+Flows#LoginAndAccountCreationFlows-ConfiguringPROJECTIDandManagingONLINEACCESSEntitlements
OriginErrorT ORIGIN_SDK_API OriginRequestAuthCode(OriginUserT user, const OriginCharT *clientId, OriginAuthCodeCallback callback, void * pcontext, uint32_t timeout, const OriginCharT *scope = NULL, const bool appendAuthSource = false);

/// \ingroup sdk
/// \brief Get a specific client setting asynchronously
///
/// \param[in] settingId enumerated value identifying the setting item we are reading from the client
/// \param[out] pSetting user allocated string of size length to contain the setting of the Origin Client
/// \param[in, out] length the length of the string. (Make the string at least 16 characters long.)
/// \return Returns \ref ORIGIN_SUCCESS if the request succeeded. Returns \ref ORIGIN_ERROR_BUFFER_TOO_SMALL if there is not enough space in the provided buffer.
OriginErrorT ORIGIN_SDK_API OriginGetSetting(enumSettings settingId, OriginSettingCallback callback, void* pContext);

/// \ingroup sdk
/// \brief Get the client settings synchronously
///
/// \param[in] settingId enumerated value identifying the setting item we are reading from the client
/// \param[out] pSetting user allocated string of size length to contain the setting of the Origin Client
/// \param[in, out] length Pass in the size of the buffer provided. Will return the actual result size, or the size needed when the buffer is too small. 
/// \return Returns \ref ORIGIN_SUCCESS if the request succeeded. Returns \ref ORIGIN_ERROR_BUFFER_TOO_SMALL if there is not enough space in the provided buffer.
OriginErrorT ORIGIN_SDK_API OriginGetSettingSync(enumSettings settingId, OriginCharT *pSetting, OriginSizeT *length);

/// \ingroup sdk
/// \brief Get settings asynchronously
///
/// \param[in] callback The callback function that will be called when the info is recieved.
/// \param[in] pContext The user provided context to the callback system. This context can contain anything. Most of the time it will be a pointer to a handler class.
OriginErrorT ORIGIN_SDK_API OriginGetSettings(OriginSettingsCallback callback, void* pContext);

/// \ingroup sdk
/// \brief Get settings synchronously
///
/// \param[out] settings The settings to get back from the client
/// \param[out] pHandle The handle associated with this resource. Call OriginDestroyHandle() when you are finished with it.
OriginErrorT ORIGIN_SDK_API OriginGetSettingsSync(OriginSettingsT* info, OriginHandleT* handle);

/// \ingroup sdk
/// \brief Get a specific game info asynchronously
///
/// \param[in] gameInfoId enumerated value identifying the game info item we are reading from the client
/// \param[in] callback The callback function that will be called when the info is recieved.
/// \param[in] pContext The user provided context to the callback system. This context can contain anything. Most of the time it will be a pointer to a handler class.
OriginErrorT ORIGIN_SDK_API OriginGetGameInfo(enumGameInfo gameInfoId, OriginSettingCallback callback, void* pContext);

/// \ingroup sdk
/// \brief Get a specific game info synchronously
///
/// \param[in] gameInfoId enumerated value identifying the game info item we are reading from the client
/// \param[out] pInfo user allocated string of size length to contain the game info
/// \param[in, out] length  Pass in the size of the buffer provided. Will return the actual result size, or the size needed when the buffer is too small. 
/// \return Returns \ref ORIGIN_SUCCESS if the request succeeded. Returns \ref ORIGIN_ERROR_BUFFER_TOO_SMALL if there is not enough space in the provided buffer.
OriginErrorT ORIGIN_SDK_API OriginGetGameInfoSync(enumGameInfo gameInfoId, OriginCharT *pInfo, OriginSizeT *length);

/// \ingroup sdk
/// \brief Get all game info asynchronously
///
/// \param[in] callback The callback function that will be called when the info is received.
/// \param[in] pContext The user provided context to the callback system. This context can contain anything. Most of the time it will be a pointer to a handler class.
OriginErrorT ORIGIN_SDK_API OriginGetAllGameInfo(OriginGameInfoCallback callback, void* pContext);

/// \ingroup sdk
/// \brief Get all game info synchronously
///
/// \param[out] info The info to get back from the client
/// \param[out] pHandle The handle associated with this resource. Call OriginDestroyHandle() when you are finished with it.
OriginErrorT ORIGIN_SDK_API OriginGetAllGameInfoSync(OriginGameInfoT* info, OriginHandleT* handle);

/// \ingroup sdk
/// \brief Requests an image asynchronously.
/// Most of the user identifying structures like \ref OriginProfileT, \ref OriginFriendT contain an ImageId. This can be used to query the cached location of the 40x40 image for
/// that user. You can also request images for other users by constructing an imageId that has the following format: "user:12270853235" and use that to query the avatar image of the following
/// sizes: 40x40, 208x208, 416x416.
/// The OriginQueryImage API will return all the available cached image sizes for that particular user, and if the requested size was not available the response code will be ORIGIN_PENDING.
/// When you receive an ORIGIN_PENDING code it will be followed up by an ORIGIN_EVENT_PROFILE event to indicate that the requested image is available. (See \ref OriginProfileChangeT)
/// \param[in] pImageId The identifier for the image belonging to game title.
/// \param[in] width The required width of the image.
/// \param[in] height The required height of the image.
/// \param[in] callback The callback function that will be called when the image is received.
/// \param[in] pContext The user provided context to the callback system. This context can contain anything. Most of the time it will be a pointer to a handler class.
/// \param[in] timeout How long you want to wait for the resource. Time in milliseconds.
/// \param[out] pHandle The enumeration handle associated with this enumeration. Call OriginDestroyHandle() when you are finished with it. This will also release the image bits in the returned OriginImageT structure.
OriginErrorT ORIGIN_SDK_API OriginQueryImage(const OriginCharT *pImageId, uint32_t width, uint32_t height, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup sdk
/// \brief Requests an image synchronously.
/// Most of the user identifying structures like \ref OriginProfileT, \ref OriginFriendT contain an ImageId. This can be used to query the cached location of the 40x40 image for
/// that user. You can also request images for other users by constructing an imageId that has the following format: "user:12270853235" and use that to query the avatar image of the following
/// sizes: 40x40, 208x208, 416x416.
/// The OriginQueryImage API will return all the available cached image sizes for that particular user, and if the requested size was not available the response code will be ORIGIN_PENDING.
/// When you receive an ORIGIN_PENDING code it will be followed up by an ORIGIN_EVENT_PROFILE event to indicate that the requested image is available. (See \ref OriginProfileChangeT)
/// \param[in] pImageId The identifier for the image belonging to game title.
/// \param[in] width The required width of the image.
/// \param[in] height The required height of the image.
/// \param[out] pTotal The number of images returned.
/// \param[in] timeout How long you want to wait for the resource, in milliseconds.
/// \param[out] pHandle The enumeration handle associated with this enumeration. Call OriginDestroyHandle() when you are finished with it. This will also release the image bits in the returned OriginImageT structure.
OriginErrorT ORIGIN_SDK_API OriginQueryImageSync(const OriginCharT *pImageId, uint32_t width, uint32_t height, OriginSizeT * pTotal, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup sdk
/// \brief Checks whether Origin is online.
///
/// This will currently only check whether the internet connection is enabled.
/// \param[out] online Receives the online status, if no error occurred.
/// \return \ref ORIGIN_SUCCESS if the check completed successfully, otherwise an error indicates what went wrong.
OriginErrorT ORIGIN_SDK_API OriginCheckOnline(int8_t *online);

/// \ingroup sdk
/// \brief Put Origin in a online state if possible.
///
/// Calling this function will try to put Origin in an online state. 
/// \note Only call this function after UI confirmation from the user. Calling this function without confirmation from the user is an ORC violation
/// \param[in] timeout How long to want for a response in milliseconds.
/// \param[in] callback The callback to be called when the Go Online command has completed/failed. Can be NULL.
/// \param[in] context An application provided context. Can be NULL.
/// \return \ref ORIGIN_SUCCESS when Origin is back online. When Origin is back online it will refresh the entitlements etc. Hookin to the content event for your contentId to see whether your game needs updating.
/// The game will be notified when origin is back online.
/// Possible error conditions: ORIGIN_ERROR_NO_NETWORK, ORIGIN_ERROR_NO_SERVICE, ORIGIN_ERROR_NOT_LOGGED_IN, ORIGIN_ERROR_MANDATORY_ORIGIN_UPDATE_PENDING, ORIGIN_ERROR_ACCOUNT_IN_USE.
OriginErrorT ORIGIN_SDK_API OriginGoOnline(uint32_t timeout, OriginErrorSuccessCallback callback, void * context);

/// \ingroup sdk
/// \brief Get UTC from Origin.
/// This time is based on the server UTC time, and is only accurate when Origin is online. In cases where Origin Starts offline the local machine time is used.
/// \param[out] utctime The utc time.
OriginErrorT ORIGIN_SDK_API OriginGetUTCTime(OriginTimeT * utctime);

/// \ingroup sdk
/// \brief Convert an EAID to UserId
///
/// This function will convert an EAID to a UserId, and the enumeration returns a list OriginFriendT structures.
/// \param[in] pIdentifier A EAID of the user you want the UserId of.
/// \param[out] pTotalCount The total number of records that will be returned.
/// \param[in] timeout How long you want to wait for the resource, in milliseconds.
/// \param[out] pHandle The enumeration handle associated with this enumeration. Call OriginDestroyHandle() when you are finished with it.
OriginErrorT ORIGIN_SDK_API OriginQueryUserIdSync(const OriginCharT *pIdentifier, OriginSizeT * pTotalCount, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup sdk
/// \brief Convert an EAID to UserId
///
/// This function will convert an EAID to a UserId, and the enumeration returns a list OriginFriendT structures.
/// \param[in] pIdentifier A EAID of the user you want the UserId of.
/// \param[in] callback The callback function that will be called when the image is received.
/// \param[in] pContext The user provided context to the callback system. This context can contain anything. Most of the time it will be a pointer to a handler class.
/// \param[in] timeout How long you want to wait for the resource, in milliseconds.
/// \param[out] pHandle The optional enumeration handle associated with this enumeration. It can be NULL. Call OriginDestroyHandle() when you are finished with it.
OriginErrorT ORIGIN_SDK_API OriginQueryUserId(const OriginCharT *pIdentifier, OriginEnumerationCallbackFunc callback, void* pContext, uint32_t timeout, OriginHandleT *pHandle);

/// \ingroup sdk
/// \brief Start a Twitch broadcast
///
/// Calling this function will try to start a Twitch broadcast. If the credentials are not set up, it will bring up the Twitch login window.
/// \return \ref ORIGIN_PENDING is always returned.  Hook in to the broadcast event to check for status of the broadcast.
OriginErrorT ORIGIN_SDK_API OriginBroadcastStart();

/// \ingroup sdk
/// \brief Stop a Twitch broadcast
///
/// Calling this function will try to stop a Twitch broadcast.
/// \return \ref ORIGIN_PENDING is always returned.  Hook in to the broadcast event to check for status of the broadcast.
OriginErrorT ORIGIN_SDK_API OriginBroadcastStop();

/// \ingroup sdk
/// \brief Checks the current broadcast status.
///
/// This will currently only check whether the internet connection is enabled.
/// \param[out] pbStatus Possible status values: ORIGIN_BROADCAST_STARTED, ORIGIN_BROADCAST_START_PENDING, ORIGIN_BROADCAST_STOPPED, ORIGIN_BROADCAST_BLOCKED, ORIGIN_BROADCAST_ACCOUNT_DISCONNECTED.
/// \return \ref ORIGIN_SUCCESS if the check completed successfully, otherwise an error indicates what went wrong.
OriginErrorT ORIGIN_SDK_API OriginBroadcastCheckStatus(int8_t *pbStatus);

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/// \ingroup sdk
/// \brief Logs in through the Origin API. **** NOT IMPLEMENTED, FOR FUTURE EXPANSION ****.
///
/// This is a test function to see whether logging in through the SDK is possible.
/// \param pEmail The email used for registering with Nucleus.
/// \param pPassword The password used when registering with Nucleus.
/// \param[out] ticket A nucleus auth token.
/// \param[out] length The length of the nucleus auth token.
/// \return \ref ORIGIN_SUCCESS is returned if everything is OK.
OriginErrorT ORIGIN_SDK_API OriginLogin(const char *pEmail, const char *pPassword, OriginHandleT *ticket, OriginSizeT *length);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */
                                          
/// \ingroup sdk
/// \brief Broadcast a message to any other Origin connected game. The message will be received by the recipient as an ORIGIN_EVENT_GAME_MESSAGE event. This could potentially be used by games like battlefield to communicate with Sparta.
/// \param gameId A unique id that identifies the sender. The masterTitleId or multiplayerId are good Ids to use here. But you could use something like a GUID as well.
/// \param message The message can be any string. However a json blob or XML message is probably the way to go.
/// \param[in] callback The callback function that will be called when the message is sent.
/// \param[in] pContext The user provided context to the callback system. This context can contain anything. Most of the time it will be a pointer to a handler class.
/// \return ORIGIN_SUCCESS when delivered to one or more recipients (This doesn't mean that the recipient can handle the message.). ORIGIN_ERROR_NOT_FOUND if no other game is connected to Origin.
OriginErrorT ORIGIN_SDK_API OriginSendGameMessage(const OriginCharT * gameId, const OriginCharT * message, OriginErrorSuccessCallback callback, void * pContext);

/// \ingroup sdk
/// \brief Checks whether the user is allowed certain actions.
///
/// This function will check whether the current user is allowed certain actions.
/// \param[in] user This has to be the local user, Use \ref OriginGetDefaultUser.
/// \param[in] perm The permission you want to check.
/// \return \ref ORIGIN_PERMISSION_GRANTED is returned if the action is allowed.
/// <br>\ref ORIGIN_PERMISSION_DENIED is returned when the operation in not allowed.
/// <br>\ref ORIGIN_PERMISSION_GRANTED_FRIENDS_ONLY is returned if the action is between friends only.
/// <br>ORIGIN_ERROR_XXX is returned when an error occurs (see \ref error).
OriginErrorT ORIGIN_SDK_API OriginCheckPermission(OriginUserT user, enumPermission perm);

/// \ingroup sdk
/// \brief Releases any resources associated with the specified handle.
///
/// It will also terminate enumerations associated with this handle.
/// \param[in] handle The handle which is to be released.
/// \return If the handle is destroyed successfully OriginDestroyHandle will return \ref ORIGIN_SUCCESS,
/// otherwise \ref ORIGIN_ERROR_INVALID_HANDLE will be returned.
OriginErrorT ORIGIN_SDK_API OriginDestroyHandle(OriginHandleT handle);

/// \ingroup sdk
/// \brief Registers a user's \ref OriginErrorCallback for reporting errors.
///
/// The callback function may be called at any moment after this function is called (even before it returns), and from
/// any thread context where Origin code is executing. The registration function is not threadsafe; users should call it
/// once prior to initializing Origin, and never again.
/// \param callback The callback function that will be called when an internal error is reported.
/// \param pContext The user context pointer for the callback function. Not used except to pass back to the callback.
void ORIGIN_SDK_API OriginRegisterErrorCallback(OriginErrorCallback callback, void * pContext);

/// \ingroup sdk
/// \brief Sets the amount of debug information you want to receive during the callback.
///
/// \param[in] severity The amount of debug information you want to receive.
/// <br>\ref ORIGIN_LEVEL_NONE = No debug info.
/// <br>\ref ORIGIN_LEVEL_SEVERE = Severe issues only.
/// <br>\ref ORIGIN_LEVEL_MAJOR = Major and Severe issues only.
/// <br>\ref ORIGIN_LEVEL_MINOR = Minor and worse.
/// <br>\ref ORIGIN_LEVEL_EVERYTHING = All debug information.
void ORIGIN_SDK_API OriginEnableDebug(enumDebugLevel severity);

/// \ingroup sdk
/// \brief Registers a debug hook that will be called.
///
/// \param[in] pPrintHook A print hook for debugging information.
/// \param[in] pContext A parameter that is a user-provided context to the print hook.
void ORIGIN_SDK_API OriginHookDebug(PrintfHook pPrintHook, void *pContext);

/// \ingroup sdk
/// \brief Gets more information about the error.
/// \param[in] err The error for which a description is to be provided.
///
const char ORIGIN_SDK_API * OriginGetErrorInfo(OriginErrorT err);

/// \ingroup sdk
/// \brief Provides a textual description of an error code.
/// \param[in] err The error for which a description is to be provided.
///
const char ORIGIN_SDK_API * OriginGetErrorDescription(OriginErrorT err);

#ifdef __cplusplus
}
#endif

#endif //__ORIGINSDK_H__
