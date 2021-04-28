/// \defgroup error Errors
/// \brief This module contains the defines that provide Origin's error message capabilities. 

#ifndef __ORIGIN_ERROR_H__
#define __ORIGIN_ERROR_H__

/// \name Error Areas
/// These defines specify the error areas.
/// @{ 

/// \ingroup error
#define ORIGIN_ERROR						0xA0000000				///< Add this to your error code to get an error. Bit 29 is set to distinguish Origin error codes from system error codes. 

/// \ingroup error
#define ORIGIN_WARNING					0x40000000				///< Add this to your warning code to get a warning. Bit 29 is set to distinguish Origin warning codes from system warnings codes.

/// \ingroup error
#define ORIGIN_ERROR_AREA_GENERAL		0x00000000				///< Add this to your error to get a general error.

/// \ingroup error
#define ORIGIN_ERROR_AREA_SDK			(1<<16)					///< Add this to your error to get an SDK error.

/// \ingroup error
#define ORIGIN_ERROR_AREA_CORE			(2<<16)					///< Add this to your error to get a core error.

/// \ingroup error
#define ORIGIN_ERROR_AREA_IGO			(3<<16)					///< Add this to your error to get an IGO error.

/// \ingroup error
#define ORIGIN_ERROR_AREA_FRIENDS		(4<<16)					///< Add this to your error to get a friends error.

/// \ingroup error
#define ORIGIN_ERROR_AREA_PRESENCE		(5<<16)					///< Add this to your error to get a presence error.

/// \ingroup error
#define ORIGIN_ERROR_AREA_COMMERCE		(6<<16)					///< Add this to your error to get a commerce error.

/// \ingroup error
#define ORIGIN_ERROR_AREA_ACHIEVEMENTS	(7<<16)					///< Add this to your error to get a achievement error.

/// \ingroup error
#define ORIGIN_ERROR_AREA_LSX			(8<<16)					///< Add this to your error to get an LSX error.

/// \ingroup error
#define ORIGIN_ERROR_AREA_PROXY			(9<<16)					///< Add this to your error to get an Origin Proxy error.

/// \ingroup error
#define ORIGIN_ERROR_AREA_IGOAPI		(10<<16)				///< Add this to your error to get an Origin IGO API error.


/// \ingroup error
#define ORIGIN_ERROR_LEVEL_SHIFT		24          ///< [A description is required for this define]

/// \ingroup error
#define ORIGIN_ERROR_LEVEL_MASK		0x0F000000		///< The error level mask.

/// \ingroup error
#define ORIGIN_LEVEL_0				(0<<24)		///< A severe error.

/// \ingroup error
#define ORIGIN_LEVEL_1				(1<<24)		///< A major error.

/// \ingroup error
#define ORIGIN_LEVEL_2				(2<<24)		///< A minor error.

/// \ingroup error
#define ORIGIN_LEVEL_3				(3<<24)		///< A trivial error.

/// \ingroup error
#define ORIGIN_LEVEL_4				(4<<24)		///< Every error.

/// @}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/// $errors

#endif /* DOXYGEN_SHOULD_SKIP_THIS */



/// \name Origin Error Codes
/// These defines specify Origin-specific errors.
/// @{

/// \ingroup error
#define ORIGIN_SUCCESS			0		///< The operation succeeded.

/// \ingroup error
#define ORIGIN_PENDING			1		///< The operation is still waiting to complete.

// General error codes

/// \ingroup error
#define ORIGIN_ERROR_GENERAL	-1		///< An unspecified error has occured. 

/// \ingroup error
#define ORIGIN_ERROR_INVALID_HANDLE				(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_GENERAL + 0)    ///< The provided handle is invalid.

/// \ingroup error
#define ORIGIN_ERROR_OUT_OF_MEMORY				(ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_GENERAL + 1)    ///< Failed to allocate memory.

/// \ingroup error
#define ORIGIN_ERROR_NOT_IMPLEMENTED			(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_GENERAL + 2)    ///< The function is not implemented.

/// \ingroup error
#define ORIGIN_ERROR_INVALID_USER				(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 3)    ///< The specified user is not valid in this context, or the userId is invalid.

/// \ingroup error
#define ORIGIN_ERROR_INVALID_ARGUMENT			(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 4)    ///< One or more arguments are invalid.

/// \ingroup error
#define ORIGIN_ERROR_NO_CALLBACK_SPECIFIED		(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 5)	///< The asynchronous operation expected a callback, but no callback was specified.

