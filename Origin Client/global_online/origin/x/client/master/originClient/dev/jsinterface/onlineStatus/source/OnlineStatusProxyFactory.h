#ifndef _ONLINESTATUSPROXYFACTORY_H
#define _ONLINESTATUSPROXYFACTORY_H

#include "WebWidget/NativeInterfaceFactory.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API OnlineStatusProxyFactory : public WebWidget::NativeInterfaceFactory
{
    WebWidget::NativeInterface createNativeInterface(QWebFrame *frame, const WebWidget::FeatureParameters &) const;
};

}
}
}

#endif

