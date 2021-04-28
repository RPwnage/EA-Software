#include "OnlineStatusProxyFactory.h"
#include "OnlineStatusProxy.h"
#include <services/debug/DebugService.h>

namespace Origin
{
namespace Client
{
namespace JsInterface
{
    
WebWidget::NativeInterface OnlineStatusProxyFactory::createNativeInterface(QWebFrame *, const WebWidget::FeatureParameters &params) const
{
    bool canRequestState = false;

    // Does the widget want to be able to request to go offline and online?
    if (params.find("canRequestState", "true") != params.constEnd())
    {
        canRequestState = true;
    }

    OnlineStatusProxy *proxy = new OnlineStatusProxy(canRequestState);
    return WebWidget::NativeInterface("onlineStatus", proxy, true);
}

}
}
}
