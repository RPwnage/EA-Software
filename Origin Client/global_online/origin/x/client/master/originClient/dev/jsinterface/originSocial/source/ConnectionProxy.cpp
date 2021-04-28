#include "ConnectionProxy.h"

#include <services/debug/DebugService.h>
#include <chat/Connection.h>
    
namespace Origin
{
namespace Client
{
namespace JsInterface
{
    ConnectionProxy::ConnectionProxy(QObject *parent, Chat::Connection *proxied) :
        QObject(parent),
        mProxied(proxied)
    {
        ORIGIN_VERIFY_CONNECT(proxied, SIGNAL(connected()), this, SIGNAL(changed()));
        ORIGIN_VERIFY_CONNECT(
                proxied, SIGNAL(disconnected(Origin::Chat::Connection::DisconnectReason)),
                this, SIGNAL(changed()));
    }

    bool ConnectionProxy::established()
    {
        if (!mProxied)
        {
            return false;
        }

        return mProxied->established();
    }
}
}
}
