package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.helpers.StringHelper;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.social.ChatWindow;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the 'Social Hub Friends List' in 'Offline Mode'.
 *
 * @author palui
 */
public class OAOfflineModeFriends extends EAXVxTestTemplate {
    
    @TestRail(caseId = 39459)
    @Test(groups = {"social", "client_only", "full_regression"})
    public void testOfflineModeFriends(ITestContext context) throws Exception {
        
        final OriginClient clientUser = OriginClientFactory.create(context);
        final OriginClient clientFriend = OriginClientFactory.create(context);
        
        UserAccount userAccount = AccountManager.getRandomAccount();
        final String username = userAccount.getUsername();
        UserAccount friendAccount = AccountManager.getRandomAccount();
        final String friendUsername = friendAccount.getUsername();
        
        userAccount.cleanFriends();
        friendAccount.cleanFriends();
        userAccount.addFriend(friendAccount);
        
        logFlowPoint("Launch and log into Origin with a user that has a friend."); // 1
        logFlowPoint("Open the 'Friends List' and verify the user's presence."); // 2
        logFlowPoint("Click 'Origin' menu, select 'Go Offline', and verify that Origin goes into 'Offline Mode'."); // 3
        logFlowPoint("Verify user can't change presence while offline."); // 4
        logFlowPoint("Verify status reads 'Offline' with a grey circle to the left of the status."); // 5
        logFlowPoint("Verify the chat is disabled."); // 6
        logFlowPoint("Verify that the 'Friends List' displays a 'Cannot Load Friends in Offline Mode' message."); // 7
        logFlowPoint("Verify that the user is not displayed online on other account."); // 8
        logFlowPoint("Click 'Go Online' and verify that the 'Friends List' refreshes once the client enters 'Online Mode'."); // 9
        
        //1
        WebDriver driverUserA = startClientObject(context, clientUser);
        if (MacroLogin.startLogin(driverUserA, userAccount)) {
            logPass("Successfully logged into Origin with user: " + username);
        } else {
            logFailExit("Failed to log into Origin with user: " + username);
        }
        
        //2
        SocialHub socialHub = new SocialHub(driverUserA);
        MacroSocial.restoreAndExpandFriends(driverUserA);
        if(socialHub.verifySocialHubVisible()){
            logPass("Verified the user's 'Friends List' presence.");
        } else {
            logFailExit("Failed to verify the user's 'Friends List' presence.");
        }

        socialHub.getFriend(friendUsername).openChat(); // needed for step 6 to avoid performing extra steps there
        
        //3
        MainMenu mainMenu = new MainMenu(driverUserA);
        mainMenu.selectGoOffline();
        sleep(1000); // wait for Origin menu to update
        if (mainMenu.verifyOfflineModeButtonText()) {
            logPass("On clicking 'Go Offline', verified Origin is now in 'Offline Mode'.");
        } else {
            logFailExit("On clicking 'Go Offline', failed to verify offline status.");
        }
        
        //4
        if(!socialHub.verifyUserPresenceDropDownVisible()){
            logPass("Verified user is not able to change presence while in 'Offline Mode'.");
        } else {
            logFailExit("Failed : User is still able to change presence.");
        }
        
        //5
        boolean statusText = StringHelper.containsIgnoreCase(socialHub.getSocialHubPresenceStatus(), "Offline");
        boolean statusDotColor = socialHub.verifyOfflineDotStatusColor();
        if(statusDotColor && statusText){
            logPass("Verified status text reads 'Offline' and the color of status dot is gray.");
        } else {
            logFailExit("Failed to verify status text and/or dot status color.");
        }
        
        //6
        if(new ChatWindow(driverUserA).verifyOfflineChatDisabled()){
            logPass("Verified chat is disabled while in 'Offline Mode'.");
        } else {
            logFailExit("Failed to verify if the chat is disabled.");
        }
        
        //7
        if(socialHub.verifyOfflineMessage()){
            logPass("Verified offline message in 'Friends List'.");
        } else {
            logFailExit("Failed to verify offline message.");
        }
        
        //8
        WebDriver driverUserB = startClientObject(context, clientFriend);
        MacroLogin.startLogin(driverUserB, friendAccount);
        MacroSocial.restoreAndExpandFriends(driverUserB);
        if(new SocialHub(driverUserB).verifyFriendOffline(username)){
            logPass("Verified that user " + username + " is offline is his friends 'Friend List'.");
        } else {
            logFailExit("Failed to verify status of user in friend's 'Friend List'.");
        }
        
        //9
        mainMenu.selectGoOnline();
        MacroSocial.restoreAndExpandFriends(driverUserA);
        boolean presence = socialHub.verifyUserOnline();
        boolean friendsVisible = socialHub.getAllFriends().size() > 0;
        if(presence && friendsVisible){
            logPass("Verified 'Friends List' is now in 'Online Mode'.");
        } else {
            logFailExit("Failed to verify if 'Friends List' is in 'Online Mode'.");
        }
        
        softAssertAll();
    }
}