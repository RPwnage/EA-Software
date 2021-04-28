package com.ea.originx.automation.scripts.social;

import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroAccount;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroProfile;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.PrivacySettingsPage;
import com.ea.originx.automation.lib.pageobjects.dialog.IgnoreFriendRequestAndBlockDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.SearchForFriendsDialog;
import com.ea.originx.automation.lib.pageobjects.dialog.UnfriendAndBlockDialog;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.social.Friend;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to verify the blocking and unblocking functions.
 *
 * @author cvanichsarn
 * @author rchoi
 * @author sbentley
 * @author cdeaconu
 */
public class OABlockUnblock extends EAXVxTestTemplate {

    @TestRail(caseId = 1016750)
    @Test(groups = {"social", "release_smoke", "client_only"})
    public void testBlockUnblock(ITestContext context) throws Exception {

        OriginClient clientA = OriginClientFactory.create(context);
        final OriginClient clientB = OriginClientFactory.create(context);
        final UserAccount userA = AccountManager.getInstance().createNewThrowAwayAccount();
        final UserAccount userB = AccountManager.getInstance().createNewThrowAwayAccount();
        final UserAccount userC = AccountManager.getInstance().createNewThrowAwayAccount();
        final String usernameA = userA.getUsername();
        final String usernameB = userB.getUsername();

        logFlowPoint("Log into Origin with User A"); // 1
        logFlowPoint("Log into Origin with User B"); // 2
        logFlowPoint("Make User A and User B friends"); // 3
        logFlowPoint("From User A send a friend request to User C"); // 4
        logFlowPoint("From User A go to the profile of the friend id you want to block"); // 5
        logFlowPoint("Click on drop down selector beside name"); // 6
        logFlowPoint("Select 'Unfriend and Block'"); // 7
        logFlowPoint("In the overlay window again click on the button that says 'Unfriend and Block'"); // 8
        logFlowPoint("Navigate to Account and Billing > Origin Blocked Users list and verify selected user is added to block list"); // 9
        logFlowPoint("Log into Origin with incoming friend request"); // 10
        logFlowPoint("Locate friend request in chat list"); // 11
        logFlowPoint("Click on drop down selector beside name"); // 12
        logFlowPoint("Select option that says 'Ignore friend request and Block'"); // 13
        logFlowPoint("In the dialog that appears, confirm 'Ignore friend request and Block'"); // 14
        logFlowPoint("Navigate to Account and Billing > Origin Blocked Users list and verify selected user is added to block list"); // 15

        // 1
        WebDriver driverA = startClientObject(context, clientA);
        logPassFail(MacroLogin.startLogin(driverA, userA), true);

        // 2
        WebDriver driverB = startClientObject(context, clientB);
        logPassFail(MacroLogin.startLogin(driverB, userB), true);

        // 3
        boolean isFriendAdded = MacroSocial.addFriendThroughUI(driverA, driverB, userA, userB);
        new MiniProfile(driverB).selectSignOut();
        Waits.sleep(5000); // wait for sign out
        logPassFail(isFriendAdded && MacroLogin.startLogin(driverB, userC), true);
        new MiniProfile(driverB).selectSignOut();
        
        // 4
        SocialHub socialHub = new SocialHub(driverA);
        socialHub.clickAddContactsButton();
        new SearchForFriendsDialog(driverA).waitIsVisible();
        logPassFail(MacroSocial.searchForUserSendFriendRequestViaProfile(driverA, userC), true);
        
        // 5
        logPassFail(MacroProfile.navigateToFriendProfile(driverA, usernameB), true);

        // 6
        ProfileHeader profileHeader = new ProfileHeader(driverA);
        profileHeader.openDropdownMenu();
        logPassFail(profileHeader.isDropdownMenuOpened(), true);

        // 7
        profileHeader.selectUnfriendAndBlockFromDropDownMenu();
        UnfriendAndBlockDialog unfriendAndBlockDialog = new UnfriendAndBlockDialog(driverA);
        logPassFail(unfriendAndBlockDialog.waitIsVisible(), true);

        // 8
        unfriendAndBlockDialog.clickUnfriendAndBlock();
        logPassFail(unfriendAndBlockDialog.waitIsClose(), true);

        // 9
        WebDriver browserDriver = clientA.getAnotherBrowserWebDriver(BrowserType.CHROME);
        MacroAccount.loginToAccountPage(browserDriver, userA);
        AccountSettingsPage accountSettingsPage = new AccountSettingsPage(browserDriver);
        accountSettingsPage.gotoPrivacySettingsSection();
        PrivacySettingsPage privacySettingsPage = new PrivacySettingsPage(browserDriver);
        logPassFail(privacySettingsPage.verifyUserOnBlockList(usernameB), true);

        // 10
        new MiniProfile(driverA).selectSignOut();
        Waits.sleep(5000); // wait for sign out
        logPassFail(MacroLogin.startLogin(driverA, userC), true);

        // 11
        MacroSocial.restoreAndExpandFriends(driverA);
        logPassFail(socialHub.verifyFriendRequestReceived(usernameA), true);

        // 12
        Friend friendRequest = socialHub.getFriendRequestReceived(usernameA);
        friendRequest.openContextMenu();
        logPassFail(friendRequest.verifyContextMenuOpen(), true);

        // 13
        friendRequest.ignoreFriendRequestAndBlock();
        IgnoreFriendRequestAndBlockDialog ignoreFriendRequestAndBlock = new IgnoreFriendRequestAndBlockDialog(driverA);
        logPassFail(ignoreFriendRequestAndBlock.waitIsVisible(), true);

        // 14
        ignoreFriendRequestAndBlock.clickIgnoreFriendRequestAndBlock();
        logPassFail(ignoreFriendRequestAndBlock.waitIsClose(), true);

        // 15
        MacroAccount.logoutOfMyAccount(browserDriver);
        AccountSettingsPage accountSettingsPageB = new AccountSettingsPage(browserDriver);
        MacroAccount.loginToAccountPage(browserDriver, userC);
        accountSettingsPageB.gotoPrivacySettingsSection();
        privacySettingsPage = new PrivacySettingsPage(browserDriver);
        logPassFail(privacySettingsPage.verifyUserOnBlockList(usernameA), true);

        softAssertAll();
    }
}