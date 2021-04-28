// ***************************************************************
//  Tracing.h                 version: 1.0.0  ·  date: 2010-01-10
//  -------------------------------------------------------------
//  defines for debug tracing output
//  -------------------------------------------------------------
//  Copyright (C) 1999 - 2010 www.madshi.net, All Rights Reserved
// ***************************************************************

#pragma warning(disable:4548)  // warning C4548: expression before comma has no effect; expected expression with side-effect

#ifndef _TRACING_H
#define _TRACING_H


void TraceImpl(const char* file, int line, const wchar_t* fmt, ...);
void TraceDebugImpl(const char* file, int line, const wchar_t* fmt, ...);
void TraceVerboseImpl(const char* file, int line, const wchar_t* fmt, ...);

#define Trace(fmt, ...)                     TraceImpl(__FILE__, __LINE__, fmt, __VA_ARGS__)
#define TraceVerbose(fmt, ...)              TraceVerboseImpl(__FILE__, __LINE__, fmt, __VA_ARGS__)
#define DebugTrace(content)                 DebugTraceIntermediate content
#define DebugTraceIntermediate(fmt, ...)    TraceDebugImpl(__FILE__, __LINE__, fmt, __VA_ARGS__)


   //// DebugTrace macro requires double parentheses 
   //// DebugTrace( (Format, ...) )
   //#ifdef _DEBUG
   //   #define DebugTrace( _x_ )  Trace _x_ 
   //#else
   //   #define DebugTrace( _x_ )
   //#endif

#endif