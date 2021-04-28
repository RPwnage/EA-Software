// *********************************************************************
//  SocialViewController.cpp
//  Copyright (c) 2012-2013 Electronic Arts Inc. All rights reserved.
// **********************************************************************

#include "SocialViewController.h"
#include "OriginChatProxy.h"
#include "RemoteUserProxy.h"

#include "RemoveContactDialog.h"
#include "EnteredEmptyMucRoomDialog.h"
#include "MainFlow.h"

#include "TelemetryAPIDLL.h"

#include "flows/ClientFlow.h"
#include "flows/SummonedToGroupChannelFlow.h"
#include "flows/SummonToGroupChannelFlow.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/voice/VoiceService.h"

#include "chat/RemoteUser.h"
#include "chat/ConnectedUser.h"
#include "chat/OriginConnection.h"
#include "chat/BlockList.h"
#include "chat/RemoteUser.h"
#include "chat/Roster.h"
#include "chat/ChatChannel.h"

#include "engine/content/ProductInfo.h"
#include "engine/social/SocialController.h"
#include "engine/social/ConversationManager.h"
#include "engine/voice/VoiceController.h"
#include "engine/voice/VoiceClient.h"
#include "engine/login/LoginController.h"
#include "engine/social/UserNamesCache.h"
#include "engine/igo/IGOController.h"
#include "engine/multiplayer/InviteController.h"

#include "Qt/originwindow.h"
#include "Qt/originwindowmanager.h"
#include "Qt/originmsgbox.h"
#include "Qt/origincheckbutton.h"
#include "flows/ClientFlow.h"

#include "OriginToastManager.h"
#include "OriginNotificationDialog.h"
    
#include "ReportUserWidget.h"
#include "services/rest/IdentityUserProfileService.h"
#include "services/rest/AtomServiceClient.h"
#include "engine/social/UserAvailabilityController.h"
#include "engine/login/User.h"

#include <QApplication>
#include <QDockWidget>
#include <QPixMap>
#include <QScopedPointer>
#include <QStyleOption>
#include <QStylePainter>
#include <QVBoxLayout>

namespace {

#if ENABLE_VOICE_CHAT

    /// \brief A simple strip chart widget for graphically debugging the voice client.
    class StripChart : public QWidget {

    public:

        StripChart(QWidget* parent = NULL, Origin::Engine::Voice::VoiceClient* voiceClient = NULL, int width = 0, int height = 0)
            : QWidget(parent)
            , _voiceClient(voiceClient)
            , _pixels(new QPixmap(width, height))
            , _pixelPainter(_pixels.data())
            , _timer(this)
            , _width(width)
            , _height(height)
        {
            setFixedSize(width, height);
            _pixels->fill(Qt::white);

            ORIGIN_VERIFY_CONNECT_EX(&_timer, &QTimer::timeout, this, &StripChart::onTimeout, Qt::QueuedConnection);
            _timer.start(500);
        }

        void paintEvent(QPaintEvent* /*event*/)
        {
            QPainter painter(this);
            painter.drawPixmap(0, 0, *_pixels);

            //if (hasFocus()) {
            //    QStylePainter spainter(this);
            //    QStyleOptionFocusRect option;
            //    option.initFrom(this);
            //    option.backgroundColor = palette().dark().color();
            //    spainter.drawPrimitive(QStyle::PE_FrameFocusRect, option);
            //}
        }

        void drawGrid(QPainter* painter)
        {
            QPen quiteDark = palette().dark().color().light();
            QPen light = palette().light().color();

            painter->setPen(quiteDark);
            painter->drawRect(50, 50, _width, _height);
        }

        void updateChart() {

            if (_voiceClient.isNull())
                return;

            scroll();

            if (int count = _voiceClient->options().stats().jitterBufferDepth()) {
                plot(count, 50, sJitterBufferDepthColor);
            }

            if (int messagesSent = _voiceClient->options().stats().audioMessagesSent()) {
                static int lastMessagesSent = 0;
                if (messagesSent < lastMessagesSent)
                    lastMessagesSent = 0;
                plot((messagesSent - lastMessagesSent), 50, sMessagesSentColor);
                lastMessagesSent = messagesSent;
            }

            if (int messagesReceived = _voiceClient->options().stats().audioMessagesReceived()) {
                static int lastMessagesReceived = 0;
                if (messagesReceived < lastMessagesReceived)
                    lastMessagesReceived = 0;
                plot((messagesReceived - lastMessagesReceived), 50, sMessagesReceivedColor);
                lastMessagesReceived = messagesReceived;
            }

            if (int messagesLost = _voiceClient->options().stats().audioMessagesLost()) {
                static int lastMessagesLost = 0;
                if (messagesLost < lastMessagesLost)
                    lastMessagesLost = 0;
                plot((messagesLost - lastMessagesLost), 20, sMessagesLostColor);
                lastMessagesLost = messagesLost;
            }
        }

    private slots:

        void onTimeout() {
            updateChart();
            update();
        }

    private:

        void scroll() {
            _pixels->scroll(-1, 0, _pixels->rect());
            _pixelPainter.setPen(Qt::white);
            int h = _width - 1;
            _pixelPainter.drawLine(h, _height, h, 0);
        }

        void plot(const int value, const int normalizer, const QColor& color) {
            if (value) {
                int h = _width - 1;
                int v = _height - _height * value / normalizer;
                _pixelPainter.setPen(color);
                _pixelPainter.drawLine(h, v - 1, h, v);
            }
        }

        static const QColor sJitterBufferDepthColor;
        static const QColor sMessagesSentColor;
        static const QColor sMessagesReceivedColor;
        static const QColor sMessagesLostColor;

        QPointer<const Origin::Engine::Voice::VoiceClient> _voiceClient;
        QScopedPointer<QPixmap> _pixels;
        QPainter _pixelPainter;
        QTimer _timer;
        int _width;
        int _height;
    };

    const int redHue = 0;
    const int yellowHue = 60;
    const int greenHue = 120;
    const int cyanHue = 180;
    const int blueHue = 240;
    const int magentaHue = 300;

    const QColor StripChart::sJitterBufferDepthColor = QColor::fromHsl(yellowHue, 128, 160);
    const QColor StripChart::sMessagesSentColor = QColor::fromHsl(greenHue, 128, 160);
    const QColor StripChart::sMessagesReceivedColor = QColor::fromHsl(blueHue, 128, 180);
    const QColor StripChart::sMessagesLostColor = QColor::fromHsl(redHue, 128, 140);

    QDockWidget* voiceChatDebug = NULL;

#endif // ENABLE_VOICE_CHAT

    const char* PROP_CHANNEL_ID = "chid";
}

