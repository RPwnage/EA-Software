#include "ToastViewController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"
#include "TelemetryAPIDLL.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QWidget>
#include "ToastView.h"
#include "MiniToastView.h"
#include "OriginNotificationDialog.h"
#include "originlabel.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QtMultimedia/QMediaPlayer>
#ifdef ORIGIN_PC
//#include <Windows.h>		//for isFullscreenAppRunning
#include <IAccess.h>
#include "flows/ClientFlow.h"
#endif
#include "chat/OriginConnection.h"
#include "engine/login/User.h"
#include "engine/igo/IGOController.h"
#include "engine/igo/IGOWindowManager.h"
#include "engine/social/SocialController.h"
#include "services/network/NetworkAccessManagerFactory.h"
#include "services/network/NetworkAccessManager.h"


namespace Origin
{
namespace Client
{
namespace 
{
    const int TOAST_MAX_LIMIT = 3;
    const QString OIG_HOTKEY_IDENTIFIER = "POPUP_OIG_HOTKEY";
}

#ifdef ORIGIN_PC
void CALLBACK HandleWinEvent(HWINEVENTHOOK hook, DWORD event, HWND hwnd, 
                             LONG idObject, LONG idChild, 
                             DWORD dwEventThread, DWORD dwmsEventTime)
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }
    
    otm->controller()->HandleWindowEvents(hwnd);
}
#endif

ToastViewController::ToastViewController()
: mNetworkManager(NULL)
, mActiveIGOToast(NULL)
, mVoiceChatCounterToast(NULL)
, mUserVoiceChatToast(NULL)
, mFakeOIG(false)
#ifdef ORIGIN_PC
, g_foregroundWindow(NULL)
, g_hook(NULL)
, fullScreenApp(NULL)
, mMediaPlayer(NULL)
#endif
{
    // Dear future engineers in this class,
    // If a thought passes in your mind to set up the signals to show a toast 
    // here instead of inside your widget DONT. I WILL FIND YOU!
    // All this class is doing is creating and managing the view of the toasts. It's not for handling the result of
    // interacting with the toast. Since each case is different it needs to be handled in a unique area. NOT HERE.
    // Thank you. Remember: I will find you if you mess this class up. I make enough money to fly to where you are.
    // Love, Katherine Winter.

    // Set up our layout manager objects so we can place the notifications later
    mDesktopLayoutManager = QSharedPointer<OriginToastLayoutManager>(new OriginToastLayoutManager(ClientScope, TOAST_MAX_LIMIT, true));
    mIGOLayoutManager = QSharedPointer<OriginToastLayoutManager>(new OriginToastLayoutManager(IGOScope, TOAST_MAX_LIMIT, true));
    mVoiceChatIdentifierLayoutManager = QSharedPointer<OriginToastLayoutManager>(new OriginToastLayoutManager(IGOScope, 4, false));
    mMediaPlayer = new QMediaPlayer();
#ifdef ORIGIN_PC
    SetUpWinHook();
#endif
}


ToastViewController::~ToastViewController()
{
#ifdef ORIGIN_PC
    ShutdownWinHook();
#endif
    ORIGIN_LOG_EVENT << "NotifyPopupPool = " << mToastViewPool.size();

    for(QList<UIToolkit::OriginNotificationDialog*>::iterator iter = mToastViewPool.begin(); iter!=mToastViewPool.end(); ++iter)
    {
        UIToolkit::OriginNotificationDialog* toast = *(iter);
        if (toast)
            delete toast;
    }
    mToastViewPool.clear();

    ORIGIN_LOG_EVENT << "IGOPopupPool = " << mIGOToastViewPool.size();

    for(QList<UIToolkit::OriginNotificationDialog*>::iterator iter = mIGOToastViewPool.begin(); iter!=mIGOToastViewPool.end(); ++iter)
    {
        UIToolkit::OriginNotificationDialog* toast = *(iter);
        if (toast)
            delete toast;
    }
    mIGOToastViewPool.clear();

    ORIGIN_LOG_EVENT << "VoiceChatIdentifierPool = " << mVoiceChatIdentifierPool.size();

    for(QList<UIToolkit::OriginNotificationDialog*>::iterator iter = mVoiceChatIdentifierPool.begin(); iter!=mVoiceChatIdentifierPool.end(); ++iter)
    {
        UIToolkit::OriginNotificationDialog* toast = *(iter);
        if (toast)
            delete toast;
    }
    mVoiceChatIdentifierPool.clear();

    ORIGIN_LOG_EVENT << "Completed pool cleanup.";

    if (mNetworkManager)
        mNetworkManager->deleteLater();

    if (mDesktopLayoutManager)
        mDesktopLayoutManager->deleteLater();

    if (mIGOLayoutManager)
        mIGOLayoutManager->deleteLater();

    if (mVoiceChatIdentifierLayoutManager)
        mVoiceChatIdentifierLayoutManager->deleteLater();

    if (mMediaPlayer)
        mMediaPlayer->deleteLater();
}

