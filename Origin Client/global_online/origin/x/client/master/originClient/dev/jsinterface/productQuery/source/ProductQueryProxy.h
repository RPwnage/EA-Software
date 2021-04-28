#ifndef _PRODUCTQUERYPROXY_H
#define _PRODUCTQUERYPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QString>
#include <QStringList>
#include <QPointer>
#include <QMap>

#include "services/session/AbstractSession.h"
#include "services/plugin/PluginAPI.h"

#include "ProductQueryOperationProxy.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API ProductQueryProxy : public QObject
{
    Q_OBJECT
public: 
    Q_INVOKABLE QObject* startQuery(QString productId);
    Q_INVOKABLE QObject* startQuery(QStringList productIds);

    Q_INVOKABLE bool userOwnsProductId(QString productId);
    Q_INVOKABLE bool userOwnsMultiplayerId(QString multiplayerId);

private:
    QMap<QString, QPointer<ProductQueryOperationProxy>> mOperations; 
};

}
}
}

#endif