namespace Origin
{
namespace Client
{

SocialViewController::SocialViewController(QWidget* parent, Engine::UserRef user) :
    mUser(user)
   ,mNoMoreFriendsAlert(NULL)
   ,mCurrentUserMuted(false)
   ,mVoiceConflictWindow(NULL)
   ,mSurveyPromptWindow(NULL)
   ,mGroupRoomInviteFailedAlert(NULL)
   ,mVoiceConflictHandled(false)
{
    // Chat Window Manager
    mChatWindowManager = new ChatWindowManager(parent);
    mLastNotifiedConversationId = "";

    Chat::OriginConnection *chatConnection = user->socialControllerInstance()->chatConnection();

    ORIGIN_VERIFY_CONNECT(
            chatConnection, SIGNAL(groupRoomInviteReceived(const QString&, const QString&, const QString&)),
            this, SLOT(onGroupRoomInviteReceived(const QString&, const QString&, const QString&)));

    Engine::Social::ConversationManager* conversationManager = user->socialControllerInstance()->conversations();

    // DON'T Use a queued connections here so that this gets handled before the notification has been created
    // This handles the case where the first time you receive a message the notification pops up because the
    // chat window has not been created yet.
    ORIGIN_VERIFY_CONNECT(conversationManager,
        SIGNAL(conversationCreated(QSharedPointer<Origin::Engine::Social::Conversation>)),
        mChatWindowManager,
        SLOT(startConversation(QSharedPointer<Origin::Engine::Social::Conversation>)));

    // and for the Voice Conflict setup
    ORIGIN_VERIFY_CONNECT(conversationManager,
        SIGNAL(conversationCreated(QSharedPointer<Origin::Engine::Social::Conversation>)),
        this,
        SLOT(onStartConversation(QSharedPointer<Origin::Engine::Social::Conversation>)));

    // Listen for game invites
    Engine::MultiplayerInvite::InviteController* inviteController = user->socialControllerInstance()->multiplayerInvites();
    if (inviteController)
    {
        ORIGIN_VERIFY_CONNECT(inviteController,
            SIGNAL(invitedToRemoteSession(const Origin::Engine::MultiplayerInvite::Invite&)),
            this, SLOT(onInvitedToRemoteSession(const Origin::Engine::MultiplayerInvite::Invite&)));
    }

    // Listen for report user coming from a game
    ORIGIN_VERIFY_CONNECT(Origin::Engine::IGOController::instance(),
        SIGNAL(showReportUser(qint64, const QString&, Origin::Engine::IIGOCommandController::CallOrigin)),
        this, SLOT(onShowReportUser(qint64, const QString&, Origin::Engine::IIGOCommandController::CallOrigin)));

    if (Engine::Social::SocialController* socialController = user->socialControllerInstance())
    {
#if 0 // removing chatrooms for origin x
        ORIGIN_VERIFY_CONNECT(
            socialController, SIGNAL(groupDestroyed(QString, qint64, QString)),
            this, SLOT(onGroupDestroyed(QString, qint64)));

        ORIGIN_VERIFY_CONNECT(
            socialController, SIGNAL(inviteToChatGroup(const QString&, const QString&)),
            this, SLOT(showInviteNotification(const QString&, const QString&)));

        ORIGIN_VERIFY_CONNECT(
            socialController, SIGNAL(kickedFromGroup(QString, QString, qint64)),
            this, SLOT(onKickedFromGroup(QString, QString, qint64)));

         ORIGIN_VERIFY_CONNECT(
            socialController, SIGNAL(failedToEnterRoom()),
            this, SLOT(onFailedToEnterRoom()));

        ORIGIN_VERIFY_CONNECT( 
            socialController->conversations(), SIGNAL(groupChatMessageReceived(QSharedPointer<Origin::Engine::Social::Conversation>, const Origin::Chat::Message &)), 
            this, SLOT(onGroupChatMessageRecieved(QSharedPointer<Origin::Engine::Social::Conversation>, const Origin::Chat::Message &)));

        ORIGIN_VERIFY_CONNECT( 
            socialController->conversations(), SIGNAL(groupRoomDestroyed(QSharedPointer<Origin::Engine::Social::Conversation>, const QString&)), 
            this, SLOT(onGroupRoomDestroyed(QSharedPointer<Origin::Engine::Social::Conversation>, const QString&)));

        ORIGIN_VERIFY_CONNECT( 
            socialController->conversations(), SIGNAL(kickedFromRoom(QSharedPointer<Origin::Engine::Social::Conversation>, const QString&)), 
            this, SLOT(onKickedFromRoom(QSharedPointer<Origin::Engine::Social::Conversation>, const QString&)));
#endif
    }

#if ENABLE_VOICE_CHAT
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( isVoiceEnabled )
    {
        // Set up VoiceController
        mVoiceClient = QPointer<Origin::Engine::Voice::VoiceClient>(user->voiceControllerInstance()->getVoiceClient());
        ORIGIN_VERIFY_CONNECT(mVoiceClient.data(), SIGNAL(voiceUserTalking(const QString&)), this, SLOT(onVoiceUserTalking(const QString&)));
        ORIGIN_VERIFY_CONNECT(mVoiceClient.data(), SIGNAL(voiceUserStoppedTalking(const QString&)), this, SLOT(onVoiceUserStoppedTalking(const QString&)));
        ORIGIN_VERIFY_CONNECT(mVoiceClient.data(), SIGNAL(voiceLocalUserMuted()), this, SLOT(onVoiceLocalUserMuted()));
        ORIGIN_VERIFY_CONNECT(mVoiceClient.data(), SIGNAL(voiceLocalUserUnMuted()), this, SLOT(onVoiceLocalUserUnMuted()));
        ORIGIN_VERIFY_CONNECT(mVoiceClient.data(), SIGNAL(voiceClientDisconnectCompleted(bool)), this, SLOT(onVoiceClientDisconnectCompleted(bool)));

        // List for IGO gameAdded so that we can show the user muted...
        ORIGIN_VERIFY_CONNECT(Origin::Engine::IGOController::instance()->gameTracker(), SIGNAL(gameAdded(uint32_t)), this, SLOT(onGameAdded(uint32_t)));
        ORIGIN_VERIFY_CONNECT(conversationManager, SIGNAL(mutedByAdmin(QSharedPointer<Origin::Engine::Social::Conversation>, const Origin::Chat::JabberID&)),
            this, SLOT(onMutedByAdmin(QSharedPointer<Origin::Engine::Social::Conversation>, const Origin::Chat::JabberID&)));

        ORIGIN_VERIFY_CONNECT(user->voiceControllerInstance(), SIGNAL(showToast_IncomingVoiceCall(const QString&, const QString&)),
            this, SLOT(showToast_IncomingVoiceCall(const QString&, const QString&)));

        ORIGIN_VERIFY_CONNECT(user->voiceControllerInstance(), SIGNAL(showSurvey_Voice(const QString&)),
            this, SLOT(sendSurveyPromptForConversation(const QString&)));
    }
#endif
    ORIGIN_VERIFY_CONNECT(conversationManager, SIGNAL(enterMucRoomFailed(Origin::Chat::ChatroomEnteredStatus)),
        this, SLOT(onEnterMucRoomFailed(Origin::Chat::ChatroomEnteredStatus)));
}

SocialViewController::~SocialViewController()
{
#if ENABLE_VOICE_CHAT
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( isVoiceEnabled )
    {
        if (mVoiceClient)
        {
            mVoiceClient->stopVoiceConnection("SocialViewController", "destructing");
        }
    }
#endif

    if(mVoiceConflictWindow)
    {
        delete mVoiceConflictWindow;
        mVoiceConflictWindow = NULL;
    }

    deleteSurveyWindow();
}

void SocialViewController::deleteSurveyWindow()
{
    if (mSurveyPromptWindow)
    {
        mSurveyPromptWindow->deleteLater();
        mSurveyPromptWindow = NULL;
    }
}

void SocialViewController::onInvitedToRemoteSession(const Origin::Engine::MultiplayerInvite::Invite &invite)
{
    using namespace Origin::Engine::Content;
    ProductInfo* info = ProductInfo::byProductId(invite.sessionInfo().productId());

    mInvite = invite;
    ORIGIN_VERIFY_CONNECT(info, SIGNAL(succeeded()), this, SLOT(onContentLookupSucceeded()));
    ORIGIN_VERIFY_CONNECT(info, SIGNAL(failed()), this, SLOT(onContentLookupFailed()));
}

void SocialViewController::onContentLookupSucceeded()
{
    if (!mInvite.isValid())
        return;

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    using namespace Origin::Engine::Content;
    ProductInfo* info = static_cast<ProductInfo*>(sender());

    QString originId = "";
    Origin::Engine::Social::SocialController *socialController = Origin::Engine::LoginController::currentUser()->socialControllerInstance();
    if (socialController)
    {
        Origin::Chat::JabberID inviterJid = mInvite.from();
        Origin::Chat::NucleusID nucleusId = socialController->chatConnection()->jabberIdToNucleusId(inviterJid);
        originId = socialController->userNames()->namesForNucleusId(nucleusId).originId();
    }
    QString boldUser = QString("<b>%0</b>").arg(originId);
    QString bodyStr = QString(tr("ebisu_client_non_friend_invite_body1").arg(boldUser).arg(info->data(mInvite.sessionInfo().productId())->displayName()));
    UIToolkit::OriginNotificationDialog* toast = otm->showToast(Services::SETTING_NOTIFY_GAMEINVITE.name(), bodyStr);
    if(toast != NULL)
    {
        // Need to find RemoteUser to open the chat window with a specific user
        Origin::Engine::UserRef currentUser = Origin::Engine::LoginController::currentUser();
        Chat::OriginConnection *conn = currentUser->socialControllerInstance()->chatConnection();
        Chat::RemoteUser *contact = conn->remoteUserForJabberId(mInvite.from());

        QSignalMapper* signalMapper =  new QSignalMapper(toast->content());
        signalMapper->setMapping(toast, contact);
        ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), signalMapper, SLOT(map()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(QObject*)), this, SLOT(showChatWindowForFriend(QObject*)));
    }

    mInvite = Origin::Engine::MultiplayerInvite::Invite();
}

void SocialViewController::onContentLookupFailed()
{
    if (!mInvite.isValid())
        return;

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    using namespace Origin::Engine::Content;
    ProductInfo* info = static_cast<ProductInfo*>(sender());

    QString originId = "";
    Origin::Engine::Social::SocialController *socialController = Origin::Engine::LoginController::currentUser()->socialControllerInstance();
    if (socialController)
    {
        Origin::Chat::JabberID inviterJid = mInvite.from();
        Origin::Chat::NucleusID nucleusId = socialController->chatConnection()->jabberIdToNucleusId(inviterJid);
        originId = socialController->userNames()->namesForNucleusId(nucleusId).originId();
    }
    QString boldUser = QString("<b>%0</b>").arg(originId);
    UIToolkit::OriginNotificationDialog* toast = otm->showToast(Services::SETTING_NOTIFY_GAMEINVITE.name(),
        QString(tr("ebisu_client_non_friend_invite_body1_no_game").arg(boldUser).arg(info->data(mInvite.sessionInfo().productId())->displayName())));
    if(toast != NULL)
    {
        // Need to find RemoteUser to open the chat window with a specific user
        Origin::Engine::UserRef currentUser = Origin::Engine::LoginController::currentUser();
        Chat::OriginConnection *conn = currentUser->socialControllerInstance()->chatConnection();
        Chat::RemoteUser *contact = conn->remoteUserForJabberId(mInvite.from());

        QSignalMapper* signalMapper =  new QSignalMapper(toast->content());
        signalMapper->setMapping(toast, contact);
        ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), signalMapper, SLOT(map()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(QObject*)), this, SLOT(showChatWindowForFriend(QObject*)));
    }

    mInvite = Origin::Engine::MultiplayerInvite::Invite();
}

