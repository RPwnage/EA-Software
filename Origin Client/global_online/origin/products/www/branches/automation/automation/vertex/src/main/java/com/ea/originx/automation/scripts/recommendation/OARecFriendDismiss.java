package com.ea.originx.automation.scripts.recommendation;

import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroAccount;
import com.ea.originx.automation.lib.macroaction.MacroDiscover;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.account.PrivacySettingsPage.PrivacySetting;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.discover.RecFriendsTile;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.utils.EntitlementService;
import com.ea.vx.originclient.utils.Waits;
import com.ea.vx.originclient.webdrivers.BrowserType;
import java.util.Arrays;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests friend recommendations for users 1 and 2 with a legal friend 3 with
 * user 1 dismissing the friend recommendation for user 2
 *
 * @author palui
 */
public class OARecFriendDismiss extends EAXVxTestTemplate {

    @TestRail(caseId = 159492)
    @Test(groups = {"recommendation", "full_regression"})
    public void testRecFriendDismiss(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final boolean isClient = ContextHelper.isOriginClientTesing(context);

        final UserAccount user1 = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        final UserAccount user2 = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        UserAccount friend3 = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();

        String username1 = user1.getUsername();
        String username2 = user2.getUsername();
        String friendName3 = friend3.getUsername();

        // grant entitlements to users in order for them to see 'People You May Know' section
        EntitlementService.grantEntitlement(user1, EntitlementId.BF4_STANDARD.getOfferId());
        EntitlementService.grantEntitlement(user2, EntitlementId.BF4_STANDARD.getOfferId());
        EntitlementService.grantEntitlement(friend3, EntitlementId.BF4_STANDARD.getOfferId());

        UserAccount users[] = {user1, user2};
        UserAccount friends[] = {friend3};
        UserAccount allUsers[] = {user1, user2, friend3};
        PrivacySetting settings[] = {PrivacySetting.EVERYONE, PrivacySetting.EVERYONE, PrivacySetting.EVERYONE};

        String usernames[] = {username1, username2};
        String friendNames[] = {friendName3};

        logFlowPoint(String.format("Set privacies for newly registered users 1-2 and friend user 3 to %s respectively", Arrays.toString(settings))); // 1
        logFlowPoint("Add friend user 3 to both users 1 and 2. Wait 30 seconds and login to user 1"); // 2
        logFlowPoint("Verify user 2 is recommended as a friend to user 1"); // 3
        logFlowPoint("Verify 'Recommend Friends' tile for user 2 has both an 'Add Friend' button and a 'Dismiss' link"); // 4
        logFlowPoint("Click 'Dismiss' link on the 'Recommended Friend' tile for user 2 and verify user 2 is no longer a recommended friend of user 1"); // 5

        // 1
        final WebDriver driver = startClientObject(context, client);
        String originUrl = driver.getCurrentUrl();
        boolean setPrivacyOK = true;
        for (int i = 0; i < allUsers.length; i++) {
            if (isClient) {
                final WebDriver settingsDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
                if (!MacroAccount.setPrivacyThroughBrowserLogin(settingsDriver, allUsers[i], settings[i])) {
                    setPrivacyOK = false;
                    settingsDriver.close();
                }
            } else {
                if (!MacroAccount.setPrivacyThroughBrowserLogin(driver, allUsers[i], settings[i])) {
                    setPrivacyOK = false;
                }
            }
        }
        if (!isClient) {
            driver.get(originUrl); // if not client, go back to origin
        }

        if (setPrivacyOK) {
            logPass(String.format("Verified privacies for newly-registered users 1-2 %s and friend user 3 %s have been set to %s respectively",
                    Arrays.toString(usernames), Arrays.toString(friendNames), Arrays.toString(settings)));
        } else {
            logFailExit(String.format("Failed: Cannot set privacies for newly-registered users 1-2 %s or friend user 3 %s to %s respectively",
                    Arrays.toString(usernames), Arrays.toString(friendNames), Arrays.toString(settings)));
        }

        // 2
        MacroSocial.cleanAndAddFriendship(users, friends);
        sleep(30000); // Pause to ensure friends recommendation is updated with the friends change. Per EADP this should happen within 30 seconds
        if (MacroLogin.startLogin(driver, user1)) {
            logPass(String.format("Verified login successful as user 1 %s", username1));
        } else {
            logFailExit(String.format("Failed: Cannot login as user 1 %s", username1));
        }

        // 3
        if (MacroDiscover.verifyUserRecommendedAsFriend(driver, user2)) {
            logPass(String.format("Verified user 2 %s is shown as a recommended friend of user 1 %s", username2, username1));
        } else {
            logFailExit(String.format("Failed: User 2 %s is not shown as a recommended friend of user 1 %s", username2, username1));
        }

        // 4
        RecFriendsTile recFriendsTile = new DiscoverPage(driver).getRecFriendsSection().getRecFriendsTile(user2);
        recFriendsTile.hoverOnTile();
        if (recFriendsTile.verifyAddFriendButtonVisible() && recFriendsTile.verifyDismissLinkVisible()) {
            logPass(String.format("Verified 'Recommend Friends' tile for user 2 %s has both an 'Add Friend' button and a 'Dismiss' link", username2));
        } else {
            logFailExit(String.format("Failed: 'Recommend Friends' tile for user 2 %s does not have both an 'Add Friend' button and a 'Dismiss' link", username2));
        }

        // 5
        recFriendsTile.clickDismissLink();
        if (Waits.pollingWait(() -> !MacroDiscover.verifyUserRecommendedAsFriend(driver, user2))) { // Wait for tile to disappear after clicking 'Dimiss'
            logPass(String.format("Verified after clicking the 'Dismiss' link, user 2 %s is no longer shown as a recommended friend of user 1 %s", username2, username1));
        } else {
            logFailExit(String.format("Failed: User 2 %s is still shown as a recommended friend of user 1 %s", username2, username1));
        }

        softAssertAll();
    }
}