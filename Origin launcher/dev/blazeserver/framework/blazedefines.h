/*************************************************************************************************/
/*!
    \file blazedefines.h

    Blaze header defines with minimal dependencies on other Blaze headers.

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_DEFINES_H
#define BLAZE_DEFINES_H

/*** Include files *******************************************************************************/
#include <stdio.h>
#include "EABase/eabase.h"
#include "EAAssert/eaassert.h"

#ifdef EA_PLATFORM_WINDOWS

    #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0500
    #endif


    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    
    // define max number of signaled sockets on windows
    #define FD_SETSIZE      256
    #include <WINSOCK2.H>

    #include <WS2tcpip.h>
#endif

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#define ASSERT EA_ASSERT

//MACRO for printing size_t, gcc has the c99 %z; 
//for anything else we hope that PRIuPTR is a close approximate
#ifndef PRIsize
    #if __STDC_VERSION__ >= 199901L
        #define PRIsize "zd"
    #elif defined(EA_PLATFORM_LINUX)
        #define PRIsize "zd"
    #elif defined(EA_PLATFORM_WINDOWS)
        #define PRIsize "Id"
    #else
        #define PRIsize PRIuPTR
    #endif
#endif

#ifdef EA_COMPILER_GNUC
    #define ATTRIBUTE_PRINTF_CHECK(x,y) __attribute__((format(printf,x,y)))
#else
    #define ATTRIBUTE_PRINTF_CHECK(x,y)
#endif

// Timer type for use with TimerQueue and Selector
typedef uint32_t TimerId;

const uint16_t RPC_COMPONENT_UNKNOWN = 0;
const uint16_t RPC_COMMAND_UNKNOWN = 0;

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
#define RpcIsFrameworkComponent(compId) (RpcGetComponentType(compId) == RPC_TYPE_FRAMEWORK)
#define RpcGetComponentType(compId)     (RpcComponentType)(((compId) & RPC_MASK_TYPE) >> 11)
#define RpcComponentId(compId)       ((compId) & 0x7ff)
#define RpcBuildComponentId(type,id) (((type) << 11) | (id))

typedef uint16_t CommandId;
typedef uint16_t NotificationId;
typedef uint16_t EventId;

const size_t INSTANCE_KEY_SHIFT_32 = 20;
// An instance key is an (unsigned) 64bit value.  The 12 most significant bits are the InstanceId. The remaining
// 52 bits are the 'base', typically an auto incremented unique number.
const size_t INSTANCE_KEY_SHIFT_64 = 52;
const uint64_t INSTANCE_KEY_MAX_KEY_BASE_64 = (1LL << INSTANCE_KEY_SHIFT_64) - 1;

typedef uint16_t InstanceId;
const InstanceId INVALID_INSTANCE_ID = 0;
const InstanceId INSTANCE_ID_MAX = (1 << (64 - INSTANCE_KEY_SHIFT_64)) - 1; // instance id space is 12 bits, last valid instance id is 4095

#define BuildInstanceKey32(_instanceId, _key) ((((uint32_t) _instanceId) << INSTANCE_KEY_SHIFT_32) | _key)
#define GetInstanceIdFromInstanceKey32(_key) static_cast<InstanceId>(((uint32_t) _key) >> INSTANCE_KEY_SHIFT_32)
#define BuildInstanceKey64(_instanceId, _key) ((((uint64_t) _instanceId) << INSTANCE_KEY_SHIFT_64) | _key)
#define GetInstanceIdFromInstanceKey64(_key) static_cast<InstanceId>(((uint64_t) _key) >> INSTANCE_KEY_SHIFT_64)
#define GetBaseFromInstanceKey64(_key) (_key & INSTANCE_KEY_MAX_KEY_BASE_64)

// A SliverKey is an (unsigned) 64bit value.  It is a combination of a unique id, and a SliverId.
// For readability in logs, and while debugging, the SliverId is the 5 least significant *digits*.
typedef uint64_t SliverKey;
typedef uint16_t SliverId;

