#ifndef _SHAREACHIEVEMENTSQUERYOPERATIONPROXY_H
#define _SHAREACHIEVEMENTSQUERYOPERATIONPROXY_H

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

class ORIGIN_PLUGIN_API ShareAchievementsQueryOperationProxy : public QueryOperationProxyBase
{
    Q_OBJECT
public:
    ShareAchievementsQueryOperationProxy(QObject *parent, quint64 nucleusId);

signals:
    void succeeded(bool);

private slots:
    void onSuccess();

};


}
}
}

#endif

