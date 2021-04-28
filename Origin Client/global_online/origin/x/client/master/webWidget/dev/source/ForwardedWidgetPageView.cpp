#include "ForwardedWidgetPageView.h"
#include "WidgetPage.h"

namespace WebWidget
{
    ForwardedWidgetPageView::ForwardedWidgetPageView(WidgetPage *page) : 
        mWidgetPage(page)
    {
    }

    bool ForwardedWidgetPageView::loadWidget(WidgetRepository *repository, const QString &widgetName, const WidgetLink &link, const QLocale &locale)
    {
        return mWidgetPage->loadWidget(repository, widgetName, link, locale);
    }
    
    bool ForwardedWidgetPageView::loadWidget(const InstalledWidget &widget, const WidgetLink &link, const QLocale &locale)
    {
        return mWidgetPage->loadWidget(widget, link, locale);
    }

    bool ForwardedWidgetPageView::loadLink( const WidgetLink &link, bool isForceReload /*= false*/ )
    {
        return mWidgetPage->loadLink(link, isForceReload);
    }

    WidgetPage* ForwardedWidgetPageView::widgetPage() const
    {
        return mWidgetPage;
    }
}
