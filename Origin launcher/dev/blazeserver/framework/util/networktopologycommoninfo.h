/*! ************************************************************************************************/
/*!
\file   networktopologycommoninfo.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#ifndef BLAZE_FRAMEWORK_UTIL_COMMON_INFO_H
#define BLAZE_FRAMEWORK_UTIL_COMMON_INFO_H

namespace Blaze
{
    inline bool isDedicatedHostedTopology(GameNetworkTopology networkTopology)
    {
        if (networkTopology == CLIENT_SERVER_DEDICATED)
        {
            return true;
        }
        return false;
    }

    inline bool isPlayerHostedTopology(GameNetworkTopology networkTopology)
    {
        if (!isDedicatedHostedTopology(networkTopology) && (networkTopology != NETWORK_DISABLED))
        {
            return true;
        }
        return false;
    }

} // namespace Blaze

#endif //BLAZE_FRAMEWORK_UTIL_COMMON_INFO_H
