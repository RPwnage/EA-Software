package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.macroaction.MacroAccount;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroProfile;
import com.ea.originx.automation.lib.pageobjects.account.PrivacySettingsPage;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.dialog.UnfriendUserDialog;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileFriendsTab;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Checks that the no friends message appears after unfriending only friend
 *
 * @author mirani
 */
public class OANoFriendsOtherProfile extends EAXVxTestTemplate {

    @TestRail(caseId = 13066)
    @Test(groups = {"social", "full_regression"})
    public void testNoFriendsBrowserProfile(ITestContext context) throws Exception {

        final OriginClient user = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getRandomAccount();
        UserAccount friendAccount = AccountManager.getRandomAccount();
        final String friendUsername = friendAccount.getUsername();

        userAccount.cleanFriends();
        friendAccount.cleanFriends();
        userAccount.addFriend(friendAccount);

        logFlowPoint("Launch a browser and navigate to Account Settings to change privacy to everyone, and then log in with a user with no friends"); // 1
        logFlowPoint("Navigate to the user's friend's profile"); // 2
        logFlowPoint("Open dropdown in top right of friend's profile "); // 3
        logFlowPoint("Verify unfriend dialog appears"); // 4
        logFlowPoint("Verify send friend request button appears after unfriending"); // 5
        logFlowPoint("Verify Friends tab is active when selected"); // 6
        logFlowPoint("Verify there is a message stating 'Gaming is better with friends. Add this person to your friends list and invite them to a game.'"); // 7

        // 1
        //Disabling privacy if enabled on friend account
        WebDriver browserDriver = user.getAnotherBrowserWebDriver(BrowserType.CHROME);
        MacroAccount.setPrivacyThroughBrowserLogin(browserDriver, friendAccount, PrivacySettingsPage.PrivacySetting.EVERYONE);
        browserDriver.close();

        WebDriver driver = startClientObject(context, user);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        logPassFail(MacroProfile.navigateToFriendProfile(driver, friendUsername), true);

        // 3
        ProfileHeader profileHeader = new ProfileHeader(driver);
        profileHeader.openDropdownMenu();
        logPassFail(profileHeader.isDropdownMenuOpened(), true);

        // 4

        profileHeader.selectUnfriendFromDropDownMenu();
        UnfriendUserDialog uuDialog = new UnfriendUserDialog(driver);
        logPassFail(uuDialog.waitIsVisible(), true);

        // 5
        uuDialog.clickUnfriend();
        logPassFail(profileHeader.verifySendFriendRequestButtonVisible(), true);

        // 6
        profileHeader.openFriendsTab();
        logPassFail(profileHeader.verifyFriendsTabActive(), true);

        // 7
        ProfileFriendsTab friendsTab = new ProfileFriendsTab(driver);
        logPassFail(friendsTab.verifyNoFriendsMessage(), true);

        softAssertAll();
    }
}
