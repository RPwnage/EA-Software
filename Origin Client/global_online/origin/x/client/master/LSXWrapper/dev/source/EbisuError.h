/// \defgroup error Errors
/// \brief This module contains the defines that provide Origin's error message capabilities. 

#ifndef __EBISU_ERROR_H__
#define __EBISU_ERROR_H__

/// \name Error Areas
/// These defines specify the error areas.
/// @{ 

/// \ingroup error
#define EBISU_ERROR						0xA0000000				///< Add this to your error code to get an error. Bit 29 is set to distinguish Origin error codes from system error codes. 

/// \ingroup error
#define EBISU_WARNING					0x40000000				///< Add this to your warning code to get a warning. Bit 29 is set to distinguish Origin warning codes from system warnings codes.

/// \ingroup error
#define EBISU_ERROR_AREA_GENERAL		0x00000000				///< Add this to your error to get a general error.

/// \ingroup error
#define EBISU_ERROR_AREA_SDK			(1<<16)					///< Add this to your error to get an SDK error.

/// \ingroup error
#define EBISU_ERROR_AREA_CORE			(2<<16)					///< Add this to your error to get a core error.

/// \ingroup error
#define EBISU_ERROR_AREA_IGO			(3<<16)					///< Add this to your error to get an IGO error.

/// \ingroup error
#define EBISU_ERROR_AREA_FRIENDS		(4<<16)					///< Add this to your error to get a friends error.

/// \ingroup error
#define EBISU_ERROR_AREA_PRESENCE		(5<<16)					///< Add this to your error to get a presence error.

/// \ingroup error
#define EBISU_ERROR_AREA_COMMERCE		(6<<16)					///< Add this to your error to get a commerce error.

/// \ingroup error
#define EBISU_ERROR_AREA_ACHIEVEMENTS	(7<<16)					///< Add this to your error to get a achievement error.

/// \ingroup error
#define EBISU_ERROR_AREA_LSX			(8<<16)					///< Add this to your error to get an LSX error.

/// \ingroup error
#define EBISU_ERROR_AREA_PROXY			(9<<16)					///< Add this to your error to get an Origin Proxy error.


/// \ingroup error
#define EBISU_ERROR_LEVEL_SHIFT		24          ///< [A description is required for this define]

/// \ingroup error
#define EBISU_ERROR_LEVEL_MASK		0x0F000000		///< The error level mask.

/// \ingroup error
#define EBISU_LEVEL_0				(0<<24)		///< A severe error.

/// \ingroup error
#define EBISU_LEVEL_1				(1<<24)		///< A major error.

/// \ingroup error
#define EBISU_LEVEL_2				(2<<24)		///< A minor error.

/// \ingroup error
#define EBISU_LEVEL_3				(3<<24)		///< A trivial error.

/// \ingroup error
#define EBISU_LEVEL_4				(4<<24)		///< Every error.

/// @}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/// $errors

#endif /* DOXYGEN_SHOULD_SKIP_THIS */



/// \name Origin Error Codes
/// These defines specify Origin-specific errors.
/// @{

/// \ingroup error
#define EBISU_SUCCESS			0		///< The operation succeeded.

#ifdef ORIGIN_MAC
/// \ingroup error
#define ERROR_SUCCESS           0       ///< The operation succeeded.
#endif

/// \ingroup error
#define EBISU_PENDING			1		///< The operation is still waiting to complete.

// General error codes

/// \ingroup error
#define ERROR_GENERAL			-1		///< An unspecified error has occured. 

/// \ingroup error
#define EBISU_ERROR_INVALID_HANDLE				(EBISU_ERROR + EBISU_LEVEL_1 + EBISU_ERROR_AREA_GENERAL + 0)    ///< The provided handle is invalid.

/// \ingroup error
#define EBISU_ERROR_OUT_OF_MEMORY				(EBISU_ERROR + EBISU_LEVEL_0 + EBISU_ERROR_AREA_GENERAL + 1)    ///< Failed to allocate memory.

/// \ingroup error
#define EBISU_ERROR_NOT_IMPLEMENTED				(EBISU_ERROR + EBISU_LEVEL_1 + EBISU_ERROR_AREA_GENERAL + 2)    ///< The function is not implemented.

/// \ingroup error
#define EBISU_ERROR_INVALID_USER				(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_GENERAL + 3)    ///< The specified user is not valid in this context, or the userId is invalid.

