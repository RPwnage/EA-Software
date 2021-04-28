// *********************************************************************
//  RemoteUserProxy.h
//  Copyright (c) 2013 Electronic Arts Inc. All rights reserved.
// **********************************************************************


#ifndef _REMOTEUSERPROXY_H
#define _REMOTEUSERPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QVariant>

#include <chat/RemoteUser.h>

#include "SocialUserProxy.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Chat
{
class BlockList;
}

namespace Client
{
namespace JsInterface
{

class AbstractRosterView;
class OriginSocialProxy;
class RosterProxy;

class ORIGIN_PLUGIN_API RemoteUserProxy : public SocialUserProxy
{
    Q_OBJECT 

    friend class AbstractRosterView;
    friend class RosterProxy;
public:
    RemoteUserProxy(OriginSocialProxy *socialProxy, Engine::Social::SocialController *socialController, int id, Chat::RemoteUser *proxied);

    Q_INVOKABLE void startConversation();

    Q_INVOKABLE void startVoiceConversation();
    
    Q_INVOKABLE void refreshBlockList();

    Q_PROPERTY(bool blocked READ blocked);
    bool blocked();
    
    Q_PROPERTY(QVariantMap subscriptionState READ subscriptionState);
    QVariantMap subscriptionState();

    Q_PROPERTY(QString capabilities READ capabilities);
    QString capabilities() const;

    Q_INVOKABLE void inviteToGame();
    Q_INVOKABLE void joinGame();
    
    Q_INVOKABLE void block();
    Q_INVOKABLE void removeAndBlock();
    Q_INVOKABLE void unblock();

    Q_INVOKABLE void acceptSubscriptionRequest();
    Q_INVOKABLE void rejectSubscriptionRequest();
    Q_INVOKABLE void cancelSubscriptionRequest();
    Q_INVOKABLE void requestSubscription();

    Q_INVOKABLE void promoteUserToAdmin(const QString& groupGuid);
    Q_INVOKABLE void transferGroupOwnership(const QString& groupGuid);
    Q_INVOKABLE void demoteAdminToMember(const QString& groupGuid);

    QVariant nickname();

    QVariant statusText();

    Q_INVOKABLE void reportTosViolation();

    Q_INVOKABLE virtual QObject *achievementSharingState();     ///< returns the achievements sharing state

    //added for OriginX
    Q_PROPERTY(QString jabberId READ jabberId);
    QString jabberId() const;

    // Returns our backing RemoteUser
    Chat::RemoteUser* proxied() { return mProxied; }

signals:
    void blockChanged(bool blocked);
    void subscriptionStateChanged();
    void capabilitiesChanged();

    void addedToRoster();
    void removedFromRoster();
    void promoteToAdmin(const QString& groupGuid);
    void promoteToAdminSuccess(QObject*);
    void transferOwnershipSuccess(QObject*);
    void demoteToMember(const QString& groupGuid);
    void demoteToMemberSuccess(QObject*);

protected:
    quint64 nucleusPersonaId();

private slots:
    void blockListChanged();

    void onPromoteToAdminSuccess();
    void onPromoteToAdminFailure(QObject*, Services::GroupResponse::GroupResponseError);
    void onTransferOwnershipSuccess(const QString& groupGuid);
    void onTransferOwnershipFailure(const QString& groupGuid);
    void onDemoteToMemberSuccess();

private:
    Chat::RemoteUser *mProxied;
    Chat::BlockList *mBlockList;
    bool mBlocked;
};

}
}
}

#endif