/// \ingroup error
#define ORIGIN_ERROR_BUFFER_TOO_SMALL			(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 6)	///< The provided buffer doesn't have enough space to contain the requested data.

/// \ingroup error
#define ORIGIN_ERROR_TOO_MANY_VALUES_IN_LIST	(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 7)	///< We are currently only supporting one item in the list. 

/// \ingroup error
#define ORIGIN_ERROR_NOT_FOUND					(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 8)	///< The requested item was not found. 

/// \ingroup error
#define ORIGIN_ERROR_INVALID_PERSONA			(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 9)    ///< The specified persona is not valid in this context, or the personaId is invalid.

/// \ingroup error
#define ORIGIN_ERROR_NO_NETWORK                  (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 10)	///< No internet connection available. 

/// \ingroup error
#define ORIGIN_ERROR_NO_SERVICE                  (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 11)	///< Origin services are unavailable. 

/// \ingroup error
#define ORIGIN_ERROR_NOT_LOGGED_IN               (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 12)	///< The user isn't logged in. No valid session is available. 

/// \ingroup error
#define ORIGIN_ERROR_MANDATORY_ORIGIN_UPDATE_PENDING    (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 13)	///< There is a mandatory update pending for Origin, this will prevent origin from going online. 

/// \ingroup error
#define ORIGIN_ERROR_ACCOUNT_IN_USE              (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 14)	///< The account is currently in use by another Origin instance. 

/// \ingroup error
#define ORIGIN_ERROR_TOO_MANY_INSTANCES          (ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_GENERAL + 15)    ///< Too many instances of the OriginSDK created.

/// \ingroup error
#define ORIGIN_ERROR_ALREADY_EXISTS              (ORIGIN_ERROR + ORIGIN_LEVEL_3 + ORIGIN_ERROR_AREA_GENERAL + 16)    ///< The item already exists in the list.

/// \ingroup error
#define ORIGIN_ERROR_INVALID_OPERATION           (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 17)    ///< The requested operation cannot be performed.

/// \ingroup error
#define ORIGIN_ERROR_AGE_RESTRICTED              (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 18)    ///< The item has age restrictions.

/// \ingroup error
#define ORIGIN_ERROR_BANNED                      (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 19)    ///< The user is banned.

/// \ingroup error
#define ORIGIN_ERROR_NOT_READY                   (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_GENERAL + 20)	///< The item is not ready. 

/// @}

/// \name SDK Error Codes
/// These defines specify Origin SDK-specific errors.
/// @{

// Sdk error codes

/// \ingroup error
#define ORIGIN_ERROR_SDK_NOT_INITIALIZED				(ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_SDK + 0)		///< The Origin SDK was not running. 

/// \ingroup error
#define ORIGIN_ERROR_SDK_INVALID_ALLOCATOR_DEALLOCATOR_COMBINATION (ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_SDK + 1) ///< Make sure that you provide both an allocator and a deallocator.

/// \ingroup error
#define ORIGIN_ERROR_SDK_IS_RUNNING					(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_SDK + 2)		///< The Origin SDK is running. This operation should only be done before the SDK is initialized or after the SDK is shutdown.

/// \ingroup error
#define ORIGIN_ERROR_SDK_NOT_ALL_RESOURCES_RELEASED	(ORIGIN_ERROR + ORIGIN_LEVEL_3 + ORIGIN_ERROR_AREA_SDK + 3)		///< The game is still holding on to the resource handles. Call #OriginDestroyHandle on resources that are no longer needed.

/// \ingroup error
#define ORIGIN_ERROR_SDK_INVALID_RESOURCE			(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_SDK + 4)		///< The resource in the resource map is invalid. Memory corruption?

/// \ingroup error
#define ORIGIN_ERROR_SDK_INTERNAL_ERROR				(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_SDK + 5)		///< The SDK experienced an internal error.

/// \ingroup error
#define ORIGIN_ERROR_SDK_INTERNAL_BUFFER_TOO_SMALL  (ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_SDK + 6)		///< The internal buffer that the SDK is using is not big enough to receive the response. Inform OriginSDK Support.

/// @}

/// \name SDK Warning Codes
/// These defines specify Origin SDK-specific warnings.
/// @{

/// \ingroup error
#define ORIGIN_WARNING_SDK_ALREADY_INITIALIZED	(ORIGIN_WARNING + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_SDK + 1)		///< The Origin SDK is already initialized. 