UIToolkit::OriginNotificationDialog* ToastViewController::showToast(OriginToastManager::ToastSetting settings, OriginToastManager::ToastBehavior behavior, const QString& name, const QString& title, const QString& message, const QString& pixmapStr, bool iconNeedsLoading, QVector<QWidget*> customWidgets, UIScope scope)
{
    UIToolkit::OriginNotificationDialog* toast = showToast(settings, behavior, name, title, message, QPixmap(pixmapStr), customWidgets, scope);
    if (iconNeedsLoading)
    {
        mUrlToastViewMap.insert(QUrl(pixmapStr), toast);
        if (mNetworkManager == NULL) 
        {
            mNetworkManager = Services::NetworkAccessManagerFactory::instance()->createNetworkAccessManager();
            ORIGIN_VERIFY_CONNECT(mNetworkManager, SIGNAL(finished(QNetworkReply*)),this, SLOT(onUrlLoaded(QNetworkReply*)));
        }
        mNetworkManager->get(QNetworkRequest(QUrl(pixmapStr)));
        // wait till we get the icon to display.
    }
    else 
    {
        showToast(toast, scope);
    }
    return toast;
}

UIToolkit::OriginNotificationDialog* ToastViewController::showToast(OriginToastManager::ToastSetting settings, OriginToastManager::ToastBehavior behavior, const QString& name, const QString& title, const QString& message, const QPixmap& pixmap, QVector<QWidget*> customWidgets, UIScope scope)
{
    UIToolkit::OriginNotificationDialog* toast = NULL;
    if(settings.alertMethod() == OriginToastManager::ToastSetting::TOAST_SOUND_ONLY || settings.alertMethod() == OriginToastManager::ToastSetting::TOAST_SOUND_ALERT)
    {
        playSound(settings.soundPath());
    }
    if(settings.alertMethod() == OriginToastManager::ToastSetting::TOAST_ALERT_ONLY || settings.alertMethod() == OriginToastManager::ToastSetting::TOAST_SOUND_ALERT)
    {
        toast = requestToast(name, scope);

        ToastView* view = new ToastView(scope);
        view->setIcon(pixmap.isNull() ? QPixmap(settings.iconPath()) : pixmap);
        view->setTitle(title);
        view->setMessage(message);
        view->setName(name);

        view->setupCustomWidgets(customWidgets, toast);
        view->handleHotkeyLables();

        toast->setMaxOpacity(behavior.maxOpacity());
        toast->showBubble(behavior.showBubble());
        toast->setIGOFlags(behavior.IGOFlags());
        toast->shouldAcceptClick(behavior.acceptClick());
        toast->setCloseTimer(behavior.closeTime());

        toast->setContent(view);
        toast->setInitalIGOMode(scope == IGOScope);
        ORIGIN_VERIFY_CONNECT(Origin::Engine::IGOController::instance(), SIGNAL(stateChanged(bool)), toast, SLOT(setIGOMode(const bool&)));
        ORIGIN_VERIFY_CONNECT(view, SIGNAL(closeClicked()), toast, SLOT(onActionRejected()));
        ORIGIN_VERIFY_CONNECT(view, SIGNAL(clicked()), toast, SLOT(windowClicked()));
        ORIGIN_VERIFY_CONNECT(toast, SIGNAL(igoActivated()), this, SLOT(onIGOActivated()));

        if(view->context() == IGOScope && !mActiveIGOToast)
        {
            mActiveIGOToast = toast;
        }

        // Do not show Achievements, this will be handled later
        if (!behavior.delayShow())
        {
            if(behavior.toastType())
            {
                showToast(toast, scope, behavior.toastType());
            }
            else
                showToast(toast, scope);
        }
    }
    return toast;
}

