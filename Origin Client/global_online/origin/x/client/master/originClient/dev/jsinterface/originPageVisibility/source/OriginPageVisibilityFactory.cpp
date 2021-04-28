#include "OriginPageVisibilityFactory.h"

#include <QAbstractScrollArea>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKitWidgets/QWebPage>

#include "OriginPageVisibilityProxy.h"
#include "ExposedWidgetDetector.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

WebWidget::NativeInterface OriginPageVisibilityFactory::createNativeInterface(QWebFrame *frame, const WebWidget::FeatureParameters &) const
{
    ExposedWidgetDetector *detector = NULL;
    
    QWebPage *page = frame->page();
    QAbstractScrollArea *maybeScroll;
    QWidget *maybeWidget;
    
    // This is awful
    // Try to work back how we're being shown from just the page
    // If we can find anything return an empty detector that always returns visible
    if ((maybeScroll = dynamic_cast<QAbstractScrollArea*>(page->parent())))
    {
        detector = new ExposedWidgetDetector(maybeScroll);
    }
    else if ((maybeWidget = dynamic_cast<QWidget*>(page->parent())))
    {
        detector = new ExposedWidgetDetector(maybeWidget);
    }

    // Return a page-specific proxy
    OriginPageVisibilityProxy *proxy = new OriginPageVisibilityProxy(detector, page);
    return WebWidget::NativeInterface("originPageVisibility", proxy, false);
}

}
}
}
