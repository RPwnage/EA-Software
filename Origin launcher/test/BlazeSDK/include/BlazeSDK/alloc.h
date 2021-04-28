/*! ***************************************************************************/
/*!
    \file alloc.h
    
    Header file for allocation routines.

    \attention
    (c) Electronic Arts. All Rights Reserved.

*****************************************************************************/

#ifndef BLAZE_ALLOCATOR_H
#define BLAZE_ALLOCATOR_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#ifndef INCLUDED_eabase_H
#include "EABase/eabase.h"
#endif

///////////////////////////////////////////////////////////////////////////////
// BLAZESDK_DLL
//
// Defined as 0 or 1. The default is dependent on the definition of EA_DLL.
// If EA_DLL is defined, then BLAZESDK_DLL is 1, else BLAZESDK_DLL is 0.
// EA_DLL is a define that controls DLL builds within the EAConfig build system. 
// BLAZESDK_DLL controls whether BLAZESDK is built and used as a DLL. 
//
#ifndef BLAZESDK_DLL
    #if defined(EA_DLL)
        #define BLAZESDK_DLL 1
    #endif
#endif

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. BLAZESDK_API symbol is defined in blazesdk script as dllexport.
// This symbol should not be defined on any project that uses this DLL. This way any other project 
// whose source files include this file see BLAZESDK_API functions as being imported from a DLL, 
// whereas this DLL sees symbols defined with this macro as being exported.
#if BLAZESDK_DLL
  #ifndef BLAZESDK_API
    #if defined(EA_PLATFORM_MICROSOFT)
      #define BLAZESDK_API __declspec(dllimport)
    #else
      #define BLAZESDK_API __attribute__ ((visibility("default")))
    #endif
  #endif
//BLAZESDK_LOCAL is a keyword reserved to mark objects as non-exportable
#  define BLAZESDK_LOCAL
#else
#  define BLAZESDK_API 
//BLAZESDK_LOCAL is a keyword reserved to mark objects as non-exportable
#  define BLAZESDK_LOCAL
#endif

namespace EA
{
    namespace Allocator
    {
        class ICoreAllocator;
    }
}


namespace Blaze
{

//
// Types
//
struct MemoryGroupId
{
    MemoryGroupId() : id(0) {}
    MemoryGroupId(uint8_t _id) : id(_id) {}

    MemoryGroupId& operator=(uint8_t _id) { id = _id; return *this; }
    uint8_t operator|(uint8_t operand) const { return (uint8_t) (id | operand); }


    operator uint8_t() const { return id; }
    operator EA::Allocator::ICoreAllocator*() const;
    operator EA::Allocator::ICoreAllocator&() const;

