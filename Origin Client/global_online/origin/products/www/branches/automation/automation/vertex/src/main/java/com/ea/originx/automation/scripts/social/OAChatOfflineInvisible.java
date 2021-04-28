package com.ea.originx.automation.scripts.social;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.social.ChatWindow;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;

import static com.ea.vx.originclient.templates.OATestBase.sleep;

import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that sending chat messages works properly when the message sender
 * and/or recipient is invisible
 *
 * @author lscholte
 */
public class OAChatOfflineInvisible extends EAXVxTestTemplate {

    @TestRail(caseId = 12645)
    @Test(groups = {"social", "client_only", "full_regression"})
    public void testChatOfflineInvisible(ITestContext context) throws Exception {

        final OriginClient clientA = OriginClientFactory.create(context);
        final OriginClient clientB = OriginClientFactory.create(context);

        final String sendToInvisibleMessage = "Send to Invisible";
        final String sendWhileInvisibleMessage = "Send While Invisible";
        final String sendWhileOnlineMessage = "Send While Online";
        final String sendWhileOfflineMessage = "Send While Offline";

        UserAccount userA = AccountManager.getRandomAccount();
        UserAccount userB = AccountManager.getRandomAccount();

        userA.cleanFriends();
        userB.cleanFriends();

        userA.addFriend(userB);

        logFlowPoint("Log into Origin with user A"); //1
        logFlowPoint("Open a chat with user B and verify the message from an online user to an offline user was received."); //2
        logFlowPoint("Verify there is a message 'User B is currently offline, they will receive your message when they sign in.'"); //3
        logFlowPoint("Set status to invisible"); //4
        logFlowPoint("Open a chat with user B"); //5
        logFlowPoint("Log into Origin with user B"); //6
        logFlowPoint("Verify user A is visible and offline on the friends list"); //7
        logFlowPoint("Open a chat with user A"); //8
        logFlowPoint("Send a message to user A and verify the message was received"); //9
        logFlowPoint("Set user B's status to invisible"); //10
        logFlowPoint("Send another message to user A and verify it was received"); //11
        logFlowPoint("Set user B's status to online"); //12
        logFlowPoint("Send another message to user A and verify it was received"); //13
        logFlowPoint("Set user A's status to online, send a message to user B and verify it was received"); //14
        
        //1
        WebDriver driverA = startClientObject(context, clientA);
        if (MacroLogin.startLogin(driverA, userA)) {
            logPass("Successfully logged user B into Origin as " + userA.getUsername());
        } else {
            logFailExit("Failed to log user B into Origin as " + userA.getUsername());
        }

        //2
        SocialHub friendSocialHub = new SocialHub(driverA);
        MacroSocial.restoreAndExpandFriends(driverA);
        friendSocialHub.getFriend(userB.getUsername()).openChat();
        ChatWindow chatWithUserB = new ChatWindow(driverA);
        chatWithUserB.sendMessage(sendWhileOfflineMessage);
        if (chatWithUserB.verifyMessagesInBubble(new String[]{sendWhileOfflineMessage})) {
            logPass("Successfully opened a chat with user B and sent a message from an online user to an offline user.");
        } else {
            logFailExit("Failed to open chat with user B and send a message from an online user to an offline user.");
        }

        //3
        if (chatWithUserB.verifyOfflineMessage(userB.getUsername())) {
            logPass("Successfully verified there is a message 'User B is currently offline, they will receive your message when they sign in.'");

        } else {
            logFailExit("Failed to verify there is a message 'User B is currently offline, they will receive your message when they sign in.'");
        }

        //4
        friendSocialHub.setUserStatusInvisible();
        if (friendSocialHub.verifyUserInvisible()) {
            logPass("Successfully set user A's status to invisible");
        } else {
            logFailExit("Failed to set user A's status to invisible");
        }
        
        //5
        if (chatWithUserB.verifyChatOpen()) {
            logPass("Successfully opened a chat with user B");

        } else {
            logFailExit("Failed to open a chat with user B");
        }
        //6
        WebDriver driverB = startClientObject(context, clientB);
        if (MacroLogin.startLogin(driverB, userB)) {
            logPass("Successfully logged user B into Origin as " + userB.getUsername());

        } else {
            logFailExit("Failed to log user B into Origin as " + userA.getUsername());
        }
        //7
        MacroSocial.restoreAndExpandFriends(driverB);
        SocialHub userSocialHub = new SocialHub(driverB);
        boolean userAOffline = userSocialHub.verifyFriendVisible(userA.getUsername())
                && userSocialHub.getFriend(userA.getUsername()).verifyPresenceOffline();
        if (userAOffline) {
            logPass("User A is displayed as offline in the social hub");
        } else {
            logFailExit("User A is either not in the social hub or is not displayed as offline");
        }
        //8
        userSocialHub.getFriend(userA.getUsername()).openChat();
        ChatWindow chatWithUserA = new ChatWindow(driverB);
        if (chatWithUserA.verifyChatOpen()) {
            logPass("Successfully opened a chat with user A");
        } else {
            logFailExit("Failed to open a chat with user A");
        }
        //9
        chatWithUserA.sendMessage(sendToInvisibleMessage);
        sleep(2000); //Allow time for message to be sent
        chatWithUserB = new ChatWindow(driverA);
        if (chatWithUserB.verifyMessagesInBubble(new String[]{sendToInvisibleMessage})) {
            logPass("Successfully sent a message from an online user to an invisible user");
        } else {
            logFailExit("Failed to send a message from an online user to an invisible user");
        }
        //10
        userSocialHub.setUserStatusInvisible();
        if (userSocialHub.verifyUserInvisible()) {
            logPass("Successfully set uesr B's status to invisible");
        } else {
            logFailExit("Failed to set user B's status to invisible");
        }
        //11
        chatWithUserA.sendMessage(sendWhileInvisibleMessage);
        sleep(2000); //Allow time for message to be sent
        if (chatWithUserB.verifyMessagesInBubble(new String[]{sendWhileInvisibleMessage})) {
            logPass("Successfully sent a message from an invisible user to another invisible user");
        } else {
            logFailExit("Failed to send a message from an invisible user to another invisible user");
        }
        //12
        userSocialHub.setUserStatusOnline();
        if (userSocialHub.verifyUserOnline()) {
            logPass("Successfully set user B's status to online");
        } else {
            logFailExit("Failed to set user B's status to online");
        }
        //13
        chatWithUserA.sendMessage(sendWhileOnlineMessage);
        sleep(2000); //Allow time for message to be sent
        if (chatWithUserB.verifyMessagesInBubble(new String[]{sendWhileOnlineMessage})) {
            logPass("Successfully sent a message from an online user to an invisible user");
        } else {
            logFailExit("Failed to send a message from an online user to an invisible user");
        }

        //14
        friendSocialHub.setUserStatusOnline();
        chatWithUserB.sendMessage(sendWhileOnlineMessage);

        if (chatWithUserA.verifyMessagesInBubble(new String[]{sendWhileOnlineMessage})) {
            logPass("Successfully sent a message from an online user to a user who changed status from invisible to online.");
        } else {
            logFailExit("Failed to send a message from an online user to a user who changed status from invisible to online.");
        }
        softAssertAll();
    }
}
