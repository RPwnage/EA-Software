/*! *************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

/*! ****************************************************************************/
/*! \class Idler

    \brief Interface for objects that need to be idled.
********************************************************************************/

#ifndef IDLER_H
#define IDLER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"

namespace Blaze
{
    class BLAZESDK_API Idler
    {
    public:

        /*! ****************************************************************************/
        /*! \brief Function called once per frame to update the object.

            \param currentTime The tick count of the system in milliseconds.
            \param elapsedTime The elapsed count of milliseconds since the last idle call.
        ********************************************************************************/
        virtual void idle(const uint32_t currentTime, const uint32_t elapsedTime) = 0;
        
        Idler() : mIdlerName("unknownIdler") {}
        virtual ~Idler() {}

        void idleWrapper(const uint32_t currentTime, const uint32_t elapsedTime)
        {
#if ENABLE_BLAZE_SDK_PROFILING
            ScopeTimer timer(mIdlerName);
#endif
            idle(currentTime, elapsedTime);
        }
        const char* mIdlerName;
    };
};
#endif // IDLER_H
