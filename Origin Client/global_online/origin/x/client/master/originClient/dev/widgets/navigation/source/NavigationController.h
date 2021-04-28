/////////////////////////////////////////////////////////////////////////////
// NavigationController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef NavigationController_H
#define NavigationController_H

#include <QUrl>
#include <QStack>
#include <QtWebKitWidgets/QWebFrame>
#include <QtWebKitWidgets/QWebPage>
#include <QtWebKit/QWebHistory>
#include <QHash>
#include "services/settings/StartupTab.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        enum NavigationItem
        {
            NO_TAB = -1,
            MY_GAMES_TAB,
            STORE_TAB,
            SOCIAL_TAB,
            MY_ORIGIN_TAB,
            ACHIEVEMENT_TAB,
            SETTINGS_ACCOUNT_TAB,
            SETTINGS_APPLICATION_TAB,
            FEEDBACK_TAB,
            SEARCH_CONTENT
        };

        enum Direction
        {
            NONE
            ,BACKWARDS
            ,FORWARDS
        };

        class ORIGIN_PLUGIN_API NavigationController : public QObject
        {
            Q_OBJECT

        public:
            // Singleton
            static NavigationController* instance();

            // functions
            bool enabled() { return mEnabled; }
            void setEnabled(bool enabled, NavigationItem item = NO_TAB);
            void clearHistory();
            void addWebHistory(NavigationItem item, QWebHistory* webHistory);
            void goBackAndRemoveForwardWebHistory(NavigationItem item);
            void recordNavigation(NavigationItem item, const QString& url = QString());
            void setStartupTab(StartupTab);
            void clicked(NavigationItem item);

        signals:
            void canGoBackChange(const bool can);
            void canGoForwardChange(const bool can);
            void navigationEnabled();
            void showStore(bool);
            void showMyOrigin(bool);
            void showMyGames(bool);
            void showSocial(bool);
            void currentId(QString);
            void showAchievements(bool);
            void showAccountSettings(bool);
            void showApplicationSettings(bool);
            void showFeedbackPage(bool);
            void showSearchPage(bool);
            void clickMyOriginLayout();
            void clickGamesLayout();
            void clickStoreLayout();
            void loadUrl(QString);

        public slots:

            void navigateBackward();
            void navigateForward();
            // executed when achievements home is chosen.
            void achievementHomeChosen();

        private:

            void debugNavigation();
            void resetNavigation();

            /// \brief erases lower tabs navigation histories
            void resetTabs();

            /// \brief perform the history and tab navigation back and  forth
            void navigation(Direction);

            /// \brief determines whether to allow navigation recording at this stage
            /// \param navigation item trying to be recorded
            /// \param url to parse and compare
            bool isAllowedRecordNavigation(NavigationItem, const QString&) const;

            struct Navi;

            /// \brief navigates tab's histories
            /// \param direction: backwards or forwards
            /// \return true if the navigation was successful
            void historyNavigate(const Navi&, Direction);

            /// \brief shows tab
            /// \param tab to show
            void showTab(NavigationItem item);

            /// \brief gets the current url and defines if it is a profile id URL. 
            /// \brief If so, emits signal to make the profile widget to load it
            void CheckForProfileId(QString);

            /// \brief set global navigation buttons.
            void setGlobalNaviButtons();

            NavigationController(QObject* parent = NULL);
            ~NavigationController();
            Q_DISABLE_COPY(NavigationController)

            NavigationItem mCurrentItem; // contains the current tab

            QMap<NavigationItem, QWebHistory*> mNavigators; // contains the navigation histories

            // true if the global navigation buttons have been pressed
            bool mIsGlobalNaviBtnPressed;

            bool mEnabled; // to enable/disable navigation

            // tab to start the client at
            StartupTab mStartupTab;

            // list of clicked navigation items. It gets filled when the user 
            // clicks on the client to navigate - not using the global navigation arrows
            QList<NavigationItem> mClicked;

            // structure to hold the values of the user's navigation
            struct Navi
            {
                static int mIndex;
                NavigationItem mItem;
                QString mUrl;
                Navi() : mItem(NO_TAB) {}
                Navi(NavigationItem item, const QString& url);
                bool operator==(const Navi& navi) const;

                static int nextIndex(Direction direction, const QList<Navi>& navi);
            };

            QList<Navi> mNavi;
            QList<Navi> navi() const { return mNavi;}

            // direction to navigate to
            Direction mWebHistoryDirection;
        };
    }
}

#endif //NavigationController_H
