//
//  OGLShader.h
//  IGO
//
//  Created by Frederic Meraud on 11/1/12.
//  Copyright (c) 2012 Electronic Arts. All rights reserved.
//

#ifndef OGLSHADER_H
#define OGLSHADER_H

#ifdef ORIGIN_MAC

#include "IGOTrace.h"
#include "eathread/eathread_futex.h"

#import <OpenGL/gl.h>
#import <ApplicationServices/ApplicationServices.h>


// Helper to keep track of OpenGL current specs
struct OpenGLProperties
{
    GLint maxVertexAttribs;
    GLint maxSharedTextureUnits;
    GLint maxSharedTextureCoordUnits;
    
    char profile[128];
    
    bool useCoreProfile;
};

// Extract the OpenGL properties for the current context
OpenGLProperties ExtractOpenGLProperties(void* ctxt);


// Helper to access OpenGL supported extensions
struct OpenGLExtensions
{
    bool supportsBRGA;
    bool supportsNPOT;
    bool supportsPurgeableObjects;
    bool supportsSRGBFramebuffer;
    bool supportsInstancedArrays;
};

// Extract the OpenGL extensions for the current context
OpenGLExtensions ExtractOpenGLExtensions(bool useCoreProfile);


// Helper to manage textures which can be directly assigned (without using glGenTextures) before OpenGL 3.0 :(
class OpenGLTextures
{
public:
    static OpenGLTextures* instance();
    
    GLuint GenTexture(bool useCoreProfile);
    void DeleteTexture(GLuint id, bool useCoreProfile);
    
private:
    OpenGLTextures();
    
    static OpenGLTextures* _instance;
    static EA::Thread::Futex _lock;
    
    static const int32_t MaxTextureCount = 256;
    static const GLuint BaseTextureID = 45678;
    static const GLuint MaxTextureID = 48000;
    
    // Note that we use the first byte as a flag to know if the textureID is in use.
    GLuint _textureIDs[MaxTextureCount];
    
    GLuint _nextAvailTextureID;
    int32_t _reservedTextureIDCnt;    
};

// Wrapper to manage a shader program
class OGLShader
{
public:
    OGLShader(const char* vertexShaderSrc, const char* fragmentShaderSrc);
    virtual ~OGLShader();
    
    void Link();
    virtual void Apply();
    virtual void Render() = 0;
    virtual void SetMVPMatrix(GLfloat mvp[16]) = 0;

    bool IsValid() const { return _isValid; }
    
protected:
    void Clean();
    void ValidateProgram();
    
    
    GLuint _program;
    GLuint _vertexShader;
    GLuint _fragmentShader;
    
    bool _isValid;
    bool _programValidated;
};


// Specific shader used to render the IGO windows
class IGOWindowOGLShader : public OGLShader
{
public:
    IGOWindowOGLShader(bool useCoreProfile);
    ~IGOWindowOGLShader();
    
    void Apply();

    // Set the window alpha (between 0.0 and 1.0)
    void SetAlpha(float alpha);
    
    // Set the texture unit that points to the window texture (0 -> max texture unit)
    void SetTextureUnitID(int texUnitID);
    
    // Set MVP matrix
    void SetMVPMatrix(GLfloat mvp[16]);
    
    // Render the geometry assiociated with the shader (quad)
    void Render();
    
    
private:
    // Vertex format used for rendering
    struct Vertex
    {
        float x;
        float y;
        float u;
        float v;
    };

private:
    static const char VertexShaderPre3_2[];
    static const char FragmentShaderPre3_2[];
    static const char VertexShaderPost3_2[];
    static const char FragmentShaderPost3_2[];

    static const Vertex Vertices[];
    
    float _alpha;
    int _texUnitID;

    GLint _mvpUniform;
    GLint _alphaUniform;
    GLint _texUnitIDUniform;
    GLuint _vertBufferIdx;
};

// Specific shader used to render background
class IGOBackgroundOGLShader : public OGLShader
{
public:
    IGOBackgroundOGLShader(bool useCoreProfile);
    ~IGOBackgroundOGLShader();
    
    void Apply();
    
    // Set the window params (alpha, window width/height, border size)
    void SetParams(float alpha, float screenWidth, float screenHeight, float borderSize);
    
    // Set MVP matrix
    void SetMVPMatrix(GLfloat mvp[16]);
    
    // Render the geometry assiociated with the shader (quad)
    void Render();
    
    
private:
    // Vertex format used for rendering
    struct Vertex
    {
        float x;
        float y;
    };
    
private:
    static const char VertexShaderPre3_2[];
    static const char FragmentShaderPre3_2[];
    static const char VertexShaderPost3_2[];
    static const char FragmentShaderPost3_2[];
    static const Vertex Vertices[];
    
    struct Params
    {
        float width;
        float height;
        float stretch;
        float alpha;
    } _params;
    
    GLint _mvpUniform;
    GLint _paramsUniform;
    GLuint _vertBufferIdx;
};

#endif // MAC_OSX

#endif
