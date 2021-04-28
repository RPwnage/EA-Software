#include "UserContactsQueryOperationProxy.h"

#include "services/rest/AtomServiceResponses.h"
#include "services/rest/AtomServiceClient.h"
#include "services/debug/DebugService.h"

#include "chat/ConnectedUser.h"
#include "chat/Roster.h"
#include "chat/RemoteUser.h"
#include "chat/OriginConnection.h"

#include "engine/social/SocialController.h"
#include "engine/social/UserNamesCache.h"
#include "engine/login/LoginController.h"

#include "OriginSocialProxy.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            using Engine::UserRef;

            UserContactsQueryOperationProxy::UserContactsQueryOperationProxy(QObject *parent, OriginSocialProxy *socialProxy, quint64 nucleusId) :
                QueryOperationProxyBase(parent)
                ,mSocialProxy(socialProxy)
                ,mNucleusID(nucleusId)
            {
            }

            void UserContactsQueryOperationProxy::getUserFriends()
            {
                UserRef user = Engine::LoginController::instance()->currentUser(); 

                if (!user)
                {
                    // Need a user
                    QMetaObject::invokeMethod(this, "failed", Qt::QueuedConnection);
                    return;
                }

                if (static_cast<quint64>(user->userId()) == mNucleusID)
                {
                    Chat::Roster *roster = user->socialControllerInstance()->chatConnection()->currentUser()->roster();

                    if (roster->hasLoaded())
                    {
                        // Send our roster once JS has a chance to connect our signals
                        QMetaObject::invokeMethod(this, "sendCurrentUserRoster", Qt::QueuedConnection);
                    }
                    else
                    {
                        // Wait for the roster to load
                        ORIGIN_VERIFY_CONNECT(roster, SIGNAL(loaded()), this, SLOT(sendCurrentUserRoster()));
                    }
                }
                else
                {
                    Services::UserFriendsResponse *resp = Services::AtomServiceClient::userFriends(user->getSession(), mNucleusID);

                    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(success()), this, SLOT(onUserFriendsSuccess()));
                    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(failQuery()));
                    ORIGIN_VERIFY_CONNECT(resp, SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onQueryError(Origin::Services::HttpStatusCode)));
                }
            }

            void UserContactsQueryOperationProxy::sendCurrentUserRoster()
            {
#if 0 //disable for Origin X
                UserRef user = Engine::LoginController::instance()->currentUser(); 

                QObjectList contactProxies;

                Chat::OriginConnection *chatConn = user->socialControllerInstance()->chatConnection();
                QSet<Chat::RemoteUser*> contacts = chatConn->currentUser()->roster()->contacts();

                for(QSet<Chat::RemoteUser*>::ConstIterator it = contacts.constBegin();
                    it != contacts.constEnd();
                    it++)
                {
                    Chat::RemoteUser *remoteUser = *it;

                    // This is consistent with what the Atom API does
                    if (remoteUser->subscriptionState().direction() == Chat::SubscriptionState::DirectionBoth)
                    {
                        contactProxies << mSocialProxy->userProxyByJabberId((*it)->jabberId());
                    }
                }

                emit succeeded(contactProxies);
                deleteLater();
#endif
            }

            void UserContactsQueryOperationProxy::onUserFriendsSuccess()
            {
#if 0 //disable for Origin X
                Services::UserFriendsResponse *resp = dynamic_cast<Services::UserFriendsResponse*>(sender());
                UserRef user = Engine::LoginController::instance()->currentUser(); 

                if (!resp || !user)
                {
                    ORIGIN_ASSERT(0);
                    deleteLater();
                    return;
                }

                // Give these instances less cumbersome names
                Engine::Social::UserNamesCache *userNames = user->socialControllerInstance()->userNames();
                Chat::OriginConnection *chatConn = user->socialControllerInstance()->chatConnection();

                QObjectList contactProxies;

                QList<Services::UserFriendData> friends = resp->userFriends();

                for(QList<Services::UserFriendData>::ConstIterator it = friends.constBegin();
                    it != friends.constEnd();
                    it++)
                {
                    // Cache this user name
                    userNames->setNamesForNucleusId(it->userId, Engine::Social::UserNames(it->EAID, it->firstName, it->lastName));

                    Chat::JabberID jid = chatConn->nucleusIdToJabberId(it->userId);

                    // Update their Nucleus persona ID
                    chatConn->remoteUserForJabberId(jid)->setNucleusPersonaId(it->personaId);

                    if (jid.isValid())
                    {
                        contactProxies << mSocialProxy->userProxyByJabberId(jid);
                    }
                }

                emit succeeded(contactProxies);

                resp->deleteLater();
                deleteLater();
#endif
            }

        }
    }
}
