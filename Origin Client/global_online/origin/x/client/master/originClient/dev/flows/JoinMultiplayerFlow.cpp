#include "JoinMultiplayerFlow.h"

#include "services/debug/DebugService.h"

#include "chat/OriginConnection.h"
#include "chat/OriginGameActivity.h"

#include "engine/social/SocialController.h"
#include "engine/multiplayer/InviteController.h"
#include "engine/igo/IGOController.h"

#include "Qt/originwindow.h"                                                                                            
#include "Qt/originwindowmanager.h"  

namespace Origin
{
namespace Client
{

using Engine::MultiplayerInvite::JoinOperation;

JoinMultiplayerFlow::JoinMultiplayerFlow() : 
	mSocialController(NULL),
	mXmppUser(NULL),
	mRetryDialog(NULL)
{
}
    
void JoinMultiplayerFlow::start(Engine::Social::SocialController *socialController, const Chat::JabberID &jabberId)
{
    // Store these away in case we need to restart ourselves
    mSocialController = socialController;
    mXmppUser = socialController->chatConnection()->userForJabberId(jabberId);
    mRetryDialog = NULL;
    
    attemptJoin();
}

void JoinMultiplayerFlow::attemptJoin()
{
    // Make sure there are no conflicting join operations
    JoinOperation *existingOp = mSocialController->multiplayerInvites()->activeJoinOperation();

    if (existingOp)
    {
        if (existingOp->jabberId().asBare() == mXmppUser->jabberId().asBare())
        {
            // We're already trying to join this session
            emit finished(false);
            return;
        }

        // We're already attempting to join - kill the last attempt silently
        existingOp->abort();
    }

    JoinOperation *op = mSocialController->multiplayerInvites()->joinRemoteSession(mXmppUser->jabberId());

    if (!op)
    {
        emit finished(false);
        return;
    }

    ORIGIN_VERIFY_CONNECT(op, SIGNAL(finished()), op, SLOT(deleteLater()));

    ORIGIN_VERIFY_CONNECT(
            op, SIGNAL(waitingForPlayingGameExit(Origin::Engine::Content::EntitlementRef)),
            this, SLOT(waitingForPlayingGameExit(Origin::Engine::Content::EntitlementRef)));

    ORIGIN_VERIFY_CONNECT(
            op, SIGNAL(failed(Origin::Engine::MultiplayerInvite::JoinOperation::FailureReason, const QString &)),
            this, SLOT(operationFailed(Origin::Engine::MultiplayerInvite::JoinOperation::FailureReason, const QString &)));
    
    ORIGIN_VERIFY_CONNECT(
            op, SIGNAL(succeeded()),
            this, SLOT(operationSucceeded()));
}

void JoinMultiplayerFlow::finishWithFailure()
{
    emit finished(false);
}
    
void JoinMultiplayerFlow::operationFailed(Origin::Engine::MultiplayerInvite::JoinOperation::FailureReason reason, const QString &productId)
{
    using namespace Origin::UIToolkit;

    const bool inGame = Engine::IGOController::instance()->igowm()->visible();

    OriginWindow *dialog = NULL;
    bool finishFlow = true;

    switch(reason)
    {
    case JoinOperation::RemoteTimeoutFailure:
    case JoinOperation::RemoteDeclinedFailure:
        {
            // Allow the user to retry
            dialog = new OriginWindow((OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
                NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
            dialog->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_join_game_error_title"),
                tr("ebisu_client_join_game_error_body"));
            dialog->manager()->setupButtonFocus();

            dialog->setButtonText(QDialogButtonBox::Ok, tr("ebisu_client_accept_invite"));

            ORIGIN_VERIFY_CONNECT(dialog->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(accept()));
            ORIGIN_VERIFY_CONNECT(dialog->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), dialog, SLOT(reject()));

            // If they close the dialog or click cancel just fail ourselves
            ORIGIN_VERIFY_CONNECT(dialog, SIGNAL(rejected()), this, SLOT(finishWithFailure()));

            // If they click retry then effectively restart the flow from the beginning
            ORIGIN_VERIFY_CONNECT(dialog, SIGNAL(accepted()), this, SLOT(attemptJoin()));

            // In either case destroy this dialog
            ORIGIN_VERIFY_CONNECT(dialog, SIGNAL(finished(int)), this, SLOT(clearRetryDialog()));

            // Update the retry button based on the joinability of the session 
            mRetryDialog = dialog;
            startRetryDialogPresenceChecks();

            // Don't finish the flow while the user has the option of retrying
            finishFlow = false;
        }

        break;

    case JoinOperation::NoOwnedEntitlementFailure:
        {
            // Content is not owned.
            const QString appName = qApp->applicationName();

            dialog = new OriginWindow((OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
                NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok);
            dialog->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_not_entitled_game_invite_title"),
                inGame ? tr("ebisu_client_not_entitled_game_invite_in_game").arg(appName) : tr("ebisu_client_not_entitled_game_invite").arg(appName));
            dialog->manager()->setupButtonFocus();

            ORIGIN_VERIFY_CONNECT(dialog, SIGNAL(rejected()), dialog, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(dialog->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(close()));
        }

        break;

    case JoinOperation::UnplayableEntitlementFailure:
        {
            // If we own the content, show them a message box saying it needs to be installed.
            dialog = new OriginWindow((OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
                NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok);
            dialog->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_not_installed_game_invite_title"), tr("ebisu_client_not_installed_game_game_invite_out_of_game"));
            dialog->manager()->setupButtonFocus();

            ORIGIN_VERIFY_CONNECT(dialog, SIGNAL(rejected()), dialog, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(dialog->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(close()));
        }

        break;
    
    default:
        // Ignore this
        break;
    }

    if (dialog != NULL)
    {
        // Show it in the right place
        if (inGame)
        {
            Engine::IGOController::instance()->igowm()->addPopupWindow(dialog, Engine::IIGOWindowManager::WindowProperties());
        }
        else
        {
            dialog->show();
        }
    }

    if (finishFlow)
    {
        emit finished(false);
    }
}
    
void JoinMultiplayerFlow::startRetryDialogPresenceChecks()
{
    ORIGIN_VERIFY_CONNECT(
            mXmppUser, SIGNAL(presenceChanged(const Origin::Chat::Presence &, const Origin::Chat::Presence &)),
            this, SLOT(updateRetryDialogWithPresence(const Origin::Chat::Presence &))); 

    // Make sure this is initially 
    updateRetryDialogWithPresence(mXmppUser->presence());
}

void JoinMultiplayerFlow::clearRetryDialog()
{
    ORIGIN_VERIFY_DISCONNECT(
            mXmppUser, SIGNAL(presenceChanged(const Origin::Chat::Presence &, const Origin::Chat::Presence &)),
            this, SLOT(updateRetryDialogWithPresence(const Origin::Chat::Presence &))); 

    mRetryDialog->deleteLater();
    mRetryDialog = NULL;
}

void JoinMultiplayerFlow::operationSucceeded()
{
    emit finished(true);
}
    
void JoinMultiplayerFlow::updateRetryDialogWithPresence(const Origin::Chat::Presence &presence)
{
    ORIGIN_ASSERT(mRetryDialog != NULL);

    mRetryDialog->button(QDialogButtonBox::Ok)->setEnabled(presence.gameActivity().joinable());
}

void JoinMultiplayerFlow::waitingForPlayingGameExit(Origin::Engine::Content::EntitlementRef toLaunch)
{
    using namespace Origin::UIToolkit;

    Engine::MultiplayerInvite::JoinOperation *joinOp = dynamic_cast<Engine::MultiplayerInvite::JoinOperation*>(sender());

    if (!joinOp)
    {
        return;
    }

    OriginWindow* cancelDialog = new OriginWindow((OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
        NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    cancelDialog->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_quit_current_game_invite_title"), tr("ebisu_client_quit_current_game_invite").arg(toLaunch->contentConfiguration()->displayName()));

    cancelDialog->manager()->setupButtonFocus();

	ORIGIN_VERIFY_CONNECT(cancelDialog->button(QDialogButtonBox::Ok), SIGNAL(clicked()), cancelDialog, SLOT(accept()));
	ORIGIN_VERIFY_CONNECT(cancelDialog->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), joinOp, SLOT(abort()));
    ORIGIN_VERIFY_CONNECT(cancelDialog->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), cancelDialog, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(cancelDialog, SIGNAL(rejected()), joinOp, SLOT(abort()));
    ORIGIN_VERIFY_CONNECT(cancelDialog, SIGNAL(rejected()), cancelDialog, SLOT(close()));

	// if we don't raise this dialog it could be covered by the IGO window
	cancelDialog->raise();

	//If the IGO is displayed then show this message in IGO
	if (Engine::IGOController::instance()->igowm()->visible())
	{
		Engine::IGOController::instance()->igowm()->addPopupWindow(cancelDialog, Engine::IIGOWindowManager::WindowProperties());
	}
	else
	{
		// Else show it on the client
		cancelDialog->exec();
	}
}
    
}
}
