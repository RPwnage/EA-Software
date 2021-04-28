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
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.utils.EntitlementService;
import com.ea.vx.originclient.webdrivers.BrowserType;

import java.util.Arrays;

import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Test friend recommendation on counts and friend usernames (with customized
 * privacy setting for friend users 3, 4 and 5) for two users with three legal
 * friends. Called by the following parameterized scripts
 * OARecFriendIdentifyEveryone, OARecFriendIdentifyFriend,
 * OARecFriendIdentifyAnonymous,
 *
 * @author palui
 */
public class OARecFriendIdentify extends EAXVxTestTemplate {

    /**
     * Enum for parameterized test type
     */
    public enum TEST_TYPE {
        EVERYONE,
        FRIEND,
        ANONYMOUS
    }

    /**
     * Get the privacy settings (to be set) for the 5 users according to the
     * type of test:<br>
     * <p>
     * EVERYONE - set privacies to 'Everyone'<br>
     * FRIEND - set privacies to 'Everyone' for first 2 users, 'Friends' for
     * next two users, and 'Friends of Friends' for the last user<br>
     * ANONYMOUS - set privacies to 'Everyone' for first 4 users, and 'No One'
     * for the last user
     *
     * @param testType type of test
     * @return array of privacy settings for the users per test type
     */
    private static PrivacySetting[] getSettings(TEST_TYPE testType) {
        PrivacySetting[] settings = new PrivacySetting[5];
        settings[0] = settings[1] = PrivacySetting.EVERYONE;
        switch (testType) {
            case EVERYONE:
                settings[2] = settings[3] = settings[4] = PrivacySetting.EVERYONE;
                break;
            case FRIEND:
                settings[2] = settings[3] = PrivacySetting.FRIENDS;
                settings[4] = PrivacySetting.FRIENDS_OF_FRIENDS;
                break;
            case ANONYMOUS:
                settings[2] = settings[3] = PrivacySetting.EVERYONE;
                settings[4] = PrivacySetting.NO_ONE;
                break;
            default:
                throw new RuntimeException(String.format("Invalid test type %s for parameterized test", testType));
        }
        return settings;
    }

    /**
     * Test method called by parameterized scripts
     *
     * @param context  test context
     * @param testType test type: EVERYONE, FRIEND, or ANONYMOUS
     * @throws Exception
     */
    public void testRecFriendIdentify(ITestContext context, TEST_TYPE testType) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final boolean isClient = ContextHelper.isOriginClientTesing(context);

        final UserAccount user1 = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        final UserAccount user2 = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        String username1 = user1.getUsername();
        String username2 = user2.getUsername();

        final UserAccount friend3 = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        final UserAccount friend4 = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        final UserAccount friend5 = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        String friendName3 = friend3.getUsername();
        String friendName4 = friend4.getUsername();
        String friendName5 = friend5.getUsername();

        // grant entitlements to users in order for them to see 'People You May Know' section
        EntitlementService.grantEntitlement(user1, EntitlementId.BF4_STANDARD.getOfferId());
        EntitlementService.grantEntitlement(user2, EntitlementId.BF4_STANDARD.getOfferId());

        UserAccount users[] = {user1, user2};
        UserAccount friends[] = {friend3, friend4, friend5};
        UserAccount anonymousFriends[] = {friend3, friend4, RecFriendsTile.ANONYMOUS_USER};
        UserAccount allUsers[] = {user1, user2, friend3, friend4, friend5};

        String usernames[] = {username1, username2};
        String friendNames[] = {friendName3, friendName4, friendName5};
        // For clarity, use "<Anonymous>" instead of an empty string for logging
        String anonymousUserName = (RecFriendsTile.USER_CARD_ANONYMOUS.equals("")) ? "<Anonymous>" : RecFriendsTile.USER_CARD_ANONYMOUS;
        String anonymousFriendsNames[] = {friendName3, friendName4, anonymousUserName};

        PrivacySetting settings[] = getSettings(testType);

        final int EXPECTED_NUM_OF_FRIENDS_IN_COMMON = 3;

        String identifyFriendsString = (testType != TEST_TYPE.ANONYMOUS)
                ? "friend users 3-5" : "friend users 3-4 as well as an anonymous friend (with privacy 'No One')";

