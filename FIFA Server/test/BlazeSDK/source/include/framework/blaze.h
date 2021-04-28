#ifndef BLAZE_INT_H
#define BLAZE_INT_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#define BLAZE_CLIENT_SDK 1

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/debug.h"

#include <stdio.h>
#include <stdarg.h>

#ifdef assert
#undef assert
#endif
#define assert BlazeAssert

#ifndef INCLUDED_eabase_H
#include "EABase/eabase.h"
#endif

//MACRO for printing size_t, gcc has the c99 %z; 
//for anything else we hope that PRIuPTR is a close approximate
#ifndef PRIsize
    #if __STDC_VERSION__ >= 199901L
        #define PRIsize "zu"
    #elif defined(EA_PLATFORM_LINUX)
        #define PRIsize "zu"
    #elif defined(EA_PLATFORM_WINDOWS)
        #define PRIsize "Iu"
    #else
        #define PRIsize PRIuPTR
    #endif
#endif

//  TODO: make common include for client and server!
const uint16_t RPC_COMPONENT_UNKNOWN = 0;

const uint16_t RPC_MASK_MASTER       = 0x8000;
const uint16_t RPC_MASK_TYPE         = 0x7800;
const uint16_t RPC_MAX_COMPONENT_ID  = 0x7ff;

typedef enum
{
    RPC_TYPE_COMPONENT_CORE   = 0,  // Reserved for core component offerings of blaze
    RPC_TYPE_COMPONENT_CUST   = 1,  // Reserved for custom game team components

    RPC_TYPE_FRAMEWORK        = 15, // Reserved for framework components
} RpcComponentType;

// Convert a slave Id to the corresponding Master ID
#define RpcMakeMasterId(compId)     ((compId) | RPC_MASK_MASTER)
#define RpcMakeSlaveId(compId)     ((compId) & ~RPC_MASK_MASTER)

#define RpcIsMasterComponent(compId) (((compId) & RPC_MASK_MASTER) != 0)
#define RpcComponentType(compId)     (RpcComponentType)(((compId) & RPC_MASK_TYPE) >> 11)
#define RpcComponentId(compId)       ((compId) & 0x7ff)
#define RpcBuildComponentId(type,id) (((type) << 11) | (id))

#include "BlazeSDK/alloc.h"

//  macro to convert an 8-bit value to its high and low hex ascii chars.
inline void blaze_char2hex(char8_t &h, char8_t &l, uint8_t ch)
{
    static const char8_t* HEX = "0123456789abcdef";
    h = HEX[((uint8_t)ch) >> 4];
    l = HEX[((uint8_t)ch) & 0xf];
}

//A few lint inclusions/exclusions we need
//lint -e1023 Lint can't seem to figure out the visit functions
//lint -e1049 Lint has problems with dispatch/functor calls, and complains about too many templates
//lint -e1024 Lint has problems with dispatch/functor calls, complains about no template matching
//lint -e64 Lint can't figure out the types from dispatch/functor calls
//lint -e119 Lint can't figure out the types from dispatch/functor calls
//lint -e1025 Lint can't figure out the types from dispatch/functor calls
//lint -e1061 Lint has problems with our inheritance scheme for pulling in eastl vectors.
//lint +e527 Turn on "unreachable code" warnings for SN compatibility
//lint -ftg Turn off trigraphs.  Our code should not use them so any instance is probably a mistake
//lint +e739 Turn on trigraphs in string literal warnings.

#endif

