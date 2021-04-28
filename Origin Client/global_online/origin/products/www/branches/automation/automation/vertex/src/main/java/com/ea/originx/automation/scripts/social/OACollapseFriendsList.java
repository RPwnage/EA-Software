package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.lib.pageobjects.social.SocialHubMinimized;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.vx.originclient.utils.Waits;

import java.util.Arrays;

import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test if outgoing and incoming friends, as well as friends are visible in the
 * list and after collapsing they are no longer visible
 *
 * @author mdobre
 */
public class OACollapseFriendsList extends EAXVxTestTemplate {

    @TestRail(caseId = 13262)
    @Test(groups = {"client_only", "full_regression", "social"})
    public void testCollapseFriendsList(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getRandomAccount();
        UserAccount outgoingFriend = AccountManager.getRandomAccount();
        UserAccount incomingFriend = AccountManager.getRandomAccount();
        UserAccount friend = AccountManager.getRandomAccount();
        UserAccount[] friends = {outgoingFriend, incomingFriend, friend};
        userAccount.cleanFriends();
        Arrays.stream(friends).forEach(UserAccount::cleanFriends);

        UserAccountHelper.inviteFriend(userAccount, outgoingFriend);
        UserAccountHelper.inviteFriend(incomingFriend, userAccount);
        UserAccountHelper.addFriends(friend, userAccount);

        logFlowPoint("Log into Origin with an account which has an outgoing friend request, an incoming friend request and a friend"); //1
        logFlowPoint("Open the friends list"); //2
        logFlowPoint("Verify the user tile for the outgoing friend invite is visible on the friends list"); //3
        logFlowPoint("Collapse the outgoing friends list and verify the outgoing friend tile is no longer visible"); //4
        logFlowPoint("Verify the user tile for the incoming friend invite is visible on the friends list"); //5
        logFlowPoint("Collapse the incoming friends list and verify the incoming friend tile is no longer visible"); //6
        logFlowPoint("Verify the user tile for the friend is visible on the friends list"); //7
        logFlowPoint("Collapse the friends list and verify the friend tile is no longer visible"); //8
        logFlowPoint("Log out of Origin"); //9
        logFlowPoint("Log into Origin"); //10
        logFlowPoint("Open the friends list and verify all the sections are collapsed"); //11
        logFlowPoint("Expand the friends list and verify the friend tile is visible"); //12

        // 1
        WebDriver driver = startClientObject(context, client);

        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        // 2  
        MacroSocial.restoreAndExpandFriends(driver);
        SocialHub socialHub = new SocialHub(driver);
        if (socialHub.verifySocialHubVisible()) {
            logPass("Successfully opened friends list");
        } else {
            logFailExit("Could not open friends list");
        }

        //3 
        socialHub.clickIncomingHeader();
        socialHub.clickFriendsHeader();
        if (socialHub.verifyFriendRequestSent(outgoingFriend.getUsername())) {
            logPass("Successfully verified the outgoing friend's tile in the friends list");
        } else {
            logFailExit("The outgoing friend's tile is not visible");
        }

        //4
        // collapse outgoing
        socialHub.clickOutgoingHeader();
        if (!socialHub.verifyFriendVisible(outgoingFriend.getUsername())) {
            logPass("Successfully verified the outgoing friend's tile in not visible when collapsing");
        } else {
            logFailExit("The outgoing friend's tile is visible when collapsing");
        }

        //5
        socialHub.clickIncomingHeader();
        if (socialHub.verifyFriendRequestReceived(incomingFriend.getUsername())) {
            logPass("Successfully verified user tile for the incoming friend invite is visible on the friends list");
        } else {
            logFailExit("The user tile for the incoming friend invite is not visible on the friends list");
        }

        //6
        // collapse incoming
        socialHub.clickIncomingHeader();
        if (!socialHub.verifyFriendRequestReceived(incomingFriend.getUsername())) {
            logPass("Successfully verified user tile for the incoming friend invite is not visible on the friends list");
        } else {
            logFailExit("The user tile for the incoming friend invite is visible on the friends list");
        }

        //7
        socialHub.clickFriendsHeader();
        if (socialHub.verifyFriendVisible(friend.getUsername())) {
            logPass("The friend is visible on the friends list");
        } else {
            logFailExit("The friend is not visible on the friends list");
        }

        //8
        socialHub.clickFriendsHeader();
        if (!socialHub.verifyFriendVisible(friend.getUsername())) {
            logPass("The friend is not visible on the friends list");
        } else {
            logFailExit("The friend is visible on the friends list");
        }

        //9
        LoginPage loginPage = new MainMenu(driver).selectLogOut();
        boolean isLoginPage = Waits.pollingWait(() -> loginPage.getCurrentURL().matches(OriginClientData.LOGIN_PAGE_URL_REGEX));
        if (isLoginPage) {
            logPass("Verified logout of Origin successful");
        } else {
            logFailExit("Failed: Cannot logout of Origin");
        }

        //10
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified re-login successful as user: " + userAccount.getUsername());
        } else {
            logFailExit("Failed: Cannot re-login as user: " + userAccount.getUsername());
        }

        //11
        SocialHubMinimized socialHubMinimized = new SocialHubMinimized(driver);
        socialHubMinimized.restoreSocialHub();
        boolean isIncomingCollapsed = socialHub.isIncomingCollapsed();
        boolean isOutgoingCollapsed = socialHub.isOutgoingCollapsed();
        boolean isFriendsCollapsed = socialHub.isFriendsCollapsed();
        if (isIncomingCollapsed && isOutgoingCollapsed && isFriendsCollapsed) {
            logPass("All the sections are collapsed");
        } else {
            logFailExit("Failed : the sections are not collapsed");
        }

        //12
        socialHub.clickFriendsHeader();
        if (socialHub.verifyFriendVisible(friend.getUsername())) {
            logPass("The friend is visible on the friends list");
        } else {
            logFailExit("The friend is not visible on the friends list");
        }

        softAssertAll();
    }

}