void SocialViewController::showChatWindowForFriend(QObject* contact)
{
    Chat::RemoteUser* remoteUser = dynamic_cast<Chat::RemoteUser*>(contact);
    if(remoteUser)
    {
        chatWindowManager()->startConversation(remoteUser, Origin::Engine::Social::Conversation::InternalRequest, Engine::IIGOCommandController::CallOrigin_CLIENT);
    }
    else
    {
        // Can't find the user - just bring the chat window to the foreground
        chatWindowManager()->raiseMainChatWindow();
    }
}

void SocialViewController::onStartConversation(QSharedPointer<Origin::Engine::Social::Conversation> conversation)
{
#if ENABLE_VOICE_CHAT
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    ORIGIN_VERIFY_CONNECT_EX(conversation.data(), SIGNAL(voiceChatConflict(const QString&)), this, SLOT(onVoiceChatConflict(const QString&)), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT(conversation.data(), SIGNAL(displaySurvey()), this, SLOT(onDisplaySurvey()));

    ORIGIN_VERIFY_CONNECT_EX(
        conversation.data(), &Origin::Engine::Social::Conversation::voiceChatConnected,
        this, &SocialViewController::onVoiceChatConnected,
        Qt::QueuedConnection);

    ORIGIN_VERIFY_CONNECT_EX(
        mUser->voiceControllerInstance(), SIGNAL(voiceChatDisconnected()),
        this, SLOT(onVoiceChatDisconnected()),
        Qt::QueuedConnection);
#endif
}

void SocialViewController::onEnterMucRoomFailed(Origin::Chat::ChatroomEnteredStatus status)
{
    if (status==Origin::Chat::CHATROOM_ENTERED_FAILURE_MAX_USERS)
    {
        ClientFlow::instance()->view()->showRoomIsFullDialog(Engine::IGOController::instance() && Engine::IGOController::instance()->isActive() ? UIScope::IGOScope : UIScope::ClientScope);
    }
    else
    {
        onFailedToEnterRoom();
    }
}

#if 1 // Origin X
void SocialViewController::showToast_IncomingVoiceCall(const QString& originId, const QString& conversationId)
{
#if ENABLE_VOICE_CHAT
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if (!isVoiceEnabled)
        return;

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR << "ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR << "OriginToastManager not available";
        return;
    }

    ORIGIN_VERIFY_CONNECT_EX(
        mUser->voiceControllerInstance(), SIGNAL(voiceCallStateChanged(const QString &, int)),
        this, SLOT(onVoiceCallStateChanged(const QString &, int)),
        Qt::QueuedConnection);

    UIToolkit::OriginNotificationDialog* toast = otm->showToast(
        Services::SETTING_NOTIFY_INCOMINGVOIPCALL.name(),
        QString(tr("ebisu_client_is_calling")).arg(originId));

    if (toast)
    {
        //toast->setProperty("channel", channel);
        mUser->voiceControllerInstance()->setProperty("toast", QVariant::fromValue(toast));
        toast->setProperty("conversationId", QVariant::fromValue(conversationId));

        // Desktop
        ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(actionAccepted()), this, SLOT(acceptIncomingVoiceCall()), Qt::QueuedConnection);
        ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(actionRejected()), this, SLOT(rejectIncomingVoiceCall()), Qt::QueuedConnection);
        // the clicked signal is emitted when the hotkey is pressed this should open/focus the conversation 
        ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), this, SLOT(showChatWindowForConversation()), Qt::QueuedConnection);
        // Need to dismiss the notification if the conversation timer is finished.  This type will not dismiss itself.
        ORIGIN_VERIFY_CONNECT(mUser->voiceControllerInstance(), SIGNAL(callTimeout()), toast, SLOT(beginFadeOut()));
    }
#endif
}
#endif

void SocialViewController::acceptIncomingVoiceCall()
{
    QObject* dialog = sender();
    if (!dialog)
        return;

#if ENABLE_VOICE_CHAT
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( isVoiceEnabled )
    {
        QString conversationId = dialog->property("conversationId").value<QString>();
        mUser->voiceControllerInstance()->joinVoice(conversationId);
    }
#endif
}

void SocialViewController::rejectIncomingVoiceCall()
{
    QObject* dialog = sender();
    if (!dialog)
        return;

#if ENABLE_VOICE_CHAT
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( isVoiceEnabled )
    {
        QString conversationId = dialog->property("conversationId").value<QString>();
        mUser->voiceControllerInstance()->leaveVoice(conversationId);
    }
#endif
}

void SocialViewController::onVoiceCallStateChanged(const QString& id, int newVoiceCallState)
{
    Engine::Voice::VoiceController* voiceController = dynamic_cast<Engine::Voice::VoiceController*>(sender());

    if ((newVoiceCallState & Origin::Engine::Social::Conversation::VOICECALLSTATE_INCOMINGCALL) == 0)
    {
        UIToolkit::OriginNotificationDialog* toast = voiceController->property("toast").value<UIToolkit::OriginNotificationDialog*>();
        if (!toast)
            return;

        // TBD: how do we know whether the toast is still open/valid???
        ORIGIN_VERIFY_DISCONNECT(toast, SIGNAL(accepted()), this, SLOT(acceptIncomingVoiceCall()));
        ORIGIN_VERIFY_DISCONNECT(toast, SIGNAL(rejected()), this, SLOT(rejectIncomingVoiceCall()));
        // Need to call reject() or accept() on notifications not close() do to IGO change
        toast->reject();
    }
}

void SocialViewController::raiseMainChatWindow()
{
    ClientFlow::instance()->socialViewController()->chatWindowManager()->raiseMainChatWindow();
}

void SocialViewController::blockContactWithConfirmation(const Chat::RemoteUser *remoteUser)
{
    blockUserWithConfirmation(mUser->socialControllerInstance()->chatConnection()->nucleusIdForUser(remoteUser));
}

void SocialViewController::blockUserWithConfirmation(quint64 nucleusId)
{
    using namespace Origin::UIToolkit;

    // Do we already have a dialog open?
    for(auto it = mBlockWindowToUserId.constBegin();
        it != mBlockWindowToUserId.constEnd();
        it++)
    {
        if (nucleusId == it.value())
        {
            return;
        }
    }

    // Look up the user's Origin ID synchronously
    // This should already be cached
    Engine::Social::UserNamesCache *userNames = mUser->socialControllerInstance()->userNames();
    const QString originId = userNames->namesForNucleusId(nucleusId).originId();

    QString messageTitleString;
    QString messageTitlebarString;
    QString messageBodyString;
    QString actionButtonString;

    messageTitleString = tr("ebisu_client_unfriend_and_block_friend_header");
    messageTitlebarString = tr("ebisu_client_unfriend_and_block_friend_title");
    messageBodyString = tr("ebisu_client_unfriend_and_block_friend_body");
    actionButtonString = tr("ebisu_client_unfriend_and_block");

    OriginMsgBox* blockMsg = new OriginMsgBox(OriginMsgBox::NoIcon, messageTitleString.arg(originId), 
        messageBodyString.arg(originId));
    
    auto blockReceiver = new BlockReceiver(this, mUser, nucleusId);

    OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close | OriginWindow::Minimize;
    OriginWindow::WindowContext windowContext = OriginWindow::Normal;
    
    OriginWindow* blockWindow = new OriginWindow(titlebarItems,
        blockMsg, OriginWindow::Scrollable, QDialogButtonBox::Ok |QDialogButtonBox::Cancel,
        OriginWindow::ChromeStyle_Dialog, windowContext);

    if (blockWindow->manager())
    {
        blockWindow->manager()->setupButtonFocus();
    }

    blockWindow->setButtonText(QDialogButtonBox::Ok, actionButtonString);
    blockWindow->setTitleBarText(messageTitlebarString);

    ORIGIN_VERIFY_CONNECT(blockWindow, SIGNAL(rejected()), blockWindow, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(blockWindow, SIGNAL(finished(int)), blockWindow, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(blockWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), blockReceiver, SLOT(checkConnection()));
    ORIGIN_VERIFY_CONNECT(blockWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), blockWindow, SLOT(accept()));
    ORIGIN_VERIFY_CONNECT(blockWindow->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), blockWindow, SLOT(reject()));
    ORIGIN_VERIFY_CONNECT(Origin::Client::MainFlow::instance(), SIGNAL(logoutConcluded()), blockWindow, SLOT(close()));

    ORIGIN_VERIFY_CONNECT(blockWindow, SIGNAL(closeEventTriggered()), this, SLOT(onBlockDialogClosed()));

    // Track we have this open so we don't open duplicate windows
    mBlockWindowToUserId.insert(blockWindow, nucleusId);

    if (Engine::IGOController::instance()->isActive())
    {
        Engine::IGOController::instance()->igowm()->addPopupWindow(blockWindow, Engine::IIGOWindowManager::WindowProperties());
    }
    else
    {
        blockWindow->showWindow();
    }
}

