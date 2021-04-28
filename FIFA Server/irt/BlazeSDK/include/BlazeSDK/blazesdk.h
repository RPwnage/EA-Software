/*! ***********************************************************************************************/
/*!
    \file blazesdk.h

    Common include for blaze public files.


    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

#ifndef BLAZESDK_H
#define BLAZESDK_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

///////////////////////////////////////////////////////////////////////////////
// BLAZESDK_USER_CONFIG_HEADER
//
// This allows the user to define a header file to be #included before the 
// BlazeSDK blazesdk.h contents are compiled. A primary use of this is to override
// the contents of this blazesdk.h file. Note that all the settings below in 
// this file are user-overridable.
// 
// Example usage:
//     #define BLAZESDK_USER_CONFIG_HEADER "MyConfigOverrides.h"
//     #include <BlazeSDK/gamemanager/gamemanagerapi.h>
//
///////////////////////////////////////////////////////////////////////////////
#ifdef BLAZESDK_USER_CONFIG_HEADER
    #include BLAZESDK_USER_CONFIG_HEADER
#endif

/******* Include files ****************************************************************************/
#ifndef INCLUDED_eabase_H
#include "EABase/eabase.h"
#endif

/******* Defines/Macros/Constants/Typedefs ********************************************************/

#define BLAZE_CLIENT_SDK 1

// By default, enable the use of telemetry SDK, unless otherwise defined
#ifndef USE_TELEMETRY_SDK
    #define USE_TELEMETRY_SDK 1
#endif

//C-style calling convention definition.
#if defined EA_PLATFORM_WINDOWS
#define BLAZESDK_CC __cdecl
#else
#define BLAZESDK_CC 
#endif

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_NX) || defined(EA_PLATFORM_STADIA)
#define BLAZESDK_CONSOLE_PLATFORM
#endif

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5)
#define BLAZESDK_TRIAL_SUPPORT
#endif

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5)
#define BLAZE_USER_PROFILE_SUPPORT
#endif

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
#define BLAZE_EXTERNAL_SESSIONS_SUPPORT
#endif

#ifdef EA_COMPILER_GNUC
#define ATTRIBUTE_PRINTF_CHECK(x,y) __attribute__((format(printf,x,y)))
#else
#define ATTRIBUTE_PRINTF_CHECK(x,y)
#endif

#ifdef EA_DEBUG

//This flag allows "pretty" logging of TDF field names.  Turning it on will compile in TDF field names
//and allow them to be printed out during logging.  It is currently enabled for default only.
#define BLAZE_ENABLE_TDF_TAG_INFO 1

//If the following is defined, only classes with the "reflect=true" attribute will generate tag info.  Use this if there
//is a specific use case you need where only a few classes need tag info.
//#define BLAZE_ENABLE_TDF_SELECTIVE_TAG_INFO 1

//Uncomment this line if you'd like to log descriptions and defaults along with tag names.
//#define BLAZE_ENABLE_EXTENDED_TDF_TAG_INFO 1
#endif

#define NON_COPYABLE(_class_) private:\
    /* Copy constructor */ \
    _class_(const _class_& otherObj);\
    /* Assignment operator */ \
    _class_& operator=(const _class_& otherObj);

#define BLAZE_ENABLE_TDF_CHANGE_TRACKING 1

#include "BlazeSDK/alloc.h"

#include "EATDF/tdfobjectid.h"
#include "EATDF/time.h"
#include "EATDF/tdfbase.h"

//These are in common enough use to justify keeping them.
namespace Blaze
{
    typedef EA::TDF::EntityId BlazeId;
    using EA::TDF::ComponentId;
    using EA::TDF::EntityType;
    using EA::TDF::EntityId;
    using EA::TDF::TimeValue;    
}

#endif // BLAZESDK_H
