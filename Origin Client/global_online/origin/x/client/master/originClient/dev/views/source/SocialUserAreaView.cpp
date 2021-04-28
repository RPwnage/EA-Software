/////////////////////////////////////////////////////////////////////////////
// SocialUserAreaView.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "SocialUserAreaView.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "engine/login/LoginController.h"
#include "engine/social/SocialController.h"
#include "engine/social/AvatarManager.h"
#include "engine/social/OnlineContactCounter.h"

#include "chat/OriginConnection.h"
#include "chat/ConnectedUser.h"
#include "chat/Roster.h"
#include "chat/RemoteUser.h"

#include "AchievementMiniView.h"

#include <QGraphicsDropShadowEffect>
#include "ui_SocialUserAreaView.h"

namespace
{
    const int UsernameMaxWidth = 132;
}

namespace Origin
{
    namespace Client
    {
        SocialUserAreaView::SocialUserAreaView(QWidget *parent)
            : QWidget(parent)
            , mAchievementMiniView(NULL)
            , mOfflineMode(false)
            , mUnderageMode(false)
            , mSocialController(Engine::LoginController::currentUser()->socialControllerInstance())
            , ui(new Ui::SocialUserAreaView())
        {
            ui->setupUi(this);

            init();
        }

        SocialUserAreaView::~SocialUserAreaView()
        {
            if(mAchievementMiniView)
            {
                delete mAchievementMiniView;
                mAchievementMiniView = NULL;
            }
        }

        void SocialUserAreaView::init()
        {
            Engine::UserRef user = Engine::LoginController::currentUser();
            ui->lblFriendCountBadge->setAttribute(Qt::WA_TransparentForMouseEvents);
            
            // Surface these signals to our controller
            ORIGIN_VERIFY_CONNECT(ui->btnShowFriends, SIGNAL(clicked()), this, SIGNAL(clickedShowFriendsList()));
            ORIGIN_VERIFY_CONNECT(ui->lblMeName, SIGNAL(clicked()), this, SIGNAL(clickedMyUsername()));
            ORIGIN_VERIFY_CONNECT(ui->btnMeAvatar, SIGNAL(clicked()), this, SIGNAL(clickedMyAvatar()));

#ifdef ORIGIN_PC
            // Procedural drop shadows FTW
            {
                QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
                shadow->setOffset(QPointF(0, 0));
                shadow->setColor(QColor(0,0,0,200));
                shadow->setBlurRadius(4);
                ui->lblMeName->setGraphicsEffect(shadow);
            }
            {
                QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
                shadow->setOffset(QPointF(0, 0));
                shadow->setColor(QColor(0,0,0,200));
                shadow->setBlurRadius(4);
                ui->btnMeAvatar->setGraphicsEffect(shadow);
            }
            {
                QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
                shadow->setOffset(QPointF(0, 0));
                shadow->setColor(QColor(0,0,0,220));
                shadow->setBlurRadius(4);
                ui->lblFriendCountBadge->setGraphicsEffect(shadow);
            }
#elif defined(ORIGIN_MAC)
            // TODO: disabled for osx for performance reasons
#endif

            // Handle online user count
            updateOnlineFriendCount(mSocialController->onlineContactCounter()->onlineContactCount());

            ORIGIN_VERIFY_CONNECT(
                    mSocialController->onlineContactCounter(), SIGNAL(onlineContactCountChanged(unsigned int)),
                    this, SLOT(updateOnlineFriendCount(unsigned int)));


            // Handle offline mode
            ORIGIN_VERIFY_CONNECT(Services::Connection::ConnectionStatesService::instance(), SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)),
                    this, SLOT(onConnectionStateChanged(bool)));

            // Handle our Origin ID
            setMyName(user->eaid());
            ORIGIN_VERIFY_CONNECT(Services::Session::SessionService::instance(), SIGNAL(userOriginIdUpdated(const QString&, Origin::Services::Session::SessionRef )), this, SLOT(setMyName(QString)));

