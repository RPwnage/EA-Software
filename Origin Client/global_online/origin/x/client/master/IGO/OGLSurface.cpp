///////////////////////////////////////////////////////////////////////////////
// OGLSurface.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGO.h"
#include "OGLSurface.h"

#if defined(ORIGIN_PC)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <gl/gl.h>
#include "glext.h"
#include "IGOLogger.h"
#include "OGLHook.h"
#elif defined(ORIGIN_MAC)

#include "OGLHook.h"
#include "OGLShader.h"
#include "MacWindows.h"
#import <OpenGL/OpenGL.h>
#import <Opengl/gl.h>
#import <Opengl/glext.h>
#include </usr/include/dlfcn.h>
#include "IGOLogger.h"
#endif

#include "IGOIPC/IGOIPC.h"

#include "IGOWindow.h"
#include "IGOApplication.h"

#define OGLFN(retval, name, params) \
    typedef retval (APIENTRY *name ## Fn) params; \
    extern name ## Fn _ ## name;

#define OGLFN_OPTIONAL(retval, name, params) \
typedef retval (APIENTRY *name ## Fn) params; \
extern name ## Fn _ ## name;

#define GETFN(name, handle) \
    _ ## name = (name ## Fn)GetProcAddress(handle, #name)

#ifdef DEBUG
#define GL(x) _ ## x; \
    if (SHOW_GL_ERRORS) { \
    uint32_t error = _glGetError(); \
    if (error != GL_NO_ERROR) { OriginIGO::IGOLogWarn("%s:%d glError(0x%x) %s", __FILE__, __LINE__, error, #x); } \
    IGO_ASSERT(error == GL_NO_ERROR); \
    }
#else
#define GL(x) _ ## x
#endif

#ifdef DEBUG
#define GL_EXT(fcn, ...) \
    if ( ## fcn) \
    { \
        ## fcn(__VA_ARGS__); \
        if (SHOW_GL_ERRORS) \
        { \
        uint32_t error = _glGetError(); \
        if (error != GL_NO_ERROR) { OriginIGO::IGOLogWarn("%s:%d glError(0x%x) %s", __FILE__, __LINE__, error, #fcn); } \
        IGO_ASSERT(error == GL_NO_ERROR); \
        } \
    }

#else
#define GL_EXT(fcn, ...) \
    if ( ## fcn) \
         ## fcn(__VA_ARGS__);

#endif


#if defined(ORIGIN_PC)

#include "OGLFunctions.h"

extern PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
extern PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
extern PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate;
extern PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
extern PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB;
extern PFNGLBINDBUFFERARBPROC glBindBufferARB;
extern PFNGLMULTITEXCOORD4FARBPROC glMultiTexCoord4fARB;
//extern PFNWGLBINDTEXIMAGEARBPROC wglBindTexImageARB;
extern PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArrayARB;
extern PFNGLBINDPROGRAMARBPROC glBindProgramARB;
extern PFNGLSECONDARYCOLORPOINTERPROC glSecondaryColorPointer;
extern PFNGLFOGCOORDPOINTEREXTPROC glFogCoordPointerEXT;
extern PFNGLDRAWBUFFERSARBPROC glDrawBuffersARB;

extern PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB;
extern PFNGLGENBUFFERSARBPROC glGenBuffersARB;

extern PFNGLDELETEOBJECTARBPROC glDeleteObjectARB;
extern PFNGLGETHANDLEARBPROC glGetHandleARB;
extern PFNGLDETACHOBJECTARBPROC glDetachObjectARB;
extern PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
extern PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
extern PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
extern PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
extern PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
extern PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
extern PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;
extern PFNGLVALIDATEPROGRAMARBPROC glValidateProgramARB;
extern PFNGLUNIFORM1FARBPROC glUniform1fARB;
extern PFNGLUNIFORM2FARBPROC glUniform2fARB;
extern PFNGLUNIFORM3FARBPROC glUniform3fARB;
extern PFNGLUNIFORM4FARBPROC glUniform4fARB;
extern PFNGLUNIFORM1IARBPROC glUniform1iARB;
extern PFNGLUNIFORM2IARBPROC glUniform2iARB;
extern PFNGLUNIFORM3IARBPROC glUniform3iARB;
extern PFNGLUNIFORM4IARBPROC glUniform4iARB;
extern PFNGLUNIFORM1FVARBPROC glUniform1fvARB;
extern PFNGLUNIFORM2FVARBPROC glUniform2fvARB;
extern PFNGLUNIFORM3FVARBPROC glUniform3fvARB;
extern PFNGLUNIFORM4FVARBPROC glUniform4fvARB;
extern PFNGLUNIFORM1IVARBPROC glUniform1ivARB;
extern PFNGLUNIFORM2IVARBPROC glUniform2ivARB;
extern PFNGLUNIFORM3IVARBPROC glUniform3ivARB;
extern PFNGLUNIFORM4IVARBPROC glUniform4ivARB;
extern PFNGLUNIFORMMATRIX2FVARBPROC glUniformMatrix2fvARB;
extern PFNGLUNIFORMMATRIX3FVARBPROC glUniformMatrix3fvARB;
extern PFNGLUNIFORMMATRIX4FVARBPROC glUniformMatrix4fvARB;
extern PFNGLGETOBJECTPARAMETERFVARBPROC glGetObjectParameterfvARB;
extern PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
extern PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
extern PFNGLGETATTACHEDOBJECTSARBPROC glGetAttachedObjectsARB;
extern PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
extern PFNGLGETACTIVEUNIFORMARBPROC glGetActiveUniformARB;
extern PFNGLGETUNIFORMFVARBPROC glGetUniformfvARB;
extern PFNGLGETUNIFORMIVARBPROC glGetUniformivARB;
extern PFNGLGETSHADERSOURCEARBPROC glGetShaderSourceARB;
extern PFNGLBINDATTRIBLOCATIONARBPROC glBindAttribLocationARB;
extern PFNGLGETACTIVEATTRIBARBPROC glGetActiveAttribARB;
extern PFNGLGETATTRIBLOCATIONARBPROC glGetAttribLocationARB;
extern PFNGLBUFFERDATAPROC glBufferDataARB;

extern PFNGLVERTEXATTRIBPOINTERARBPROC glVertexAttribPointerARB;
extern PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB;


namespace OriginIGO {

#define SHOW_OPENGL_SETUP

    // Helper to multiple 2 matrices (and not to use legacy OpenGL APIs)
    void MultMatrix(GLfloat dest[16], const GLfloat src1[16], const GLfloat src2[16])
    {
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                dest[j * 4 + i] = (src1[j * 4 + 0] * src2[0 + i])
                + (src1[j * 4 + 1] * src2[4 + i])
                + (src1[j * 4 + 2] * src2[8 + i])
                + (src1[j * 4 + 3] * src2[12 + i]);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////

    OGLShader::OGLShader(const char* vertexShaderSrc, const char* fragmentShaderSrc)
        : _program(0), _vertexShader(0), _fragmentShader(0), _isValid(true), _programValidated(false)
    {
        GL(glGetError());
    
        // Create vertex shader
        if (glCreateShaderObjectARB && glCreateProgramObjectARB)
        {
            _vertexShader = glCreateShaderObjectARB(GL_VERTEX_SHADER);
            GL_EXT(glShaderSourceARB, _vertexShader, 1, &vertexShaderSrc, NULL);
            GL_EXT(glCompileShaderARB, _vertexShader);
            GLenum error = _glGetError();
            
#ifdef SHOW_OPENGL_SETUP
            char buffer[2048];
            buffer[0] = '\0';
        
            GL_EXT(glGetInfoLogARB, _vertexShader, sizeof(buffer), NULL, buffer);
            OriginIGO::IGOLogWarn("Vertex shader info: %s", buffer);
#endif
            
            if (error != GL_NO_ERROR)
            {
                OriginIGO::IGOLogWarn("Failed to create vertex shader (%d)", error);
                Clean();
            }

            else
            {
                // Create fragment shader
                _fragmentShader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER);
                GL_EXT(glShaderSourceARB, _fragmentShader, 1, &fragmentShaderSrc, NULL);
                GL_EXT(glCompileShaderARB, _fragmentShader);

                error = _glGetError();
                
#ifdef SHOW_OPENGL_SETUP
                buffer[0] = '\0';
                GL_EXT(glGetInfoLogARB, _fragmentShader, sizeof(buffer), NULL, buffer);
                OriginIGO::IGOLogWarn("Fragment shader info: %s", buffer);
#endif
                
                if (error != GL_NO_ERROR)
                {
                    OriginIGO::IGOLogWarn("Failed to create fragment shader (%d)", error);
                    Clean();
                }
                else
                {
                    // Create/compile the shader program
                    _program = glCreateProgramObjectARB();
                    GL_EXT(glAttachObjectARB, _program, _vertexShader);
                    GL_EXT(glAttachObjectARB, _program, _fragmentShader);
            
                    OriginIGO::IGOLogDebug("Created OGLShader (program=%d, vShader=%d, fShader=%d)", _program, _vertexShader, _fragmentShader);
                    
                    ReLink();
                }
            }
        }
        else
        {
            OriginIGO::IGOLogWarn("Failed to create fragment shader - glCreateShaderObjectARB and/or glCreateProgramObjectARB are not available.");
            Clean();
        }
    }
    
    OGLShader::~OGLShader()
    {
        Clean();
    }

    void OGLShader::Clean()
    {
        OriginIGO::IGOLogDebug("Cleaning OGLShader (program=%d, vShader=%d, fShader=%d)", _program, _vertexShader, _fragmentShader);
        
        _isValid = false;
    
        if (_program)
        {
            GL_EXT(glDetachObjectARB, _program, _vertexShader);
            GL_EXT(glDetachObjectARB, _program, _fragmentShader);
        }

        if (_vertexShader)
            GL_EXT(glDeleteObjectARB, _vertexShader);
    
        if (_fragmentShader)
            GL_EXT(glDeleteObjectARB, _fragmentShader);

        if (_program)
        {
            GL_EXT(glDeleteObjectARB, _program);
            _program = 0;
        }
    }

    void OGLShader::ReLink()
    {
        if (!_isValid)
            return;
    
        GL_EXT(glLinkProgramARB, _program);
        GL_EXT(glValidateProgramARB, _program);
    
    #ifdef SHOW_OPENGL_SETUP
        char buffer[2048];
    
        buffer[0] = '\0';
        GL_EXT(glGetInfoLogARB, _program, sizeof(buffer), NULL, buffer);
        OriginIGO::IGOLogWarn("Compiled shader info: %s", buffer);
    #endif
    
        // Check we're good to go!
        GLint isValid = GL_FALSE;
        GL_EXT(glGetObjectParameterivARB, _program, GL_LINK_STATUS, &isValid);
        if (isValid == GL_FALSE)
        {
            OriginIGO::IGOLogWarn("Shader couldn't compile properly...");
            Clean();
        }
    }

    void OGLShader::Apply()
    {
        if (!_isValid)
            return;
        
#ifndef _DEBUG
        if (!_programValidated)
        {
            _programValidated = true;
            GL_EXT(glValidateProgramARB, _program);
        
            GLint status = GL_FALSE;
            GL_EXT(glGetObjectParameterivARB, _program, GL_VALIDATE_STATUS, &status);
            if (status != GL_TRUE)
            {
                char buffer[2048];
                buffer[0] = '\0';
            
                GL_EXT(glGetInfoLogARB, _program, sizeof(buffer), NULL, buffer);
                OriginIGO::IGOLogWarn("Program cannot execute with current OpenGL state: %s", buffer);
            }
        }
#endif
    
        GL_EXT(glUseProgramObjectARB, _program);
    }
        
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////

    const char IGOWindowOGLShader::VertexShader[] =
    "#version 150\n\
    attribute vec4 xyAnduv; \
    uniform mat4 mvp; \
    out vec2 texCoord; \
    \
    void main() \
    { \
        texCoord.xy = xyAnduv.zw; \
        vec4 vertex = vec4(xyAnduv.x, xyAnduv.y, 0.0, 1.0); \
        gl_Position = mvp * vertex; \
    }";

    const char IGOWindowOGLShader::FragmentShader[] =
    "#version 150\n\
    uniform float alpha; \
    uniform sampler2D texUnitID; \
    in vec2 texCoord; \
    \
    void main() \
    { \
        vec4 color = vec4(1.0, 1.0, 1.0, alpha); \
        gl_FragColor = texture(texUnitID, texCoord.st) * color; \
    }";

    const IGOWindowOGLShader::Vertex IGOWindowOGLShader::Vertices[] =
    {
        { 0.0f, -1.0f, 0.0f, 1.0f },
        { 0.0f,  0.0f, 0.0f, 0.0f },
        { 1.0f, -1.0f, 1.0f, 1.0f },
        { 1.0f,  0.0f, 1.0f, 0.0f },
    };

    IGOWindowOGLShader::IGOWindowOGLShader()
        : OGLShader(VertexShader, FragmentShader)
		, _alpha(-1.0f)
		, _texUnitID(-1)
		, _vertBufferIdx(0)
    {
        if (IsValid())
        {
            // Bind attribute and relink
            GL_EXT(glBindAttribLocationARB, _program, 0, "xyAnduv");
            ReLink();
        
            if (IsValid())
            {
                // Get our dynamic uniforms
                _mvpUniform = glGetUniformLocationARB(_program, "mvp");
                _alphaUniform = glGetUniformLocationARB(_program, "alpha");
                _texUnitIDUniform = glGetUniformLocationARB(_program, "texUnitID");
            
                if (_texUnitIDUniform < 0 || _alphaUniform < 0 || _mvpUniform < 0)
                {
    #ifdef SHOW_OPENGL_SETUP
                    OriginIGO::IGOLogWarn("Unable to access uniforms (texID=%d, alpha=%d, mvp=%d)", _texUnitIDUniform, _alphaUniform, _mvpUniform);
    #endif
                    _isValid = false;
                }
        
                if (_isValid)
                {
                    // Create our vertex data buffer
                    GL_EXT(glGenBuffersARB, 1, &_vertBufferIdx);
                    if (_vertBufferIdx > 0)
                    {
                        GLint bufferID = 0;
                        GL(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bufferID));
                        GL_EXT(glBindBufferARB, GL_ARRAY_BUFFER, _vertBufferIdx);
                        GL_EXT(glBufferDataARB, GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
                        GL_EXT(glBindBufferARB, GL_ARRAY_BUFFER, bufferID);
                    }
                
                    else
                    {
    #ifdef SHOW_OPENGL_SETUP
                        OriginIGO::IGOLogWarn("Unable to create vertex buffer");
    #endif
                        _isValid = false;
                    }
                }
            }
        }
    }

    IGOWindowOGLShader::~IGOWindowOGLShader()
    {
        OriginIGO::IGOLogDebug("Destroying IGOWindowOGLShader");
        
        if (_vertBufferIdx > 0)
        {
            GL_EXT(glDeleteBuffersARB, 1, &_vertBufferIdx);
        }
    }

    // Set the window alpha (between 0.0 and 1.0)
    void IGOWindowOGLShader::SetAlpha(float alpha)
    {
        if (IsValid())
        {
            if (_alpha != alpha)
            {
                GL_EXT(glUniform1fARB, _alphaUniform, alpha);
                _alpha = alpha;
            }
        }
    }

    // Set the texture unit that points to the window texture (0 -> max texture unit)
    void IGOWindowOGLShader::SetTextureUnitID(int texUnitID)
    {
        if (IsValid())
        {
            if (_texUnitID != texUnitID)
            {
                GL_EXT(glUniform1iARB, _texUnitIDUniform, texUnitID);
                _texUnitID = texUnitID;
            }
        }
    }

    // Set MVP matrix
    void IGOWindowOGLShader::SetMVPMatrix(GLfloat mvp[16])
    {
        if (IsValid())
        {
            GL_EXT(glUniformMatrix4fvARB, _mvpUniform, 1, FALSE, mvp);
        }
    }

    // Apply shader/reset uniform local states
    void IGOWindowOGLShader::Apply()
    {
        if (IsValid())
        {
            _alpha = -1.0f;
            _texUnitID = -1;
    
            OGLShader::Apply();
    
            // Setup vertices for render
            GL_EXT(glBindFramebufferEXT, GL_FRAMEBUFFER, 0);
            GL_EXT(glBindBufferARB, GL_ARRAY_BUFFER, _vertBufferIdx);
            GL_EXT(glBindBufferARB, GL_ELEMENT_ARRAY_BUFFER, 0);

            GL_EXT(glEnableVertexAttribArrayARB, 0);
            GL_EXT(glVertexAttribPointerARB, 0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
        }
    }

    // Actually render the geometry associated with the shader
    void IGOWindowOGLShader::Render()
    {
        if (IsValid())
            GL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////

    const char IGOBackgroundOGLShader::VertexShader[] =
    "#version 150\n\
    attribute vec2 xy; \
    out vec4 screenCoord; \
    out float smoothStepLimit; \
    uniform mat4 mvp; \
    uniform vec4 params; \
    \
    void main() \
    { \
        /* float screenRatio = params.y / params.x; */ \
        float screenAnimStretch = params.z + 0.8; \
        vec4 resizedDims = vec4(/* params.x * screenRatio */ params.y * screenAnimStretch, -params.y * screenAnimStretch, params.y, params.y); \
        screenCoord.xy = xy * resizedDims.xy; \
        screenCoord.zw = resizedDims.xy * vec2(0.5, -0.5); \
    \
        vec2 tmp = resizedDims.zw * resizedDims.zw; \
        smoothStepLimit = tmp.x * tmp.y * 0.03; \
    \
        vec4 vertex = vec4(xy.x, xy.y, 0.0, 1.0); \
        gl_Position = mvp * vertex; \
    }";
    
    const char IGOBackgroundOGLShader::FragmentShader[] =
    "#version 150\n\
    in vec4 screenCoord; \
    in float smoothStepLimit; \
    uniform vec4 params; \
    \
    void main() \
    { \
        vec2 diff = abs(screenCoord.xy - screenCoord.zw); \
        vec2 diff2 = diff * diff; \
        vec2 diff4 = diff2 * diff2; \
    \
        float radius2 = diff4.x + diff4.y; \
        float alpha = 0.68 + smoothstep(0.0, smoothStepLimit, radius2) * 3.2 * params.z; \
        alpha *= params.w; \
    \
        gl_FragColor = vec4(0.0, 0.0, 0.0, alpha); \
    }";

    const IGOBackgroundOGLShader::Vertex IGOBackgroundOGLShader::Vertices[] =
    {
        { 0.0f, -1.0f },
        { 0.0f, 0.0f },
        { 1.0f, -1.0f },
        { 1.0f, 0.0f },
    };

    IGOBackgroundOGLShader::IGOBackgroundOGLShader()
        : OGLShader(VertexShader, FragmentShader)
        , _vertBufferIdx(0)
    {
        memset(&_params, 0, sizeof(_params));
        _params.alpha = -1.0f;

        if (IsValid())
        {
            // Bind attribute and relink
            GL_EXT(glBindAttribLocationARB, _program, 0, "xy");
            ReLink();

            if (IsValid())
            {
                // Get our dynamic uniforms
                _mvpUniform = glGetUniformLocationARB(_program, "mvp");
                _paramsUniform = glGetUniformLocationARB(_program, "params");

                if (_paramsUniform < 0 || _mvpUniform < 0)
                {
#ifdef SHOW_OPENGL_SETUP
                    OriginIGO::IGOLogWarn("Unable to access uniforms (params=%d, mvp=%d)", _paramsUniform, _mvpUniform);
#endif
                    _isValid = false;
                }

                if (_isValid)
                {
                    // Create our vertex data buffer
                    GL_EXT(glGenBuffersARB, 1, &_vertBufferIdx);
                    if (_vertBufferIdx > 0)
                    {
                        GLint bufferID = 0;
                        GL(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bufferID));
                        GL_EXT(glBindBufferARB, GL_ARRAY_BUFFER, _vertBufferIdx);
                        GL_EXT(glBufferDataARB, GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
                        GL_EXT(glBindBufferARB, GL_ARRAY_BUFFER, bufferID);
                    }

                    else
                    {
#ifdef SHOW_OPENGL_SETUP
                        OriginIGO::IGOLogWarn("Unable to create vertex buffer");
#endif
                        _isValid = false;
                    }
                }
            }
        }
    }

    IGOBackgroundOGLShader::~IGOBackgroundOGLShader()
    {
        if (_vertBufferIdx > 0)
        {
            GL_EXT(glDeleteBuffersARB, 1, &_vertBufferIdx);
        }
    }

    // Set the window params (window width/height, border size, alpha)
    void IGOBackgroundOGLShader::SetParams(float screenWidth, float screenHeight, float stretch, float alpha)
    {
        if (IsValid())
        {
            if (_params.width != screenWidth || _params.height != screenHeight || _params.stretch != stretch || _params.alpha != alpha)
            {
                GL_EXT(glUniform4fARB, _paramsUniform, screenWidth, screenHeight, stretch, alpha);
                _params.width = screenWidth;
                _params.height = screenHeight;
                _params.stretch = stretch;
                _params.alpha = alpha;
            }
        }
    }

    // Set MVP matrix
    void IGOBackgroundOGLShader::SetMVPMatrix(GLfloat mvp[16])
    {
        if (IsValid())
        {
            GL_EXT(glUniformMatrix4fvARB, _mvpUniform, 1, FALSE, mvp);
        }
    }

    // Apply shader/reset uniform local states
    void IGOBackgroundOGLShader::Apply()
    {
        if (IsValid())
        {
            _params.alpha = -1.0f;

            OGLShader::Apply();

            // Setup vertices for render
            GL_EXT(glBindFramebufferEXT, GL_FRAMEBUFFER, 0);
            GL_EXT(glBindBufferARB, GL_ARRAY_BUFFER, _vertBufferIdx);
            GL_EXT(glBindBufferARB, GL_ELEMENT_ARRAY_BUFFER, 0);

            GL_EXT(glEnableVertexAttribArrayARB, 0);
            GL_EXT(glVertexAttribPointerARB, 0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
        }
    }

    // Actually render the geometry associated with the shader
    void IGOBackgroundOGLShader::Render()
    {
        if (IsValid())
            GL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////

    // This class manages an intermediate buffer that can be used to copy sub-regions of an original texture.
    class OGLSurfaceRegion
    {
    public:
        OGLSurfaceRegion(GLint width, GLint height)
            : mBufferSize(width * height)
        {

            mBuffer = (char*)_aligned_malloc(mBufferSize * sizeof(int), 4);
        }

        ~OGLSurfaceRegion()
        {
            _aligned_free(mBuffer);
        }

        char* data() const { return mBuffer; }

        void clear()
        {
            memset(mBuffer, 0, mBufferSize * sizeof(int));
        }

        void fill(GLint xOffset, GLint yOffset, GLint width, GLint height, GLint subWidth, GLint subHeight, const char* data)
        {
            GLint copySize = subWidth * subHeight;
            if (copySize > mBufferSize)
            {
                OriginIGO::IGOLogWarn("Unable to copy texture sub-region (%d, %d)", subWidth, subHeight);
                return;
            }

            if ((width < (xOffset + subWidth)) || (height < (yOffset + subHeight)))
            {
                OriginIGO::IGOLogWarn("Invalid subregion parameters (%d, %d, %d, %d, %d, %d)", xOffset, yOffset, width, height, subWidth, subHeight);
                return;
            }

            // Ok, now we're working with bytes
            width *= sizeof(int);
            subWidth *= sizeof(int);

            const char* src = data + yOffset * width + xOffset * sizeof(int);
            char* dst = mBuffer;
            while (subHeight > 0)
            {
                memcpy(dst, src, subWidth);

                --subHeight;

                dst += subWidth;
                src += width;
            }
        }

    private:
        char* mBuffer;

        GLint mBufferSize;
    };

    // This class represents the actual surface/Origin-side image to display. It hides the fact that we may need several textures to 
    // do so, as well as handling NPT textures when the context OpenGL version doesn't support them.
    class OGLSurfacePrivate
    {
    public:
        OGLSurfacePrivate(GLint maxTextureSize, bool openGL11)
            : mMaxTextureSize(maxTextureSize), mWidth(0), mHeight(0), mTexCount(0), mTextures(NULL), mLocalBuffer(NULL), mOpenGL11(openGL11)
        { 
            // For now, only use NPT Texture logic for OpenGL1.1 instead of checking for supported extensions
            mSupportsNPTTextures = !openGL11;

            if (mMaxTextureSize < 64)
                mMaxTextureSize = 64;
        }

        ~OGLSurfacePrivate() 
        {
            cleanup();

            delete mLocalBuffer; // only gets created if necessary
        }

        // Is the surface defined yet?
        bool isReady() const { return mTexCount > 0; }

        // Setup the set of textures to use when displaying the original image
        bool setup(GLint width, GLint height, GLenum dataFormat, const char* data)
        {
            mWidth = width;
            mHeight = height;
            mDataFormat = dataFormat;

            // How many  textures do we need?
            GLint hTexCnt = (width + mMaxTextureSize - 1) / mMaxTextureSize;
            GLint vTexCnt = (height + mMaxTextureSize - 1) / mMaxTextureSize;

            mTexCount = hTexCnt * vTexCnt;
            mTextures = new GLuint[mTexCount];

			_glGetError(); // Make sure to reset previous errors before checking whether the texture allocation worked!
			_glGenTextures(mTexCount, mTextures);

            GLenum error = _glGetError();
            if (error != GL_NO_ERROR)
            {
                OriginIGO::IGOLogWarn("%s:%d glError(0x%x) %s", __FILE__, __LINE__, error, "_glGenTextures");
                    
                _glEnd();   // buggy game/benchmark app like Heaven 4.0???
                _glGenTextures(mTexCount, mTextures);

                error = _glGetError();
                if (error != GL_NO_ERROR) 
                { 
                    OriginIGO::IGOLogWarn("%s:%d glError(0x%x) %s", __FILE__, __LINE__, error, "_glGenTextures second try");

                    mTexCount = 0;
                    delete []mTextures;

                    return false;
                }
            }

            GLenum dataType = GL_UNSIGNED_INT_8_8_8_8_REV;
            if (mOpenGL11)
                dataType = GL_UNSIGNED_BYTE;

            // Cut the image using the max texture size we know of.
            GLint texIdx = 0;
            for (GLint vTexPos = 0; vTexPos < mHeight; vTexPos += mMaxTextureSize)
            {
                for (GLint hTexPos = 0; hTexPos < mWidth; hTexPos += mMaxTextureSize)
                {
                    GL(glBindTexture(GL_TEXTURE_2D, mTextures[texIdx]));
                    if (mOpenGL11)
                    {
                        GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
                        GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
                        GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP));
                        GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP));
                    }

                    else
                    {
                        GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0));
                        GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
                        GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
                        GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
                        GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
                        GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
                        GL(glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE));
                    }

                    GLint subWidth = (hTexPos + mMaxTextureSize) > mWidth ? (mWidth - hTexPos): mMaxTextureSize;
                    GLint subHeight = (vTexPos + mMaxTextureSize) > mHeight ? (mHeight - vTexPos) : mMaxTextureSize;

                    GLint texWidth = subWidth;
                    GLint texHeight = subHeight;

                    // Do we support NPT textures?
                    if (!mSupportsNPTTextures)
                    {
                        // Nope -> clamp width/height to next 2exp(n)
                        texWidth = p2Ceiling(subWidth);
                        texHeight = p2Ceiling(subHeight);

                        // If this is not a PT texture, we initially use 2 steps to setup the texture:
                        // 1) use glTexImage2D to define the new properly sized texture
                        // 2) use glSubTexImage2D, which supports any size, to copy the actual pixels to have
                        if (texWidth != subWidth || texHeight != subHeight)
                        {
                            LocalBuffer()->clear();
                            GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texWidth, texHeight, 0, mDataFormat, dataType, LocalBuffer()->data()));
                        }
                    }

                    // Do we have to split the image and use our intermediate buffer?
                    const char* texData = data;
                    if (mTexCount > 1)
                    {
                        LocalBuffer()->fill(hTexPos, vTexPos, mWidth, mHeight, subWidth, subHeight, data);
                        texData = LocalBuffer()->data();
                    }

                    if (texWidth != subWidth || texHeight != subHeight)
                    {
                        GL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, subWidth, subHeight, mDataFormat, dataType, texData));
                    }

                    else
                    {
                        GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texWidth, texHeight, 0, mDataFormat, dataType, texData));
                    }

                    ++texIdx;
                }
            }

            return true;
        }

        // Update the content of the textures used to display the original image
        void update(const char* data)
        {
            GLenum dataType = GL_UNSIGNED_INT_8_8_8_8_REV;
            if (mOpenGL11)
                dataType = GL_UNSIGNED_BYTE;

            if (mTexCount == 1)
            {
                GL(glBindTexture(GL_TEXTURE_2D, mTextures[0]));
                GL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mWidth, mHeight, mDataFormat, dataType, data));
            }

            else
            {
                GLint texIdx = 0;
                OGLSurfaceRegion* localBuffer = LocalBuffer();

                for (GLint vTexPos = 0; vTexPos < mHeight; vTexPos += mMaxTextureSize)
                {
                    for (GLint hTexPos = 0; hTexPos < mWidth; hTexPos += mMaxTextureSize)
                    {
                        GLint subWidth = (hTexPos + mMaxTextureSize) > mWidth ? (mWidth - hTexPos): mMaxTextureSize;
                        GLint subHeight = (vTexPos + mMaxTextureSize) > mHeight ? (mHeight - vTexPos) : mMaxTextureSize;
                        localBuffer->fill(hTexPos, vTexPos, mWidth, mHeight, subWidth, subHeight, data);

                        GL(glBindTexture(GL_TEXTURE_2D, mTextures[texIdx]));
                        GL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, subWidth, subHeight, mDataFormat, dataType, localBuffer->data()));

                        ++texIdx;
                    }
                }
            }
        }

        // Render the image using the OpenGL fixed pipeline
        void applyFixedPoint(OriginIGO::IGOWindow* window, FLOAT xOnScreen, FLOAT yOnScreen)
        {
            // Set model view matrix
            FLOAT modelViewMatrix[16];
            ZeroMemory(modelViewMatrix, sizeof(modelViewMatrix));
            modelViewMatrix[10] = modelViewMatrix[15] = 1.0f;

            if (mTexCount == 1)
            {
                modelViewMatrix[0] = (FLOAT)window->width();
                modelViewMatrix[5] = (FLOAT)window->height();
                modelViewMatrix[12] = xOnScreen;
                modelViewMatrix[13] = yOnScreen;

                GL(glLoadMatrixf(modelViewMatrix));
            
                // set texture
                GL(glBindTexture(GL_TEXTURE_2D, mTextures[0]));

                // Compute correct texture coords if NPT textures not supported
                float widthTCoord = 1.0f;
                float heightTCoord = 1.0f;
                if (!mSupportsNPTTextures)
                {
                    GLint texWidth = p2Ceiling(mWidth);
                    GLint texHeight = p2Ceiling(mHeight);

                    if (texWidth != mWidth)
                        widthTCoord = (float)mWidth / texWidth;

                    if (texHeight != mHeight)
                        heightTCoord = (float)mHeight / texHeight;
                }

                // do not use GL here, glerror() is forbidden inside glbegin/glend
                _glBegin(GL_QUADS);
                _glTexCoord2f(0.0f,        0.0f);         _glVertex2f(0.0f,  0.0f);
                _glTexCoord2f(widthTCoord, 0.0f);         _glVertex2f(1.0f,  0.0f);
                _glTexCoord2f(widthTCoord, heightTCoord); _glVertex2f(1.0f, -1.0f);
                _glTexCoord2f(0.0f,        heightTCoord); _glVertex2f(0.0f, -1.0f);
                _glEnd();
            }

            else
            {
                GLint texIdx = 0;
                for (GLint vTexPos = 0; vTexPos < mHeight; vTexPos += mMaxTextureSize)
                {
                    for (GLint hTexPos = 0; hTexPos < mWidth; hTexPos += mMaxTextureSize)
                    {
                        float widthTCoord = 1.0f;
                        float heightTCoord = 1.0f;

                        GLint subWidth = mMaxTextureSize;
                        GLint subHeight = mMaxTextureSize;

                        if ((hTexPos + mMaxTextureSize) > mWidth)
                        {
                            subWidth = mWidth - hTexPos;

                            // Compute correct texture coords if NPT textures not supported
                            if (!mSupportsNPTTextures)
                            {
                                GLint texWidth = p2Ceiling(subWidth);
                                if (texWidth != subWidth)
                                    widthTCoord = (float)subWidth / texWidth;
                            }
                        }

                        if ((vTexPos + mMaxTextureSize) > mHeight)
                        {
                            subHeight = mHeight - vTexPos;

                            // Compute correct texture coords if NPT textures not supported
                            if (!mSupportsNPTTextures)
                            {
                                GLint texHeight = p2Ceiling(subHeight);
                                if (texHeight != subHeight)
                                    heightTCoord = (float)subHeight / texHeight;
                            }
                        }

                        modelViewMatrix[0] = (FLOAT)subWidth;
                        modelViewMatrix[5] = (FLOAT)subHeight;
                        modelViewMatrix[12] = xOnScreen + hTexPos;
                        modelViewMatrix[13] = yOnScreen - vTexPos;

                        GL(glLoadMatrixf(modelViewMatrix));
            
                        // set texture
                        GL(glBindTexture(GL_TEXTURE_2D, mTextures[texIdx]));
            
                        // do not use GL here, glerror() is forbidden inside glbegin/glend
                        _glBegin(GL_QUADS);
                        _glTexCoord2f(0.0f,        0.0f);         _glVertex2f(0.0f,  0.0f);
                        _glTexCoord2f(widthTCoord, 0.0f);         _glVertex2f(1.0f,  0.0f);
                        _glTexCoord2f(widthTCoord, heightTCoord); _glVertex2f(1.0f, -1.0f);
                        _glTexCoord2f(0.0f,        heightTCoord); _glVertex2f(0.0f, -1.0f);
                        _glEnd();

                        ++texIdx;
                    }
                }
            }
        }

        // Render the image using shaders
        void applyShader(OGLHook::DevContext *context, OGLShader* shader, OriginIGO::IGOWindow* window, FLOAT xOnScreen, FLOAT yOnScreen)
        {
            // IF WE CAN APPLY SHADERS, WE ASSUME NPT textures are supported!!!
            GLfloat xForm[16];

            // set model view matrix
            FLOAT modelViewMatrix[16];
            ZeroMemory(modelViewMatrix, sizeof(modelViewMatrix));
            modelViewMatrix[10] = modelViewMatrix[15] = 1.0f;

            if (mTexCount == 1)
            {
                modelViewMatrix[0] = (FLOAT)window->width();
                modelViewMatrix[5] = (FLOAT)window->height();
                modelViewMatrix[12] = xOnScreen;
                modelViewMatrix[13] = yOnScreen;

                MultMatrix(xForm, modelViewMatrix, context->projMatrix);
                shader->SetMVPMatrix(xForm);

                // Set texture
                GL(glBindTexture(GL_TEXTURE_2D, mTextures[0]));
            
                // Render!
                shader->Render();
            }

            else
            {
                GLint texIdx = 0;
                for (GLint vTexPos = 0; vTexPos < mHeight; vTexPos += mMaxTextureSize)
                {
                    for (GLint hTexPos = 0; hTexPos < mWidth; hTexPos += mMaxTextureSize)
                    {
                        GLint subWidth = (hTexPos + mMaxTextureSize) > mWidth ? (mWidth - hTexPos): mMaxTextureSize;
                        GLint subHeight = (vTexPos + mMaxTextureSize) > mHeight ? (mHeight - vTexPos) : mMaxTextureSize;

                        modelViewMatrix[0] = (FLOAT)subWidth;
                        modelViewMatrix[5] = (FLOAT)subHeight;
                        modelViewMatrix[12] = xOnScreen + hTexPos;
                        modelViewMatrix[13] = yOnScreen - vTexPos;

                        MultMatrix(xForm, modelViewMatrix, context->projMatrix);
                        shader->SetMVPMatrix(xForm);

                        // Set texture
                        GL(glBindTexture(GL_TEXTURE_2D, mTextures[texIdx]));
            
                        // Render!
                        shader->Render();

                        ++texIdx;
                    }
                }
            }
        }

        // Deallocate current set of textures used to render the original image
        void cleanup()
        {
            if (isReady())
            {
                GL(glDeleteTextures(mTexCount, mTextures));
                delete []mTextures;

                mTexCount = 0;
                mTextures = NULL;
            }
        }

    private:
        // Only allocate temporary buffer to copy sub-regions of the original image when necessary
        OGLSurfaceRegion* LocalBuffer()
        {
            if (!mLocalBuffer)
                mLocalBuffer = new OGLSurfaceRegion(mMaxTextureSize, mMaxTextureSize);

            return mLocalBuffer;
        }

        // Find 2exp(n) value that's equal or greater to the argument
        GLint p2Ceiling(GLint value)
        {
            GLint retVal = 1;
            while (retVal < value)
            {
                retVal <<= 1;
            }

            return retVal;
        }


        GLuint* mTextures;
        OGLSurfaceRegion* mLocalBuffer;

        GLint mWidth;
        GLint mHeight;
        GLint mTexCount;
        GLint mMaxTextureSize;

        GLenum mDataFormat;

        bool mOpenGL11;
        bool mSupportsNPTTextures;
    };
    
    OGLSurface::OGLSurface(void *pDev, IGOWindow *pWindow) :
    mWindow(pWindow),
    mWidth(0),
    mHeight(0),
    mAlpha(255)
    {
        OGLHook::DevContext *context = (OGLHook::DevContext*)pDev;

        bool isOpenGL11 = context->version <= 11;
        mPrivate = new OGLSurfacePrivate(context->maxTextureSize, isOpenGL11);
        bSupportsBRGA = isExtensionSupported("GL_EXT_bgra");
    }
    
    OGLSurface::~OGLSurface()
    {
        delete mPrivate;
    }
    
    // this call is "optimized" to only be callable once
    // we might change that using a map with extensions in the future if we need more of them
    bool OGLSurface::isExtensionSupported(const char *extension)
    {
        static bool support = false;
        static bool checked = false;

        if(!extension || checked)
            return support;

        typedef const GLubyte *(APIENTRY *glGetStringFuncPtr)(GLenum name);

        glGetStringFuncPtr glGetStringCall = (glGetStringFuncPtr) GetProcAddress(GetModuleHandle(L"opengl32.dll"), "glGetString");
        if (glGetStringCall != NULL)
        {
            const char *extensions = (const char*)glGetStringCall(GL_EXTENSIONS);
            OriginIGO::IGOLogWarn("OpenGL Extensions %s", extensions!=NULL ? extensions : "NULL");
            support = (extensions != NULL) ? (strstr(extensions, extension) != NULL) : false;
            checked = true;
        }
        
        return support;
    }
    
    
    void OGLSurface::render(void *pDevice)
    {
        if (mPrivate && mPrivate->isReady())
        {
            OGLHook::DevContext *context = (OGLHook::DevContext*)pDevice;

            mWindow->checkDirty();
            
            float X;
            float Y;
            
		    //Get coordinates
		    X = mWindow->x() - (float)SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenWidth) / 2;
		    Y = (-mWindow->y()) + (float)SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenHeight) / 2;
            
            // Are we using shaders?
            if (context != NULL && context->windowShader && context->bgWindowShader)
            {
                // setup projection matrix
                ZeroMemory(context->projMatrix, sizeof(context->projMatrix));

                uint32_t screenWidth = SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenWidth);
                uint32_t screenHeight = SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenHeight);

                if (screenWidth == 0)
                    screenWidth = 1;

                if (screenHeight == 0)
                    screenHeight = 1;

                context->projMatrix[0] = 2.0f / screenWidth;
                context->projMatrix[5] = 2.0f / screenHeight;
                context->projMatrix[10] = -2.0f / (9);
                context->projMatrix[14] = 2.0f / (9);
                context->projMatrix[15] = 1.0f;

                float alpha = mWindow->alpha() / 255.0f;


                // Is this the background?
                if (mWindow->id() == BACKGROUND_WINDOW_ID)
                {
                    // Make sure to reset program
                    if (context->lastShader != context->bgWindowShader)
                    {
                        context->lastShader = context->bgWindowShader;
                        context->bgWindowShader->Apply();
                    }

                    context->bgWindowShader->SetParams(static_cast<float>(screenWidth), static_cast<float>(screenHeight), mWindow->customValue() / 10000.0f, alpha);

                    mPrivate->applyShader(context, context->bgWindowShader, mWindow, X, Y);
                }

                else // Nope, regular window
                {
                    // Make sure to reset program
                    if (context->lastShader != context->windowShader)
                    {
                        context->lastShader = context->windowShader;
                        context->windowShader->Apply();
                        context->windowShader->SetTextureUnitID(context->texUnitIdx);
                    }

                    // Set window alpha
                    context->windowShader->SetAlpha(alpha);

                    mPrivate->applyShader(context, context->windowShader, mWindow, X, Y);
                }
            }
            else
            {
                // set alpha
                GL(glColor4f(1.0f, 1.0f, 1.0f, mWindow->alpha() / 255.0f));
                mPrivate->applyFixedPoint(mWindow, X, Y);
            }
        }
    }
    
    void OGLSurface::update(void *device, uint8_t alpha, int32_t width, int32_t height, const char *data, int size, uint32_t flags)
    {
        if (!(flags&IGOIPC::WINDOW_UPDATE_WIDTH))
            width = mWidth;

        if (!(flags&IGOIPC::WINDOW_UPDATE_HEIGHT))
            height = mHeight;

        if (flags&IGOIPC::WINDOW_UPDATE_ALPHA)
        {
            mAlpha = alpha;
        }

        bool sizeChanged = mWidth != width || mHeight != height;
        if (sizeChanged)
        {
            mPrivate->cleanup();

            mWidth = 0;
            mHeight = 0;
        }
        
        if ((!mPrivate->isReady() || (flags & IGOIPC::WINDOW_UPDATE_BITS)) && width > 0 && height > 0)
        {
            if (!bSupportsBRGA)
            {
                // format conversion for opengl
                uint32_t* srcPtr = (uint32_t*)data;
                uint32_t* dataPtr = (uint32_t*)data;
                
                uint32_t count = height * width;
                if (srcPtr != NULL && count > 0)
                {
                    for (uint32_t i = 0; i < count; ++i)
                    {
                        // from B8G8R8A8
                        // to   R8G8B8A8
                        uint32_t ref = srcPtr[i];
                        uint32_t masked = ref & 0x00ff00ff;
                        uint32_t swap = masked | ((ref & 0xff000000) >> 16) | ((ref & 0x0000ff00) << 16);
                        
                        dataPtr[i] = swap;
                    }
                }
            }

            if (!mPrivate->isReady())
            {
                if (!mPrivate->setup(width, height, bSupportsBRGA ? GL_BGRA_EXT : GL_RGBA, data))
                    return;
            }

            else
                mPrivate->update(data);

            mWidth = width;
            mHeight = height;
        }        
    }
}
    
