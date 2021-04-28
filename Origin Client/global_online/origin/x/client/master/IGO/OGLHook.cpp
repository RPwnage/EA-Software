///////////////////////////////////////////////////////////////////////////////
// OGLHook.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGO.h"
#include "OGLHook.h"
#include "IGOLogger.h"
#include "MacDllMain.h"
#include "IGOSharedStructs.h"
#include "IGOTelemetry.h"

#if defined(ORIGIN_PC)

#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include "glext.h"
#include "resource.h"
#include "OGLSurface.h"
#include "InputHook.h"
#include "twitchsdk.h"  // always include it before TwitchManager.h !!!
#include "TwitchManager.h"
#include "IGOTelemetry.h"


namespace OriginIGO {
    extern HINSTANCE gInstDLL;
}


#define GET_PROC_ADDRESS _wglGetProcAddress
  
#define QUERY_EXTENSION_ENTRY_POINT_GL3(type, name)               \
    name = (type)GET_PROC_ADDRESS(#name);\
    if (! ## name) \
    { \
        OriginIGO::IGOLogWarn("OpenGL extension missing: %s", #name); \
    }

PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT=NULL;
PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT=NULL;
PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT=NULL;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT=NULL;
PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT=NULL;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT=NULL;
PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate=NULL;
PFNGLBLENDEQUATIONSEPARATEPROC glBlendEquationSeparate=NULL;
PFNGLACTIVETEXTUREARBPROC glActiveTextureARB=NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB=NULL;
PFNGLBINDBUFFERARBPROC glBindBufferARB=NULL;
PFNGLMULTITEXCOORD4FARBPROC glMultiTexCoord4fARB=NULL;
//PFNWGLBINDTEXIMAGEARBPROC wglBindTexImageARB=NULL;
PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArrayARB=NULL;
PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB=NULL;
PFNGLBINDPROGRAMARBPROC glBindProgramARB=NULL;
PFNGLSECONDARYCOLORPOINTERPROC glSecondaryColorPointer=NULL;
PFNGLFOGCOORDPOINTEREXTPROC glFogCoordPointerEXT=NULL;
PFNGLDRAWBUFFERSARBPROC glDrawBuffersARB=NULL;

PFNGLBLENDEQUATIONEXTPROC glBlendEquationEXT = NULL;

PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB=NULL;
PFNGLGENBUFFERSARBPROC glGenBuffersARB=NULL;

PFNGLDELETEOBJECTARBPROC glDeleteObjectARB=NULL;
PFNGLGETHANDLEARBPROC glGetHandleARB=NULL;
PFNGLDETACHOBJECTARBPROC glDetachObjectARB=NULL;
PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB=NULL;
PFNGLSHADERSOURCEARBPROC glShaderSourceARB=NULL;
PFNGLCOMPILESHADERARBPROC glCompileShaderARB=NULL;
PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB=NULL;
PFNGLATTACHOBJECTARBPROC glAttachObjectARB=NULL;
PFNGLLINKPROGRAMARBPROC glLinkProgramARB=NULL;
PFNGLVALIDATEPROGRAMARBPROC glValidateProgramARB=NULL;
PFNGLUNIFORM1FARBPROC glUniform1fARB=NULL;
PFNGLUNIFORM2FARBPROC glUniform2fARB=NULL;
PFNGLUNIFORM3FARBPROC glUniform3fARB=NULL;
PFNGLUNIFORM4FARBPROC glUniform4fARB=NULL;
PFNGLUNIFORM1IARBPROC glUniform1iARB=NULL;
PFNGLUNIFORM2IARBPROC glUniform2iARB=NULL;
PFNGLUNIFORM3IARBPROC glUniform3iARB=NULL;
PFNGLUNIFORM4IARBPROC glUniform4iARB=NULL;
PFNGLUNIFORM1FVARBPROC glUniform1fvARB=NULL;
PFNGLUNIFORM2FVARBPROC glUniform2fvARB=NULL;
PFNGLUNIFORM3FVARBPROC glUniform3fvARB=NULL;
PFNGLUNIFORM4FVARBPROC glUniform4fvARB=NULL;
PFNGLUNIFORM1IVARBPROC glUniform1ivARB=NULL;
PFNGLUNIFORM2IVARBPROC glUniform2ivARB=NULL;
PFNGLUNIFORM3IVARBPROC glUniform3ivARB=NULL;
PFNGLUNIFORM4IVARBPROC glUniform4ivARB=NULL;
PFNGLUNIFORMMATRIX2FVARBPROC glUniformMatrix2fvARB=NULL;
PFNGLUNIFORMMATRIX3FVARBPROC glUniformMatrix3fvARB=NULL;
PFNGLUNIFORMMATRIX4FVARBPROC glUniformMatrix4fvARB=NULL;
PFNGLGETOBJECTPARAMETERFVARBPROC glGetObjectParameterfvARB=NULL;
PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB=NULL;
PFNGLGETINFOLOGARBPROC glGetInfoLogARB=NULL;
PFNGLGETATTACHEDOBJECTSARBPROC glGetAttachedObjectsARB=NULL;
PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB=NULL;
PFNGLGETACTIVEUNIFORMARBPROC glGetActiveUniformARB=NULL;
PFNGLGETUNIFORMFVARBPROC glGetUniformfvARB=NULL;
PFNGLGETUNIFORMIVARBPROC glGetUniformivARB=NULL;
PFNGLGETSHADERSOURCEARBPROC glGetShaderSourceARB=NULL;
PFNGLBINDATTRIBLOCATIONARBPROC glBindAttribLocationARB=NULL;
PFNGLGETACTIVEATTRIBARBPROC glGetActiveAttribARB=NULL;
PFNGLGETATTRIBLOCATIONARBPROC glGetAttribLocationARB=NULL;
PFNGLBUFFERDATAPROC glBufferDataARB=NULL;

PFNGLVERTEXATTRIBPOINTERARBPROC glVertexAttribPointerARB=NULL;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB=NULL;
PFNGLGETVERTEXATTRIBIVARBPROC glGetVertexAttribivARB=NULL;
PFNGLGETVERTEXATTRIBPOINTERVARBPROC glGetVertexAttribPointervARB=NULL;


#include "ScreenCopyGL.h"

#elif defined(ORIGIN_MAC) // MAC OSX

#include <dlfcn.h>
#import <AGL/Agl.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>
#include <ApplicationServices/ApplicationServices.h>

#include "MacInputHook.h"
#include "MacCursorHook.h"
#include "MacRenderHook.h"

namespace OriginIGO
{
    extern BYTE gIGOKeyboardData[256];
}
#endif

#include "HookAPI.h"
#include "IGOApplication.h"

#include "EASTL/hash_map.h"
#include "IGOTrace.h"

namespace OriginIGO {

    extern bool gInputHooked;
    extern volatile bool gQuitting;
    extern bool gForceUnloadIGO;

#if defined(ORIGIN_PC)
    extern volatile DWORD gPresentHookCalled;
    static GLint gCurrentStateViewport[4] = {0};
    static int gCurrentStateUnpackBufferBindID = 0;
    static GLint gCurrentStateProgramID = 0;
    static GLint gMaxSharedTextureUnits = 0;
    static GLint gMaxSharedTextureCoordUnits = 0;    
    static GLint gMaxFixedPipelineTextureUnits = 0;

    static void CreateScreenCopyGL(HWND window, OGLHook::DevContext *context = NULL)
    {
        if(gScreenCopy == NULL)
        {
            gScreenCopy = new ScreenCopyGL();
            gScreenCopy->Create((void*)&window, (void*)context);
        }
    }

    static void DestroyScreenCopyGL()
    {
        if(gScreenCopy)
        {
            gScreenCopy->Destroy();
            delete gScreenCopy;
            gScreenCopy = NULL;
        }
    }
#endif // ORIGIN_PC
}

using namespace OriginIGO;

#define OGLFN(retval, name, params) \
    typedef retval (APIENTRY *name ## Fn) params; \
    name ## Fn _ ## name = NULL;

#define OGLFN_OPTIONAL(retval, name, params) \
    typedef retval (APIENTRY *name ## Fn) params; \
    name ## Fn _ ## name = NULL;

#define GETFN(name, handle) \
    _ ## name = (name ## Fn)GetProcAddress(handle, #name)

#ifdef DEBUG
#define GL(x) _ ## x; \
    if (SHOW_GL_ERRORS) { \
    uint32_t error = _glGetError(); \
    if (error != GL_NO_ERROR) { OriginIGO::IGOLogDebug("%s:%d glError(0x%x) %s", __FILE__, __LINE__, error, #x); } \
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
        if (error != GL_NO_ERROR) { OriginIGO::IGOLogDebug("%s:%d glError(0x%x) %s", __FILE__, __LINE__, error, #fcn); } \
        IGO_ASSERT(error == GL_NO_ERROR); \
        } \
    }

#else
#define GL_EXT(fcn, ...) \
    if ( ## fcn) \
         ## fcn(__VA_ARGS__);

#endif

#if defined(ORIGIN_PC)

HANDLE OGLHook::mIGOOGLHookDestroyThreadHandle = NULL;
HANDLE OGLHook::mIGOOGLHookCreateThreadHandle = NULL;
EA::Thread::Futex OGLHook::mInstanceHookMutex;
bool OGLHook::isHooked = false;
DWORD OGLHook::mLastHookTime = 0;

OGLHook::DevContext gContext = {0};

#include "OGLFunctions.h"

static bool saveRenderState()
{
    SAFE_CALL_LOCK_ENTER
    uint32_t width = SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenWidth);
    uint32_t height = SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenHeight);
    SAFE_CALL_LOCK_LEAVE

    GL(glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS));
    GL(glPushAttrib(GL_ALL_ATTRIB_BITS));

    GL(glGetIntegerv(GL_VIEWPORT, gCurrentStateViewport));
    gCurrentStateProgramID = 0;

    bool isOpenGL11 = gContext.version <= 11;
    if (!isOpenGL11)
    {
        GL(glGetIntegerv(GL_CURRENT_PROGRAM, &gCurrentStateProgramID));
    }
    
    // setup matrix
    GL(glMatrixMode(GL_MODELVIEW));
    GL(glPushMatrix());
    GL(glLoadIdentity());
    GL(glMatrixMode(GL_PROJECTION));
    GL(glPushMatrix());

    float orthoMatrix[16];
    ZeroMemory(orthoMatrix, sizeof(orthoMatrix));
    orthoMatrix[0] = 2.0f/width;
    orthoMatrix[5] = 2.0f/height;
    orthoMatrix[10] = -2.0f/(9);
    orthoMatrix[14] =2.0f/(9);
    orthoMatrix[15] = 1.0f;
    GL(glLoadMatrixf(orthoMatrix));

    GL(glViewport(0, 0, width, height));

    GL(glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR));
    GL(glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR));
    GL(glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR));
    GL(glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR));
    GL(glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE)); 
    GL(glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE)); 
    GL(glPixelStorei(GL_UNPACK_ROW_LENGTH , 0)); 
    GL(glPixelStorei(GL_UNPACK_SKIP_ROWS, 0)); 
    GL(glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0)); 
    GL(glPixelStorei(GL_UNPACK_ALIGNMENT, 4)); 
    GL(glPixelStorei(GL_PACK_SWAP_BYTES, GL_FALSE)); 
    GL(glPixelStorei(GL_PACK_LSB_FIRST, GL_FALSE)); 
    GL(glPixelStorei(GL_PACK_ROW_LENGTH, 0)); 
    GL(glPixelStorei(GL_PACK_SKIP_ROWS, 0)); 
    GL(glPixelStorei(GL_PACK_SKIP_PIXELS, 0));
    GL(glPixelStorei(GL_PACK_ALIGNMENT, 4));
    GL(glPixelTransferi(GL_MAP_COLOR, GL_FALSE));
    GL(glPixelTransferi(GL_MAP_STENCIL, GL_FALSE));
    GL(glPixelTransferi(GL_INDEX_SHIFT, 0));
    GL(glPixelTransferi(GL_INDEX_OFFSET, 0));
    GL(glPixelTransferf(GL_RED_SCALE, 1.0f));
    GL(glPixelTransferf(GL_GREEN_SCALE, 1.0f));
    GL(glPixelTransferf(GL_BLUE_SCALE, 1.0f));
    GL(glPixelTransferf(GL_ALPHA_SCALE, 1.0f));
    GL(glPixelTransferf(GL_DEPTH_SCALE, 1.0f));
    GL(glPixelTransferf(GL_RED_BIAS, 0.0f));
    GL(glPixelTransferf(GL_GREEN_BIAS, 0.0f));
    GL(glPixelTransferf(GL_BLUE_BIAS, 0.0f));
    GL(glPixelTransferf(GL_ALPHA_BIAS, 0.0f));
    GL(glPixelTransferf(GL_DEPTH_BIAS, 0.0f));    
    GL(glPixelZoom(1.0f, 1.0f));
    GL(glDisable(GL_POLYGON_OFFSET_FILL));
    GL(glDisable(GL_POLYGON_OFFSET_LINE));
    GL(glDisable(GL_POLYGON_OFFSET_POINT));
    GL(glDisable(GL_NORMALIZE));
    GL(glDisable(GL_POLYGON_OFFSET_EXT));
    GL(glDisable(GL_DEPTH_TEST));
    GL(glDisable(GL_LOGIC_OP));
    GL(glDisable(GL_CULL_FACE));
    GL(glDisable(GL_LIGHTING));
    GL(glDisable(GL_FOG));
    GL(glDisable(GL_SCISSOR_TEST)); 
    GL(glDisable(GL_ALPHA_TEST));
    GL(glDisable(GL_STENCIL_TEST));
    GL(glDisable(GL_LINE_STIPPLE));
    GL(glDisable(GL_COLOR_MATERIAL));
    GL(glStencilMask(FALSE));
    GL(glColorMask(TRUE, TRUE, TRUE, TRUE));
    GL(glDepthMask(FALSE));
    GL(glAlphaFunc(GL_ALWAYS, 0));
    GL(glDepthFunc(GL_ALWAYS));
    GL(glShadeModel(GL_SMOOTH));
    GL(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    GL(glDisable(GL_AUTO_NORMAL));
    GL(glDisable(GL_COLOR_LOGIC_OP));
    GL(glDisable(GL_DITHER));
    GL(glDisable(GL_INDEX_LOGIC_OP));
    GL(glRenderMode(GL_RENDER));
    
    GL(glDisableClientState(GL_VERTEX_ARRAY));
    GL(glDisableClientState(GL_NORMAL_ARRAY));
    GL(glDisableClientState(GL_COLOR_ARRAY));
    GL(glDisableClientState(GL_INDEX_ARRAY));
    GL(glDisableClientState(GL_EDGE_FLAG_ARRAY));

    if (!isOpenGL11)
    {
        GL(glDisableClientState(GL_SECONDARY_COLOR_ARRAY));
        GL(glDisableClientState(GL_FOG_COORD_ARRAY));

        GL(glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT));
        GL(glDisable(GL_ACTIVE_STENCIL_FACE_EXT));
        GL(glDisable(GL_COLOR_TABLE));
        GL(glDisable(GL_CONVOLUTION_1D));
        GL(glDisable(GL_CONVOLUTION_2D));
        GL(glDisable(GL_HISTOGRAM));
        GL(glDisable(GL_MINMAX));
        GL(glDisable(GL_SEPARABLE_2D));   
    }

    for (GLint idx = 0; idx < gMaxSharedTextureCoordUnits; ++idx)
    {
        GL_EXT(glClientActiveTextureARB, GL_TEXTURE0 + idx);
        GL(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
    }
    
    GL(glPixelStorei(GL_UNPACK_SWAP_BYTES, 0));
    GL(glPixelStorei(GL_UNPACK_LSB_FIRST, 0));
    GL(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
    GL(glPixelStorei(GL_UNPACK_SKIP_ROWS, 0));
    GL(glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0));
    
    GLfloat color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    
    for (GLint i=gMaxSharedTextureUnits-1;i>=0;--i) 
    {
        GL_EXT(glActiveTextureARB, GL_TEXTURE0 + i);
        GL(glBindTexture(GL_TEXTURE_1D, 0));
        GL(glBindTexture(GL_TEXTURE_2D, 0));

        if (!isOpenGL11)
        {
                GL(glBindTexture(GL_TEXTURE_3D, 0));
                GL(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
                GL(glBindTexture(GL_TEXTURE_1D_ARRAY_EXT, 0));
                GL(glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, 0));
        }

        if (i < gMaxFixedPipelineTextureUnits)
        {
            GL(glDisable(GL_TEXTURE_1D));
            GL(glDisable(GL_TEXTURE_2D));
            
            GL(glDisable(GL_TEXTURE_GEN_S));
            GL(glDisable(GL_TEXTURE_GEN_T));
            GL(glDisable(GL_TEXTURE_GEN_R));
            GL(glDisable(GL_TEXTURE_GEN_Q));
            
            GL(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE));
            GL(glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color));

            if (!isOpenGL11)
            {
                GL(glDisable(GL_TEXTURE_3D));
                GL(glTexEnvi(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, 0));
            }
        }
    }

    if (!isOpenGL11)
    {
        GL(glDisable(GL_TEXTURE_CUBE_MAP));
        GL(glDisable(GL_VERTEX_PROGRAM_ARB));
        GL(glDisable(GL_FRAGMENT_PROGRAM_ARB));

        GL_EXT(glUseProgramObjectARB, 0);
    }
    
    GL_EXT(glActiveTextureARB, GL_TEXTURE0);
    GL(glEnable(GL_TEXTURE_2D));

    // Setup it up AFTER enabling Texture unit
    GL(glMatrixMode(GL_TEXTURE));
    GL(glPushMatrix());
    GL(glLoadIdentity());

    gCurrentStateUnpackBufferBindID = 0;
    if (!isOpenGL11)
    {
        GL(glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &gCurrentStateUnpackBufferBindID));
    }

    GL(glEnable(GL_BLEND));
    GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    GL(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE));

    GL(glMatrixMode(GL_MODELVIEW));
    return true;
}

static void restoreRenderState()
{
    GL(glMatrixMode(GL_MODELVIEW));
    GL(glPopMatrix());
    GL(glMatrixMode(GL_PROJECTION));
    GL(glPopMatrix());
    GL(glMatrixMode(GL_TEXTURE));
    GL(glPopMatrix());
    GL(glPopAttrib());
    GL(glPopClientAttrib());
    
    if (gCurrentStateUnpackBufferBindID)
    {
        GL_EXT(glBindBufferARB, GL_PIXEL_UNPACK_BUFFER_ARB, gCurrentStateUnpackBufferBindID);
    }
    
    GL(glViewport(gCurrentStateViewport[0], gCurrentStateViewport[1], gCurrentStateViewport[2], gCurrentStateViewport[3]));

    bool isOpenGL11 = gContext.version <= 11;
    if (!isOpenGL11)
    {
        GL_EXT(glUseProgramObjectARB, gCurrentStateProgramID);
    }
}




