#pragma once

#include "Origin.h"

#define OIG_API_MAJOR 1
#define OIG_API_MINOR 1
#define OIG_API_PATCH 0

#if !defined(ORIGIN_MAC)
#include <Windows.h>
#endif

// Get the definitions for our integers. This is supported on MAC, PC, and XBOX
#if !defined(INCLUDED_eabase_H)

#if _MSC_VER>=1600
#include <stdint.h>
#include <stddef.h>
#include <yvals.h>
#elif defined ORIGIN_MAC
#include <stdint.h>
#include <stddef.h>
#else
#define _STDINT
#include "OriginIntTypes.h"
#endif

#endif

#include "OriginBaseTypes.h"

/// \defgroup igoapi IGO_API
/// \brief This module contains the functions for the Origin IGO API.
///
/// For more information on the integration of this feature, see \ref aa-intguideapi-general in the <em>Integrator's
/// Guide</em>.


/// \ingroup enums
/// \brief Defines the available IGO renderers.
enum enumIGORenderer
{
    ORIGIN_IGO_RENDERER_NONE,   ///< no IGO renderer specified - this is an invalid state.
    ORIGIN_IGO_RENDERER_DX8,    ///< DirectX IGO renderer (currently unsupported).
    ORIGIN_IGO_RENDERER_DX9,    ///< DirectX 9 IGO renderer.
    ORIGIN_IGO_RENDERER_DX10,   ///< DirectX 10 IGO renderer.
    ORIGIN_IGO_RENDERER_DX11,   ///< DirectX 11 IGO renderer.
    ORIGIN_IGO_RENDERER_Mantle, ///< Mantle IGO renderer (currently unsupported).
    ORIGIN_IGO_RENDERER_OGL,    ///< OpenGL 3.2+ IGO renderer.
    ORIGIN_IGO_RENDERER_DX12,   ///< DirectX 12 IGO renderer.
    ORIGIN_IGO_RENDERER_CALLBACK    ///< Callback based renderer, rendering code lives inside the game engine.
};

/// \ingroup enums
/// \brief Defines the available commands
enum enumIGOExecutionType
{
    ORIGIN_IGO_RENDER_IGO = 1<<1,      ///< render the IGO.
    ORIGIN_IGO_CAPTURE_VIDEO = 1<<2    ///< capture a frame for DVR.
};


/// \ingroup enums
/// \brief Defines the IGO usage mode.
enum enumIGOMode
{
    ORIGIN_IGO_MODE_DEFAULT,    ///< IGO is running in release mode.
    ORIGIN_IGO_MODE_VALIDATION  ///< IGO is running in validation mode, which is recommended for debug builds to enable more error and warning messages.   
};
 
#define ORIGIN_MAKE_VERSION(major, minor, patch) \
    ((major << 22) | (minor << 12) | patch)
 
#pragma pack(push, 1)   // pack structures
 
/// \ingroup types
/// \brief The asynchronous callback signature for reporting debug/validation/errors encountered by the Origin IGO API.
///
/// A function with this signature can be registered to receive error reports with precise debug/validation/errors  information directly
/// during OriginIGO API code execution.
///
/// The game defines a callback that matches this signature to receive Origin SDK & IGO debug + error messages.
/// \param pcontext The user-provided context to the error callback.  This context can contain anything, it is only used to pass back to the user callback.
/// \param error_code The Origin error code being reported. This code will also generally be returned from the function that was executing.
/// \param message_text Some errors are reported with additional message text.
/// \param file The Origin source file that is reporting the error.
/// \param line The line of the Origin source file that is reporting the error.
typedef void(*OriginIGOErrorDebugCallbackFunc)(void * pcontext, OriginErrorT error_code, const char* message_text, const char* file, int line); ///< test.

/// \ingroup types
/// \brief Detailed information about our IGO mode.
struct OriginIGOModeT
{
    OriginSizeT size;                      ///< The size of this data structure, in bytes. Set this member to sizeof( OriginIGOModeT ).
     
    enumIGOMode usage;                  ///< In ORIGIN_IGO_MODE_VALIDATION mode, IGO will use certain API calls and additional parameters to ensure it's API is used correctly.
#if !defined(ORIGIN_MAC)
    HWND inputWindow;                   ///< message loop window handle
    HWND renderingWindow;               ///< IGO rendering window handle
#else
    void* inputWindow;                  ///< message loop NSWindow pointer
    void* renderingWindow;              ///< IGO rendering NSWindow pointer
#endif
    OriginIGOErrorDebugCallbackFunc errorDebugCallback;   ///< In ORIGIN_IGO_MODE_VALIDATION mode then we need a callback for debug / validation / error messages.