void SocialViewController::blockUser(quint64 nucleusId)
{
    // This function assumes the user is connected and online
    auto blockReceiver = new BlockReceiver(NULL, mUser, nucleusId);
    blockReceiver->performBlock();
    blockReceiver->deleteLater();
}

void SocialViewController::onBlockDialogClosed()
{
    mBlockWindowToUserId.remove(sender());
}

QString SocialViewController::runningGameMasterTitle()
{
    // Check the running games, and return the masterTitleId of the running game.
    Origin::Engine::Content::ContentController *controller = Origin::Engine::Content::ContentController::currentUserContentController();

    if(controller)
    {
        foreach(Origin::Engine::Content::EntitlementRef entitlement, controller->entitlements())
        {
            if(entitlement->localContent()->playing())
            {
                return entitlement->contentConfiguration()->masterTitle();
            }
        }
    }
    return QString();
}

void SocialViewController::onShowReportUser(qint64 target, const QString& masterTitle, Engine::IIGOCommandController::CallOrigin callOrigin)
{
    reportTosViolation(target, masterTitle, callOrigin);
}

void SocialViewController::reportTosViolation(const Chat::RemoteUser *contact)
{
    reportTosViolation(contact->nucleusId());
}

void SocialViewController::reportTosViolation(qint64 nucleusId, const QString& masterTitle/*=""*/, Engine::IIGOCommandController::CallOrigin callOrigin)
{
    // Do we already have a dialog open?
    for(auto it = mReportWindowToUserId.constBegin();
        it != mReportWindowToUserId.constEnd();
        it++)
    {
        if (nucleusId == it.value())
        {
            Origin::UIToolkit::OriginWindow* reportUserWindow = dynamic_cast<Origin::UIToolkit::OriginWindow *>(it.key());
            if (reportUserWindow)
                reportUserWindow->raise();
            return;
        }
    }
    Engine::Social::UserNamesCache *userNames = mUser->socialControllerInstance()->userNames();
    const QString originId = userNames->namesForNucleusId(nucleusId).originId();
    ReportUserDialog* reportUserDialog = new ReportUserDialog(originId, nucleusId, masterTitle.isEmpty() ? runningGameMasterTitle() : masterTitle);
    Origin::UIToolkit::OriginWindow* reportUserWindow = reportUserDialog->getWindow();
    ORIGIN_VERIFY_CONNECT(Origin::Client::MainFlow::instance(), SIGNAL(logoutConcluded()), reportUserWindow, SLOT(close()));    
    ORIGIN_VERIFY_CONNECT(reportUserWindow, SIGNAL(closeEventTriggered()), this, SLOT(onReportUserClosed()));
    ORIGIN_VERIFY_CONNECT(reportUserDialog, SIGNAL(submitReportUser(quint64, QString, QString, QString, QString)), this, SLOT(onReportUser(quint64, QString, QString, QString, QString)));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(reportSubmitted()), reportUserDialog, SLOT(onReportSubmitted()));
    ORIGIN_VERIFY_CONNECT(reportUserDialog, SIGNAL(reportUserDialogDelete()), this, SLOT(doReportUserDialogDelete()));
    // Track we have this open so we don't open duplicate windows
    mReportWindowToUserId.insert(reportUserWindow, nucleusId);

    if (Engine::IGOController::instance()->isActive())
    {
        Engine::IIGOWindowManager::WindowProperties properties;
        properties.setCallOrigin(callOrigin);
        Engine::IIGOWindowManager::WindowBehavior behavior;
        Engine::IGOController::instance()->igowm()->addPopupWindow(reportUserWindow, properties, behavior);
    }
    else
    {
        reportUserWindow->showWindow();
    }
}

void SocialViewController::onReportUser(quint64 id, QString reasonStr, QString whereStr, QString masterTitle, QString comments)
{

    using namespace Services;

    AtomServiceClient::ReportContentType contentType;
    AtomServiceClient::ReportReason reason;

    Engine::UserRef user = Engine::LoginController::instance()->currentUser();

    if (!user)
    {
        ORIGIN_ASSERT(0);
        return;
    }

    if (whereStr == REPORT_NAME_STR)
    {
        contentType = AtomServiceClient::NameContent;
    }
    else if (whereStr == REPORT_AVATAR_STR)
    {
        contentType = AtomServiceClient::AvatarContent;
    }
    else if (whereStr == REPORT_CUSTOM_GAME_NAME_STR)
    {
        contentType = AtomServiceClient::CustomGameNameContent;
    }
    else if (whereStr == REPORT_CHAT_ROOM_NAME_STR)
    {
        contentType = AtomServiceClient::RoomNameContent;
    }
    else if (whereStr == REPORT_IN_GAME_STR)
    {
        contentType = AtomServiceClient::InGame;
    }
    else
    {
        ORIGIN_ASSERT(0);
        return;
    }

    if (reasonStr == REPORT_CHILD_SOLICITATION_STR)
    {
        reason = AtomServiceClient::ChildSolicitationReason; 
    }
    else if (reasonStr == REPORT_TERRORIST_THREAT_STR)
    {
        reason = AtomServiceClient::TerroristThreatReason;
    }
    else if (reasonStr == REPORT_HARASSMENT_STR)
    {
        reason = AtomServiceClient::HarassmentReason;
    }
    else if (reasonStr == REPORT_SPAM_STR)
    {
        reason = AtomServiceClient::SpamReason;
    }
    else if (reasonStr == REPORT_CHEATING_STR)
    {
        reason = AtomServiceClient::CheatingReason;
    }
    else if (reasonStr == REPORT_OFFENSIVE_CONTENT_STR)
    {
        reason = AtomServiceClient::OffensiveContentReason;
    }
    else if (reasonStr == REPORT_SUICIDE_THREAT_STR)
    {
        reason = AtomServiceClient::SuicideThreatReason;
    }
    else if (reasonStr == REPORT_CLIENT_HACK_STR)
    {
        reason = AtomServiceClient::ClientHackReason;
    }
    else
    {
        ORIGIN_ASSERT(0);
        return;
    }

    AtomServiceClient::reportUser(user->getSession(), id, contentType, reason, masterTitle, comments);
    GetTelemetryInterface()->Metric_FRIEND_REPORT_SENT();
  
    emit reportSubmitted();
}

void SocialViewController::onReportUserClosed()
{
    mReportWindowToUserId.remove(sender());
}

void SocialViewController::doReportUserDialogDelete()
{
    sender()->deleteLater();
}

void SocialViewController::removeContactWithConfirmation(const Chat::RemoteUser *contact)
{
    using namespace Origin::UIToolkit;

    OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close | OriginWindow::Minimize;
    OriginWindow::WindowContext windowContext = OriginWindow::Normal;

    Chat::Roster *roster = mUser->socialControllerInstance()->chatConnection()->currentUser()->roster();

    RemoveContactDialog* content = new RemoveContactDialog(roster, contact);
    content->setMaximumHeight(0); // The content in this case has no size

    OriginWindow* unsubscribeWindow = new OriginWindow(titlebarItems,
        content, OriginWindow::MsgBox, QDialogButtonBox::Ok |QDialogButtonBox::Cancel, 
        OriginWindow::ChromeStyle_Dialog, windowContext);

    unsubscribeWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_delete_friend_title"), 
        tr("ebisu_client_delete_friend_body").arg(contact->originId()));

    if(unsubscribeWindow->manager())
    {
        unsubscribeWindow->manager()->setupButtonFocus();
    }

    unsubscribeWindow->setButtonText(QDialogButtonBox::Ok, tr("ebisu_client_action_delete_friend").arg(contact->originId()));

    ORIGIN_VERIFY_CONNECT(unsubscribeWindow, SIGNAL(rejected()), unsubscribeWindow, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(unsubscribeWindow, SIGNAL(finished(int)), unsubscribeWindow, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(unsubscribeWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), (RemoveContactDialog*)unsubscribeWindow->content(), SLOT(onDeleteRequested()));
    ORIGIN_VERIFY_CONNECT(unsubscribeWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), unsubscribeWindow, SLOT(accept()));
    ORIGIN_VERIFY_CONNECT(unsubscribeWindow->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), unsubscribeWindow, SLOT(reject()));
    ORIGIN_VERIFY_CONNECT(Origin::Client::MainFlow::instance(), SIGNAL(logoutConcluded()), unsubscribeWindow, SLOT(close()));

    if (Engine::IGOController::instance()->isActive())
    {
        Engine::IGOController::instance()->igowm()->addPopupWindow(unsubscribeWindow, Engine::IIGOWindowManager::WindowProperties());
    }
    else
    {
        unsubscribeWindow->showWindow();
    }
}

