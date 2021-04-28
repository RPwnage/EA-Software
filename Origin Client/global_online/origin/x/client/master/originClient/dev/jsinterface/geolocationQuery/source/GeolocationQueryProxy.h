#ifndef _GEOLOCATIONQUERYPROXY_H
#define _GEOLOCATIONQUERYPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QString>
#include <QPointer>
#include <QMap>

#include "GeolocationQueryOperationProxy.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API GeolocationQueryProxy : public QObject
{
    Q_OBJECT
public: 
    Q_INVOKABLE QObject* startQuery(QString ipAddress);

private:
    QMap<QString, QPointer<GeolocationQueryOperationProxy>> mOperations; 
};

}
}
}

#endif