void Init()
{
    if (!gInputHooked && !gQuitting)
        InputHook::TryHookLater(&gInputHooked);

    // create the IGO IGOApplication::instance()
    if (!IGOApplication::instance())
        IGOApplication::createInstance(RendererOGL, "OpenGL has no renderer device!");
}
OGLHook::OGLHook()
{
    isHooked = false;
}

OGLHook::~OGLHook()
{
}

static bool OGLLoadFunctions(HMODULE hOGL)
{
#undef OGLFN
#define OGLFN(retval, name, params) \
    _ ## name = (name ## Fn)GetProcAddress(hOGL, #name); \
if (! _ ## name) \
{ \
    OriginIGO::IGOLogWarn("OpenGL function missing: %s", #name); \
    return false; \
}

#include "OGLFunctions.h"

#undef OGLFN

    return true;
}

static void Init(HWND hWnd)
{
    if (!gInputHooked && !gQuitting)
        InputHook::TryHookLater(&gInputHooked);

    // create the IGO IGOApplication::instance()
    // recreate the API hooks

    if (!IGOApplication::instance())
        IGOApplication::createInstance(RendererOGL, hWnd);
    

    // check for resize
    RECT rect;
    GetClientRect(hWnd, &rect);

    int newWidth = rect.right - rect.left;
    int newHeight = rect.bottom - rect.top;

    SAFE_CALL_LOCK_ENTER
    SAFE_CALL(IGOApplication::instance(), &IGOApplication::setScreenSize, newWidth, newHeight);
    SAFE_CALL_LOCK_LEAVE
    
    // Time to turn off logging if the user hasn't explicitely enabled it from the client preferences
    static bool loggingChecked = false;
    if (!loggingChecked)
    {
        loggingChecked = true;
        OriginIGO::IGOLogger::instance()->enableLogging(getIGOConfigurationFlags().gEnabledLogging);
    }
}

static void UpdateOpenGLContext(HDC hdc)
{
    static HGLRC currentContext = NULL;
    
    if (currentContext == _wglGetCurrentContext() )
        return;

    currentContext = _wglGetCurrentContext();

    // Are we using the base OpenGL 1.1 specs (ie this is only for automation)?
    gContext.version = 10;
    const unsigned char* version = GL(glGetString(GL_VERSION));
    if (version)
    {
        OriginIGO::IGOLogInfo("New context using OpenGL Version = %s", version);
        gContext.version = (GLint)(atof((const char*)version) * 10.0);
    }

    // Setup minimum texture support by default in case some of the checks fail
    gMaxSharedTextureUnits = 1;
    gMaxSharedTextureCoordUnits = 1;
    gMaxFixedPipelineTextureUnits = 1;

    bool isOpenGL11 = gContext.version <= 11;
    if (!isOpenGL11)
    {
        GL(glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &gMaxSharedTextureUnits));
        GL(glGetIntegerv(GL_MAX_TEXTURE_COORDS, &gMaxSharedTextureCoordUnits));
        GL(glGetIntegerv(GL_MAX_TEXTURE_UNITS, &gMaxFixedPipelineTextureUnits));
    }

    else
    {
        GL(glGetIntegerv(GL_MAX_TEXTURE_UNITS, &gMaxFixedPipelineTextureUnits));
        gMaxSharedTextureUnits = gMaxSharedTextureCoordUnits = gMaxFixedPipelineTextureUnits;
    }

    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLGENFRAMEBUFFERSEXTPROC, glGenFramebuffersEXT);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLBINDFRAMEBUFFEREXTPROC, glBindFramebufferEXT);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLRENDERBUFFERSTORAGEEXTPROC, glRenderbufferStorageEXT);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC, glCheckFramebufferStatusEXT);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLDELETEFRAMEBUFFERSEXTPROC, glDeleteFramebuffersEXT);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLFRAMEBUFFERTEXTURE2DEXTPROC, glFramebufferTexture2DEXT);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLBLENDFUNCSEPARATEPROC, glBlendFuncSeparate);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLBLENDEQUATIONSEPARATEPROC, glBlendEquationSeparate);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLACTIVETEXTUREARBPROC, glActiveTextureARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLCLIENTACTIVETEXTUREARBPROC, glClientActiveTextureARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLBINDBUFFERARBPROC, glBindBufferARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLMULTITEXCOORD4FARBPROC, glMultiTexCoord4fARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLBINDPROGRAMARBPROC, glBindProgramARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLSECONDARYCOLORPOINTERPROC, glSecondaryColorPointer);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLFOGCOORDPOINTEREXTPROC, glFogCoordPointerEXT);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLDRAWBUFFERSARBPROC, glDrawBuffersARB);

    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLDELETEBUFFERSARBPROC, glDeleteBuffersARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLGENBUFFERSARBPROC, glGenBuffersARB);

    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLBLENDEQUATIONEXTPROC, glBlendEquationEXT);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLDELETEOBJECTARBPROC, glDeleteObjectARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLGETHANDLEARBPROC, glGetHandleARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLDETACHOBJECTARBPROC, glDetachObjectARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLCREATESHADEROBJECTARBPROC, glCreateShaderObjectARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLSHADERSOURCEARBPROC, glShaderSourceARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLCOMPILESHADERARBPROC, glCompileShaderARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLCREATEPROGRAMOBJECTARBPROC, glCreateProgramObjectARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLATTACHOBJECTARBPROC, glAttachObjectARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLLINKPROGRAMARBPROC, glLinkProgramARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUSEPROGRAMOBJECTARBPROC, glUseProgramObjectARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLVALIDATEPROGRAMARBPROC, glValidateProgramARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUNIFORM1FARBPROC, glUniform1fARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUNIFORM2FARBPROC, glUniform2fARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUNIFORM3FARBPROC, glUniform3fARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUNIFORM4FARBPROC, glUniform4fARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUNIFORM1IARBPROC, glUniform1iARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUNIFORM2IARBPROC, glUniform2iARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUNIFORM3IARBPROC, glUniform3iARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUNIFORM4IARBPROC, glUniform4iARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUNIFORM1FVARBPROC, glUniform1fvARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUNIFORM2FVARBPROC, glUniform2fvARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUNIFORM3FVARBPROC, glUniform3fvARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUNIFORM4FVARBPROC, glUniform4fvARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUNIFORM1IVARBPROC, glUniform1ivARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUNIFORM2IVARBPROC, glUniform2ivARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUNIFORM3IVARBPROC, glUniform3ivARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUNIFORM4IVARBPROC, glUniform4ivARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUNIFORMMATRIX2FVARBPROC, glUniformMatrix2fvARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUNIFORMMATRIX3FVARBPROC, glUniformMatrix3fvARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLUNIFORMMATRIX4FVARBPROC, glUniformMatrix4fvARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLGETOBJECTPARAMETERFVARBPROC, glGetObjectParameterfvARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLGETOBJECTPARAMETERIVARBPROC, glGetObjectParameterivARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLGETINFOLOGARBPROC, glGetInfoLogARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLGETATTACHEDOBJECTSARBPROC, glGetAttachedObjectsARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLGETUNIFORMLOCATIONARBPROC, glGetUniformLocationARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLGETACTIVEUNIFORMARBPROC, glGetActiveUniformARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLGETUNIFORMFVARBPROC, glGetUniformfvARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLGETUNIFORMIVARBPROC, glGetUniformivARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLGETSHADERSOURCEARBPROC, glGetShaderSourceARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLBINDATTRIBLOCATIONARBPROC, glBindAttribLocationARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLGETACTIVEATTRIBARBPROC, glGetActiveAttribARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLGETATTRIBLOCATIONARBPROC, glGetAttribLocationARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLBUFFERDATAPROC, glBufferDataARB);

    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLVERTEXATTRIBPOINTERARBPROC, glVertexAttribPointerARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLENABLEVERTEXATTRIBARRAYARBPROC, glEnableVertexAttribArrayARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLGETVERTEXATTRIBIVARBPROC, glGetVertexAttribivARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLGETVERTEXATTRIBPOINTERVARBPROC, glGetVertexAttribPointervARB);
    QUERY_EXTENSION_ENTRY_POINT_GL3(PFNGLDISABLEVERTEXATTRIBARRAYARBPROC, glDisableVertexAttribArrayARB);


    // Reset context
    if (gContext.windowShader)
        delete gContext.windowShader;
    gContext.windowShader = NULL;

    if (gContext.bgWindowShader)
        delete gContext.bgWindowShader;
    gContext.bgWindowShader = NULL;

    gContext.maxTextureSize = 0;
    GL(glGetIntegerv(GL_MAX_TEXTURE_SIZE, &gContext.maxTextureSize));
    if (gContext.maxTextureSize <= 64)
        gContext.maxTextureSize = 64;

    if (!isOpenGL11)
    {
        gContext.windowShader = new OriginIGO::IGOWindowOGLShader();
        gContext.bgWindowShader = new OriginIGO::IGOBackgroundOGLShader();
        if (!gContext.windowShader || !gContext.windowShader->IsValid()
            || !gContext.bgWindowShader || !gContext.bgWindowShader->IsValid())
        {
            delete gContext.windowShader;
            gContext.windowShader = NULL;

            delete gContext.bgWindowShader;
            gContext.bgWindowShader = NULL;
        }
    }

    if (gContext.windowShader && gContext.bgWindowShader)
        OriginIGO::IGOLogInfo("Using OpenGL shaders...");
    else
        OriginIGO::IGOLogInfo("Using OpenGL fixed-pipeline...");
}



struct OpenGLState
{
    OpenGLState()
    {
        memset(this, 0, sizeof(*this));
    }
    
    GLint programID;
    GLint viewport[4];
    
    GLint texID_1D;
    GLint texID_1D_ARRAY;
    GLint texID_2D;
    GLint texID_2D_MULTISAMPLE;     // Core only
    GLint texID_2D_ARRAY;
    GLint texID_3D;
    GLint texID_CUBE_MAP;
    GLint texID_RECT;
    GLint texID_BUFFER;             // Core only
    
    GLint drawFramebuffer;          // Core only
    GLint readFramebuffer;          // Core only
    GLint bufferID_ARRAY_BUFFER;
    GLint bufferID_ELEMENT_BUFFER;
    GLint bufferID_PIXEL_UNPACK;
    
    GLint vertexAttrib0Enabled;
    GLint vertexAttrib0Size;
    GLint vertexAttrib0Stride;
    GLint vertexAttrib0Type;
    GLint vertexAttrib0Normalized;
    void* vertexAttrib0Pointer;
    
    GLint texUnitIdx;
    GLint currTexUnitID;
    GLint currTexRangeLength;
    GLvoid* currTexRangePointer;

    GLint blendEqRgb;
    GLint blendSrcRgb;
    GLint blendDstRgb;
    GLint blendEqAlpha;
    GLint blendSrcAlpha;
    GLint blendDstAlpha;
    
    GLint unpackAlignment;
    GLint unpackImageHeight;
    GLint unpackRowLength;
    GLint unpackSkipImages;
    GLint unpackSkipPixels;
    GLint unpackSkipRows;
    
    GLboolean unpackLsbFirst;
    GLboolean unpackSwapBytes;
    
    GLboolean depthTestOn;
    GLboolean cullFaceOn;
    GLboolean scissorTestOn;
    GLboolean alphaTestOn;
    GLboolean blendOn;
    GLboolean sRGBOn;
    GLboolean stencilTestOn;
    GLboolean depthMaskOn;
    GLboolean colorMask[4];
};

static void SaveShaderRenderState(OpenGLState* oglState)
{    
    GL(glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS));
    GL(glPushAttrib(GL_ALL_ATTRIB_BITS));
    
    GL(glGetIntegerv(GL_VIEWPORT, oglState->viewport));
    GL(glGetIntegerv(GL_CURRENT_PROGRAM, &oglState->programID));
    
    uint32_t width = IGOApplication::instance()->getScreenWidth();
    uint32_t height = IGOApplication::instance()->getScreenHeight();    
    GL(glViewport(0, 0, width, height));
    
    //
    //
    
    GL(glGetBooleanv(GL_DEPTH_TEST, &oglState->depthTestOn));
    if (oglState->depthTestOn)
        GL(glDisable(GL_DEPTH_TEST));
    
    GL(glGetBooleanv(GL_CULL_FACE, &oglState->cullFaceOn));
    if (oglState->cullFaceOn)
        GL(glDisable(GL_CULL_FACE));
    
    GL(glGetBooleanv(GL_SCISSOR_TEST, &oglState->scissorTestOn));
    if (oglState->scissorTestOn)
        GL(glDisable(GL_SCISSOR_TEST));
    
    GL(glGetBooleanv(GL_ALPHA_TEST, &oglState->alphaTestOn));
    if (oglState->alphaTestOn)
        GL(glDisable(GL_ALPHA_TEST));
    
    GL(glGetBooleanv(GL_BLEND, &oglState->blendOn));
    if (!oglState->blendOn)
        GL(glEnable(GL_BLEND));
    
    GL(glGetBooleanv(GL_FRAMEBUFFER_SRGB_EXT, &oglState->sRGBOn));
    if (oglState->sRGBOn)
        GL(glDisable(GL_FRAMEBUFFER_SRGB_EXT));
    
    oglState->blendSrcRgb = GL_ONE;
    oglState->blendDstRgb = GL_ZERO;
    oglState->blendSrcAlpha = GL_ONE;
    oglState->blendDstAlpha = GL_ZERO;
    GL(glGetIntegerv(GL_BLEND_SRC_RGB, &oglState->blendSrcRgb));
    GL(glGetIntegerv(GL_BLEND_DST_RGB, &oglState->blendDstRgb));
    GL(glGetIntegerv(GL_BLEND_SRC_ALPHA, &oglState->blendSrcAlpha));
    GL(glGetIntegerv(GL_BLEND_DST_ALPHA, &oglState->blendDstAlpha));
    if (oglState->blendSrcRgb != GL_SRC_ALPHA || oglState->blendDstRgb != GL_ONE_MINUS_SRC_ALPHA
         || oglState->blendSrcAlpha != GL_SRC_ALPHA || oglState->blendDstAlpha != GL_ONE_MINUS_SRC_ALPHA)
        GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    
    oglState->blendEqRgb = GL_FUNC_ADD;
    oglState->blendEqAlpha = GL_FUNC_ADD;
    GL(glGetIntegerv(GL_BLEND_EQUATION_RGB, &oglState->blendEqRgb));
    GL(glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &oglState->blendEqAlpha));
    if (oglState->blendEqRgb != GL_FUNC_ADD || oglState->blendEqAlpha != GL_FUNC_ADD)
        GL_EXT(glBlendEquationEXT, GL_FUNC_ADD);
    
    GL(glGetBooleanv(GL_STENCIL_TEST, &oglState->stencilTestOn));
    if (oglState->stencilTestOn)
        GL(glDisable(GL_STENCIL_TEST));
    
    GL(glGetBooleanv(GL_COLOR_WRITEMASK, oglState->colorMask));
    if ((oglState->colorMask[0] & oglState->colorMask[1] & oglState->colorMask[2] & oglState->colorMask[3]) == GL_FALSE)
        GL(glColorMask(TRUE, TRUE, TRUE, TRUE));
    
    GL(glGetBooleanv(GL_DEPTH_WRITEMASK, &oglState->depthMaskOn));
    if (oglState->depthMaskOn)
        GL(glDepthMask(GL_FALSE));
    
    GL(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    
    oglState->texUnitIdx = gMaxSharedTextureUnits - 1;
    if (oglState->texUnitIdx < 0)
        oglState->texUnitIdx = 0;
    
    oglState->currTexUnitID = GL_TEXTURE0;
    GL(glGetIntegerv(GL_ACTIVE_TEXTURE, &oglState->currTexUnitID));
    GL_EXT(glActiveTextureARB, GL_TEXTURE0 + oglState->texUnitIdx);
    
    GLint clipPlaneCnt = 0;
    GL(glGetIntegerv(GL_MAX_CLIP_PLANES, &clipPlaneCnt));
    for (GLint idx = 0; idx < clipPlaneCnt; ++idx)
    {
        GL(glDisable(GL_CLIP_PLANE0 + idx));
    }
    
    GL(glGetIntegerv(GL_TEXTURE_BINDING_1D, &oglState->texID_1D));
    if (oglState->texID_1D)
        GL(glBindTexture(GL_TEXTURE_BINDING_1D, 0));
    
    GL(glGetIntegerv(GL_TEXTURE_BINDING_1D_ARRAY_EXT, &oglState->texID_1D_ARRAY));
    if (oglState->texID_1D_ARRAY)
        GL(glBindTexture(GL_TEXTURE_BINDING_1D_ARRAY_EXT, 0));

    GL(glGetIntegerv(GL_TEXTURE_BINDING_2D, &oglState->texID_2D));
    if (oglState->texID_2D)
        GL(glBindTexture(GL_TEXTURE_BINDING_2D, 0));
    
    GL(glGetIntegerv(GL_TEXTURE_BINDING_2D_MULTISAMPLE, &oglState->texID_2D_MULTISAMPLE));
    
    GL(glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY_EXT, &oglState->texID_2D_ARRAY));
    if (oglState->texID_2D_ARRAY)
        GL(glBindTexture(GL_TEXTURE_BINDING_2D_ARRAY_EXT, 0));
    
    GL(glGetIntegerv(GL_TEXTURE_BINDING_3D, &oglState->texID_3D));
    if (oglState->texID_3D)
        GL(glBindTexture(GL_TEXTURE_BINDING_3D, 0));
    
    GL(glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &oglState->texID_CUBE_MAP));
    if (oglState->texID_CUBE_MAP)
        GL(glBindTexture(GL_TEXTURE_BINDING_CUBE_MAP, 0));
    
    GL(glGetIntegerv(GL_TEXTURE_BINDING_RECTANGLE_ARB, &oglState->texID_RECT));
    if (oglState->texID_RECT)
        GL(glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0));
    
    GL(glGetIntegerv(GL_TEXTURE_BINDING_BUFFER, &oglState->texID_BUFFER));
    
    GL(glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oglState->drawFramebuffer));
    GL(glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &oglState->readFramebuffer));
    if (oglState->drawFramebuffer || oglState->readFramebuffer)
        GL_EXT(glBindFramebufferEXT, GL_FRAMEBUFFER, 0);
    
    GL(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &oglState->bufferID_ARRAY_BUFFER));
    GL(glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &oglState->bufferID_ELEMENT_BUFFER));
    GL_EXT(glGetVertexAttribivARB, 0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &oglState->vertexAttrib0Enabled);
    GL_EXT(glGetVertexAttribivARB, 0, GL_VERTEX_ATTRIB_ARRAY_SIZE, &oglState->vertexAttrib0Size);
    GL_EXT(glGetVertexAttribivARB, 0, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &oglState->vertexAttrib0Stride);
    GL_EXT(glGetVertexAttribivARB, 0, GL_VERTEX_ATTRIB_ARRAY_TYPE, &oglState->vertexAttrib0Type);
    GL_EXT(glGetVertexAttribivARB, 0, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &oglState->vertexAttrib0Normalized);
    GL_EXT(glGetVertexAttribPointervARB, 0, GL_VERTEX_ATTRIB_ARRAY_POINTER, &oglState->vertexAttrib0Pointer);
    
    GL(glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &oglState->bufferID_PIXEL_UNPACK));
    if (oglState->bufferID_PIXEL_UNPACK)
        GL(glBindTexture(GL_PIXEL_UNPACK_BUFFER_BINDING, 0));
    
    GL(glGetIntegerv(GL_UNPACK_ALIGNMENT, &oglState->unpackAlignment));
    if (oglState->unpackAlignment != 4)
        GL(glPixelStorei(GL_UNPACK_ALIGNMENT, 4));
    
    GL(glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &oglState->unpackImageHeight));
    if (oglState->unpackImageHeight)
        GL(glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0));
    
    GL(glGetBooleanv(GL_UNPACK_LSB_FIRST, &oglState->unpackLsbFirst));
    if (oglState->unpackLsbFirst)
        GL(glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE));
    
    GL(glGetIntegerv(GL_UNPACK_ROW_LENGTH, &oglState->unpackRowLength));
    if (oglState->unpackRowLength)
        GL(glPixelStorei(GL_UNPACK_ROW_LENGTH , 0));
    
    GL(glGetIntegerv(GL_UNPACK_SKIP_IMAGES, &oglState->unpackSkipImages));
    if (oglState->unpackSkipImages)
        GL(glPixelStorei(GL_UNPACK_SKIP_IMAGES, 0));
    
    GL(glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &oglState->unpackSkipPixels));
    if (oglState->unpackSkipPixels)
        GL(glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0));
    
    GL(glGetIntegerv(GL_UNPACK_SKIP_ROWS, &oglState->unpackSkipRows));
    if (oglState->unpackSkipRows)
        GL(glPixelStorei(GL_UNPACK_SKIP_ROWS, 0));
    
    GL(glGetBooleanv(GL_UNPACK_SWAP_BYTES, &oglState->unpackSwapBytes));
    if (oglState->unpackSwapBytes)
        GL(glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE));    
}

