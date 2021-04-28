#ifndef _OFFLINECDNNETWORKACCESSMANAGER_H
#define _OFFLINECDNNETWORKACCESSMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{

///
/// Simple class to rewrite accesses to the CDN to prefer our local cache instead of the network
///
/// This allows web widgets to show static assets in offline mode
///
class ORIGIN_PLUGIN_API OfflineCdnNetworkAccessManager : public QNetworkAccessManager
{
public:
    OfflineCdnNetworkAccessManager(QObject *parent = NULL); 

protected:
    virtual QNetworkReply *createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *outgoingData);
};

}
}

#endif

