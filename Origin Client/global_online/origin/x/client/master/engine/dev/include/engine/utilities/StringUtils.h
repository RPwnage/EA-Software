///////////////////////////////////////////////////////////////////////////////
// StringUtils.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef _STRINGUTILS_H
#define _STRINGUTILS_H

#include <QString>

#include "services/plugin/PluginAPI.h"

// Forward declarations
class QStringList;

namespace Origin
{
    namespace StringUtils
    {

        /// \brief Helper function that collapses and formats a string list of locales
        ///        into a single QString.
        ///
        ///        An input string list of locales of the following format:
        ///        "en_US", "de_DE", "ru_RU"
        ///
        ///        Will result in the following string:
        ///        "enUS,deDE,ruRU"
        ORIGIN_PLUGIN_API QString formatLocaleListAsQString(const QStringList& localeList);

    } // namespace StringUtils

} // namespace Origin

#endif
