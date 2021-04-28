//    Copyright 2011-2013, Electronic Arts
//    All rights reserved.

#ifndef NETUTIL_H
#define NETUTIL_H

#include "services/plugin/PluginAPI.h"

class QString;

namespace Origin
{
    namespace Util
    {
        namespace NetUtil
        {
            /// \brief  Returns the string representation of the CDN vendor IP address for the current download URL.
            ///
            ///         Blocking call - won't return until the IP address has been resolved.
            ///
            /// \note   The IP address returned from this function may not be the same IP address from the active
            ///         connection (if there is one) that's established to this URL. The underlying socket of that
            ///         connection must be exposed in order to obtain the actual IP address.
            ///
            /// \return String representation of URL host IP address. If an error occurs, "HostNotFound" or "UnknownError"
            //          (depending on the error). If a file-override is passed, "FileOverride" is returned. If an unknown
            ///         error type is returned from the IP address lookup, a default-constructed QString may be returned.
            ORIGIN_PLUGIN_API QString ipFromUrl(const QString& url);
        }
    }
}
#endif // NETUTIL_H
