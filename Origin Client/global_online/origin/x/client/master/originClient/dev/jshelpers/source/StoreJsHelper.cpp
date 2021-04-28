///////////////////////////////////////////////////////////////////////////////
// StoreJsHelper.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "StoreJsHelper.h"
#include "OriginApplication.h"
#include "services/platform/PlatformService.h"
#include "services/qt/QtUtil.h"
#include "TelemetryAPIDLL.h"
#include "flows/MainFlow.h"
#include "flows/ClientFlow.h"

namespace Origin
{
    namespace Client
    {
        StoreJsHelper::StoreJsHelper(QObject *parent) : MyGameJsHelper(parent)
        {
        }

        void StoreJsHelper::goOnline()
        {
            //MainPage::instance()->goOnline();
        }

        void StoreJsHelper::restartOrigin()
        {
            MainFlow::instance()->restart();
        }

        void StoreJsHelper::launchExternalBrowser(QString urlstring)
        {
            QUrl url(QUrl::fromEncoded(urlstring.toUtf8()));
            Origin::Services::PlatformService::asyncOpenUrl(url);
            GetTelemetryInterface()->Metric_STORE_NAVIGATE_URL(Services::EncodeUrlForTelemetry(url).data());
        }

        void StoreJsHelper::launchInicisSsoCheckout(QString urlparams)
        {
            Origin::Client::ClientFlow::instance()->launchInicisSsoCheckout(urlparams);
        }

        void StoreJsHelper::continueShopping(const QString& url)
        {
            if(url.isEmpty() || url.compare("home", Qt::CaseInsensitive) == 0)
            {
                loadStoreHome();
            }
            else
            {
                Origin::Client::ClientFlow::instance()->showUrlInStore(QUrl(url));
            }
        }

        void StoreJsHelper::print()
        {
            emit printRequested();
        }

    }
}