    /// Additions to this structure have to follow down here, it is forbidden to remove elements, due to compatibility reason!
};
 

/// \ingroup control
/// \brief The interface signature for a game-defined memory allocator.
///
/// The game may register a memory allocation routine matching the following signature during the initialization of the API.
/// \param alignment defines the alignment of the base address of the allocated memory. The alignment must be a power of two and multiple of sizeof(void*).
/// \param size the size in bytes of memory to allocate.
/// \return a pointer to the allocated memory.
typedef void* (*OriginIGOAllocAlignedFunc)(OriginSizeT size, OriginSizeT alignment);


/// \ingroup control
/// \brief The interface signature for a game-defined memory de-allocator.
///
/// The game may register a memory de-allocation routine matching the following signature during the initialization of the API.
/// \param ptr pointer to memory address previously allocated by the allocator registered during the initialization of the API.
/// \param size the size in bytes of memory to allocate.
typedef void (*OriginIGOFreeAlignedFunc)(void* ptr);


/// \ingroup types
/// \brief Detailed information about screen and window dimensions
struct OriginIGORenderingResizeT{
    OriginSizeT size;                   ///< The size of this data structure, in bytes. Set this member to sizeof( OriginIGORenderingResizeT ).

    int32_t igoLeft;                ///< The IGO area left in absolute pixels.
    int32_t igoTop;                 ///< The IGO area top in absolute pixels.
    int32_t igoRight;               ///< The IGO area right in absolute pixels.
    int32_t igoBottom;              ///< The IGO area bottom in absolute pixels.
    int32_t screenLeft;             ///< The screen area left in absolute pixels.
    int32_t screenTop;              ///< The screen area topin absolute pixels.
    int32_t screenRight;            ///< The screen area right in absolute pixels.
    int32_t screenBottom;           ///< The screen area bottom in absolute pixels.
    
    /// Additions to this structure have to follow down here, it is forbidden to remove elements, due to compatibility reason!
};


/// \ingroup control
/// \brief The callback signature for a IGO visibility state callback functions.
///
/// The game defines a callback that matches this signature to receive IGO state capabilities.
/// \param visible A pointer to the boolean visibility state of the IGO.
typedef void (*OriginIGOVisibilityCallbackFunc)(bool *visible);


/// \ingroup types
/// \brief Detailed information about our IGO notification callbacks.
/// \note Callbacks set to NULL won't be used.
struct OriginIGONotificationCallbacksT
{
    OriginSizeT size;                               ///< The size of this data structure, in bytes. Set this member to sizeof( OriginIGONotificationCallbacksT ).
    OriginIGOVisibilityCallbackFunc visibilityStateCallback;  ///< [required] This callback will indicate IGO's visibility on screen.

    /// Additions to this structure have to follow down here, it is forbidden to remove elements, due to compatibility reason!
};


/// \ingroup types
/// \brief Detailed information about our IGO initialization
struct OriginIGOContextInitT
{
    OriginSizeT size;                       ///< The size of this data structure, in bytes. Set this member to sizeof( OriginIGOContextInitT ).

    OriginIGORenderingResizeT dimensions;   ///< The initial screen and window dimensions.

    uint32_t apiVersion;                    ///< The API version encoded using ORIGIN_MAKE_VERSION macro.

    OriginIGONotificationCallbacksT callbacks;   ///< A integrator provided OriginIGONotificationCallbacksT filled with notification callbacks.

    OriginIGOModeT mode;                   ///< debug / validation mode parameters
         
    OriginIGOAllocAlignedFunc memAlloc;     ///<[optional] A Callback for aligned memory allocations.
    OriginIGOFreeAlignedFunc memFree;       ///<[optional] A Callback for freeing aligned memory allocations.

    /// Additions to this structure have to follow down here, it is forbidden to remove elements, due to compatibility reason!
};


/// \ingroup types
/// \brief The base context type used by our public API.
struct OriginIGORenderingContextT
{
    OriginSizeT size;                       ///< The size of this data structure, in bytes. Set this member to sizeof( OriginIGORenderingContext_XXX_T ).
    enumIGORenderer rendererType;           ///< The rendering API that will be used.
    enumIGOExecutionType executionType;     ///< The execution that will be done.

    /// Additions to this structure have to follow down here, it is forbidden to remove elements, due to compatibility reason!
};

