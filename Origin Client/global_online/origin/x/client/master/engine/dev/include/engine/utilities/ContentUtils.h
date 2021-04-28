///////////////////////////////////////////////////////////////////////////////
// ContentUtils.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include <QString>
#include "engine/content/ContentController.h"
#include "engine/content/Entitlement.h"
#include "engine/content/ContentTypes.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace ContentUtils
    {
        ORIGIN_PLUGIN_API QString getOfferId(const QString &contentId);

    } // namespace ContentUtils

} // namespace Origin
