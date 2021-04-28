#include <QUrl>

#include "ClientNavigationProxy.h"
#include "RemoteUserProxy.h"
#include "ClientFlow.h"
#include "StoreUrlBuilder.h"
#include "chat/RemoteUser.h"
#include "engine/content/ContentController.h"
#include "engine/igo/IGOController.h"
#include "services/debug/DebugService.h"
#include "services/platform/PlatformService.h"
#include "originwindow.h"
#include "TelemetryAPIDLL.h"
#include "SocialViewController.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

ClientNavigationProxy::ClientNavigationProxy()
{
    //Using signals instead of calling because this is running on a different thread than the IGO Controller
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(openIGOBrowser()), Engine::IGOController::instance(), SLOT(igoShowBrowserForGame()));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(openIGOFriends()), this, SLOT(onOpenIGOFriends()));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(openIGOChat()), this, SLOT(onOpenIGOChat()));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(openIGOCS()), Engine::IGOController::instance(), SLOT(igoShowCustomerSupport()));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(openIGOSettings(Engine::ISettingsViewController::Tab)), Engine::IGOController::instance(), SLOT(igoShowSettings(Engine::ISettingsViewController::Tab)));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(openIGODownloads()), Engine::IGOController::instance(), SLOT(igoShowDownloads()));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(openIGOBroadcast()), Engine::IGOController::instance(), SLOT(igoShowBroadcast()));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(closeIGO()), Engine::IGOController::instance(), SLOT(igoHideUI()));    
}

ClientNavigationProxy::~ClientNavigationProxy()
{

}

void ClientNavigationProxy::showStoreHome()
{
    GetTelemetryInterface()->Metric_ENTITLEMENTS_NONE_GOTO_STORE(EbisuTelemetryAPI::ENTITLEMENTS_NONE_GOTO_STOREHOME);
    Client::ClientFlow::instance()->showStoreHome();
}

void ClientNavigationProxy::showMyGames()
{
    Client::ClientFlow::instance()->showMyGames();
}

void ClientNavigationProxy::showMyProfile(const QString& source)
{
    UIScope scope = ClientScope;
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
        scope = IGOScope;

    Client::ClientFlow::instance()->showMyProfile(scope, stringToProfileSource(source), Engine::IIGOCommandController::CallOrigin_CLIENT);
}

void ClientNavigationProxy::showFriends()
{
    if (Engine::IGOController::instance()->isVisible())
        emit openIGOFriends();
    else
        Client::ClientFlow::instance()->showMyFriends();
}

void ClientNavigationProxy::showAchievements()
{
    Client::ClientFlow::instance()->showAchievementsHome();
}

void ClientNavigationProxy::showAchievementSetDetails(const QString& achievementSetId, const QString& userId, const QString& gameTitle)
{
    Client::ClientFlow::instance()->showAchievementSetDetails(achievementSetId, userId, gameTitle);
}

void ClientNavigationProxy::showFeedbackPage()
{
    Client::ClientFlow::instance()->showFeedbackPage();
}

void ClientNavigationProxy::showStoreProductPage(const QString &productId)
{
    StoreUrlBuilder builder;
    Client::ClientFlow::instance()->showProductIDInStore(productId);
}

void ClientNavigationProxy::showSubscriptionPage()
{
    showStoreUrl(StoreUrlBuilder().subscriptionUrl().toString());
}

void ClientNavigationProxy::showStoreMasterTitlePage(const QString &masterTitleId)
{
    Client::ClientFlow::instance()->showMasterTitleInStore(masterTitleId);
}

void ClientNavigationProxy::showStoreOrderHistory()
{
    Client::ClientFlow::instance()->showOrderHistory();
}

void ClientNavigationProxy::showStoreUrl(const QString &url)
{
    ClientFlow::instance()->showUrlInStore(QUrl(url));
}

void ClientNavigationProxy::showGeneralSettingsPage()
{
    if (Engine::IGOController::instance()->isVisible())
        emit openIGOSettings(Engine::ISettingsViewController::Tab_GENERAL);
    else
        ClientFlow::instance()->showSettingsGeneral();
}

