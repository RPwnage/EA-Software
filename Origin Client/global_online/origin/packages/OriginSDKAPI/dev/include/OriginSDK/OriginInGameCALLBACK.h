#pragma once

#include "OriginInGame.h"

#pragma pack(push, 1)   // pack structures

/// \ingroup enums
/// \brief Defines types of IGO rendering shader effects.
enum enumOriginIGOShader {
    ORIGINIGOSHADER_NONE = 0,
    ORIGINIGOSHADER_BACKGROUND = 1
};

enum enumOriginIGOTextureID {
    ORIGINIGOTEXTUREID_INVALID = -1 // invalid ID
};

/// \ingroup enums
/// \brief Defines types of IGO streaming formates
enum enumOriginIGOStreamFormat {
    ORIGINIGOSTREAMFORMAT_RGBA8888 = 0,
    ORIGINIGOSTREAMFORMAT_BGRA8888 = 1,
    ORIGINIGOSTREAMFORMAT_YUV444 = 2,

    // not yet supported by the IGO (05/14/2017)
    ORIGINIGOSTREAMFORMAT_RGB101010X2 = 3,
    ORIGINIGOSTREAMFORMAT_BGR101010X2 = 4,
    ORIGINIGOSTREAMFORMAT_H264 = 5,
    ORIGINIGOSTREAMFORMAT_H265 = 6,
};

#define ORIGIN_SDK_API_MISMATCH "Origin SDK / OIG API version missmatch. Structures do have a different size!"

/// \ingroup types
/// \brief Detailed information about 2d texture
struct OriginIGORenderingTexture2D_T {
    OriginSizeT size;               ///< The size of this data structure, in bytes. Set this member to sizeof( OriginIGORenderingTexture2D_T ).

    int32_t texID;
    uint32_t width;
    uint32_t height;
    size_t dataSize;
    const char* data;

    /// Additions to this structure have to follow down here, it is forbidden to remove elements, due to compatibility reason!
};


/// \ingroup types
/// \brief Detailed information about the quad
struct OriginIGORenderingQuad_T {
    OriginSizeT size;               ///< The size of this data structure, in bytes. Set this member to sizeof( OriginIGORenderingQuad_T ).

    int32_t texID;
    float x;
    float y;
    float width;
    float height;
    float alpha;
    float effectParam;
    enumOriginIGOShader effect;

    /// Additions to this structure have to follow down here, it is forbidden to remove elements, due to compatibility reason!
};


/// \ingroup types
/// \brief Detailed information about the streaming properties - This structure is used for bidirectional communication!
struct OriginIGOStreamingInfo_T {
    OriginSizeT size;               ///< The size of this data structure, in bytes. Set this member to sizeof( OriginIGOStreamingInfo_T ).

                                    ///< Used for input properties
    float fps;
    uint32_t width;
    uint32_t height;
    bool streaming;
    enumOriginIGOStreamFormat format;

    ///< Used for output properties
    uint64_t frameTimeStamp;
    bool matchRequestedProperties;  ///< Did the requested frame properties match what we supply by the callback?

                                    ///< For future update to support encoded frames
    bool isKeyFrame;
    int maxKiloBitPerSecond;
    int qualityLevel;

    ///< For data transfer - managed by the IGO!
    size_t dataSize;
    char* data;

    /// Additions to this structure have to follow down here, it is forbidden to remove elements, due to compatibility reason!
};



/// \ingroup igoapi
/// \interface IOriginIGORenderer
/// \brief The interface for the origin overlay renderer. It should be "interface", but doxygen only knows "class" and "struct".
///
struct IOriginIGORenderer {
public:

    /// \fn virtual OriginErrorT beginFrame() = 0
    /// \brief The synchronous call for begin a frame.
    ///
    /// A function with this signature is used to indicate the start of rendering.
    /// \return \ref ORIGIN_SUCCESS_IGOAPI if succeeded otherwise ORIGIN_ERROR_IGOAPI_XXX
    virtual OriginErrorT beginFrame() = 0;

