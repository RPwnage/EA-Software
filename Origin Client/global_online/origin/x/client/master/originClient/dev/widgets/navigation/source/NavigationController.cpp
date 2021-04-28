/////////////////////////////////////////////////////////////////////////////
// NavigationController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "NavigationController.h"
#include "services/log/LogService.h"
#include "services/Debug/DebugService.h"
#include "services/settings/SettingsManager.h"

namespace Origin
{
    namespace Client
    {

        int NavigationController::Navi::mIndex = -1;

        namespace
        {
            bool isDebugNavigation = false;
            bool isDebugWebHistories = false;
            QMap<NavigationItem,QString> navigationItemsLabels;
            NavigationController *gNavManager = NULL;

            void cleanUrl(QString& myUrlStr)
            {
                if (myUrlStr.endsWith('#'))
                {
                    myUrlStr.chop(1);
                }

            }

            bool isProfileURL(QString url, QString& id)
            {
                if (url.startsWith("widget://profile"))
                {
                    cleanUrl(url);
                    QUrl myUrl(url);
                    QUrlQuery myQuery(myUrl);

                    if (myQuery.hasQueryItem("id"))
                    {
                        id = myQuery.queryItemValue("id");
                        return !id.isEmpty();
                    }
                }
                return false;
            }

            bool isAchievementURL(QString url)
            {
                if (url.startsWith("widget://achievements") && !url.endsWith("index.html"))
                {
                    cleanUrl(url);
                    QUrl myUrl(url);
                    QUrlQuery myQuery(myUrl);
                    return (myQuery.hasQueryItem("id") && myQuery.hasQueryItem("userid") && myQuery.hasQueryItem("gameTitle"));
                }
                return false;
            }
        }

        int NavigationController::Navi::nextIndex( Direction direction,const QList<Navi>& navi )
        {
            if (BACKWARDS==direction)
            {
                if (Navi::mIndex > -1)
                {
                    return Navi::mIndex - 1;
                }
            }
            else
            {
                if (Navi::mIndex < navi.size()-1)
                {
                    return Navi::mIndex + 1;
                }
            }
            return Navi::mIndex;
        }

        NavigationController::Navi::Navi( NavigationItem item, const QString& url ) : mItem(item), mUrl(url)
        {
            cleanUrl(mUrl);
        }

        bool NavigationController::Navi::operator==( const Navi& navi ) const
        {
            return navi.mItem == mItem && navi.mUrl == mUrl;
        }

        NavigationController* NavigationController::instance()
        {
            if (gNavManager == NULL)
                gNavManager = new NavigationController();

            return gNavManager;
        }

        NavigationController::NavigationController(QObject* parent)
            : QObject(parent)
            ,mCurrentItem(NO_TAB)
            ,mIsGlobalNaviBtnPressed(false)
            ,mEnabled(false)
            ,mStartupTab(TabNone)
            ,mWebHistoryDirection(NONE)
        {
            setEnabled(false);

            QString gNavigationDebugging(Origin::Services::readSetting(Origin::Services::SETTING_NavigationDebugging));
            isDebugNavigation = gNavigationDebugging.contains("navigation",Qt::CaseInsensitive);
            isDebugWebHistories = gNavigationDebugging.contains("webhistor",Qt::CaseInsensitive);
            if (navigationItemsLabels.size() == 0)
            {
                navigationItemsLabels[NO_TAB]                       = "NO_TAB";
                navigationItemsLabels[MY_GAMES_TAB]                 = "MY_GAMES_TAB";
                navigationItemsLabels[STORE_TAB]                    = "STORE_TAB";
                navigationItemsLabels[SOCIAL_TAB]                   = "SOCIAL_TAB";
                navigationItemsLabels[MY_ORIGIN_TAB]                = "MY_ORIGIN_TAB";
                navigationItemsLabels[ACHIEVEMENT_TAB]              = "ACHIEVEMENT_TAB";
                navigationItemsLabels[SETTINGS_ACCOUNT_TAB]         = "SETTINGS_ACCOUNT_TAB";
                navigationItemsLabels[SETTINGS_APPLICATION_TAB]     = "SETTINGS_APPLICATION_TAB";
                navigationItemsLabels[FEEDBACK_TAB]                 = "FEEDBACK_TAB";
                navigationItemsLabels[SEARCH_CONTENT]               = "SEARCH_CONTENT";
            }

        }

        NavigationController::~NavigationController()
        {
            clearHistory();
        }