#elif defined(ORIGIN_MAC) // MAC OSX

#import "MacOGLFunctions.h"

namespace OriginIGO
{
    // Helper to multiple 2 matrices (and not to use legacy OpenGL APIs)
    void MultMatrix(GLfloat dest[16], const GLfloat src1[16], const GLfloat src2[16])
    {
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                dest[j * 4 + i] = (src1[j * 4 + 0] * src2[0 + i])
                + (src1[j * 4 + 1] * src2[4 + i])
                + (src1[j * 4 + 2] * src2[8 + i])
                + (src1[j * 4 + 3] * src2[12 + i]);
            }
        }
    }

    class OGLSurfacePrivate
    {
    public:
        OGLSurfacePrivate() { }
        ~OGLSurfacePrivate() { }

        GLuint mTexture;
        bool mUseCoreProfile;
    };

    OGLSurface::OGLSurface(void *pDev, IGOWindow *pWindow) :
        mWindow(pWindow),
        mWidth(0),
        mHeight(0),
        mAlpha(255)
    {
        mPrivate = new OGLSurfacePrivate();
        mPrivate->mTexture = 0;
        mPrivate->mUseCoreProfile = false;
        
        OGLHook::DevContext* ctxt = (OGLHook::DevContext*)pDev;
        if (ctxt)
            bSupportsBRGA = ctxt->base.oglExtensions.supportsBRGA;
        
        else
            bSupportsBRGA = false;
    }

    OGLSurface::~OGLSurface()
    {
        if (mPrivate->mTexture)
            OpenGLTextures::instance()->DeleteTexture(mPrivate->mTexture, mPrivate->mUseCoreProfile);
        
        delete mPrivate;
    }



    void OGLSurface::render(void *pDevice)
    {
        if (mPrivate && mPrivate->mTexture)
        {
            float X;
            float Y;
    
            //Get coordinates
            float screenWidth = (float)SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenWidth);
            float screenHeight = (float)SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenHeight);
            
            if (screenWidth == 0)
                screenWidth = 1;
            
            if (screenHeight == 0)
                screenHeight = 1;

            X = mWindow->x() - screenWidth / 2;
            Y = -mWindow->y() + screenHeight / 2;

            float alpha = mWindow->alpha() / 255.0f;
            
            OGLHook::DevContext* ctxt = (OGLHook::DevContext*)pDevice;
            
            OGLShader* shader = NULL;
            if (mWindow->id() == BACKGROUND_WINDOW_ID)
            {
                shader = ctxt->base.bgShader;
                if (ctxt->lastShader != shader)
                {
                    ctxt->lastShader = shader;
                    shader->Apply();
                }
                
                ctxt->base.bgShader->SetParams(static_cast<float>(screenWidth), static_cast<float>(screenHeight), mWindow->customValue() / 10000.0f, alpha);
            }
            
            else
            {
                shader = ctxt->base.shader;
                if (ctxt->lastShader != shader)
                {
                    ctxt->lastShader = shader;
                    shader->Apply();
                    ctxt->base.shader->SetTextureUnitID(ctxt->texUnitIdx);
                }
                
                ctxt->base.shader->SetAlpha(alpha);
            }
            
            // Compute/set xform matrix
            GLfloat modelViewMatrix[16];
            ZeroMemory(modelViewMatrix, sizeof(modelViewMatrix));
            modelViewMatrix[0] = mWindow->width() * ctxt->pixelToPointScaleFactor;
            modelViewMatrix[5] = mWindow->height() * ctxt->pixelToPointScaleFactor;
            modelViewMatrix[10] = modelViewMatrix[15] = 1.0f;
            modelViewMatrix[12] = X * ctxt->pixelToPointScaleFactor;
            modelViewMatrix[13] = Y * ctxt->pixelToPointScaleFactor;

            GLfloat xForm[16];
            MultMatrix(xForm, modelViewMatrix, ctxt->projMatrix);
            shader->SetMVPMatrix(xForm);

            // Set texture
