#ifndef _LOGINNETWORKACCESSMANAGER_H
#define _LOGINNETWORKACCESSMANAGER_H

#include <QList>
#include <QObject>

#include "services/network/NetworkAccessManager.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{

class ORIGIN_PLUGIN_API LoginNetworkAccessManager : public Services::NetworkAccessManager
{
public:
    LoginNetworkAccessManager(); 

    void setPreferNetwork();
    void setAlwaysUseCache();

protected:
    virtual QNetworkReply* createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &req, QIODevice *outgoingData);

private:

    bool mUseCache;

};

}
}

#endif