void ToastViewController::showMiniToast(OriginToastManager::ToastSetting settings, OriginToastManager::ToastBehavior behavior, int voiceType, const QString& title, const QPixmap& pixmap)
{
    UIToolkit::OriginNotificationDialog* toast = NULL;
    //THIS IS FOR DEBUG PURPOSES
    // These notifications will never have a sound.
    //if(1)
    //{
    //    QSound::play(settings.soundPath());
    //}
    ///
    if(settings.alertMethod() == OriginToastManager::ToastSetting::TOAST_ALERT_ONLY || settings.alertMethod() == OriginToastManager::ToastSetting::TOAST_SOUND_ALERT)
    {
        if (voiceType == OriginToastManager::UserVoiceType_CurrentUserMuted ||
            voiceType == OriginToastManager::UserVoiceType_CurrentUserTalking)
        {
            // The current user is talking
            if (!mUserVoiceChatToast)
            {
                mUserVoiceChatToast = new UIToolkit::OriginNotificationDialog();
                // Summon a toast window
                mUserVoiceChatToast->setObjectName("ToastDialog");
                mUserVoiceChatToast->setAttribute(Qt::WA_DeleteOnClose, false);
                mUserVoiceChatToast->setAttribute(Qt::WA_ShowWithoutActivating);
                mUserVoiceChatToast->setWindowFlags(mUserVoiceChatToast->windowFlags() | Qt::Popup | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput);

                // never show this bubble for these toasts
                mUserVoiceChatToast->showBubble(behavior.showBubble());
                mUserVoiceChatToast->shouldAcceptClick(behavior.acceptClick());
                mUserVoiceChatToast->setIGOFlags(behavior.IGOFlags());
                mUserVoiceChatToast->setCloseTimer(behavior.closeTime());
                // set the fade timer to -1 so we never fade.
                mUserVoiceChatToast->setFadeInTime(-1);
                mUserVoiceChatToast->setInitalOpacity(behavior.maxOpacity());

                // hide the toast so the show actually happens correctly and we get a showEvent
                if (mUserVoiceChatToast->isVisible())
                    mUserVoiceChatToast->hide();

                ORIGIN_VERIFY_CONNECT(mUserVoiceChatToast, SIGNAL(finished(int)), this, SLOT(onToastDone()));
            }
            MiniToastView* view = new MiniToastView();
            view->setIcon(pixmap.isNull() ? QPixmap(settings.iconPath()) : pixmap);
            view->setTitle(title);
            view->setName(title);
            mUserVoiceChatToast->setContent(view);
            showMiniToast(mUserVoiceChatToast, ToastType_UserVoiceIdentifier);

            // Are we showing 3 or more?
            if (visibleVoiceIdentifiers() > TOAST_MAX_LIMIT)
            {
                UIToolkit::OriginNotificationDialog* toast = oldestToast(mVoiceChatIdentifierPool);
                MiniToastView* oldView = dynamic_cast<MiniToastView*>(toast->content());
                if (oldView)
                {
                    QString oldName = oldView->name();
                    toast->done(0);// remove the "oldest" and add it to the counter
                    showVoiceCounter(oldName, behavior);
                }   
            }
        }
        else
        {
            // A remote user is talking
            if (visibleVoiceIdentifiers() >= TOAST_MAX_LIMIT)
            {
                showVoiceCounter(title, behavior);
            }
            else
            {
                toast = requestMiniToast();
                if (toast)
                {
                    toast->setIGOFlags(behavior.IGOFlags());
                    toast->shouldAcceptClick(behavior.acceptClick());
                    toast->setCloseTimer(behavior.closeTime());
                    toast->showBubble(behavior.showBubble());
                    toast->setInitalOpacity(behavior.maxOpacity());
                    MiniToastView* view = new MiniToastView();
                    view->setIcon(pixmap.isNull() ? QPixmap(settings.iconPath()) : pixmap);
                    view->setTitle(title);
                    view->setName(title);
                    toast->setContent(view);
                    showMiniToast(toast);
                    updateVoiceCounter();
                }
            }
        }
    }
}