        logFlowPoint(String.format("Set privacies for newly-register users 1-2 and friend users 3-5 to %s respectively", Arrays.toString(settings))); // 1
        logFlowPoint("Add friend users 3-5 as friends to users 1 and 2. Wait 30 seconds and login to user 1"); // 2
        logFlowPoint("Navigate to Discover page for user 1. Verify user 2 is shown as a recommended friend"); // 3
        logFlowPoint(String.format("Verify user 2 'Recommended Friends' tile shows %s friends in legal with user 1", EXPECTED_NUM_OF_FRIENDS_IN_COMMON)); // 4
        logFlowPoint(String.format("Verify user 2 'Recommended Friends' tile shows %s are in legal with user 1", identifyFriendsString)); // 5
        logFlowPoint("Logout from user 1 and login to user 2"); //6
        logFlowPoint("Navigate to Discover page for user 2. Verify user 1 is shown as a recommended friend"); //7
        logFlowPoint(String.format("Verify user 1 'Recommended Friends' tile shows %s friends in legal with user 2", EXPECTED_NUM_OF_FRIENDS_IN_COMMON)); // 8
        logFlowPoint(String.format("Verify user 1 'Recommended Friends' tile shows %s as in legal with user 2", identifyFriendsString)); // 9

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
                if (i == allUsers.length - 1) {
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
            logPass(String.format("Verified privacies for newly-registered users 1-2 %s and friend users 3-5 %s have been set to %s, respectively",
                    Arrays.toString(usernames), Arrays.toString(friendNames), Arrays.toString(settings)));
        } else {
            logFailExit(String.format("Failed: Cannot set privacies for newly-registered users 1-2 %s or friend users 3-5 %s to %s, respectively",
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
            logPass(String.format("Verified user 2 %s is shown in the Discover page as a recommended friend of user 1 %s", username2, username1));
        } else {
            logFailExit(String.format("Failed: User 2 %s is not shown in the Discover page as a recommended friend of user 1 %s", username2, username1));
        }

        // 4
        RecFriendsTile recTile = new DiscoverPage(driver).getRecFriendsSection().getRecFriendsTile(user2);
        recTile.hoverOnTile(); // Not necessary for the test but improves on CRS screen capture
        int nFriendsInCommon = recTile.getFriendsInCommonCount();
        int nFriendsInCommonAvatars = recTile.getFriendsInCommonAvatarsCount();
        boolean countOK = (nFriendsInCommon == nFriendsInCommonAvatars) && (nFriendsInCommon == EXPECTED_NUM_OF_FRIENDS_IN_COMMON);
        if (countOK) {
            logPass(String.format("Verified user 2 %s's 'Recommended Friends' tile shows %s friends in legal with user 1 %s",
                    username2, EXPECTED_NUM_OF_FRIENDS_IN_COMMON, username1));
        } else {
            logFailExit(String.format("Failed: User 2 %s's 'Recommended Friends' tile shows %s count and %s avatars which do not match the expected %s friends in legal with user 1 %s",
                    username2, nFriendsInCommon, nFriendsInCommonAvatars, EXPECTED_NUM_OF_FRIENDS_IN_COMMON, username1));
        }

        // 5
        UserAccount[] matchUsers = (testType != TEST_TYPE.ANONYMOUS) ? friends : anonymousFriends;
        String[] matchNames = (testType != TEST_TYPE.ANONYMOUS) ? friendNames : anonymousFriendsNames;
        if (recTile.verifyFriendsInCommonExist(matchUsers)) {
            logPass(String.format("Verified user 2 %s's 'Recommended Friends' tile shows friend users 3-5 %s as in legal with user 1 %s",
                    username2, Arrays.toString(matchNames), username1));
        } else {
            logFailExit(String.format("Failed: User 2 %s's 'Recommended Friends' tile does not show friend users 3-5 %s as in legal with user 1 %s",
                    username2, Arrays.toString(matchNames), username1));
        }

        // 6
        new MiniProfile(driver).selectSignOut();
        if (MacroLogin.startLogin(driver, user2)) {
            logPass(String.format("Verified login successful as user 2 %s", username2));
        } else {
            logFailExit(String.format("Failed: Cannot login as user 2 %s", username2));
        }

        // 7
        if (MacroDiscover.verifyUserRecommendedAsFriend(driver, user1)) {
            logPass(String.format("Verified user 1 %s is shown in the Discover page as a recommended friend of user 2 %s", username1, username2));
        } else {
            logFailExit(String.format("Failed: User 1 %s is not shown in the Discover page as a recommended friend of user 2 %s", username1, username2));
        }

        // 8
        recTile = new DiscoverPage(driver).getRecFriendsSection().getRecFriendsTile(user1);
        recTile.hoverOnTile(); // Not necessary for the test but improves on CRS screen capture
        nFriendsInCommon = recTile.getFriendsInCommonCount();
        nFriendsInCommonAvatars = recTile.getFriendsInCommonAvatarsCount();
        countOK = (nFriendsInCommon == nFriendsInCommonAvatars) && (nFriendsInCommon == EXPECTED_NUM_OF_FRIENDS_IN_COMMON);
        if (countOK) {
            logPass(String.format("Verified user 1 %s's 'Recommended Friends' tile shows %s friends in legal with user 2 %s",
                    username1, EXPECTED_NUM_OF_FRIENDS_IN_COMMON, username2));
        } else {
            logFailExit(String.format("Failed: User 1 %s's 'Recommended Friends' tile shows %s count and %s avatars which do not match the expected %s friends in legal with user 2 %s",
                    username1, nFriendsInCommon, nFriendsInCommonAvatars, EXPECTED_NUM_OF_FRIENDS_IN_COMMON, username2));
        }

        // 9
        if (recTile.verifyFriendsInCommonExist(matchUsers)) {
            logPass(String.format("Verified user 1 %s's 'Recommended Friends' tile shows friend users 3-5 %s as in legal with user 2 %s",
                    username1, Arrays.toString(matchNames), username2));
        } else {
            logFailExit(String.format("Failed: User 1 %s's 'Recommended Friends' tile does not show friend users 3-5 %s as in legal with user 2 %s",
                    username1, Arrays.toString(matchNames), username2));
        }

        softAssertAll();
    }
}
