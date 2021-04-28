#include <QDesktopServices>
#include <QWebFrame>

#include "OriginWebPage.h"
#include "originwindow.h"
#include "engine/igo/IGOController.h"
#include "services/network/NetworkAccessManager.h"
#include "services/network/NetworkAccessManagerFactory.h"
#include "services/platform/PlatformService.h"
#include "services/rest/UserAgent.h"
#include "services/log/LogService.h"
#include "PopOutController.h"

namespace Origin
{
namespace Client
{

OriginWebPage::OriginWebPage(QObject *parent)
: QWebPage(parent)
{
    // We need longer names 
    Origin::Services::NetworkAccessManager *manager = 
        Origin::Services::NetworkAccessManagerFactory::instance()->createNetworkAccessManager();

    setNetworkAccessManager(manager);
}

QString OriginWebPage::userAgentForUrl ( const QUrl & url ) const
{
    const QString userAgent = QWebPage::userAgentForUrl(url) + QString(" ") + Origin::Services::userAgentHeaderNoMozilla();
    return userAgent;
}

QString OriginWebPage::chooseFile(QWebFrame *parentFrame, const QString &)
{
    const QString homeDirectory(Origin::Services::PlatformService::getStorageLocation(QStandardPaths::HomeLocation));
    return QWebPage::chooseFile(parentFrame, homeDirectory);
}

QWebPage* OriginWebPage::createWindow(WebWindowType type)
{
    QWebPage* webPage = NULL;

    // Do we have access to the request?
    QVariant urlProp = property(CreateWindowUrlKey());
    if (urlProp.isValid())
    {
        // Is it a request for an OIG window?
        QUrl url(urlProp.toString());
        webPage = createJSWindow(url);

        // Make sure to clean up
        setProperty(CreateWindowUrlKey(), QVariant());
    }

    if (!webPage)
        webPage = QWebPage::createWindow(type);

    return webPage;
}

bool OriginWebPage::acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type)
{
    bool accept = QWebPage::acceptNavigationRequest(frame, request, type);

    ORIGIN_LOG_DEBUG << "AcceptNavigationRequest: " << accept << " for request: " << request.url().toString();
     
    return accept;
}

QWebPage* OriginWebPage::createJSWindow(QUrl url)
{
    // Check if this was a request for an IGO window
   QWebPage* childWebPage = Origin::Engine::IGOController::instance()->createJSWindow(url);
  
   if (!childWebPage )
   {
        childWebPage = Origin::Client::PopOutController::instance()->createJSWindow(url);
   }

   return childWebPage;
}


}
}