    uint8_t id;
};

//
// Const
//
const int32_t MEM_ID_DIRTY_SOCK = 0x6fffffff;

#ifdef EA_DEBUG
const int32_t MAX_MEM_GROUP_NAME_LEN = 64;
const int32_t MAX_USER_DEBUG_STRING_LEN = 512;
#endif

const MemoryGroupId MEM_GROUP_TEMP_FLAG = 0x80;    // To be ORed with mem group when mem block needs to go to temp area of heap.

const MemoryGroupId MEM_GROUP_DEFAULT               = 0;
const MemoryGroupId MEM_GROUP_DEFAULT_TEMP          = (MEM_GROUP_DEFAULT | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_FRAMEWORK             = 1;
const MemoryGroupId MEM_GROUP_FRAMEWORK_RAWBUF      = MEM_GROUP_FRAMEWORK;
const MemoryGroupId MEM_GROUP_FRAMEWORK_HTTP        = MEM_GROUP_FRAMEWORK;
const MemoryGroupId MEM_GROUP_FRAMEWORK_DEFAULT     = MEM_GROUP_FRAMEWORK;
const MemoryGroupId MEM_GROUP_FRAMEWORK_TEMP        = (MEM_GROUP_FRAMEWORK | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_GAMEMANAGER           = 2;
const MemoryGroupId MEM_GROUP_GAMEMANAGER_TEMP      = (MEM_GROUP_GAMEMANAGER | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_GAMEBROWSER           = 3;
const MemoryGroupId MEM_GROUP_GAMEBROWSER_TEMP      = (MEM_GROUP_GAMEBROWSER | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_STATS                 = 4;
const MemoryGroupId MEM_GROUP_STATS_TEMP            = (MEM_GROUP_STATS | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_LEADERBOARD           = 5;
const MemoryGroupId MEM_GROUP_LEADERBOARD_TEMP      = (MEM_GROUP_LEADERBOARD | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_LOGINMANAGER          = 6;
const MemoryGroupId MEM_GROUP_LOGINMANAGER_TEMP     = (MEM_GROUP_LOGINMANAGER | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_MESSAGING             = 7;
const MemoryGroupId MEM_GROUP_MESSAGING_TEMP        = (MEM_GROUP_MESSAGING | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_MAIL                  = 8;
const MemoryGroupId MEM_GROUP_MAIL_TEMP             = (MEM_GROUP_MAIL | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_NETWORKADAPTER        = 9;                                                // in case you want to use BLAZE_NEW in title code
const MemoryGroupId MEM_GROUP_NETWORKADAPTER_TEMP   = (MEM_GROUP_NETWORKADAPTER | MEM_GROUP_TEMP_FLAG);  // in case you want to use BLAZE_NEW in title code
const MemoryGroupId MEM_GROUP_CENSUSDATA            = 10;
const MemoryGroupId MEM_GROUP_CENSUSDATA_TEMP       = (MEM_GROUP_CENSUSDATA | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_ASSOCIATIONLIST       = 11;
const MemoryGroupId MEM_GROUP_ASSOCIATIONLIST_TEMP  = (MEM_GROUP_ASSOCIATIONLIST | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_UTIL                  = 12;
const MemoryGroupId MEM_GROUP_UTIL_TEMP             = (MEM_GROUP_UTIL | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_TELEMETRY             = 13;
const MemoryGroupId MEM_GROUP_TELEMETRY_TEMP        = (MEM_GROUP_TELEMETRY | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_TITLE                 = 14;
const MemoryGroupId MEM_GROUP_TITLE_TEMP            = (MEM_GROUP_TITLE | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_TITLE_2               = 15;
const MemoryGroupId MEM_GROUP_TITLE_2_TEMP          = (MEM_GROUP_TITLE_2 | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_TITLE_3               = 16;
const MemoryGroupId MEM_GROUP_TITLE_3_TEMP          = (MEM_GROUP_TITLE_3 | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_TITLE_4               = 17;
const MemoryGroupId MEM_GROUP_TITLE_4_TEMP          = (MEM_GROUP_TITLE_4 | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_TITLE_5               = 18;
const MemoryGroupId MEM_GROUP_TITLE_5_TEMP          = (MEM_GROUP_TITLE_5 | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_TITLE_6               = 19;
const MemoryGroupId MEM_GROUP_TITLE_6_TEMP          = (MEM_GROUP_TITLE_6 | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_TITLE_7               = 20;
const MemoryGroupId MEM_GROUP_TITLE_7_TEMP          = (MEM_GROUP_TITLE_7 | MEM_GROUP_TEMP_FLAG);
const MemoryGroupId MEM_GROUP_TITLE_8               = 21;
const MemoryGroupId MEM_GROUP_TITLE_8_TEMP          = (MEM_GROUP_TITLE_8 | MEM_GROUP_TEMP_FLAG);

const uint8_t MEM_GROUP_MAX                         = 22;

//in case we don't have a default, use this.
#ifndef DEFAULT_BLAZE_MEMGROUP
#define DEFAULT_BLAZE_MEMGROUP Blaze::MEM_GROUP_DEFAULT
#endif

#define BLAZE_NEW_MGID BLAZE_NEW
#define BLAZE_ALLOC_MGID BLAZE_ALLOC

#define BLAZE_PUSH_ALLOC_OVERRIDE(_memgroupid_, _name_)
#define BLAZE_POP_ALLOC_OVERRIDE()

/*! ****************************************************************************/
/*!
    \brief Provides memory management functions for Blaze SDK classes.

    This is the common allocator used by all classes within the BlazeSDK.  No 
    allocation should pass outside this class. From a user's perspective, the only 
    function of interest in this class is "setAllocator" which ties the SDK's
    allocation functions into the game's allocators.  "setAllocator" should be
    called previous to any other SDK call - if not then the allocations will use
    system allocators.
********************************************************************************/

class BLAZESDK_API Allocator
{
public:
    static void setAllocator(EA::Allocator::ICoreAllocator *ator);
    static void setAllocator(MemoryGroupId memGroupId, EA::Allocator::ICoreAllocator *ator);

    static void clearAllocator(MemoryGroupId memGroupId);
    static void clearAllAllocators();

    static EA::Allocator::ICoreAllocator* getAllocator() { return getAllocator(MEM_GROUP_DEFAULT); }
    static EA::Allocator::ICoreAllocator* getAllocator(MemoryGroupId memGroupId);
    static MemoryGroupId getMemGroupId(EA::Allocator::ICoreAllocator* ator);
#ifdef EA_DEBUG
    static const char8_t* buildDebugNameString(MemoryGroupId memGroupId, const char8_t*);
#endif

private:
    static EA::Allocator::ICoreAllocator* msAllocator[MEM_GROUP_MAX];
};

inline MemoryGroupId::operator EA::Allocator::ICoreAllocator*() const { return Allocator::getAllocator(*this); }
inline MemoryGroupId::operator EA::Allocator::ICoreAllocator&() const { return *Allocator::getAllocator(*this); }

} // namespace Blaze

#endif // BLAZE_ALLOCATOR_H
