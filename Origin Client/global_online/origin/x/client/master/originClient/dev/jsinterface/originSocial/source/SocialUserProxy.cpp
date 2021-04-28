#include "SocialUserProxy.h"

#include <QFile>
#include <QBuffer>

#include "services/debug/DebugService.h"
#include "engine/social/SocialController.h"
#include "engine/social/UserNamesCache.h"
#include "engine/social/AvatarManager.h"
#include "engine/igo/IGOController.h"
#include "flows/ClientFlow.h"

#include "chat/OriginGameActivity.h"
#include "chat/OriginConnection.h"

#include "ConversionHelpers.h"
#include "OwnedProductsQueryOperationProxy.h"
#include "UserContactsQueryOperationProxy.h"
#include "OriginSocialProxy.h"

#include "AchievementShareQueryOperationProxy.h"
#include "AchievementShareSetOperationProxy.h"

#include "ClientViewController.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

namespace
{
    QVariant dataUriVariantForAvatarBinary(const Engine::Social::AvatarBinary &binary)
    {
        QByteArray rawUri = binary.toDataUri();

        if (rawUri.isNull())
        {
            // Null QStrings become blank strings when passed across the JavaScript bridge
            return QVariant();
        }

        return QString::fromLatin1(rawUri);
    }
}

SocialUserProxy::SocialUserProxy(OriginSocialProxy *socialProxy, Engine::Social::SocialController *socialController, int id, Chat::XMPPUser *proxied) :
    QObject(socialProxy),
    m_socialProxy(socialProxy),
    m_socialController(socialController),
    m_id(id),
    m_proxied(proxied)
{
    // Request their Origin ID if it hasn't already been loaded
    m_socialController->userNames()->subscribe(nucleusId(), false);

    ORIGIN_VERIFY_CONNECT(socialController->smallAvatarManager(), SIGNAL(avatarChanged(quint64)),
            this, SLOT(onSmallAvatarChanged(quint64)));

    ORIGIN_VERIFY_CONNECT(socialController->mediumAvatarManager(), SIGNAL(avatarChanged(quint64)),
            this, SLOT(onMediumAvatarChanged(quint64)));

    ORIGIN_VERIFY_CONNECT(socialController->userNames(), SIGNAL(namesChanged(quint64, const Origin::Engine::Social::UserNames &)),
            this, SLOT(onUserNamesChanged(quint64)));

    ORIGIN_VERIFY_CONNECT(proxied, SIGNAL(presenceChanged(const Origin::Chat::Presence &, const Origin::Chat::Presence &)), this, SIGNAL(presenceChanged()));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(presenceChanged()), this, SLOT(onPresenceChanged()));

    ORIGIN_VERIFY_CONNECT(this, SIGNAL(anyChange(quint64)), socialProxy, SIGNAL(anyChange(quint64)));
}

QString SocialUserProxy::id()
{
    return QString::number(m_id);
}
    
QVariant SocialUserProxy::avatarUrl()
{
    return dataUriVariantForAvatarBinary(m_socialController->smallAvatarManager()->binaryForNucleusId(nucleusId()));
}

QVariant SocialUserProxy::largeAvatarUrl()
{
    // Get the avatar manager for 208x208 avatars
    return dataUriVariantForAvatarBinary(m_socialController->mediumAvatarManager()->binaryForNucleusId(nucleusId()));
}

QString SocialUserProxy::presence()
{
    if (m_proxied.isNull() || m_proxied->presence().isNull())
    {
        return "UNKNOWN";
    }

    const Chat::Presence presence = m_proxied->presence();

    if (!presence.gameActivity().isNull())
    {
        return presence.gameActivity().joinable() ? "JOINABLE" : "IN_GAME";
    }

    switch(presence.availability())
    {
        case Chat::Presence::Chat:
        case Chat::Presence::Online:
            return "ONLINE";
        case Chat::Presence::Dnd:
            return "BUSY";
        case Chat::Presence::Away:
        case Chat::Presence::Xa:
            return "IDLE";
        case Chat::Presence::Unavailable:
            return "OFFLINE";
    }

    ORIGIN_ASSERT(0);
    return "UNKNOWN";
}

