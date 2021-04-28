#ifndef _IGOWEBBROWSERCONTEXTMENU_H
#define _IGOWEBBROWSERCONTEXTMENU_H

#include <QVariant>
#include <QPointer>

#include "engine/igo/IIGOWindowManager.h"
#include "services/plugin/PluginAPI.h"

class QMenu;
class QAction;
class QLineEdit;

namespace Origin
{
namespace Client
{
    class IGOWebBrowser;

class ORIGIN_PLUGIN_API IGOWebBrowserContextMenu: public QObject, public Origin::Engine::IIGOWindowManager::IContextMenuListener
{
    Q_OBJECT

    public:
        IGOWebBrowserContextMenu(QWidget* window, IGOWebBrowser *browser, QLineEdit* urlText = NULL);
        ~IGOWebBrowserContextMenu();

        void popup();

    private slots:

        // IContextMenuListener impl
        virtual void onFocusChanged(QWidget* widget, bool hasFocus);
        virtual void onDeactivate();

        void hiding();
        void openLinkInNewWindow();
        void openLinkInNewTab();
        void copyLinkAddress();

        void linkHovered(const QString & link, const QString & title, const QString & textContent);

        void initMenuActions();

        void onCopyUrlText();
        void onPasteUrlText();

    private:
        QPointer<QMenu> mNativeMenu;
        QWidget *mWindow;
        IGOWebBrowser *mBrowser;
    
        QAction *mActionLinkTab;
        QAction *mActionLinkWindow;
        QAction *mActionCopyLinkAddress;
        QAction *mActionCopy;
        QAction *mActionCopyUrlText;
        QAction *mActionPaste;
        QAction *mActionPasteUrlText;

        QString mCurrentLinkHovered;
        QString mLinkHovered;
        QLineEdit* mUrlText;
};

} // Client
} // Origin

#endif
