#ifndef _PRODUCTQUERYOPERATIONPROXY_H
#define _PRODUCTQUERYOPERATIONPROXY_H

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
#include <QVariant>

#include "WebWidget/ScriptOwnedObject.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Engine
{
namespace Content
{
    class ProductInfo;
}
}

namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API ProductQueryOperationProxy : public QObject
{
    Q_OBJECT
public: 
    ProductQueryOperationProxy(QObject *parent, Engine::Content::ProductInfo *info, QString productId);
    ProductQueryOperationProxy(QObject *parent, Engine::Content::ProductInfo *info, QStringList productIds);

signals:
    void succeeded(QVariant info);
    void failed(QVariant ids);

private slots:
    void querySucceeded();
    void queryFailed();

private:
    void attachSignals(); 

    Engine::Content::ProductInfo *mProductInfo;
    QStringList mProductIds;
    bool mQueriedAsArray;
};

}
}
}

#endif

