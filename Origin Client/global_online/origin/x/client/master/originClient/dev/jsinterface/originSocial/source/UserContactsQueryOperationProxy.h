#ifndef _USERCONTACTSQUERYOPERATIONPROXY_H
#define _USERCONTACTSQUERYOPERATIONPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include "QueryOperationProxyBase.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class OriginSocialProxy;

class ORIGIN_PLUGIN_API UserContactsQueryOperationProxy : public QueryOperationProxyBase
{
    Q_OBJECT
public:
    UserContactsQueryOperationProxy(QObject *parent, OriginSocialProxy *socialProxy, quint64 nucleusId);
    Q_INVOKABLE void getUserFriends();

signals:
    void succeeded(QObjectList contacts);

private slots:
    void onUserFriendsSuccess();
    void sendCurrentUserRoster();

private:
    OriginSocialProxy *mSocialProxy;
    quint64 mNucleusID;
};


}
}
}

#endif