    /// \ingroup igo_renderer
    /// \brief The synchronous call for ending a frame.
    ///
    /// A function with this signature is used to indicate the end of rendering.
    /// \return \ref ORIGIN_SUCCESS_IGOAPI if succeeded otherwise ORIGIN_ERROR_IGOAPI_XXX
    virtual OriginErrorT endFrame() = 0;

    /// \ingroup igo_renderer
    /// \brief The synchronous call for creating a texture.
    ///
    /// A function with this signature is used to create a 2d textures.
    /// \param input The user-provided input properties.
    /// \param output_textureID The id of the created texture.
    /// \return The id of the new texture, otherwise \ref enumOriginIGOTextureID::ORIGINIGOTEXTUREID_INVALID
    virtual int32_t createTexture2D(const OriginIGORenderingTexture2D_T* input) = 0;

    /// \ingroup igo_renderer
    /// \brief The synchronous call for updating a texture.
    ///
    /// A function with this signature is used to update a 2d textures.
    /// \param input The user-provided input properties. It requires a valid texture id (returned from createTexture2D).
    /// \return \ref ORIGIN_SUCCESS_IGOAPI if succeeded otherwise ORIGIN_ERROR_IGOAPI_XXX
    virtual OriginErrorT updateTexture2D(const OriginIGORenderingTexture2D_T* input) = 0;

    /// \ingroup igo_renderer
    /// \brief The synchronous call for deleting a texture.
    ///
    /// A function with this signature is used to delete a 2d textures.
    /// \param input_textureID The user-provided texture id.
    /// \return \ref ORIGIN_SUCCESS_IGOAPI if succeeded otherwise ORIGIN_ERROR_IGOAPI_XXX
    virtual OriginErrorT deleteTexture2D(int32_t inputTextureID) = 0;

    /// \ingroup igo_renderer
    /// \brief The synchronous call for rendering a quad.
    ///
    /// A function with this signature is used to render a quad.
    /// \param input The user-provided input properties.
    /// \return \ref ORIGIN_SUCCESS_IGOAPI if succeeded otherwise ORIGIN_ERROR_IGOAPI_XXX
    virtual OriginErrorT renderQuad(const OriginIGORenderingQuad_T* input) = 0;

    /// \ingroup igo_renderer
    /// \brief The synchronous call for setting streaming properties.
    ///
    /// A function with this signature is used to setup video streaming.
    /// \param input The user-provided input properties.
    /// \return \ref ORIGIN_SUCCESS_IGOAPI if succeeded otherwise ORIGIN_ERROR_IGOAPI_XXX
    /// \note \ref ORIGIN_ERROR_IGOAPI_STREAMING_NOT_SUPPORTED indicates that streaming is not provided by the game!
    virtual OriginErrorT streamingSetup(const OriginIGOStreamingInfo_T* input) = 0;

    /// \ingroup igo_renderer
    /// \brief The synchronous call for streaming a frame.
    ///
    /// A function with this signature is used to transfer a frame to the IGO.
    /// \param input The user-provided output frame.
    /// \return \ref ORIGIN_SUCCESS_IGOAPI if succeeded otherwise ORIGIN_ERROR_IGOAPI_XXX
    /// \note \ref ORIGIN_ERROR_IGOAPI_STREAMING_NOT_SUPPORTED indicates that streaming is not provided by the game!
    virtual OriginErrorT transferFrame(OriginIGOStreamingInfo_T* output) = 0;

};

/// \ingroup types
/// \brief Detailed information about our DirectX 12 IGO rendering context.
struct OriginIGORenderingContext_CALLBACK_T
{
    OriginIGORenderingContextT base;

    IOriginIGORenderer *rendererInstance;
    /// Additions to this structure have to follow down here, it is forbidden to remove elements, due to compatibility reason!
};

#pragma pack(pop)