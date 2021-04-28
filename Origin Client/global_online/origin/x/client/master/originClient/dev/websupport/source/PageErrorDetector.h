#ifndef _PAGEERRORDETECTOR_H
#define _PAGEERRORDETECTOR_H

#include <QObject>
#include <QFlags>

#include "services/plugin/PluginAPI.h"

class QWebPage;
class QTimer;
class QNetworkReply;

namespace Origin
{
namespace Client
{

///
/// \brief Detects various error conditions in a QWebPage 
/// 
/// This works around the poor error detection QtWebKit provides out of the box by using various heuristics to 
/// detect errors with finer granularity 
///
class ORIGIN_PLUGIN_API PageErrorDetector : public QObject
{
    Q_OBJECT
public:
    enum ErrorType
    {
        /// \brief No error. This is provided for convenience; it will never be emitted
        NoError = 0x0,

        /// \brief QtWebKit emitted loadFinished(false)
        FinishedWithFailure = 0x1,

        /// \brief The network request for the content of our main frame failed
        MainFrameRequestFailed= 0x2,

        /// \brief Timed out after 30 seconds
        LoadTimedOut = 0x4,
    };

    Q_DECLARE_FLAGS(ErrorTypes, ErrorType)

    ///
    /// \brief Constructs an instance monitoring the given page
    ///
    /// No error checks are detected by default
    ///
    /// \param  page Page to monitor for errors. This will also become the QObject parent
    ///
    explicit PageErrorDetector(QWebPage *page);

    /// \brief Sets if an error check is enabled
    void setErrorCheckEnabled(ErrorType type, bool enabled);

    /// \brief Enables error checking for a specific type
    void enableErrorCheck(ErrorType type);

    /// \brief Disables error checking for a specific type
    void disableErrorCheck(ErrorType type);

    /// \brief Returns if an error check is enabled
    bool errorCheckEnabled(ErrorType type) const;

    /// \brief Returns the current error type
    ErrorType currentErrorType() const {return mCurrentErrorType;}

    /// \brief Connects/Disconnects load signals from page
    void connectDisconnectPageLoadSignals(const bool& shouldConnect) const;

signals:

    /// \brief Emitted when an error is detected.
    ///
    /// \param  type      Type of error detected.
    /// \param  reply     TBD.
    void errorDetected(Origin::Client::PageErrorDetector::ErrorType type, QNetworkReply *reply);

    /// \brief Emitted when our connection has been reestablished
    void connectionReestablished();

protected slots:
    void pageLoadStarted();
    void pageLoadFinished(bool);
    void pageLoadProgress();

    void loadTimedOut();

    void networkRequestFinished(QNetworkReply *);

private:
    void handleDetectedError(ErrorType type, QNetworkReply *reply = NULL);
    void disarmTimeoutTimer();

    ErrorTypes mEnabledErrorChecks;

    QWebPage *mPage;

    QTimer *mTimeoutTimer;
    ErrorType mCurrentErrorType;
    int mPageTimeoutMilliseconds;
};
    
Q_DECLARE_OPERATORS_FOR_FLAGS(PageErrorDetector::ErrorTypes)

}
}

#endif
