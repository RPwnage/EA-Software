//
//  OGLDump.cpp
//  IGO
//
//  Created by Frederic Meraud on 11/1/12.
//  Copyright (c) 2012 Electronic Arts. All rights reserved.
//

#ifdef ORIGIN_MAC

#include "OGLDump.h"
#include "IGOLogger.h"
#include "HookAPI.h"
#include "MacWindows.h"

#include <OpenGL/OpenGL.h>


static bool InIGOUpdate = false;

#define USER_DEFINE_HOOK_ARGS_ONLY(result, name, args, callArgs) \
typedef result (WINAPI *name ## DumpFunc) args; \
name ## DumpFunc name ## DumpHookNext = NULL; \
static result WINAPI name ## DumpHook args \

#define DEFINE_HOOK_ARGS_ONLY(result, name, args, callArgs) \
typedef result (WINAPI *name ## DumpFunc) args; \
name ## DumpFunc name ## DumpHookNext = NULL; \
static result WINAPI name ## DumpHook args \
{ \
    if (!InIGOUpdate) \
        IGO_TRACE("Calling " #name "..."); \
\
    name ## DumpHookNext callArgs; \
}

#define DEFINE_HOOK_ARGS_AND_RET(result, name, args, callArgs) \
typedef result (WINAPI *name ## DumpFunc) args; \
name ## DumpFunc name ## DumpHookNext = NULL; \
static result WINAPI name ## DumpHook args \
{ \
    if (!InIGOUpdate) \
        IGO_TRACE("Calling " #name "..."); \
\
    return name ## DumpHookNext callArgs; \
}

DEFINE_HOOK_ARGS_ONLY(void, glActiveTexture, (GLenum texture), (texture));
DEFINE_HOOK_ARGS_ONLY(void, glPixelStorei, (GLenum pname, GLint param), (pname, param));
DEFINE_HOOK_ARGS_ONLY(void, glColorMask, (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha), (red, green, blue, alpha));
DEFINE_HOOK_ARGS_ONLY(void, glPolygonMode, (GLenum face, GLenum mode), (face, mode));
DEFINE_HOOK_ARGS_ONLY(void, glBindBuffer, (GLenum target, GLuint buffer), (target, buffer));
DEFINE_HOOK_ARGS_ONLY(void, glBindTexture, (GLenum target, GLuint texture), (target, texture));

DEFINE_HOOK_ARGS_ONLY(void, glBlendFunc, (GLenum sfactor, GLenum dfactor), (sfactor, dfactor));
DEFINE_HOOK_ARGS_ONLY(void, glCullFace, (GLenum mode), (mode));
DEFINE_HOOK_ARGS_ONLY(void, glDeleteTextures, (GLsizei n, const GLuint *textures), (n, textures));
DEFINE_HOOK_ARGS_ONLY(void, glDepthFunc, (GLenum func), (func));
DEFINE_HOOK_ARGS_ONLY(void, glDepthMask, (GLboolean flag), (flag));
DEFINE_HOOK_ARGS_ONLY(void, glDepthRange, (GLclampd zNear, GLclampd zFar), (zNear, zFar));
DEFINE_HOOK_ARGS_ONLY(void, glDisable, (GLenum cap), (cap));

// LEGACY - will have to go when we start getting games for core profile v3.2
DEFINE_HOOK_ARGS_ONLY(void, glPopAttrib, (void), ());
DEFINE_HOOK_ARGS_ONLY(void, glPopClientAttrib, (void), ());
DEFINE_HOOK_ARGS_ONLY(void, glPushAttrib, (GLbitfield mask), (mask));
DEFINE_HOOK_ARGS_ONLY(void, glPushClientAttrib, (GLbitfield mask), (mask));
// LEGACY

DEFINE_HOOK_ARGS_ONLY(void, glDrawArrays, (GLenum mode, GLint first, GLsizei count), (mode, first, count));
DEFINE_HOOK_ARGS_ONLY(void, glEnable, (GLenum cap), (cap));
DEFINE_HOOK_ARGS_ONLY(void, glFinish, (void), ());
// DON'T HOOK - ALREADY USED IN OGLHOOK - DEFINE_HOOK_ARGS_ONLY(void, glFlush, (void), ());
DEFINE_HOOK_ARGS_ONLY(void, glGenTextures, (GLsizei n, GLuint *textures), (n, textures));
DEFINE_HOOK_ARGS_ONLY(void, glGetBooleanv, (GLenum pname, GLboolean *params), (pname, params));
DEFINE_HOOK_ARGS_AND_RET(GLenum, glGetError, (void), ());
DEFINE_HOOK_ARGS_ONLY(void, glGetIntegerv, (GLenum pname, GLint *params), (pname, params));
DEFINE_HOOK_ARGS_AND_RET(const GLubyte *, glGetString, (GLenum name), (name));
DEFINE_HOOK_ARGS_ONLY(void, glGetTexImage, (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels), (target, level, format, type, pixels));
DEFINE_HOOK_ARGS_ONLY(void, glGetTexLevelParameteriv, (GLenum target, GLint level, GLenum pname, GLint *params), (target, level, pname, params));
DEFINE_HOOK_ARGS_AND_RET(GLboolean, glIsTexture, (GLuint texture), (texture));
DEFINE_HOOK_ARGS_ONLY(void, glTexImage2D, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels), (target, level, internalformat, width, height, border, format, type, pixels));
DEFINE_HOOK_ARGS_ONLY(void, glTexParameteri, (GLenum target, GLenum pname, GLint param), (target, pname, param));
DEFINE_HOOK_ARGS_ONLY(void, glTexSubImage2D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels), (target, level, xoffset, yoffset, width, height, format, type, pixels));
DEFINE_HOOK_ARGS_ONLY(void, glUseProgram, (GLuint program), (program));
DEFINE_HOOK_ARGS_ONLY(void, glViewport, (GLint x, GLint y, GLsizei width, GLsizei height), (x, y, width, height));

DEFINE_HOOK_ARGS_AND_RET(GLuint, glCreateShader, (GLenum shaderType), (shaderType))
DEFINE_HOOK_ARGS_ONLY(void, glShaderSource, (GLuint shader, GLsizei count, const GLchar** string, const GLint* length), (shader, count, string, length));
DEFINE_HOOK_ARGS_ONLY(void, glCompileShader, (GLuint shader), (shader));
DEFINE_HOOK_ARGS_ONLY(void, glGetShaderInfoLog, (GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog), (shader, maxLength, length, infoLog));
DEFINE_HOOK_ARGS_AND_RET(GLuint, glCreateProgram, (void), ());
DEFINE_HOOK_ARGS_ONLY(void, glAttachShader, (GLuint program, GLuint shader), (program, shader));
DEFINE_HOOK_ARGS_ONLY(void, glLinkProgram, (GLuint program), (program));
DEFINE_HOOK_ARGS_ONLY(void, glGetProgramiv, (GLuint program, GLenum pname, GLint* params), (program, pname, params));
DEFINE_HOOK_ARGS_ONLY(void, glGetProgramInfoLog, (GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog), (program, maxLength, length, infoLog));
DEFINE_HOOK_ARGS_ONLY(void, glDetachShader, (GLuint program, GLuint shader), (program, shader));
DEFINE_HOOK_ARGS_ONLY(void, glDeleteProgram, (GLuint program), (program));
DEFINE_HOOK_ARGS_ONLY(void, glDeleteShader, (GLuint shader), (shader));
DEFINE_HOOK_ARGS_ONLY(void, glValidateProgram, (GLuint program), (program));
DEFINE_HOOK_ARGS_AND_RET(GLint, glGetUniformLocation, (GLuint program, const GLchar* name), (program, name));
DEFINE_HOOK_ARGS_ONLY(void, glUniform1f, (GLint location, GLfloat v0), (location, v0));
DEFINE_HOOK_ARGS_ONLY(void, glUniform1i, (GLint location, GLint v0), (location, v0));
DEFINE_HOOK_ARGS_ONLY(void, glUniformMatrix4fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat* value), (location, count, transpose, value));
DEFINE_HOOK_ARGS_AND_RET(GLint, glGetAttribLocation, (GLuint program, const GLchar* name), (program, name));
DEFINE_HOOK_ARGS_ONLY(void, glBindAttribLocation, (GLuint program, GLuint index, const GLchar* name), (program, index, name));
DEFINE_HOOK_ARGS_ONLY(void, glVertexAttribPointer, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer), (index, size, type, normalized, stride, pointer));
DEFINE_HOOK_ARGS_ONLY(void, glGetVertexAttribiv, (GLuint index, GLenum pname, GLint* params), (index, pname, params));
DEFINE_HOOK_ARGS_ONLY(void, glGetVertexAttribPointerv, (GLuint index, GLenum pname, void** pointer), (index, pname, pointer));
DEFINE_HOOK_ARGS_ONLY(void, glEnableVertexAttribArray, (GLuint index), (index));
DEFINE_HOOK_ARGS_ONLY(void, glDisableVertexAttribArray, (GLuint index), (index));
DEFINE_HOOK_ARGS_ONLY(void, glGenBuffers, (GLsizei n, GLuint* buffers), (n, buffers));
DEFINE_HOOK_ARGS_ONLY(void, glDeleteBuffers, (GLsizei n, const GLuint* buffers), (n, buffers));
DEFINE_HOOK_ARGS_ONLY(void, glBufferData, (GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage), (target, size, data, usage));
DEFINE_HOOK_ARGS_ONLY(void, glBindFramebuffer, (GLenum target, GLuint framebuffer), (target, framebuffer));
DEFINE_HOOK_ARGS_ONLY(void, glBlendEquation, (GLenum mode), (mode));
DEFINE_HOOK_ARGS_ONLY(void, glBlendFuncSeparate, (GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha), (srcRGB, dstRGB, srcAlpha, dstAlpha));
DEFINE_HOOK_ARGS_ONLY(void, glBlendEquationSeparate, (GLenum modeRGB, GLenum modeAlpha), (modeRGB, modeAlpha));


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

#define SETUPHOOK(name) HookAPI(NULL, #name, (LPVOID) name ## DumpHook, (LPVOID*)& name ## DumpHookNext)

OriginIGO::OGLDump::OGLDump()
{
    SETUPHOOK(glActiveTexture);
    SETUPHOOK(glPixelStorei);
    SETUPHOOK(glColorMask);
    SETUPHOOK(glPolygonMode);
    SETUPHOOK(glBindBuffer);
    SETUPHOOK(glBindTexture);
    SETUPHOOK(glBlendFunc);
    SETUPHOOK(glCullFace);
    SETUPHOOK(glDeleteTextures);
    SETUPHOOK(glDepthFunc);
    SETUPHOOK(glDepthMask);
    SETUPHOOK(glDepthRange);
    SETUPHOOK(glDisable);
    
    // LEGACY - will have to go when we start getting games for core profile v3.2
    SETUPHOOK(glPopAttrib);
    SETUPHOOK(glPopClientAttrib);
    SETUPHOOK(glPushAttrib);
    SETUPHOOK(glPushClientAttrib);
    // LEGACY
    
    SETUPHOOK(glDrawArrays);
    SETUPHOOK(glEnable);
    SETUPHOOK(glFinish);
    // DON'T HOOK - ALREADY USED IN OGLHOOK - SETUPHOOK(glFlush);
    SETUPHOOK(glGenTextures);
    SETUPHOOK(glGetBooleanv);
    SETUPHOOK(glGetError);
    SETUPHOOK(glGetIntegerv);
    SETUPHOOK(glGetString);
    SETUPHOOK(glGetTexImage);
    SETUPHOOK(glGetTexLevelParameteriv);
    SETUPHOOK(glIsTexture);
    SETUPHOOK(glTexImage2D);
    SETUPHOOK(glTexParameteri);
    SETUPHOOK(glTexSubImage2D);
    SETUPHOOK(glUseProgram);
    SETUPHOOK(glViewport);
    
    SETUPHOOK(glCreateShader);
    SETUPHOOK(glShaderSource);
    SETUPHOOK(glCompileShader);
    SETUPHOOK(glGetShaderInfoLog);
    SETUPHOOK(glCreateProgram);
    SETUPHOOK(glAttachShader);
    SETUPHOOK(glLinkProgram);
    SETUPHOOK(glGetProgramiv);
    SETUPHOOK(glGetProgramInfoLog);
    SETUPHOOK(glDetachShader);
    SETUPHOOK(glDeleteProgram);
    SETUPHOOK(glDeleteShader);
    SETUPHOOK(glValidateProgram);
    SETUPHOOK(glGetUniformLocation);
    SETUPHOOK(glUniform1f);
    SETUPHOOK(glUniform1i);
    SETUPHOOK(glUniformMatrix4fv);
    SETUPHOOK(glGetAttribLocation);
    SETUPHOOK(glBindAttribLocation);
    SETUPHOOK(glVertexAttribPointer);
    SETUPHOOK(glGetVertexAttribiv);
    SETUPHOOK(glGetVertexAttribPointerv);
    SETUPHOOK(glEnableVertexAttribArray);
    SETUPHOOK(glDisableVertexAttribArray);
    SETUPHOOK(glGenBuffers);
    SETUPHOOK(glDeleteBuffers);
    SETUPHOOK(glBufferData);
    SETUPHOOK(glBindFramebuffer);
    SETUPHOOK(glBlendEquation);
    SETUPHOOK(glBlendFuncSeparate);
    SETUPHOOK(glBlendEquationSeparate);
}

OriginIGO::OGLDump::~OGLDump()
{
    
}

void OriginIGO::OGLDump::StartUpdate()
{
    InIGOUpdate = true;
    //OriginIGO::IGOLogInfo("Starting OIG update on thread 0x%08x", GetCurrentThreadId());
}

void OriginIGO::OGLDump::EndUpdate()
{
    InIGOUpdate = false;
    
    //OriginIGO::IGOLogInfo("End IGO update");
}

#endif // MAC_OSX