#ifdef _DEBUG
            if (!glIsTexture(mPrivate->mTexture))
            {
                OriginIGO::IGOLogWarn("Trying to use invalid texture id=%d", mPrivate->mTexture);
                return;
            }
#endif
            GL(glBindTexture(GL_TEXTURE_2D, mPrivate->mTexture));
            
            // Render!
            shader->Render();
        }
    }
    
    // Dump window info
    void OGLSurface::dumpInfo(char* buffer, size_t bufferSize)
    {
        if (!buffer || bufferSize == 0)
            return;
        
        if (mPrivate && mPrivate->mTexture)
        {
            snprintf(buffer, bufferSize, "id=%d, x=%d, y=%d, width=%d, height=%d, alwaysVisible=%d, texID=%d, texWidth=%d, texHeight=%d, texAlpha=%d, brga=%d"
                     , mWindow->id(), mWindow->x(), mWindow->y(), mWindow->width(), mWindow->height(), mWindow->isAlwaysVisible() ? 1 : 0
                     , mPrivate->mTexture, mWidth, mHeight, mAlpha, bSupportsBRGA ? 1 : 0);
        }
        
        else
            snprintf(buffer, bufferSize, "id=%d, no texture defined", mWindow->id());
        
        buffer[bufferSize - 1] = '\0';
    }
    
    // Dump the current window texture
    void OGLSurface::dumpBitmap(void *pDev)
    {
        if (mPrivate && mPrivate->mTexture && mWidth > 0 && mHeight > 0)
        {
            GLint oldTex;
            GL(glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTex));
            GL(glBindTexture(GL_TEXTURE_2D, mPrivate->mTexture));

            
            GLint texWidth;
            GLint texHeight;
            
            GL(glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texWidth));
            GL(glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight));
            if (texWidth != mWidth || texHeight != mHeight)
            {
                OriginIGO::IGOLogWarn("TEXTURE WIDTH/HEIGHT DON'T MATCH INTERNAL REPRESENTATION (%d/%d, %d/%d)", mWidth, texWidth, mHeight, texHeight);
            }
               
            else
            {
                char homeDir[MAX_PATH];
                if (Origin::Mac::GetHomeDirectory(homeDir, sizeof(homeDir)) == 0)
                {
                    char fileName[MAX_PATH];
                    snprintf(fileName, sizeof(fileName), "%s/Library/Application Support/Origin/Logs/IGOWindow_%d.tga", homeDir, mWindow->id());

                    unsigned char* data = new unsigned char[mWidth * mHeight * 4];
                    if (data)
                    {
                        GLenum format = bSupportsBRGA ? GL_BGRA_EXT : GL_RGBA;
                        GL(glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_BYTE, data));
                        Origin::Mac::SaveRawRGBAToTGA(fileName, mWidth, mHeight, data, true);
                        delete[] data;
                    }
                }
            }
            
            GL(glBindTexture(GL_TEXTURE_2D, oldTex));
        }
        
        else
            OriginIGO::IGOLogWarn("No texture data for window %d", mWindow->id());
    }
    
    void OGLSurface::update(void *pDev, uint8_t alpha, int32_t width, int32_t height, const char *data, int size, uint32_t flags)
    {
        if (!(flags&IGOIPC::WINDOW_UPDATE_WIDTH))
            width = mWidth;

        if (!(flags&IGOIPC::WINDOW_UPDATE_HEIGHT))
            height = mHeight;

        if (flags&IGOIPC::WINDOW_UPDATE_ALPHA)
        {
            mAlpha = alpha;
        }

#ifdef SHOW_OPENGL_SETUP
        if (mPrivate->mTexture && !glIsTexture(mPrivate->mTexture))
        {
            OriginIGO::IGOLogWarn("Invalid texutre id=%d", mPrivate->mTexture);
            mPrivate->mTexture = 0;
        }
#endif
        
        bool sizeChanged = mWidth != width || mHeight != height;
        if (sizeChanged)
        {
            // Trying this simply to see if it makes any difference when using glTexImage vs glTexSubImage
            if (mPrivate->mTexture)
            {
#ifdef SHOW_OPENGL_SETUP
                OriginIGO::IGOLogInfo("Size change - deleting texture");
#endif
                OpenGLTextures::instance()->DeleteTexture(mPrivate->mTexture, mPrivate->mUseCoreProfile);
                mPrivate->mTexture = 0;
            }
        }
        

        
        if ((mPrivate->mTexture == 0 || (flags & IGOIPC::WINDOW_UPDATE_BITS)) && width > 0 && height > 0)
        {
            bool newTex = false;
            

            if (mPrivate->mTexture == 0)
            {
                newTex = true;
                
                OGLHook::DevContext* ctxt = (OGLHook::DevContext*)pDev;
                if (ctxt)
                {
                    mPrivate->mTexture = OpenGLTextures::instance()->GenTexture(ctxt->base.oglProperties.useCoreProfile);
                    mPrivate->mUseCoreProfile = ctxt->base.oglProperties.useCoreProfile;
                }
                
                if (!mPrivate->mTexture)
                    return;
            }
            
            GLenum dataType = GL_UNSIGNED_INT_8_8_8_8_REV;
            GL(glBindTexture(GL_TEXTURE_2D, mPrivate->mTexture));

            if (newTex)
            {
                GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0));
                GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0));
                GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
                GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
                GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
                GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
            }