/// \ingroup error
#define EBISU_ERROR_INVALID_ARGUMENT			(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_GENERAL + 4)    ///< One or more arguments are invalid.

/// \ingroup error
#define EBISU_ERROR_NO_CALLBACK_SPECIFIED		(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_GENERAL + 5)	///< The asynchronous operation expected a callback, but no callback was specified.

/// \ingroup error
#define EBISU_ERROR_BUFFER_TOO_SMALL			(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_GENERAL + 6)	///< The provided buffer doesn't have enough space to contain the requested data.

/// \ingroup error
#define EBISU_ERROR_TOO_MANY_VALUES_IN_LIST		(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_GENERAL + 7)	///< We are currently only supporting one item in the list. 

/// \ingroup error
#define EBISU_ERROR_NOT_FOUND					(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_GENERAL + 8)	///< The requested item was not found. 

/// \ingroup error
#define EBISU_ERROR_INVALID_PERSONA			    (EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_GENERAL + 9)    ///< The specified persona is not valid in this context, or the personaId is invalid.

/// \ingroup error
#define EBISU_ERROR_NO_NETWORK                  (EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_GENERAL + 10)	///< No internet connection available. 

/// \ingroup error
#define EBISU_ERROR_NO_SERVICE                  (EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_GENERAL + 11)	///< Origin services are unavailable. 

/// \ingroup error
#define EBISU_ERROR_NOT_LOGGED_IN               (EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_GENERAL + 12)	///< The user isn't logged in. No valid session is available. 

/// \ingroup error
#define EBISU_ERROR_MANDATORY_ORIGIN_UPDATE_PENDING       (EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_GENERAL + 13)	///< There is a mandatory update pending for Origin, this will prevent origin from going online.  

/// \ingroup error
#define EBISU_ERROR_ACCOUNT_IN_USE              (EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_GENERAL + 14)	///< The account is currently in use by another Origin instance. 

/// \ingroup error
#define EBISU_ERROR_AGE_RESTRICTED              (EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_GENERAL + 18)    ///< The item has age restrictions.

/// \ingroup error
#define EBISU_ERROR_BANNED                      (EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_GENERAL + 19)    ///< The user is banned.

/// \ingroup error
#define EBISU_ERROR_NOT_READY                   (EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_GENERAL + 20)	///< The requested action could not be performed, as the item is not ready. 

/// @}

/// \name SDK Error Codes
/// These defines specify Origin SDK-specific errors.
/// @{

// Sdk error codes

/// \ingroup error
#define EBISU_ERROR_SDK_NOT_INITIALIZED				(EBISU_ERROR + EBISU_LEVEL_0 + EBISU_ERROR_AREA_SDK + 0)		///< The Origin SDK was not running. 

/// \ingroup error
#define EBISU_ERROR_SDK_INVALID_ALLOCATOR_DEALLOCATOR_COMBINATION (EBISU_ERROR + EBISU_LEVEL_1 + EBISU_ERROR_AREA_SDK + 1) ///< Make sure that you provide both an allocator and a deallocator.

/// \ingroup error
#define EBISU_ERROR_SDK_IS_RUNNING					(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_SDK + 2)		///< The Origin SDK is running. This operation should only be done before the SDK is initialized or after the SDK is shutdown.

/// \ingroup error
#define EBISU_ERROR_SDK_NOT_ALL_RESOURCES_RELEASED	(EBISU_ERROR + EBISU_LEVEL_3 + EBISU_ERROR_AREA_SDK + 3)		///< The game is still holding on to the resource handles. Call #EbisuDestroyHandle on resources that are no longer needed.

/// \ingroup error
#define EBISU_ERROR_SDK_INVALID_RESOURCE			(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_SDK + 4)		///< The resource in the resource map is invalid. Memory corruption?

/// \ingroup error
#define EBISU_ERROR_SDK_INTERNAL_ERROR				(EBISU_ERROR + EBISU_LEVEL_1 + EBISU_ERROR_AREA_SDK + 5)		///< The SDK experienced an internal error.

/// @}

/// \name SDK Warning Codes
/// These defines specify Origin SDK-specific warnings.
/// @{

/// \ingroup error
#define EBISU_WARNING_SDK_ALREADY_INITIALIZED	(EBISU_WARNING + EBISU_LEVEL_2 + EBISU_ERROR_AREA_SDK + 1)		///< The Origin SDK is already initialized. 

