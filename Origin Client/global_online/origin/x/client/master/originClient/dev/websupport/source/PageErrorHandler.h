#ifndef _PAGEERRORHANDLER_H
#define _PAGEERRORHANDLER_H

#include <QObject>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

#include "PageErrorDetector.h"
#include "services/plugin/PluginAPI.h"

class QWebPage;

namespace Origin
{
namespace Client
{

class AbstractErrorPage;

///
/// \brief Handles page error and offline mode display for a QWebPAge
///
/// This builds on PageErrorDetector to actually present errors to the user. See the PageErrorDetector
/// documentation for more information
///
class ORIGIN_PLUGIN_API PageErrorHandler : public QObject
{
    Q_OBJECT
public:
    ///
    /// \brief Creates an error handler instance for the passed QWebPage
    ///
    /// \param  page       Page to handler errors for. This will also be used as the QObject parent
    /// \param  errorPage  Delegate to build error page HTML. Defaults to an instance of DefaultErrorPage
    ///
    explicit PageErrorHandler(QWebPage *page, AbstractErrorPage *errorPage = NULL);
    ~PageErrorHandler();

    ///
    /// \brief Returns page error detector instance.
    ///
    /// This can be used to configure the types of errors that are detected and handled
    ///
    PageErrorDetector* errorDetector() const { return mPageErrorDetector; }

    ///
    /// \brief Returns the current error page
    ///
    AbstractErrorPage* errorPage() const { return mErrorPage; }

    ///
    /// \brief Displays the offline error page
    ///
    void showOfflineError();
    
    ///
    /// \brief Displays the page error page
    ///
    void showPageError(PageErrorDetector::ErrorType type, QNetworkReply *reply = NULL);

    ///
    /// \brief Sets the error page
    ///
    /// \param page New error page to set. PageErrorHandler takes ownership of the page
    ///
    void setErrorPage(AbstractErrorPage *page);

    ///
    /// \brief Returns if we're handling offline mode as an error
    ///
    /// This defaults to false
    ///
    bool handleOfflineAsError() const { return mHandleOfflineAsError; }

    ///
    /// \brief Sets if we're handling offline mode as an error
    ///
    void setHandleOfflineAsError(bool handleAsError);

    ///
    /// \brief Sets whether we should track the current URL in case of an error.
    ///
    /// If true, we'll capture the web view's current URL when an error occurs and attempt to reload that URL upon recovery.
    /// If false, we'll attempt to reload the initial request upon recovery.
    /// This is useful when the initial request uses a non-standard method (PUT/POST/etc).
    /// This defaults to true, and is only respected if mHandleOfflineAsError is true.
    ///
    void setTrackCurrentUrl(bool track) { mTrackCurrentUrl = track; }

    ///
    /// \brief Sets the URL the web page will restore to when coming back from offline mode.
    ///
    void setRequestToRestore(const QNetworkRequest &req, QNetworkAccessManager::Operation op = QNetworkAccessManager::GetOperation, const QByteArray &body = QByteArray());

private slots:
    void onConnectionStateChanged();

    void loadStartedWhileOffline();

    void pageErrorDetected(Origin::Client::PageErrorDetector::ErrorType type, QNetworkReply *reply);

    void onRecovering();

private:
    Q_INVOKABLE void deferredShowErrorContent(QString content);

    void updateConnectionState();
    void nowOffline();
    void nowOnline();

    QWebPage *mPage;
    PageErrorDetector *mPageErrorDetector;
    AbstractErrorPage *mErrorPage;
    bool mHandleOfflineAsError;
    bool mTrackCurrentUrl;
    bool mLoadingErrorContent;
	bool mOffline;

    QNetworkRequest mRequestToRestore;
    QNetworkAccessManager::Operation mRequestToRestoreOperation;
    QByteArray mRequestToRestoreBody;
};

}
}

#endif
