/////////////////////////////////////////////////////////////////////////////
// StoreViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef STOREVIEWCONTROLLER_H
#define STOREVIEWCONTROLLER_H

#include <QObject>
#include <QUrl>
#include <QNetworkCookie>

#include "services/network/AuthenticationMonitor.h"
#include "services/session/AbstractSession.h"
#include "services/settings/Setting.h"
#include "services/plugin/PluginAPI.h"

class QWidget;

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWebView;
    }
    namespace Client
    {

        class StoreView;
        class StoreUrlBuilder;

        class ORIGIN_PLUGIN_API StoreViewController : public QObject
        {
            Q_OBJECT
        public:
            StoreViewController(QWidget *parent, bool mainStoreTab=true);
            ~StoreViewController();

            void loadStoreUrl(const QUrl &url);

            // Uses the default initial page
            void loadEntryUrlAndExpose(const QUrl &url);

            const StoreUrlBuilder *storeUrlBuilder() { return mStoreUrlBuilder; }

            QWidget *view() const;
            StoreView* storeView() const;

        protected slots:
            void storeUrlChanged(const QUrl &);

            void onLostAuthentication();

            void onLoadFinished(bool);

        signals:
            /// \brief Emitted when the user loses authentication.
            void lostAuthentication();

        protected:
            const StoreUrlBuilder *mStoreUrlBuilder;
            UIToolkit::OriginWebView* mWebViewContainer;
            StoreView *mStoreView;
            bool mIsMainStoreTab;

        private:
            void loadPersistentCookie(const QByteArray& cookieName, const Services::Setting& settingKey);
            void persistCookie(const QByteArray& name, const Services::Setting& settingKey);

            Services::AuthenticationMonitor mAuthenticationMonitor;
        };

    }
}

#endif
