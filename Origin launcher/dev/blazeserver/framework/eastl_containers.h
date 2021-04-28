/*************************************************************************************************/
/*!
    \file
         eastl_containers.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_EASTL_CONTAINERS_H
#define BLAZE_EASTL_CONTAINERS_H

/*** Include files *******************************************************************************/

#ifndef BLAZE_CLIENT_SDK
#include "EASTL/allocator.h"
#include "EASTL/hash_map.h"
#include "EASTL/hash_set.h"
#include "EASTL/list.h"
#include "EASTL/map.h"
#include "EASTL/slist.h"
#include "EASTL/string.h"
#include "EASTL/vector.h"
#include "EASTL/vector_map.h"
#include "EASTL/vector_set.h"
#else
#include "BlazeSDK/blaze_eastl/blazeallocator.h"
#include "BlazeSDK/blaze_eastl/hash_map.h"
#include "BlazeSDK/blaze_eastl/hash_set.h"
#include "BlazeSDK/blaze_eastl/list.h"
#include "BlazeSDK/blaze_eastl/map.h"
#include "BlazeSDK/blaze_eastl/slist.h"
#include "BlazeSDK/blaze_eastl/string.h"
#include "BlazeSDK/blaze_eastl/vector.h"
#include "BlazeSDK/blaze_eastl/vector_map.h"
#include "BlazeSDK/blaze_eastl/vector_set.h"
#endif

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#ifndef BLAZE_CLIENT_SDK
    #define BLAZE_EASTL_HASH_MAP eastl::hash_map
    #define BLAZE_EASTL_HASH_SET eastl::hash_set
    #define BLAZE_EASTL_LIST eastl::list
    #define BLAZE_EASTL_MAP eastl::map
    #define BLAZE_EASTL_SLIST eastl::slist
    #define BLAZE_EASTL_STRING eastl::string
    #define BLAZE_EASTL_VECTOR eastl::vector
    #define BLAZE_EASTL_VECTOR_MAP eastl::vector_map
    #define BLAZE_EASTL_VECTOR_SET eastl::vector_set
    #define BLAZE_SHARED_EASTL_VAR(name, sdkMemGroupId, sdkDebugMemName) name
#else
    #define BLAZE_EASTL_HASH_MAP Blaze::hash_map
    #define BLAZE_EASTL_HASH_SET Blaze::hash_set
    #define BLAZE_EASTL_LIST Blaze::list
    #define BLAZE_EASTL_MAP Blaze::map
    #define BLAZE_EASTL_SLIST Blaze::slist
    #define BLAZE_EASTL_STRING Blaze::string
    #define BLAZE_EASTL_VECTOR Blaze::vector
    #define BLAZE_EASTL_VECTOR_MAP Blaze::vector_map
    #define BLAZE_EASTL_VECTOR_SET Blaze::vector_set
    #define BLAZE_SHARED_EASTL_VAR(name, sdkMemGroupId, sdkDebugMemName) name(sdkMemGroupId, sdkDebugMemName)
#endif

#endif
