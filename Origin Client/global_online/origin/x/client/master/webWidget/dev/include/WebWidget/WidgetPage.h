#ifndef _WEBWIDGET_WIDGETPAGE_H
#define _WEBWIDGET_WIDGETPAGE_H

#include <QtWebKitWidgets/QWebPage>
#include <QLocale>
#include <QObject>
#include <QList>
#include <QNetworkAccessManager>

#include "WebWidgetPluginAPI.h"
#include "InstalledWidget.h"
#include "NativeInterface.h"
#include "SecurityOrigin.h"
#include "WidgetConfiguration.h"
#include "WidgetLink.h"

class QWebFrame;

namespace WebWidget
{
    class WidgetRepository;
    class InstalledWidget;
    class WidgetPackageReader;
    class NativeInterfaceInjector;

    ///
    /// QWebPage container for executing web widgets
    ///
    /// This contains the backend implementation of WidgetView. This class can be used to directly to render
    /// web widgets on different views or hide the web widget from the user entirely.
    ///
    class WEBWIDGET_PLUGIN_API WidgetPage : public QWebPage
    {
        Q_OBJECT
    public:
        ///
        /// Creates a new widget page
        ///
        /// \param  parent  Parent object of the container
        ///
        WidgetPage(QObject *parent = NULL);
        virtual ~WidgetPage();

        ///
        /// Loads the named widget
        ///
        /// \param  repository  Widget repository to display the widget from
        /// \param  widgetName  Name of the widget to load
        /// \param  link        Initial widget link to load. If none is specified then the widget's configured
        ///                     default start file will be used.
        /// \param  locale      Locale to show the widget in. Defaults to the current locale
        /// \return True if the widget is loading, false if a problem was encountered during widget setup
        ///
        bool loadWidget(WidgetRepository *repository, const QString &widgetName, const WidgetLink &link = WidgetLink(), const QLocale &locale = QLocale());
        
        ///
        /// Loads an installed widget
        ///
        /// \param  widget  Widget to load
        /// \param  link    Initial widget link to load. If none is specified then the widget's configured default
        ///                 start file will be used.
        /// \param  locale  Locale to show the widget in. Defaults to the current locale
        /// \return True if the widget is loading, false if a problem was encountered during widget setup
        ///
        bool loadWidget(const InstalledWidget &widget, const WidgetLink &link = WidgetLink(), const QLocale &locale = QLocale());

        ///
        /// Loads a widget link defined the widget's configuration document
        ///
        /// If the resolved link matches the current widget URL the page will not be reloaded
        ///
        /// \param  link  Widget link to load
        /// \param  isForceReload If true, makes the widget force the reload.
        /// \return True if a link matching the role URI was found and loading has begun
        ///
        /// \sa WidgetConfiguration::links
        /// \sa UriTemplate
        ///
        bool loadLink(const WidgetLink &link, bool isForceReload = false);

        ///
        /// Sets the network access manager used to access resources outside the widget package
        ///
        /// This is used to serve HTTP and HTTPS URIs authorized by the widget's access request policy.
        /// By default external network access is disabled - only data: and widget:// URIs are accessible.
        ///
        /// It is not possible to change external network access managers after a widget has been loaded
        ///
        /// \param manager  Network access manager to use for external resources
        ///
        /// \sa externalNetworkAccessManager
        ///
        void setExternalNetworkAccessManager(QNetworkAccessManager *manager);

        ///
        /// Returns the widget configuration for the currently loaded widget
        ///
        /// \return Widget configuration for the currently loaded widget. Will be null if no widget is loaded
        ///
        WidgetConfiguration widgetConfiguration() const;

        ///
        /// Returns the current external network access manager
        ///
        /// \return Current network access manager or NULL if external network access is disabled
        ///
        /// \sa setExternalNetworkAccessManager
        ///
        QNetworkAccessManager *externalNetworkAccessManager() const;

        ///
        /// Adds a page-specific native interface
        ///
        /// This must be called before loadWidget()
        ///
        /// Page specific interfaces are intended for situations where a widget requires direct communication with its
        /// C++ container. This creates a tight binding between the widget and C++ and is discouraged except where
        /// absolutely necessary.
        ///
        /// See NativeInterfaceRegistry for registering interfaces that widgets can explicitly request.
        ///
        void addPageSpecificInterface(const NativeInterface &);

        /// \brief returns the start URL for the profile
		QUrl startUrl() const;

    public slots:
        ///
        /// Called if a long running JavaScript is detected
        ///
        /// Qt's default implementation prompts the user with a QMessageBox. This is overriden to never interrupt the 
        /// script. The reasons for this are:
        /// - Web Widgets are inherently trusted and any excessive script execution is likely to be transient
        /// - Users are typically not aware of the web technology basis of web widgets and would be confused by the
        ///   message
        /// - Terminating widget scripts could cause the widget to malfunction
        ///
        /// Subclasses may override this function again to redefine its behaviour
        ///
        /// \return True to terminate the long running script, false to allow it to continue
        ///
        virtual bool shouldInterruptJavaScript();

    private slots:
        void onFrameCreated(QWebFrame *);

    private:
        void resetWidgetContext();

        ///
        /// Returns a widget:// URL for a widget link
        ///
        /// \return Resolved QUrl or an empty QUrl if an error occurred
        ///
        QUrl resolveWidgetLink(const WidgetLink &, const QUrl &baseUrl) const;
    
        /// 
        /// Create a native interface injector satisfying the widget's feature requests and including the page specific
        /// interfaces
        ///
        NativeInterfaceInjector* createInterfaceInjectorForFrame(QWebFrame *frame);

        SecurityOrigin mSecurityOrigin;
        WidgetConfiguration mWidgetConfiguration;

        QNetworkAccessManager *mExternalNetworkAccessManager;
        
        QList<NativeInterface> mPageSpecificInterfaces;
        NativeInterfaceInjector *mMainFrameInjector;

        /// \brief stores the start URL for the profile
		QUrl mStartUrl;
	};
}

#endif
