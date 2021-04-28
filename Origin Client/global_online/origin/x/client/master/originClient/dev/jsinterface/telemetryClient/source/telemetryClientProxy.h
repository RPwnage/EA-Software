///////////////////////////////////////////////////////////////////////////////
// telemetryClientProxy.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef _TELEMETRYCLIENTPROXY_H
#define _TELEMETRYCLIENTPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API TelemetryClientProxy : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE void sendFreeTrialPurchase(const QString &contentId, const QString &expired, const QString &source);
    Q_INVOKABLE void sendContentSalesPurchase(const QString &id, const QString &productType, const QString &context);
    Q_INVOKABLE void sendContentSalesViewInStore(const QString &id, const QString &productType, const QString &context);
    Q_INVOKABLE void sendAchievementWidgetPageView(const QString &pageName, const QString &productId, bool inIgo);
    Q_INVOKABLE void sendAchievementWidgetPurchaseClick(const QString &productId);
    
    Q_INVOKABLE void sendMyGamesCloudStorageTabView(const QString &productId);
    Q_INVOKABLE void sendMyGamesManualLinkClick(const QString &productId);
    Q_INVOKABLE void sendMyGamesDetailsUpdateClick(const QString& productId);

    Q_INVOKABLE void sendBroadcastJoined(const QString& source);
    Q_INVOKABLE void sendAchievementSharing(const QString& myVal, const QString& source);


    Q_INVOKABLE void sendGamePlaySource(const QString& productId, const QString& source);

    Q_INVOKABLE void sendProfileGameSource(const QString& source);

    Q_INVOKABLE void sendAddonDescriptionExpanded(const QString& addon);
    Q_INVOKABLE void sendAddonBuyClick(const QString& addon);

    Q_INVOKABLE void sendHovercardBuyNowClick(const QString& gameId);
    Q_INVOKABLE void sendHovercardBuyClick(const QString& gameId);
    Q_INVOKABLE void sendHovercardDownloadClick(const QString& gameId);
    Q_INVOKABLE void sendHovercardDetailsClick(const QString& gameId);
    Q_INVOKABLE void sendHovercardAchievementsClick(const QString& gameId);
    Q_INVOKABLE void sendHovercardUpdateClick(const QString& gameId);

    Q_INVOKABLE void sendAchievementComparisonView(const QString& productid, bool userOwnsProduct);
    
    Q_INVOKABLE void sendOnTheHouseBannerLoaded();
    Q_INVOKABLE void sendQueueOrderChanged(const QString& oldHeadid, const QString& newHeadid, const QString& source);
    Q_INVOKABLE void sendQueueItemRemoved(const QString& productId, const QString& source);
    Q_INVOKABLE void sendNetPromoterResults(const QString& offerId, const QString& score, const bool& canContact, const QString& feedback);

    Q_INVOKABLE void sendFeatureCalloutDimissed(const QString& seriesName, const QString& parentName, const bool& wasEngaged);
    Q_INVOKABLE void sendFeatureCalloutSeriesComplete(const QString& seriesName, const bool& isComplete);
    Q_INVOKABLE void sendSubscriptionUpsell(const QString& context);
};

}
}
}

#endif