void ClientNavigationProxy::showVoiceSettingsPage()
{
    if (Engine::IGOController::instance()->isVisible())
        emit openIGOSettings(Engine::ISettingsViewController::Tab_VOICE);
    else
        ClientFlow::instance()->showSettingsVoice();
}

void ClientNavigationProxy::showPromoManager(const QString &productId, const QString& promoTypeString, const QString& scopeString)
{
    Engine::Content::EntitlementRef entitlement = 
        Engine::Content::ContentController::currentUserContentController()->entitlementById(productId);

    PromoContext::PromoType promoType = PromoContext::promoTypeFromString(promoTypeString);
    PromoContext::Scope scope = PromoContext::scopeFromString(scopeString);

    // Note that entitlement pointer could be null if the entitlement for the
    // given product ID wasn't found.
    ClientFlow::instance()->showPromoDialog(PromoBrowserContext(promoType, scope), entitlement);
}

void ClientNavigationProxy::launchExternalBrowser(const QString &url)
{
    if (Engine::IGOController::instance()->isVisible())
    {
        emit Engine::IGOController::instance()->showBrowser(url, false);
    }
    else
    {
        Origin::Services::PlatformService::asyncOpenUrl(QUrl::fromEncoded(url.toUtf8()));
    }
}

void ClientNavigationProxy::showIGOBrowser()
{
    emit openIGOBrowser();
}

void ClientNavigationProxy::showPDLCStore(const QString &contentId)
{
    using namespace Engine::Content;

    EntitlementRef entitlement = 
        ContentController::currentUserContentController()->entitlementById(contentId);

    if (!entitlement.isNull())
    {
        ClientFlow::instance()->showPDLCStore(entitlement);
    }
}

void ClientNavigationProxy::showDownloadProgressDialog()
{
    if (Engine::IGOController::instance()->isVisible())
        emit openIGODownloads();
    else
        Client::ClientFlow::instance()->showDownloadProgressDialog();
}

void ClientNavigationProxy::showGameDetails(const QString &contentId)
{
    using namespace Engine::Content;

    EntitlementRef entitlement = 
        ContentController::currentUserContentController()->entitlementById(contentId);

    if (!entitlement.isNull())
    {
        Client::ClientFlow::instance()->showGameDetails(entitlement);
    }
}

void ClientNavigationProxy::showStoreFreeGames()
{
    GetTelemetryInterface()->Metric_ENTITLEMENTS_NONE_GOTO_STORE(EbisuTelemetryAPI::ENTITLEMENTS_NONE_GOTO_FREEGAMES);
    Client::ClientFlow::instance()->showStoreFreeGames();
}

void ClientNavigationProxy::showStoreOnTheHouse(bool hasGames, const QString& trackingParam)
{
    if (hasGames)
        GetTelemetryInterface()->Metric_STORE_NAVIGATE_OTH(EbisuTelemetryAPI::OtHMyGames);
    else
        GetTelemetryInterface()->Metric_STORE_NAVIGATE_OTH(EbisuTelemetryAPI::OtHNoGames);
    Client::ClientFlow::instance()->showStoreOnTheHouse(trackingParam);
}

void ClientNavigationProxy::showAvatarChooser()
{
    ClientFlow::instance()->showSelectAvatar();
}

void ClientNavigationProxy::showOriginHelp()
{
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
        emit openIGOCS();
    }
    else
    {
        ClientFlow::instance()->showHelp();
    }
}

void ClientNavigationProxy::showFriendSearchDialog()
{
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
        ClientFlow::instance()->showFriendSearchDialog(IGOScope, Engine::IIGOCommandController::CallOrigin_CLIENT);
    }
    else
    {
        Client::ClientFlow::instance()->showFriendSearchDialog();
    }
}

