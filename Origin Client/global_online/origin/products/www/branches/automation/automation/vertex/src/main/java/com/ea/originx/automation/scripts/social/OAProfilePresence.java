package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.macroaction.MacroGameLibrary;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearch;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearchResults;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.gamelibrary.GameTile;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.lib.pageobjects.social.SocialHubMinimized;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.originx.automation.lib.resources.games.EntitlementInfoHelper;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.games.EntitlementInfo;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Checks the appearance of a user's profile as the presence status changes. Verifies changes
 * in both the user's client and the client of a friend account
 *
 * @author jdickens
 */
public class OAProfilePresence extends EAXVxTestTemplate {

    public enum TEST_TYPE {
        RELEASE,
        NON_RELEASE
    }

    public void testProfilePresence(ITestContext context, TEST_TYPE test_type) throws Exception {

        final OriginClient clientA = OriginClientFactory.create(context);
        final OriginClient clientB = OriginClientFactory.create(context);

        final EntitlementInfo entitlement = EntitlementInfoHelper.getEntitlementInfoForId(EntitlementId.DIP_SMALL);
        final String offerId = entitlement.getOfferId();
        final String entitlementName = entitlement.getName();
        final UserAccount userAccountA = AccountManager.getRandomAccount();
        String userAccountAName = userAccountA.getUsername();
        UserAccount userAccountB = AccountManager.getEntitledUserAccount(entitlement);
        String userAccountBName = userAccountB.getUsername();
        UserAccount strangerAccount = AccountManager.getRandomAccount();
        String strangerAccountName = strangerAccount.getUsername();
        userAccountA.cleanFriends();
        userAccountB.cleanFriends();
        strangerAccount.cleanFriends();
        userAccountA.addFriend(userAccountB);

        logFlowPoint("Login to Origin as User A"); // 1
        if (test_type == TEST_TYPE.NON_RELEASE) {
            logFlowPoint("Navigate to the profile of a stranger's account in client A and verify the presence is not visible"); // 1a
        }
        logFlowPoint("Navigate to User B's profile in client A and verify the presence is 'Offline'"); // 2
        logFlowPoint("Login to Origin as User B that is entitled with any entitlement"); // 3
        logFlowPoint("Navigate to User B's profile in client B and verify that User B's presence is 'Online' in both clients"); // 4
        logFlowPoint("Change User B's status to 'Invisible' and verify that User B's presence is 'Invisible' in both clients"); // 5
        logFlowPoint("Change User B's status to 'Away' and verify that User B's presence is 'Away' in both clients"); // 6
        logFlowPoint("Navigate to the 'Game Library' in client B and download, install, and launch any entitlement"); // 7
        logFlowPoint("Navigate to the profile of User B in client B and verify that in both clients the user's presence is 'In-Game'"); // 8

        // 1
        WebDriver driverA = startClientObject(context, clientA);
        if (MacroLogin.startLogin(driverA, userAccountA)) {
            logPass("Successfully logged into Origin as: " + userAccountAName);
        } else {
            logFailExit("Could not log into Origin as: " + userAccountAName);
        }

        ProfileHeader profileHeaderInClientA = new ProfileHeader(driverA);
        if (test_type == TEST_TYPE.NON_RELEASE) {
            // 1a
            new GlobalSearch(driverA).enterSearchText(strangerAccount.getEmail());
            new GlobalSearchResults(driverA).viewProfileOfUser(strangerAccount);
            if (!profileHeaderInClientA.verifyPresenceStatusVisible()) {
                logPass("Successfully navigated to the stranger's profile in client A and verified that " + strangerAccountName + "'s presence is not visible.");
            } else {
                logFailExit("Failed to navigate to the stranger's profile in client A or verify that " + strangerAccountName + "'s presence is not visible.");
            }
        }

        // 2
        new GlobalSearch(driverA).enterSearchText(userAccountB.getEmail());
        new GlobalSearchResults(driverA).viewProfileOfUser(userAccountB);
        if (profileHeaderInClientA.verifyOfflineStatusColor()) {
            logPass("Successfully navigated to the User B's profile in client A and verified that " + userAccountBName + " is offline.");
        } else {
            logFailExit("Failed to navigate to the User B's profile in client A or verify that " + userAccountBName + " is offline.");
        }

        // 3
        WebDriver driverB = startClientObject(context, clientB);
        if (MacroLogin.startLogin(driverB, userAccountB)) {
            logPass("Successfully logged into Origin as: " + userAccountBName);
        } else {
            logFailExit("Could not log into Origin as: " + userAccountBName);
        }

        // 4
        new MiniProfile(driverB).selectViewMyProfile();
        ProfileHeader profileHeaderInClientB = new ProfileHeader(driverB);
        // adding polling wait because there can be a slight delay before Client A detects that User B is online
        boolean isUserOnlineInClientA = Waits.pollingWait(() ->profileHeaderInClientA.verifyOnlineStatusColor());
        boolean isUserOnlineInClientB = profileHeaderInClientB.verifyOnlineStatusColor();
        if (isUserOnlineInClientA && isUserOnlineInClientB) {
            logPass("Verified that User B's presence is 'Online' in both clients.");
        } else {
            logFail("Failed to verify that User B's presence is 'Online' in both clients.");
        }

        // 5
        new SocialHubMinimized(driverB).restoreSocialHub();
        SocialHub clientBSocialHub = new SocialHub(driverB);
        clientBSocialHub.setUserStatusInvisible();
        boolean isUserInvisibleInClientA = profileHeaderInClientA.verifyInvisibleStatusColor();
        boolean isUserInvisibleInClientB = profileHeaderInClientB.verifyInvisibleStatusColor();
        if (isUserInvisibleInClientA && isUserInvisibleInClientB) {
            logPass("Verified that User B's presence is 'Invisible' in both clients.");
        } else {
            logFail("Failed to verify User B's user's presence is 'Invisible' in both clients.");
        }

        // 6
        clientBSocialHub.setUserStatusAway();
        boolean isUserAwayInClientA = profileHeaderInClientA.verifyAwayStatusColor();
        boolean isUserAwayInClientB = profileHeaderInClientB.verifyAwayStatusColor();
        if (isUserAwayInClientA && isUserAwayInClientB) {
            logPass("Verified that User B's presence is 'Away' in both clients.");
        } else {
            logFail("Failed to verify that User B's presence is 'Away' in both clients.");
        }

        // 7
        clientBSocialHub.clickMinimizeSocialHub(); // Necessary because sometimes the social hub blocks the entitlement to right click for downloading
        new NavigationSidebar(driverB).gotoGameLibrary();
        boolean isEntitlementDownloadSuccessful = MacroGameLibrary.downloadFullEntitlement(driverB, offerId);
        GameTile gameTile = new GameTile(driverB, offerId);
        Waits.pollingWait(() -> gameTile.waitForReadyToPlay());
        gameTile.play();
        boolean isGameLaunched = Waits.pollingWaitEx(() ->entitlement.isLaunched(clientB));
        if (isEntitlementDownloadSuccessful && isGameLaunched) {
            logPass("Successfully downloaded, installed, and launched " + entitlementName + "from client B");
        } else {
            logFailExit("Failed to download, launch, or install " + entitlementName + "from client B");
        }

        // 8
        new MiniProfile(driverB).selectViewMyProfile();
        // adding polling waits because after launching the game because it can take time for the User B's
        // status to show in client A
        boolean isUserInGameInClientA = Waits.pollingWait(() ->profileHeaderInClientA.verifyInGameStatusColor());
        boolean isUserInGameInClientB = profileHeaderInClientB.verifyInGameStatusColor();
        if (isUserInGameInClientA && isUserInGameInClientB) {
            logPass("Verified that User B's presence is 'In-Game' in both clients.");
        } else {
            logFail("Failed to verify that User B's presence is 'In-Game' in both clients.");
        }

        softAssertAll();
    }
}