const SliverId INVALID_SLIVER_ID = 0;
const SliverId SLIVER_ID_MIN = 1;
const SliverId SLIVER_ID_MAX = 9999;

const SliverKey SLIVER_KEY_BASE_OFFSET = (SLIVER_ID_MAX + 1);

const SliverKey SLIVER_KEY_BASE_MIN = 1;
const SliverKey SLIVER_KEY_BASE_MAX = (UINT64_MAX - SLIVER_KEY_BASE_OFFSET);

#define BuildSliverKey(_sliverId, _base) static_cast<SliverKey>(((SliverKey)(_base) * SLIVER_KEY_BASE_OFFSET) + (_sliverId))
#define GetSliverIdFromSliverKey(_key) static_cast<SliverId>((_key) - (GetBaseFromSliverKey(_key) * SLIVER_KEY_BASE_OFFSET))
#define GetBaseFromSliverKey(_key) static_cast<SliverKey>((_key) / SLIVER_KEY_BASE_OFFSET)

typedef uint32_t SliverIdentity;
#define MakeSliverIdentity(_sliverNamespace, _sliverId) ((SliverIdentity)(((_sliverNamespace) << 16) | (_sliverId)))
#define GetSliverNamespaceFromSliverIdentity(_sliverIdentity) ((_sliverIdentity) >> 16)
#define GetSliverIdFromSliverIdentity(_sliverIdentity) ((SliverId)((_sliverIdentity) & ((1LL << 16) - 1)))
const SliverIdentity INVALID_SLIVER_IDENTITY = MakeSliverIdentity(RPC_COMPONENT_UNKNOWN, INVALID_SLIVER_ID);

#define BUFFER_SIZE_32          32
#define BUFFER_SIZE_64          64
#define BUFFER_SIZE_128         128
#define BUFFER_SIZE_256         256
#define BUFFER_SIZE_512         512
#define BUFFER_SIZE_1K          1024
#define BUFFER_SIZE_2K          2048
#define BUFFER_SIZE_4K          4096
#define BUFFER_SIZE_8K          8192
#define BUFFER_SIZE_16K         16384
#define BUFFER_SIZE_32K         32768
#define BUFFER_SIZE_64K         65536
#define BUFFER_SIZE_128K        131072
#define BUFFER_SIZE_256K        262144
#define BUFFER_SIZE_1024K       1048576


const int32_t ERR_OK = 0;

// Get rid of compiler warnings on warning level 4
#ifdef EA_PLATFORM_WINDOWS
#define UNREFERENCED_PARAMETER(P) (P)
#else
#define UNREFERENCED_PARAMETER(P)
#endif

// The following two methods are intentionally private and unimplemented to prevent passing
// and returning objects by value.  The compiler will give a linking error if either is used.
#define NON_COPYABLE(_class_) private:\
    /* Copy constructor */ \
    _class_(const _class_& otherObj);\
    /* Assignment operator */ \
    _class_& operator=(const _class_& otherObj);

// Sleep macro - sleep for the given number of milliseconds
#ifdef EA_PLATFORM_WINDOWS
#define doSleep(ms) Sleep(ms)
#else
#define doSleep(ms) usleep(ms * 1000);
#endif

#define SAFE_DELETE_REF(obj) /*lint -save -e424*/ delete &obj; /*lint -restore*/

//  inline function to convert an 8-bit value to its high and low hex ascii chars.
inline void blaze_char2hex(char8_t &h, char8_t &l, char8_t ch)
{
    static const char8_t* HEX = "0123456789abcdef";
    h = HEX[((uint8_t)ch) >> 4];
    l = HEX[((uint8_t)ch) & 0xf];
}

//These put the file and line together in a single constant string
#define DEFINETOSTRING_(x) #x
#define DEFINETOSTRING(x) DEFINETOSTRING_(x)  
#define __FILEANDLINE__ __FILE__ ":" DEFINETOSTRING(__LINE__)

