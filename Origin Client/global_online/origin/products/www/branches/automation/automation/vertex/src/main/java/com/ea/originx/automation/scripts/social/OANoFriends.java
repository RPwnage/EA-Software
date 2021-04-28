package com.ea.originx.automation.scripts.social;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.common.GlobalSearchResults;
import com.ea.originx.automation.lib.pageobjects.dialog.SearchForFriendsDialog;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * A test to verify that the 'No Friends' message is displayed correctly in the
 * social hub. As part of this verification, this test also checks that a friend
 * request can be sent, accepted, and that the friend can then be removed.
 *
 * @author lscholte
 * @author tdhillon
 */
public class OANoFriends extends EAXVxTestTemplate {

    @TestRail(caseId = 1016702)
    @Test(groups = {"client_only", "social", "release_smoke"})
    public void testNoFriends(ITestContext context) throws Exception {

        final OriginClient client1 = OriginClientFactory.create(context);
        final OriginClient client2 = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final UserAccount friendAccount = AccountManager.getInstance().createNewThrowAwayAccount();

        logFlowPoint("Login to Origin with User A and User B in different Origin Clients"); // 1
        logFlowPoint("Verify that the 'No Friends' message is displayed in the 'Social Hub'."); // 2
        logFlowPoint("Verify that there is a 'Find Friends' button below the message."); // 3
        logFlowPoint("Verify that clicking the button opens the 'Search for Friends' dialog."); // 4
        logFlowPoint("Search for User B."); // 5
        logFlowPoint("Navigate to the profile page of User B."); // 6
        logFlowPoint("Send a friend request to User B."); // 7
        logFlowPoint("Accept the friend request."); // 8
        logFlowPoint("Verify that User A was added as a friend to User B."); // 9
        logFlowPoint("Verify that the 'No Friends' message disappears when the user has a friend."); // 10
        logFlowPoint("Unfriend User B from User A's account."); // 11
        logFlowPoint("Verify that the 'No Friends' message appears again."); // 12

        // 1
        WebDriver driver1 = startClientObject(context, client1);
        boolean userAIsLoggedIn = MacroLogin.startLogin(driver1, userAccount);
        WebDriver driver2 = startClientObject(context, client2);
        boolean userBIsLoggedIn = MacroLogin.startLogin(driver2, friendAccount);
        logPassFail(userAIsLoggedIn && userBIsLoggedIn, true);

        // 2
        MacroSocial.restoreAndExpandFriends(driver1);
        SocialHub socialHub = new SocialHub(driver1);
        logPassFail(socialHub.verifyNoFriendsMessage(), true);

        // 3
        logPassFail(socialHub.verifyFindFriendsButtonDisplayed(), true);

        // 4
        socialHub.clickFindFriendsButton();
        SearchForFriendsDialog searchFriendsDialog = new SearchForFriendsDialog(driver1);
        logPassFail(searchFriendsDialog.waitIsVisible(), true);

        // 5
        searchFriendsDialog.enterSearchText(friendAccount.getEmail());
        searchFriendsDialog.clickSearch();
        GlobalSearchResults searchResults = new GlobalSearchResults(driver1);
        logPassFail(searchResults.verifyUserFound(friendAccount), true);

        // 6
        searchResults.viewProfileOfUser(friendAccount);
        ProfileHeader friendProfile = new ProfileHeader(driver1);
        logPassFail(friendProfile.verifyUsername(friendAccount.getUsername()), true);

        // 7
        friendProfile.clickSendFriendRequest();
        logPassFail(Waits.pollingWait(() -> friendProfile.verifyFriendRequestSentVisible()), true);

        // 9
        MacroSocial.restoreAndExpandFriends(driver2);
        SocialHub socialHubFriend = new SocialHub(driver2);
        socialHubFriend.acceptIncomingFriendRequest(userAccount.getUsername());
        sleep(1000); // Wait for friend to be visible in the friend's social hub after accepting the friend request
        logPassFail(socialHubFriend.verifyFriendVisible(userAccount.getUsername()), true);

        // 10
        sleep(1000); // Wait for the friend to be visible after accepting the friend request
        logPassFail(socialHub.verifyFriendVisible(friendAccount.getUsername()), true);

        // 11
        logPassFail(!socialHub.verifyNoFriendsMessage(), true);

        // 12
        socialHub.getFriend(friendAccount.getUsername()).unfriend(driver1);
        logPassFail(!socialHub.verifyFriendVisible(friendAccount.getUsername()), true);

        // 13
        logPassFail(socialHub.verifyNoFriendsMessage(), true);

        softAssertAll();
    }
}