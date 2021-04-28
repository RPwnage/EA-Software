#include "WidgetView.h"
#include "WidgetPage.h"

namespace WebWidget
{
    WidgetView::WidgetView(QWidget *parent) : 
        QWebView(parent),
        ForwardedWidgetPageView(new WidgetPage(this))
    {
        setPage(widgetPage());
    }

    WidgetView::~WidgetView()
    {
    }
}
