//
//  OGLShader.cpp
//  IGO
//
//  Created by Frederic Meraud on 11/1/12.
//  Copyright (c) 2012 Electronic Arts. All rights reserved.
//

#ifdef ORIGIN_MAC

#ifndef GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
#define GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED 1
#endif // GL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED

#include "OGLShader.h"
#include "IGOLogger.h"
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl3.h>
#include <dlfcn.h>


#define OGLFN(retval, name, params) \
typedef retval (APIENTRY *name ## Fn) params; \
extern name ## Fn _ ## name;

#define OGLFN_OPTIONAL(retval, name, params) \
typedef retval (APIENTRY *name ## Fn) params; \
extern name ## Fn _ ## name;

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

#include "MacOGLFunctions.h"


#define OGLFN32(retval, name, params) \
typedef retval (APIENTRY *name ## Fn) params; \
name ## Fn _ ## name;

#define OGLFN32_IMPL(name) \
_ ## name = (name ## Fn)dlsym(RTLD_DEFAULT, #name); \
if (! _ ## name) \
{ \
OriginIGO::IGOLogWarn("Missing OpenGL function: " #name); \
}


// 3.2 only
OGLFN32(const GLubyte *, glGetStringi, (GLenum name, GLuint index));

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

// Extract the OpenGL properties for the current context
OpenGLProperties ExtractOpenGLProperties(void* ctxt)
{
    static bool ogl32Initialized = false;
    if (!ogl32Initialized)
    {
        // Initialized 3.2 core specific methods
        ogl32Initialized = true;
        
        OGLFN32_IMPL(glGetStringi);
    }
    
    OpenGLProperties props;
    
    GL(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &props.maxVertexAttribs));
    GL(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &props.maxSharedTextureUnits));
    GL(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &props.maxSharedTextureCoordUnits));
    
    const GLubyte* vendor      = GL(glGetString(GL_VENDOR));
    const GLubyte* renderer    = GL(glGetString(GL_RENDERER));
    const GLubyte* version     = GL(glGetString(GL_VERSION));
    const GLubyte* glslVersion = GL(glGetString(GL_SHADING_LANGUAGE_VERSION));
    snprintf(props.profile, sizeof(props.profile), "%s/%s", version, glslVersion);
    
    OriginIGO::IGOLogInfo("New ctxt %p on thread 0x%08x", ctxt, GetCurrentThreadId());
    OriginIGO::IGOLogInfo("OpenGL info:\nVendor: %s\nRenderer: %s\nVersion: %s\nGLSL Version: %s", vendor, renderer, version, glslVersion);
    
    // We do more work than necessary - just to be safe
    props.useCoreProfile = false;
    
    const char* start = (const char*)version;
    while ((*start) && !isdigit(*start))
        ++start;
    
    if (start[1] == '.' && isdigit(start[2]))
    {
        double version = atof(start);
        if (version >= 3.2)
            props.useCoreProfile = true;
    }
    
    OriginIGO::IGOLogInfo("UseCoreImpl = %d", props.useCoreProfile ? 1 : 0);
    
    return props;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

// Extract the OpenGL extensions for the current context
OpenGLExtensions ExtractOpenGLExtensions(bool useCoreProfile)
{
    OpenGLExtensions exts;
    
    if (!useCoreProfile)
    {
        const char *extensions = (const char*)GL(glGetString(GL_EXTENSIONS));
        
        if (extensions)
        {
            exts.supportsBRGA = strstr(extensions, "GL_EXT_bgra");
            exts.supportsNPOT = strstr(extensions, "GL_ARB_texture_non_power_of_two");
            exts.supportsPurgeableObjects = strstr(extensions, "GL_APPLE_object_purgeable");
            exts.supportsSRGBFramebuffer = strstr(extensions, "GL_EXT_framebuffer_sRGB"); // May have to do something with this one!
            exts.supportsInstancedArrays = strstr(extensions, "GL_ARB_instanced_arrays");
        }
        
        else
        {
            exts.supportsBRGA = false;
            exts.supportsNPOT = false;
            exts.supportsPurgeableObjects = false;
            exts.supportsSRGBFramebuffer = false;
            exts.supportsInstancedArrays = false;
        }
    }
    
    else
    {
        // Most of the extensions are actually part of the Core specs.
        exts.supportsBRGA = true;
        exts.supportsNPOT = true;
        exts.supportsPurgeableObjects = false;
        exts.supportsSRGBFramebuffer = true;
        exts.supportsInstancedArrays = false;
        
        GLint numExtensions = 0;
        GL(glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions));
        for (int idx = 0; idx < numExtensions; ++idx)
        {
            const char* extName = (const char*)GL(glGetStringi(GL_EXTENSIONS, idx));
            if (extName)
            {
                if (strstr(extName, "GL_APPLE_object_purgeable"))
                    exts.supportsPurgeableObjects = true;
                
                if (strstr(extName, "GL_ARB_instanced_arrays"))
                    exts.supportsInstancedArrays = true;
            }
        }
    }
    
    OriginIGO::IGOLogInfo("Supported extensions:\n");
    if (exts.supportsBRGA)
        OriginIGO::IGOLogInfo(" - BRGA");
    
    if (exts.supportsPurgeableObjects)
        OriginIGO::IGOLogInfo(" - Purgeable");
    
    if (exts.supportsNPOT)
        OriginIGO::IGOLogInfo(" - NPOT");
    
    if (exts.supportsSRGBFramebuffer)
        OriginIGO::IGOLogInfo(" - sRGB");
    
    if (exts.supportsInstancedArrays)
        OriginIGO::IGOLogInfo(" - VertexArrayDivisor");
    
    return exts;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

OpenGLTextures* OpenGLTextures::_instance = NULL;
EA::Thread::Futex OpenGLTextures::_lock;

// Initialize the singleton we use to store the OpenGL extensions
OpenGLTextures* OpenGLTextures::instance()
{
    if (!_instance)
    {
        EA::Thread::AutoFutex m(_lock);
        if (!_instance)
        {
            _instance = new OpenGLTextures();
        }
    }
    
    return _instance;
}

OpenGLTextures::OpenGLTextures()
{
    _nextAvailTextureID = BaseTextureID;
    _reservedTextureIDCnt = 0;
    ZeroMemory(_textureIDs, sizeof(_textureIDs));
}

GLuint OpenGLTextures::GenTexture(bool useCoreProfile)
{
    if (useCoreProfile)
    {
        // Use default API
        GLuint id = 0;
        GL(glGenTextures(1, &id));
        
#ifdef SHOW_OPENGL_SETUP
        OriginIGO::IGOLogInfo("GenTexture id=%d, useCoreProfile=%d", id, useCoreProfile);
#endif
        return id;
    }
    
    else
    {
        GLuint texID = 0;
        int32_t emptyEntryIdx = -1;
        
        EA::Thread::AutoFutex m(_lock);
        
        // Check if we have already an entry we already use
        for (int32_t idx = 0; idx < _reservedTextureIDCnt; ++idx)
        {
            // Skip cleared up entry because the game is now using it!
            if (_textureIDs[idx] == 0)
            {
                emptyEntryIdx = idx;
                continue;
            }
            
            if ((_textureIDs[idx] & 1) == 0)
            {
                texID = _textureIDs[idx] >> 8;
                
                // Shouldn't be in use by anybody!
                if (!glIsTexture(texID))
                {
#ifdef SHOW_OPENGL_SETUP
                    OriginIGO::IGOLogInfo("Reusing slot %d with texture id=%d", idx, texID);
#endif
                    _textureIDs[idx] |= 1;
                    break;
                }
                
                else
                {
                    // Somebody else decided to use this ID -> clear up the table entry
#ifdef SHOW_OPENGL_SETUP
                    OriginIGO::IGOLogInfo("Clearing entry %d (texture id=%d) in use by game?", idx, texID);
#endif
                    texID = 0;
                    _textureIDs[idx] = 0;
                }
            }
        }
        
        // If no pre-reserved textureID, look for a new one
        if (texID == 0)
        {
            // Make sure we're going to be able to store a new textureID in the table
            if (_reservedTextureIDCnt < MaxTextureCount || emptyEntryIdx >= 0)
            {
                while (texID == 0 && _nextAvailTextureID < MaxTextureID)
                {
                    if (!glIsTexture(_nextAvailTextureID))
                    {
                        texID = _nextAvailTextureID;
                        
                        if (emptyEntryIdx >= 0)
                        {
#ifdef SHOW_OPENGL_SETUP
                            OriginIGO::IGOLogInfo("Using empty slot %d for new texture id=%d", emptyEntryIdx, texID);
#endif
                            _textureIDs[emptyEntryIdx] = (texID << 8) | 1;
                        }
                        
                        else
                        {
#ifdef SHOW_OPENGL_SETUP
                            OriginIGO::IGOLogInfo("Using new slot %d for new texture id=%d", _reservedTextureIDCnt, texID);
#endif
                            _textureIDs[_reservedTextureIDCnt] = (texID << 8) | 1;
                            ++_reservedTextureIDCnt;
                        }
                    }
                    
                    ++_nextAvailTextureID;
                }
                
                if (texID == 0)
                    OriginIGO::IGOLogWarn("UNABLE TO FIND AVAILABLE TEXTURE ID");
            }
            
            else
                OriginIGO::IGOLogWarn("No more slot to allocate new texture id");
        }
        
        return texID;
    }
}

void OpenGLTextures::DeleteTexture(GLuint id, bool useCoreProfile)
{
#ifdef SHOW_OPENGL_SETUP
    OriginIGO::IGOLogInfo("Deleting texture id=%d, useCoreProfile=%d", id, useCoreProfile);
#endif
    
    // Always delete the texture!
    GL(glDeleteTextures(1, &id));
    
    // But only update our locally maintained textureID table for non core profile game (ie all of them right now!)
    if (!useCoreProfile)
    {
        // Simply lookup the id in the list of already "reserved" texture IDs
        bool found = false;
        
        {
            EA::Thread::AutoFutex m(_lock);
            
            GLuint entryID = (id << 8) | 1;
            for (size_t idx = 0; idx < _reservedTextureIDCnt; ++idx)
            {
                if (_textureIDs[idx] == entryID)
                {
#ifdef SHOW_OPENGL_SETUP
                    OriginIGO::IGOLogInfo("Release texture id=%d in slot %d", id, idx);
#endif
                    _textureIDs[idx] = id << 8;
                    found = true;
                }
            }
        }
        
        if (!found)
            OriginIGO::IGOLogWarn("Unable to release texture id=%d", id);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

OGLShader::OGLShader(const char* vertexShaderSrc, const char* fragmentShaderSrc)
: _program(0), _vertexShader(0), _fragmentShader(0), _isValid(true), _programValidated(false)
{
    GL(glGetError());
    
    // Create vertex shader
    _vertexShader = GL(glCreateShader(GL_VERTEX_SHADER));
    GL(glShaderSource(_vertexShader, 1, &vertexShaderSrc, NULL));
    GL(glCompileShader(_vertexShader));
    
    GLenum error = _glGetError();
    
#ifdef SHOW_OPENGL_SETUP
    char buffer[2048];
    buffer[0] = '\0';
    
    GL(glGetShaderInfoLog(_vertexShader, sizeof(buffer), NULL, buffer));
    OriginIGO::IGOLogInfo("Vertex shader info: %s", buffer);
#endif
    
    if (error != GL_NO_ERROR)
    {
        OriginIGO::IGOLogWarn("Failed to create vertex shader (%d)", error);
        Clean();
    }
    
    else
    {
        // Create fragment shader
        _fragmentShader = GL(glCreateShader(GL_FRAGMENT_SHADER));
        GL(glShaderSource(_fragmentShader, 1, &fragmentShaderSrc, NULL));
        GL(glCompileShader(_fragmentShader));
        
        error = _glGetError();
        
#ifdef SHOW_OPENGL_SETUP
        buffer[0] = '\0';
        GL(glGetShaderInfoLog(_fragmentShader, sizeof(buffer), NULL, buffer));
        OriginIGO::IGOLogInfo("Fragment shader info: %s", buffer);
#endif
        
        if (error != GL_NO_ERROR)
        {
            OriginIGO::IGOLogWarn("Failed to create fragment shader (%d)", error);
            Clean();
        }
        
        else
        {
            // Create/compile the shader program
            _program = GL(glCreateProgram());
            GL(glAttachShader(_program, _vertexShader));
            GL(glAttachShader(_program, _fragmentShader));
        
#ifdef SHOW_OPENGL_SETUP
            OriginIGO::IGOLogInfo("Program=%d, vShader=%d, fShader=%d", _program, _vertexShader, _fragmentShader);
#endif
        }
    }
}

OGLShader::~OGLShader()
{
#ifdef SHOW_OPENGL_SETUP
    OriginIGO::IGOLogInfo("Destroying shader");
#endif
    Clean();
}

void OGLShader::Clean()
{
    _isValid = false;
    
    if (_vertexShader)
    {
        if (_program)
        {
            GL(glDetachShader(_program, _vertexShader));
        }
        
        GL(glDeleteShader(_vertexShader));
    }
    
    if (_fragmentShader)
    {
        if (_program)
        {
            GL(glDetachShader(_program, _fragmentShader));
        }
        
        GL(glDeleteShader(_fragmentShader));
    }
    
    if (_program)
    {
        GL(glDeleteProgram(_program));
        _program = 0;
    }
}

void OGLShader::Link()
{
    if (!_isValid)
        return;
    
    GL(glLinkProgram(_program));
    GL(glValidateProgram(_program));
    
#ifdef SHOW_OPENGL_SETUP
    char buffer[2048];
    
    buffer[0] = '\0';
    GL(glGetProgramInfoLog(_program, sizeof(buffer), NULL, buffer));
    OriginIGO::IGOLogInfo("Compiled shader info: %s", buffer);
#endif
    
    // Check we're good to go!
    GLint isValid = GL_FALSE;
    GL(glGetProgramiv(_program, GL_LINK_STATUS, &isValid));
    if (isValid == GL_FALSE)
    {
        OriginIGO::IGOLogWarn("Shader couldn't compile properly...");
        Clean();
    }
    
    else
    {
        // We don't need the separate shaders at that point!
        GL(glDetachShader(_program, _vertexShader));
        GL(glDetachShader(_program, _fragmentShader));
        
        GL(glDeleteShader(_vertexShader));
        GL(glDeleteShader(_fragmentShader));
        
        _vertexShader = NULL;
        _fragmentShader = NULL;
    }
}

void OGLShader::Apply()
{
#ifdef SHOW_OPENGL_SETUP
    if (!_programValidated)
    {
        _programValidated = true;
        GL(glValidateProgram(_program));
        
        GLint status = GL_FALSE;
        GL(glGetProgramiv(_program, GL_VALIDATE_STATUS, &status));
        if (status != GL_TRUE)
        {
            char buffer[2048];
            buffer[0] = '\0';
            
            GL(glGetProgramInfoLog(_program, sizeof(buffer), NULL, buffer));
            OriginIGO::IGOLogWarn("Program cannot execute with current OpenGL state: %s", buffer);
        }
    }
#endif
    
    GL(glUseProgram(_program));
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

// Shaders for pre 3.2 core profile
const char IGOWindowOGLShader::VertexShaderPre3_2[] =
"attribute vec4 xyAnduv; \
uniform mat4 mvp; \
\
void main() \
{ \
gl_TexCoord[0].xy = xyAnduv.zw; \
vec4 vertex = vec4(xyAnduv.x, xyAnduv.y, 0.0, 1.0); \
gl_Position = mvp * vertex; \
}";

const char IGOWindowOGLShader::FragmentShaderPre3_2[] =
"uniform float alpha; \
uniform sampler2D texUnitID; \
\
void main() \
{ \
vec4 color = vec4(1.0, 1.0, 1.0, alpha); \
gl_FragColor = texture2D(texUnitID, gl_TexCoord[0].st) * color; \
}";

// Shaders starting with 3.2 core profile
const char IGOWindowOGLShader::VertexShaderPost3_2[] =
"#version 150 core \
in vec4 xyAnduv; \
out vec2 texCoords; \
uniform mat4 mvp; \
\
void main() \
{ \
texCoords = xyAnduv.zw; \
vec4 vertex = vec4(xyAnduv.x, xyAnduv.y, 0.0, 1.0); \
gl_Position = mvp * vertex; \
}";

const char IGOWindowOGLShader::FragmentShaderPost3_2[] =
"#version 150 core \
in vec2 texCoords; \
out vec4 fragColor; \
uniform float alpha; \
uniform sampler2D texUnitID; \
\
void main() \
{ \
vec4 color = vec4(1.0, 1.0, 1.0, alpha); \
vec4 texel = texture(texUnitID, texCoords); \
if (texel.w < 0.1) \
discard; \
\
fragColor = texel * color; \
}";

const IGOWindowOGLShader::Vertex IGOWindowOGLShader::Vertices[] =
{
    { 0.0f, -1.0f, 0.0f, 1.0f },
    { 0.0f,  0.0f, 0.0f, 0.0f },
    { 1.0f, -1.0f, 1.0f, 1.0f },
    { 1.0f,  0.0f, 1.0f, 0.0f },
};

IGOWindowOGLShader::IGOWindowOGLShader(bool useCoreProfile)
: OGLShader(useCoreProfile ? VertexShaderPost3_2 : VertexShaderPre3_2, useCoreProfile ? FragmentShaderPost3_2 : FragmentShaderPre3_2)
, _alpha(0.0f)
, _texUnitID(0)
, _vertBufferIdx(0)
{
    if (IsValid())
    {
        // Bind attribute(s) and link the program
        GL(glBindAttribLocation(_program, 0, "xyAnduv"));
        Link();
        
        if (IsValid())
        {
            // Get our dynamic uniforms
            _mvpUniform = GL(glGetUniformLocation(_program, "mvp"));
            _alphaUniform = GL(glGetUniformLocation(_program, "alpha"));
            _texUnitIDUniform = GL(glGetUniformLocation(_program, "texUnitID"));
            
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
                GL(glGenBuffers(1, &_vertBufferIdx));
                if (_vertBufferIdx > 0)
                {
                    GLint bufferID = 0;
                    GL(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bufferID));
                    GL(glBindBuffer(GL_ARRAY_BUFFER, _vertBufferIdx));
                    GL(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW));
                    GL(glBindBuffer(GL_ARRAY_BUFFER, bufferID));
                }
                
                else
                {
#ifdef SHOW_OPENGL_SETUP
                    OriginIGO::IGOLogInfo("Unable to create vertex buffer");
#endif
                    _isValid = false;
                }
            }
        }
    }
}

IGOWindowOGLShader::~IGOWindowOGLShader()
{
    if (_vertBufferIdx > 0)
    {
        GL(glDeleteBuffers(1, &_vertBufferIdx));
    }
}

// Set the window alpha (between 0.0 and 1.0)
void IGOWindowOGLShader::SetAlpha(float alpha)
{
    if (IsValid())
    {
        if (_alpha != alpha)
        {
            GL(glUniform1f(_alphaUniform, alpha));
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
            GL(glUniform1i(_texUnitIDUniform, texUnitID));
            _texUnitID = texUnitID;
        }
    }
}

// Set MVP matrix
void IGOWindowOGLShader::SetMVPMatrix(GLfloat mvp[16])
{
    if (IsValid())
    {
        GL(glUniformMatrix4fv(_mvpUniform, 1, FALSE, mvp));
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
        GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        GL(glBindBuffer(GL_ARRAY_BUFFER, _vertBufferIdx));
        GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
        
        GL(glEnableVertexAttribArray(0));
        GL(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0));
    }
}

// Actually render the geometry associated with the shader
void IGOWindowOGLShader::Render()
{
    if (IsValid())
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

// Shaders for pre 3.2 core profile
const char IGOBackgroundOGLShader::VertexShaderPre3_2[] =
"attribute vec2 xy; \
uniform mat4 mvp; \
uniform vec4 params; \
varying float smoothStepLimit; \
\
void main() \
{ \
    /* float screenRatio = params.y / params.x; */ \
    float screenAnimStretch = params.z + 0.8; \
    vec4 resizedDims = vec4(/* params.x * screenRatio */ params.y * screenAnimStretch, -params.y * screenAnimStretch, params.y, params.y); \
    gl_TexCoord[0].st = xy * resizedDims.xy; \
    gl_TexCoord[0].pq = resizedDims.xy * vec2(0.5, -0.5); \
\
    vec2 tmp = resizedDims.zw * resizedDims.zw; \
    smoothStepLimit = tmp.x * tmp.y * 0.04; \
\
    vec4 vertex = vec4(xy.x, xy.y, 0.0, 1.0); \
    gl_Position = mvp * vertex; \
}";

const char IGOBackgroundOGLShader::FragmentShaderPre3_2[] =
"uniform vec4 params; \
varying float smoothStepLimit; \
\
void main() \
{ \
    vec2 diff = abs(gl_TexCoord[0].st - gl_TexCoord[0].pq); \
    vec2 diff2 = diff * diff; \
    vec2 diff4 = diff2 * diff2; \
\
    float radius2 = diff4.x + diff4.y; \
    float alpha = 0.68 + smoothstep(0.0, smoothStepLimit, radius2) * 3.2 * params.z; \
    alpha *= params.w; \
\
    gl_FragColor = vec4(0.0, 0.0, 0.0, alpha); \
}";

// Shaders starting with 3.2 core profile
const char IGOBackgroundOGLShader::VertexShaderPost3_2[] =
"#version 150 core \
in vec2 xy; \
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
    smoothStepLimit = tmp.x * tmp.y * 0.04; \
\
    vec4 vertex = vec4(xy.x, xy.y, 0.0, 1.0); \
    gl_Position = mvp * vertex; \
}";

const char IGOBackgroundOGLShader::FragmentShaderPost3_2[] =
"#version 150 core \
in vec4 screenCoord; \
in float smoothStepLimit; \
out vec4 fragColor; \
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
    fragColor = vec4(0.0, 0.0, 0.0, alpha); \
}";

const IGOBackgroundOGLShader::Vertex IGOBackgroundOGLShader::Vertices[] =
{
    { 0.0f, -1.0f },
    { 0.0f,  0.0f },
    { 1.0f, -1.0f },
    { 1.0f,  0.0f },
};

IGOBackgroundOGLShader::IGOBackgroundOGLShader(bool useCoreProfile)
: OGLShader(useCoreProfile ? VertexShaderPost3_2 : VertexShaderPre3_2, useCoreProfile ? FragmentShaderPost3_2 : FragmentShaderPre3_2)
, _vertBufferIdx(0)
{
    memset(&_params, 0, sizeof(_params));
    _params.alpha = -1.0f;

    if (IsValid())
    {
        // Bind attribute(s) and link the program
        GL(glBindAttribLocation(_program, 0, "xy"));
        Link();
        
        if (IsValid())
        {
            // Get our dynamic uniforms
            _mvpUniform = GL(glGetUniformLocation(_program, "mvp"));
            _paramsUniform = GL(glGetUniformLocation(_program, "params"));
            
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
                GL(glGenBuffers(1, &_vertBufferIdx));
                if (_vertBufferIdx > 0)
                {
                    GLint bufferID = 0;
                    GL(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bufferID));
                    GL(glBindBuffer(GL_ARRAY_BUFFER, _vertBufferIdx));
                    GL(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW));
                    GL(glBindBuffer(GL_ARRAY_BUFFER, bufferID));
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
        GL(glDeleteBuffers(1, &_vertBufferIdx));
    }
}

// Set the window params (window width/height, border size, alpha)
void IGOBackgroundOGLShader::SetParams(float screenWidth, float screenHeight, float stretch, float alpha)
{
    if (IsValid())
    {
        if (_params.width != screenWidth || _params.height != screenHeight || _params.stretch != stretch || _params.alpha != alpha)
        {
            OriginIGO::IGOLogWarn("Stretch: %f", stretch);
            
            GL(glUniform4f(_paramsUniform, screenWidth, screenHeight, stretch, alpha));
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
        GL(glUniformMatrix4fv(_mvpUniform, 1, FALSE, mvp));
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
        GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        GL(glBindBuffer(GL_ARRAY_BUFFER, _vertBufferIdx));
        GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
        
        GL(glEnableVertexAttribArray(0));
        GL(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0)); // sizeof(Vertex), 0));
    }
}

// Actually render the geometry associated with the shader
void IGOBackgroundOGLShader::Render()
{
    if (IsValid())
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}


#endif // MAC_OSX
