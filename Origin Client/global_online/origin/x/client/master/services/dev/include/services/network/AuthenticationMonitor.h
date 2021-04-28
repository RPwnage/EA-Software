//  AuthenticationMonitor.h
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.

#ifndef AUTHENTICATIONMONITOR_H
#define AUTHENTICATIONMONITOR_H

#include <QNetworkReply>
#include <QWebPage>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API AuthenticationMonitor : public QObject
        {
            Q_OBJECT

            public:
                AuthenticationMonitor(QWebPage* webPage = NULL);

                void setWebPage(QWebPage* page);
                void setRelevantHosts(const QStringList& hostList);
                void start();
                void shutdown();
                      
            signals:
                void lostAuthentication();

            private slots:

                void onRequestFinished(QNetworkReply* reply);

            private:

                QWebPage* mWebPage;
                QStringList mRelevantHosts;

                bool isRelevantHost(const QString& host);
        };
    }
}

#endif // AUTHENTICATIONMONITOR_H
