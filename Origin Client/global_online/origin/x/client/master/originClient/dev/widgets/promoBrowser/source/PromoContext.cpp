/////////////////////////////////////////////////////////////////////////////
// PromoContext.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "PromoContext.h"
#include "PromoBrowserViewController.h"

#include <QString>

namespace Origin
{
	namespace Client
    {
        PromoContext::PromoType PromoContext::promoTypeFromString(const QString& promoTypeString)
        {
#define RETURN_PROMOCONTEXT_PROMOTYPE_ENUM(enumString) if (#enumString == promoTypeString) { return PromoContext::enumString; }
            RETURN_PROMOCONTEXT_PROMOTYPE_ENUM(OriginStarted);
            RETURN_PROMOCONTEXT_PROMOTYPE_ENUM(GameFinished);
            RETURN_PROMOCONTEXT_PROMOTYPE_ENUM(FreeTrialExited);
            RETURN_PROMOCONTEXT_PROMOTYPE_ENUM(FreeTrialExpired);
            RETURN_PROMOCONTEXT_PROMOTYPE_ENUM(DownloadUnderway);
            RETURN_PROMOCONTEXT_PROMOTYPE_ENUM(MenuPromotions);
#undef RETURN_PROMOCONTEXT_PROMOTYPE_ENUM

            // Return NoType if we didn't get a match
            return PromoContext::NoType;
        }

        PromoContext::Scope PromoContext::scopeFromString(const QString& scopeString)
        {
#define RETURN_PROMOCONTEXT_SCOPE_ENUM(enumString) if (#enumString == scopeString) { return PromoContext::enumString; }
            RETURN_PROMOCONTEXT_SCOPE_ENUM(GameTileContextMenu);
            RETURN_PROMOCONTEXT_SCOPE_ENUM(GameHovercard);
            RETURN_PROMOCONTEXT_SCOPE_ENUM(OwnedGameDetails);
#undef RETURN_PROMOCONTEXT_SCOPE_ENUM

            // Return NoScope if we didn't get a match
            return PromoContext::NoScope;
        }

        QString PromoContext::promoTypeString(const PromoContext::PromoType promoType)
        {
#define RETURN_PROMOCONTEXT_PROMOTYPE_STRING(promoTypeEnum) if (PromoContext::promoTypeEnum == promoType) { return #promoTypeEnum; }
            RETURN_PROMOCONTEXT_PROMOTYPE_STRING(OriginStarted);
            RETURN_PROMOCONTEXT_PROMOTYPE_STRING(GameFinished);
            RETURN_PROMOCONTEXT_PROMOTYPE_STRING(FreeTrialExited);
            RETURN_PROMOCONTEXT_PROMOTYPE_STRING(FreeTrialExpired);
            RETURN_PROMOCONTEXT_PROMOTYPE_STRING(DownloadUnderway);
            RETURN_PROMOCONTEXT_PROMOTYPE_STRING(MenuPromotions);
#undef RETURN_PROMOCONTEXT_PROMOTYPE_STRING

            // Return NoType if we didn't get a match
            return "NoType";
        }

        QString PromoContext::scopeString(const PromoContext::Scope scope)
        {
#define RETURN_PROMOCONTEXT_SCOPE_STRING(scopeContext) if (PromoContext::scopeContext == scope) { return #scopeContext; }
            RETURN_PROMOCONTEXT_SCOPE_STRING(GameTileContextMenu);
            RETURN_PROMOCONTEXT_SCOPE_STRING(GameHovercard);
            RETURN_PROMOCONTEXT_SCOPE_STRING(OwnedGameDetails);
#undef RETURN_PROMOCONTEXT_SCOPE_ENUM

            // Return NoScope if we didn't get a match
            return "NoScope";
        }

        bool PromoContext::userInitiatedContext(const PromoBrowserContext& context)
        {
            // The promo browser was invoked by the user if they've selected
            // to view the "featured today" promotions (MenuPromotions) or if
            // there is any secondary context (game tile context menu,
            // hover card, etc.).
            return context.promoType == PromoContext::MenuPromotions || context.scope != PromoContext::NoScope;
        }

        PromoBrowserContext::PromoBrowserContext(const PromoContext::PromoType promoType, const PromoContext::Scope scope)
        :   promoType(promoType), 
            scope(scope)
        {
        }
    }
}
