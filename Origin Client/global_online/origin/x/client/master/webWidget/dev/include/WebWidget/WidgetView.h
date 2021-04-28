#ifndef _WEBWIDGET_WIDGETVIEW_H
#define _WEBWIDGET_WIDGETVIEW_H

#include <QtWebKitWidgets/QWebView>
#include <QLocale>
#include <QObject>
#include <QList>

#include "WebWidgetPluginAPI.h"
#include "NativeInterface.h"
#include "ForwardedWidgetPageView.h"

class QWidget;

///
/// Namespace for HTML/CSS/JavaScript widget support
///
namespace WebWidget
{
    class WidgetRepository;
    class WidgetPage;

    ///
    /// Qt widget for displaying web widgets
    ///
    /// This is a Qt widget container for the backend implementation in WidgetPage
    ///
    class WEBWIDGET_PLUGIN_API WidgetView : public QWebView, public ForwardedWidgetPageView
    {
        Q_OBJECT
    public:
        ///
        /// Creates a new widget container
        ///
        /// \param  parent  Parent widget of the container
        ///
        WidgetView(QWidget *parent = NULL);
        virtual ~WidgetView();
    };
}

#endif
