#ifndef PDLCVIEW_H
#define PDLCVIEW_H

#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"
#include "originwebview.h"
#include <QUrl>

namespace Origin
{
    namespace Client
    {
        class OriginWebController;
        class PDLCJsHelper;

        class ORIGIN_PLUGIN_API PDLCView : public UIToolkit::OriginWebView
        {
            Q_OBJECT

        public:
            PDLCView(bool inIGO, QWidget* parent = NULL);
           ~PDLCView();

            void setUrl(const QUrl& url);
            void setEntitlement(const Engine::Content::EntitlementRef entitlement);

            void setUrlCategoryIds(const QString &categoryIds){ mCategoryIds = categoryIds; }
            QString urlCategoryIds() const {return mCategoryIds;}
            void setUrlOfferIds(const QString &offerIds){ mOfferIds = offerIds; }
            QString urlOfferIds() const {return mOfferIds;}
            void setUrlMasterTitleId(const QString &masterTitleId){ mMasterTitleId = masterTitleId; }
            QString urlMasterTitleId() const {return mMasterTitleId;}
			QString productId() const { return mProductId; }

            // Disconnect navigation from setting query parameters.
            void navigate();

			// Directly go to lockbox checkout for specified product id
			void purchase(QString productId);

        protected slots:
            void updateCemLoginCookie();
            void onPrintRequested();
            void onUrlChanged(const QUrl& url);
            void onContentsSizeChanged(const QSize& size);
            void onRequestFinished(QNetworkReply *reply);

        signals:
            void closeStore();
            void sizeChanged(const QSize& size);

        private:
            void setUrl(const QString& gamerId, const QString& masterTitleId, const QString& categoryIds, const QString& offerIds);

        private:
            OriginWebController* mWebController;
            PDLCJsHelper* mHelper;
            QString mCategoryIds;
            QString mOfferIds;
            QString mMasterTitleId;
			QString mProductId;
        };
    }
}

#endif
