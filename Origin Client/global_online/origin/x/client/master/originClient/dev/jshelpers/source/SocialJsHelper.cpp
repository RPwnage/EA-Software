#include <QMetaObject>
#include <QDesktopWidget>
#include "SocialJsHelper.h"
#include "flows/ClientFlow.h"
#include "TelemetryAPIDLL.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "Qt/originwindow.h"
#include "Qt/originwebview.h"
#include "Qt/originwindowmanager.h"
#include "OriginWebController.h"
#include "services/debug/DebugService.h"
#include "engine/content/ContentController.h"
#include "widgets/social/source/SocialViewController.h"

#include "engine/social/SocialController.h"
#include "engine/igo/IGOController.h"

#include "chat/OriginConnection.h"
#include "chat/ConnectedUser.h"
#include "chat/BlockList.h"
#include "chat/Roster.h"

// when you convert, this will need to change!
#include "Qt/originwindow.h"
#include "Qt/originmsgbox.h"

namespace Origin
{
    namespace Client
    {

        SocialJsHelper::SocialJsHelper(Engine::UserRef user, QObject *parent, UIScope scope)
            : QObject(parent)
            ,mUser(user)
            ,mWindow(NULL)
			,mWindowWebController(NULL)
            ,mScope(scope)
        {
        }

        // Show result web pages

        void SocialJsHelper::showWindow(const QString& url)
        {
            if (mWindow == NULL)
            {
                using namespace Origin::UIToolkit;
                OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close;
                OriginWindow::WindowContext windowContext = OriginWindow::OIG;

                if (mScope == ClientScope)
                {
                    titlebarItems |= OriginWindow::Minimize;
                    windowContext = OriginWindow::Normal;
                }

                mWindow = new OriginWindow(titlebarItems, NULL, OriginWindow::WebContent, QDialogButtonBox::NoButton,
                    OriginWindow::ChromeStyle_Dialog, windowContext);
                ORIGIN_VERIFY_CONNECT(mWindow, SIGNAL(rejected()), this, SLOT(closeWindow()));
                mWindow->webview()->setHasSpinner(true);
                mWindow->webview()->setIsContentSize(false);
                mWindowWebController = new OriginWebController(mWindow->webview()->webview());
                mWindowWebController->loadTrustedUrl(url);
            }

            if (mScope == IGOScope && mWindow)
            {
                Engine::IIGOWindowManager::WindowBehavior behavior;
                behavior.alwaysVisible = true;
                Engine::IGOController::instance()->igowm()->addPopupWindow(mWindow, Engine::IIGOWindowManager::WindowProperties(), behavior);
            }
            else
            {
                mWindow->show();
            }
        }

        void SocialJsHelper::closeWindow()
        {
            mWindowWebController = NULL;

            if(mWindow)
            {
                ORIGIN_VERIFY_DISCONNECT(mWindow, SIGNAL(rejected()), this, SLOT(closeWindow()));
                mWindow->close();
                mWindow = NULL;
            }
        }

        void SocialJsHelper::showSearchFriends()
        {
            // TODO: SocialJsHelper::showSearchFriends()
        }

        void SocialJsHelper::showChatWindowForFriend(const QString &userId)
        {
            Chat::OriginConnection *conn = mUser->socialControllerInstance()->chatConnection();
            const Chat::JabberID jid(conn->nucleusIdToJabberId(userId.toULongLong()));

            Chat::RemoteUser *remoteUser = conn->remoteUserForJabberId(jid);

            if (remoteUser)
            {
                ClientFlow::instance()->socialViewController()->chatWindowManager()->startConversation(remoteUser, Engine::Social::Conversation::InternalRequest, Engine::IIGOCommandController::CallOrigin_CLIENT);
            }
        }

        void SocialJsHelper::showFriendResultURL(const QString &searchKeyword)
        {
            ClientFlow::instance()->showProfileSearchResult(searchKeyword, mScope);

            QWidget* parentContainer = static_cast<QWidget*>( parent() );
            if( parentContainer ) 
            {
                parentContainer->close();
            }
        }

        void SocialJsHelper::showProductPage(const QString &productId)
        {
            ClientFlow::instance()->showProductIDInStore(productId);
        }

        // Friendship actions
        void SocialJsHelper::addFriend(const QString & userId, const QString &source)
        {
            bool isInIgo = Engine::IGOController::instance()->isActive();

            ORIGIN_LOG_EVENT << "User sent friend invite from " << source;
            GetTelemetryInterface()->Metric_FRIEND_INVITE_SENT(source.toUtf8().data(), isInIgo);

            // Find our Jabber ID
            Chat::OriginConnection *conn = mUser->socialControllerInstance()->chatConnection();
            const Chat::JabberID jid(conn->nucleusIdToJabberId(userId.toULongLong()));

            conn->currentUser()->roster()->requestSubscription(jid, source);
        }

