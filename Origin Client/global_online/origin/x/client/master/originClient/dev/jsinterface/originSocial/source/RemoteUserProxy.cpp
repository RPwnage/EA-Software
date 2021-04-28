// *********************************************************************
//  RemoteUserProxy.cpp
//  Copyright (c) 2013 Electronic Arts Inc. All rights reserved.
// **********************************************************************

#include "RemoteUserProxy.h"

#include <QRegExp>

#include "services/debug/DebugService.h"
#include "services/rest/AtomServiceClient.h"

#include "chat/Roster.h"
#include "chat/BlockList.h"
#include "chat/ConnectedUser.h"
#include "chat/Conversable.h"
#include "chat/OriginConnection.h"
#include "chat/ChatGroup.h"

#include "TelemetryAPIDLL.h"

#include "engine/login/LoginController.h"
#include "engine/social/SocialController.h"
#include "engine/social/AvatarManager.h"
#include "engine/social/UserNamesCache.h"
#include "engine/multiplayer/InviteController.h"
#include "engine/multiplayer/JoinOperation.h"
#include "engine/igo/IGOController.h"

#include "ClientFlow.h"
#include "JoinMultiplayerFlow.h"
#include "widgets/social/source/SocialViewController.h"
#include "ShareAchievementsQueryOperationProxy.h"

