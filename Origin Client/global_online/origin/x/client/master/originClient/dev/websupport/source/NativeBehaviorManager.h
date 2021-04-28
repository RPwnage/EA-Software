#ifndef _NATIVEBEHAVIORMANAGER_H
#define _NATIVEBEHAVIORMANAGER_H

#include <QObject>
#include <QPointer>

#include "services/plugin/PluginAPI.h"

class QWebPage;
class QWebView;
class QWidget;
class QPoint;
class QEvent;
class QMenu;
class QKeyEvent;
class QContextMenuEvent;

namespace Origin
{
namespace Client
{
///
/// /brief Masks certain web-like behaviors on the passed QWidget and QWebPage 
/// 
/// This has the following effects:
/// - Navigation/reload/etc page actions are suppressed. Only text editing actions are allowed
/// - Scrollbars will use native styling
/// - The backspace key won't cause page navigation
/// - Control-U will copy the current URL to the clipboard when web debug is enabled
/// - Inspect will be allowed when web debug is enabled
///
class ORIGIN_PLUGIN_API NativeBehaviorManager : public QObject
{
    Q_OBJECT
public:
    ///
    /// \brief Applies native behavior tor the passed view and page
    ///
    /// \param  viewWidge   The Widget backing the QWebPage.
    /// \param  page        QWebPage being displayed by the viewWidget.
    ///
    static void applyNativeBehavior(QWidget *viewWidge, QWebPage *page);

    ///
    /// \brief Applies native behavior tor the passed QWebView
    ///
    /// \param  webView  QWebView to apply native behavior to
    ///
    static void applyNativeBehavior(QWebView *webView);
    
    ///
    /// \brief Removes native behavior from all active web views.
    ///
    static void shutdown();

    ///
    /// \brief Removes native behavior from the associated web view.
    ///
    void removeNativeBehavior();

protected slots:
    void showBusyCursor();
    void restoreCursor(bool ok);

protected:
    NativeBehaviorManager(QWidget *viewWidget, QWebPage *page);

    void updateMenuActions();
    
    bool eventFilter(QObject *, QEvent *);
    bool filterKeyPressEvent(QKeyEvent *);
    void handleContextMenuEvent(QContextMenuEvent *);

    QPointer<QWidget> mViewWidget;
    QWebPage *mPage;

    static QList<QPointer<NativeBehaviorManager> > sInstances;
};

}
}

#endif
