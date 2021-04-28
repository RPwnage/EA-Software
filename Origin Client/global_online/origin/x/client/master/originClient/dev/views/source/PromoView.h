/////////////////////////////////////////////////////////////////////////////
// PromoView.cpp
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef PROMOVIEW_H
#define PROMOVIEW_H

#include "services/plugin/PluginAPI.h"
#include "engine/content/ContentTypes.h"

#include "PromoContext.h"
#include "PageErrorDetector.h"

#include "originwebview.h"

#include <QObject>
#include <QNetworkRequest>
#include <QString>
#include <QQueue>
#include <QUrl>

class QNetworkReply;

namespace Origin
{
    namespace Client
    {
        class OriginWebController;

        class ORIGIN_PLUGIN_API PromoView : public Origin::UIToolkit::OriginWebView
        {
            Q_OBJECT

        public:
            PromoView(QWidget* parent = NULL);
            ~PromoView();

            void load(const PromoBrowserContext& context, const Engine::Content::EntitlementRef ent);

        signals:
            void loadFinished(int httpCode, bool successful);
            void closeWindow();

        private slots:
            void onRequestFinished(QNetworkReply *reply);
            void onLoadFinished(bool successful);
            void onErrorDetected(Origin::Client::PageErrorDetector::ErrorType errorType);

        private:
            OriginWebController* mWebController;

            int mPromoUrlStatusCode;
            QUrl mCurrentPromoUrl;

            struct PromoTraffic 
            {
                int statusCode;
                QString url;
            };
            QQueue<PromoTraffic> mDebugTraffic;
        };
    }
}

#endif
