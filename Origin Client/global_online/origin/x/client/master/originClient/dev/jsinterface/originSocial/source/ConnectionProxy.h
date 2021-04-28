#ifndef _CONNECTIONPROXY_H
#define _CONNECTIONPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QPointer>

#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Chat
{
    class Connection;
}

namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API ConnectionProxy : public QObject
{
    Q_OBJECT
public:
    ConnectionProxy(QObject *parent, Chat::Connection *proxied);

    Q_PROPERTY(bool established READ established);
    bool established();

signals:
    void changed();

private:
    QPointer<Chat::Connection> mProxied;
};

}
}
}

#endif

