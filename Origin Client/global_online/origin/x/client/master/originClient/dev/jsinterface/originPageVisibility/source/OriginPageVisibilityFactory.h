#ifndef _ORIGINPAGEVISIBILITYFACTORY_H
#define _ORIGINPAGEVISIBILITYFACTORY_H

#include "WebWidget/NativeInterfaceFactory.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API OriginPageVisibilityFactory : public WebWidget::NativeInterfaceFactory
{
    WebWidget::NativeInterface createNativeInterface(QWebFrame *frame, const WebWidget::FeatureParameters &) const;
};

}
}
}

#endif