        void NavigationController::setEnabled(bool enabled, NavigationItem item) 
        { 
            if(!enabled)
            {
                emit canGoBackChange(false);
                emit canGoForwardChange(false);
            }
            else
            {
                if (mStartupTab == TabMyGames && (item == STORE_TAB || item == MY_ORIGIN_TAB))
                {
                    return;
                }
                else if (mStartupTab == TabStore && (item == MY_ORIGIN_TAB || item == MY_GAMES_TAB))
                {
                    return;
                }
                setGlobalNaviButtons();
                emit navigationEnabled();
            }
            mEnabled = enabled; 
        }

        void NavigationController::clearHistory()
        {
            mNavigators.clear();
            mClicked.clear();
            mCurrentItem = NO_TAB;
            mNavi.clear();
            setGlobalNaviButtons(); 
            NavigationController::Navi::mIndex = -1;
        }

        void NavigationController::CheckForProfileId(QString url )
        {
            QString id;
            if (isProfileURL(url, id))
            {
                emit currentId(id);
            }
        }

        void NavigationController::historyNavigate(const Navi& navi ,  Direction direction )
        {
            ORIGIN_LOG_EVENT_IF(isDebugNavigation) << "[-_-_-_-_-] History navigating for Tab: [" << navigationItemsLabels[mCurrentItem] << "]";
            if (navi.mItem == SOCIAL_TAB)
            {
                CheckForProfileId(navi.mUrl);
            } else if (navi.mItem == ACHIEVEMENT_TAB)
            {
                emit loadUrl(navi.mUrl);
            } 
            else
            {
                if (BACKWARDS==direction)
                {
                    mNavigators[navi.mItem]->back();
                } 
                else
                {
                    mNavigators[navi.mItem]->forward();
                }
            }
        }

        void NavigationController::navigation(Direction direction)
        {
            if (!mEnabled || mCurrentItem == NO_TAB)
                return;

            mIsGlobalNaviBtnPressed = true;

            Navi::mIndex =  Navi::nextIndex(direction,mNavi);
            Navi navi = mNavi.at(Navi::mIndex);

            if (navi.mItem ==  mCurrentItem)
            {
                historyNavigate(navi, direction);
            } 
            else
            {
                // test to see if the URL we are going to is the correct one.
                QWebHistoryItem hi = mNavigators[navi.mItem]->currentItem();
                if (hi.url().toString() != navi.mUrl)
                {
                    historyNavigate(navi,direction);
                }
                showTab(navi.mItem);
            }
            setGlobalNaviButtons();
        }

        void NavigationController::navigateBackward()
        {
            navigation(BACKWARDS);
        }

        void NavigationController::navigateForward()
        {
            navigation(FORWARDS);
        }

        void NavigationController::recordNavigation(NavigationItem item, const QString& urlStr)
        {
            ORIGIN_LOG_EVENT_IF(isDebugNavigation) << "[--------] RecordNavigation: tab: [" << navigationItemsLabels[item] << "] url [" << urlStr << "]";
            if (!isAllowedRecordNavigation(item, urlStr))
            {
                ORIGIN_LOG_EVENT_IF(isDebugNavigation) << "[warning] Detected navigation from item: [" << navigationItemsLabels[mCurrentItem] << "] to item: [" << navigationItemsLabels[item] << "] when it hasn't been clicked and it is not the startup tab. Bailing.";
                resetNavigation();
                debugNavigation();
                return;
            }

            if (!mEnabled)
            {
                setEnabled(true, item);

                if (!mEnabled)
                {
                    ORIGIN_LOG_EVENT_IF(isDebugNavigation) << "[warning] Detected navigation from item: [" << navigationItemsLabels[mCurrentItem] << "] to item: [" << navigationItemsLabels[item] << "] when NavigationController is disabled. Bailing.";
                    resetNavigation();
                    debugNavigation();
                    return;
                }
            }

            if (mCurrentItem != item)
            {
                if (!mIsGlobalNaviBtnPressed && Navi::mIndex == 0)
                {
                    //  went back and clicked on another tab. start new history
                    resetTabs();
                }

                ORIGIN_LOG_EVENT_IF(isDebugNavigation) << "[information] Detected navigation from item: [" << navigationItemsLabels[mCurrentItem] << "] to item: [" << navigationItemsLabels[item] << "]";
                mCurrentItem = item;
            }

            QWebHistory* currentHistory = mNavigators[mCurrentItem];
            QString currentUrl = urlStr;
            if (currentHistory && !currentHistory->currentItem().url().toString().isEmpty())
                currentUrl = currentHistory->currentItem().url().toString();

            cleanUrl(currentUrl);

            if (!currentUrl.isEmpty())
            {
                if (Navi::mIndex > -1 && (mNavi.at(Navi::mIndex) == Navi(item,currentUrl)))
                {
                    ORIGIN_LOG_EVENT_IF(isDebugNavigation) << "[warning] Detected navigation to the same item: [" << navigationItemsLabels[item] << "] and to the same url [" << currentUrl << "]";
                    resetNavigation();
                    debugNavigation();
                    return;
                }
                if (!mIsGlobalNaviBtnPressed)
                {
                    mNavi.append(Navi(item, currentUrl));
                    Navi::mIndex = mNavi.size() - 1;
                }

            }

            setGlobalNaviButtons();
            resetNavigation();
            debugNavigation();
            return;
        }