void ClientNavigationProxy::showProfileSearchResult(const QString &keywords)
{

    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
        Client::ClientFlow::instance()->showProfileSearchResult(keywords, IGOScope);
    }
    else
    {
        Client::ClientFlow::instance()->showProfileSearchResult(keywords, ClientScope);
    }
}

void ClientNavigationProxy::showCreateGroupDialog()
{
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
        ClientFlow::instance()->showCreateGroupDialog(IGOScope);
    }
    else
    {
        Client::ClientFlow::instance()->showCreateGroupDialog();
    }
}

void ClientNavigationProxy::showCreateRoom(const QString& groupName, const QString& groupGuid)
{
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
        ClientFlow::instance()->showCreateRoomDialog(groupName, groupGuid, IGOScope);
    }
    else
    {
        Client::ClientFlow::instance()->showCreateRoomDialog(groupName, groupGuid);
    }
}

void ClientNavigationProxy::showEditGroup(const QString& groupName, const QString& groupGuid)
{
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
        ClientFlow::instance()->showEditGroupDialog(groupName, groupGuid, IGOScope);
    }
    else
    {
        Client::ClientFlow::instance()->showEditGroupDialog(groupName, groupGuid);
    }
}

void ClientNavigationProxy::showDeleteGroup(const QString& groupName, const QString& groupGuid)
{
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
        ClientFlow::instance()->showDeleteGroupDialog(groupName, groupGuid, IGOScope);
    }
    else
    {
        Client::ClientFlow::instance()->showDeleteGroupDialog(groupName, groupGuid);
    }
}

void ClientNavigationProxy::showLeaveGroup(const QString& groupName, const QString& groupGuid)
{
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
        ClientFlow::instance()->showLeaveGroupDialog(groupName, groupGuid, IGOScope);
    }
    else
    {
        Client::ClientFlow::instance()->showLeaveGroupDialog(groupName, groupGuid);
    }
}

void ClientNavigationProxy::showDeleteRoom(const QString& groupGuid, const QString& channelId, const QString& roomName)
{
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
        ClientFlow::instance()->showDeleteRoomDialog(groupGuid, channelId, roomName, IGOScope);
    }
    else
    {
        ClientFlow::instance()->showDeleteRoomDialog(groupGuid, channelId, roomName);
    }
}

void ClientNavigationProxy::showEnterRoomPassword(const QString& groupGuid, const QString& channelId)
{
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
        ClientFlow::instance()->showEnterRoomPasswordDialog(groupGuid, channelId, IGOScope);
    }
    else
    {
        Client::ClientFlow::instance()->showEnterRoomPasswordDialog(groupGuid, channelId);
    }
}

void ClientNavigationProxy::showInviteFriendsToGroupDialog(const QString& groupGuid)
{
    UIScope scope = ClientScope;
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
       scope = IGOScope;
    }
    ClientFlow::instance()->showInviteFriendsToGroupDialog(groupGuid, scope);
}

void ClientNavigationProxy::showInviteFriendsToRoomDialog(const QString& groupGuid, const QString& channelId, const QString& conversationId)
{
    UIScope scope = ClientScope;
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
        scope = IGOScope;
    }
    ClientFlow::instance()->showInviteFriendsToRoomDialog(groupGuid, channelId, conversationId, scope);
}


void ClientNavigationProxy::showYouNeedFriendsDialog()
{
    UIScope scope = ClientScope;
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
        scope = IGOScope;
    }
    ClientFlow::instance()->showYouNeedFriendsDialog(scope);
}

void ClientNavigationProxy::showGroupMembersDialog(const QString& groupName, const QString& groupGuid)
{
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
        ClientFlow::instance()->showGroupMembersDialog(groupName, groupGuid, IGOScope);
    }
    else
    {
        Client::ClientFlow::instance()->showGroupMembersDialog(groupName, groupGuid);
    }
}

void ClientNavigationProxy::showGroupBannedMembersDialog(const QString& groupName, const QString& groupGuid)
{
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
        ClientFlow::instance()->showGroupBannedMembersDialog(groupName, groupGuid, IGOScope);
    }
    else
    {
        Client::ClientFlow::instance()->showGroupBannedMembersDialog(groupName, groupGuid);
    }
}