void ToastViewController::showVoiceCounter(const QString& title, OriginToastManager::ToastBehavior behavior)
{
    // We should only ever have one instance of a user in this list
    if (!mVoiceChatCounterList.contains(title))
        mVoiceChatCounterList.append(title);

    if (!mVoiceChatCounterToast)
    {
        // create counter
        mVoiceChatCounterToast = new UIToolkit::OriginNotificationDialog();
        // Summon a toast window
        mVoiceChatCounterToast->setObjectName("ToastDialog");
        mVoiceChatCounterToast->setAttribute(Qt::WA_DeleteOnClose, false);
        mVoiceChatCounterToast->setAttribute(Qt::WA_ShowWithoutActivating);
        mVoiceChatCounterToast->setWindowFlags(mVoiceChatCounterToast->windowFlags() | Qt::Popup | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput);

        // never show this bubble for these toasts
        mVoiceChatCounterToast->showBubble(behavior.showBubble());
        mVoiceChatCounterToast->shouldAcceptClick(behavior.acceptClick());
        mVoiceChatCounterToast->setIGOFlags(behavior.IGOFlags());
        mVoiceChatCounterToast->setCloseTimer(behavior.closeTime());
        // set the fade timer to -1 so we never fade.
        mVoiceChatCounterToast->setFadeInTime(-1);
        mVoiceChatCounterToast->setInitalOpacity(behavior.maxOpacity());

        // hide the toast so the show actually happens correctly and we get a showEvent
        if (mVoiceChatCounterToast->isVisible())
            mVoiceChatCounterToast->hide();

        ORIGIN_VERIFY_CONNECT(mVoiceChatCounterToast, SIGNAL(finished(int)), this, SLOT(onToastDone()));
    }

    updateVoiceCounter();
}

void ToastViewController::updateVoiceCounter()
{
    if (!mVoiceChatCounterToast)
        return;
    if (mVoiceChatCounterList.size() == 0)
    {
        mVoiceChatCounterToast->done(0);
        return;
    }
    MiniToastView* view = new MiniToastView();
    QString counter = QString("+%1").arg(mVoiceChatCounterList.size());
    view->setTitle(counter);
    view->setIcon(QPixmap());
    mVoiceChatCounterToast->setContent(view);
    showMiniToast(mVoiceChatCounterToast, ToastType_VoiceIdentifierCounter);
}