/// \ingroup error
#define ORIGIN_WARNING_SDK_STILL_RUNNING			(ORIGIN_WARNING + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_SDK + 2)		///< The Origin SDK is still running.

/// \ingroup error
#define ORIGIN_WARNING_SDK_ENUMERATOR_IN_USE		(ORIGIN_WARNING + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_SDK + 3)		///< The Enumerator associated with the handle was in use.

/// \ingroup error
#define ORIGIN_WARNING_SDK_ENUMERATOR_TERMINATED		(ORIGIN_WARNING + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_SDK + 4)		///< The Enumerator associated with the handle was not finished.

/// @}

/// \name Core Error Codes
/// These defines specify Origin Core-specific errors.
/// @{

/// \ingroup error
#define ORIGIN_ERROR_CORE_NOTLOADED				(ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_CORE + 0)		///< The Origin desktop application is not loaded.

/// \ingroup error
#define ORIGIN_ERROR_CORE_LOGIN_FAILED			(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_CORE + 1)		///< Origin couldn't authenticate with the Origin Servers.

/// \ingroup error
#define ORIGIN_ERROR_CORE_AUTHENTICATION_FAILED  (ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_CORE + 2)		///< Origin seems to be running, but the LSX Authentication Challenge failed. No communication with Core is possible.

/// \ingroup error
#define ORIGIN_ERROR_CORE_SEND_FAILED			(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_CORE + 4)		///< Sending data to Origin failed.

/// \ingroup error
#define ORIGIN_ERROR_CORE_RECEIVE_FAILED			(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_CORE + 5)		///< Receiving data from Origin failed.

/// \ingroup error
#define ORIGIN_ERROR_CORE_RESOURCE_NOT_FOUND		(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_CORE + 6)		///< The requested resource could not be located.

/// \ingroup error
#define ORIGIN_ERROR_CORE_INCOMPATIBLE_VERSION	(ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_CORE + 7)		///< The Origin version is too old to work with this SDK version.

/// \ingroup error
#define ORIGIN_ERROR_CORE_NOT_INSTALLED			(ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_CORE + 8)		///< The Origin installation couldn't be found.

/// @}

/// \name IGO Error and Warning Codes
/// These defines specify In-game Overlay errors and warnings.
/// @{

/// \ingroup error
#define ORIGIN_WARNING_IGO_NOTLOADED				(ORIGIN_WARNING + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_IGO + 0)	///< The IGO could not be loaded, so SDK functionality is degraded.

/// \ingroup error
#define ORIGIN_WARNING_IGO_SUPPORT_NOTLOADED		(ORIGIN_WARNING + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_IGO + 1)	///< IGO support is not loaded, so SDK functionality is degraded.

/// \ingroup error
#define ORIGIN_ERROR_IGO_ILLEGAL_ANCHOR_POINT	(ORIGIN_ERROR + ORIGIN_LEVEL_3 + ORIGIN_ERROR_AREA_IGO + 2)	///< The combination of anchor point bits doesn't resolve to a proper dialog anchor point.

/// \ingroup error
#define ORIGIN_ERROR_IGO_ILLEGAL_DOCK_POINT		(ORIGIN_ERROR + ORIGIN_LEVEL_3 + ORIGIN_ERROR_AREA_IGO + 3)	///< The combination of dock point bits doesn't resolve to a proper dock point.

/// \ingroup error
#define ORIGIN_ERROR_IGO_NOT_AVAILABLE			(ORIGIN_ERROR + ORIGIN_LEVEL_3 + ORIGIN_ERROR_AREA_IGO + 4)	///< The IGO is not available.

/// @}

/// \name Origin IGO API Error and Warning Codes
/// These defines specify In-game Overlay API errors and warnings.
/// @{