//A few lint inclusions/exclusions we need
//ones that probably can't be fixed
//lint -e537 Bulkbuild needs these off since include files will naturally show up multiple times
//lint -e1061 Lint has problems with our inheritance scheme for pulling in eastl vectors.
//lint -e1023 Lint can't seem to figure out the visit functions
//lint -e1049 Lint has problems with dispatch/functor calls, and complains about too many templates
//lint -e1024 Lint has problems with dispatch/functor calls, complains about no template matching
//lint -e64 Lint can't figure out the types from dispatch/functor calls
//lint -e119 Lint can't figure out the types from dispatch/functor calls
//lint -e1025 Lint can't figure out the types from dispatch/functor calls
//lint -e1703 Lint can't figure out the types from dispatch/functor/schedule calls
//lint -e1551 Exception handling message (most destructors)
//lint -e788 symbols not used in a switch case with a default. (We have the one where its not caught with a default)
//lint -e1702 Operator== or * is both an ordinary function and a member function.  Change of coding standard required to fix this.
//lint -e1774 We don't worry about dynamic casts.
//lint -e1705 Don't care about using static scope operator - sometimes provides more info anyways
//lint -e1704 We use private constructors
//lint -e1712 We have a lot of places that don't have default constructors
//lint -e1725 We allow class members to be references

//ones we want to specialize
//lint -e641 Enums to integers (logger)
//lint -e534 General return value checking (seems to be printf)
//lint -e737 Loss of sign in promotion from int to unsigned int
//lint -e747 Significatn prototype coercion
//lint -e713 loss of precision
//lint -e732 loss of sign
//lint -e740 Pointer cast
//lint -e1411 Can't figure out visitor functions
//lint -e701 -e702 -e703 -e704 signed shift warning
//lint -e1514  Creating a temporary copy of a param
//lint -e830
//lint -e831


//ones we want to fix
//lint -e715      Unused symbols
//lint -e1540     Pointers not zero'd or freed in destructor
//lint -e1740     Pointers not zero'd or freed in destructor
//lint -e429      Custodial pointer warnings

//Stopgap measures to get all the nullptr pointer warnings
//lint -w0  Turn everything off
//int fpn Assume all pointers passed to functions are nullptr
//lint +e413 +e613 +e794 +e418  nullptr related errors
//lint +e1746 parameter 'Symbol' of function 'Symbol' could be made const reference
//lint +e1762 Member function 'Symbol' could be made const
//lint +e1961 virtual member function 'Symbol' could be made const

#define DEFAULT_BLAZE_ALLOCATOR *::Blaze::Allocator::getAllocator(DEFAULT_BLAZE_MEMGROUP)
#define EA_TDF_GET_DEFAULT_ICOREALLOCATOR *::Blaze::Allocator::getAllocator(DEFAULT_BLAZE_MEMGROUP)
#define EA_TDF_GET_DEFAULT_ALLOCATOR_PTR *::Blaze::Allocator::getAllocator(DEFAULT_BLAZE_MEMGROUP)

#ifndef BLAZE_ALLOC_NO_FILE_AND_LINE
#define EA_TDF_DEFAULT_ALLOC_NAME __FILEANDLINE__
#endif

#include "EASTL/hash_map.h"
#include "EASTL/hash_set.h"
#include "EASTL/list.h"
#include "EASTL/string.h"
#include "EASTL/fixed_string.h"

namespace Blaze
{
    // typedefs for allocation efficient fixed string that automatically overflow to BlazeStlAllocator
    typedef eastl::fixed_string<char8_t, 8> FixedString8;
    typedef eastl::fixed_string<char8_t, 16> FixedString16;
    typedef eastl::fixed_string<char8_t, 32> FixedString32;
    typedef eastl::fixed_string<char8_t, 64> FixedString64;
    typedef eastl::fixed_string<char8_t, 128> FixedString128;
    typedef eastl::fixed_string<char8_t, 256> FixedString256;
}

#endif // BLAZE_DEFINES_H