/// \ingroup error
#define EBISU_WARNING_SDK_STILL_RUNNING			(EBISU_WARNING + EBISU_LEVEL_2 + EBISU_ERROR_AREA_SDK + 2)		///< The Origin SDK is still running.

/// \ingroup error
#define EBISU_WARNING_SDK_ENUMERATOR_IN_USE		(EBISU_WARNING + EBISU_LEVEL_2 + EBISU_ERROR_AREA_SDK + 3)		///< The Enumerator associated with the handle was in use.

/// \ingroup error
#define EBISU_WARNING_SDK_ENUMERATOR_TERMINATED		(EBISU_WARNING + EBISU_LEVEL_2 + EBISU_ERROR_AREA_SDK + 4)		///< The Enumerator associated with the handle was not finished.

/// \ingroup error
#define EBISU_ERROR_SDK_INTERNAL_BUFFER_TOO_SMALL  (EBISU_ERROR + EBISU_LEVEL_1 + EBISU_ERROR_AREA_SDK + 6)		///< The internal buffer that the SDK is using is not big enough to receive the response. Inform OriginSDK Support.
/// @}

/// \name Core Error Codes
/// These defines specify Origin Core-specific errors.
/// @{

/// \ingroup error
#define EBISU_ERROR_CORE_NOTLOADED				(EBISU_ERROR + EBISU_LEVEL_0 + EBISU_ERROR_AREA_CORE + 0)		///< The Origin desktop application is not loaded.

/// \ingroup error
#define EBISU_ERROR_CORE_LOGIN_FAILED			(EBISU_ERROR + EBISU_LEVEL_1 + EBISU_ERROR_AREA_CORE + 1)		///< Core couldn't authenticate with the Origin Servers.

/// \ingroup error
#define EBISU_ERROR_CORE_AUTHENTICATION_FAILED  (EBISU_ERROR + EBISU_LEVEL_1 + EBISU_ERROR_AREA_CORE + 2)		///< Origin Core seems to be running, but the LSX Authentication Challenge failed. No communication with Core is possible.

/// \ingroup error
#define EBISU_ERROR_CORE_SEND_FAILED			(EBISU_ERROR + EBISU_LEVEL_1 + EBISU_ERROR_AREA_CORE + 4)		///< Sending data to Core failed.

/// \ingroup error
#define EBISU_ERROR_CORE_RECEIVE_FAILED			(EBISU_ERROR + EBISU_LEVEL_1 + EBISU_ERROR_AREA_CORE + 5)		///< Receiving data from Core failed.

/// \ingroup error
#define EBISU_ERROR_CORE_RESOURCE_NOT_FOUND		(EBISU_ERROR + EBISU_LEVEL_1 + EBISU_ERROR_AREA_CORE + 6)		///< The requested resource could not be located.

/// \ingroup error
#define EBISU_ERROR_CORE_INCOMPATIBLE_VERSION	(EBISU_ERROR + EBISU_LEVEL_0 + EBISU_ERROR_AREA_CORE + 7)		///< The core version is too old to work with this SDK version.

/// \ingroup error
#define EBISU_ERROR_CORE_NOT_INSTALLED			(EBISU_ERROR + EBISU_LEVEL_0 + EBISU_ERROR_AREA_CORE + 8)		///< The Origin installation couldn't be found.

/// @}

/// \name IGO Error and Warning Codes
/// These defines specify In-game Overlay errors and warnings.
/// @{

/// \ingroup error
#define EBISU_WARNING_IGO_NOTLOADED				(EBISU_WARNING + EBISU_LEVEL_1 + EBISU_ERROR_AREA_IGO + 0)	///< The IGO could not be loaded, so SDK functionality is degraded.

/// \ingroup error
#define EBISU_WARNING_IGO_SUPPORT_NOTLOADED		(EBISU_WARNING + EBISU_LEVEL_1 + EBISU_ERROR_AREA_IGO + 1)	///< IGO support is not loaded, so SDK functionality is degraded.

/// \ingroup error
#define EBISU_ERROR_IGO_ILLEGAL_ANCHOR_POINT	(EBISU_ERROR + EBISU_LEVEL_3 + EBISU_ERROR_AREA_IGO + 2)	///< The combination of anchor point bits doesn't resolve to a proper dialog anchor point.