#include "ConversionHelpers.h"
#include "OriginSocialProxy.h"
#include "ChatGroupProxy.h"
#include "services/log/LogService.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            using Engine::Social::SocialController;

            RemoteUserProxy::RemoteUserProxy(OriginSocialProxy *socialProxy, SocialController *socialController, int id, Chat::RemoteUser *proxied) :
                SocialUserProxy(socialProxy, socialController, id, proxied),
                mProxied(proxied),
                mBlockList(socialController->chatConnection()->currentUser()->blockList()),
                mBlocked(false)
            {
                // If we're being exposed through JS we're likely interested in the avatar
                // For contacts we'll already be pre-subscribed by SocialController but for non-contacts this lets us request 
                // it at the last moment
                socialController->smallAvatarManager()->subscribe(mProxied->nucleusId());

                ORIGIN_VERIFY_CONNECT(mBlockList, SIGNAL(loaded()), this, SLOT(blockListChanged()));
                ORIGIN_VERIFY_CONNECT(mBlockList, SIGNAL(updated()), this, SLOT(blockListChanged()));

                ORIGIN_VERIFY_CONNECT(proxied, SIGNAL(subscriptionStateChanged(Origin::Chat::SubscriptionState, Origin::Chat::SubscriptionState)), 
                    this, SIGNAL(subscriptionStateChanged()));

                ORIGIN_VERIFY_CONNECT(proxied, SIGNAL(addedToRoster()), this, SIGNAL(addedToRoster()));
                ORIGIN_VERIFY_CONNECT(proxied, SIGNAL(removedFromRoster()), this, SIGNAL(removedFromRoster()));

                ORIGIN_VERIFY_CONNECT(this, SIGNAL(anyChange(quint64)), proxied, SIGNAL(anyChange()));
                ORIGIN_VERIFY_CONNECT(this, SIGNAL(promoteToAdmin(const QString&)), proxied, SLOT(promoteToAdmin(const QString&)));
                ORIGIN_VERIFY_CONNECT(this, SIGNAL(demoteToMember(const QString&)), proxied, SLOT(demoteToMember(const QString&)));

                ORIGIN_VERIFY_CONNECT(proxied, SIGNAL(capabilitiesChanged()), this, SIGNAL(capabilitiesChanged()));

                ORIGIN_VERIFY_CONNECT(proxied, SIGNAL(transferOwnershipSuccess(const QString&)), this, SLOT(onTransferOwnershipSuccess(const QString&)));
                ORIGIN_VERIFY_CONNECT(proxied, SIGNAL(transferOwnershipFailure(const QString&)), this, SLOT(onTransferOwnershipFailure(const QString&)));
                ORIGIN_VERIFY_CONNECT(proxied, SIGNAL(promoteToAdminSuccess(QObject*)), this, SLOT(onPromoteToAdminSuccess()));
                ORIGIN_VERIFY_CONNECT(proxied, SIGNAL(promoteToAdminFailure(QObject*, Services::GroupResponse::GroupResponseError)), this, SLOT(onPromoteToAdminFailure(QObject*, Services::GroupResponse::GroupResponseError)));
                ORIGIN_VERIFY_CONNECT(proxied, SIGNAL(demoteToMemberSuccess(QObject*)), this, SLOT(onDemoteToMemberSuccess()));

                blockListChanged();
            }

            void RemoteUserProxy::startConversation()
            {
                // Start the conversation UI
                ClientFlow::instance()->socialViewController()->chatWindowManager()->startConversation(mProxied, Engine::Social::Conversation::InternalRequest, Engine::IIGOCommandController::CallOrigin_CLIENT);
            }

            void RemoteUserProxy::startVoiceConversation()
            {
                // Start the conversation UI
                ClientFlow::instance()->socialViewController()->chatWindowManager()->startConversation(mProxied, Engine::Social::Conversation::StartVoice, Engine::IIGOCommandController::CallOrigin_CLIENT);
            }

            bool RemoteUserProxy::blocked()
            {
                return mBlocked;
            }

            void RemoteUserProxy::refreshBlockList()
            {
                socialController()->chatConnection()->currentUser()->blockList()->forceRefresh();
            }

            void RemoteUserProxy::inviteToGame()
            {
                socialController()->multiplayerInvites()->inviteToLocalSession(mProxied->jabberId());
            }

            void RemoteUserProxy::joinGame()
            {
                JoinMultiplayerFlow *flow = new JoinMultiplayerFlow;

                ORIGIN_VERIFY_CONNECT(flow, SIGNAL(finished(bool)), flow, SLOT(deleteLater()));
                flow->start(socialController(), mProxied->jabberId());
            }

            QVariantMap RemoteUserProxy::subscriptionState()
            {
                return ConversionHelpers::subscriptionStateToDict(mProxied->subscriptionState());
            }

            QString RemoteUserProxy::capabilities() const
            {
                return mProxied->capabilities();
            }

            void RemoteUserProxy::block()
            {
                ClientFlow::instance()->socialViewController()->blockContactWithConfirmation(mProxied);
            }
            
            void RemoteUserProxy::removeAndBlock()
            {
                // This should be removed once we know that no one is using this call
               block();
            }

            void RemoteUserProxy::unblock()
            {
                mBlockList->removeJabberID(mProxied->jabberId().asBare());
                GetTelemetryInterface()->Metric_FRIEND_BLOCK_REMOVE_SENT();
            }

            void RemoteUserProxy::blockListChanged()
            {
                const bool wasBlocked = mBlocked;

                // Find out if we're blocked now
                Chat::ConnectedUser *currentUser = socialController()->chatConnection()->currentUser();
                mBlocked = currentUser->blockList()->jabberIDs().contains(mProxied->jabberId());

                // Emit a signal if we're changed
                if (wasBlocked != mBlocked)
                {
                    emit blockChanged(mBlocked);
                }
            }

            void RemoteUserProxy::acceptSubscriptionRequest()
            {
                Chat::Roster *roster = socialController()->chatConnection()->currentUser()->roster();
                roster->approveSubscriptionRequest(mProxied->jabberId());

                bool isInIgo = Engine::IGOController::instance() && Engine::IGOController::instance()->isActive();

                GetTelemetryInterface()->Metric_FRIEND_INVITE_ACCEPTED(isInIgo);
            }

            void RemoteUserProxy::rejectSubscriptionRequest()
            {
                Chat::Roster *roster = socialController()->chatConnection()->currentUser()->roster();
                roster->denySubscriptionRequest(mProxied->jabberId());

                bool isInIgo = Engine::IGOController::instance() && Engine::IGOController::instance()->isActive();

                GetTelemetryInterface()->Metric_FRIEND_INVITE_IGNORED(isInIgo);
            }

            void RemoteUserProxy::cancelSubscriptionRequest()
            {
                Chat::Roster *roster = socialController()->chatConnection()->currentUser()->roster();
                roster->cancelSubscriptionRequest(mProxied->jabberId());
            }

            void RemoteUserProxy::requestSubscription()
            {
                bool isInIgo = Engine::IGOController::instance() && Engine::IGOController::instance()->isActive();

                ORIGIN_LOG_EVENT << "User sent friend invite from Profile search";
                GetTelemetryInterface()->Metric_FRIEND_INVITE_SENT("profile Search", isInIgo);

                Chat::Roster *roster = socialController()->chatConnection()->currentUser()->roster();
                roster->requestSubscription(mProxied->jabberId());
            }

            quint64 RemoteUserProxy::nucleusPersonaId()
            {
                return mProxied->nucleusPersonaId();
            }

            QVariant RemoteUserProxy::nickname()
            {
                const QString originId = mProxied->originId();

                if (originId.isNull())
                {
                    // This should never get hit, but just in case
                    // We want an empty string rather than a null variant
                    return QVariant("");
                }

                return originId;
            }

            QVariant RemoteUserProxy::statusText()
            {
                QString statusText(mProxied->presence().statusText());

                if (statusText.isNull())
                {
                    return QVariant();
                }
                else if (statusText.contains('|'))
                {
                    // Remove Origin <= 9.2 encoded content IDs from the status
                    // This is gross and can be killed once 9.3 goes required
                    statusText.replace(QRegExp("^[a-zA-Z0-9:\\-_]+\\|"), "");
                }

                return statusText;
            }

            QString RemoteUserProxy::jabberId() const
            {
                QString jabberIdText(mProxied->jabberId().toEncoded());
                return jabberIdText;
            }

            void RemoteUserProxy::reportTosViolation()
            {
                ClientFlow::instance()->socialViewController()->reportTosViolation(mProxied);
            }

            Q_INVOKABLE  QObject * RemoteUserProxy::achievementSharingState()
            {
                return new ShareAchievementsQueryOperationProxy(this, nucleusId());
            }

            void RemoteUserProxy::promoteUserToAdmin(const QString& groupGuid)
            {
                emit(promoteToAdmin(groupGuid));
            }

            void RemoteUserProxy::transferGroupOwnership(const QString& groupGuid)
            {
                ClientFlow::instance()->showTransferOwnershipConfirmationDialog(groupGuid, this->proxied(), this->nickname().toString(), ( Engine::IGOController::instance()->isActive() ? IGOScope : ClientScope ));
            }

            void RemoteUserProxy::demoteAdminToMember(const QString& groupGuid)
            {
                emit (demoteToMember(groupGuid));
            }

            void RemoteUserProxy::onPromoteToAdminSuccess()
            {
                emit(promoteToAdminSuccess(this));
            }

            void RemoteUserProxy::onPromoteToAdminFailure(QObject* user, Services::GroupResponse::GroupResponseError errorCode)
            {
                // Show failure dialog, except for case of silent failure such as user is already admin
                if (errorCode != Services::GroupResponse::GroupUserAlreadyAdminError)
                {
                    ClientFlow::instance()->showPromoteToAdminFailureDialog(this->nickname().toString(), ( Engine::IGOController::instance()->isActive() ? IGOScope : ClientScope ));
                }
            }

            void RemoteUserProxy::onTransferOwnershipFailure(const QString& groupGuid)
            {
                // Show failure dialog
                ClientFlow::instance()->showTransferOwnershipFailureDialog(this->nickname().toString(), ( Engine::IGOController::instance()->isActive() ? IGOScope : ClientScope ));
            }

            void RemoteUserProxy::onTransferOwnershipSuccess(const QString& groupGuid)
            {
#if 0 //disable for Origin X
                emit(transferOwnershipSuccess(this));

                // Show success dialog
                JsInterface::OriginSocialProxy* social = JsInterface::OriginSocialProxy::instance();
                if (!social)
                {
                    ORIGIN_LOG_ERROR<< "OriginSocialProxy not available";
                    return;
                }
                JsInterface::ChatGroupProxy* chatGroupProxy = social->groupProxyByGroupId(groupGuid);

                ClientFlow::instance()->showTransferOwnershipSuccessDialog(chatGroupProxy->proxied()->getName(), this->nickname().toString(), ( Engine::IGOController::instance()->isActive() ? IGOScope : ClientScope ));
#endif
            }

            void RemoteUserProxy::onDemoteToMemberSuccess()
            {
                emit(demoteToMemberSuccess(this));
            }

        }
    }
}