static void RestoreShaderRenderState(const OpenGLState& oglState)
{
    GL_EXT(glBindFramebufferEXT, GL_DRAW_FRAMEBUFFER, oglState.drawFramebuffer);
    GL_EXT(glBindFramebufferEXT, GL_READ_FRAMEBUFFER, oglState.readFramebuffer);
    
    
    GL(glViewport(oglState.viewport[0], oglState.viewport[1], oglState.viewport[2], oglState.viewport[3]));
    GL_EXT(glUseProgramObjectARB, oglState.programID);
    
    GL_EXT(glBindBufferARB, GL_ARRAY_BUFFER, oglState.bufferID_ARRAY_BUFFER);
    GL_EXT(glBindBufferARB, GL_ELEMENT_ARRAY_BUFFER, oglState.bufferID_ELEMENT_BUFFER);
    if (oglState.vertexAttrib0Enabled == GL_FALSE)
    {
        GL_EXT(glDisableVertexAttribArrayARB, 0);
    }
    
    else
        GL_EXT(glVertexAttribPointerARB, 0, oglState.vertexAttrib0Size, oglState.vertexAttrib0Type, oglState.vertexAttrib0Normalized != 0, oglState.vertexAttrib0Stride, oglState.vertexAttrib0Pointer);
    
    if (oglState.bufferID_PIXEL_UNPACK)
        GL_EXT(glBindBufferARB, GL_PIXEL_UNPACK_BUFFER, oglState.bufferID_PIXEL_UNPACK);
    
    if (oglState.unpackAlignment != 4)
        GL(glPixelStorei(GL_UNPACK_ALIGNMENT, oglState.unpackAlignment));
    
    if (oglState.unpackImageHeight != 0)
        GL(glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, oglState.unpackImageHeight));
    
    if (oglState.unpackLsbFirst)
        GL(glPixelStorei(GL_UNPACK_LSB_FIRST, GL_TRUE));
    
    if (oglState.unpackRowLength != 0)
        GL(glPixelStorei(GL_UNPACK_ROW_LENGTH , oglState.unpackRowLength));
    
    if (oglState.unpackSkipImages != 0)
        GL(glPixelStorei(GL_UNPACK_SKIP_IMAGES, oglState.unpackSkipImages));
    
    if (oglState.unpackSkipPixels != 0)
        GL(glPixelStorei(GL_UNPACK_SKIP_PIXELS, oglState.unpackSkipPixels));
    
    if (oglState.unpackSkipRows != 0)
        GL(glPixelStorei(GL_UNPACK_SKIP_ROWS, oglState.unpackSkipRows));
    
    if (oglState.unpackSwapBytes)
        GL(glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE));
    
    if (oglState.texID_1D > 0)
        GL(glBindTexture(GL_TEXTURE_1D, oglState.texID_1D));
     
    if (oglState.texID_1D_ARRAY > 0)
        GL(glBindTexture(GL_TEXTURE_1D_ARRAY_EXT, oglState.texID_1D_ARRAY));
    
    if (oglState.texID_2D > 0) 
    GL(glBindTexture(GL_TEXTURE_2D, oglState.texID_2D));
     
    if (oglState.texID_2D_MULTISAMPLE > 0)
        GL(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, oglState.texID_2D_MULTISAMPLE));

    if (oglState.texID_2D_ARRAY > 0)
        GL(glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, oglState.texID_2D_ARRAY));
     
    if (oglState.texID_3D > 0)
        GL(glBindTexture(GL_TEXTURE_3D, oglState.texID_3D));
     
    if (oglState.texID_CUBE_MAP > 0)
        GL(glBindTexture(GL_TEXTURE_CUBE_MAP, oglState.texID_CUBE_MAP));
    
    if (oglState.texID_RECT)
        GL(glBindTexture(GL_TEXTURE_RECTANGLE_ARB, oglState.texID_RECT));
     
    if (oglState.texID_BUFFER > 0)
        GL(glBindTexture(GL_TEXTURE_BUFFER, oglState.texID_BUFFER));
    
    
    //
    //
    
    GL_EXT(glActiveTextureARB, oglState.currTexUnitID);
    
    if (oglState.depthMaskOn)
        GL(glDepthMask(GL_TRUE));
    
    if ((oglState.colorMask[0] & oglState.colorMask[1] & oglState.colorMask[2] & oglState.colorMask[3]) == GL_FALSE)
        GL(glColorMask(oglState.colorMask[0], oglState.colorMask[1], oglState.colorMask[2], oglState.colorMask[3]));
    
    if (oglState.stencilTestOn)
        GL(glEnable(GL_STENCIL_TEST));
    
    if (oglState.blendSrcRgb != GL_SRC_ALPHA || oglState.blendDstRgb != GL_ONE_MINUS_SRC_ALPHA
        || oglState.blendSrcAlpha != GL_SRC_ALPHA || oglState.blendDstAlpha != GL_ONE_MINUS_SRC_ALPHA)
        GL_EXT(glBlendFuncSeparate, oglState.blendSrcRgb, oglState.blendDstRgb, oglState.blendSrcAlpha, oglState.blendDstAlpha);
    
    if (oglState.blendEqRgb != GL_FUNC_ADD || oglState.blendEqAlpha != GL_FUNC_ADD)
        GL_EXT(glBlendEquationSeparate, oglState.blendEqRgb, oglState.blendEqAlpha);
    
    if (oglState.sRGBOn)
        GL(glEnable(GL_FRAMEBUFFER_SRGB_EXT));
    
    if (!oglState.blendOn)
        GL(glDisable(GL_BLEND));
    
    if (oglState.alphaTestOn)
        GL(glEnable(GL_ALPHA_TEST));
    
    if (oglState.scissorTestOn)
        GL(glEnable(GL_SCISSOR_TEST));
    
    if (oglState.cullFaceOn)
        GL(glEnable(GL_CULL_FACE));
    
    if (oglState.depthTestOn)
        GL(glEnable(GL_DEPTH_TEST));    

    GL(glPopAttrib());
    GL(glPopClientAttrib());
}


static bool IGOUpdate(HDC hdc)
{
    gPresentHookCalled = GetTickCount();

    HWND hWnd = WindowFromDC(hdc);
    if (!IsWindow(hWnd))
        return false;

    UpdateOpenGLContext(hdc);
    
    if (!IGOApplication::instance() && !gQuitting)
    {
        Init(hWnd);

        // hook windows message
        InputHook::HookWindow(hWnd);
        return false;
    }

    if( IGOApplication::isPendingDeletion())
    {
        IGOApplication::deleteInstance();
        DestroyScreenCopyGL();
        return false;
    }
    
    InputHook::HookWindow(hWnd);
        
    SAFE_CALL_LOCK_ENTER
    SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowedMode, gWindowedMode);
    SAFE_CALL_LOCK_LEAVE

    if (IGOApplication::instance())
    {
        // check for resize
        RECT rect;
        GetClientRect(hWnd, &rect);

        int newWidth = rect.right - rect.left;
        int newHeight = rect.bottom - rect.top;
        SAFE_CALL_LOCK_ENTER
            int width = SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenWidth);
        int height = SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenHeight);
        SAFE_CALL_LOCK_LEAVE
            if (newWidth != width || newHeight != height)
            {
                DestroyScreenCopyGL();
                SAFE_CALL_LOCK_ENTER
                    SAFE_CALL(IGOApplication::instance(), &IGOApplication::setScreenSize, newWidth, newHeight);
                SAFE_CALL_LOCK_LEAVE
            }

        // render OpenGL 3.2+
        if (gContext.windowShader != NULL)
        {
            // Render with OpenGL code 3.2 (shaders!) 
            OpenGLState oglState;
            SaveShaderRenderState(&oglState);

            if (TwitchManager::IsBroadCastingInitiated())
                CreateScreenCopyGL(hWnd, &gContext);
            else
                DestroyScreenCopyGL();

            if (TwitchManager::IsBroadCastingInitiated())
            {
                TwitchManager::TTV_PollTasks();
                if (TTV_SUCCEEDED(TwitchManager::IsReadyForStreaming()) && TwitchManager::IsBroadCastingRunning())
                {
                    // Setup shader
                    gContext.windowShader->Apply();

                    // Render
                    gContext.windowShader->SetTextureUnitID(oglState.texUnitIdx);
                    SAFE_CALL(gScreenCopy, &IScreenCopy::Render, (void*)&gContext, NULL);
                }
            }

            // Prepare for new rendering loop
            gContext.lastShader = NULL;
            gContext.texUnitIdx = oglState.texUnitIdx;

            // Go
            SAFE_CALL_LOCK_ENTER
                SAFE_CALL(IGOApplication::instance(), &IGOApplication::render, (void*)&gContext);
            SAFE_CALL_LOCK_LEAVE

            RestoreShaderRenderState(oglState);
        }
        else
        {
            // legacy rendering OpenGL < 3.2
            if (saveRenderState())
            {
                if (TwitchManager::IsBroadCastingInitiated())
                    CreateScreenCopyGL(hWnd);
                else
                    DestroyScreenCopyGL();

                if (TwitchManager::IsBroadCastingInitiated())
                {
                    TwitchManager::TTV_PollTasks();
                    if (TTV_SUCCEEDED(TwitchManager::IsReadyForStreaming()) && TwitchManager::IsBroadCastingRunning())
                    {
                        SAFE_CALL(gScreenCopy, &IScreenCopy::Render, NULL, NULL);
                    }
                }
                SAFE_CALL_LOCK_ENTER
                    SAFE_CALL(IGOApplication::instance(), &IGOApplication::render, (void*)&gContext);
                SAFE_CALL_LOCK_LEAVE
                    restoreRenderState();
            }
        }
    }

    return true;
}

// hooks
DEFINE_HOOK_SAFE(BOOL, wglSwapBuffersHook, (HDC hdc))

    IGOUpdate(hdc);
    BOOL r = wglSwapBuffersHookNext(hdc);
    return r;
}

DEFINE_HOOK_SAFE(BOOL, wglSwapLayerBuffersHook, (HDC hdc, UINT fuPlanes))

    IGOUpdate(hdc);
    BOOL r = wglSwapLayerBuffersHookNext(hdc, fuPlanes);
    return r;
}

DEFINE_HOOK_SAFE(LONG, ChangeDisplaySettingsAHook, (DEVMODE *lpDevMode, DWORD dwFlags))

    // call reset
    
    gWindowedMode = (dwFlags & CDS_FULLSCREEN) ? false : true;
    DestroyScreenCopyGL();
    IGOApplication::deleteInstance();
    return ChangeDisplaySettingsAHookNext(lpDevMode, dwFlags);
}

DEFINE_HOOK_SAFE(LONG, ChangeDisplaySettingsWHook, (DEVMODE *lpDevMode, DWORD dwFlags))

    // call reset
    
    gWindowedMode = (dwFlags & CDS_FULLSCREEN) ? false : true;
    DestroyScreenCopyGL();
    IGOApplication::deleteInstance();
    return ChangeDisplaySettingsWHookNext(lpDevMode, dwFlags);
}

DEFINE_HOOK_SAFE(BOOL, wglMakeCurrentHook, (HDC a, HGLRC b))

    // destroy IGO IF we are actually changing context!
    HDC prevDC = _wglGetCurrentDC();
    HGLRC prevGL = _wglGetCurrentContext();

    if (prevDC != a || prevGL != b)
    {
        DestroyScreenCopyGL();
        IGOApplication::deleteInstance();
    }

    return wglMakeCurrentHookNext(a, b);
}

static bool OGLInit(HMODULE hOGL)
{
    if (!OGLLoadFunctions(hOGL))
    {
        OriginIGO::IGOLogWarn("Hooking init failed - missing OpenGL methods");
        return false;
    }
    
    if (OGLHook::IsBroken())
        return false;

    // hook the functions
    //HOOKAPI_SAFE("gdi32.dll", "SwapBuffers", SwapBuffersHook); ->_wglSwapBuffers covers this call!!!
    HOOKAPI_SAFE("user32.dll", "ChangeDisplaySettingsA", ChangeDisplaySettingsAHook);
    HOOKAPI_SAFE("user32.dll", "ChangeDisplaySettingsW", ChangeDisplaySettingsWHook);

    UNHOOK_SAFE(wglSwapBuffersHook);
    UNHOOK_SAFE(wglSwapLayerBuffersHook);
    UNHOOK_SAFE(wglMakeCurrentHook);

    HOOKAPI_SAFE("opengl32.dll", "wglMakeCurrent", wglMakeCurrentHook);
    HOOKAPI_SAFE("opengl32.dll", "wglSwapBuffers", wglSwapBuffersHook);
    HOOKAPI_SAFE("opengl32.dll", "wglSwapLayerBuffers", wglSwapLayerBuffersHook);

    return (isChangeDisplaySettingsAHooked || isChangeDisplaySettingsWHooked) && iswglMakeCurrentHooked && iswglSwapBuffersHooked && iswglSwapLayerBuffersHooked;
}


static DWORD WINAPI IGOOGLHookCreateThread(LPVOID *lpParam)
{
    if (lpParam !=NULL)
        *((bool*)lpParam) = OGLHook::TryHook();
    else
        OGLHook::TryHook();

    CloseHandle(OGLHook::mIGOOGLHookCreateThreadHandle);
    OGLHook::mIGOOGLHookCreateThreadHandle = NULL;
    return 0;
}


void OGLHook::TryHookLater(bool *isHooked)
{
    if (OGLHook::mIGOOGLHookCreateThreadHandle==NULL)
        OGLHook::mIGOOGLHookCreateThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IGOOGLHookCreateThread, isHooked, 0, NULL);
}


    
bool OGLHook::TryHook()
{

    if (OGLHook::mInstanceHookMutex.TryLock())
    {
        mLastHookTime = GetTickCount();

        OriginIGO::IGOLogInfo("OGLHook::TryHook()");  
        // do not hook another GFX api, if we already have IGO running in this process!!!
        {
            SAFE_CALL_LOCK_AUTO
            if (IGOApplication::instance() && (IGOApplication::instance()->rendererType() != RendererOGL))
            {
                OriginIGO::IGOLogWarn("Already have hooked a different API.");
                OGLHook::mInstanceHookMutex.Unlock();
		        return false;
            }
        }

        HMODULE hOGL;
        if ((hOGL = GetModuleHandle(L"opengl32.dll")) == NULL)
        {
            OGLHook::mInstanceHookMutex.Unlock();
            return false;
        }

        isHooked = OGLInit(hOGL);
        if (isHooked)
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_GENERIC, TelemetryRenderer_OPENGL, "Hooked at least once"))

        OGLHook::mInstanceHookMutex.Unlock();
    }
    return isHooked;
}


static DWORD WINAPI IGOOGLHookDestroyThread(LPVOID *lpParam)
{
    OGLHook::Cleanup();
        
    CloseHandle(OGLHook::mIGOOGLHookDestroyThreadHandle);
    OGLHook::mIGOOGLHookDestroyThreadHandle = NULL;
    return 0;
}


void OGLHook::CleanupLater()
{
    if (OGLHook::mIGOOGLHookDestroyThreadHandle==NULL)
        OGLHook::mIGOOGLHookDestroyThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)IGOOGLHookDestroyThread, NULL, 0, NULL);
}

bool OGLHook::IsReadyForRehook()
{
    // limit re-hooking to have a 15 second timeout!
    if (GetTickCount() - mLastHookTime < REHOOKCHECK_DELAY)
        return false;

    return true;
}

bool OGLHook::IsBroken()
{
    bool broken = false;

    if (!isHooked)
        return broken;

    if (!OriginIGO::CheckHook((LPVOID *)&(ChangeDisplaySettingsAHookNext), ChangeDisplaySettingsAHook, true) ||
        !OriginIGO::CheckHook((LPVOID *)&(ChangeDisplaySettingsWHookNext), ChangeDisplaySettingsWHook, true) ||
        !OriginIGO::CheckHook((LPVOID *)&(wglSwapBuffersHookNext), wglSwapBuffersHook, true) ||
        !OriginIGO::CheckHook((LPVOID *)&(wglSwapLayerBuffersHookNext), wglSwapLayerBuffersHook, true) ||
        !OriginIGO::CheckHook((LPVOID *)&(wglMakeCurrentHookNext), wglMakeCurrentHook, true))
    {
        IGOLogWarn("OGLHook broken.");
        CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_THIRD_PARTY, TelemetryRenderer_OPENGL, "OGLHook is broken."));
        broken = true;

        // disable logging, if IGO is broken
        if (OriginIGO::IGOLogger::instance())
        {
            OriginIGO::IGOLogger::instance()->enableLogging(getIGOConfigurationFlags().gEnabledLogging);
        }

    }

    return broken;
}

