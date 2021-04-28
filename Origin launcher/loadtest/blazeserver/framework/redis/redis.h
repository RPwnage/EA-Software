/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_H
#define BLAZE_REDIS_H

/*** Include files *******************************************************************************/

#include "blazerpcerrors.h"
#include "framework/system/fiber.h"
#include "framework/system/frameworkresource.h"

#include "framework/redis/rediscluster.h"
#include "framework/redis/rediscollection.h"
#include "framework/redis/rediscollectionmanager.h"
#include "framework/redis/rediscommand.h"
#include "framework/redis/redisconn.h"
#include "framework/redis/redisdecoder.h"
#include "framework/redis/rediserrors.h"
#include "framework/redis/redisid.h"
#include "framework/redis/rediskeylist.h"
#include "framework/redis/rediskeyset.h"
#include "framework/redis/redislockmanager.h"
#include "framework/redis/redisresponse.h"
#include "framework/redis/redisscriptregistry.h"

#include "EASTL/intrusive_list.h"
#include "EASTL/list.h"
#include "hiredis/hiredis.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

} // Blaze

#endif // BLAZE_REDIS_H
