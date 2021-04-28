#ifndef _CONNECTEDUSERPROXY_H
#define _CONNECTEDUSERPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QPointer>

#include "SocialUserProxy.h"
#include "engine/login/User.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Chat
{
    class ConnectedUser;
}

namespace Client
{
namespace JsInterface
{

class OriginSocialProxy;

// This has the same public interface as SocialUserProxy; it just patches up id() and nickname()
class ORIGIN_PLUGIN_API ConnectedUserProxy : public SocialUserProxy
{
    Q_OBJECT
public:
    ConnectedUserProxy(OriginSocialProxy *socialProxy, Engine::UserRef user, int id, Chat::ConnectedUser *proxied);

    QVariant nickname();
    
    Q_PROPERTY(QString requestedAvailability READ requestedAvailability WRITE setRequestedAvailability)
    QString requestedAvailability();
    void setRequestedAvailability(const QString &);

    Q_PROPERTY(QString visibility READ visibility);
    QString visibility();
    
    Q_PROPERTY(QString requestedVisibility READ requestedVisibility WRITE setRequestedVisibility)
    QString requestedVisibility();
    void setRequestedVisibility(const QString &);

    void setInitial(const QString &);

    Q_PROPERTY(QStringList allowedAvailabilityTransitions READ allowedAvailabilityTransitions);
    QStringList allowedAvailabilityTransitions();

signals:
    void allowedAvailabilityTransitionsChanged();
    void visibilityChanged();

protected:
    quint64 nucleusPersonaId();

private:
    QPointer<Chat::ConnectedUser> mConnectedUser;
    Engine::UserRef mUser;
};

}
}
}

#endif