void OGLHook::Cleanup()
{
    OGLHook::mInstanceHookMutex.Lock();
    if (!IsBroken())
    {
        UNHOOK_SAFE(ChangeDisplaySettingsAHook);
        UNHOOK_SAFE(ChangeDisplaySettingsWHook);
        UNHOOK_SAFE(wglSwapBuffersHook);
        UNHOOK_SAFE(wglSwapLayerBuffersHook);
        UNHOOK_SAFE(wglMakeCurrentHook);
        isHooked = false;

    }
    
    OGLHook::mInstanceHookMutex.Unlock();

    // only kill IGO if it was created by this render
    SAFE_CALL_LOCK_ENTER
    if (IGOApplication::instance()!=NULL && (IGOApplication::instance()->rendererType() == RendererOGL))
    {
        bool safeToCall = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isThreadSafe);
        SAFE_CALL_LOCK_LEAVE

        if (safeToCall)
        {
            DestroyScreenCopyGL();
            IGOApplication::deleteInstance();
        }
    }
    else
    {
        SAFE_CALL_LOCK_LEAVE
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#elif defined(ORIGIN_MAC) // MAC OSX

#include "IGOLogger.h"
#include "OGLDump.h"
#include "OGLShader.h"
#include "MacOGLFunctions.h"
#include "IGOSharedStructs.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Handle saving/restoring of current game OpenGL state + setting rendering shader
OGLHook::DevContext::DevContext()
{
    mStateSaved = false;
    base.shader = NULL;
    base.bgShader = NULL;
    
    pixelToPointScaleFactor = 1.0f;
}

void OGLHook::DevContext::SetRequiresRendering()
{
    if (!mStateSaved)
    {
        SaveRenderState(base.oglProperties);
        
        lastShader = NULL;
        texUnitIdx = mOglState.texUnitIdx;
        
        mStateSaved = true;
    }
}

OGLHook::DevContext::~DevContext()
{
    if (mStateSaved)
        RestoreRenderState(base.oglProperties);
}


void OGLHook::DevContext::SaveRenderState(const OpenGLProperties& oglProperties)
{
    GL(glGetIntegerv(GL_VIEWPORT, mOglState.viewport));
    GL(glGetIntegerv(GL_CURRENT_PROGRAM, &mOglState.programID));

    uint32_t width = IGOApplication::instance()->getScreenWidth();
    uint32_t height = IGOApplication::instance()->getScreenHeight();
    GL(glViewport(0, 0, (uint32_t)(width * pixelToPointScaleFactor), (uint32_t)(height * pixelToPointScaleFactor)));
    
    //
    //
    
    GL(glGetBooleanv(GL_DEPTH_TEST, &mOglState.depthTestOn));
    if (mOglState.depthTestOn)
        GL(glDisable(GL_DEPTH_TEST));
    
    GL(glGetBooleanv(GL_CULL_FACE, &mOglState.cullFaceOn));
    if (mOglState.cullFaceOn)
        GL(glDisable(GL_CULL_FACE));
    
    GL(glGetBooleanv(GL_SCISSOR_TEST, &mOglState.scissorTestOn));
    if (mOglState.scissorTestOn)
        GL(glDisable(GL_SCISSOR_TEST));
    
    if (!oglProperties.useCoreProfile)
    {
        GL(glGetBooleanv(GL_ALPHA_TEST, &mOglState.alphaTestOn));
    }
    
    else
        mOglState.alphaTestOn = false;
    
    if (mOglState.alphaTestOn)
        GL(glDisable(GL_ALPHA_TEST));
    
    GL(glGetBooleanv(GL_BLEND, &mOglState.blendOn));
    if (!mOglState.blendOn)
        GL(glEnable(GL_BLEND));
    
    GL(glGetBooleanv(GL_FRAMEBUFFER_SRGB_EXT, &mOglState.sRGBOn));
    if (mOglState.sRGBOn)
        GL(glDisable(GL_FRAMEBUFFER_SRGB_EXT));
    
    mOglState.blendSrcRgb = GL_ONE;
    mOglState.blendDstRgb = GL_ZERO;
    mOglState.blendSrcAlpha = GL_ONE;
    mOglState.blendDstAlpha = GL_ZERO;
    GL(glGetIntegerv(GL_BLEND_SRC_RGB, &mOglState.blendSrcRgb));
    GL(glGetIntegerv(GL_BLEND_DST_RGB, &mOglState.blendDstRgb));
    GL(glGetIntegerv(GL_BLEND_SRC_ALPHA, &mOglState.blendSrcAlpha));
    GL(glGetIntegerv(GL_BLEND_DST_ALPHA, &mOglState.blendDstAlpha));
    if (mOglState.blendSrcRgb != GL_SRC_ALPHA || mOglState.blendDstRgb != GL_ONE_MINUS_SRC_ALPHA
        || mOglState.blendSrcAlpha != GL_SRC_ALPHA || mOglState.blendDstAlpha != GL_ONE_MINUS_SRC_ALPHA)
        GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    
    mOglState.blendEqRgb = GL_FUNC_ADD;
    mOglState.blendEqAlpha = GL_FUNC_ADD;
    GL(glGetIntegerv(GL_BLEND_EQUATION_RGB, &mOglState.blendEqRgb));
    GL(glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &mOglState.blendEqAlpha));
    if (mOglState.blendEqRgb != GL_FUNC_ADD || mOglState.blendEqAlpha != GL_FUNC_ADD)
        GL(glBlendEquation(GL_FUNC_ADD));
    
    GL(glGetBooleanv(GL_STENCIL_TEST, &mOglState.stencilTestOn));
    if (mOglState.stencilTestOn)
        GL(glDisable(GL_STENCIL_TEST));
    
    GL(glGetBooleanv(GL_COLOR_WRITEMASK, mOglState.colorMask));
    if ((mOglState.colorMask[0] & mOglState.colorMask[1] & mOglState.colorMask[2] & mOglState.colorMask[3]) == GL_FALSE)
        GL(glColorMask(TRUE, TRUE, TRUE, TRUE));
    
    GL(glGetBooleanv(GL_DEPTH_WRITEMASK, &mOglState.depthMaskOn));
    if (mOglState.depthMaskOn)
        GL(glDepthMask(GL_FALSE));
    
    GL(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
    
    mOglState.texUnitIdx = oglProperties.maxSharedTextureUnits - 1;
    if (mOglState.texUnitIdx < 0)
        mOglState.texUnitIdx = 0;
    
    mOglState.currTexUnitID = GL_TEXTURE0;
    GL(glGetIntegerv(GL_ACTIVE_TEXTURE, &mOglState.currTexUnitID));
    GL(glActiveTexture(GL_TEXTURE0 + mOglState.texUnitIdx));
    
    // Will need to complete the state retrieval/restore when we get games that use core profile 3.2
    // DO NOT PUSH CLIENT STATE - KEPT IT BEFORE TO BE SAFE, BUT NOT NECESSARY SINCE WE SAVE WHAT WE OVERRIDE OURSELVES
    // AND THE IMPLEMENTATION BREAKS SOME GAMES - for example LEGO Harry Potter GL(glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS));
    if (!oglProperties.useCoreProfile)
    {
        GL(glPushAttrib(GL_ALL_ATTRIB_BITS));
    }
    
    GLint clipPlaneCnt = 0;
    GL(glGetIntegerv(GL_MAX_CLIP_PLANES, &clipPlaneCnt));
    for (GLint idx = 0; idx < clipPlaneCnt; ++idx)
    {
        GL(glDisable(GL_CLIP_PLANE0 + idx));
    }
    
    //GL(glGetIntegerv(GL_TEXTURE_BINDING_1D, &oglState->texID_1D));
    //if (oglState->texID_1D)
    //    GL(glBindTexture(GL_TEXTURE_BINDING_1D, 0));
    
    //GL(glGetIntegerv(GL_TEXTURE_BINDING_1D_ARRAY_EXT, &oglState->texID_1D_ARRAY));
    //if (oglState->texID_1D_ARRAY)
    //    GL(glBindTexture(GL_TEXTURE_BINDING_1D_ARRAY_EXT, 0));
    
    GL(glGetIntegerv(GL_TEXTURE_BINDING_2D, &mOglState.texID_2D));
    
    //GL(glTexParameteriv(GL_TEXTURE_BINDING_2D, GL_TEXTURE_RANGE_LENGTH_APPLE, &oglState->currTexRangeLength));
    //GL(glGetTexParameterPointervAPPLE(GL_TEXTURE_BINDING_2D, GL_TEXTURE_RANGE_POINTER_APPLE, &oglState->currTexRangePointer));
    //GL(glTextureRangeAPPLE(GL_TEXTURE_BINDING_2D, 0, NULL));
    //if (oglState->texID_2D)
    //    GL(glBindTexture(GL_TEXTURE_BINDING_2D, 0));
    
    // CHECK FOR CORE? - GL(glGetIntegerv(GL_TEXTURE_BINDING_2D_MULTISAMPLE, &oglState->texID_2D_MULTISAMPLE));
    
    //GL(glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY_EXT, &oglState->texID_2D_ARRAY));
    //if (oglState->texID_2D_ARRAY)
    //    GL(glBindTexture(GL_TEXTURE_BINDING_2D_ARRAY_EXT, 0));
    
    //GL(glGetIntegerv(GL_TEXTURE_BINDING_3D, &oglState->texID_3D));
    //if (oglState->texID_3D)
    //    GL(glBindTexture(GL_TEXTURE_BINDING_3D, 0));
    
    //GL(glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &oglState->texID_CUBE_MAP));
    //if (oglState->texID_CUBE_MAP)
    //    GL(glBindTexture(GL_TEXTURE_BINDING_CUBE_MAP, 0));
    
    //GL(glGetIntegerv(GL_TEXTURE_BINDING_RECTANGLE_ARB, &oglState->texID_RECT));
    //if (oglState->texID_RECT)
    //    GL(glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0));
    
    // CHECK FOR CORE? - GL(glGetIntegerv(GL_TEXTURE_BINDING_BUFFER, &oglState->texID_BUFFER));
    
    GL(glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &mOglState.drawFramebuffer));
    GL(glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &mOglState.readFramebuffer));
    if (mOglState.drawFramebuffer || mOglState.readFramebuffer)
        GL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    
    GL(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &mOglState.bufferID_ARRAY_BUFFER));
    GL(glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &mOglState.bufferID_ELEMENT_BUFFER));
    GL(glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &mOglState.vertexAttrib0Enabled));
    GL(glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_SIZE, &mOglState.vertexAttrib0Size));
    GL(glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &mOglState.vertexAttrib0Stride));
    GL(glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_TYPE, &mOglState.vertexAttrib0Type));
    GL(glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &mOglState.vertexAttrib0Normalized));
    if (base.oglExtensions.supportsInstancedArrays && glVertexAttribDivisorARB)
    {
        GL(glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_DIVISOR_ARB, &mOglState.vertexAttrib0Divisor));
        GL(glVertexAttribDivisorARB(0, 0));
    }
    GL(glGetVertexAttribPointerv(0, GL_VERTEX_ATTRIB_ARRAY_POINTER, &mOglState.vertexAttrib0Pointer));
    
    GL(glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &mOglState.bufferID_PIXEL_UNPACK));
    if (mOglState.bufferID_PIXEL_UNPACK)
        GL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
    
    GL(glGetIntegerv(GL_UNPACK_ALIGNMENT, &mOglState.unpackAlignment));
    if (mOglState.unpackAlignment != 4)
        GL(glPixelStorei(GL_UNPACK_ALIGNMENT, 4));
    
    GL(glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &mOglState.unpackImageHeight));
    if (mOglState.unpackImageHeight)
        GL(glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0));
    
    GL(glGetBooleanv(GL_UNPACK_LSB_FIRST, &mOglState.unpackLsbFirst));
    if (mOglState.unpackLsbFirst)
        GL(glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE));
    
    GL(glGetIntegerv(GL_UNPACK_ROW_LENGTH, &mOglState.unpackRowLength));
    if (mOglState.unpackRowLength)
        GL(glPixelStorei(GL_UNPACK_ROW_LENGTH , 0));
    
    GL(glGetIntegerv(GL_UNPACK_SKIP_IMAGES, &mOglState.unpackSkipImages));
    if (mOglState.unpackSkipImages)
        GL(glPixelStorei(GL_UNPACK_SKIP_IMAGES, 0));
    
    GL(glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &mOglState.unpackSkipPixels));
    if (mOglState.unpackSkipPixels)
        GL(glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0));
    
    GL(glGetIntegerv(GL_UNPACK_SKIP_ROWS, &mOglState.unpackSkipRows));
    if (mOglState.unpackSkipRows)
        GL(glPixelStorei(GL_UNPACK_SKIP_ROWS, 0));
    
    GL(glGetBooleanv(GL_UNPACK_SWAP_BYTES, &mOglState.unpackSwapBytes));
    if (mOglState.unpackSwapBytes)
        GL(glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE));
    
    
    GL(glGetBooleanv(GL_UNPACK_CLIENT_STORAGE_APPLE, &mOglState.unpackLocalStorage));
    if (mOglState.unpackLocalStorage)
        GL(glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE));
}

void OGLHook::DevContext::RestoreRenderState(const OpenGLProperties& oglProperties)
{
    // CHECK FOR CORE? MULTIPLE TOO?
//     if (mOglState.drawFramebuffer || mOglState.readFramebuffer)
//     {
//         GL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mOglState.drawFramebuffer));
//         GL(glBindFramebuffer(GL_READ_FRAMEBUFFER, mOglState.readFramebuffer));
//     }
    
    GL(glViewport(mOglState.viewport[0], mOglState.viewport[1], mOglState.viewport[2], mOglState.viewport[3]));
    GL(glUseProgram(mOglState.programID));
    
    GL(glBindBuffer(GL_ARRAY_BUFFER, mOglState.bufferID_ARRAY_BUFFER));
    GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mOglState.bufferID_ELEMENT_BUFFER));
    if (mOglState.vertexAttrib0Enabled == GL_FALSE)
    {
        GL(glDisableVertexAttribArray(0));
    }
    
    else
    {
        // Avoid a GL_INVALID_OPERATION
        if (mOglState.vertexAttrib0Pointer != 0 || mOglState.bufferID_ARRAY_BUFFER != 0)
        {
            GL(glVertexAttribPointer(0, mOglState.vertexAttrib0Size, mOglState.vertexAttrib0Type, mOglState.vertexAttrib0Normalized, mOglState.vertexAttrib0Stride, mOglState.vertexAttrib0Pointer));
            
            if (base.oglExtensions.supportsInstancedArrays && glVertexAttribDivisorARB)
            {
                GL(glVertexAttribDivisorARB(0, mOglState.vertexAttrib0Divisor));
            }
        }
    }
    
    if (mOglState.bufferID_PIXEL_UNPACK)
        GL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mOglState.bufferID_PIXEL_UNPACK));
    
    if (mOglState.unpackAlignment != 4)
        GL(glPixelStorei(GL_UNPACK_ALIGNMENT, mOglState.unpackAlignment));
    
    if (mOglState.unpackImageHeight != 0)
        GL(glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, mOglState.unpackImageHeight));
    
    if (mOglState.unpackLsbFirst)
        GL(glPixelStorei(GL_UNPACK_LSB_FIRST, GL_TRUE));
    
    if (mOglState.unpackRowLength != 0)
        GL(glPixelStorei(GL_UNPACK_ROW_LENGTH , mOglState.unpackRowLength));
    
    if (mOglState.unpackSkipImages != 0)
        GL(glPixelStorei(GL_UNPACK_SKIP_IMAGES, mOglState.unpackSkipImages));
    
    if (mOglState.unpackSkipPixels != 0)
        GL(glPixelStorei(GL_UNPACK_SKIP_PIXELS, mOglState.unpackSkipPixels));
    
    if (mOglState.unpackSkipRows != 0)
        GL(glPixelStorei(GL_UNPACK_SKIP_ROWS, mOglState.unpackSkipRows));
    
    if (mOglState.unpackSwapBytes)
        GL(glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE));
    
    if (mOglState.unpackLocalStorage)
        GL(glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE));
    
    
    //if (oglState.texID_1D > 0)
    //    GL(glBindTexture(GL_TEXTURE_1D, oglState.texID_1D));
    
    //if (oglState.texID_1D_ARRAY > 0)
    //    GL(glBindTexture(GL_TEXTURE_1D_ARRAY_EXT, oglState.texID_1D_ARRAY));
    
    GL(glBindTexture(GL_TEXTURE_2D, mOglState.texID_2D));
    
    //if (oglState.texID_2D_MULTISAMPLE > 0)
    //    GL(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, oglState.texID_2D_MULTISAMPLE));
    
    //if (oglState.texID_2D_ARRAY > 0)
    //    GL(glBindTexture(GL_TEXTURE_2D_ARRAY_EXT, oglState.texID_2D_ARRAY));
    
    //if (oglState.texID_3D > 0)
    //    GL(glBindTexture(GL_TEXTURE_3D, oglState.texID_3D));
    
    //if (oglState.texID_CUBE_MAP > 0)
    //    GL(glBindTexture(GL_TEXTURE_CUBE_MAP, oglState.texID_CUBE_MAP));
    
    //if (oglState.texID_RECT)
    //    GL(glBindTexture(GL_TEXTURE_RECTANGLE_ARB, oglState.texID_RECT));
    
    //if (oglState.texID_BUFFER > 0)
    //    GL(glBindTexture(GL_TEXTURE_BUFFER, oglState.texID_BUFFER));
    
    if (!oglProperties.useCoreProfile)
    {
        GL(glPopAttrib());
    }
    
    //
    //
    
    GL(glActiveTexture(mOglState.currTexUnitID));
    
    if (mOglState.depthMaskOn)
        GL(glDepthMask(GL_TRUE));
    
    if ((mOglState.colorMask[0] & mOglState.colorMask[1] & mOglState.colorMask[2] & mOglState.colorMask[3]) == GL_FALSE)
        GL(glColorMask(mOglState.colorMask[0], mOglState.colorMask[1], mOglState.colorMask[2], mOglState.colorMask[3]));
    
    if (mOglState.stencilTestOn)
        GL(glEnable(GL_STENCIL_TEST));
    
    if (mOglState.blendSrcRgb != GL_SRC_ALPHA || mOglState.blendDstRgb != GL_ONE_MINUS_SRC_ALPHA
        || mOglState.blendSrcAlpha != GL_SRC_ALPHA || mOglState.blendDstAlpha != GL_ONE_MINUS_SRC_ALPHA)
        GL(glBlendFuncSeparate(mOglState.blendSrcRgb, mOglState.blendDstRgb, mOglState.blendSrcAlpha, mOglState.blendDstAlpha));
    
    if (mOglState.blendEqRgb != GL_FUNC_ADD || mOglState.blendEqAlpha != GL_FUNC_ADD)
        GL(glBlendEquationSeparate(mOglState.blendEqRgb, mOglState.blendEqAlpha));
    
    if (mOglState.sRGBOn)
        GL(glEnable(GL_FRAMEBUFFER_SRGB_EXT));
    
    if (!mOglState.blendOn)
        GL(glDisable(GL_BLEND));
    
    if (mOglState.alphaTestOn)
        GL(glEnable(GL_ALPHA_TEST));
    
    if (mOglState.scissorTestOn)
        GL(glEnable(GL_SCISSOR_TEST));
    
    if (mOglState.cullFaceOn)
        GL(glEnable(GL_CULL_FACE));
    
    if (mOglState.depthTestOn)
        GL(glEnable(GL_DEPTH_TEST));
}

