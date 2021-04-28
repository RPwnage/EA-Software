///////////////////////////////////////////////////////////////////////////////
// OGLHook.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef OGLHOOK_H
#define OGLHOOK_H

#ifdef ORIGIN_MAC

#import <OpenGL/OpenGL.h>

// Fwd decls
class IGOWindowOGLShader;
class IGOBackgroundOGLShader;

#include "OGLShader.h"

#else

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <gl/gl.h>
#include "eathread/eathread_futex.h"

// Fwd decls
namespace OriginIGO{
    class OGLShader;
    class IGOWindowOGLShader;
    class IGOBackgroundOGLShader;
}

#endif

class OGLHook
{
public:
    OGLHook();
    ~OGLHook();

    static bool TryHook();
    static void Cleanup();

#ifdef ORIGIN_MAC
    static bool LockGLContext();
    static void UnlockGLContext();
    
    // Encapsulate current game OpenGL state
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
        GLint vertexAttrib0Divisor;
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
        GLboolean unpackLocalStorage;
        
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
    
    // Encapsulate actual game context + associated OpenGL profile
    struct AvailableContext
    {
        uint64_t timeStamp;
        
        CGLContextObj ctxt;
        IGOWindowOGLShader* shader;
        IGOBackgroundOGLShader* bgShader;
        
        OpenGLProperties oglProperties;
        OpenGLExtensions oglExtensions;
    };
    
    // Encapsulate OpenGL state to limit saving/restoring for performance
    class DevContext
    {
        public:
            DevContext();
            ~DevContext();
        
            void SetRequiresRendering();
        
            AvailableContext base;

            OGLShader* lastShader;
            GLint texUnitIdx;
            GLfloat projMatrix[16];
            float pixelToPointScaleFactor;

        private:
            void SaveRenderState(const OpenGLProperties& oglProperties);
            void RestoreRenderState(const OpenGLProperties& oglProperties);
        
            OpenGLState mOglState;
            bool mStateSaved;
    };
#else
    static void TryHookLater(bool *isHooked = NULL);
    static void CleanupLater();
    static bool IsHooked() { return isHooked; };    
    static bool IsReadyForRehook();
    static bool IsBroken();

    static HANDLE mIGOOGLHookDestroyThreadHandle;
    static HANDLE mIGOOGLHookCreateThreadHandle;

    struct DevContext
    {
        OriginIGO::OGLShader* lastShader;   // used to limit the number of transitions
        OriginIGO::IGOWindowOGLShader* windowShader;
        OriginIGO::IGOBackgroundOGLShader* bgWindowShader;
        
        GLfloat projMatrix[16];

        GLint version;
        GLint maxTextureSize;
        GLint texUnitIdx;   // Unit used for our diffuse texture unit
    };

    static DevContext gContext;

private:
    static EA::Thread::Futex mInstanceHookMutex;
    static bool isHooked;
    static DWORD mLastHookTime;

#endif
};

#endif
