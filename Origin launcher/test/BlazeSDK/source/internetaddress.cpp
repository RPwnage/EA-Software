/*************************************************************************************************/
/*!
    \file internetaddress.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/internetaddress.h"
#include "BlazeSDK/shared/framework/util/blazestring.h"

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtynet.h"

namespace Blaze
{
  
    void InternetAddress::setIpAndPort(const char *ip, uint16_t port)
    {
        mIP = (uint32_t)SocketInTextGetAddr(ip);
        mPort = port;
    }

    void InternetAddress::setIpAndPort(uint32_t ip, uint16_t port)
    {
        mIP = ip;
        mPort = port;
    }


    char* InternetAddress::asString( char *buf, size_t len ) const
    {
        BlazeAssert( len >= 16 );

        blaze_snzprintf( buf,len,
                         "%hu.%hu.%hu.%hu",
                         static_cast<uint8_t>( mIP >> 24 ),
                         static_cast<uint8_t>( mIP >> 16 ),
                         static_cast<uint8_t>( mIP >>  8 ),
                         static_cast<uint8_t>( mIP >>  0 ) );
        return buf;
    }

}// namespace Blaze
