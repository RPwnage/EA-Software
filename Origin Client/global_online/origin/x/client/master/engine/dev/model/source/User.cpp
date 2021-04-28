//    Copyright � 2011-2012, Electronic Arts
//    All rights reserved.
//    Author: Hans van Veenendaal

#include <QtXml/QDomDocument>
#include <QVariant>

#include "engine/login/User.h"
#include "engine/content/ContentController.h"
#include "engine/content/GamesController.h"
#include "engine/content/ContentOperationQueueController.h"
#include "engine/social/SocialController.h"
#include "engine/voice/VoiceController.h"
#include "engine/achievements/achievementmanager.h"
#include "services/session/SessionService.h"
#include "services/voice/VoiceService.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

using namespace Origin::Engine;

namespace
{
    void doDeleteLater(User *user)
    {
        user->deleteLater();
    }
}

UserRef User::create(Origin::Services::Session::SessionRef session, Origin::Services::Authentication2 const& serverData, bool online, QObject *parent)
{
    UserRef user = QSharedPointer<User>(new Engine::User(session, serverData, online, parent), doDeleteLater);
    
    user->setSelf(user); // because Qt doesn't have enable_shared_from_this
    user->createContentController();
    user->createGamesController();
    user->createAchievementController(session, online);
    user->createSocialController(); // create this before voice since there's a dependency
    user->createVoiceController();
    user->createContentOperationQueueController();
    
    return user;
}

User::User(Origin::Services::Session::SessionRef session, Origin::Services::Authentication2 const& serverData, bool online, QObject *parent)
    : QObject(parent)
	, mServerData(serverData)
    , mSession(session)
{
}

User::~User()
{
    cleanUp();

//this seems to cause compile error for the Mac, so just enable it for windows
#ifdef ORIGIN_PC
    //at this point, User will still have a reference to mSession so refcount of 1 would be expected
    if (mSession.d->strongref._q_value > 1)
    {
        ORIGIN_LOG_ERROR << "Someone is still holding a strong reference to LoginRegistrationServie, it will not be deleted! refcnt = " << mSession.d->strongref._q_value;
    }
#endif
}

void User::cleanUp()
{
    m_ContentController.reset();
    m_GamesController.reset();
    m_VoiceController.reset();
    m_SocialController.reset();
    m_ContentOperationQueueController.reset();
}

void User::setSelf(UserRef self)
{
    mSelf = self;
}

UserRef User::self()
{
    UserRef ret = mSelf.toStrongRef();
    ORIGIN_ASSERT(ret.data()==this);
    return ret;
}

void User::createContentController()
{
    //we must have a valid pointer to the UserRef
    ORIGIN_ASSERT(mSelf);
    if (m_ContentController.isNull())
    {
        m_ContentController.reset(Origin::Engine::Content::ContentController::create(self()));
    }
}

void User::createGamesController()
{
    //we must have a valid pointer to the UserRef
    ORIGIN_ASSERT(mSelf);
    if (m_GamesController.isNull())
    {
        m_GamesController.reset(Origin::Engine::Content::GamesController::create(self()));
    }
}

void User::createSocialController()
{
    //we must have a valid pointer to the UserRef
    ORIGIN_ASSERT(mSelf);
    if (m_SocialController.isNull())
    {
        m_SocialController.reset(Origin::Engine::Social::SocialController::create(self()));
    }
}

void User::createVoiceController()
{
#if ENABLE_VOICE_CHAT
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( isVoiceEnabled)
    {
        //we must have a valid pointer to the UserRef
        ORIGIN_ASSERT(mSelf);
        if (m_VoiceController.isNull())
        {
            m_VoiceController.reset(Origin::Engine::Voice::VoiceController::create(self()));
        }
    }
    else
    {
#endif
    m_VoiceController.reset();
#if ENABLE_VOICE_CHAT
    }
#endif
}

void User::createAchievementController(Origin::Services::Session::SessionRef session, bool online)
{
    Achievements::AchievementManager::instance()->createAchievementPortfolio(
        Origin::Services::Session::SessionService::instance()->nucleusUser(session), 
        Origin::Services::Session::SessionService::instance()->nucleusPersona(session));

    // Load the offline achievements.
    Achievements::AchievementManager::instance()->serialize(false);
}

void User::createContentOperationQueueController()
{
    //we must have a valid pointer to the UserRef
    ORIGIN_ASSERT(mSelf);
    if (m_ContentOperationQueueController.isNull())
    {
        m_ContentOperationQueueController.reset(Origin::Engine::Content::ContentOperationQueueController::create(mSelf));
    }
}

void User::setLocale(QString const& locale)
{
    if (mLocale != locale)
    {
        mLocale = locale;
        emit localeChanged(locale);
    }
}

Origin::Engine::Content::ContentController* User::contentControllerInstance()
{
    return dynamic_cast<Origin::Engine::Content::ContentController*>(m_ContentController.data());
}

Origin::Engine::Content::GamesController* User::gamesControllerInstance()
{
    return dynamic_cast<Origin::Engine::Content::GamesController*>(m_GamesController.data());
}

Origin::Engine::Social::SocialController* User::socialControllerInstance()
{
    return dynamic_cast<Origin::Engine::Social::SocialController*>(m_SocialController.data());
}

Origin::Engine::Content::ContentOperationQueueController* User::contentOperationQueueControllerInstance()
{
    return dynamic_cast<Origin::Engine::Content::ContentOperationQueueController*>(m_ContentOperationQueueController.data());
}

Origin::Engine::Voice::VoiceController* User::voiceControllerInstance()
{
    return dynamic_cast<Origin::Engine::Voice::VoiceController*>(m_VoiceController.data());
}
