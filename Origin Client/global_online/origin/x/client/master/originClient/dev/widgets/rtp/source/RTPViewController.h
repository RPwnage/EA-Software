/////////////////////////////////////////////////////////////////////////////
// RTPViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef RTPVIEWCONTROLLER_H
#define RTPVIEWCONTROLLER_H

#include <QObject>
#include <QSharedPointer>
#include <QJsonObject>
#include "engine/content/ContentTypes.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace UIToolkit
{
    class OriginWindow;
}

namespace Client
{
class ORIGIN_PLUGIN_API RTPViewController : public QObject
{
    Q_OBJECT

public:
    RTPViewController(QObject *parent = 0);
    ~RTPViewController();

    enum rtpHiddenUI
    {
        ShowNothing = 0,
        ShowDeveloperMode,
        ShowOfAgeContent
    };

    /// \brief Show an error dialog indicating that Origin is currently offline
    void showOfflineErrorDialog();
    void showGameShouldBeInstalledErrorDialog(const QString& gameTitle);
    void showGameUnreleasedErroDialog(const QString& gameTitle, const QString& releasedate);
    void showWrongCodeDialog(const QString& gameTitle);
    void showNotEntitledDialog(const QString& eaId, const QString& gameTitle, const rtpHiddenUI& showOfAgeContent, const QString& storeLink);
    void showIncorrectEnvironment(const QString& gameTitle, const QString& currentEnv, const rtpHiddenUI& developerModeEnabled);
    void showFreeTrialExpiredDialog(const Engine::Content::EntitlementRef entitlement);
    void showEarlyTrialExpiredDialog(const Engine::Content::EntitlementRef entitlement);
    void showEarlyTrialDisabledByAdminDialog(const Engine::Content::EntitlementRef entitlement);
    void showSystemRequirementsNotMetDialog(const Engine::Content::EntitlementRef entitlement);
    void showOnlineRequiredForEarlyTrialDialog(const QString& gameTitle);
    void showOnlineRequiredForTrialDialog();
    void showBusyDialog();
    void showSubscriptionOfflinePlayExpired();
    void showSubscriptionMembershipExpired(const Engine::Content::EntitlementRef entitlement, const bool& isUnderAge);
    void showEntitlementRetiredFromSubscription(const Engine::Content::EntitlementRef entitlement, const bool& isUnderAge);
    void showSubscriptionNotAvailableOnPlatform(const QString& eaId, const QString& gameTitle, const bool& isUnderAge, const QString& storeLink);

public slots:
    void onCommandLinksDone(QJsonObject);
    void onFreeTrialExpiredDone(QJsonObject);

private slots:
#if 0 //TODO FIXUP_ORIGINX
    void showProductIdInStore(const QString &productId);
#endif
    void onGoOnline();

signals:
    void showProductPage();
    void cancel();
    void redeemGame();
    void goOnline();
    void notThisUser();
    void purchaseGame(const QString& masterTitleId);
    void renewSubscriptionMembership();
    void showDeveloperTool();
};
}
}

#endif