        void NavigationController::setStartupTab( StartupTab tab )
        {
            mStartupTab = tab;
            if (mStartupTab == TabMyGames)
            {
                clicked(MY_GAMES_TAB);
            }
            else if (mStartupTab == TabStore)
            {
                clicked(STORE_TAB);
            }

        }

        bool NavigationController::isAllowedRecordNavigation(NavigationItem item, const QString& urlStr) const
        {

            if (item == MY_GAMES_TAB || item == MY_ORIGIN_TAB)
                return mClicked.contains(item);
            if (item == STORE_TAB) 
                return mClicked.contains(item);
            else if (item == ACHIEVEMENT_TAB)
                return mClicked.contains(item) || isAchievementURL(urlStr);
            else
                return mEnabled;
        }

        void NavigationController::clicked( NavigationItem item )
        {
            if (!mClicked.contains(item))
            {
                mClicked.append(item);
            }
        }

        void NavigationController::resetTabs()
        {
            ORIGIN_LOG_EVENT_IF(isDebugNavigation) << "[######] REMOVED TABS.";
            Navi navi = mNavi.at(0);
            mNavi.clear();
            mNavi.append(navi);
            Navi::mIndex = mNavi.size() - 1;
        }

        void NavigationController::showTab( NavigationItem item )
        {
            switch (item)
            {
            case MY_GAMES_TAB:
                {
                    emit (showMyGames(true));
                    // It's possible that the "my games" page hasn't loaded yet
                    // due to waiting on entitlement refresh to complete. If that's
                    // the case, there won't be any history available, so just 
                    // display "my games" (it would have been impossible for a user
                    // to navigate to a games details page at this point anyways).
                    emit (clickGamesLayout());
                }
                break;
            case STORE_TAB:
                {
                    emit (showStore(true));
                    emit (clickStoreLayout());
                }
                break;
            case MY_ORIGIN_TAB:
                {
                    emit (showMyOrigin(false));
                    emit (clickMyOriginLayout());
                }
                break;
            case SOCIAL_TAB:
                {
                    emit (showSocial(true));
                }
                break;
            case SEARCH_CONTENT:
                {
                    emit (showSearchPage(true));
                }
                break;
            case ACHIEVEMENT_TAB:
                {
                    emit (showAchievements(true));
                }
                break;
            case SETTINGS_ACCOUNT_TAB:
                {
                    emit (showAccountSettings(false));
                }
                break;
            case SETTINGS_APPLICATION_TAB:
                {
                    emit (showApplicationSettings(false));
                }
                break;
            case FEEDBACK_TAB:
                {
                    emit (showFeedbackPage(false));
                }
                break;
            default:
                {
                    ORIGIN_LOG_DEBUG << "[error] Navigation item: [" << item << "] unknown.";
                    ORIGIN_ASSERT(false);
                }
                break;
            }
        }

        void NavigationController::addWebHistory(NavigationItem item, QWebHistory* webHistory)
        {
            mNavigators[item] = webHistory;
        }

        void NavigationController::goBackAndRemoveForwardWebHistory(NavigationItem item)
        {
            mNavigators[item]->back();
            mNavigators[item]->forwardItems(1000).clear();
            setGlobalNaviButtons();
        }

        void NavigationController::setGlobalNaviButtons()
        {
            emit canGoBackChange(Navi::mIndex > 0);
            emit canGoForwardChange(Navi::mIndex < mNavi.size() - 1 );
        }

        void NavigationController::achievementHomeChosen()
        {
        }

        void NavigationController::resetNavigation()
        {
            mIsGlobalNaviBtnPressed = false;
        }

        void NavigationController::debugNavigation()
        {
            ORIGIN_LOG_DEBUG_IF(isDebugNavigation) << "[#######]  INDEX [" << Navi::mIndex << "]";
            if (isDebugNavigation)
            {
                for (int i = 0; i < mNavi.size(); ++i)
                {
                    ORIGIN_LOG_DEBUG << (  (Navi::mIndex == i) ? "[-----------] " : "[information] " ) << "[" << i << "] TAB [" << navigationItemsLabels[mNavi.at(i).mItem] << "] URL [" << mNavi.at(i).mUrl << "]";
                }
            }
        }

    }
}