        void SocialJsHelper::deleteFriend(const QString& userId)
        {
            Chat::OriginConnection *conn = mUser->socialControllerInstance()->chatConnection();
            const Chat::JabberID jid(conn->nucleusIdToJabberId(userId.toULongLong()));

            Chat::RemoteUser *remoteUser = conn->remoteUserForJabberId(jid);

            if (remoteUser && conn->currentUser()->roster()->contacts().contains(remoteUser))
            {
                ClientFlow::instance()->socialViewController()->removeContactWithConfirmation(remoteUser); 
            }
            else
            {
                using namespace Origin::UIToolkit;

                OriginWindow* messageBox = new OriginWindow(
                    (OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
                    NULL, OriginWindow::MsgBox, QDialogButtonBox::Close);

                messageBox->msgBox()->setup(OriginMsgBox::Info, 
                    tr("ebisu_client_error_delete_friend_title"),
                    tr("ebisu_client_error_delete_friend_text"));

                messageBox->setButtonText(QDialogButtonBox::Close, tr("ebisu_client_finish"));

                ORIGIN_VERIFY_CONNECT(messageBox, SIGNAL(rejected()), messageBox, SLOT(close()));
                ORIGIN_VERIFY_CONNECT(messageBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), messageBox, SLOT(close()));

                messageBox->manager()->setupButtonFocus();

                messageBox->exec();
            }
        }

        void SocialJsHelper::blockFriend(const QString& userId, const QString& friendName)
        {
            ClientFlow::instance()->socialViewController()->blockUserWithConfirmation(userId.toULongLong());
        }

        void SocialJsHelper::unblockFriend(const QString& userId)
        {
            Chat::OriginConnection *connection = mUser->socialControllerInstance()->chatConnection();
            Chat::ConnectedUser *currentUser = connection->currentUser();

            currentUser->blockList()->removeJabberID(connection->nucleusIdToJabberId(userId.toULongLong()));
        }

        void SocialJsHelper::reportFriend(const QString& /*friendID*/, const QString& /*friendName*/)
        {
            //    Callback for when a report friend request is sent.
            GetTelemetryInterface()->Metric_FRIEND_REPORT_SENT();
        }

        void SocialJsHelper::acceptFriendRequest(const QString& userId)
        {
            Chat::OriginConnection *conn = mUser->socialControllerInstance()->chatConnection();
            const Chat::JabberID jid(conn->nucleusIdToJabberId(userId.toULongLong()));

            conn->currentUser()->roster()->approveSubscriptionRequest(jid);
        }

        bool SocialJsHelper::isPendingFriend(const QString& userId)
        {
            Chat::OriginConnection *conn = mUser->socialControllerInstance()->chatConnection();
            const Chat::JabberID jid(conn->nucleusIdToJabberId(userId.toULongLong()));

            Chat::RemoteUser *remoteUser = conn->remoteUserForJabberId(jid);

            if (remoteUser)
            {
                return remoteUser->subscriptionState().isPendingContactApproval();
            }

            return false;
        }

        void SocialJsHelper::inviteToMultiplayer(const QString& to, const QString& from, const QString& message)
        {
            // SRTODO
        }

        // Display friends pages

        void SocialJsHelper::onCurrentWidgetChanged(int)
        {
            mProfileViewingUserId.clear();
        }

        void SocialJsHelper::showUserProfile()
        {
            ClientFlow::instance()->showMyProfile(Engine::IIGOCommandController::CallOrigin_CLIENT);
        }

        void SocialJsHelper::showFriendProfile(const QString& userId)
        {
            ClientFlow::instance()->showProfile(userId.toULongLong());
        }

        void SocialJsHelper::viewProfilePage(const QString& userId)
        {
            //    TELEMETRY:  Notify when profile is received
            bool isInIgo = Engine::IGOController::instance()->isActive();
            GetTelemetryInterface()->Metric_USER_PROFILE_VIEW(isInIgo?"OIG":"Client", "UNKNOWN", "UNKNOWN");

            ClientFlow::instance()->setInSocial(userId);

            // store the nucleusId of the user whose profile is being viewed.
            mProfileViewingUserId = userId;
        }

        bool SocialJsHelper::isTriggeredFromIGO()
        {
            return mScope == IGOScope;
        }

    }
}


