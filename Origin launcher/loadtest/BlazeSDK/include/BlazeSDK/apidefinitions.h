/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_API_DEFINITIONS_H
#define BLAZE_API_DEFINITIONS_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"

namespace Blaze
{
    typedef uint16_t APIId;

    //This API outlines the defined API ids.  New Blaze APIs should be added to the 
    enum APIIdDefinitions
    {
        NULL_API            = 0x0000,
        GAMEMANAGER_API     = 0x0001,
        STATS_API           = 0x0002,
        LEADERBOARD_API     = 0x0003,
        MESSAGING_API       = 0x0005,
        CENSUSDATA_API      = 0x0006,
        UTIL_API            = 0x0008,
        TELEMETRY_API       = 0x0009,
        ASSOCIATIONLIST_API = 0x000A,

        /* Add new Blaze APIs here - if the value exceeds that of CUSTOM_API_FLAG, 
           then the value needs to be increased.*/

        FIRST_CUSTOM_API    = 0x0010, //< New APIs added by the game team should start with this id.

        MAX_API_ID          = 0x0020 //< No id should be higher than this value. 
    };
};
#endif // BLAZE_API_DEFINITIONS_H
