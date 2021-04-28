/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CONNECTION_UTIL_H
#define BLAZE_CONNECTION_UTIL_H

/*** Include files *******************************************************************************/
#include "framework/tdf/networkaddress.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
    class ConnectUtil
    {
    public:
        static void setExternalAddr(NetworkAddress& destAddr, const InetAddress& sourceAddr, bool force = false);
    };

} // Blaze

#endif // BLAZE_CONNECTION_UTIL_H

