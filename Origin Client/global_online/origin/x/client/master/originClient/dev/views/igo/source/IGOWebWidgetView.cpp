#include "IGOWebWidgetView.h"

#include <QtWebKitWidgets/QGraphicsWebView>
#include <QResizeEvent>

#include "WebWidget/WidgetPage.h"
#include "originwebview.h"

namespace Origin
{
namespace Client
{

IGOWebWidgetView::IGOWebWidgetView() :
    ForwardedWidgetPageView(new WebWidget::WidgetPage(this))
{
    // Create a graphics web view
    mWebView = new QGraphicsWebView;
    // Set the page
    mWebView->setPage(widgetPage());
    // Inject scrollbar CSS
    if(mWebView->settings())
    {
        mWebView->settings()->setUserStyleSheetUrl(QUrl("data:text/css;charset=utf-8;base64," +
            Origin::UIToolkit::OriginWebView::getStyleSheet(true, true).toUtf8().toBase64()));
    }
}

IGOWebWidgetView::~IGOWebWidgetView()
{
    if (mWebView)
        mWebView->deleteLater();
    mWebView = NULL;
}

QGraphicsItem* IGOWebWidgetView::item() const
{
    return mWebView;
}
    
void IGOWebWidgetView::resizeEvent(QResizeEvent *e)
{
    // Use us as a size proxy for our actual web view graphics item
    mWebView->setPos(mapToGlobal(QPoint(0, 0)));
    mWebView->resize(e->size());
}
    
} // Client
} // Origin