/// \ingroup error
#define EBISU_ERROR_IGO_ILLEGAL_DOCK_POINT		(EBISU_ERROR + EBISU_LEVEL_3 + EBISU_ERROR_AREA_IGO + 3)	///< The combination of dock point bits doesn't resolve to a proper dock point.

/// \ingroup error
#define EBISU_ERROR_IGO_NOT_AVAILABLE			(EBISU_ERROR + EBISU_LEVEL_3 + EBISU_ERROR_AREA_IGO + 4)	///< The IGO is not available.

/// @}

/// \name Presence Error Codes
/// These defines specify Origin Presence errors.
/// @{

/// \ingroup error
#define EBISU_ERROR_NO_MULTIPLAYER_ID           (EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_PRESENCE + 0)   ///< It is not possible to set the presence to JOINABLE when no multiplayer Id is defined on the offer. 

/// @}

/// \name Friends Error Codes
/// These defines specify Origin Friends errors.
/// @{

/// \ingroup error
#define EBISU_ERROR_LSX_INVALID_RESPONSE		(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_LSX + 0)	///< The LSX Decoder didn't expect this response.

/// \ingroup error
#define EBISU_ERROR_LSX_NO_RESPONSE				(EBISU_ERROR + EBISU_LEVEL_1 + EBISU_ERROR_AREA_LSX + 1)	///< The LSX server didn't respond within the set timeout.

/// \ingroup error
#define EBISU_ERROR_LSX_INVALID_REQUEST			(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_LSX + 2)	///< The LSX Decoder didn't expect this request.

/// @}

/// \name Commerce Error Codes
/// These defines specify Origin Commerce errors.
/// @{

/// \ingroup error
#define EBISU_ERROR_COMMERCE_NO_SUCH_STORE		(EBISU_ERROR + EBISU_LEVEL_1 + EBISU_ERROR_AREA_COMMERCE + 0)	///< The store could not be found.

/// \ingroup error
#define EBISU_ERROR_COMMERCE_NO_SUCH_CATALOG	(EBISU_ERROR + EBISU_LEVEL_1 + EBISU_ERROR_AREA_COMMERCE + 1)	///< The catalog could not be found.

/// \ingroup error
#define EBISU_ERROR_COMMERCE_INVALID_REPLY		(EBISU_ERROR + EBISU_LEVEL_1 + EBISU_ERROR_AREA_COMMERCE + 2)	///< The server reply was not understood.

/// \ingroup error
#define EBISU_ERROR_COMMERCE_NO_CATEGORIES		(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_COMMERCE + 3)	///< No categories were found.

/// \ingroup error
#define EBISU_ERROR_COMMERCE_NO_PRODUCTS		(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_COMMERCE + 4)	///< No products were found.

/// \ingroup error
#define EBISU_ERROR_COMMERCE_UNDERAGE_USER		(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_COMMERCE + 5)	///< The user is under age and is blocked to perform this action.

/// @}


/// \name Origin Proxy Error Codes.
/// These defines specify Origin Proxy errors.
/// @{
/// \ingroup error
#define EBISU_ERROR_PROXY						(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_PROXY + 0)	///< Base proxy error. You shouldn't get this error.

/// \ingroup error
#define EBISU_ERROR_PROXY_NO_RESPONSE			(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_PROXY + 204)  ///< Server error: No Response.

/// \ingroup error
#define EBISU_ERROR_PROXY_BAD_REQUEST			(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_PROXY + 400)  ///< Server error: Bad Request

/// \ingroup error
#define EBISU_ERROR_PROXY_UNAUTHORIZED			(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_PROXY + 401)  ///< Server error: Unauthorized. 

/// \ingroup error
#define EBISU_ERROR_PROXY_FORBIDDEN 			(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_PROXY + 403)  ///< Server error: Forbidden.

/// \ingroup error
#define EBISU_ERROR_PROXY_NOT_FOUND				(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_PROXY + 404)  ///< Server error: Not found.

/// \ingroup error
#define EBISU_ERROR_PROXY_INTERNAL_ERROR		(EBISU_ERROR + EBISU_LEVEL_2 + EBISU_ERROR_AREA_PROXY + 500)  ///< Server error: Internal Server Error.

/// @}

#endif //__EBISU_ERROR_H__
