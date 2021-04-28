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
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.EntitlementService;
import com.ea.vx.originclient.webdrivers.BrowserType;
import java.util.Arrays;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests friend recommendations for users 1 and 2 with a legal friend user 3
 * (who blocks user 1).
 *
 * @author palui
 */
public class OARecFriendBlocked extends EAXVxTestTemplate {

    @TestRail(caseId = 159495)
    @Test(groups = {"recommendation", "client_only", "full_regression"})
    public void testRecFriendBlocked(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

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

        logFlowPoint(String.format("Register users 1-2 and friend user 3 and set privacies to %s respectively.", Arrays.toString(settings))); // 1
        logFlowPoint("Add friend user 3 to both users 1 and 2. Wait 30 seconds and login to user 1."); // 2
        logFlowPoint("Verify user 2 is recommended as a friend to user 1."); // 3
        logFlowPoint("Logout from user 1 and login to friend user 3."); // 4
        logFlowPoint("From the Social Hub, block user 1 from friend user 3."); // 5
        logFlowPoint("Logout from friend user 3. Wait 30 seconds and login to user 1."); // 6
        logFlowPoint("Verify user 2 is no longer recommended as a friend to user 1."); // 7
        logFlowPoint("Logout from user 1 and login to user 2."); // 8
        logFlowPoint("Verify user 1 is not shown as a recommended friend of user 2."); // 9

        // 1
        boolean setPrivacyOK = true;
        for (int i = 0; i < allUsers.length; i++) {
            WebDriver browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
            if (!MacroAccount.setPrivacyThroughBrowserLogin(browserDriver, allUsers[i], settings[i])) {
                setPrivacyOK = false;
                break;
            }
            client.stop(); // Stop client otherwise we may run out of ports
        }

        if (setPrivacyOK) {
            logPass(String.format("Verified users 1-2 %s and friend user 3 %s registered successfully with privacies set to %s respectively",
                    Arrays.toString(usernames), Arrays.toString(friendNames), Arrays.toString(settings)));
        } else {
            logFailExit(String.format("Failed: Cannot register users 1-2 %s or friend user 3 %s, or set their privacies to %s respectively",
                    Arrays.toString(usernames), Arrays.toString(friendNames), Arrays.toString(settings)));
        }

        // 2
        MacroSocial.cleanAndAddFriendship(users, friends);
        sleep(30000); // Pause to ensure friends recommendation is updated with the friends change. Per EADP this should happen within 30 seconds
        final WebDriver driver = startClientObject(context, client);
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
        if (MacroLogin.startLogin(driver, friend3)) {
            logPass(String.format("Verified login successful as friend 3 %s", friendName3));
        } else {
            logFailExit(String.format("Failed: Cannot login as friend 3 %s", friendName3));
        }

        // 5
        if (MacroSocial.unfriendAndBlock(driver, user1)) {
            logPass("Verified user 1 unfriended and blocked by friend 3");
        } else {
            logFailExit("Failed: User 1 unfriended and blocked by friend 3");
        }

        // 6
        new MiniProfile(driver).selectSignOut();
        sleep(30000); // Pause to ensure friends recommendation is updated with the friends change. Per EADP this should happen within 30 seconds
        if (MacroLogin.startLogin(driver, user1)) {
            logPass(String.format("Verified login successful as user 1 %s", username1));
        } else {
            logFailExit(String.format("Failed: Cannot login as user 1 %s", username1));
        }

        // 7
        if (!MacroDiscover.verifyUserRecommendedAsFriend(driver, user2)) {
            logPass(String.format("Verified user 2 %s is no longer shown as a recommended friend of user 1 %s", username2, username1));
        } else {
            logFailExit(String.format("Failed: User 2 %s is still shown as a recommended friend of user 1 %s", username2, username1));
        }

        // 8
        new MiniProfile(driver).selectSignOut();
        if (MacroLogin.startLogin(driver, user2)) {
            logPass(String.format("Verified login successful as user 2 %s", username2));
        } else {
            logFailExit(String.format("Failed: Cannot login as user 2 %s", username2));
        }

        // 9
        if (!MacroDiscover.verifyUserRecommendedAsFriend(driver, user1)) {
            logPass(String.format("Verified user 1 %s is not shown as a recommended friend of user 2 %s", username1, username2));
        } else {
            logFailExit(String.format("Failed: User 1 %s is shown as a recommended friend of user 2 %s", username1, username2));
        }

        softAssertAll();
    }
}