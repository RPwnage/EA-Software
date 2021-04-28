///////////////////////////////////////////////////////////////////////////////
// telemetryClientProxy.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include <QUrl>

#include "TelemetryClientProxy.h"
#include "TelemetryAPIDLL.h"
#include "engine/content/ContentController.h"
#include "engine/igo/IGOController.h"
#include "engine/content/ContentOperationQueueController.h"
#include "engine/subscription/SubscriptionManager.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {

            void TelemetryClientProxy::sendFreeTrialPurchase(const QString &contentId, const QString &expired, const QString &source)
            {
                GetTelemetryInterface()->Metric_FREETRIAL_PURCHASE( contentId.toUtf8().data(), expired.toUtf8().data(), source.toUtf8().data() );
            }

            void TelemetryClientProxy::sendContentSalesPurchase(const QString &id, const QString &productType, const QString &context)
            {
                GetTelemetryInterface()->Metric_CONTENTSALES_PURCHASE( id.toUtf8().data(), productType.toUtf8().data(), context.toUtf8().data() );
            }

            void TelemetryClientProxy::sendContentSalesViewInStore(const QString &id, const QString &productType, const QString &context)
            {
                GetTelemetryInterface()->Metric_CONTENTSALES_VIEW_IN_STORE( id.toUtf8().data(), productType.toUtf8().data(), context.toUtf8().data() );
            }

            void TelemetryClientProxy::sendAchievementWidgetPageView(const QString &pageName, const QString &productId, bool inIgo)
            {
                GetTelemetryInterface()->Metric_ACHIEVEMENT_WIDGET_PAGE_VIEW(pageName.toUtf8().data(), productId.toUtf8().data(), inIgo);
            }

            void TelemetryClientProxy::sendAchievementWidgetPurchaseClick(const QString &productId)
            {
                GetTelemetryInterface()->Metric_ACHIEVEMENT_WIDGET_PURCHASE_CLICK(productId.toUtf8().data());
            }

            void TelemetryClientProxy::sendMyGamesCloudStorageTabView(const QString &productId)
            {
                GetTelemetryInterface()->Metric_GUI_MYGAMES_CLOUD_STORAGE_TAB_VIEW(productId.toUtf8().data());
            }

            void TelemetryClientProxy::sendMyGamesManualLinkClick(const QString &productId)
            {
                GetTelemetryInterface()->Metric_GUI_MYGAMES_MANUAL_LINK_CLICK(productId.toUtf8().data());
            }

            void TelemetryClientProxy::sendMyGamesDetailsUpdateClick(const QString &productId)
            {
                GetTelemetryInterface()->Metric_GUI_DETAILS_UPDATE_CLICKED(productId.toUtf8().data());
            }

            void TelemetryClientProxy::sendBroadcastJoined(const QString& source)
            {
                GetTelemetryInterface()->Metric_BROADCAST_JOINED(source.toUtf8().data());
            }

            void TelemetryClientProxy::sendAchievementSharing( const QString& myVal, const QString& source )
            {
                if (myVal=="show")
                {
                    GetTelemetryInterface()->Metric_ACHIEVEMENT_SHARE_ACHIEVEMENT_SHOW(source.toUtf8());
                }
                else if (myVal=="dismiss")
                {
                    GetTelemetryInterface()->Metric_ACHIEVEMENT_SHARE_ACHIEVEMENT_DISMISSED(source.toUtf8());
                }
            }

            void TelemetryClientProxy::sendGamePlaySource(const QString& productId, const QString& source)
            {
                GetTelemetryInterface()->Metric_GUI_MYGAMES_PLAY_SOURCE(productId.toUtf8(), source.toUtf8());

                Engine::Content::EntitlementRef entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementByProductId(productId);
                if(entitlement.isNull() == false && entitlement->localContent() && entitlement->localContent()->installFlow())
                {
                    Downloader::IContentInstallFlow* installFlow = entitlement->localContent()->installFlow();
                    if(installFlow->isDynamicDownload())
                    {
                        GetTelemetryInterface()->Metric_DYNAMICDOWNLOAD_GAMELAUNCH(productId.toUtf8(), installFlow->currentDownloadJobId().toUtf8(), source.toUtf8(), 
                            installFlow->progressDetail().mBytesDownloaded, installFlow->progressDetail().mDynamicDownloadRequiredPortionSize,
                            installFlow->progressDetail().mTotalBytes);
                    }
                }
            }

            void TelemetryClientProxy::sendProfileGameSource(const QString& source)
            {
                GetTelemetryInterface()->Metric_USER_PROFILE_VIEWGAMESOURCE(source.toUtf8());
            }

            void TelemetryClientProxy::sendAddonDescriptionExpanded(const QString& addon)
            {
                GetTelemetryInterface()->Metric_GAMEPROPERTIES_ADDON_DESCR_EXPANDED(addon.toUtf8());
            }

            void TelemetryClientProxy::sendAddonBuyClick(const QString& addon)
            {
                GetTelemetryInterface()->Metric_GAMEPROPERTIES_ADDON_BUY_CLICK(addon.toUtf8());
            }

            void TelemetryClientProxy::sendHovercardBuyNowClick(const QString& gameId)
            {
                GetTelemetryInterface()->Metric_HOVERCARD_BUYNOW_CLICK(gameId.toUtf8());
            }

            void TelemetryClientProxy::sendHovercardBuyClick(const QString& gameId)
            {
                GetTelemetryInterface()->Metric_HOVERCARD_BUY_CLICK(gameId.toUtf8());
            }

            void TelemetryClientProxy::sendHovercardDownloadClick(const QString& gameId)
            {
                GetTelemetryInterface()->Metric_HOVERCARD_DOWNLOAD_CLICK(gameId.toUtf8());
            }

            void TelemetryClientProxy::sendHovercardDetailsClick(const QString& gameId)
            {
                GetTelemetryInterface()->Metric_HOVERCARD_DETAILS_CLICK(gameId.toUtf8());
            }

            void TelemetryClientProxy::sendHovercardAchievementsClick(const QString& gameId)
            {
                GetTelemetryInterface()->Metric_HOVERCARD_ACHIEVEMENTS_CLICK(gameId.toUtf8());
            }

            void TelemetryClientProxy::sendHovercardUpdateClick(const QString& gameId)
            {
                GetTelemetryInterface()->Metric_HOVERCARD_UPDATE_CLICK(gameId.toUtf8());
            }

            void TelemetryClientProxy::sendAchievementComparisonView(const QString& productid, bool userOwnsProduct)
            {
                GetTelemetryInterface()->Metric_ACHIEVEMENT_COMPARISON_VIEW(productid.toUtf8(), userOwnsProduct);
            }

			void TelemetryClientProxy::sendOnTheHouseBannerLoaded()
			{
				GetTelemetryInterface()->Metric_OTH_BANNER_LOADED();
			}

            void TelemetryClientProxy::sendQueueOrderChanged(const QString& oldHeadid, const QString& newHeadid, const QString& source)
            {
                QString from = Engine::IGOController::instance()->isActive() ? "oig-" : "client-";
                from += source;
                GetTelemetryInterface()->Metric_QUEUE_ORDER_CHANGED(oldHeadid.toUtf8(), newHeadid.toUtf8(), from.toUtf8());
            }

            void TelemetryClientProxy::sendQueueItemRemoved(const QString& productId, const QString& source)
            {
                int index = -1;

				// must protect against user logging out which results in a null queueController
				Engine::Content::ContentOperationQueueController* queueController = Engine::Content::ContentOperationQueueController::currentUserContentOperationQueueController();
				if (queueController)
					index = queueController->indexInQueue(productId);

                QString from = Engine::IGOController::instance()->isActive() ? "oig-" : "client-";
                from += source;
                GetTelemetryInterface()->Metric_QUEUE_ITEM_REMOVED(productId.toUtf8(), index == 0 ? "true" : "false", from.toUtf8());
            }

            void TelemetryClientProxy::sendNetPromoterResults(const QString& offerId, const QString& score, const bool& canContact, const QString& feedback)
            {
                const QString locale = QLocale::system().name();
                QString feedbackCopy = feedback;
                QByteArray feedbackBytes = feedback.toUtf8().toBase64();
                // adjust length by chopping of characters (in the QString), chopping bytes 
                // (in the QByteArray) might upset the last encoded character.
                const int TEXT_LIMIT = 4000; // telemetry limit is 4k
                while(feedbackBytes.size() > TEXT_LIMIT)
                {
                    feedbackCopy.chop(1);
                    feedbackBytes = feedbackCopy.toUtf8().toBase64();
                }
                GetTelemetryInterface()->Metric_NET_PROMOTER_GAME_SCORE(offerId.toLatin1().data(), score.toLatin1().data(), 
                    feedbackBytes.data(), locale.toLatin1().data(), canContact);
            }

            void TelemetryClientProxy::sendFeatureCalloutDimissed(const QString& seriesName, const QString& parentName, const bool& wasEngaged)
            {
                GetTelemetryInterface()->Metric_FEATURE_CALLOUT_DISMISSED(seriesName.toStdString().c_str(), parentName.toStdString().c_str(), wasEngaged);
            }

            void TelemetryClientProxy::sendFeatureCalloutSeriesComplete(const QString& seriesName, const bool& isComplete)
            {
                GetTelemetryInterface()->Metric_FEATURE_CALLOUT_SERIES_COMPLETE(seriesName.toStdString().c_str(), isComplete);
            }

            void TelemetryClientProxy::sendSubscriptionUpsell(const QString& context)
            {
                GetTelemetryInterface()->Metric_SUBSCRIPTION_UPSELL(context.toStdString().c_str(), Engine::Subscription::SubscriptionManager::instance()->isActive());
            }
        }
    }
}
