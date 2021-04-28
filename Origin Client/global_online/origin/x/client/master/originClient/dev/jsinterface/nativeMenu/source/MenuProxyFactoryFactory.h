#ifndef _MENUPROXYFACTORYFACTORY_H
#define _MENUPROXYFACTORYFACTORY_H

#include "WebWidget/NativeInterfaceFactory.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {

            class ORIGIN_PLUGIN_API MenuProxyFactoryFactory : public WebWidget::NativeInterfaceFactory
            {
                WebWidget::NativeInterface createNativeInterface(QWebFrame *frame, const WebWidget::FeatureParameters &) const;
            };

        }
    }
}

#endif

