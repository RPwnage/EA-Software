/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_ENDPOINT_HELPER_H
#define BLAZE_ENDPOINT_HELPER_H

/*** Include files *******************************************************************************/

#include "EATDF/time.h"
#include "framework/tdf/network.h"
#include "framework/connection/inetaddress.h"

namespace Blaze
{
class InetAddress;

InetAddress getInetAddress(Blaze::BindType bindType, uint16_t port);

} // Blaze

#endif // BLAZE_ENDPOINT_HELPER_H
