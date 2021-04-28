package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.dialog.SearchForFriendsDialog;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * A test that verify an account with no friends display the message 
 * 'Build your friends list' in the friends list, can sent a friend request, 
 * and the 'Build your friends list' message disappears.
 * 
 * As part of this verification, this test also checks that an account with
 * friends has the 'Find Friends' button displayed inside friends list
 * 
 * @author cdeaconu
 */
public class OANoFriendsAdd extends EAXVxTestTemplate {    
    
    @TestRail(caseId = 13203)
    @Test(groups = {"client_only", "social", "full_regression"})
    public void testNoFriendsAdd(ITestContext context) throws Exception {
        
        final OriginClient client1 = OriginClientFactory.create(context);
        final OriginClient client2 = OriginClientFactory.create(context);
        
        UserAccount userAccount = AccountManager.getRandomAccount();
        UserAccount friendAccount = AccountManager.getRandomAccount();
        
        String accountUsername = userAccount.getUsername();
        
        userAccount.cleanFriends();

        logFlowPoint("Login to Origin with user A that has no friends."); // 1
        logFlowPoint("Open the friends list and verify that the 'Build your friends list' message is displayed."); // 2
        logFlowPoint("Verify that there is a 'Find Friends' button below the message."); // 3
        logFlowPoint("Verify that clicking the 'Find Friends' button opens the 'Search for Friends' dialog."); // 4
        logFlowPoint("Search for user B, navigate to his profile and send a friend request"); // 5
        logFlowPoint("Log into Origin with User B in another client."); // 6
        logFlowPoint("Accept the friend request."); // 7
        logFlowPoint("Verify that the 'Build your friends list' message disappears when the user has a friend."); // 8
        logFlowPoint("Verify that there is a 'Find Friends' button."); // 9
        logFlowPoint("Verify that clicking the 'Find Friends' button opens the 'Search for Friends' dialog."); // 10
        
        // 1
        WebDriver driver1 = startClientObject(context, client1);
        logPassFail(MacroLogin.startLogin(driver1, userAccount), true);
        
        // 2
        MacroSocial.restoreAndExpandFriends(driver1);
        SocialHub socialHub = new SocialHub(driver1);
        logPassFail(socialHub.verifyNoFriendsMessage(), false);

        // 3
        logPassFail(socialHub.verifyFindFriendsButtonDisplayed(), true);

        // 4
        socialHub.clickFindFriendsButton();
        SearchForFriendsDialog searchFriendsDialog = new SearchForFriendsDialog(driver1);
        logPassFail(searchFriendsDialog.waitIsVisible(), true);
        
        // 5
        logPassFail(MacroSocial.searchForUserSendFriendRequestViaProfile(driver1, friendAccount), true);

        // 6
        WebDriver driver2 = startClientObject(context, client2);
        logPassFail(MacroLogin.startLogin(driver2, friendAccount), true);

        // 7
        MacroSocial.restoreAndExpandFriends(driver2);
        SocialHub socialHubFriend = new SocialHub(driver2);
        socialHubFriend.acceptIncomingFriendRequest(accountUsername);
        logPassFail(Waits.pollingWait(() -> socialHubFriend.verifyFriendVisible(accountUsername)), true);

        // 8
        logPassFail(!socialHubFriend.verifyNoFriendsMessage(), false);
        
        // 9
        logPassFail(socialHubFriend.verifyAddContactsButtonVisible(), true);
        
        // 10
        socialHubFriend.clickAddContactsButton();
        SearchForFriendsDialog searchFriendsPanel = new SearchForFriendsDialog(driver2);
        logPassFail(searchFriendsPanel.waitIsVisible(), true);
    }
}
