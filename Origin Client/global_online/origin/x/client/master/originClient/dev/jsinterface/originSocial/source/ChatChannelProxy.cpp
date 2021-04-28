#include "ChatChannelProxy.h"

#include "engine/login/LoginController.h"
#include "SocialUserProxy.h"

#include "flows/SummonToGroupChannelFlow.h"

#include "services/debug/DebugService.h"
#include "OriginSocialProxy.h"
#include "RemoteUserProxy.h"
#include "ConnectedUserProxy.h"

#include <chat/RemoteUser.h>
#include "flows/ToVisibleFlow.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            ChatChannelProxy::ChatChannelProxy(Chat::ChatChannel *proxied, int id, Engine::UserRef user)
                : mProxied(proxied)
                , mId(id)
                , mUser(user)
            {

            }

            QString ChatChannelProxy::id()
            {
                return QString::number(mId);
            }

            void ChatChannelProxy::joinChannel()
            {
                ToVisibleFlow *flow = new ToVisibleFlow(mUser->socialControllerInstance());
                ORIGIN_VERIFY_CONNECT(flow, SIGNAL(finished(const bool&)), this, SLOT(startJoinChannel(const bool&)));
                flow->start();
            }

            void ChatChannelProxy::startJoinChannel(const bool& success)
            {
                sender()->deleteLater();
                if(success)
                {
                    mProxied->joinChannel();
                }
            }

            void ChatChannelProxy::rejoinChannel()
            {
                mProxied->rejoinChannel();
            }

            void ChatChannelProxy::getChannelPresence()
            {
                mProxied->getPresence();
            }

            void ChatChannelProxy::inviteUser(QObject* user)
            {
                RemoteUserProxy* toUserProxy = dynamic_cast<RemoteUserProxy*>(user);
                if (toUserProxy)
                {
                    const Chat::JabberID toJid = toUserProxy->proxied()->jabberId();

                    QString groupId = mProxied->getGroupGuid();
                    QString channelId = mProxied->getChannelId();

                    SummonToGroupChannelFlow *flow = new SummonToGroupChannelFlow(mProxied, mUser, toJid, groupId, channelId);
                    ORIGIN_VERIFY_CONNECT(flow, SIGNAL(finished()), this, SLOT(inviteUserFinished()));
                    flow->start();
                }
            }

            void ChatChannelProxy::inviteUserFinished()
            {
                // Do we care if it succeeded?
                sender()->deleteLater();
            }

            QString ChatChannelProxy::getParticipantRole(QObject* user)
            {
                RemoteUserProxy* remoteUser = dynamic_cast<RemoteUserProxy*>(user);
                if (!remoteUser)
                {
                    return "";
                }
                return mProxied->getParticipantRole(remoteUser->proxied()->jabberId());
            }

            QString ChatChannelProxy::name()
            {
                return mProxied->getChannelName();
            }

            QString ChatChannelProxy::channelId()
            {
                return mProxied->getChannelId();
            }

            QString ChatChannelProxy::groupGuid()
            {
                return mProxied->getGroupGuid();
            }

            bool ChatChannelProxy::locked()
            {
                return mProxied->isLocked();
            }

            QString ChatChannelProxy::status()
            {
                return mProxied->getStatus();
            }

            QString ChatChannelProxy::role()
            {
                return mProxied->role();
            }

            QObjectList ChatChannelProxy::channelMembers()
            {
                QObjectList ret; 
#if 0 //disable for OriginX
                for(QVector<Chat::JabberID>::ConstIterator it = mProxied->getChannelUsers().constBegin();
                    it != mProxied->getChannelUsers().constEnd();
                    ++it)
                {
                    QObject *proxy = NULL;
                    OriginSocialProxy* osp = OriginSocialProxy::instance();
                    if (osp)
                    {
                        proxy = osp->userProxyByJabberId(*it);
                    }

                    if (proxy)
                    {
                        ret.append(proxy);
                    }
                }
#endif
                return ret;
            }

            bool ChatChannelProxy::inChannel()
            {
                return mProxied->isUserInChannel();
            }

            QString ChatChannelProxy::invitedBy()
            {
                return mProxied->invitedBy();
            }

        }
    }
}