void SocialViewController::showEnteredEmptyMucRoomDialog()
{
    EnteredEmptyMucRoomDialog *dialog = new EnteredEmptyMucRoomDialog();

    ORIGIN_VERIFY_CONNECT(dialog, SIGNAL(closeEventTriggered()), dialog, SLOT(deleteLater()));

    if (Engine::IGOController::instance()->isActive())
    {
        Engine::IGOController::instance()->igowm()->addPopupWindow(dialog, Engine::IIGOWindowManager::WindowProperties());
    }
    else
    {
        dialog->showWindow();
    }
}
    
    
void SocialViewController::setLastNotifiedConversationId(QString id)
{
        mLastNotifiedConversationId = id;
}

QSharedPointer<Engine::Social::Conversation> SocialViewController::findLastNotifiedConversation()
{
    return Origin::Client::JsInterface::OriginChatProxy::instance()->conversationForId(mLastNotifiedConversationId);    
}

void SocialViewController::onGroupRoomInviteReceived(const QString& from, const QString& groupId, const QString& channelId)
{
#if 0 // Remove chatrooms for Origin X
    // If the current user is already in the room, ignore the summons
    Chat::Connection* connection = mUser->socialControllerInstance()->chatConnection();
    Chat::ConnectedUser* currentUser = connection->currentUser();
    Chat::ChatGroups* groups = currentUser->chatGroups();
    Chat::ChatChannel* channel = NULL;
    Chat::ChatGroup* group = NULL;
    if (groups)
    {
        group = groups->chatGroupForGroupGuid(groupId);
        if (group)
        {
            channel = group->channelForChannelId(channelId); 
            if (channel && channel->isUserInChannel()) return;
        }
    }

    QMap<QString, QObject*>::iterator iter = mSummonedToRoomFlowList.find(groupId + channelId);
    if (iter == mSummonedToRoomFlowList.end())
    {
        SummonedToGroupChannelFlow *flow = new SummonedToGroupChannelFlow(mUser, from, groupId, channelId);
        ORIGIN_VERIFY_CONNECT(flow, SIGNAL(handled()), this, SLOT(onGroupRoomInviteHandled()));
        ORIGIN_VERIFY_CONNECT(flow, SIGNAL(succeeded()), this, SLOT(onGroupRoomInviteFinished()));
        ORIGIN_VERIFY_CONNECT(flow, SIGNAL(failed()), this, SLOT(onGroupRoomInviteFailed()));
        ORIGIN_VERIFY_CONNECT(this, SIGNAL(showInviteAlertDialog(const QString&)), flow, SLOT(onShowInviteAlertDialog(const QString&)));

        mSummonedToRoomFlowList.insert(groupId + channelId, flow);
        flow->start();
    } else {
        SummonedToGroupChannelFlow *flow =  dynamic_cast<SummonedToGroupChannelFlow*>(*iter);
        flow->updateFrom(from);
    }
#endif
}

void SocialViewController::showInviteNotification(const QString& from, const QString& groupId)
{
    // Don't show notification if the client has focus, or if OIG is active
    if(QApplication::focusWidget() || (Origin::Engine::IGOController::instance()->igowm()->visible()))
        return;

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    QString groupName;
    Chat::Connection* connection = mUser->socialControllerInstance()->chatConnection();
    Chat::ConnectedUser* currentUser = connection->currentUser();
    Chat::ChatGroups* groups = currentUser->chatGroups();
    Chat::ChatGroup* group = NULL;
    if (groups)
    {
        group = groups->chatGroupForGroupGuid(groupId);
        if (group)
        {
            groupName = group->getName();

            QString title = QString("<b>%1</b>").arg(tr("ebisu_client_groip_group_invite_title").arg( QString("<i>%1</i>").arg(groupName) ));

            QString fromUsername;
            const Chat::JabberID currentUserJid = currentUser->jabberId().asBare();
            const Chat::JabberID fromUserJid(from, currentUserJid.domain());
            Chat::RemoteUser* remoteUser = connection->remoteUserForJabberId(fromUserJid);
            if (remoteUser)
            {
                fromUsername = remoteUser->originId();

                QString message = tr("ebisu_client_groip_group_invite").arg(fromUsername);

                Origin::UIToolkit::OriginNotificationDialog* toast = otm->showToast(Services::SETTING_NOTIFY_INVITETOCHATROOM.name(),title, message);
                if(toast != NULL)
                {
                    QSignalMapper* signalMapper =  new QSignalMapper(toast->content());
                    signalMapper->setMapping(toast, group);
                    ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), signalMapper, SLOT(map()), Qt::QueuedConnection);
                    ORIGIN_VERIFY_CONNECT(signalMapper, SIGNAL(mapped(QObject*)), this, SLOT(onInviteAlertClicked(QObject*)));
                }

            }

        }
    }
}

void SocialViewController::onInviteAlertClicked(QObject* groupObject)
{
    Chat::ChatGroup* group = dynamic_cast<Chat::ChatGroup*>(groupObject);
    if(group)
    {
        emit showInviteAlertDialog(group->getGroupGuid());
    }
}

void SocialViewController::onGroupRoomInviteHandled()
{
    SummonedToGroupChannelFlow* flow = dynamic_cast<SummonedToGroupChannelFlow*>(sender());
    mSummonedToRoomFlowList.remove(flow->getGroupId() + flow->getChannelId());
}

void SocialViewController::onGroupRoomInviteFinished()
{
    SummonedToGroupChannelFlow* flow = dynamic_cast<SummonedToGroupChannelFlow*>(sender());
    mSummonedToRoomFlowList.remove(flow->getGroupId() + flow->getChannelId());
    flow->deleteLater();
}

void SocialViewController::onGroupRoomInviteFailed()
{
    SummonedToGroupChannelFlow* flow = dynamic_cast<SummonedToGroupChannelFlow*>(sender());
    mSummonedToRoomFlowList.remove(flow->getGroupId() + flow->getChannelId());
    flow->deleteLater();
    onMessageGroupRoomInviteFailed();
}

void SocialViewController::onMessageGroupRoomInviteFailed()
{
    using namespace Origin::UIToolkit;

    if (!mGroupRoomInviteFailedAlert)
    {
        mGroupRoomInviteFailedAlert = OriginWindow::alertNonModal(OriginMsgBox::Error, 
            tr("ebisu_client_unable_to_join_chat_room_title"),
            tr("ebisu_oops_your_friend_tried_to_invite_you_to_a_chat_room"), tr("ebisu_client_close"));
        ORIGIN_VERIFY_CONNECT(mGroupRoomInviteFailedAlert, SIGNAL(finished(int)), this, SLOT(onGroupRoomInviteFailedAlertClosed()));

        if (Engine::IGOController::instance()->isActive())
        {
            mGroupRoomInviteFailedAlert->configForOIG(true);
            Engine::IGOController::instance()->igowm()->addPopupWindow(mGroupRoomInviteFailedAlert, Engine::IIGOWindowManager::WindowProperties());
        }
        else
        {
            mGroupRoomInviteFailedAlert->showWindow();
        }
    }
}

void SocialViewController::onGroupRoomInviteFailedAlertClosed()
{
    if (mGroupRoomInviteFailedAlert) 
    {
        ORIGIN_VERIFY_DISCONNECT(mGroupRoomInviteFailedAlert, NULL, this, NULL);
        mGroupRoomInviteFailedAlert->deleteLater();
        mGroupRoomInviteFailedAlert = NULL;
    }
}

void SocialViewController::showChatWindowForConversation()
{
    QObject* toast = sender();
    if(!toast)
        return;

    QSharedPointer<Origin::Engine::Social::Conversation> conversation =  toast->property("conversation").value< QSharedPointer<Origin::Engine::Social::Conversation> >();
    if (!conversation)
        return;
    conversation->setCreationReason(Origin::Engine::Social::Conversation::InternalRequest);
    mChatWindowManager->startConversation(conversation, Engine::IIGOCommandController::CallOrigin_CLIENT);
}

void SocialViewController::onDisplaySurvey()
{
    QString channel;
    
#if ENABLE_VOICE_CHAT
    Origin::Engine::Social::Conversation* conversation = dynamic_cast<Origin::Engine::Social::Conversation*>(sender());
    if (conversation)
        channel = conversation->voiceChannel();
#endif

    //
    // 9.5-SPECIFIC GROIP SURVEY PROMPT
    //
    if (mSurveyPromptWindow)
    {
        mSurveyPromptWindow->raise();
        mSurveyPromptWindow->activateWindow();
        mSurveyPromptWindow->setProperty(PROP_CHANNEL_ID, channel);
    }
    else
    {
        mSurveyPromptWindow = UIToolkit::OriginWindow::promptNonModal(UIToolkit::OriginMsgBox::NoIcon, 
            tr("ebisu_client_let_us_know_title"),
            tr("ebisu_client_please_take_a_minute_to_help_us_text"),
            tr("ebisu_client_take_survey"), tr("ebisu_client_close"), QDialogButtonBox::Yes);
        mSurveyPromptWindow->setTitleBarText(tr("ebisu_client_chat_survey"));
        mSurveyPromptWindow->setAttribute(Qt::WA_DeleteOnClose, false);
        mSurveyPromptWindow->setProperty(PROP_CHANNEL_ID, channel);
        ORIGIN_VERIFY_CONNECT(mSurveyPromptWindow, SIGNAL(finished(int)), this, SLOT(onSurveyPromptClosed(const int&)));
        mSurveyPromptWindow->showWindow();
    }
}

