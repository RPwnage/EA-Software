#ifndef _ONTHEHOUSEQUERYPROXY_H
#define _ONTHEHOUSEQUERYPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QVariant>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API OnTheHouseQueryProxy : public QObject
{
    Q_OBJECT
public:
    OnTheHouseQueryProxy(QObject* parent = 0);
    Q_INVOKABLE void startQuery();

signals:
    void succeeded(QVariant info);
    void failed();
};

}
}
}

#endif

