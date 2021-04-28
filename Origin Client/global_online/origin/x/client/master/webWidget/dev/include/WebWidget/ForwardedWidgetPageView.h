#ifndef _FORWARDEDWIDGETPAGE_H
#define _FORWARDEDWIDGETPAGE_H

#include <QLocale>

#include "WebWidgetPluginAPI.h"
#include "WidgetLink.h"

class QWidget;
class QString;

namespace WebWidget
{
    class WidgetPage;
    class InstalledWidget;
    class WidgetRepository;

    ///
    /// Forwards functions on an internal widget page to a public interface
    ///
    /// This is intended for classes deriving from QWebView or QGraphicsWebView to additionally inherit from
    /// this class to forward functions on the WidgetPage to the WebView subclass. This function forwarding is
    /// analogous to the function forwarding on QWebView to QWebPage
    ///
    class WEBWIDGET_PLUGIN_API ForwardedWidgetPageView
    {
    public:
        ///
        /// Creates a forwarded page view forwarded the passed page
        ///
        /// \param page  WidgetPage to forward. Ownership remains with the caller.
        ///
        ForwardedWidgetPageView(WidgetPage *page);

        /// \copydoc WidgetPage::loadWidget(WidgetRepository *,const QString &,const WidgetLink &,const QLocale &)
        bool loadWidget(WidgetRepository *repository, const QString &widgetName, const WidgetLink &link = WidgetLink(), const QLocale &locale = QLocale());
        
        /// \copydoc WidgetPage::loadWidget(const InstalledWidget &,const WidgetLink &,const QLocale &)
        bool loadWidget(const InstalledWidget &widget, const WidgetLink &link = WidgetLink(), const QLocale &locale = QLocale());

        /// \copydoc WidgetPage::loadLink
        bool loadLink(const WidgetLink &link, bool isForceReload = false);

        ///
        /// Returns a pointer to the underlying WidgetPage
        ///
        WidgetPage *widgetPage() const;
    
    private:
        WidgetPage *mWidgetPage;
    };
}

#endif

