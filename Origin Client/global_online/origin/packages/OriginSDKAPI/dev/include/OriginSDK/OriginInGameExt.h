#pragma once

#include "OriginInGame.h"

#pragma pack(push, 1)   // pack structures

/// \ingroup texture resources
/// \brief The callback signature for a graphics callback functions.
///
/// The game defines a callback that matches this signature to provide IGO texture creation capabilities.
/// \param handle The handle associated with this resource.
/// \param width The width in pixels of this 2D texture.
/// \param height The height in pixels of this 2D texture.
/// \param data A pointer to the actual texture data in ARGB8. Can be NULL.
/// \param size The size in bytes that data points to. Can be 0.
/// \return true if call succeeded.
typedef bool(*OriginIGOCreateTextureCallbackFunc)(OriginHandleT* resourceHandle, int32_t width, int32_t height, const uint8_t* data, OriginSizeT size);
 
/// \ingroup texture resources
/// \brief The callback signature for a graphics callback functions.
///
/// The game defines a callback that matches this signature to provide IGO texture update capabilities.
/// \param handle The handle associated with this resource.
/// \param width The width in pixels of this 2D texture.
/// \param height The height in pixels of this 2D texture.
/// \param data A pointer to the actual texture data in ARGB8. Can't be NULL.
/// \param size The size in bytes that data points to. Can't be 0.
/// \return true if call succeeded.
typedef bool(*OriginIGOUpdateTextureCallbackFunc)(OriginHandleT* resourceHandle, int32_t width, int32_t height, const uint8_t* data, OriginSizeT size);

/// \ingroup texture resources
/// \brief The callback signature for a graphics callback functions.
///
/// The game defines a callback that matches this signature to provide IGO texture destory capabilities.
/// \param handle The handle associated with this resource.
/// \return true if call succeeded.
typedef bool(*OriginIGODestroyTextureCallbackFunc)(OriginHandleT* resourceHandle);
 
/// \ingroup vertex resources
/// \brief The callback signature for a graphics callback functions.
///
/// The game defines a callback that matches this signature to provide IGO vertex resource creation capabilities.
/// \param data A pointer to the actual vertex data. Can be NULL.
/// \param sizePerVertex The size of a single vertex in bytes. Can't be 0.
/// \param numVertices The number of vertices for this resource. Can't be 0.
/// \return true if call succeeded.
typedef bool(*OriginIGOCreateVertexCallbackFunc)(OriginHandleT* resourceHandle, const uint8_t* data, OriginSizeT sizePerVertex, uint32_t numVertices);

/// \ingroup vertex resources
/// \brief The callback signature for a graphics callback functions.
///
/// The game defines a callback that matches this signature to provide IGO vertex resource destory capabilities.
/// \param handle The handle associated with this resource.
/// \return true if call succeeded.
typedef bool(*OriginIGODestroyVertexCallbackFunc)(OriginHandleT* resourceHandle);

/// \ingroup types
/// \brief Detailed information about our IGO UI element.
struct OriginIGOUIElementT
{
    OriginSizeT size;                   ///< The size of this data structure, in bytes. Set this member to sizeof( OriginIGOUIElementT ).

    OriginHandleT* vertexResourceHandle; ///< The vertex resource associated with this UI element.
    OriginHandleT* textureResourceHandle;///< The texture resource associated with this UI element.
    uint8_t alpha;                      ///< The alpha value 0..255 of this UI element.
    uint32_t position;                  ///< The absolute position in screen space of this UI element.
    uint32_t width;                     ///< The absolute width in screen space of this UI element.
    uint32_t height;                    ///< The absolute height in screen space of this UI element.

    // additions to those structs have to follow down here, it is forbidden to remove elements, due to size check invalidations!
};
 
/// \ingroup rendering
/// \brief The callback signature for a graphics callback functions.
///
/// The game defines a callback that matches this signature to provide IGO UI element rendering capabilities.
/// \param elements[] A pointer to the actual UI elements data. Can be NULL if numElements is 0.
/// \param numElements The number of UI elements to be rendered. Can't be 0.
/// \note The expected order of rendering elements[] is 0 ... 1+ !
/// \return true if call succeeded.
typedef bool(*OriginIGORenderUIElementCallbackFunc)(const OriginIGOUIElementT* const elements[], uint32_t numElements);
 
/// \ingroup types
/// \brief Detailed information about our IGO frame capturing element.
struct OriginIGOFrameCaptureElementT
{
    __in OriginSizeT size;                   ///< The size of this data structure, in bytes. Set this member to sizeof( OriginIGOFrameCaptureElementT ).

    __in uint32_t requiredWidth;             ///< The required width in screen space of the captured frame data.
    __in uint32_t requiredHeight;            ///< The required height in screen space of the captured frame data.
    __in bool useOriginEncoder;              ///< out_data has to be in YUV format in order to use the high performance Origin video encoder
    __out uint8_t **outData;            ///< The captured data - RGBA8 (R/G/B/1) or YUV8 (Y/U/0/V). IGO::setFrameDestroyCallback will handle resource releasing!

    ///< additions to those structs have to follow down here, it is forbidden to remove elements, due to size check invalidations!
};
 
/// \ingroup capturing
/// \brief The callback signature for a graphics callback functions.
///
/// The game defines a callback that matches this signature to provide IGO frame capturing capabilities.
/// \param element A pointer to the actual captured video data via OriginIGOFrameCaptureElementT.
/// \return true if call succeeded.
typedef bool(*OriginIGOCaptureCallbackFunc)(OriginIGOFrameCaptureElementT* const element);
 
/// \ingroup capturing
/// \brief The callback signature for a graphics callback functions.
///
/// The game defines a callback that matches this signature to provide IGO captured frame destruction capabilities.
/// \param element A pointer to the actual captured video data via OriginIGOFrameCaptureElementT.
/// \return true if call succeeded.
typedef bool(*OriginIGODestroyCaptureCallbackFunc)(OriginIGOFrameCaptureElementT* element);
 
/// \ingroup types
/// \brief Detailed information about our IGO graphics callbacks.
struct OriginIGOGraphicsCallbacksT{
    OriginSizeT size;                      ///< The size of this data structure, in bytes. Set this member to sizeof( OriginIGOUIElementT ).

    OriginIGOCreateTextureCallbackFunc      createTextureCallback;
    OriginIGOUpdateTextureCallbackFunc      updateTextureCallback;
    OriginIGODestroyTextureCallbackFunc     destroyTextureCallback;
    OriginIGOCreateVertexCallbackFunc       createVertexCallback;
    OriginIGODestroyVertexCallbackFunc      destroyVertexCallback;
    OriginIGORenderUIElementCallbackFunc    renderUICallback;
    OriginIGOCaptureCallbackFunc            captureFrameCallback;
    OriginIGODestroyCaptureCallbackFunc     destroyFrameCallback;

    ///< additions to those structs have to follow down here, it is forbidden to remove elements, due to size check invalidations!
};
 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief Call to set all low level callback functions.
/// \param callbacks A integrator provided pointer OriginIGOCallbacksT filled with callbacks. NULL will revert to default IGO callbacks.
/// \return \ref ORIGIN_SUCCESS_IGO if succeeded otherwise ORIGIN_ERROR_IGO_XXX values.
/// \note You have to use all or none callbacks!
OriginErrorT OriginIGOSetGraphicsCallbacks(OriginIGOGraphicsCallbacksT* const callbacks);

#pragma pack(pop)