void ToastViewController::removeMiniToast(const QString& title, OriginToastManager::ToastBehavior behavior)
{
    // if our list is empty we have nothing to remove exit early
    if (mVoiceChatIdentifierPool.count() == 0 && !mUserVoiceChatToast  && !mVoiceChatCounterToast)
        return;

    bool showing = false;
    bool found = false;
    QList<UIToolkit::OriginNotificationDialog*>& toastPool = mVoiceChatIdentifierPool;

    for(int i = 0, numToast = toastPool.count(); i < numToast; i++)
    {
        UIToolkit::OriginNotificationDialog* toast = toastPool[i];
        MiniToastView* view = dynamic_cast<MiniToastView*>(toast->content());
        if (view && view->name() == title)
        {
            showing = true;
            found = true;
            toast->done(0);
        }
    }

    if (!found && mUserVoiceChatToast)
    {
        MiniToastView* view = dynamic_cast<MiniToastView*>(mUserVoiceChatToast->content());
        if (view && view->name() == title)
        {
            showing = true;
            found = true;
            mUserVoiceChatToast->done(0);
        }
    }
    // if we were showing this toast need to grab next from our list
    if (showing)
    {
        if (mVoiceChatCounterList.size())
        {
            QString nextUser = mVoiceChatCounterList.at(0);
            mVoiceChatCounterList.remove(0);

            // NEED TO CALL NEW TOAST WITH THIS NAME
            emit newMiniToast(nextUser);
        }
        // if we don't ahve anyone in our list need to update existing so that are placed correctly 
        else
        {
            mVoiceChatIdentifierLayoutManager->update();
        }
    }
    else
    {
        // Update our counter since the user was not being shown
        if (mVoiceChatCounterToast)
        {
            int old = mVoiceChatCounterList.indexOf(title);
            if(old >=0)
                mVoiceChatCounterList.remove(old);

            if (mVoiceChatCounterList.size() > 0)
            {
                showVoiceCounter(mVoiceChatCounterList[0], behavior);
            }
            else
            {
                mVoiceChatCounterToast->done(0);
            }
        }
    }
}

void ToastViewController::removeAllMiniToasts()
{
    QList<UIToolkit::OriginNotificationDialog*>& toastPool = mVoiceChatIdentifierPool;

    for(int i = 0, numToast = toastPool.count(); i < numToast; i++)
    {
        UIToolkit::OriginNotificationDialog* toast = toastPool[i];
        MiniToastView* view = dynamic_cast<MiniToastView*>(toast->content());
        if (view)
        {
            toast->done(0);
        }
    }
    if (mUserVoiceChatToast)
        mUserVoiceChatToast->done(0);

    if (mVoiceChatCounterToast)
        mVoiceChatCounterToast->done(0);
    mVoiceChatCounterList.clear();
}

void ToastViewController::onUrlLoaded(QNetworkReply* nreply)
{
    QUrl nurl = nreply->url();

    // no url in map? return
    if (!mUrlToastViewMap.contains(nurl))
        return;

    //Find our toast from the map
    UIToolkit::OriginNotificationDialog* toast = mUrlToastViewMap.value(nurl, NULL);
    // for some reason no toast? let's leave
    if (!toast)
        return;
    ToastView* tv = dynamic_cast<ToastView*>(toast->content());
    if (!tv)
        return;
    // no pixmap, show the one that is already been placed there
    QNetworkReply::NetworkError errorReply = nreply->error();
    if (errorReply == QNetworkReply::NoError)
    {
        QPixmap p;
        QByteArray bArray = nreply->readAll();
        p.loadFromData(bArray, "PNG");
        if(p.width()!=28)
        {
            p = p.scaled(QSize(28, 28), Qt::KeepAspectRatio);
        }
        tv->setIcon(p);
    }
    else
    {
        ORIGIN_LOG_EVENT << "Unable to load achievement pixmap: " <<  nreply->errorString();
    }
    UIScope scope = tv->context();
    showToast(toast, scope);
    mUrlToastViewMap.remove(nurl);
}

