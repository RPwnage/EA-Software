/////////////////////////////////////////////////////////////////////////////
// PromoContext.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef PROMO_CONTEXT_H
#define PROMO_CONTEXT_H

#include "services/plugin/PluginAPI.h"

class QString;

namespace Origin
{
    namespace Client
    {
        struct PromoBrowserContext;

        namespace PromoContext
        {
            /// \brief Types of promo windows that are shown. Used for
            /// recording when we last showed a promo given the type. Also for
            /// setting raw headers in network request.
            ///
            /// OriginStarted = 'on successful login'
            /// GameFinished  = 'user quit a game back to origin; the game did not crash'
            /// FreeTrialExited = 'user quit a trial back to origin'
            /// FreeTrialExpired = 'free trial expired and was forced to quit'
            /// DownloadUnderway = 'user started a download of more than 500MB'
            /// MenuPromotions = 'user manually invoked promo manager via menu item'
            /// NoType = no promotype
            ///
            /// \sa promoTypeFromString
            enum PromoType
            { 
                OriginStarted, 
                GameFinished,
                FreeTrialExited,
                FreeTrialExpired,
                DownloadUnderway,
                MenuPromotions,
                NoType
            };

            /// \brief Additional parameters that govern the behavior
            /// and presentation of the promo manager. Also used to provide
            /// more context for conditions in telemetry.
            ///
            /// GameTileContextMenu = 'invoked from game tile context menu (within MyGames)'
            /// GameHovercard = 'invoked from game hovercard (within MyGames)'
            /// OwnedGameDetails = 'invoked from the owned game details (OGD) page'
            /// NoScope = no scope provided
            ///
            /// \note Currently this enum only contains user-intiated context. If new
            /// scope is added that doesn't imply a user invocation, then please check
            /// the rest of the code base to ensure assumptions aren't made regarding
            /// these enum values.
            ///
            /// \sa scopeFromString
            enum Scope
            { 
                GameTileContextMenu, 
                GameHovercard,
                OwnedGameDetails,
                NoScope
            };

            /// \return PromoType enum that maps to the given string. PromoType::NoType is
            /// returned if no match is made.
            ORIGIN_PLUGIN_API PromoType promoTypeFromString(const QString& promoTypeString);

            /// \return Promo scope that maps to the given string. Scope::NoScope is returned
            /// if a match isn't made.
            ORIGIN_PLUGIN_API Scope scopeFromString(const QString& scopeString);

            /// \return PromoType string representation of the given promotype enum. "NoType"
            /// is returned if the given enum value couldn't be matched.
            ORIGIN_PLUGIN_API QString promoTypeString(const PromoContext::PromoType promoType);

            /// \return Promo scope string representation of the given scope enum. "NoScope"
            /// is returned if the given enum value couldn't be matched.
            ORIGIN_PLUGIN_API QString scopeString(const PromoContext::Scope scope);

            /// \return True if the promo browser was invoked by a user action, false otherwise.
            ORIGIN_PLUGIN_API bool userInitiatedContext(const PromoBrowserContext& context);
        }

        /// \brief Provides a primary and secondary context that influences behavior,
        /// presentation, and telemetry parameters surrounding the promo manager dialog.
        ///
        /// \sa PromoBrowserViewController
        struct PromoBrowserContext 
        {
            ORIGIN_PLUGIN_API PromoBrowserContext(const PromoContext::PromoType promoType, const PromoContext::Scope scope = PromoContext::NoScope);

            PromoContext::PromoType promoType;
            PromoContext::Scope scope;
        };
    }
}

#endif
