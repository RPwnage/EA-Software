// *********************************************************************
//  TwitchUrlContextMenu.h
//  Copyright (c) 2013 Electronic Arts Inc. All rights reserved.
// **********************************************************************

#ifndef _TWITCHURLCONTEXTMENU_H
#define _TWITCHURLCONTEXTMENU_H

#include <QPointer>
#include <QColor>

#include "engine/igo/IIGOWindowManager.h"
#include "services/plugin/PluginAPI.h"

class QMenu;
class QAction;
class QLabel;


class ORIGIN_PLUGIN_API TwitchUrlContextMenu: public QObject, public Origin::Engine::IIGOWindowManager::IContextMenuListener
{
    Q_OBJECT

    public:
        TwitchUrlContextMenu(QWidget* window, QObject *parent = NULL);
        ~TwitchUrlContextMenu();

        void popup(QLabel *label);

    private slots:

        // IContextMenuListener impl
        virtual void onFocusChanged(QWidget* window, bool hasFocus);
        virtual void onDeactivate();

        void hiding();
        void onCopy();

    private:
        void initMenuActions();
    
    private:
        QPointer<QMenu> mNativeMenu;
        QLabel* mCurrentLabel;
        QWidget* mWindow;

        QAction *mActionCopy;
        QColor mInactiveHighlightColor;
        QColor mInactiveHighlightedTextColor;
};


#endif // _TWITCHURLCONTEXTMENU_H