/// \ingroup error
#define ORIGIN_SUCCESS_IGOAPI       0  ///< The operation succeeded.

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_GENERAL       -1  ///< An unspecified error has occured.

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_RENDERING_CONTEXT_NOT_AVAILABLE		(ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 0)	///< The IGO API context is not available.

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_RENDERING_CONTEXT_SIZE_MISMATCH	(ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 1)	///< The IGO API context has the wrong size. SDK version mismatch?

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_RENDERING_CONTEXT_UNKNOWN		(ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 2)	///< The IGO API context is unknown. SDK version mismatch?

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_INIT_CONTEXT_NOT_AVAILABLE		(ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 3)	///< The IGO API init context is not available.

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_INIT_CONTEXT_SIZE_MISMATCH     (ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 4)	///< The IGO API init context has the wrong size. SDK version mismatch?

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_VERSION_MISMATCH     (ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 5)	///< The IGO API and the used API do not match!

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_RENDERING_CONTEXT_SWITCHED     (ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 6)	///< The IGO API rendering context has been changed without calling OriginIGOReset or OriginIGOShutdown.

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_INVALID_ALLOCATOR_DEALLOCATOR_COMBINATION     (ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 7)	///< Make sure that you provide both an allocator and a deallocator.

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_ACTIVE_ALLOCATOR_DEALLOCATOR_COMBINATION     (ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 8)	///< Make sure you have deallocated all resources before switching memory management functions.

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_MEMORY_LEAK_DETECTED     (ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 9)	///< IGO API memory leaks detected.

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_INVALID_SCREEN_DIMENSIONS     (ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 10)	///< Rendering screen size is invalid.

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_INIT_CONTEXT_NOT_COMPLETE		(ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 11)	///< The IGO API init context is not complete.

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_RESIZE_SIZE_MISMATCH     (ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 12)	///< The IGO API resize structure does not fit. SDK version mismatch?

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_MODE_STUCT_NOT_COMPLETE   (ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 13)	///< The IGO API mode structure is not complete.

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_MODE_STUCT_SIZE_MISMATCH   (ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 14)	///< The IGO API mode structure does not fit. SDK version mismatch?

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_NOTLOADED   (ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 15)	///< The IGO API could not be loaded!

/// \ingroup warning
#define ORIGIN_WARNING_IGOAPI_NOT_ALL_RESOURCES_RELEASED    (ORIGIN_WARNING + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 16)	///< The IGO API is still in use or has memory allocations! Call OriginIGOShutdown before shutting down the graphics context and calling OriginShutdown !

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_HOOKING_ALREADY_USED    (ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 17)	///< The IGO API can't be used, because IGO is configured to use the old API hooking! Configure the product properly or use the override ( eacore.ini OverrideIGOAPISetting::NOG_XXXXXXXXXX )

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_CALLBACKS_NOT_COMPLETE    (ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 18)	///< The IGO API callbacks are not initialized!

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_DX_ERROR    (ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 19)	///< The IGO API encountered and DirectX error.

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_INVALID_WINDOW_HANDLE     (ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 20)	///< A provided window handle is invalid.

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_CONTEXT_MODE_INVALID		(ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 21)	///< The IGO API context is invalid. Probably mixing immediate and queued rendering mode.

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_STREAMING_PROPERTIES_MISMATCH		(ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 22)	///< Streaming is not working due to wrong streaming properties provided by the game.

/// \ingroup error
#define ORIGIN_ERROR_IGOAPI_STREAMING_NOT_SUPPORTED		(ORIGIN_ERROR + ORIGIN_LEVEL_0 + ORIGIN_ERROR_AREA_IGOAPI + 23)	///< Streaming is not supported by the game.

/// @}

/// \name Presence Error Codes
/// These defines specify Origin Presence errors.
/// @{

/// \ingroup error
#define ORIGIN_ERROR_NO_MULTIPLAYER_ID           (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PRESENCE + 0)   ///< It is not possible to set the presence to JOINABLE when no multiplayer Id is defined on the offer. 

/// @}

/// \name Friends Error Codes
/// These defines specify Origin Friends errors.
/// @{

/// \ingroup error
#define ORIGIN_ERROR_LSX_INVALID_RESPONSE		(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_LSX + 0)	///< The LSX Decoder didn't expect this response.

/// \ingroup error
#define ORIGIN_ERROR_LSX_NO_RESPONSE				(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_LSX + 1)	///< The LSX server didn't respond within the set timeout.

/// \ingroup error
#define ORIGIN_ERROR_LSX_INVALID_REQUEST			(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_LSX + 2)	///< The LSX Decoder didn't expect this request.

/// @}

/// \name Commerce Error Codes
/// These defines specify Origin Commerce errors.
/// @{

/// \ingroup error
#define ORIGIN_ERROR_COMMERCE_NO_SUCH_STORE		(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_COMMERCE + 0)	///< The store could not be found.

/// \ingroup error
#define ORIGIN_ERROR_COMMERCE_NO_SUCH_CATALOG	(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_COMMERCE + 1)	///< The catalog could not be found.

