#include "MenuProxyFactoryFactory.h"

#include <QAbstractScrollArea>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKitWidgets/QWebPage>

#include "MenuProxyFactory.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {

            WebWidget::NativeInterface MenuProxyFactoryFactory::createNativeInterface(QWebFrame *frame, const WebWidget::FeatureParameters &) const
            {
                QWebPage *page = frame->page();

                // Return a page-specific proxy
                MenuProxyFactory *proxy = new MenuProxyFactory(page);
                return WebWidget::NativeInterface("nativeMenu", proxy, false);
            }

        }
    }
}