OGLHook::OGLHook()
{
}


OGLHook::~OGLHook()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class IGOFrameContext
{
    static const size_t MAX_EVENT_PER_FRAME = 64;
    
public:
    IGOFrameContext()
    : _eventsLock(OS_SPINLOCK_INIT), _lastMoveLocation(INVALID_MOVE_LOCATION), _eventBufferIdx(0)
    , _viewPosX(0), _viewPosY(0), _viewWidth(0), _viewHeight(0), _viewOffsetX(0), _viewOffsetY(0)
    , _frameIdx(0), _bsWidth(0), _bsHeight(0), _bsCtxtWidth(0), _bsCtxtHeight(0)
    , _prevPosX(0), _prevPosY(0), _prevWidth(0), _prevHeight(0), _prevOffsetX(0), _prevOffsetY(0), _pixelToPointScaleFactor(1.0f)
    , _viewPropertiesSource(ViewPropertiesSource_UNDEFINED), _prevViewPropertiesSource(ViewPropertiesSource_UNDEFINED)
    , _hasFocus(false), _isWindowed(false), _isKeyWindow(false), _isFullscreen(false), _receivedMoveEvent(false), _viewPropertiesSet(false), _viewPropertiesSetThisFrame(false)
    {
        _cgMainDisplayID_APIAvail = dlsym(RTLD_DEFAULT, "CGMainDisplayID");
        
        _events[0].reserve(MAX_EVENT_PER_FRAME);
        _events[1].reserve(MAX_EVENT_PER_FRAME);
        
        _keyboardData[0].reserve(4096);
        _keyboardData[1].reserve(4096);
    }
    
    ~IGOFrameContext()
    {
    }
    
    int GetFrameIdx() const { return _frameIdx; }
    
    int Width() const { return _bsCtxtWidth > 0 ? _bsCtxtWidth : _viewWidth; }
    int Height() const { return _bsCtxtHeight > 0 ? _bsCtxtHeight : _viewHeight; }
    bool IsWindowed() const { return _isWindowed; }
    int BackingStoreWidth() const { return _bsWidth; }
    int BackingStoreHeight() const { return _bsHeight; }
    float PixelToPointScaleFactor() const { return _pixelToPointScaleFactor; }
    void ResetExtractedViewProperties() { _viewPropertiesSetThisFrame = false; }
    bool SuccessfullyExtractedViewProperties() const { return _viewPropertiesSetThisFrame; }
    
    void SetFocus(bool focus)
    {
        _hasFocus = focus;
    }
    
    void SetImmediateViewProperties(int posX, int posY, int width, int height, int offsetX, int offsetY, bool hasFocus, bool isKeyWindow, bool isFullscreen, float pixelToPointScaleFactor)
    {
        _viewPosX = posX;
        _viewPosY = posY;
        _viewWidth = width;
        _viewHeight = height;
        _viewOffsetX = offsetX;
        _viewOffsetY = offsetY;

        _hasFocus = hasFocus;
        _isKeyWindow = isKeyWindow;
        _isFullscreen = isFullscreen; // this is not a "complete" state as we may think this is not a fullscreen window, yet the game captured the display!
        _pixelToPointScaleFactor = pixelToPointScaleFactor;
        
        _viewPropertiesSet = true;
        _viewPropertiesSetThisFrame = true;
        _viewPropertiesSource = ViewPropertiesSource_SWIZZLE_CALL;
    }
    
    void Reset(CGLContextObj ctxt)
    {
        _bsCtxtWidth = 0;
        _bsCtxtHeight = 0;
        
        // Check the backing size - gives us the correct render size to use when available (for example for
        // Batman Arkham Asylum, which renders to an off-screen buffer before scaling it up to the display size.
        GLint dims[2];
        CGLError error = GL(CGLGetParameter(ctxt, kCGLCPSurfaceBackingSize, dims));
        if (error == kCGLNoError)
        {
            if (dims[0] > 0 && dims[1] > 0)
            {
                _bsCtxtWidth = dims[0];
                _bsCtxtHeight = dims[1];
                
                OriginIGO::IGOLogInfo("Backing store dims (%d/%d)", dims[0], dims[1]);
            }
            
            else
            {
                OriginIGO::IGOLogInfo("Invalid backing store dims");
            }
        }
        
        else
        {
            OriginIGO::IGOLogInfo("No backing size available (%d/%s)", error, CGLErrorString(error));
        }
    }
    
    void AddEvent(const IGOIPC::InputEvent& event, const unsigned char* data)
    {
        OSSpinLockLock(&_eventsLock);
        
        if (_events[_eventBufferIdx].size() < MAX_EVENT_PER_FRAME)
        {
#ifdef FILTER_MOUSE_MOVE_EVENTS
            // Keep track to check if we recieved a mouve event this frame
            if (event.type == IGOIPC::EVENT_MOUSE_MOVE)
                _receivedMoveEvent = true;
            
            else
            if (!_receivedMoveEvent)
            {
                // If we didn't get the last mouse event, check what the tap got last/if we need to push that
                // down the pipe
                _receivedMoveEvent = true;
                uint64_t moveLocation = GetLastMoveLocation();
                if (moveLocation != INVALID_MOVE_LOCATION)
                {
                    _lastMoveLocation = moveLocation;
                        
                    int32_t x = EXTRACT_MOVE_LOCATION_X(_lastMoveLocation);
                    int32_t y = EXTRACT_MOVE_LOCATION_Y(_lastMoveLocation);
                        
                    // NOTE: use 0 for timestamp to force acknowledgment of the event before anything else
                    _events[_eventBufferIdx].push_back(IGOIPC::InputEvent(IGOIPC::EVENT_MOUSE_MOVE, x, y, 0, 0));
                }
            }
#endif
            
            _events[_eventBufferIdx].push_back(event);
            if (data)
                _keyboardData[_eventBufferIdx].insert(_keyboardData[_eventBufferIdx].end(), data, data + event.param1);
        }
        
        OSSpinLockUnlock(&_eventsLock);
    }
    
    void Flush(CGLContextObj ctxt, bool enableFakeMoveEvents)
    {
        // Compute final view properties!
        bool done = false;
        _isWindowed = !_isFullscreen;
        if (_cgMainDisplayID_APIAvail)
        {
            CGDirectDisplayID id = CGMainDisplayID();
            if (CGDisplayIsCaptured(id))
            {
                CGRect bounds = CGDisplayBounds(id);
                
                // This is the best way I found to handle the potential scaling of the window for now!
                if (_viewPropertiesSet)
                {
                    _bsWidth = _viewWidth;
                    _bsHeight = _viewHeight;
                }
                
                _viewPosX = bounds.origin.x;
                _viewPosY = bounds.origin.y;
                _viewWidth = CGDisplayPixelsWide(id);
                _viewHeight = CGDisplayPixelsHigh(id);
                _viewOffsetX = 0;
                _viewOffsetY = 0;
                
                done = true;
                _isWindowed = false;
                _isKeyWindow = true;
                _viewPropertiesSource = ViewPropertiesSource_CAPTURED_FULLSCREEN;
            }
        }
        
        if (!done)
        {
            if (!_viewPropertiesSet)
            {
                GLint viewport[4];
                GL(glGetIntegerv(GL_VIEWPORT, viewport));
                
                _viewPosX = 0;
                _viewPosY = 0;
                if (viewport[2] != 0 && viewport[3] != 0)
                {
                    _viewWidth = viewport[2];
                    _viewHeight = viewport[3];
                    _viewPropertiesSource = ViewPropertiesSource_VIEWPORT;
                }
                
                else
                {
                    _viewWidth = 1024; // Well, at that point, we're in deep ...
                    _viewHeight = 768;
                    _viewPropertiesSource = ViewPropertiesSource_DEFAULT;
                }
                
                _viewOffsetX = 0;
                _viewOffsetY = 0;
            }
            
            _bsWidth = _viewWidth;
            _bsHeight = _viewHeight;
        }
        
        if (_viewPropertiesSource != _prevViewPropertiesSource)
        {
            _prevPosX = _viewPosX;
            _prevPosY = _viewPosY;
            _prevWidth = _viewWidth;
            _prevHeight = _viewHeight;
            _prevOffsetX = _viewOffsetX;
            _prevOffsetY = _viewOffsetY;
            
            if (_viewPropertiesSource == ViewPropertiesSource_CAPTURED_FULLSCREEN)
            {
                OriginIGO::IGOLogInfo("Using captured fullscreen for view properties (%d, %d, %d, %d, %d, %d)", _viewPosX, _viewPosY, _viewWidth, _viewHeight, _viewOffsetX, _viewOffsetY);
            }
            
            else
            if (_viewPropertiesSource == ViewPropertiesSource_SWIZZLE_CALL)
            {
                OriginIGO::IGOLogInfo("Using swizzle callback for view properties (%d, %d, %d, %d, %d, %d)", _viewPosX, _viewPosY, _viewWidth, _viewHeight, _viewOffsetX, _viewOffsetY);
            }
            
            else
            if (_viewPropertiesSource == ViewPropertiesSource_VIEWPORT)
            {
                OriginIGO::IGOLogInfo("Using viewport parameters for view properties - Not good (%d, %d, %d, %d, %d, %d)", _viewPosX, _viewPosY, _viewWidth, _viewHeight, _viewOffsetX, _viewOffsetY);
            }
            
            else
            {
                OriginIGO::IGOLogInfo("Using defaults for view properties - Not good! (%d, %d, %d, %d, %d, %d)", _viewPosX, _viewPosY, _viewWidth, _viewHeight, _viewOffsetX, _viewOffsetY);
            }
        }
        
        else
        {
            if (_prevPosX != _viewPosX || _prevPosY != _viewPosY || _prevWidth != _viewWidth || _prevHeight != _viewHeight || _prevOffsetX != _viewOffsetX || _prevOffsetY != _viewOffsetY)
            {
                _prevPosX = _viewPosX;
                _prevPosY = _viewPosY;
                _prevWidth = _viewWidth;
                _prevHeight = _viewHeight;
                _prevOffsetX = _viewOffsetX;
                _prevOffsetY = _viewOffsetY;
                
                OriginIGO::IGOLogInfo("New view properties: (%d, %d, %d, %d, %d, %d)", _viewPosX, _viewPosY, _viewWidth, _viewHeight, _viewOffsetX, _viewOffsetY);
            }
        }
        
#if 0 // KEEPING THIS AROUND FOR REFERENCE -
        // Try accessing the backbuffer to take into account any type of scaling
        done = false;
        int backBufferWidth = 0;
        int backBufferHeight = 0;
        
        GLint enabled;
        CGLError error = GL(CGLIsEnabled(ctxt, kCGLCESwapRectangle, &enabled));
        if (error == kCGLNoError && enabled)
        {
            GLint swapRect[4];
            error = GL(CGLGetParameter(ctxt, kCGLCPSwapRectangle, swapRect));
            if (error == kCGLNoError)
            {
                backBufferWidth = swapRect[2];
                backBufferHeight = swapRect[3];
                done = true;
            }
        }
        
        else
        if (error != kCGLNoError)
        {
            OriginIGO::IGOLogWarn("Unable to check if context swap rectable is actif (%d - %s)", error, GL(CGLErrorString(error)));
        }
        
        if (!done)
        {
            error = GL(CGLIsEnabled(ctxt, kCGLCESurfaceBackingSize, &enabled));
            if (error == kCGLNoError && enabled)
            {
                GLint dims[2];
                error = GL(CGLGetParameter(ctxt, kCGLCPSurfaceBackingSize, dims));
                if (error == kCGLNoError)
                {
                    backBufferWidth = dims[0];
                    backBufferHeight = dims[1];
                    done = true;
                }
                
                else
                {
                    OriginIGO::IGOLogWarn("Unable to retrieve back buffer extents (%d - %s)", error, CGLErrorString(error));
                }
            }
        }
        
        if (done)
        {
            if (backBufferWidth != _prevBackBufferWidth || backBufferHeight != _prevBackBufferHeight)
            {
                _prevBackBufferWidth = backBufferWidth;
                _prevBackBufferHeight = backBufferHeight;
                
                OriginIGO::IGOLogInfo("New backbuffer extents - width=%d, height=%d", backBufferWidth, backBufferHeight);
            }
        }
        
        else
        {
            OriginIGO::IGOLogWarn("Unable to access backbuffer extents");
        }
#endif // TRYING BACKING STORE INFO
        
        if (!_isWindowed)
            _hasFocus = true;   
        
        SetCursorFocusState(_hasFocus);
        SetInputFocusState(_hasFocus);
        
        SetInputFullscreenState(!_isWindowed);
        
        FlushEvents(enableFakeMoveEvents);
        
        ++_frameIdx;
        _isKeyWindow = false;
        // Keep previous set in case we had to force it first - _viewPropertiesSet = false;
        _viewPropertiesSetThisFrame = false;
        _prevViewPropertiesSource = _viewPropertiesSource;
    }
    
    // Dump the content of last back buffer
    void dumpBuffer(CGLContextObj ctxt)
    {
        char homeDir[MAX_PATH];
        if (Origin::Mac::GetHomeDirectory(homeDir, sizeof(homeDir)) == 0)
        {
            char fileName[MAX_PATH];
            snprintf(fileName, sizeof(fileName), "%s/Library/Application Support/Origin/Logs/Framebuffer.tga", homeDir);
            
            CGLPixelFormatObj pixelFormat = CGLGetPixelFormat(ctxt);
            if (pixelFormat)
            {
                GLint bytesPerPixel = 0;
                
                if (CGLDescribePixelFormat(pixelFormat, 0, kCGLPFAColorSize, &bytesPerPixel) == kCGLNoError)
                {
                    OriginIGO::IGOLogInfo("Draw buffer uses %d bytes per pixel", bytesPerPixel);
                    
                    if (bytesPerPixel == 32)
                    {
                        int width = Width();
                        int height = Height();
                        unsigned char* data = new unsigned char[width * height * bytesPerPixel];
                        
                        glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, data);
                        Origin::Mac::SaveRawRGBAToTGA(fileName, width, height, data, true);
                        
                        delete[] data;
                    }
                    
                    else
                        OriginIGO::IGOLogWarn("Unsupported pixel format for ctxt=%p", ctxt);
                }
                
                else
                {
                    OriginIGO::IGOLogWarn("Unable to get bytes per pixel for ctxt=%p", ctxt);
                }
            }
            
            else
            {
                OriginIGO::IGOLogWarn("Unable to access pixel format for ctxt=%p", ctxt);
            }
        }
        
        else
        {
            OriginIGO::IGOLogWarn("Unable to access home directory");
        }
    }
    
private:
    void FlushEvents(bool enableFakeMoveEvents)
    {
        int posX = _viewPosX;
        int posY = _viewPosY;
        int width = _viewWidth;
        int height = _viewHeight;
        int offsetX = _viewOffsetX;
        int offsetY = _viewOffsetY;
        
        
        SetInputWindowInfo(posX, posY, width, height, _isKeyWindow);
        SetCursorWindowInfo(posX, posY, width, height);
        
        GetKeyboardState(gIGOKeyboardData);
        
        OSSpinLockLock(&_eventsLock);
        
#ifdef FILTER_MOUSE_MOVE_EVENTS
        // If we didn't get the last mouse event, check what the tap got last/if we need to push that
        // down the pipe
        if (enableFakeMoveEvents && _hasFocus && !_receivedMoveEvent)
        {
            uint64_t moveLocation = GetLastMoveLocation();
#ifdef USE_CLIENT_SIDE_FILTERING
            if (moveLocation != INVALID_MOVE_LOCATION)
#else
            if (moveLocation != INVALID_MODE_LOCATION && moveLocation != _lastMoveLocation)
#endif
            {
                _lastMoveLocation = moveLocation;
                    
                int32_t x = EXTRACT_MOVE_LOCATION_X(moveLocation);
                int32_t y = EXTRACT_MOVE_LOCATION_Y(moveLocation);
                    
#ifdef USE_CLIENT_SIDE_FILTERING
                uint64_t timestamp = UnsignedWideToUInt64(AbsoluteToNanoseconds(UpTime()));
                uint32_t lowTS = (uint32_t)timestamp;
                uint32_t highTS = (uint32_t)(timestamp >> 32);
#else
                uint32_t lowTS = 0;
                uint32_t highTS = 0;
#endif
                    
                _events[_eventBufferIdx].push_back(IGOIPC::InputEvent(IGOIPC::EVENT_MOUSE_MOVE, x, y, lowTS, highTS));
            }
        }
#endif
        
        int bufferIdx = _eventBufferIdx;
        _eventBufferIdx = 1 - _eventBufferIdx;
        _events[_eventBufferIdx].clear();
        _keyboardData[_eventBufferIdx].clear();
        _receivedMoveEvent = false;
        
        OSSpinLockUnlock(&_eventsLock);
        
        IGOApplication* app = IGOApplication::instance();
        uint32_t kDataIndex = 0;
        
        if (app && width > 0 && height > 0)
        {
            for (eastl::vector<IGOIPC::InputEvent>::const_iterator iter = _events[bufferIdx].begin(); iter != _events[bufferIdx].end(); ++iter)
            {
                if (iter->type == IGOIPC::EVENT_KEYBOARD)
                {
                    eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgKeyboardEvent((char*)&(_keyboardData[bufferIdx][kDataIndex]), iter->param1));
                    kDataIndex += iter->param1;
                    app->send(msg);
                }
                
                else
                {
                    int32_t localX = iter->param1 - posX;
                    int32_t localY = height - (iter->param2 - posY);
                    
                    float sX= app->getScalingX();
                    float sY= app->getScalingY();
                    
                    int32_t correctedX = app->getOffsetX() + (int32_t)(((float)localX)*sX);
                    int32_t correctedY = app->getOffsetY() + (int32_t)(((float)localY)*sY);
                    
                    if (iter->type == IGOIPC::EVENT_MOUSE_MOVE)
                    {
#ifdef USE_CLIENT_SIDE_FILTERING
                        uint64_t timestamp = ((uint64_t)((uint32_t)iter->param4)) << 32;
                        timestamp |= (uint64_t)((uint32_t)iter->param3);
#else
                        uint64_t timestamp = 0;
#endif
                        
                        eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgInputEvent(iter->type, correctedX, correctedY, offsetX, offsetY, sX, sY, timestamp));
                        app->send(msg);
                    }
                    
                    else
                    {
                        eastl::shared_ptr<IGOIPCMessage> msg (IGOIPC::instance()->createMsgInputEvent(iter->type, correctedX, correctedY, iter->param3, iter->param4));
                        app->send(msg);
                    }
                }
            }
        }
    }
    
    
    OSSpinLock _eventsLock;
    eastl::vector<IGOIPC::InputEvent> _events[2];
    eastl::vector<unsigned char> _keyboardData[2];

    uint64_t _lastMoveLocation;
    
    int _eventBufferIdx;
    
    int _viewPosX;
    int _viewPosY;
    int _viewWidth;
    int _viewHeight;
    int _viewOffsetX;
    int _viewOffsetY;
    int _frameIdx;
    
    int _bsWidth;
    int _bsHeight;
    int _bsCtxtWidth;
    int _bsCtxtHeight;
    
    int _prevPosX;
    int _prevPosY;
    int _prevWidth;
    int _prevHeight;
    int _prevOffsetX;
    int _prevOffsetY;
    float _pixelToPointScaleFactor;

    enum ViewPropertiesSource
    {
        ViewPropertiesSource_UNDEFINED,
        ViewPropertiesSource_CAPTURED_FULLSCREEN,
        ViewPropertiesSource_SWIZZLE_CALL,
        ViewPropertiesSource_VIEWPORT,
        ViewPropertiesSource_DEFAULT
    };
    
    enum ViewPropertiesSource _viewPropertiesSource;
    enum ViewPropertiesSource _prevViewPropertiesSource;
    
    bool _hasFocus;
    bool _isWindowed;
    bool _isKeyWindow;
    bool _isFullscreen;
    bool _receivedMoveEvent;
    bool _viewPropertiesSet;
    bool _viewPropertiesSetThisFrame;
    bool _cgMainDisplayID_APIAvail;
};

