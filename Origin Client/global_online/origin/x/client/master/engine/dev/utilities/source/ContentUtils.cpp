///////////////////////////////////////////////////////////////////////////////
// ContentUtils.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "engine/utilities/ContentUtils.h"

#include "engine/content/ContentController.h"
#include "engine/content/Entitlement.h"
#include "engine/content/ContentTypes.h"
#include "engine/content/DiPManifest.h"

#include "services/log/LogService.h"

namespace Origin
{
    namespace ContentUtils
    {

        QString getOfferId(const QString &contentId)
        {
            Origin::Engine::Content::EntitlementRef e;
            Origin::Engine::Content::ContentController *cc = Origin::Engine::Content::ContentController::currentUserContentController();
            if(cc != NULL)
                e = cc->entitlementById(contentId);
            else
                // if ContentController does not exist (user hasn't logged in yet), return NULL string
                return QString();

            if (e)
            {
                Origin::Engine::Content::ContentConfigurationRef c = e->contentConfiguration();
                if (c)
                {
                    QString offerId = c->productId();
                    return offerId;
                }
            }
            //if content id isn't found, return empty string
            return QString();
        }

    } // namespace ContentUtils

} // namespace Origin