void ClientNavigationProxy::showRemoveGroupUser(const QString& groupGuid, QObject* user)
{
    RemoteUserProxy* remoteUser = dynamic_cast<RemoteUserProxy*>(user);
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
        ClientFlow::instance()->showRemoveGroupUserDialog(groupGuid, QString::number(remoteUser->proxied()->nucleusId()), remoteUser->nickname().toString(), IGOScope);
    }
    else
    {
        Client::ClientFlow::instance()->showRemoveGroupUserDialog(groupGuid, QString::number(remoteUser->proxied()->nucleusId()), remoteUser->nickname().toString());
    }
}

void ClientNavigationProxy::showRemoveRoomUser(const QString& groupGuid, const QString& channelId, QObject* user)
{
    RemoteUserProxy* remoteUser = dynamic_cast<RemoteUserProxy*>(user);
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
        ClientFlow::instance()->showRemoveRoomUserDialog(groupGuid, channelId, remoteUser->proxied()->originId(), IGOScope);
    }
    else
    {
        Client::ClientFlow::instance()->showRemoveRoomUserDialog(groupGuid, channelId, remoteUser->proxied()->originId());
    }
}

void ClientNavigationProxy::showPromoteToAdminSuccessDialog(QObject* user, const QString& groupName)
{
    RemoteUserProxy* remoteUser = dynamic_cast<RemoteUserProxy*>(user);
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
        ClientFlow::instance()->showPromoteToAdminSuccessDialog(groupName, remoteUser->proxied()->originId(), IGOScope);
    }
    else
    {
        Client::ClientFlow::instance()->showPromoteToAdminSuccessDialog(groupName, remoteUser->proxied()->originId());
    }
}

void ClientNavigationProxy::showDemoteToMemberSuccessDialog(QObject* user, const QString& groupName)
{
    RemoteUserProxy* remoteUser = dynamic_cast<RemoteUserProxy*>(user);
    if (Engine::IGOController::instance() && Engine::IGOController::instance()->isActive())
    {
        ClientFlow::instance()->showDemoteToMemberSuccessDialog(groupName, remoteUser->proxied()->originId(), IGOScope);
    }
    else
    {
        Client::ClientFlow::instance()->showDemoteToMemberSuccessDialog(groupName, remoteUser->proxied()->originId());
    }
}

void ClientNavigationProxy::showMyAccount()
{
    ClientFlow::instance()->view()->onMainMenuAccount();
}

void ClientNavigationProxy::showMyAccountPath(const QString& path, const QString& status)
{
    if(path == "Home")
        ClientFlow::instance()->view()->onMainMenuAccount();
    else if(path == "Privacy")
        ClientFlow::instance()->showAccountProfilePrivacy();
    else if(path == "Security")
        ClientFlow::instance()->showAccountProfileSecurity();
    else if(path == "OrderHistory")
        ClientFlow::instance()->showOrderHistory();
    else if(path == "PaymentShipping")
        ClientFlow::instance()->showAccountProfilePaymentShipping();
    else if(path == "Subscription")
        ClientFlow::instance()->showAccountProfileSubscription(status);
    else if(path == "Redemption")
        ClientFlow::instance()->showAccountProfileRedeem();
}

void ClientNavigationProxy::showBroadcast()
{
    // Adding function instead of plain signal in case we want to show broadcast in not oig one day.
    emit openIGOBroadcast();
}

void ClientNavigationProxy::onOpenIGOChat()
{
    Engine::IGOController::instance()->igoShowChatWindow(Engine::IIGOCommandController::CallOrigin_CLIENT);
}

void ClientNavigationProxy::onOpenIGOFriends()
{
    Engine::IGOController::instance()->igoShowFriendsList(Engine::IIGOCommandController::CallOrigin_CLIENT);
}

}
}
}
