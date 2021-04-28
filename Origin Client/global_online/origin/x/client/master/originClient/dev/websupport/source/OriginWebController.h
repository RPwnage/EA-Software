#ifndef _ORIGINWEBCONTROLLER_H
#define _ORIGINWEBCONTROLLER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

// Convenience includes for users
// All of our functionality is behind these objects; it's annoying to have to include them for common tasks 
#include "PageErrorHandler.h"
#include "PageErrorDetector.h"
#include "JavascriptCommunicationManager.h"
#include "DefaultErrorPage.h"
#include "services/plugin/PluginAPI.h"

class QWebView;
class QWebPage;
class QWidget;
class QUrl;

namespace Origin
{
namespace Client
{

///
/// \brief Class implementing some common web related functionality
///
/// This provides:
/// - Standard NetworkAccessManager support using HTTP caching, proxy detection, etc
/// - JavaScript helpers and JavaScript bridge (native feature) support
/// - Native behavior such as styled scrollbars and modified context menu
/// - Page error detection 
///
/// This class bind a number of lower level objects from websupport/ together. Callers with more exotic needs
/// needs may prefer to use those classes directly
///
/// \sa JavascriptCommunicationManager
/// \sa PageErrorHandler
/// \sa PageErrorDetector
/// \sa NativeBehaviorManager
///
class ORIGIN_PLUGIN_API OriginWebController : public QObject
{
Q_OBJECT

public:    
    ///
    /// \brief Constructs a controller managing a QWebView.
    ///
    /// The passed web view will have its page replaced with an OriginWebPage instance.
    ///
    /// \param  webView               The Web view to manage.
    /// \param  errorPage             The error page delegate. Defaults to a left aligned error page.
    /// \param  handleOfflineAsError  TBD.
    OriginWebController(QWebView *webView, AbstractErrorPage *errorPage = NULL, bool handleOfflineAsError = true);

    ///
    /// \brief Constructs a controller managing  a specified view widget and page
    ///
    /// The passed page will not be replaced or modified.
    ///
    /// \param  viewWidget            The widget displaying the page
    /// \param  page                  The page to manage.
    /// \param  errorPage             The error page delegate. Defaults to a left aligned error page.
    /// \param  handleOfflineAsError  TBD.
    OriginWebController(QWidget *viewWidget, QWebPage *page, AbstractErrorPage *errorPage = NULL, bool handleOfflineAsError = true);

    /// \brief Returns the JavaScript bridge manager for this controller
    JavascriptCommunicationManager *jsBridge() const { return mJsBridge; }

    /// \brief Returns the page error handler for this controller
    PageErrorHandler *errorHandler() const { return mPageErrorHandler; }

    /// \brief Shortcut for errorHandler()->errorDetector()
    PageErrorDetector *errorDetector() const;

    /// \brief Returns the QWebPage being managed
    QWebPage *page() const { return mPage; }

    ///
    /// \brief Loads a URL that's trusted to access client JavaScript APIs
    ///
    /// The hostname of the URL will become trusted by the JavaScript bridge. If the browser navigates off of the
    /// hostname all JavaScript helpers will be disabled
    ///
    /// \sa JavascriptCommunicationManager::setTrustedHostname
    ///
    void loadTrustedUrl(const QUrl &);
    
    ///
    /// \brief Loads a request that's trusted to access client JavaScript APIs
    ///
    /// The hostname of the URL will become trusted by the JavaScript bridge. If the browser navigates off of the
    /// hostname all JavaScript helpers will be disabled
    ///
    /// \sa JavascriptCommunicationManager::setTrustedHostname
    ///
    void loadTrustedRequest(const QNetworkRequest &req, QNetworkAccessManager::Operation op = QNetworkAccessManager::GetOperation, const QByteArray &body = QByteArray());

protected:
    void init(QWidget *viewWidget, QWebPage *page, AbstractErrorPage *errorPage = NULL, bool handleOfflineAsError = true);

    QWebPage *mPage;
    JavascriptCommunicationManager *mJsBridge;
    PageErrorHandler *mPageErrorHandler;
};

}
}

#endif