void SocialViewController::onSurveyPromptClosed(const int& result)
{
    if(result == QDialog::Accepted)
    {
        QString voiceChatChannelId;
        if (sender()) {
            voiceChatChannelId = sender()->property(PROP_CHANNEL_ID).toString();
        }

        sendSurveyPromptForConversation(voiceChatChannelId);
    }

    deleteSurveyWindow();
}

void SocialViewController::sendSurveyPromptForConversation(QString const& voiceChatChannelId)
{
    const QString locale = Services::readSetting(Services::SETTING_LOCALE);
    QUrlQuery query;
    query.addQueryItem("l", locale);
    query.addQueryItem("x", GetTelemetryInterface()->GetMacHash().c_str());
    query.addQueryItem("c", voiceChatChannelId);
    query.addQueryItem("t", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    const QString SURVEY_URL("https://motd.dm.origin.com/service.sumo");

    QUrl url(SURVEY_URL);
    url.setQuery(query);

    // If OIG is visible open the website within OIG
    if (Engine::IGOController::instance()->isVisible())
        emit Engine::IGOController::instance()->showBrowser(url.toString(), false);
    else
        Origin::Services::PlatformService::asyncOpenUrl(url);
}

void SocialViewController::onInviteToRoom(const QObjectList& users, const QString& groupGuid, const QString& channelId)
{
    // We need a reference to the channel object
    Chat::ChatGroups* groups = mUser->socialControllerInstance()->chatConnection()->currentUser()->chatGroups();
    Chat::ChatChannel* channel = NULL;
    if (groups)
    {
        Chat::ChatGroup* group = groups->chatGroupForGroupGuid(groupGuid);
        if (group)
        {
            channel = group->channelForId(channelId);
        }
    }

    // Iterate users and summon to the channel
    if (channel)
    {
        for(QObjectList::const_iterator it = users.begin(); it != users.end(); it++) 
        {                
            JsInterface::RemoteUserProxy* pr = dynamic_cast<JsInterface::RemoteUserProxy*>(*it);
            if (pr) 
            {
                const Chat::JabberID toJid = pr->proxied()->jabberId();

                SummonToGroupChannelFlow *flow = new SummonToGroupChannelFlow(channel, mUser, toJid, groupGuid, channelId);
                ORIGIN_VERIFY_CONNECT(flow, SIGNAL(finished()), this, SLOT(onInviteUserToRoomFinished()));
                flow->start();
            }
        }

        // Message the user that the invitation(s) have been sent
        showGroupInviteSentDialog(users);
    }
}

void SocialViewController::onInviteUserToRoomFinished()
{
    // Do we care if it succeeded?
    sender()->deleteLater();
}

void SocialViewController::onFailedToEnterRoom()
{
    // Bring up generic error dialog
    UIToolkit::OriginMsgBox::IconType icon = UIToolkit::OriginMsgBox::Error;

    UIToolkit::OriginWindow* window = 
        UIToolkit::OriginWindow::alertNonModal(icon, tr("ebisu_client_unable_enter_room_title"), tr("ebisu_client_error_try_again_later"), tr("ebisu_client_close"));

    window->setTitleBarText(tr("ebisu_client_enter_room"));

    if (Engine::IGOController::instance()->isActive())
    {
        window->configForOIG(true);
        Engine::IGOController::instance()->igowm()->addPopupWindow(window, Engine::IIGOWindowManager::WindowProperties());
    }
    else
    {
        window->showWindow();
    }
}

void SocialViewController::onGroupChatMessageRecieved(QSharedPointer<Origin::Engine::Social::Conversation> conversation, const Origin::Chat::Message& message)
{
    QString from = mUser->socialControllerInstance()->chatConnection()->remoteUserForJabberId(message.from())->originId();
    if (from.isEmpty()) // if this is empty the current user sent this message.
        return;

    // Don't send notification if the Desktop Chat window has focus
    if (mChatWindowManager->isChatWindowActive())
        return;

    // Don't send notification if the OIG Chat window has focus
    if (Origin::Engine::IGOController::instance()->igowm()->visible() && mChatWindowManager->isIGOChatWindowActive())
        return;

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    QString group = conversation->getGroupName();
    QString boldUser = QString("<b>%1</b>").arg(from);
    QString groupName = QString("<i>%1</i>").arg(group);
    QString title = QString(boldUser + "\r\n" + groupName);
    
    Origin::UIToolkit::OriginNotificationDialog* toast = otm->showToast(Services::SETTING_NOTIFY_INCOMINGCHATROOMMESSAGE.name(),title, message.body());

    if (toast)
    {
        toast->setProperty("conversation", QVariant::fromValue(conversation));
        ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), this, SLOT(showChatWindowForConversation()), Qt::QueuedConnection);
    }

}

void SocialViewController::onGroupRoomDestroyed(QSharedPointer<Origin::Engine::Social::Conversation> conversation, const QString& owner)
{
    // Don't send notification if the Desktop Chat window has focus
    if (mChatWindowManager->isChatWindowActive())
        return;

    // Don't send notification if the OIG Chat window has focus
    if (Origin::Engine::IGOController::instance()->igowm()->visible() && mChatWindowManager->isIGOChatWindowActive())
        return;

    qint64 nucleusId = owner.toDouble();
    
    // Did we destry the room ourselves?
    if (mUser->userId() == nucleusId)
        return;

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    QString title = QString("<b>%1</b>").arg(QString(tr("ebisu_client_groip_room_deleted_title")));
    QString names = QString(conversation->getGroupName());

    QString by = Engine::LoginController::instance()->currentUser()->eaid(); // default to current user

    Chat::JabberID jid = mUser->socialControllerInstance()->chatConnection()->nucleusIdToJabberId(nucleusId);
    Chat::XMPPUser *user = mUser->socialControllerInstance()->chatConnection()->userForJabberId(jid);

    Chat::RemoteUser *remoteUser = dynamic_cast<Chat::RemoteUser*>(user); // Are we a remote user?
    if( remoteUser )
    {
        by = remoteUser->originId();
    }

    QString body = QString(tr("ebisu_client_groip_room_deleted_by.")).arg(names).arg(by); // This need string should be updated to remove the "."

    Origin::UIToolkit::OriginNotificationDialog* toast = otm->showToast("POPUP_ROOM_DESTROYED",title, body);

    if (toast)
    {
        toast->setProperty("conversation", QVariant::fromValue(conversation));
        ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), this, SLOT(showChatWindowForConversation()), Qt::QueuedConnection);
    }
}

void SocialViewController::onGroupDestroyed(QString groupName, qint64 removedBy)
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    Chat::JabberID jid = mUser->socialControllerInstance()->chatConnection()->nucleusIdToJabberId(removedBy);
    QString from = mUser->socialControllerInstance()->chatConnection()->remoteUserForJabberId(jid)->originId();
    QString group = QString("<i>%1</i>").arg(groupName.toHtmlEscaped());
    QString title = QString("<b>%1</b>").arg(QString(tr("ebisu_client_groip_group_deleted_title")).arg(group));
    QString body = QString(tr("ebisu_client_groip_group_deleted")).arg(from);

    otm->showToast("POPUP_GROUP_DESTROYED",title, body);
}

void SocialViewController::onKickedFromRoom(QSharedPointer<Origin::Engine::Social::Conversation> conversation, const QString& owner)
{
    // Don't send notification if the Desktop Chat window has focus
    if (mChatWindowManager->isChatWindowActive())
        return;

    // Don't send notification if the OIG Chat window has focus
    if (Origin::Engine::IGOController::instance()->igowm()->visible() && mChatWindowManager->isIGOChatWindowActive())
        return;

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    QString names = QString("<i>%1</i>").arg(QString(conversation->getGroupName()));
    QString title = QString("<b>%1</b>").arg(QString(tr("ebisu_client_groip_kicked_from_room_title")).arg(names));

    qint64 nucleusId = owner.toDouble();
    Chat::JabberID jid = mUser->socialControllerInstance()->chatConnection()->nucleusIdToJabberId(nucleusId);
    QString by = mUser->socialControllerInstance()->chatConnection()->remoteUserForJabberId(jid)->originId();

    QString body = QString(tr("ebisu_client_groip_kicked_from_room")).arg(by);

    Origin::UIToolkit::OriginNotificationDialog* toast = otm->showToast("POPUP_USER_KICKED_FROM_ROOM",title, body);

    if (toast)
    {
        toast->setProperty("conversation", QVariant::fromValue(conversation));
        ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), this, SLOT(showChatWindowForConversation()), Qt::QueuedConnection);
    }
}

