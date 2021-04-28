/////////////////////////////////////////////////////////////////////////////
// NavBarView.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef NAV_BAR_VIEW_H
#define NAV_BAR_VIEW_H

#include <QWidget>

#include "services/plugin/PluginAPI.h"

// forward declares
class QPushButton;

namespace Ui
{
    class NavBarView;
}

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API NavBarView : public QWidget
        {
            Q_OBJECT

        public:

            NavBarView(QWidget *parent);
            ~NavBarView();

            void init();

            //UserArea* getUserArea() { return ui.userArea; }

            void updateMyGamesTab();
            void updateStoreTab();
            void clearTabs(bool setStyleProperty = true);

            void handleUnderAgeMode(bool isUnderAge = false);

            bool eventFilter(QObject* watched, QEvent* e);

            QWidget* getSocialUserArea();
            QLayout* getSocialUserAreaLayout();

        signals:
            
            void showSettings();
            void logout();
            void exit();
            void showMyGames();
            void showStore();
            void addFriend();

        protected slots:

            void onMyGamesClicked();
            void onStoreClicked();

        private:
            void doToolbarButtonClick(const QPoint& pt);
            void checkToolbarMouseDown(const QPoint& pt, bool& myGamesPressed, bool& storePressed);
            void highlightTab(QPushButton* selectedTab, const bool isSelected = true);

        private:
            Ui::NavBarView*         ui;
            bool mMyGamesPressed;
            bool mStorePressed;
        };
    }
}

#endif