#ifdef __cplusplus
extern "C"
{
#endif

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \ingroup igoapi
    /// \brief Initialize IGO API.
    /// \param initialize A OriginIGOContextInitT structure to initialize IGO.
    /// \return \ref ORIGIN_SUCCESS_IGOAPI if everything went fine, otherwise ORIGIN_ERROR_IGOAPI_XXX values.
    /// \note This function has to be called to setup the IGO API.
    OriginErrorT ORIGIN_SDK_API OriginIGOInit(OriginIGOContextInitT* const initialize);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \ingroup igoapi
    /// \brief Runs the IGO message loop, captures and encodes the render target's content
    /// for streaming and renders the IGO UI to the back buffer entry of context
    /// \param context Defines GFX context that was specified in OriginIGOInit(...).
    /// \return \ref ORIGIN_SUCCESS_IGOAPI if succeeded otherwise ORIGIN_ERROR_IGOAPI_XXX values.
    /// \note This function has to be called at every frame from the rendering thread to update the IGO and let it run its internal message loop.
    OriginErrorT ORIGIN_SDK_API OriginIGOUpdate(OriginIGORenderingContextT* const context);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \brief Resizes IGO's internal surfaces.
    /// \ingroup igoapi
    /// \param dimensions A pointer to the actual OriginIGORenderingResizeT structure.
    /// \return \ref ORIGIN_SUCCESS_IGOAPI if succeeded otherwise ORIGIN_ERROR_IGOAPI_XXX
    /// \note This function has to be called whenever the render target / capture source / swap chain dimensions are about to change, before OriginIGOReset is called!
    /// screenRight-screenLeft has to match the render target width and screenBottom-screenTop has to match the render target height.
    OriginErrorT ORIGIN_SDK_API OriginIGOResize(OriginIGORenderingResizeT* const dimensions);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \ingroup igoapi
    /// \brief Resets IGO's internal surfaces.
    /// \return \ref ORIGIN_SUCCESS_IGOAPI if succeeded otherwise ORIGIN_ERROR_IGOAPI_XXX values.
    /// \note This function has to be called before you reset or release the actual render device, it will release all graphics memory / objects of the IGO!
    OriginErrorT ORIGIN_SDK_API OriginIGOReset();

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \ingroup igoapi
    /// \brief Shutdown IGO and releases all memory. You need to call OriginIGOInit(...) to use IGO again!
    /// \return \ref ORIGIN_SUCCESS_IGOAPI if succeeded otherwise ORIGIN_ERROR_IGOAPI_XXX
    /// \note This function has to be called before you release the graphics device and before you shut down the Origin SDK via OriginShutdown, otherwise you will leak memory!
    OriginErrorT ORIGIN_SDK_API OriginIGOShutdown();

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// \ingroup igoapi
    /// \brief Transmit events and states to the IGO.
#if !defined(ORIGIN_MAC)
    /// \param lpMsg The window message that OIG can consume(including, but not limited to WM_KEYFIRST - WM_KEYLAST, WM_MOUSEFIRST - WM_MOUSELAST, WM_INPUTLANGCHANGE, WM_SETCURSOR and WM_ACTIVATE).
    /// \note Please tell the IGO if it's specified HWND is in foreground or background by using WM_ACTIVATE message
#else
    /// \param NSEvent The window event that OIG can consume(including, but not limited to NSEventTypeLeftMouseDown, NSEventTypeLeftMouseUp, NSEventTypeRightMouseDown, NSEventTypeRightMouseUp, NSEventTypeMouseMoved, NSEventTypeLeftMouseDragged, NSEventTypeRightMouseDragged, NSEventTypeOtherMouseDown, NSEventTypeOtherMouseUp, NSEventTypeOtherMouseDragged, NSEventTypeScrollWheel, NSEventTypeMouseMoved, NSEventTypeLeftMouseDragged, NSEventTypeRightMouseDragged, NSEventTypeOtherMouseDragged).

#endif
    /// \return true if IGO processed the event, otherwise false so the App should process the message.
    /// \note The game has to block all input APIs while the IGO is visible on screen!
    /// This call has to be called in the main message thread to ensure internal Win32 APIs(SetCursor, ShowCursor) still work!
#if !defined(ORIGIN_MAC)
bool ORIGIN_SDK_API OriginIGOProcessEvents(const LPMSG lpMsg);
#else
bool ORIGIN_SDK_API OriginIGOProcessEvents(void *const NSEvent);
#endif

#ifdef __cplusplus
};
#endif

#pragma pack(pop)
