#ifndef _SOCIALUSERPROXY_H
#define _SOCIALUSERPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QPointer>
#include <QVariant>
#include <AchievementPortfolioProxy.h>
#include <AchievementManagerProxy.h>
#include "chat/XMPPUser.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Engine
{
namespace Social
{
    class SocialController;
}
}

namespace Client
{
namespace JsInterface
{
    
class OriginSocialProxy;

class ORIGIN_PLUGIN_API SocialUserProxy : public QObject
{
    Q_OBJECT
    friend class OriginSocialProxy;
public:
    SocialUserProxy(OriginSocialProxy *socialProxy, Engine::Social::SocialController *socialController, int id, Chat::XMPPUser *proxied);

    Q_PROPERTY(QString id READ id);
    QString id();

    Q_PROPERTY(QVariant avatarUrl READ avatarUrl)
    QVariant avatarUrl();

    Q_PROPERTY(QVariant nickname READ nickname)
    virtual QVariant nickname() = 0;
    
    Q_PROPERTY(QVariant availability READ availability)
    QVariant availability();

    Q_PROPERTY(QVariant statusText READ statusText)
    // RemoteUserProxy needs to override this for a compat hack
    virtual QVariant statusText();

    // Undocumented legacy property
    // My Origin Home Now and the new profile widget will temporarily need this
    Q_PROPERTY(QString presence READ presence)
    QString presence();

    Q_PROPERTY(QVariant playingGame READ playingGame)
    QVariant playingGame();

    Q_PROPERTY(QVariant realName READ realName)
    virtual QVariant realName();

    Q_INVOKABLE void showProfile(const QString &org);

    Q_INVOKABLE QVariant updateAchievements();

    Q_INVOKABLE bool hasPid(const QString &pid);

    Q_INVOKABLE void requestRealName();
    Q_INVOKABLE void requestLargeAvatar();

    Q_INVOKABLE virtual QObject *queryOwnedProducts();
    Q_INVOKABLE virtual QObject *queryContacts();

    Q_PROPERTY(QVariant largeAvatarUrl READ largeAvatarUrl)
    QVariant largeAvatarUrl();

    Q_INVOKABLE virtual QObject *achievementSharingState();     ///< returns the achievements sharing state

    Q_INVOKABLE virtual QObject *setAchievementSharingState(const QVariant& );     ///< sets the achievements sharing state


    QPointer<Chat::XMPPUser> proxied() { return m_proxied; }

signals:
    void avatarChanged();
    void largeAvatarChanged();

    void presenceChanged();
    void nameChanged();

    void anyChange(quint64 id);

    void succeeded(QVariant);

protected:
    Engine::Social::SocialController *socialController() { return m_socialController; }

    quint64 nucleusId();
    virtual quint64 nucleusPersonaId() = 0; 

private slots:
    void onSmallAvatarChanged(quint64 nucleusId);
    void onMediumAvatarChanged(quint64 nucleusId);
    void onUserNamesChanged(quint64 nucleusId);
    void onPresenceChanged();

private:
    OriginSocialProxy *m_socialProxy;
    Engine::Social::SocialController *m_socialController;

    int m_id;
    QPointer<Chat::XMPPUser> m_proxied;
};

}
}
}

#endif