void SocialViewController::onKickedFromGroup(QString groupName, QString groupGuid, qint64 removedBy)
{
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    Chat::JabberID jid = mUser->socialControllerInstance()->chatConnection()->nucleusIdToJabberId(removedBy);
    QString from = mUser->socialControllerInstance()->chatConnection()->remoteUserForJabberId(jid)->originId();
    QString group = QString("<i>%1</i>").arg(groupName.toHtmlEscaped());
    QString title = QString("<b>%1</b>").arg(QString(tr("ebisu_client_groip_kicked_from_group_title")).arg(group));
    QString body = QString(tr("ebisu_client_groip_kicked_from_group")).arg(from);

    otm->showToast("POPUP_USER_KICKED_FROM_GROUP",title, body);
}

#if ENABLE_VOICE_CHAT
void SocialViewController::onVoiceChatConflict(const QString& channel)
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    Engine::Social::Conversation* conversation = dynamic_cast<Engine::Social::Conversation*>(sender());

    if (!conversation)
        return;

    // disconnect anything that might have been connected such as an older conflicted conversation that's still open.
    ORIGIN_VERIFY_DISCONNECT(this, SIGNAL(conflictAccepted(const QString&)), 0, 0);
    ORIGIN_VERIFY_DISCONNECT(this, SIGNAL(conflictDeclined()), 0, 0);
    // Connect our current conflicted conversation so we can trigger the proper join or cancel operation
    ORIGIN_VERIFY_CONNECT_EX(this, SIGNAL(conflictAccepted(const QString&)), conversation, SLOT(joiningVoice(const QString&)), Qt::DirectConnection);
    ORIGIN_VERIFY_CONNECT_EX(this, SIGNAL(conflictDeclined()), conversation, SLOT(cancelingVoice()), Qt::DirectConnection);

    if (Origin::Services::readSetting(Origin::Services::SETTING_NoWarnAboutVoiceChatConflict))
    {
        emit conflictAccepted(channel);
        return;
    }

    using namespace Origin::UIToolkit;

    UIScope scope = ClientScope;
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
       scope = IGOScope;
    }

    OriginWindow::TitlebarItems titlebarItems = OriginWindow::Icon | OriginWindow::Close;
    OriginWindow::WindowContext windowContext = OriginWindow::OIG;
    if(scope != IGOScope)
    {
        windowContext = OriginWindow::Normal;
        titlebarItems |= OriginWindow::Minimize;
    }

    if (mVoiceConflictWindow)
        closeConflictWindow();

    mVoiceConflictWindow = new OriginWindow(
        titlebarItems,
        NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        OriginWindow::ChromeStyle_Dialog, windowContext);

    mVoiceConflictWindow->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_starting_another_voicechat_will_end_current_title"), tr("ebisu_client_you_can_only_be_in_one_voicechat_at_a_time"));
    mVoiceConflictWindow->setTitleBarText(tr("ebisu_client_voice_chat"));

    mVoiceConflictHandled = false;
    ORIGIN_VERIFY_CONNECT(mVoiceConflictWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onConflictButtonClicked()));
    ORIGIN_VERIFY_CONNECT(mVoiceConflictWindow->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(onCloseConflictWindow()));
    // Make this connection so that if the conversation finishes for any reason we close the pop-up
    ORIGIN_VERIFY_CONNECT(conversation, SIGNAL(conversationFinished(Origin::Engine::Social::Conversation::FinishReason)), this,  SLOT(onCloseConflictWindow()));
    ORIGIN_VERIFY_CONNECT(mVoiceClient.data(), SIGNAL(voiceClientDisconnectCompleted(bool)), this, SLOT(onCloseConflictWindow()));
    
    mVoiceConflictWindow->button(QDialogButtonBox::Ok)->setText(tr("ebisu_client_start_voice_chat"));

    QWidget* windowContent = new QWidget();
    QVBoxLayout* verticalLayout = new QVBoxLayout(windowContent);
    verticalLayout->setContentsMargins(0, 0, 0, 0);

    OriginCheckButton* checkButton = new OriginCheckButton(windowContent);
    checkButton->setText(tr("ebisu_client_dont_show_dialog"));
    verticalLayout->addWidget(checkButton);
    mVoiceConflictWindow->setProperty("checkButton", QVariant::fromValue(checkButton));
    mVoiceConflictWindow->setProperty("channel", channel);
    windowContent->adjustSize();
    mVoiceConflictWindow->setContent(windowContent);
    mVoiceConflictWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    if (scope == ClientScope)
    {
        ORIGIN_VERIFY_CONNECT(mVoiceConflictWindow, SIGNAL(rejected()), this, SLOT(onCloseConflictWindow()));
        mVoiceConflictWindow->showWindow();
    }
    else
    {
        Origin::Engine::IIGOWindowManager::WindowBehavior behavior;
        Engine::IGOController::instance()->igowm()->addPopupWindow(mVoiceConflictWindow, Engine::IIGOWindowManager::WindowProperties(), behavior);
        ORIGIN_VERIFY_CONNECT(mVoiceConflictWindow, SIGNAL(rejected()), this, SLOT(onCloseConflictWindow()));
    }
}

void SocialViewController::onConflictButtonClicked()
{
    if (mVoiceConflictHandled)
        return;

    mVoiceConflictHandled = true;

    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    if (mVoiceConflictWindow)
    {
        Origin::UIToolkit::OriginCheckButton* checkButton = mVoiceConflictWindow->property("checkButton").value<Origin::UIToolkit::OriginCheckButton*>();
 
        if (checkButton && checkButton->isChecked())
        {
            Origin::Services::writeSetting(Origin::Services::SETTING_NoWarnAboutVoiceChatConflict, true);
        }
        
        const QString channel = mVoiceConflictWindow->property("channel").toString();
        emit conflictAccepted(channel);

        closeConflictWindow();
    }
}

void SocialViewController::onCloseConflictWindow()
{
    if (mVoiceConflictHandled)
        return;

    mVoiceConflictHandled = true;

    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    emit conflictDeclined();
    closeConflictWindow();
}

void SocialViewController::closeConflictWindow()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    if (mVoiceConflictWindow)
    {
        ORIGIN_VERIFY_DISCONNECT(mVoiceConflictWindow, SIGNAL(rejected()), 0, 0);
        mVoiceConflictWindow->close();
        mVoiceConflictWindow->deleteLater();
        mVoiceConflictWindow->setProperty("CheckButton", QVariant());
        mVoiceConflictWindow = NULL;
    }
}

void SocialViewController::onVoiceUserTalking(const QString& userId)
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }
    if (mVoiceClient.data()->isConnectedToSettings())
    {
        return;
    }

    bool currentUserTalking = false;
    using namespace Origin::Engine;
    UserRef user = LoginController::currentUser();
    Social::SocialController* socialController = LoginController::currentUser()->socialControllerInstance();
    const Social::UserNamesCache *userNames  = socialController->userNames();
    QString originId = "";
    if(user->userId() == userId.toLongLong())
    {
        currentUserTalking = true;
        originId = user->eaid();
        ORIGIN_ASSERT(!originId.isEmpty());
        otm->showVoiceChatIdentifier(originId, OriginToastManager::UserVoiceType_CurrentUserTalking);
    }
    else
    {
        originId = userNames->namesForNucleusId(userId.toULongLong()).originId();
        
        if(originId.isEmpty())
        {
            // IF we can't find the user we can't do anything with them...
            ORIGIN_LOG_ERROR<<"Could not find origin id for user talking.";
            return;
        }

        otm->showVoiceChatIdentifier(originId);
    }
}

void SocialViewController::onVoiceUserStoppedTalking(const QString& userId)
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    using namespace Origin::Engine;
    UserRef user = LoginController::currentUser();
    QString originId = "";
    if(user->userId() == userId.toLongLong())
    {
        originId = user->eaid();
    }
    else
    {
        Social::SocialController* socialController = user->socialControllerInstance();
        const Social::UserNamesCache *userNames  = socialController->userNames();
        originId = userNames->namesForNucleusId(userId.toULongLong()).originId();
    }

    if(originId.isEmpty())
    {
        // IF we can't find the user we can't do anything with them...
        ORIGIN_LOG_ERROR<<"Could not find origin id for user who stopped talking.";
        return;
    }

    otm->removeVoiceChatIdentifier(originId);
}

void SocialViewController::onVoiceLocalUserMuted()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    mCurrentUserMuted = true;

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->showVoiceChatIdentifier(Engine::LoginController::currentUser()->eaid(), OriginToastManager::UserVoiceType_CurrentUserMuted);
}

void SocialViewController::onVoiceLocalUserUnMuted()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    mCurrentUserMuted = false;

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    otm->removeVoiceChatIdentifier(Engine::LoginController::currentUser()->eaid());
}

