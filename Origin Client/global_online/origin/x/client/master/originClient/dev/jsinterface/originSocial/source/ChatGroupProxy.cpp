#include "ChatGroupProxy.h"
#include "ChatChannelProxy.h"
#include "OriginSocialProxy.h"
#include "RemoteUserProxy.h"
#include "ConnectedUserProxy.h"
#include "services/debug/DebugService.h"
#include "engine/igo/IGOController.h"
#include "ClientFlow.h"
#include <chat/RemoteUser.h>
#include "flows/AcceptInviteAndOpenChannelFlow.h"
#include "engine/social/SocialController.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            int ChatGroupProxy::mNextId = 0;

            ChatGroupProxy::ChatGroupProxy(Chat::ChatGroup *proxied)
                : QObject(proxied)
                , mProxied(proxied)
            {
#if 0 //disable for Origin X
                OriginSocialProxy* social = OriginSocialProxy::instance();
                ORIGIN_VERIFY_CONNECT(social, SIGNAL(anyChange(quint64)), this, SLOT(onAnyChange(quint64)));   
#endif
                ORIGIN_VERIFY_CONNECT(proxied, SIGNAL(groupMemberRemoved(Origin::Chat::JabberID)), this, SLOT(onGroupMemberRemoved(Origin::Chat::JabberID)));
                ORIGIN_VERIFY_CONNECT(proxied, SIGNAL(groupMembersLoadFinished()), this, SLOT(onMembersLoadFinished()));
                ORIGIN_VERIFY_CONNECT(proxied, SIGNAL(groupMembersRoleChange(quint64 , const QString& )), this, SLOT(onGroupMembersRoleChange(quint64 , const QString& )));                
                ORIGIN_VERIFY_CONNECT(proxied, SIGNAL(enterMucRoomFailed(int)), this, SIGNAL(enterMucRoomFailed(int)));                
                ORIGIN_VERIFY_CONNECT(proxied, SIGNAL(rejoinMucRoomSuccess()), this, SIGNAL(rejoinMucRoomSuccess()));    

                mId = ++mNextId;
            }

            ChatGroupProxy::~ChatGroupProxy()
            {
            }

            void ChatGroupProxy::getGroupChannelInformation()
            {
                mProxied->getGroupChannelInformation();
            }

            qint64 ChatGroupProxy::getUserNucleusId(QObject* user)
            {
                qint64 id = -1;       
#if 0 //disable for Origin X
                RemoteUserProxy* remoteUser = dynamic_cast<RemoteUserProxy*>(user);
                if (remoteUser) 
                {
                    id = remoteUser->proxied()->nucleusId();
                }
                else
                {
                    ConnectedUserProxy* connectedUser = dynamic_cast<ConnectedUserProxy*>(user);
                    if (connectedUser)
                    {
                        OriginSocialProxy* social = OriginSocialProxy::instance();
                        if (social)
                            id = social->nucleusIdByUserId(connectedUser->id());
                    }
                }
#endif
                return id;
            }
            QString ChatGroupProxy::getRemoteUserRole(QObject* user)
            {
                qint64 id = getUserNucleusId(user);
                if (id > 0)
                {
                    return mProxied->getUserRole(id);
                }
                return "";
            }

            bool ChatGroupProxy::isUserInGroup(QObject* user)
            {
                qint64 id = getUserNucleusId(user);
                if (id > 0)
                {
                    return mProxied->isUserInGroup(id);
                }
                return false;
            }

            void ChatGroupProxy::inviteUsersToGroup(const QObjectList& users)
            {
                ORIGIN_VERIFY_CONNECT(this->proxied(), SIGNAL(usersInvitedToGroup(QList<Origin::Chat::RemoteUser*>)), this, SLOT(onUsersInvitedToGroup(QList<Origin::Chat::RemoteUser*>)));
                QList<Origin::Chat::RemoteUser*> remoteUsers;
                for(QObjectList::const_iterator it = users.begin(); it != users.end(); it++) 
                {                
                    JsInterface::RemoteUserProxy* pr = dynamic_cast<JsInterface::RemoteUserProxy*>(*it);
                    if (pr) 
                    {
                        remoteUsers.append(pr->proxied());
                    }
                }

                this->proxied()->inviteUsersToGroup(remoteUsers);
            }

            void ChatGroupProxy::acceptGroupInvite()
            {
                AcceptInviteAndOpenChannelFlow *flow = new AcceptInviteAndOpenChannelFlow(mProxied);
                flow->start();
            }

            void ChatGroupProxy::rejectGroupInvite()
            {
                mProxied->rejectGroupInvite();
            }

            void ChatGroupProxy::onUsersInvitedToGroup(QList<Origin::Chat::RemoteUser*> users)
            {
                ORIGIN_VERIFY_DISCONNECT(this->proxied(), SIGNAL(usersInvitedToGroup(QList<Origin::Chat::RemoteUser*>)), this, SLOT(onUsersInvitedToGroup(QList<Origin::Chat::RemoteUser*>)));
                UIScope scope = ClientScope;
                if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
                    scope = IGOScope;

                ClientFlow::instance()->showGroupInviteSentDialog(users, scope);
            }

            void ChatGroupProxy::addChatChannelProxy(ChatChannelProxy *proxy)
            {
                mChatChannelProxies.insert(proxy);
            }

            void ChatGroupProxy::removeChatChannelProxy(ChatChannelProxy *proxy)
            {
                mChatChannelProxies.remove(proxy);
            }

            QString ChatGroupProxy::id()
            {
                return QString::number(mId);
            }

            QString ChatGroupProxy::name()
            {
                return mProxied->getName();
            }

            QString ChatGroupProxy::groupGuid()
            {
                return mProxied->getGroupGuid();
            }

            QVariant ChatGroupProxy::invitedBy()
            {
#if 0 //disable for Origin X
                OriginSocialProxy* social = OriginSocialProxy::instance();
                qint64 invitedBy = mProxied->getInvitedBy();
                if (invitedBy > 0 && social)
                {
                    return social->getUserById( social->userIdByNucleusId(invitedBy) );
                }
#endif
                return QVariant();
            }

            QString ChatGroupProxy::role()
            {
                return mProxied->getGroupRole();
            }

            QObjectList ChatGroupProxy::channels()
            {
                QObjectList ret; 

                for(QSet<ChatChannelProxy*>::const_iterator it = mChatChannelProxies.constBegin();
                    it != mChatChannelProxies.constEnd();
                    it++)
                {
                    ret.append(*it);
                }
                return ret;
            }

            void ChatGroupProxy::onMembersLoadFinished()
            {
                emit membersLoadFinished();
            }

            void ChatGroupProxy::onGroupMembersRoleChange(quint64 userId, const QString& role)
            {
                emit anyChange();
            }

            void ChatGroupProxy::onGroupMemberRemoved(Origin::Chat::JabberID jid)
            {
                emit anyChange();
            }

            QVariantList ChatGroupProxy::owners()
            {
#if 0 //disable for Origin X
                OriginSocialProxy* social = OriginSocialProxy::instance();
                if (social)
                {
                    mOwners.clear();
                    QList<qint64> ownersList = mProxied->getOwnersList();
                    for(QList<qint64>::const_iterator it = ownersList.constBegin();
                        it != ownersList.constEnd();
                        it++)
                    {                    
                        QString userId = social->userIdByNucleusId((*it));
                        mOwners.append(social->getUserById( userId )); 
                    }
                }
#endif
                return mOwners;
            }

            QVariantList ChatGroupProxy::admins()
            {
#if 0 //disable for Origin X
                OriginSocialProxy* social = OriginSocialProxy::instance();
                if (social)
                {
                    mAdmins.clear();
                    QList<qint64> adminsList = mProxied->getAdminsList();
                    for(QList<qint64>::const_iterator it = adminsList.constBegin();
                        it != adminsList.constEnd();
                        it++)
                    {                    
                        QString userId = social->userIdByNucleusId((*it));
                        mAdmins.append(social->getUserById( userId )); 
                    }
                }
#endif
                return mAdmins;
            }

            QVariantList ChatGroupProxy::members()
            {
#if 0 //disable for Origin X
                OriginSocialProxy* social = OriginSocialProxy::instance();
                if (social)
                {
                    mMembers.clear();
                    QList<qint64> memberList = mProxied->getMembersList();
                    for(QList<qint64>::const_iterator it = memberList.constBegin();
                        it != memberList.constEnd();
                        it++)
                    {                    
                        QString userId = social->userIdByNucleusId((*it));
                        mMembers.append(social->getUserById( userId )); 
                    }
                }
#endif
                return mMembers;
            }

            bool ChatGroupProxy::inviteGroup()
            {
                return mProxied->isInviteGroup();
            }

            void ChatGroupProxy::onAnyChange(quint64 id)
            {
                if (mProxied->isUserInGroup(id))
                {
                    emit anyChange();
                }
            }

            void ChatGroupProxy::failedToEnterRoom()
            {
                Origin::Engine::Social::SocialController* socialController = Origin::Engine::LoginController::currentUser()->socialControllerInstance();
                if (socialController)
                    emit socialController->failedToEnterRoom();
            }
        }
    }
}
