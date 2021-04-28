#ifndef _CLIENTNAVIGATIONPROXY_H
#define _CLIENTNAVIGATIONPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>

#include "services/plugin/PluginAPI.h"
#include "engine/igo/IGOController.h"
namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API ClientNavigationProxy : public QObject
{
    Q_OBJECT
public:
    ClientNavigationProxy();
    ~ClientNavigationProxy();
    Q_INVOKABLE void showStoreHome();
    Q_INVOKABLE void showStoreProductPage(const QString &productId);
    Q_INVOKABLE void showStoreMasterTitlePage(const QString &masterTitleId);
    Q_INVOKABLE void showStoreOrderHistory();
    Q_INVOKABLE void showStoreFreeGames();
    Q_INVOKABLE void showStoreOnTheHouse(bool hasGames, const QString& trackingParam = "");
    Q_INVOKABLE void showStoreUrl(const QString &);
    Q_INVOKABLE void showSubscriptionPage();

    // Settings pages
    Q_INVOKABLE void showGeneralSettingsPage();
    Q_INVOKABLE void showVoiceSettingsPage();

    /// \brief Displays the promo manager for the given product ID and governs
    /// its behavior using PromoContext strings.
    /// \param productId The Offer ID of the entitlement to invoke the promo manager upon.
    /// \param promoTypeString The "promo type", represented as a string.
    /// \param scopeString Promo "scope" string under which the promo manager will function.
    /// \sa PromoContext::PromoType, PromoContext::Scope, PromoBrowserContext, PromoBrowserViewController
    Q_INVOKABLE void showPromoManager(const QString &productId, const QString& promoTypeString, const QString& scopeString);

    Q_INVOKABLE void showMyGames();
    Q_INVOKABLE void showMyProfile(const QString& source = "MY_PROFILE");
    Q_INVOKABLE void showFriends();
    Q_INVOKABLE void showAchievements();
    Q_INVOKABLE void showAchievementSetDetails(const QString& achievementSetId, const QString& userId = "", const QString& gameTitle = "");

    Q_INVOKABLE void showFeedbackPage();

    Q_INVOKABLE void launchExternalBrowser(const QString &url);
    Q_INVOKABLE void showIGOBrowser();
    Q_INVOKABLE void showPDLCStore(const QString &contentId);

    Q_INVOKABLE void showDownloadProgressDialog();

    Q_INVOKABLE void showGameDetails(const QString &contentId);

    Q_INVOKABLE void showAvatarChooser();

    Q_INVOKABLE void showOriginHelp();

    Q_INVOKABLE void showFriendSearchDialog();
    Q_INVOKABLE void showProfileSearchResult(const QString &keywords);

    Q_INVOKABLE void showCreateGroupDialog();
    Q_INVOKABLE void showCreateRoom(const QString& groupName, const QString& groupGuid);
    Q_INVOKABLE void showEditGroup(const QString& groupname, const QString& groupGuid);
    Q_INVOKABLE void showDeleteGroup(const QString& groupName, const QString& groupGuid);
    Q_INVOKABLE void showLeaveGroup(const QString& groupName, const QString& groupGuid);
    Q_INVOKABLE void showDeleteRoom(const QString& groupGuid, const QString& channelId, const QString& roomName);
    Q_INVOKABLE void showEnterRoomPassword(const QString& groupGuid, const QString& channelId);
    Q_INVOKABLE void showInviteFriendsToGroupDialog(const QString& groupGuid);
    Q_INVOKABLE void showInviteFriendsToRoomDialog(const QString& groupGuid, const QString& channelId, const QString& conversationId);
    Q_INVOKABLE void showYouNeedFriendsDialog();
    Q_INVOKABLE void showGroupMembersDialog(const QString& groupName, const QString& groupGuid);
    Q_INVOKABLE void showGroupBannedMembersDialog(const QString& groupName, const QString& groupGuid);
    Q_INVOKABLE void showRemoveGroupUser(const QString& groupGuid, QObject* user);
    Q_INVOKABLE void showRemoveRoomUser(const QString& groupGuid, const QString& channelId, QObject* user);
    Q_INVOKABLE void showPromoteToAdminSuccessDialog(QObject* user, const QString& groupName);
    Q_INVOKABLE void showDemoteToMemberSuccessDialog(QObject* user, const QString& groupName);

    /// \brief shows user's profile
    Q_INVOKABLE void showMyAccount();
    Q_INVOKABLE void showMyAccountPath(const QString& path, const QString& status = "");

    Q_INVOKABLE void showBroadcast();

signals:
    void openIGOBrowser();
    void openIGOFriends();
    void openIGOChat();
    void openIGOCS();
    void openIGOSettings(Engine::ISettingsViewController::Tab);
    void openIGODownloads();
    void closeIGO();
    void openIGOBroadcast();

private slots:
    void onOpenIGOChat();
    void onOpenIGOFriends();
};

}
}
}

#endif