#ifdef SHOW_OPENGL_SETUP
            if (mPrivate->mTexture && !glIsTexture(mPrivate->mTexture))
                OriginIGO::IGOLogWarn("Invalid texture id=%d", mPrivate->mTexture);
#endif
            
            if (!bSupportsBRGA)
            {
                // format conversion for opengl
                uint32_t* srcPtr = (uint32_t*)data;
                uint32_t* dataPtr = (uint32_t*)data;
                
                uint32_t count = height * width;
                if (srcPtr != NULL && count > 0)
                {
                    for (int i = 0; i < count; ++i)
                    {
                        // from B8G8R8A8
                        // to   R8G8B8A8
                        uint32_t ref = srcPtr[i];
                        uint32_t masked = ref & 0x00ff00ff;
                        uint32_t swap = masked | ((ref & 0xff000000) >> 16) | ((ref & 0x0000ff00) << 16);
                        
                        dataPtr[i] = swap;
                    }
                }
                
                if (newTex)
                {
                    GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, dataType, dataPtr));
                }
                
                else
                {
                    GL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, dataType, dataPtr));
                }
            }
            else
            {
                if (newTex)
                {
                    GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA_EXT, dataType, data));
                }
                
                else
                {
                    GL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA_EXT, dataType, data));
                }
            }

            mWidth = width;
            mHeight = height;
        }
    }
    
    bool OGLSurface::isValid() const
    {
        return mPrivate && mPrivate->mTexture;
    }
}

#endif // ORIGIN_MAC
