// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "EABase/EAbase.h"

#if !defined(ORIGIN_PC) && !defined(ORIGIN_MAC)
#ifndef __APPLE__
#define ORIGIN_PC
#else
#define ORIGIN_MAC
#endif
#endif

#ifdef ORIGIN_PC

#include "targetver.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#endif

#ifdef _WINSOCKAPI_
#undef _WINSOCKAPI_ // this is redefined in Winsock2.h
#endif

#include <windows.h>
#include <Winsock2.h>
#include <Ws2tcpip.h>

#else

#include <stdlib.h>
#include <sys/time.h>

#endif

#include <OriginSDK/OriginSDK.h>
