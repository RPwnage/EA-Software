#ifndef _OWNEDPRODUCTSQUERYOPERATIONPROXY_H
#define _OWNEDPRODUCTSQUERYOPERATIONPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include "QueryOperationProxyBase.h"
#include <services/rest/AtomServiceResponses.h>
#include <services/plugin/PluginAPI.h>
#include <QVariant>

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API OwnedProductsQueryOperationProxy : public QueryOperationProxyBase
{
    Q_OBJECT
public:
    OwnedProductsQueryOperationProxy(QObject *parent, quint64 nucleusId);
    Q_INVOKABLE void getUserProducts();

signals:
    void succeeded(QVariantList products);

private slots:
    void onUserGamesSuccess();

    void sendCurrentUserEntitlements();

private:
    void createPayload( const QList<Services::UserGameData> &games );

private:
    quint64 mNucleusID;
};


}
}
}


#endif