/// \ingroup error
#define ORIGIN_ERROR_COMMERCE_INVALID_REPLY		(ORIGIN_ERROR + ORIGIN_LEVEL_1 + ORIGIN_ERROR_AREA_COMMERCE + 2)	///< The server reply was not understood.

/// \ingroup error
#define ORIGIN_ERROR_COMMERCE_NO_CATEGORIES		(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_COMMERCE + 3)	///< No categories were found.

/// \ingroup error
#define ORIGIN_ERROR_COMMERCE_NO_PRODUCTS		(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_COMMERCE + 4)	///< No products were found.

/// @}


/// \name Origin Proxy Error Codes.
/// These defines specify Origin Proxy errors.
/// @{
/// \ingroup error
#define ORIGIN_ERROR_PROXY						(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 0)	///< Base proxy error. You shouldn't get this error.

/// \ingroup error
#define ORIGIN_SUCCESS_PROXY_OK			        (ORIGIN_WARNING + ORIGIN_LEVEL_4 + ORIGIN_ERROR_AREA_PROXY + 200)  ///< Server success: OK.

/// \ingroup error
#define ORIGIN_SUCCESS_PROXY_CREATED			(ORIGIN_WARNING + ORIGIN_LEVEL_4 + ORIGIN_ERROR_AREA_PROXY + 201)  ///< Server success: Created.

/// \ingroup error
#define ORIGIN_SUCCESS_PROXY_ACCEPTED			(ORIGIN_WARNING + ORIGIN_LEVEL_4 + ORIGIN_ERROR_AREA_PROXY + 202)  ///< Server success: Accepted.

/// \ingroup error
#define ORIGIN_SUCCESS_PROXY_NON_AUTH_INFO		(ORIGIN_WARNING + ORIGIN_LEVEL_4 + ORIGIN_ERROR_AREA_PROXY + 203)  ///< Server success: Non-Authoritative Information.

/// \ingroup error
#define ORIGIN_SUCCESS_PROXY_NO_CONTENT			(ORIGIN_WARNING + ORIGIN_LEVEL_4 + ORIGIN_ERROR_AREA_PROXY + 204)  ///< Server success: No Content.

/// \ingroup error
#define ORIGIN_SUCCESS_RESET_CONTENT	        (ORIGIN_WARNING + ORIGIN_LEVEL_4 + ORIGIN_ERROR_AREA_PROXY + 205)  ///< Server success: Reset Content.

/// \ingroup error
#define ORIGIN_SUCCESS_PARTIAL_CONTENT		    (ORIGIN_WARNING + ORIGIN_LEVEL_4 + ORIGIN_ERROR_AREA_PROXY + 206)  ///< Server success: Partial Content.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_BAD_REQUEST			(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 400)  ///< Server error: Bad Request

/// \ingroup error
#define ORIGIN_ERROR_PROXY_UNAUTHORIZED			(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 401)  ///< Server error: Unauthorized. 

/// \ingroup error
#define ORIGIN_ERROR_PROXY_PAYMENT_REQUIRED		(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 402)  ///< Server error: Payment Required. 

/// \ingroup error
#define ORIGIN_ERROR_PROXY_FORBIDDEN 			(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 403)  ///< Server error: Forbidden.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_NOT_FOUND				(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 404)  ///< Server error: Not found.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_METHOD_NOT_ALLOWED	(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 405)  ///< Server error: Method not Allowed.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_NOT_ACCEPTABLE	    (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 406)  ///< Server error: Not Acceptable.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_REQUEST_TIMEOUT	    (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 408)  ///< Server error: Request Timeout.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_CONFLICT	            (ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 409)  ///< Server error: Conflict.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_INTERNAL_ERROR		(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 500)  ///< Server error: Internal Server Error.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_NOT_IMPLEMENTED		(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 501)  ///< Server error: Not Implemented.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_BAD_GATEWAY	(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 502)  ///< Server error: Bad Gateway.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_SERVICE_UNAVAILABLE	(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 503)  ///< Server error: Service Unavailable.

/// \ingroup error
#define ORIGIN_ERROR_PROXY_GATEWAY_TIMEOUT	(ORIGIN_ERROR + ORIGIN_LEVEL_2 + ORIGIN_ERROR_AREA_PROXY + 504)  ///< Server error: Gateway Timeout.


/// @}

#endif //__ORIGIN_ERROR_H__