            // Set up our avatar
            if(Services::Connection::ConnectionStatesService::instance()->isUserOnline(user->getSession()))
            {
                setAvatar(mSocialController->smallAvatarManager()->pixmapForNucleusId(user->userId()));
            }
            else
            {
                // EBIBUGS-25680: Read in cached avatar
                const QString userAvatarCacheUrl = Services::readSetting(Services::SETTING_USERAVATARCACHEURL, user->getSession());
                setAvatar(QPixmap(userAvatarCacheUrl));
            }
            ORIGIN_VERIFY_CONNECT(mSocialController->smallAvatarManager(), SIGNAL(avatarChanged(quint64)),
                    this, SLOT(onAvatarChanged(quint64)));
        }

        void SocialUserAreaView::updateOnlineFriendCount(unsigned int count)
        {
            const QString onlineStr(QString::number(count));

            ui->lblFriendCountBadge->setText(onlineStr);

            // Manually size and position the count badge label
            int w = ui->lblFriendCountBadge->fontMetrics().boundingRect(onlineStr).width();
            int x = ui->friendsButtonWidget->width() - (15 + (w/2));
            ui->lblFriendCountBadge->move(x, ui->lblFriendCountBadge->y());
            ui->lblFriendCountBadge->resize(w, ui->lblFriendCountBadge->height());
        }
            
        void SocialUserAreaView::onAvatarChanged(quint64 nucleusId)
        {
            Engine::UserRef user = Engine::LoginController::currentUser();

            if (nucleusId == user->userId())
            {
                setAvatar(mSocialController->smallAvatarManager()->pixmapForNucleusId(nucleusId));
            }
        }

        void SocialUserAreaView::setAvatar(QPixmap userAvatar)
        {
            ui->btnMeAvatar->setIcon(userAvatar.scaled(28, 28));
        }
            
        void SocialUserAreaView::setMyName(const QString &name)
        {
            const QString nameElided = ui->lblMeName->fontMetrics().elidedText(name, Qt::ElideRight, UsernameMaxWidth);
            ui->lblMeName->setText(nameElided);
        }

        void SocialUserAreaView::onAchievementEnabledChanged(const bool& enable)
        {
            if(enable && mAchievementMiniView == NULL)
            {
                mAchievementMiniView = new AchievementMiniView();
                ui->userInfoLayout->addWidget(mAchievementMiniView);
                mAchievementMiniView->setAlignment(AchievementMiniView::ALIGN_RIGHT);
                ORIGIN_VERIFY_CONNECT(mAchievementMiniView, SIGNAL(clicked()), this, SIGNAL(clickedAchievements()));
            }
            else
            {
                if(mAchievementMiniView)
                {
                    ui->userInfoLayout->removeWidget(mAchievementMiniView);
                    delete mAchievementMiniView;
                    mAchievementMiniView = NULL;
                }
            }
        }

        void SocialUserAreaView::setAchievementXp(const int& xp)
        {
            if (mAchievementMiniView)
            {
                mAchievementMiniView->setXP(xp);
            }
        }

        void SocialUserAreaView::onConnectionStateChanged(bool online)
        {
            mOfflineMode = !online;
            updateUI(mOfflineMode, mUnderageMode);
        }

        void SocialUserAreaView::setUnderageMode(bool aUnderage)
        {
            mUnderageMode = aUnderage;
            updateUI(mOfflineMode, mUnderageMode);
        }

        void SocialUserAreaView::updateUI(bool isOffline, bool isUnderage /*= false*/)
        {
            if(mAchievementMiniView)
            {
                mAchievementMiniView->setVisible(!isUnderage);
                mAchievementMiniView->setEnabled(/*!isOffline &&*/ !isUnderage);
            }

            ui->btnMeAvatar->setVisible(!isUnderage);
            ui->friendsButtonWidget->setVisible(!isUnderage);

            ui->btnMeAvatar->setEnabled(!isOffline && !isUnderage);
            ui->lblMeName->setEnabled(!isOffline && !isUnderage);
        }

    }
}