QVariant SocialUserProxy::availability()
{
    return ConversionHelpers::availabilityToString(m_proxied->presence().availability());
}

QVariant SocialUserProxy::statusText()
{
    if (m_proxied->presence().statusText().isNull())
    {
        return QVariant();
    }

    return m_proxied->presence().statusText();
}

QVariant SocialUserProxy::playingGame()
{
    return ConversionHelpers::originGameActivityToDict(m_proxied->presence().gameActivity());
}

void SocialUserProxy::showProfile(const QString &org)
{
    bool isInIgo = Engine::IGOController::instance()->isActive();
    ShowProfileSource sps = stringToProfileSource(org);
    ClientFlow::instance()->showProfile(nucleusId(), isInIgo ? IGOScope : ClientScope, sps, Engine::IIGOCommandController::CallOrigin_CLIENT);
}
    
QVariant SocialUserProxy::realName()
{
    const Engine::Social::UserNames names = m_socialController->userNames()->namesForNucleusId(nucleusId());

    if (names.firstName().isEmpty() && names.lastName().isEmpty())
    {
        return QVariant();
    }

    QVariantMap ret;

    ret["firstName"] = names.firstName();
    ret["lastName"] = names.lastName();

    return ret;

}

void SocialUserProxy::requestRealName()
{
    m_socialController->userNames()->subscribe(nucleusId(), true);
}

void SocialUserProxy::requestLargeAvatar()
{
    m_socialController->mediumAvatarManager()->subscribe(nucleusId());
}
    
quint64 SocialUserProxy::nucleusId()
{
    if (m_proxied)
    {
        return socialController()->chatConnection()->jabberIdToNucleusId(m_proxied->jabberId());
    }

    return 0;
}

QVariant SocialUserProxy::updateAchievements()
{
    const QString nucleusIdStr(QString::number(nucleusId()));
    const QString nucleusPersonaIdStr(QString::number(nucleusPersonaId()));

    AchievementManagerProxy::instance()->updateAchievementPortfolio(nucleusIdStr, nucleusPersonaIdStr);
    return AchievementManagerProxy::instance()->getAchievementPortfolioByPersonaId(nucleusPersonaIdStr);
}

bool SocialUserProxy::hasPid(const QString &pid)
{
    return QString::number(nucleusId()) == pid;
}

void SocialUserProxy::onSmallAvatarChanged(quint64 changedNucleusId)
{
    if (changedNucleusId == nucleusId())
    {
        emit avatarChanged();
        emit anyChange(changedNucleusId);
    }
}

void SocialUserProxy::onMediumAvatarChanged(quint64 changedNucleusId)
{
    if (changedNucleusId == nucleusId())
    {
        emit largeAvatarChanged();
        emit anyChange(changedNucleusId);
    }
}

void SocialUserProxy::onUserNamesChanged(quint64 changedNucleusId)
{
    if (changedNucleusId == nucleusId())
    {
        emit nameChanged();
        emit anyChange(changedNucleusId);
    }
}

void SocialUserProxy::onPresenceChanged()
{
    emit anyChange(nucleusId());
}

QObject* SocialUserProxy::queryOwnedProducts()
{
    return new OwnedProductsQueryOperationProxy(this, nucleusId());
}

QObject* SocialUserProxy::queryContacts()
{
    return new UserContactsQueryOperationProxy(this, m_socialProxy, nucleusId());
}

            QObject * SocialUserProxy::achievementSharingState()
            {
                QObject * obj = new AchievementShareQueryOperationProxy(this);
                ORIGIN_VERIFY_CONNECT(obj, SIGNAL(succeeded(QVariant)), this, SIGNAL(succeeded(QVariant)));
                return obj;
            }

            QObject * SocialUserProxy::setAchievementSharingState(const QVariant& data)
            {
                QObject * obj = new AchievementShareSetOperationProxy(this, data);
                ORIGIN_VERIFY_CONNECT(obj, SIGNAL(succeeded(QVariant)), this, SIGNAL(succeeded(QVariant)));
                return obj;
            }


        }
    }
}
