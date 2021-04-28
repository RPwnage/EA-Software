//  AuthenticationMonitor.cpp
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.

#include "AuthenticationMonitor.h"

#include "services/rest/HttpStatusCodes.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "TelemetryAPIDLL.h"

#include <QWebFrame>

namespace Origin
{
    namespace Services
    {

        AuthenticationMonitor::AuthenticationMonitor(QWebPage* webPage) :
            mWebPage(webPage)
        {

        }
        
        void AuthenticationMonitor::setWebPage(QWebPage* page)
        {
            mWebPage = page;
        }

        void AuthenticationMonitor::setRelevantHosts(const QStringList& hostList)
        {
            mRelevantHosts = hostList;
        }

        void AuthenticationMonitor::start()
        {
            if (mWebPage)
            {
                ORIGIN_VERIFY_CONNECT(mWebPage->networkAccessManager(), SIGNAL(finished(QNetworkReply *)), this, SLOT(onRequestFinished(QNetworkReply *)));
            }
        }

        void AuthenticationMonitor::shutdown()
        {
            if (mWebPage)
            {
                ORIGIN_VERIFY_DISCONNECT(mWebPage->networkAccessManager(), SIGNAL(finished(QNetworkReply *)), this, SLOT(onRequestFinished(QNetworkReply *)));
            }
        }
            
        void AuthenticationMonitor::onRequestFinished(QNetworkReply* reply)
        {
            if (!reply || !mWebPage)
            {
                return;
            }

            const QString lowercaseReplyHost = reply->url().host().toLower();

            if (lowercaseReplyHost != mWebPage->mainFrame()->url().host().toLower())
            {
                // This is for a different host
                return;
            }
 
            if(isRelevantHost(lowercaseReplyHost))
            {
                if (reply->error() == QNetworkReply::NoError)
                {
                    // 200-like response; assume we're authenticated;
                }
                else if (reply->error() == QNetworkReply::OperationCanceledError)
                {
                    // Qt uses this for 304s - ignore
                    return;
                }
                else
                {
                    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                    bool is403Error = statusCode == Origin::Services::Http403ClientErrorForbidden || reply->error() == QNetworkReply::ContentOperationNotPermittedError;

                    if (is403Error)
                    {
                        const QString logSafeUrl(LogUtil::logSafeUrl(reply->url().toString()));
						ORIGIN_LOG_ERROR << "Lost authentication.  Error code " << reply->error() << ", HTTP " << statusCode << ", URL " << logSafeUrl;
                        GetTelemetryInterface()->Metric_AUTHENTICATION_LOST(reply->error(), statusCode, logSafeUrl.toUtf8().constData());
                        emit lostAuthentication();
                    }
                }
            }
        }

        bool AuthenticationMonitor::isRelevantHost(const QString& host)
        {
            bool found = false;
            int i = 0;
            while (!found && i < mRelevantHosts.size())
            {
                found = !mRelevantHosts.at(i).isEmpty() && host.endsWith(mRelevantHosts.at(i), Qt::CaseInsensitive);
                ++i;
            }

            return found;
        }
    }
}