UIToolkit::OriginNotificationDialog* ToastViewController::requestToast(QString name, UIScope scope)
{
    QList<UIToolkit::OriginNotificationDialog*>& toastPool = getProperPool(name, scope);

    UIToolkit::OriginNotificationDialog* toast = NULL;

    // First look for hidden stale toasts
    for(int i = 0, numToast = toastPool.count(); i < numToast; i++)
    {
        UIToolkit::OriginNotificationDialog* toastValue = toastPool[i];
        ToastView* view = dynamic_cast<ToastView*>(toastValue->content());
        if ((view && name == OIG_HOTKEY_IDENTIFIER && view->name() == name) || (toastValue && !toastValue->isVisible()))
        {
            // This is a stale window that we can reuse
            // Show this bubble by deafult, it will be removed when needed down the line
            toastValue->showBubble(true);
            toast = toastValue;
            break;
        }
    }
    // None found
    if(toast == NULL)
    {
        // Can we create a new one?
        if (toastPool.count() < TOAST_MAX_LIMIT)
        {
            // Summon a toast window
            toast = new UIToolkit::OriginNotificationDialog();
            toast->setObjectName("ToastDialog");
            toast->setAttribute(Qt::WA_DeleteOnClose, false);
            toast->setAttribute(Qt::WA_ShowWithoutActivating);
            toast->setMouseTracking(true);
#ifdef ORIGIN_MAC
            // On OSX this prevents the Toast from bringing the entire app into the foreground
            toast->setWindowFlags(toast->windowFlags() | Qt::SubWindow);
#else
            toast->setWindowFlags(toast->windowFlags() | Qt::Popup);
#endif
            //EBIBUGS-11624: if something else is full screen don't show the Toast as topmost because then the Toast appears behind the fullscreen
            //(so user can't see it) but the input is still active so if you click over it, it minimizes the fullscreen app
            Qt::WindowFlags wflags = toast->windowFlags();
            if(isFullscreenAppRunning())
            {
                wflags &= ~Qt::WindowStaysOnTopHint; //turn off the topmost so it doesn't get input
            }
            else
            {
                wflags |= Qt::WindowStaysOnTopHint;    //restore topmost feature
            }
            toast->setWindowFlags(wflags);
            toastPool.append(toast);
        }
        // if not find the oldest and reuse
        else
        {
            toast = oldestToast(toastPool);
            toast->showBubble(true);
        }
    }
    // remove all toast connections because we might be reusing this notification
    // Any new connection need to be cleaned up here, but plese do not call 
    // disconnect(toast, 0,0,0) <-- this will cause issues with IGO
    ORIGIN_VERIFY_DISCONNECT(toast, SIGNAL(clicked()), 0, 0);
    ORIGIN_VERIFY_DISCONNECT(toast, SIGNAL(accepted()), 0, 0);
    ORIGIN_VERIFY_DISCONNECT(toast, SIGNAL(rejected()), 0, 0);
    ORIGIN_VERIFY_DISCONNECT(toast, SIGNAL(igoActivated()), 0, 0);
    ORIGIN_VERIFY_DISCONNECT(toast, SIGNAL(actionAccepted()), 0, 0);
    ORIGIN_VERIFY_DISCONNECT(toast, SIGNAL(actionRejected()), 0, 0);


    // Clean up all dynamic properties that might have been added to this object.
    QList<QByteArray> dynamicProperties = toast->dynamicPropertyNames();
    for (int i=0; i<dynamicProperties.count(); i++)
    {
        QByteArray dpropName = dynamicProperties[i];
        // According to the Qt 5.2 documentation:
        // Dynamic properties starting with "_q_" are reserved for internal purposes.
        if (dpropName.startsWith("_q_"))
            continue;
        toast->setProperty(dynamicProperties[i], QVariant());
    }

    //EBIBUGS-26798: if something else is full screen remove toast input because then the Toast appears behind the fullscreen
    //(so user can't see it) but the input is still active so if you click over it, it minimizes the fullscreen app
    Qt::WindowFlags wflags = toast->windowFlags();
    wflags |= Qt::WindowStaysOnTopHint;
    if(isFullscreenAppRunning())
    {
        wflags |= Qt::WindowTransparentForInput; // Turn off input
    }
    else
    {
        wflags &= ~Qt::WindowTransparentForInput; // Turn on input
    }
    toast->setWindowFlags(wflags);

    // reset the fade timer incase this is a re-used notificiation.
    toast->setFadeInTime(Services::readSetting(Services::SETTING_NotificationExpiryTime));
    // Need to reset the toast sizes incase it previously had a fixed height (in-game notifications)
    toast->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    toast->setMinimumSize(0, 0);
    toast->setInitalOpacity(0);
    // hook this up so we remove the contect and close this properly upon fadeout or close
    ORIGIN_VERIFY_CONNECT(toast, SIGNAL(finished(int)), this, SLOT(onToastDone()));
    // hide the toast so the show actually happens correctly and we get a showEvent
    if (toast->isVisible())
        toast->hide();
    return toast;
}

