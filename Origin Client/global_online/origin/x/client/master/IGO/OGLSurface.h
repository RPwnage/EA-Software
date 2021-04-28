///////////////////////////////////////////////////////////////////////////////
// OGLSurface.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef OGLSURFACE_H
#define OGLSURFACE_H

#include "IGOSurface.h"
#if defined(ORIGIN_PC)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <gl/gl.h>
#endif

namespace OriginIGO {

class OGLSurfacePrivate;
class IGOWindow;

#if defined(ORIGIN_PC)

void MultMatrix(GLfloat dest[16], const GLfloat src1[16], const GLfloat src2[16]);

// Wrapper to manage a shader program
class OGLShader
{
public:
    OGLShader(const char* vertexShaderSrc, const char* fragmentShaderSrc);
    virtual ~OGLShader();
    
    void ReLink();
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
    IGOWindowOGLShader();
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
    static const char VertexShader[];
    static const char FragmentShader[];
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
    IGOBackgroundOGLShader();
    ~IGOBackgroundOGLShader();

    void Apply();

    // Set the window params (alpha, window width/height, border size)
    void SetParams(float screenWidth, float screenHeight, float stretch, float alpha);

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
    static const char VertexShader[];
    static const char FragmentShader[];
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

#endif

class OGLSurface : public IGOSurface
    {
    public:
        OGLSurface(void *pDev, IGOWindow *pWindow);
        virtual ~OGLSurface();
		
		virtual void render(void *pDev);
#ifdef ORIGIN_MAC
        virtual void dumpInfo(char* buffer, size_t bufferSize);
        virtual void dumpBitmap(void *pDev);
        virtual bool isValid() const;
#endif
#if defined(ORIGIN_PC)
    static bool isExtensionSupported(const char *extension);
#endif

	virtual void update(void *pDev, uint8_t alpha, int32_t width, int32_t height, const char *data, int size, uint32_t flags);

    private:
        IGOWindow *mWindow;
        OGLSurfacePrivate *mPrivate;
        int mWidth, mHeight;
        int mAlpha;
        bool bSupportsBRGA;
    };
}
#endif 