void SocialViewController::onGameAdded(uint32_t gameId)
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* otm = clientFlow->originToastManager();
    if (!otm)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    if (mCurrentUserMuted)
        otm->showVoiceChatIdentifier(Engine::LoginController::currentUser()->eaid(), OriginToastManager::UserVoiceType_CurrentUserMuted);
}

void SocialViewController::onMutedByAdmin(QSharedPointer<Origin::Engine::Social::Conversation> conv, const Origin::Chat::JabberID& removedBy)
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    //// Don't send notification if the Desktop Chat window has focus
    if (mChatWindowManager->isChatWindowActive())
        return;

    // Don't send notification if the OIG Chat window has focus
    if (Origin::Engine::IGOController::instance()->igowm()->visible() && mChatWindowManager->isIGOChatWindowActive())
        return;
    
    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* manager = clientFlow->originToastManager();
    if (!manager)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    QString from = mUser->socialControllerInstance()->chatConnection()->remoteUserForJabberId(removedBy)->originId();
    QString groupName = conv->getGroupName();
    QString title = QString("<b>%1</b>").arg(QString(tr("ebisu_client_groip_globally_muted_title")));
    QString body = QString(tr("ebisu_client_groip_globally_muted")).arg(from).arg(groupName);

    Origin::UIToolkit::OriginNotificationDialog* toast = manager->showToast("POPUP_GLOBAL_MUTE",title, body);
    if (toast)
    {
        toast->setProperty("conversation", QVariant::fromValue(conv));
        ORIGIN_VERIFY_CONNECT_EX(toast, SIGNAL(clicked()), this, SLOT(showChatWindowForConversation()), Qt::QueuedConnection);
    }
}

void SocialViewController::onVoiceClientDisconnectCompleted(bool joiningVoice)
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    ClientFlow* clientFlow = ClientFlow::instance();
    if (!clientFlow)
    {
        ORIGIN_LOG_ERROR<<"ClientFlow instance not available";
        return;
    }
    OriginToastManager* manager = clientFlow->originToastManager();
    if (!manager)
    {
        ORIGIN_LOG_ERROR<<"OriginToastManager not available";
        return;
    }

    manager->removeAllVoiceChatIdentifiers();
}

void SocialViewController::onVoiceChatConnected(Origin::Engine::Voice::VoiceClient* voiceClient)
{
#if ENABLE_VOICE_CHAT

    const bool isLoggingStats = Services::readSetting(Services::SETTING_LogSonarConnectionStats);

    if (isLoggingStats && voiceClient) {
        if (!voiceChatDebug) {
            voiceChatDebug = new QDockWidget("Voice");
            voiceChatDebug->setFeatures(QDockWidget::DockWidgetMovable);
            voiceChatDebug->setAllowedAreas(NULL);
            voiceChatDebug->setFloating(true);
            voiceChatDebug->setFixedSize(340, 100);
            QFrame* f = new QFrame(voiceChatDebug);
            f->move(18, 18);
            f->setFixedSize(304, 64);
            f->setFrameStyle(QFrame::Panel | QFrame::Sunken);
            f->setLineWidth(2);
            StripChart* g = new StripChart(f, voiceClient, 300, 60);
            g->move(2, 2);
            g->show();
            voiceChatDebug->show();
        }
    }
    
#endif // ENABLE_VOICE_CHAT
}

void SocialViewController::onVoiceChatDisconnected()
{
#if ENABLE_VOICE_CHAT

    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    const bool isLoggingStats = Services::readSetting(Services::SETTING_LogSonarConnectionStats);

    if (isLoggingStats) {
        if (voiceChatDebug) {
            delete voiceChatDebug;
            voiceChatDebug = NULL;
        }
    }
    
#endif // ENABLE_VOICE_CHAT
}

void SocialViewController::onVoiceConnectError(int connectError)
{
#if 0 // Origin X - this dialog has been changed to an inline system message in the chat window
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    ORIGIN_VERIFY_DISCONNECT(mVoiceClient.data(), SIGNAL(voiceConnectError(int)), this, SLOT(onVoiceConnectError(int)));

    QString description(tr("ebisu_client_there_was_an_error_with_your_sound_drivers"));
    description.append("\n\n");
    description.append(tr("ebisu_client_see_origin_help_to_troubleshoot"));
    description.append("\n\n");
    description.append(tr("ebisu_client_directsound_error"));
    description = description.arg(QString::number(connectError, 16).right(8));

    createErrorDialog(
        tr("ebisu_client_unable_to_start_voice_chat_title"), 
        description, 
        tr("ebisu_client_unable_to_start_voice_chat_title"));
#endif
}

#endif //ENABLE_VOICE_CHAT

void SocialViewController::acceptInviteFailed()
{
    createErrorDialog(tr("ebisu_client_unable_to_accept_invite"), tr("ebisu_client_error_try_again_later"), tr("ebisu_client_accept_room_invite"));
}

void SocialViewController::rejectInviteFailed()
{
    createErrorDialog(tr("ebisu_client_unable_to_reject_invite"), tr("ebisu_client_error_try_again_later"), tr("ebisu_client_reject_room_invite"));
}

void SocialViewController::createErrorDialog(const QString& heading, const QString& description, const QString& title)
{
    // Bring up generic error dialog
    UIToolkit::OriginScrollableMsgBox::IconType icon = UIToolkit::OriginScrollableMsgBox::Error;

    UIToolkit::OriginWindow* window = 
        UIToolkit::OriginWindow::alertNonModalScrollable(icon, heading, description, tr("ebisu_client_close"));

    window->setTitleBarText(title);

    if (Engine::IGOController::instance()->isActive())
    {
        window->configForOIG(true);
        Engine::IGOController::instance()->igowm()->addPopupWindow(window, Engine::IIGOWindowManager::WindowProperties());
    }
    else
    {
        window->showWindow();
    }
}

void SocialViewController::showGroupInviteSentDialog(const QObjectList& users)
{
    UIScope scope = ClientScope;
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
        scope = IGOScope;

    // This is stupid but the cost of reusing the existing "invitation sent" UI
    QList<Origin::Chat::RemoteUser*> remoteUsers;
    for(QObjectList::const_iterator it = users.begin(); it != users.end(); it++) 
    {                
        JsInterface::RemoteUserProxy* pr = dynamic_cast<JsInterface::RemoteUserProxy*>(*it);
        if (pr) 
        {
            remoteUsers.append(pr->proxied());
        }
    }
    ClientFlow::instance()->showGroupInviteSentDialog(remoteUsers, scope);
}



BlockReceiver::BlockReceiver(QObject *parent, Engine::UserRef user, quint64 nucleusId)
: QObject(parent)
, mUser(user)
, mNucleusId(nucleusId)
{
}

void BlockReceiver::checkConnection()
{
    if (!Services::Connection::ConnectionStatesService::isUserOnline(mUser->getSession()))
    {
        QString messageBodyString;
        QString titleString;
        // Must have lost connection while the dialog box was present. Show a message of some sort explaining why
        // this is going to fail now.
        using namespace Origin::UIToolkit;

        titleString = tr("ebisu_client_unfriend_and_block_error_offline_header");
        messageBodyString = tr("ebisu_client_unfriend_and_block_error_offline_text");

        OriginWindow* offlineMessage = new OriginWindow(
            (OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
            NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok);

        offlineMessage->msgBox()->setup(OriginMsgBox::NoIcon, titleString, messageBodyString);

        ORIGIN_VERIFY_CONNECT(offlineMessage, SIGNAL(rejected()), offlineMessage, SLOT(close()));
        ORIGIN_VERIFY_CONNECT(offlineMessage->button(QDialogButtonBox::Ok), SIGNAL(clicked()), offlineMessage, SLOT(close()));

        offlineMessage->setModal(true);
        offlineMessage->show();

        return;
    }
    performBlock();
}


void BlockReceiver::performBlock()
{
    ORIGIN_LOG_EVENT << "Blocking a friend";
    Chat::OriginConnection *connection = mUser->socialControllerInstance()->chatConnection();
    Chat::ConnectedUser *currentUser = connection->currentUser();
    const Chat::JabberID jidToBlock = connection->nucleusIdToJabberId(mNucleusId);

    // Get the remote user
    Chat::RemoteUser *remoteUser = connection->remoteUserForJabberId(jidToBlock);

    // The Origin XMPP server handles the block/pending subscription interaction very poorly
    // Clear out any pending requests before we block
    if (remoteUser->subscriptionState().isPendingCurrentUserApproval())
    {
        currentUser->roster()->denySubscriptionRequest(jidToBlock);
    }
    else if (remoteUser->subscriptionState().isPendingContactApproval())
    {
        currentUser->roster()->cancelSubscriptionRequest(jidToBlock);
    }

    // Blocking should always remove the user from the friends list to keep inline with PSN and XBL
    // We have to do this before blocking or we can hit a server bug where we still appear in the other user's roster
    currentUser->roster()->removeContact(remoteUser);

    currentUser->blockList()->addJabberID(connection->nucleusIdToJabberId(mNucleusId));

    GetTelemetryInterface()->Metric_FRIEND_BLOCK_ADD_SENT();
}
    
    
}
}