UIToolkit::OriginNotificationDialog* ToastViewController::requestMiniToast()
{
    QList<UIToolkit::OriginNotificationDialog*>& toastPool = mVoiceChatIdentifierPool;
    UIToolkit::OriginNotificationDialog* toast = NULL;

    // First look for hidden stale toasts
    for(int i = 0, numToast = toastPool.count(); i < numToast; i++)
    {
        UIToolkit::OriginNotificationDialog* toastValue = toastPool[i];
        if ((toastValue && !toastValue->isVisible()))
        {
            // This is a stale window that we can reuse
            toast = toastValue;
            break;
        }
    }
    // None found
    if(toast == NULL)
    {
        // Can we create a new one?
        if (toastPool.count() < TOAST_MAX_LIMIT)
        {
            // Summon a toast window
            toast = new UIToolkit::OriginNotificationDialog();
            toast->setObjectName("ToastDialog");
            toast->setAttribute(Qt::WA_DeleteOnClose, false);
            ORIGIN_VERIFY_CONNECT(toast, SIGNAL(finished(int)), this, SLOT(onToastDone()));
            toast->setAttribute(Qt::WA_ShowWithoutActivating);
            toast->setWindowFlags(toast->windowFlags() | Qt::Popup | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput);

            toastPool.append(toast);
        }
        // if not find the oldest and reuse
        else
        {
            toast = oldestToast(toastPool);
        }
    }

    // set the fade timer to -1 so we never fade.
    toast->setFadeInTime(-1);
    toast->setInitalOpacity(1);
    // hide the toast so the show actually happens correctly and we get a showEvent
    if (toast->isVisible())
        toast->hide();
    return toast;
}

void ToastViewController::showToast(QWidget* toast, UIScope scope, int type)
{
    if(toast)
    {
        toast->adjustSize();
        if(scope == IGOScope)
        {
            showNotification(toast);
            mIGOLayoutManager->add(toast, type);
        }
        else
        {
            mDesktopLayoutManager->add(toast, type);
        }
        toast->show();
    }
}

void ToastViewController::showMiniToast(QWidget* toast, int type)
{
    if (toast)
    {
        showNotification(toast);
        mVoiceChatIdentifierLayoutManager->add(toast, type);
        toast->show();
    }
}


void ToastViewController::onToastDone()
{
    UIToolkit::OriginNotificationDialog* toast = dynamic_cast<UIToolkit::OriginNotificationDialog*>(sender());
    if(toast)
    {
        // If this was our active Toast clear the Variable
        if (toast == mActiveIGOToast)
            mActiveIGOToast = NULL;

        // We recycle our toaster objects
        Origin::Engine::IGOController::instance()->igowm()->removeWindow(toast, false);

        // Mark this toast as stale.
        toast->removeContent();
    }
}

bool ToastViewController::showNotification(QWidget* widget)
{
    Origin::Engine::IIGOWindowManager* igowm = Origin::Engine::IGOController::instance()->igowm();

#ifndef ORIGIN_MAC
    // Notifications do not make sense, when IGO can't be displayed!!!
    if (!igowm->isScreenLargeEnough())
    {
        QSize screen = igowm->screenSize();
        ORIGIN_LOG_EVENT << "Skipping showing notification: width = " << screen.width() << ", height = " << screen.height();
        return false;
    }
#endif
    UIToolkit::OriginNotificationDialog* toast = dynamic_cast<UIToolkit::OriginNotificationDialog*>(widget);
    Origin::Engine::IIGOWindowManager::WindowProperties properties;
    properties.setPosition(widget->pos());   // use pre-computed location
    properties.setOpacity(toast->opacity());            // force refresh since reusing windows
    properties.setSize(widget->size());      // force refresh since reusing windows
    
    // We may already have this toast showing...
    if (igowm->setWindowProperties(widget, properties))
        return true;

    Origin::Engine::IIGOWindowManager::WindowBehavior behavior;
    behavior.allowClose = true;
    behavior.allowMoveable = true;
    behavior.alwaysVisible = true;
    behavior.alwaysOnTop = true;
    behavior.disableDragging = true;
    behavior.doNotDeleteOnClose = true;
    behavior.dontStealFocus = true;
    
    return igowm->addPopupWindow(widget, properties, behavior);
}

