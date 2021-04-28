package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.lib.pageobjects.social.SocialHubMinimized;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that when a user views the profile tab and modifies the presence status in the 'Social Hub'
 * that the user's presence circle and text change accordingly
 *
 * @author jdickens
 */
public class OAProfileUserPresence extends EAXVxTestTemplate {

    @TestRail(caseId = 13047)
    @Test(groups = {"social"})
    public void testProfileUserPresence(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_SMALL);
        final String offerId = entitlement.getOfferId();
        final String entitlementName = entitlement.getName();
        final UserAccount userAccount = AccountManager.getEntitledUserAccount(entitlement);
        final String userAccountName = userAccount.getUsername();

        logFlowPoint("Login to Origin with an account that has any entitlement"); // 1
        logFlowPoint("Navigate to the user's profile and verify that you can see the user's status"); // 2
        logFlowPoint("Change the user's status to 'Online' and verify that the presence status color is green"); // 3
        logFlowPoint("Change the user's status to 'Away' and verify that the presence status color is red"); // 4
        logFlowPoint("Change the user's status to 'Invisible' and verify that the presence status color is grey"); // 5
        logFlowPoint("Navigate to the 'Game Library' and download, install, and then launch any entitlement"); // 7
        logFlowPoint("Verify that the user's status is 'In-Game' and that the presence status color is blue"); // 8

        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin as: " + userAccountName + ".");
        } else {
            logFailExit("Could not log into Origin as: " + userAccountName + ".");
        }

        // 2
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.selectViewMyProfile();
        ProfileHeader userProfile = new ProfileHeader(driver);
        if (userProfile.verifyPresenceStatusVisible()) {
            logPass("Successfully navigated to the user's profile and verified that the user's presence status is visible.");
        } else {
            logFailExit("Failed to navigate to the user's profile or verify that the user's presence status is visible.");
        }

        // 3
        if (userProfile.verifyOnlineStatusColor()) {
            logPass("Verified that the user's presence status color is green (the user is online).");
        } else {
            logFailExit("Failed to verify that the user's presence status color is green (the user is online).");
        }

        // 4
        new SocialHubMinimized(driver).restoreSocialHub();
        SocialHub userSocialHub = new SocialHub(driver);
        userSocialHub.setUserStatusAway();
        if (userProfile.verifyAwayStatusColor()) {
            logPass("Successfully set the user's status to 'Away' and verified that the user's presence status color is red.");
        } else {
            logFailExit("Failed to set the user's status to 'Away' or verify that the user's presence status color is red.");
        }

        // 5
        userSocialHub.setUserStatusInvisible();
        if (userProfile.verifyInvisibleStatusColor()) {
            logPass("Successfully set the user's status to 'Invisible' and verified that the user's presence status color is grey.");
        } else {
            logFailExit("Failed to set the user's status to 'Invisible' or verify that the user's presence status color is grey.");
        }

        // 7
        userSocialHub.setUserStatusOnline();
        userSocialHub.clickMinimizeSocialHub(); // Necessary because sometimes the social hub blocks the entitlement to right click for downloading
        new NavigationSidebar(driver).gotoGameLibrary();
        boolean isEntitlementDownloadSuccessful = MacroGameLibrary.downloadFullEntitlement(driver, offerId);
        GameTile gameTile = new GameTile(driver, entitlement.getOfferId());
        Waits.pollingWait(() -> gameTile.isReadyToPlay());
        gameTile.play();
        boolean isGameLaunched =  Waits.pollingWaitEx(() ->entitlement.isLaunched(client));
        if (isEntitlementDownloadSuccessful && isGameLaunched) {
            logPass("Successfully downloaded, installed, and launched " + entitlementName);
        } else {
            logFail("Failed to download, launch, or install " + entitlementName);
        }

        // 8
        miniProfile.selectViewMyProfile();
        if (userProfile.verifyInGameStatusColor()) {
            logPass("Verified that the user's presence status color is blue after launching a game.");
        } else {
            logFailExit("Failed to verify that the user's presence status color is blue after launching a game.");
        }

        softAssertAll();
    }
}
