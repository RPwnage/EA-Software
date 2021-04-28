#ifndef OFFLINE_MODE_REPLY_H
#define OFFLINE_MODE_REPLY_H

#include <QNetworkReply>

#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Services
{

///
/// Network reply for immediately failed requests
///
class ORIGIN_PLUGIN_API OfflineModeReply : public QNetworkReply
{
public:
    OfflineModeReply(QNetworkAccessManager::Operation op, const QNetworkRequest &req, NetworkError networkError = QNetworkReply::UnknownNetworkError);
    void abort();
private:
    qint64 readData(char *data, qint64 maxSize);
};

}

}


#endif // OFFLINE_MODE_REPLY_H
