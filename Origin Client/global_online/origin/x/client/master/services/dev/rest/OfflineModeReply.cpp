#include "services/network/OfflineModeReply.h"

namespace Origin
{
namespace Services
{

OfflineModeReply::OfflineModeReply(QNetworkAccessManager::Operation op, const QNetworkRequest &req, 
    NetworkError networkError)
{
    // Do basic set up
    setRequest(req);
    setUrl(req.url());
    setOperation(op);
    setError(networkError, "");

    // Simulate failure when we next enter the event loop
    QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection, 
        Q_ARG(QNetworkReply::NetworkError, networkError));

    setFinished(true);
    QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
}

void OfflineModeReply::abort()
{
    // Don't do anything - we'll be denied next event loop
}

qint64 OfflineModeReply::readData(char *, qint64)
{
    return -1;
}

}

}