bool ToastViewController::isFullscreenAppRunning() const
{
    bool fullscreen = false;
#ifdef ORIGIN_PC    //Windows specific; for now, this fn will return false for other OSes
    HWND topWin;
    topWin = ::GetForegroundWindow ();
    if (Origin::Services::PlatformService::isWindowFullScreen(topWin))
    {
        fullscreen = true;
        fullScreenApp = topWin;
    }
#endif

    return fullscreen;
}

UIToolkit::OriginNotificationDialog* ToastViewController::oldestToast(QList<UIToolkit::OriginNotificationDialog*>& toastPool)
{
    int oldest = 0;
    UIToolkit::OriginNotificationDialog* toast = NULL;
    for(int i=0; i < toastPool.count(); ++i)
    {
        if (oldest < toastPool[i]->getLifeSpan())
        {
            oldest = toastPool[i]->getLifeSpan();
            toast = toastPool[i];
        }
    }
    toast->resetLifeSpan();
    return toast;
}

void ToastViewController::setActiveIGOToast(UIToolkit::OriginNotificationDialog* toast)
{
    mActiveIGOToast = toast;
}

void ToastViewController::onIGOActivated()
{
    UIToolkit::OriginNotificationDialog* toast = static_cast<UIToolkit::OriginNotificationDialog*>(sender());
    if (toast == mActiveIGOToast)
        toast->activated();
    else
        toast->close();
}

int ToastViewController::visibleVoiceIdentifiers()
{
    int visibleToastCounter = 0;
    for(int i = 0, numToast = mVoiceChatIdentifierPool.count(); i < numToast; i++)
    {
        UIToolkit::OriginNotificationDialog* toastValue = mVoiceChatIdentifierPool[i];
        if ((toastValue && toastValue->isVisible()))
        {
            visibleToastCounter++;
        }
    }
    // are we showing the current user??
    if (mUserVoiceChatToast && mUserVoiceChatToast->isVisible())
    {
        visibleToastCounter++;
    }
    return visibleToastCounter;
}

void ToastViewController::playSound(const QString& path)
{
    // Grab the appliction's current volume
    float volume = Origin::Services::PlatformService::GetCurrentApplicationVolume();
    
    // Use QMediaPlayer instead of QSound so we can set the volume properly
    QUrl pathUrl = QUrl::fromLocalFile(path);
    mMediaPlayer->setMedia(pathUrl);
    mMediaPlayer->setVolume(volume * 100);
    mMediaPlayer->play();
}

#ifdef ORIGIN_PC
void ToastViewController::SetUpWinHook()
{
    g_hook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,  // Range of events (3 to 3).
        NULL,                                          // Handle to DLL.
        HandleWinEvent,                                // The callback.
        0, 0,              // Process and thread IDs of interest (0 = all)
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS); // Flags.  
}

void ToastViewController::ShutdownWinHook()
{
    UnhookWinEvent(g_hook);
}

void ToastViewController::HandleWindowEvents(HWND hwnd)
{
    if (!g_foregroundWindow || g_foregroundWindow != hwnd)
    {
        g_foregroundWindow = hwnd;
        // First check to see if new window if full screen
        if (isFullscreenAppRunning())
        {
            return;
        }

        if (fullScreenApp && fullScreenApp != g_foregroundWindow)
        {
            foreach (UIToolkit::OriginNotificationDialog* toast, mToastViewPool)// We should only ever need to loop through desktop notifications here
            {
                if (toast->content())// does this toast have any content to show?
                {
                    Qt::WindowFlags wflags = toast->windowFlags();
                    wflags &= ~Qt::WindowTransparentForInput;    //restore mouse input
                    toast->setWindowFlags(wflags);
                    toast->update();
                    toast->show();
                }
            }
        }
    }
}
#endif




}
}
