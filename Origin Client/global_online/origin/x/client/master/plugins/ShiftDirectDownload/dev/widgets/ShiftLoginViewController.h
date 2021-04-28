// ShiftLoginViewController.h
// Copyright 2014 Electronic Arts Inc. All rights reserved.

#ifndef SHIFT_LOGIN_VIEW_CONTROLLER_H
#define SHIFT_LOGIN_VIEW_CONTROLLER_H

#include "Qt/originwindow.h"
#include "WebWidget/WidgetView.h"

#include "../jsinterface/ShiftQueryProxy.h"

namespace Origin
{

namespace Plugins
{

namespace ShiftDirectDownload
{

/// \brief Controller for Login window. Helps load Login web window,
/// display, and log debug info.
class ShiftLoginViewController : public QObject
{
	Q_OBJECT

public:
    /// \brief Creates the current LoginViewController instance.
    static void init() { sInstance = new ShiftLoginViewController(); }

    /// \brief Destroys the current LoginViewController instance.
    static void destroy();

    /// \brief Returns the current LoginViewController instance.
    /// \return The current LoginViewController instance.
    static ShiftLoginViewController* instance();

    /// \brief Shows the window.
	void show();

public slots:
    /// \brief Closes the window.
    bool close();

protected slots:
	/// \brief Called when the page title has changed.
    void pageTitleChanged(const QString& newTitle);

private:
    /// \brief Constructor
    /// \param parent The parent of the widget - shouldn't be used.
    ShiftLoginViewController(QWidget* parent = 0);

    /// \brief Destructor
	~ShiftLoginViewController();

    Q_DISABLE_COPY(ShiftLoginViewController);

    UIToolkit::OriginWindow* mLoginWindow;
    UIToolkit::OriginWebView* mWebViewContainer;
    WebWidget::WidgetView* mWebView;

    static ShiftLoginViewController* sInstance;
};

} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin

#endif
