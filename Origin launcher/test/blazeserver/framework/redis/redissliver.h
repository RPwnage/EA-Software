/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_SLIVER_H
#define BLAZE_REDIS_SLIVER_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


namespace Blaze
{

typedef uint16_t RedisSliverId;

struct RedisSliver
{
    RedisSliver() : sliverId(0), isTagged(false) {}
    RedisSliver(RedisSliverId id, bool tagged) : sliverId(id), isTagged(tagged) {}

    RedisSliverId sliverId;
    bool isTagged;
};

} // Blaze

#endif // BLAZE_REDIS_SLIVER_H