static IGOFrameContext* _igoFrameContext = NULL;

void AddInputEventCallback(const IGOIPC::InputEvent& event, const unsigned char* data)
{
    if (_igoFrameContext)
        _igoFrameContext->AddEvent(event, data);
}

void SetRenderViewPropertiesCallback(int posX, int posY, int width, int height, int offsetX, int offsetY, bool hasFocus, bool isKeyWindow, bool isFullscreen, float pixelToPointScaleFactor)
{
    if (_igoFrameContext)
        _igoFrameContext->SetImmediateViewProperties(posX, posY, width, height, offsetX, offsetY, hasFocus, isKeyWindow, isFullscreen, pixelToPointScaleFactor);
}

void MainMenuActivationCallback(bool isActive)
{
    SetInputEnableFiltering(!isActive);
}

void FocusChanged(bool hasFocus)
{
    if (_igoFrameContext)
    {
        _igoFrameContext->SetFocus(hasFocus);
        SetCursorFocusState(hasFocus);
        SetInputFocusState(hasFocus);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool OGLLoadFunctions(void* module)
{
#undef OGLFN
#define OGLFN(retval, name, params) \
_ ## name = (name ## Fn)dlsym(module, #name); \
if (! _ ## name) \
{ \
    OriginIGO::IGOLogWarn("Missing OpenGL function: " #name); \
    return false; \
}

#undef OGLFN_OPTIONAL
#define OGLFN_OPTIONAL(retval, name, params) \
_ ## name = (name ## Fn)dlsym(module, #name); \

#include "MacOGLFunctions.h"
    
#undef OGLFN
    
    return true;
}

static void Init(CGLContextObj ctxt, int width, int height)
{
    if (!gInputHooked && !gQuitting)
    {
        gInputHooked = SetupInputTap(AddInputEventCallback);
        
        if (gInputHooked)
        {
            // Check the initial focus state to capture keybaord input/can't rely on the activate/deactivate events for that!
            if (IsFrontProcess())
            {
                FocusChanged(true);
            }
        }
    }
    
    if (!IGOApplication::instance())
        IGOApplication::createInstance(RendererOGL, ctxt);
    
    int newWidth = width;
    int newHeight = height;
    
    IGOApplication* instance = IGOApplication::instance();
    if (instance)
        instance->setScreenSize(newWidth, newHeight);
    
    // Time to turn off logging if the user hasn't explicitely enabled it from the client preferences
    OriginIGO::IGOLogger::instance()->enableLogging(::IsIGOLogEnabled());
}

DEFINE_HOOK(CGLError, CGLLockContextHook, (CGLContextObj ctx))
{
    //OriginIGO::IGOLogInfo("Calling CGLLockContext (%p) on thread 0x%08x...", ctx, GetCurrentThreadId());
    return CGLLockContextHookNext(ctx);
}

DEFINE_HOOK(CGLError, CGLUnlockContextHook, (CGLContextObj ctx))
{
    //OriginIGO::IGOLogInfo("Calling CGLUnlockContext (%p) on thread 0x%08x...", ctx, GetCurrentThreadId());
    return CGLUnlockContextHookNext(ctx);
}

static EA::Thread::Futex ContextFutex;

static CGLContextObj ContextInUse = NULL;
static CGLContextObj TextureAllocationContext = NULL;
static OriginIGO::OGLDump* OGLDumpInstance = NULL;
static bool WasIGOOnWhenChangingContext = false;
static volatile bool SetupCompleted = false;

static eastl::vector<OGLHook::AvailableContext> AvailableContexts;
typedef eastl::vector<OGLHook::AvailableContext>::iterator AvailableContextIterator;

static eastl::vector<void*> PendingResets;
typedef eastl::vector<void*>::iterator PendingResetsIterator;

// Clear up our working context if it gets destroyed/momentarily clear up IGO
// We assume this is called from rendering thread!
void ResetIGO(void* ctx)
{
    bool setupCompleted = SetupCompleted;
    if (!setupCompleted)
        return;
    
    EA::Thread::AutoFutex m(ContextFutex);
    {
        OriginIGO::IGOLogWarn("Resetting for %p", ctx);
        
        // Clean up shader for this context
        int idx = 0;
        bool foundCtxt = false;

        AvailableContextIterator iter = AvailableContexts.begin();
        for (; iter != AvailableContexts.end(); ++iter, ++idx)
        {
            if (iter->ctxt == ctx)
            {
                foundCtxt = true;
                OriginIGO::IGOLogInfo("Found context %p to reset", ctx);

                CGLContextObj ctxt = (CGLContextObj)ctx;
                CGLContextObj currentCtxt = CGLGetCurrentContext();

                if (ctxt != currentCtxt)
                {
                    CGLLockContextHookNext(ctxt);
                    CGLSetCurrentContext(ctxt);
                }

                // Only cleanup the windows if they were allocated in this context
                if (ctx == TextureAllocationContext && IGOApplication::instance())
                {
                    OriginIGO::IGOLogInfo("Resetting textures since on correct context %p (current=%p)", ctx, currentCtxt);

                    // Clean up our windows immediately
                    IGOApplication::instance()->clearWindows(NULL);
                    TextureAllocationContext = NULL;
                }
                
                delete iter->shader;
                delete iter->bgShader;
                AvailableContexts.erase(iter);

                if (ctxt != currentCtxt)
                {
                    CGLSetCurrentContext(currentCtxt);
                    CGLUnlockContextHookNext(ctxt);
                }

                break;
            }
        }
        
        if (!foundCtxt)
        {
            OriginIGO::IGOLogInfo("Unable to find context %p in our available list", ctx);
        }
        
        // Resetting IGO cursor during invalid context, but only if we are coming from a valid context
        // (ie we could have multiple resets at once: if IGO was on, first loop -> WasIGOOnWhenChangingContext=true, but
        // second loop WasIGOOnWhenChangingContext=false -> we would never restore IGO properly!)
        if (ContextInUse)
        {
            WasIGOOnWhenChangingContext = IsIGOEnabled();
            if (WasIGOOnWhenChangingContext)
                SetCursorIGOState(false, true, false);
        
            // Resetting current context
            ContextInUse = NULL;
        }
        
        // Don't rely on previous window settings
        if (_igoFrameContext)
            _igoFrameContext->ResetExtractedViewProperties();
    }
}

// Add a request to reset a context (to make sure we use the same thread we used to manage the shared/textures
void RequestResetIGO(void* ctx)
{
    EA::Thread::AutoFutex m(ContextFutex);
    
    PendingResetsIterator iter = PendingResets.begin();
    for (; iter != PendingResets.end(); ++iter)
    {
        if (*iter == ctx)
            return; // we already know about this guy
    }
    
    PendingResets.push_back(ctx);
}

// Process pending resets
void ProcessResetIGORequests()
{
    EA::Thread::AutoFutex m(ContextFutex);
    
    if (PendingResets.size() > 0)
    {
        PendingResetsIterator iter = PendingResets.begin();
        for (; iter != PendingResets.end(); ++iter)
            ResetIGO(*iter);
        
        PendingResets.clear();
    }
}

bool OGLHook::LockGLContext()
{
    bool retVal = false;

    EA::Thread::AutoFutex m(ContextFutex);
    if (ContextInUse)
    {
        CGLError error = CGLLockContextHookNext(ContextInUse);
        if (error == kCGLNoError)
            retVal = true;
        
        else
        {
            OriginIGO::IGOLogWarn("Error when trying to lock context (%d)", error);
        }
    }
    
    return retVal;
}

void OGLHook::UnlockGLContext()
{
    EA::Thread::AutoFutex m(ContextFutex);
    if (ContextInUse)
    {
        CGLError error = CGLUnlockContextHookNext(ContextInUse);
        if (error != kCGLNoError)
        {
            OriginIGO::IGOLogWarn("Error when trying to unlock context (%d)", error);
        }
    }
    
    else
    {
        OriginIGO::IGOLogWarn("TRYING TO UNLOCK AN INVALID CONTEXT!!!!");
    }
}

DEFINE_HOOK(CGLError, CGLSetParameterHook, (CGLContextObj ctx, CGLContextParameter pname, const GLint* params))
{
    // We could be changing the backing buffer, or other parameters that would change our windows computation
    OriginIGO::IGOLogInfo("Calling CGLSetParameter (%p, %d) on thread 0x%08x...", ctx, pname, GetCurrentThreadId());
    
    bool setupCompleted = SetupCompleted;
    if (setupCompleted)
        RequestResetIGO(ctx);   // NOTE: you definitely want to delay the reset since this method could be call from a context creation method like aglCreateContext
                                // -> you want to want until the context object is fully initialized before doing anything with it
    
    return CGLSetParameterHookNext(ctx, pname, params);
}

DEFINE_HOOK(CGError, CGLReleaseContextHook, (CGLContextObj ctx))
{
    OriginIGO::IGOLogInfo("Calling CGLReleaseContext (%p) on thread 0x%08x...", ctx, GetCurrentThreadId());
    GLuint retainCnt = CGLGetContextRetainCount(ctx);
    bool setupCompleted = SetupCompleted;
    
    if (setupCompleted && retainCnt == 1)
        ResetIGO(ctx);
    
    return CGLReleaseContextHookNext(ctx);
}

DEFINE_HOOK(CGError, CGLDestroyContextHook, (CGLContextObj ctx))
{
    OriginIGO::IGOLogInfo("Calling CGLDestroyContext (%p) on thread 0x%08x...", ctx, GetCurrentThreadId());
    //No need to reset - This guy looks like it's calling CGLReleaseContext
    //bool setupCompleted = SetupCompleted;
    //if (setupCompleted)
    //    ResetIGO(ctx);
    
    return CGLDestroyContextHookNext(ctx);
}

DEFINE_HOOK(GLboolean, aglDestroyContextHook, (AGLContext ctx))
{
    OriginIGO::IGOLogInfo("Calling aglDestroyContext (%p/%p)...", ctx, CGLGetCurrentContext());
    
    bool setupCompleted = SetupCompleted;
    if (setupCompleted)
        ResetIGO(ctx);
    
    return aglDestroyContextHookNext(ctx);
}

DEFINE_HOOK(CGError, CGDisplayCaptureHook, (CGDirectDisplayID display))
{
#ifdef DEBUG
    CGLContextObj ctx = CGLGetCurrentContext();
    OriginIGO::IGOLogInfo("Calling CGDisplayCapture (%p)...", ctx);
#endif
    //ResetIGO(ctx);
    
#ifdef SKIP_DISPLAY_CAPTURING
    return kCGErrorSuccess;
#else
    return CGDisplayCaptureHookNext(display);
#endif
}

DEFINE_HOOK(CGError, CGDisplayReleaseHook, (CGDirectDisplayID display))
{
#ifdef DEBUG
    CGLContextObj ctx = CGLGetCurrentContext();
    OriginIGO::IGOLogInfo("Calling CGDisplayRelease (%p)...", ctx);
#endif
    //ResetIGO(ctx);
    
#ifdef SKIP_DISPLAY_CAPTURING
    return kCGErrorSuccess;
#else
    return CGDisplayReleaseHookNext(display);
#endif
}

DEFINE_HOOK(CGError, CGReleaseAllDisplaysHook, ())
{
#ifdef DEBUG
    CGLContextObj ctx = CGLGetCurrentContext();
    OriginIGO::IGOLogInfo("Calling CGReleaseAllDisplays (%p)...", ctx);
#endif
    //ResetIGO(ctx);
    
#ifdef SKIP_DISPLAY_CAPTURING
    return kCGErrorSuccess;
#else
    return CGReleaseAllDisplaysHookNext();
#endif
}

DEFINE_HOOK(CGError, CGDisplayCaptureWithOptionsHook, (CGDirectDisplayID display, CGCaptureOptions options))
{
#ifdef DEBUG
    CGLContextObj ctx = CGLGetCurrentContext();
    OriginIGO::IGOLogInfo("Calling CGDisplayCaptureWithOptions (%p)...", ctx);
#endif
    //ResetIGO(ctx);
    
#ifdef SKIP_DISPLAY_CAPTURING
    return kCGErrorSuccess;
#else
    return CGDisplayCaptureWithOptionsHookNext(display, options);
#endif
}

DEFINE_HOOK(CGError, CGCaptureAllDisplaysHook, ())
{
#ifdef DEBUG
    CGLContextObj ctx = CGLGetCurrentContext();
    OriginIGO::IGOLogInfo("Calling CGCaptureAllDisplays (%p)...", ctx);
#endif
    //ResetIGO(ctx);
    
#ifdef SKIP_DISPLAY_CAPTURING
    return kCGErrorSuccess;
#else
    return CGCaptureAllDisplaysHookNext();
#endif
}

DEFINE_HOOK(CGError, CGCaptureAllDisplaysWithOptionsHook, (CGCaptureOptions options))
{
#ifdef DEBUG
    CGLContextObj ctx = CGLGetCurrentContext();
    OriginIGO::IGOLogInfo("Calling CGCaptureAllDisplaysWithOptions (%p)...", ctx);
#endif
    //ResetIGO(ctx);
    
#ifdef SKIP_DISPLAY_CAPTURING
    return kCGErrorSuccess;
#else
    return CGCaptureAllDisplaysWithOptionsHookNext(options);
#endif
}

static void IGOUpdate(CGLContextObj ctxt)
{
    // Use "timestamp" to determine whether a context is still active
    const uint64_t MinFlushesForInactiveContext = 10;
    static uint64_t timeStamp = 0;
    static bool igoLostConnection = false;
    
    ++timeStamp;
    
    if (OGLDumpInstance)
        OGLDumpInstance->StartUpdate();
    
    // Make sure we don't have any pending OpenGL error from the game itself that would mess up our setup!! (thx Bejeweled3!)
    GLenum error = GL(glGetError());
    if (error)
    {
        static bool warnAboutGameOpenGLError = false;
        if (!warnAboutGameOpenGLError)
        {
            OriginIGO::IGOLogInfo("Getting OpenGL error from game itself");
            warnAboutGameOpenGLError = true;
        }
    }
    
    IGOApplication* instance = IGOApplication::instance();
    if (instance)
        instance->startTime();
    
    // Encapsulate use of DevContext
    {
        bool reset = false;
        OGLHook::DevContext devCtxt;
        
        // Check if we have any "unactive" contexts
        AvailableContextIterator iter = AvailableContexts.begin();
        for (; iter != AvailableContexts.end(); ++iter)
        {
            if ((timeStamp - iter->timeStamp) >= MinFlushesForInactiveContext)
                RequestResetIGO(iter->ctxt);
        }
        
        // Apply any pending resets
        ProcessResetIGORequests();
        
        
        if (ContextInUse != ctxt)
        {
#ifdef _DEBUG
            if (!OGLDumpInstance)
            {
                // HACK - FOR DEBUG ONLY - OTHERWISE SLOWS DOWN THINGS A BIT - OGLDumpInstance = new OriginIGO::OGLDump();
                if (OGLDumpInstance)
                    OGLDumpInstance->StartUpdate();
            }
#endif
            
            // Did we already encounter this context? + how many contexts are "active"
            int activeContextCnt = 0;
            bool usingCoreProfile = false;
            iter = AvailableContexts.begin();
            for (; iter != AvailableContexts.end(); ++iter)
            {
                if ((timeStamp - iter->timeStamp) < MinFlushesForInactiveContext)
                    ++activeContextCnt;
                
                if (iter->oglProperties.useCoreProfile)
                    usingCoreProfile = true;
                
                if (iter->ctxt == ctxt)
                {
                    // Make sure we know the context is being used
                    iter->timeStamp = timeStamp;
                    
                    devCtxt.base = *iter;
                    ContextInUse = ctxt;
                }
            }
            
            if (!devCtxt.base.shader) // didn't find it
            {
                // FIRST MAKE SURE WE DON'T MIX "COMPATIBILITY" 2.5 AND CORE PROFILES - THIS IS MAINLY FOR SIMCITY MAC RIGHT NOW - FORTUNATELY THEY SEEM TO CREATE THEIR 3.2 CORE PROFILE FIRST AND
                // ONLY USE 2.5 PROFILES WHEN CREATING "TEMPORARY" GRAPHICS CONTEXTS TO TRANSITION TO FULLSCREEN
                // BTW - IF WE DO FIND OUT THAT WE NEED 3.2 CORE PROFILES AND WE ALREADY ALLOCATED 2.5 PROFILES, WE COULD MAKE SURE NOT TO EXPLICITELY DELETE THE SHADERS BECAUSE IT LOOKS LIKE THE POTENTIAL
                // FOR A CRASH IS HIGHER (BUT THE MIX OF SHADERS MAY STILL GOING TO CREATE RENDERING ANOMALIES!)
                OpenGLProperties props = ExtractOpenGLProperties(ctxt);
                if (usingCoreProfile && !props.useCoreProfile)
                {
                    OriginIGO::IGOLogInfo("Ignoring 2.5 ctxt since we detected use of core profile");
                    instance->stopTime();
                    
                    if (OGLDumpInstance)
                        OGLDumpInstance->EndUpdate();
                    
                    return;
                }
                
                if (!usingCoreProfile && AvailableContexts.size() > 0 && props.useCoreProfile)
                {
                    OriginIGO::IGOLogInfo("Detecting new 3.2 core profile -> need to clean up %d old profile contexts", AvailableContexts.size());
                    
                    iter = AvailableContexts.begin();
                    for (; iter != AvailableContexts.end(); ++iter)
                        RequestResetIGO(iter->ctxt);
                    
                    ProcessResetIGORequests();
                    
                    usingCoreProfile = true;
                    activeContextCnt = 0;
                }
                
                
                // Ok, looks like we have a context to work with
                OriginIGO::IGOLogInfo("Setting up new rendering context (old:%p , new:%p)", ContextInUse, ctxt);
                ContextInUse = ctxt;
                
                OpenGLExtensions exts = ExtractOpenGLExtensions(props.useCoreProfile);
                IGOWindowOGLShader* shader = new IGOWindowOGLShader(props.useCoreProfile);
                IGOBackgroundOGLShader* bgShader = new IGOBackgroundOGLShader(props.useCoreProfile);
                if (!shader->IsValid() || !bgShader->IsValid())
                {
                    OriginIGO::IGOLogInfo("Shader failed - global disable of OIG");
                    delete shader;
                    shader = NULL;
                    
                    delete bgShader;
                    bgShader = NULL;
                }
                
                else
                {
                    // Add new supported context
                    OGLHook::AvailableContext availableCtxt;
                    
                    availableCtxt.ctxt = ctxt;
                    availableCtxt.shader = shader;
                    availableCtxt.bgShader = bgShader;
                    availableCtxt.oglProperties = props;
                    availableCtxt.oglExtensions = exts;
                    availableCtxt.timeStamp = timeStamp;
                    
                    AvailableContexts.push_back(availableCtxt);
                    devCtxt.base = availableCtxt;
                    ++activeContextCnt;
                    
                }
            }
            
            bool enableIGO = devCtxt.base.shader && devCtxt.base.bgShader && activeContextCnt == 1 && !igoLostConnection;
            if (!TextureAllocationContext && devCtxt.base.shader && devCtxt.base.bgShader)
                TextureAllocationContext = ctxt;
            
            SetGlobalEnableIGO(enableIGO);
            
            if (!enableIGO)
            {
                OriginIGO::IGOLogInfo("IGO disabled for new context %p on thread 0x%08x", ctxt, GetCurrentThreadId());
                OriginIGO::IGOLogInfo("Shader: %p", devCtxt.base.shader);
                OriginIGO::IGOLogInfo("Background Shader: %p", devCtxt.base.bgShader);
                OriginIGO::IGOLogInfo("Timestamp: %lld", devCtxt.base.timeStamp);
                OriginIGO::IGOLogInfo("Active contexts: %d", activeContextCnt);
                OriginIGO::IGOLogInfo("Allocation ctxt: %p", TextureAllocationContext);
            }
            
            else
                OriginIGO::IGOLogInfo("IGO enabled for new context %p on thread 0x%08x (ts=%lld)", ctxt, GetCurrentThreadId(), devCtxt.base.timeStamp);

            // Reset now if we can afford to do it (ie we are on the current thread) - even if we disable OIG right now because
            // we have multiple contexts
            if (enableIGO)
                reset = true;
        }
        
        else
        {
            // Lookup our shader + how many contexts are "active"
            int activeContextCnt = 0;
            iter = AvailableContexts.begin();
            for (; iter != AvailableContexts.end(); ++iter)
            {
                if ((timeStamp - iter->timeStamp) < MinFlushesForInactiveContext)
                    ++activeContextCnt;
                
                if (iter->ctxt == ctxt)
                {
                    // Make sure we know the context is being used
                    iter->timeStamp = timeStamp;
                    
                    devCtxt.base = *iter;
                }
            }
            
            // Check if we are still disabled
            bool enableIGO = devCtxt.base.shader && devCtxt.base.bgShader && activeContextCnt == 1 &&  !igoLostConnection;
            if (!TextureAllocationContext)
                TextureAllocationContext = ctxt;
            
            SetGlobalEnableIGO(enableIGO);
            
            
            static bool wasEnabled = false;
            if (enableIGO != wasEnabled)
            {
                if (!enableIGO)
                {
                    OriginIGO::IGOLogInfo("IGO disabled for current context %p on thread 0x%08x", ctxt, GetCurrentThreadId());
                    OriginIGO::IGOLogInfo("Shader: %p", devCtxt.base.shader);
                    OriginIGO::IGOLogInfo("Background Shader: %p", devCtxt.base.bgShader);
                    OriginIGO::IGOLogInfo("Timestamp: %lld", devCtxt.base.timeStamp);
                    OriginIGO::IGOLogInfo("Active contexts: %d", activeContextCnt);
                    OriginIGO::IGOLogInfo("Allocation context: %p", TextureAllocationContext);
                }
                else
                    OriginIGO::IGOLogInfo("IGO enabled for current context %p on thread 0x%08x (ts=%lld)", ctxt, GetCurrentThreadId(), devCtxt.base.timeStamp);
            }
            
            wasEnabled = enableIGO;
        }
        
        if (_igoFrameContext)
        {
            if (reset)
                _igoFrameContext->Reset(ctxt);
            
            if (!_igoFrameContext->SuccessfullyExtractedViewProperties())
                ExtractCGLContextViewProperties(ctxt, SetRenderViewPropertiesCallback);
            
            // Make sure to flush the new window properties before re-enabling IGO after a reset
            bool enableFakeMoveEvents = false;
            if (instance)
                enableFakeMoveEvents = instance->isActive() && instance->hasFocus();
            
            _igoFrameContext->Flush(ctxt, enableFakeMoveEvents);
            SetInputFrameIdx(_igoFrameContext->GetFrameIdx());
            
            if (reset)
            {
                // Was IGO on when the context changed?
                if (WasIGOOnWhenChangingContext)
                    SetCursorIGOState(true, true, false);
            }
            
            int newWidth = _igoFrameContext->Width();
            int newHeight = _igoFrameContext->Height();
            if (!instance)
            {
                Init(ctxt, newWidth, newHeight);
                
                if (OGLDumpInstance)
                    OGLDumpInstance->EndUpdate();
                
                return;
            }
            
            gWindowedMode = _igoFrameContext->IsWindowed();
            instance->setWindowedMode(gWindowedMode);
            
            bool isIGOAllowed = IsIGOAllowed();
            instance->setScreenSizeAndRenderState(newWidth,newHeight, _igoFrameContext->PixelToPointScaleFactor(), !isIGOAllowed);
            instance->setWindowScreenSize(_igoFrameContext->BackingStoreWidth(), _igoFrameContext->BackingStoreHeight());
            
            // Check if we still have a valid connection with the server
            static uint32_t connectionCount = 0;
            if (!igoLostConnection)
            {
                if (!instance->checkPipe())
                {
                    // No connection -> hide IGO
                    if (instance->isActive())
                        instance->toggleIGO();
                    
                    if (connectionCount > 0)
                        igoLostConnection = true;
                }
                
                else
                    ++connectionCount;
            }
#ifndef DISABLE_RECONNECT
            else
            {
                if (instance->checkPipe())
                {
                    connectionCount = 0;
                    igoLostConnection = false;
                }
            }
#endif
            
            if (reset)
            {
                instance->reset();
            }
            
            if (isIGOAllowed && devCtxt.base.shader && devCtxt.base.bgShader)
            {
                // setup projection matrix
                ZeroMemory(devCtxt.projMatrix, sizeof(devCtxt.projMatrix));
                
                uint32_t screenWidth = instance->getScreenWidth();
                if (screenWidth == 0)
                    screenWidth = 1;
                
                uint32_t screenHeight = instance->getScreenHeight();
                if (screenHeight == 0)
                    screenHeight = 1;
                
                devCtxt.projMatrix[0]  =  2.0f / (screenWidth * _igoFrameContext->PixelToPointScaleFactor());
                devCtxt.projMatrix[5]  =  2.0f / (screenHeight * _igoFrameContext->PixelToPointScaleFactor());
                devCtxt.projMatrix[10] = -2.0f / (9);
                devCtxt.projMatrix[14] =  2.0f / (9);
                devCtxt.projMatrix[15] =  1.0f;
                
                devCtxt.pixelToPointScaleFactor = _igoFrameContext->PixelToPointScaleFactor();
                
                // Render with OpenGL code 3.2 (shaders!)
                instance->render(&devCtxt);
            }
            
            else
            {
                // We still need to process some of the messages (like the focus) not to confuse the entire system when dealing with multiple open games
                instance->render(NULL);
            }
            
            // Check debug option to reset context
            if (instance->resetContext())
                ResetIGO(ctxt);
        }
    } // clean up DevContext
    
    instance->stopTime();
    
    if (OGLDumpInstance)
        OGLDumpInstance->EndUpdate();
}

// Methods required for EA libs
void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return ::new char[size];
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return ::new char[size];
}

DEFINE_HOOK(CGError, CGLCreateContextHook, (CGLPixelFormatObj pix, CGLContextObj share, CGLContextObj* ctx))
{
    CGError result = CGLCreateContextHookNext(pix, share, ctx);
    OriginIGO::IGOLogInfo("Calling CGLCreateContext (%p) on thread 0x%08x...", *ctx, GetCurrentThreadId());
    return result;
}

/*
 DEFINE_HOOK(CGError, CGLSetCurrentContextHook, (CGLContextObj ctx))
 {
 OriginIGO::IGOLogInfo("Calling CGLSetCurrentContext (%p)...", ctx);
 return CGLSetCurrentContextHookNext(ctx);
 }
 */

DEFINE_HOOK(void, CGRestorePermanentDisplayConfigurationHook, ())
{
    OriginIGO::IGOLogInfo("Calling CGRestorePermanentDisplayConfiguration...");
    
#ifndef SKIP_DISPLAY_CAPTURING
    CGRestorePermanentDisplayConfigurationHookNext();
#endif
}

/*
 DEFINE_HOOK(CGError, CGBeginDisplayConfigurationHook, (CGDisplayConfigRef* pConfigRef))
 {
 OriginIGO::IGOLogInfo("Calling CGBeginDisplayConfiguration...");
 return BypassDisplayCapture? kCGErrorSuccess : CGBeginDisplayConfigurationHookNext(pConfigRef);
 }
 
 DEFINE_HOOK(CGError, CGCancelDisplayConfigurationHook, (CGDisplayConfigRef configRef))
 {
 OriginIGO::IGOLogInfo("Calling CGCancelDisplayConfiguration...");
 return BypassDisplayCapture? kCGErrorSuccess : CGCancelDisplayConfigurationHookNext(configRef);
 }
 
 DEFINE_HOOK(CGError, CGCompleteDisplayConfigurationHook, (CGDisplayConfigRef configRef, CGConfigureOption option))
 {
 OriginIGO::IGOLogInfo("Calling CGCompleteDisplayConfiguration...");
 return BypassDisplayCapture? kCGErrorSuccess : CGCompleteDisplayConfigurationHookNext(configRef, option);
 }
 */

DEFINE_HOOK(CGError, CGConfigureDisplayWithDisplayModeHook, (CGDisplayConfigRef config, CGDirectDisplayID display, CGDisplayModeRef mode, CFDictionaryRef options))
{
    OriginIGO::IGOLogInfo("Calling CGConfigureDisplayWithDisplayMode...");
    
#ifdef SKIP_DISPLAY_CAPTURING
    return kCGErrorSuccess;
#else
    return CGConfigureDisplayWithDisplayModeHookNext(config, display, mode, options);
#endif
}

DEFINE_HOOK(CGError, CGDisplaySetDisplayModeHook, (CGDirectDisplayID display, CGDisplayModeRef mode, CFDictionaryRef options))
{
    OriginIGO::IGOLogInfo("Calling CGDisplaySetDisplayMode...");
    
#ifdef SKIP_DISPLAY_CAPTURING
    return kCGErrorSuccess;
#else
    return CGDisplaySetDisplayModeHookNext(display, mode, options);
#endif
}

DEFINE_HOOK(CGError, CGDisplaySwitchToModeHook, (CGDirectDisplayID display, CFDictionaryRef mode))
{
    OriginIGO::IGOLogInfo("Calling CGDisplaySwitchToMode...");
    
    CFNumberRef ioKitMode;
    if (CFDictionaryGetValueIfPresent(mode, kCGDisplayMode, (const void**)&ioKitMode))
    {
        long value = 0;
        CFNumberGetValue(ioKitMode, kCFNumberLongType, &value);
        OriginIGO::IGOLogInfo("IO Kit Mode = %ld", value);
    }
    
#ifdef SKIP_DISPLAY_CAPTURING
    return kCGErrorSuccess;
#else
    return CGDisplaySwitchToModeHookNext(display, mode);
#endif
}

DEFINE_HOOK(GLboolean, aglSetFullScreenHook, (AGLContext ctx, GLsizei width, GLsizei height, GLsizei freq, GLint device))
{
    OriginIGO::IGOLogInfo("Calling aglSetFullScreen %d, %d, %d, %d...", width, height, freq, device);
    return aglSetFullScreenHookNext(ctx, width, height, freq, device);
}

DEFINE_HOOK(void, aglSwapBuffersHook, (AGLContext ctx))
{
    bool setupCompleted = SetupCompleted;
    if (setupCompleted)
    {
        ExtractAGLContextViewProperties(ctx, SetRenderViewPropertiesCallback);
    }
    
    return aglSwapBuffersHookNext(ctx);
}

DEFINE_HOOK(AGLContext, aglCreateContextHook, (AGLPixelFormat pix, AGLContext share))
{
    AGLContext ctx = aglCreateContextHookNext(pix, share);
    OriginIGO::IGOLogInfo("Calling aglCreateContext (%p) on thread 0x%08x...", ctx, GetCurrentThreadId());
    return ctx;
}

DEFINE_HOOK(GLboolean, aglSetCurrentContextHook, (AGLContext ctx))
{
    // LEGO games set this guy all the time -> kill the game when debugging!
    //OriginIGO::IGOLogInfo("Calling aglSetCurrentContext (%p) on thread 0x%08x...", ctx, GetCurrentThreadId());
    return aglSetCurrentContextHookNext(ctx);
}

DEFINE_HOOK(GLboolean, aglSetDrawableHook, (AGLContext ctx, AGLDrawable draw))
{
    OriginIGO::IGOLogInfo("Calling aglSetDrawable (%p - %p)...", ctx, draw);
    return aglSetDrawableHookNext(ctx, draw);
}

DEFINE_HOOK(void, aglResetLibraryHook, (void))
{
    OriginIGO::IGOLogInfo("Calling aglResetLibrary...");
    return aglResetLibraryHookNext();
}

static bool flushDrawableUsed = false;
DEFINE_HOOK(void, glFlushHook, ())
{
    //OriginIGO::IGOLogInfo("Calling flush on thread 0x%08x...", GetCurrentThreadId());
    
    bool setupCompleted = SetupCompleted;
    if (!setupCompleted)
    {
        glFlushHookNext();
        return;
    }
    
    CGLContextObj ctxt = NULL;
    if (!flushDrawableUsed)
    {
        // This one sucks... but used for Popcap games
        static int flushCountDown = 10;
        
        if (flushCountDown > 0)
            --flushCountDown;
        
        if (flushCountDown == 0)
        {
            --flushCountDown; // don't do this again
            CGLContextObj currCtxt = CGLGetCurrentContext();
            if (currCtxt)
            {
                CGLPixelFormatObj pFormat = CGLGetPixelFormat(currCtxt);
                if (pFormat)
                {
                    GLint virtualScreen = 0;
                    GLint doubleBuffered = 0;
                    CGLError error = CGLGetVirtualScreen(currCtxt, &virtualScreen);
                    if (error == kCGLNoError)
                    {
                        error = CGLDescribePixelFormat(pFormat, virtualScreen, kCGLPFADoubleBuffer, &doubleBuffered);
                        if (error == kCGLNoError)
                        {
                            if (!doubleBuffered)
                            {
                                --flushCountDown;
                                OriginIGO::IGOLogInfo("Using Flush hook for single buffered game (popcap?)");
                            }
                        }
                    }
                }
            }
        }
        
        if (flushCountDown == -2)
        {
            ctxt = CGLGetCurrentContext();
            if (ctxt)
            {
                GLint hasDrawable = 0;
                CGLError error = CGLGetParameter(ctxt, kCGLCPHasDrawable, &hasDrawable);
                if (error != kCGLNoError || hasDrawable)
                    IGOUpdate(ctxt);
            }
        }
    }
    
    glFlushHookNext();
    
    // DEBUG - dump final buffer
    if (ctxt && _igoFrameContext)
    {
        IGOApplication* instance = IGOApplication::instance();
        if (instance && instance->dumpWindowBitmaps())
            _igoFrameContext->dumpBuffer(ctxt);
    }
}

DEFINE_HOOK(CGLError, CGLFlushDrawableHook, (CGLContextObj ctxt))
{
    //OriginIGO::IGOLogInfo("Calling flushDrawable on thread 0x%08x...", GetCurrentThreadId());
    
    bool setupCompleted = SetupCompleted;
    if (!setupCompleted)
        return CGLFlushDrawableHookNext(ctxt);
    
    if (!flushDrawableUsed)
        flushDrawableUsed = true;
    
    if (ctxt != NULL)
    {
        CGLLockContextHookNext(ctxt);
        CGLContextObj currCtxt = CGLGetCurrentContext();
        if (ctxt != currCtxt)
            CGLSetCurrentContext(ctxt);
        
        GLint hasDrawable = 0;
        CGLError error = CGLGetParameter(ctxt, kCGLCPHasDrawable, &hasDrawable);
        if (error != kCGLNoError || hasDrawable)
            IGOUpdate(ctxt);
        
        if (ctxt != currCtxt)
            CGLSetCurrentContext(currCtxt);
        
        CGLUnlockContextHookNext(ctxt);
    }
    
    CGLError flushError = CGLFlushDrawableHookNext(ctxt);
    
    // DEBUG - dump final buffer
    if (ctxt && _igoFrameContext)
    {
        IGOApplication* instance = IGOApplication::instance();
        if (instance && instance->dumpWindowBitmaps())
        {
            CGLLockContextHookNext(ctxt);
            CGLContextObj currCtxt = CGLGetCurrentContext();
            if (ctxt != currCtxt)
                CGLSetCurrentContext(ctxt);
            
            _igoFrameContext->dumpBuffer(ctxt);
            
            if (ctxt != currCtxt)
                CGLSetCurrentContext(currCtxt);
            
            CGLUnlockContextHookNext(ctxt);
        }
    }
    
    return flushError;
}

// Dump content of an environment dictionary
void DumpEnvDictionary(CFDictionaryRef env)
{
    if (env)
    {
        CFIndex cnt = CFDictionaryGetCount(env);
        CFStringRef* keys = new CFStringRef[cnt];
        CFStringRef* values = new CFStringRef[cnt];
        CFDictionaryGetKeysAndValues(env, (const void**)keys, (const void**)values);
        
        char keyString[256];
        char valueString[256];
        for (CFIndex idx = 0; idx < cnt; ++idx)
        {
            Boolean validKey = CFStringGetCString(keys[idx], keyString, sizeof(keyString), kCFStringEncodingUTF8);
            Boolean validValue = CFStringGetCString(values[idx], valueString, sizeof(valueString), kCFStringEncodingUTF8);
            if (validKey && validValue)
            {
                OriginIGO::IGOLogInfo("   %d. %s = %s", idx + 1, keyString, valueString);
            }
            
            else
            {
                OriginIGO::IGOLogInfo("   %d. skipping", idx + 1);
            }
        }
        
        delete []keys;
        delete []values;
    }
}

DEFINE_HOOK(OSStatus, LSOpenApplicationHook, (const LSApplicationParameters* appParams, ProcessSerialNumber* outPSN))
{
    OriginIGO::IGOLogInfo("Calling LSOpenApplication...");
    
    static const CFStringRef DYLD_INSERT_LIBRARIES_KEY = CFSTR("DYLD_INSERT_LIBRARIES");
    static const CFStringRef DYLD_INSERT_LIBRARIES_SEPARATOR = CFSTR(":");
    
    LSApplicationParameters newParams;
    const LSApplicationParameters* actualParams = appParams;
    
    // Make sure we have access to this module's path
    CFStringRef injectEnv = NULL;
    
    // GetIGOLoaderPath is defined in OIGLoader, where we hook up execve/posix_spawn early
    typedef const char* (*LoaderPathFcnDef)();
    LoaderPathFcnDef loaderPathFcn = (LoaderPathFcnDef)dlsym(RTLD_DEFAULT, "GetIGOLoaderPath");
    if (loaderPathFcn)
    {
        const char* loaderPath = loaderPathFcn();
        if (loaderPath)
            injectEnv = CFStringCreateWithCString(NULL, loaderPath, kCFStringEncodingUTF8);
    }
    
    if (injectEnv && appParams)
    {
#if DEBUG
        {
            CFURLRef appURL = CFURLCreateFromFSRef(NULL, appParams->application);
            
            unsigned char appName[256];
            if (CFURLGetFileSystemRepresentation(appURL, true, appName, sizeof(appName)))
                OriginIGO::IGOLogInfo(" - FileName: %s", appName);

            CFDictionaryRef env = appParams->environment;
            OriginIGO::IGOLogInfo(" - Initial envs:");
            DumpEnvDictionary(env);
        }
#endif
        CFDictionaryRef env = appParams->environment;
        if (env)
        {
            CFIndex cnt = CFDictionaryGetCount(env);
            CFStringRef* keys = new CFStringRef[cnt];
            CFStringRef* values = new CFStringRef[cnt];
            CFDictionaryGetKeysAndValues(env, (const void**)keys, (const void**)values);
    
            CFMutableDictionaryRef newEnv = CFDictionaryCreateMutable(NULL, cnt + 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
            
            newParams = *appParams;
            actualParams = &newParams;
            newParams.environment = newEnv;
            
            for (CFIndex idx = 0; idx < cnt; ++idx)
            {
                if (CFStringCompare(keys[idx], DYLD_INSERT_LIBRARIES_KEY, kCFCompareCaseInsensitive) == kCFCompareEqualTo)
                {
#ifdef OVERRIDE_STEAM
                    CFDictionaryAddValue(newEnv, DYLD_INSERT_LIBRARIES_KEY, injectEnv);
#else
                    const void* data[2];
                    data[0] = injectEnv;
                    data[1] = values[idx];
                    CFArrayRef stringArray = CFArrayCreate(NULL, data, 2, &kCFTypeArrayCallBacks);
                    CFStringRef newValue = CFStringCreateByCombiningStrings(NULL, stringArray, DYLD_INSERT_LIBRARIES_SEPARATOR);
                    
                    CFDictionaryAddValue(newEnv, DYLD_INSERT_LIBRARIES_KEY, newValue);
                    
                    CFRelease(stringArray);
                    CFRelease(newValue);
#endif
                }
                
                else
                {
                    CFDictionaryAddValue(newEnv, keys[idx], values[idx]);
                }
            }
            
            delete []keys;
            delete []values;
        }
        
        else
        {
            env = CFDictionaryCreate(NULL, (const void**)&DYLD_INSERT_LIBRARIES_KEY, (const void**)&injectEnv, 1, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

            newParams = *appParams;
            actualParams = &newParams;
            newParams.environment = env;
        }
        
#if DEBUG
        OriginIGO::IGOLogInfo(" - Actual envs:");
        DumpEnvDictionary(actualParams->environment);
#endif
    }
    
    OSStatus result = LSOpenApplicationHookNext(actualParams, outPSN);
    
    if (actualParams != appParams)
    {
        CFRelease(newParams.environment);
        CFRelease(injectEnv);
    }
    
    return result;
}

typedef CGError (*CGDisplayRegisterReconfigurationCallbackFunc)(CGDisplayReconfigurationCallBack proc, void* userInfo);
void DisplayRegisterCallbackImpl(CGDirectDisplayID display, CGDisplayChangeSummaryFlags flags, void* userInfo)
{
    CGDisplayModeRef mode = CGDisplayCopyDisplayMode(display);
    
#ifdef DEBUG
    long height = CGDisplayModeGetHeight(mode);
    long width = CGDisplayModeGetWidth(mode);
    
    OriginIGO::IGOLogInfo("Calling display register callback function (%d, %d, %ld, %ld)...", display, flags, width, height);
#endif
    
    CGDisplayModeRelease(mode);
}

typedef CGError (*CGRegisterScreenRefreshCallbackFunc)(CGScreenRefreshCallback proc, void* userInfo);
void ScreenRefreshCallbackImpl(CGRectCount count, const CGRect* rectArray, void* userInfo)
{
    
}

static bool OGLInit(void* module)
{
    bool success = false;
    
    if (!SetupCompleted)
    {
        // Don't inject if Steam overlay is running
        if (IsSteamRunning())
        {
            OriginIGO::IGOLogInfo("Detected Steam overlay running");
            TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_THIRD_PARTY, TelemetryRenderer_OPENGL, "Detected Steam overlay");
            return false;
        }
        
        if (!OGLLoadFunctions(module))
        {
            OriginIGO::IGOLogInfo("Hooking init failed - missing OpenGL methods");
            TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_GENERIC, TelemetryRenderer_OPENGL, "Missing OpenGL methods");
            return false;
        }
        
        // Make sure Cocoa is initialized (mainly to access NSCursors)
        InitializeCocoa();
        
        _igoFrameContext = new IGOFrameContext();
        
        if (dlsym(RTLD_DEFAULT, "NSClassFromString"))
        {
            success = SetupRenderHooks(SetRenderViewPropertiesCallback, MainMenuActivationCallback);
            if (success)
            {
                success &= SetupInputHooks();
                if (success)
                {
                    success &= SetupCursorHooks();
                    if (!success)
                        TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_GENERIC, TelemetryRenderer_OPENGL, "Failed to setup cursor hooks");
                }
                
                else
                    TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_GENERIC, TelemetryRenderer_OPENGL, "Failed to setup input hooks");
            }
            else
                TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_GENERIC, TelemetryRenderer_OPENGL, "Failed to setup render hooks");
            
            if (success)
            {
                success &= HookAPI(NULL, "CGLFlushDrawable", (LPVOID)CGLFlushDrawableHook, (LPVOID *)&CGLFlushDrawableHookNext);
                success &= HookAPI(NULL, "glFlush", (LPVOID)glFlushHook, (LPVOID*)&glFlushHookNext);
                success &= HookAPI(NULL, "CGLCreateContext", (LPVOID)CGLCreateContextHook, (LPVOID*)&CGLCreateContextHookNext);
                success &= HookAPI(NULL, "CGLReleaseContext", (LPVOID)CGLReleaseContextHook, (LPVOID*)&CGLReleaseContextHookNext);
                success &= HookAPI(NULL, "CGLDestroyContext", (LPVOID)CGLDestroyContextHook, (LPVOID*)&CGLDestroyContextHookNext);
                success &= HookAPI(NULL, "CGLLockContext", (LPVOID)CGLLockContextHook, (LPVOID*)&CGLLockContextHookNext);
                success &= HookAPI(NULL, "CGLUnlockContext", (LPVOID)CGLUnlockContextHook, (LPVOID*)&CGLUnlockContextHookNext);
                success &= HookAPI(NULL, "CGLSetParameter", (LPVOID)CGLSetParameterHook, (LPVOID*)&CGLSetParameterHookNext);
                
                // Not using CGLSetCurrentContext right now, so skipping - otherwise need to update override to handle 0xE8 0x00 0x00 0x00 0x00,
                // ie jump to next instruction to grab ip with a pop %eax
                //HookAPI(NULL, "CGLSetCurrentContext", (LPVOID)CGLSetCurrentContextHook, (LPVOID*)&CGLSetCurrentContextHookNext);
                HookAPI(NULL, "CGDisplayCapture", (LPVOID)CGDisplayCaptureHook, (LPVOID *)&CGDisplayCaptureHookNext);
                HookAPI(NULL, "CGDisplayRelease", (LPVOID)CGDisplayReleaseHook, (LPVOID *)&CGDisplayReleaseHookNext);
                HookAPI(NULL, "CGReleaseAllDisplays", (LPVOID)CGReleaseAllDisplaysHook, (LPVOID *)&CGReleaseAllDisplaysHookNext);
                HookAPI(NULL, "CGRestorePermanentDisplayConfiguration", (LPVOID)CGRestorePermanentDisplayConfigurationHook, (LPVOID *)&CGRestorePermanentDisplayConfigurationHookNext);
                HookAPI(NULL, "CGDisplayCaptureWithOptions", (LPVOID)CGDisplayCaptureWithOptionsHook, (LPVOID *)&CGDisplayCaptureWithOptionsHookNext);
                HookAPI(NULL, "CGCaptureAllDisplays", (LPVOID)CGCaptureAllDisplaysHook, (LPVOID *)&CGCaptureAllDisplaysHookNext);
                HookAPI(NULL, "CGCaptureAllDisplaysWithOptions", (LPVOID)CGCaptureAllDisplaysWithOptionsHook, (LPVOID *)&CGCaptureAllDisplaysWithOptionsHookNext);
                //HookAPI(NULL, "CGBeginDisplayConfiguration", (LPVOID)CGBeginDisplayConfigurationHook, (LPVOID *)&CGBeginDisplayConfigurationHookNext);
                //HookAPI(NULL, "CGCancelDisplayConfiguration", (LPVOID)CGCancelDisplayConfigurationHook, (LPVOID *)&CGCancelDisplayConfigurationHookNext);
                //HookAPI(NULL, "CGCompleteDisplayConfiguration", (LPVOID)CGCompleteDisplayConfigurationHook, (LPVOID *)&CGCompleteDisplayConfigurationHookNext);
                HookAPI(NULL, "CGConfigureDisplayWithDisplayMode", (LPVOID)CGConfigureDisplayWithDisplayModeHook, (LPVOID *)&CGConfigureDisplayWithDisplayModeHookNext);
                HookAPI(NULL, "CGDisplaySetDisplayMode", (LPVOID)CGDisplaySetDisplayModeHook, (LPVOID *)&CGDisplaySetDisplayModeHookNext);
                HookAPI(NULL, "CGDisplaySwitchToMode", (LPVOID)CGDisplaySwitchToModeHook, (LPVOID *)&CGDisplaySwitchToModeHookNext);
                // Override for Steam
                HookAPI(NULL, "LSOpenApplication", (LPVOID)LSOpenApplicationHook, (LPVOID*)&LSOpenApplicationHookNext);
                
                // Do we have AGL available around?
                if (HookAPI(NULL, "aglSwapBuffers", (LPVOID)aglSwapBuffersHook, (LPVOID*)&aglSwapBuffersHookNext))
                {
                    success &= HookAPI(NULL, "aglCreateContext", (LPVOID)aglCreateContextHook, (LPVOID*)&aglCreateContextHookNext);
                    success &= HookAPI(NULL, "aglDestroyContext", (LPVOID)aglDestroyContextHook, (LPVOID*)&aglDestroyContextHookNext);
                    success &= HookAPI(NULL, "aglSetCurrentContext", (LPVOID)aglSetCurrentContextHook, (LPVOID*)&aglSetCurrentContextHookNext);
                    
                    HookAPI(NULL, "aglSetDrawable", (LPVOID)aglSetDrawableHook, (LPVOID*)&aglSetDrawableHookNext);
                    HookAPI(NULL, "aglSetFullScreen", (LPVOID)aglSetFullScreenHook, (LPVOID*)&aglSetFullScreenHookNext);
                    HookAPI(NULL, "aglResetLibrary", (LPVOID)aglResetLibraryHook, (LPVOID*)&aglResetLibraryHookNext);
                }
                
#if DEBUG // DO NOT ENABLE THIS CODE IN RELEASE!!!!
                // Register callbacks only if we're good to go
                if (success)
                {
                    CGDisplayRegisterReconfigurationCallbackFunc displayRegisterCallback = (CGDisplayRegisterReconfigurationCallbackFunc)dlsym(RTLD_DEFAULT, "CGDisplayRegisterReconfigurationCallback");
                    if (displayRegisterCallback)
                    {
                        CGError error = displayRegisterCallback(DisplayRegisterCallbackImpl, NULL);
                        if (error == kCGErrorSuccess)
                        {
                            OriginIGO::IGOLogInfo("Registered for display callback");
                        }
                        
                        else
                            OriginIGO::IGOLogWarn("Unable to register for display callback (%d)", error);
                    }
                    
                    else
                        OriginIGO::IGOLogWarn("Unable to find CGDisplayRegisterReconfigurationCallback function");
                    
                    CGRegisterScreenRefreshCallbackFunc screenRefreshCallback = (CGRegisterScreenRefreshCallbackFunc)dlsym(RTLD_DEFAULT, "CGRegisterScreenRefreshCallback");
                    if (screenRefreshCallback)
                    {
                        CGError error = screenRefreshCallback(ScreenRefreshCallbackImpl, NULL);
                        if (error == kCGErrorSuccess)
                        {
                            OriginIGO::IGOLogInfo("Registered for screen refresh callback");
                        }
                        
                        else
                            OriginIGO::IGOLogWarn("Unable to register for refresh display callback (%d)", error);
                    }
                    else
                        OriginIGO::IGOLogWarn("Unable to find CGRegisterScreenRefreshCallback function");
                }
#endif
            }
        }
        
        // Clean up if things didn't work out!
        if (!success)
        {
            OGLHook::Cleanup();
        }
        
        SetupCompleted = true;
        
        EnableRenderHooks(true);
        EnableCursorHooks(true);
        EnableInputHooks(true);
        
#ifdef SKIP_DISPLAY_CAPTURING
        // Uncapture displays if necessary
        if (CGReleaseAllDisplaysHookNext)
            CGReleaseAllDisplaysHookNext();
#endif
    }
    
    return success;
}

