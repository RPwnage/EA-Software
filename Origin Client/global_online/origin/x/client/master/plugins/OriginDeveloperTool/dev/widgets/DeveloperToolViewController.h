/////////////////////////////////////////////////////////////////////////////
// DeveloperToolViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef DEVELOPER_TOOL_VIEW_CONTROLLER_H
#define DEVELOPER_TOOL_VIEW_CONTROLLER_H

#include <QObject>
#include "PageErrorDetector.h"
#include "WebWidget/WidgetView.h"
#include "services/plugin/PluginAPI.h"


namespace Origin
{

namespace UIToolkit
{
class OriginWindow;
class OriginWebView;
}

namespace Services
{
class Setting;
}
    
namespace Plugins
{

namespace DeveloperTool
{

    /// \brief Controller for ODT window. Helps load ODT web window,
    /// display, and log debug info.
    class DeveloperToolViewController : public QObject
    {
        Q_OBJECT

    public:

        enum CloseMode
        {
            CloseWindow_PromptUser,
            CloseWindow_Force
        };

        /// \brief Creates the current DeveloperToolViewController instance.
        static void init() { sInstance = new DeveloperToolViewController(); }

        /// \brief Destroys the current DeveloperToolViewController instance.
        static void destroy() { delete sInstance; sInstance = NULL; }
            
        /// \brief Returns the current DeveloperToolViewController instance.
        /// \return The current DeveloperToolViewController instance.
        static DeveloperToolViewController* instance();

        Q_INVOKABLE void queuedDestroy() { destroy(); }

        /// \brief Shows the ODT window.
        Q_INVOKABLE void show();

    public slots:
        /// \brief Closes the ODT window.
        /// \param mode Whether the user should be prompted to confirm the closing of the window.
        bool close(const CloseMode mode = CloseWindow_PromptUser);

        /// \brief Closes the ODT window.
        /// Identical to the other close method but takes an int to be friendlier
        /// with Qt's meta type system. I had issues getting a simple enum
        /// type to register with the Qt meta system, so this method simply
        /// casts the INT value to the CloseMode enum type and calls close.
        bool close(const int closeMode);
            
        /// \brief Prompts the user and restarts the client.
        void restart();

    protected slots:
        /// \brief Closes the ODT window.
        void closeDeveloperTool();

        /// \brief Called when the page title has changed.
        void pageTitleChanged(const QString& newTitle);

    private:
        /// \brief Constructor
        /// \param parent The parent of the DevTool - shouldn't be used.
        DeveloperToolViewController(QWidget* parent = 0);

        /// \brief Destructor
        ~DeveloperToolViewController();

        Q_DISABLE_COPY(DeveloperToolViewController);

        UIToolkit::OriginWindow* mDeveloperToolWindow;
        UIToolkit::OriginWebView* mWebViewContainer;
        WebWidget::WidgetView* mWebView;

        static DeveloperToolViewController* sInstance;
    };

} // namespace DeveloperTool

} // namespace Plugins

} // namespace Origin

#endif
