package com.ea.originx.automation.scripts.recommendation;

import com.ea.originx.automation.lib.pageobjects.account.PrivacySettingsPage;
import com.ea.originx.automation.lib.resources.games.EntitlementId;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroAccount;
import com.ea.originx.automation.lib.macroaction.MacroDiscover;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.account.PrivacySettingsPage.PrivacySetting;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.utils.EntitlementService;
import com.ea.vx.originclient.utils.Waits;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Test friend recommendations (with customized privacy setting for user 1)
 * between two users with a legal friend. Called bey the following
 * parameterized scripts OARecFriendPrivacyEveryone, OARecFriendPrivacyFriend,
 * OARecFriendPrivacyFriendFriend, OARecFriendPrivacyNoOne
 *
 * @author palui
 */
public class OARecFriendPrivacy extends EAXVxTestTemplate {

    public void testRecFriendPrivacy(ITestContext context, PrivacySetting setting) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final boolean isClient = ContextHelper.isOriginClientTesing(context);

        final UserAccount user1 = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        final UserAccount user2 = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        UserAccount commonFriend = AccountManager.getRandomAccount();

        String username1 = user1.getUsername();
        String username2 = user2.getUsername();

        // grant entitlements to users in order for them to see 'People You May Know' section
        EntitlementService.grantEntitlement(user1, EntitlementId.BF4_STANDARD.getOfferId());
        EntitlementService.grantEntitlement(user2, EntitlementId.BF4_STANDARD.getOfferId());

        boolean showUser1AsRecommended = setting.equals(PrivacySetting.EVERYONE) || setting.equals(PrivacySetting.FRIENDS_OF_FRIENDS);
        String user1RecommendedString = (showUser1AsRecommended ? "shown" : "not shown");
        String user1RecommendedFailString = (showUser1AsRecommended ? "not shown" : "shown");

        logFlowPoint(String.format("Set newly-registered user 1's privacy to '%s' (newly-registered user 2 is 'Everyone' by default)", setting.toString())); // 1
        logFlowPoint("Add a legal friend to both users. Wait 30 seconds and login to user 1"); // 2
        logFlowPoint("Navigate to Discover page for user 1. Verify user 2 is shown as a recommended friend"); // 3
        logFlowPoint("Logout from user 1 and login to user 2"); // 4
        logFlowPoint(String.format("Navigate to Discover page for user 2. Verify user 1 is %s as a recommended friend", user1RecommendedString)); // 5

        // 1
        final WebDriver driver = startClientObject(context, client);
        String originUrl = driver.getCurrentUrl();
        boolean setPrivacy = true;
        final WebDriver settingsDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        if (isClient) {
            if (!MacroAccount.setPrivacyThroughBrowserLogin(settingsDriver, user1, setting)) {
                setPrivacy = false;
            }
            settingsDriver.close();
        } else {
            if (!MacroAccount.setPrivacyThroughBrowserLogin(driver, user1, setting)) {
                setPrivacy = false;
            }
        }
        if (!isClient) {
            driver.get(originUrl); // if not client, go back to origin
        }
        if (setPrivacy) {
            logPass(String.format("Verified user 1 %s's privacy has been set to '%s'", username1, setting.toString()));
        } else {
            logFailExit(String.format("Failed: Cannot set user 1 %s's privacy to '%s'", username1, setting.toString()));
        }

        // 2
        user1.cleanFriends();
        user2.cleanFriends();
        commonFriend.cleanFriends();
        user1.addFriend(commonFriend);
        user2.addFriend(commonFriend);
        sleep(30000); // Pause to ensure friends recommendation is updated with the friends change. Per EADP this should happen within 30 seconds
        startClientObject(context, client);
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
        new MiniProfile(driver).selectSignOut();
        if (MacroLogin.startLogin(driver, user2)) {
            logPass(String.format("Verified login successful as user 2 %s", username2));
        } else {
            logFailExit(String.format("Failed: Cannot login as user 2 %s", username2));
        }

        // 5
        if (Waits.pollingWait(() -> MacroDiscover.verifyUserRecommendedAsFriend(driver, user1) == showUser1AsRecommended)) {
            logPass(String.format("Verified user 1 %s is %s as a recommended friend of user 2 %s", username1, user1RecommendedString, username2));
        } else {
            logFailExit(String.format("Failed: User 1 %s is %s as a recommended friend of user 2 %s", username1, user1RecommendedFailString, username2));
        }

        softAssertAll();
    }
}