bool OGLHook::TryHook()
{
    // do not hook another GFX api, if we already have IGO running in this process!!!
    if (IGOApplication::instance() && (IGOApplication::instance()->rendererType() != RendererOGL))
        return false;
    
    return OGLInit(RTLD_DEFAULT);
}

void OGLHook::Cleanup()
{
    OriginIGO::IGOLogInfo("Cleaning up OIG...");
    
    // Disable some of the hooks to prevent locks
    SetupCompleted = false;
    
#if 0 // Don't do any of this because of race conditions
    EnableInputHooks(false);
    EnableCursorHooks(false);
    EnableRenderHooks(false);
    
    delete _igoFrameContext;
    _igoFrameContext = NULL;
    
    IGOApplication::deleteInstance();
    
    UnhookAPI(NULL, "CGLFlushDrawable", (LPVOID *)&CGLFlushDrawableHookNext);
    UnhookAPI(NULL, "glFlush", (LPVOID*)&glFlushHookNext);
    UnhookAPI(NULL, "CGLCreateContext", (LPVOID*)&CGLCreateContextHookNext);
    UnhookAPI(NULL, "CGLDestroyContext", (LPVOID*)&CGLDestroyContextHookNext);
    UnhookAPI(NULL, "CGLSetParameter", (LPVOID*)&CGLSetParameterHookNext);
    
    //UnhookAPI(NULL, "CGLSetCurrentContext", (LPVOID*)&CGLSetCurrentContextHookNext);
    UnhookAPI(NULL, "CGDisplayCapture", (LPVOID *)&CGDisplayCaptureHookNext);
    UnhookAPI(NULL, "CGDisplayRelease", (LPVOID *)&CGDisplayReleaseHookNext);
    UnhookAPI(NULL, "CGReleaseAllDisplays", (LPVOID *)&CGReleaseAllDisplaysHookNext);
    UnhookAPI(NULL, "CGRestorePermanentDisplayConfiguration", (LPVOID *)&CGRestorePermanentDisplayConfigurationHookNext);
    UnhookAPI(NULL, "CGDisplayCaptureWithOptions", (LPVOID *)&CGDisplayCaptureWithOptionsHookNext);
    UnhookAPI(NULL, "CGCaptureAllDisplays", (LPVOID *)&CGCaptureAllDisplaysHookNext);
    UnhookAPI(NULL, "CGCaptureAllDisplaysWithOptions", (LPVOID *)&CGCaptureAllDisplaysWithOptionsHookNext);
    //UnhookAPI(NULL, "CGBeginDisplayConfiguration", (LPVOID *)&CGBeginDisplayConfigurationHookNext);
    //UnhookAPI(NULL, "CGCancelDisplayConfiguration", (LPVOID *)&CGCancelDisplayConfigurationHookNext);
    //UnhookAPI(NULL, "CGCompleteDisplayConfiguration", (LPVOID *)&CGCompleteDisplayConfigurationHookNext);
    UnhookAPI(NULL, "CGConfigureDisplayWithDisplayMode", (LPVOID *)&CGConfigureDisplayWithDisplayModeHookNext);
    UnhookAPI(NULL, "CGDisplaySetDisplayMode", (LPVOID *)&CGDisplaySetDisplayModeHookNext);
    UnhookAPI(NULL, "CGDisplaySwitchToMode", (LPVOID *)&CGDisplaySwitchToModeHookNext);
    
    UnhookAPI(NULL, "LSOpenApplication", (LPVOID*)&LSOpenApplicationHookNext);

    UnhookAPI(NULL, "aglSwapBuffers", (LPVOID*)&aglSwapBuffersHookNext);
    UnhookAPI(NULL, "aglCreateContext", (LPVOID*)&aglCreateContextHookNext);
    UnhookAPI(NULL, "aglDestroyContext", (LPVOID*)&aglDestroyContextHookNext);
    UnhookAPI(NULL, "aglSetCurrentContext", (LPVOID*)&aglSetCurrentContextHookNext);
    UnhookAPI(NULL, "aglSetDrawable", (LPVOID*)&aglSetDrawableHookNext);
    
    UnhookAPI(NULL, "aglSetFullScreen", (LPVOID*)&aglSetFullScreenHookNext);
    UnhookAPI(NULL, "aglResetLibrary", (LPVOID*)&aglResetLibraryHookNext);
    
    CleanupRenderHooks();
    CleanupInputHooks();
    CleanupCursorHooks();
    
#ifdef _DEBUG
    if (OGLDumpInstance)
        delete OGLDumpInstance;
#endif
#endif
}

#endif // MAC OSX
