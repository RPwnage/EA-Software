package com.ea.originx.automation.scripts.social;

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
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the functionality (or lack thereof) while the user is in offline mode
 *
 * @author JMittertreiner
 */
public class OAChatOffline extends EAXVxTestTemplate {

    @TestRail(caseId = 12644)
    @Test(groups = {"social", "client_only", "full_regression"})
    public void testChatOffline(ITestContext context) throws Exception {

        final OriginClient clientUser = OriginClientFactory.create(context);
        final OriginClient clientFriend = OriginClientFactory.create(context);

        // Setup Friends
        UserAccount userAccount = AccountManager.getRandomAccount();
        UserAccount friendAccount = AccountManager.getRandomAccount();
        userAccount.cleanFriends();
        friendAccount.cleanFriends();
        userAccount.addFriend(friendAccount);

        final String message1 = "Online: User to Friend";
        final String message2 = "Online: Friend to User";
        final String message3 = "Offline: Friend to User";
        final String message4 = "Back Online: User to Friend";
        final String message5 = "Back Online: Friend to User";
        final String[] messageGroup1 = {message1};
        final String[] messageGroup2 = {message2};
        final String[] messageGroup3 = {message3};
        final String[] messageGroup4 = {message4};
        final String[] messageGroup5 = {message5};

        logFlowPoint("Log into Origin with " + userAccount.getUsername()); // 1
        logFlowPoint("Log into Origin with " + friendAccount.getUsername()); // 2
        logFlowPoint("Open a chat window from the friend to the user and send a message"); // 3
        logFlowPoint("Send a message from the user to the friend"); // 4
        logFlowPoint("Go offline with the User Account"); // 5
        logFlowPoint("Verify: Both the sent and received messages remain in the chat window"); // 6
        logFlowPoint("Verify: There is a 'You are offline' system message in the chat window"); // 7
        logFlowPoint("Verify: You cannot send a message from the user when you are offline"); // 8
        logFlowPoint("Verify: There is no 'send a message' test in the text input field"); // 9
        logFlowPoint("Verify: The presence circle in the chat window is grey when offline"); // 10
        logFlowPoint("Send a message from the friend to the user while the user is offline"); // 11
        logFlowPoint("Go back online as user account"); // 12
        logFlowPoint("Verify: The message sent by the friend account appear when the user is back online"); // 13
        logFlowPoint("Verify: The message received while offline are timestamped"); // 14
        logFlowPoint("Verify: The presence circle in the chat windo is green when online"); // 15
        logFlowPoint("Send a message from the user to the friend"); // 16
        logFlowPoint("Send a message from the friend to the user"); // 17
        logFlowPoint("Verify: Both message were received without issue"); // 18

        // 1
        WebDriver driverUser = startClientObject(context, clientUser);
        if (MacroLogin.startLogin(driverUser, userAccount)) {
            logPass("Successfully logged into Origin as user");
        } else {
            logFailExit("Could not log into Origin as user");
        }

        // 2
        WebDriver driverFriend = startClientObject(context, clientFriend);
        if (MacroLogin.startLogin(driverFriend, friendAccount)) {
            logPass("Successfully logged into Origin with the friend");
        } else {
            logFailExit("Could not log into Origin with the friend");
        }

        SocialHub socialHubUser = new SocialHub(driverUser);
        MacroSocial.restoreAndExpandFriends(driverUser);
        ChatWindow chatWindowUser = new ChatWindow(driverUser);
        SocialHub socialHubFriend = new SocialHub(driverFriend);
        MacroSocial.restoreAndExpandFriends(driverFriend);
        ChatWindow chatWindowFriend = new ChatWindow(driverFriend);
        // Call verify just for the side effect of waiting
        // for the chat social window to be ready
        socialHubFriend.verifyFriendVisible(friendAccount.getUsername());

        // 3
        socialHubFriend.getFriend(userAccount.getUsername()).openChat();
        chatWindowFriend.sendMessage(message2);
        if (Waits.pollingWait(() -> chatWindowUser.verifyMessagesInBubble(messageGroup2))) {
            logPass("Successfully sent a message from the user to friend");
        } else {
            logFail("Failed to send a message from the user to friend");
        }

        // 4
        socialHubUser.getFriend(friendAccount.getUsername()).openChat();
        chatWindowUser.sendMessage(message1);
        if (Waits.pollingWait(() -> chatWindowFriend.verifyMessagesInBubble(messageGroup1))) {
            logPass("Successfully sent a message from the friend to user");
        } else {
            logFail("Failed to send a message from the friend to user");
        }

        // 5
        new MainMenu(driverUser).selectGoOffline();
        if (socialHubUser.verifyUserOffline()) {
            logPass("Successfully went offline");
        } else {
            logFail("Failed to go offline");
        }

        // 6
        final boolean firstMessageOK = chatWindowUser.verifyMessagesInBubble(messageGroup2);
        final boolean secondMessageOK = chatWindowFriend.verifyMessagesInBubble(messageGroup1);
        if (firstMessageOK && secondMessageOK) {
            logPass("Both the sent and received messages remain in the chat window");
        } else {
            logFail("The first and second messages do not remain in the chat window");
        }

        // 7
        if (chatWindowUser.verifyOfflineSystemMessage()) {
            logPass("There is a 'You are offline' message in the chat");
        } else {
            logFail("There is not a 'You are offline' message in the chat");
        }

        // 8
        if (chatWindowUser.verifyOfflineChatDisabled()) {
            logPass("You cannot send a message from the user when you are offline");
        } else {
            logFail("The chat box for the user is not disabled when you are offline");
        }

        // 9
        if (chatWindowUser.verifyChatPlaceholderIs("")) {
            logPass("There is no 'Send a message' text in the text input field");
        } else {
            logFail("The 'Send a message' text is in the text input field");
        }

        // 10
        if (chatWindowUser.verifyPresenceDot(ChatWindow.Presence.OFFLINE)) {
            logPass("The presence circles in the chat are grey when offline");
        } else {
            logFail("The presence circles in the char are not grey when offline");
        }

        // 11
        //Sleep for a full minute to get a new timestamp. It seems that sleeping
        //only until the minute rolls over is not robust enough (perhaps the time
        //that Java gets and Origin's time are out of sync?)
        sleep(60000);

        chatWindowFriend.sendMessage(message3);
        if (Waits.pollingWait(() -> chatWindowFriend.verifyMessagesInBubble(messageGroup3))) {
            logPass("Send a message to the offline user");
        } else {
            logFail("Failed to send a message to the offline user");
        }

        // 12
        new MainMenu(driverUser).selectGoOnline();
        MacroSocial.restoreAndExpandFriends(driverUser);
        if (socialHubUser.verifyUserOnline()) {
            logPass("Successfully went back online");
        } else {
            logFail("Failed to go back online");
        }

        // 13
        final boolean thirdMessageOK = chatWindowUser.verifyMessagesInBubble(messageGroup3);
        if (thirdMessageOK) {
            logPass("The offline message appears when you log back on");
        } else {
            logFail("The offline message does not appear when you log back on");
        }

        // 14
        final boolean hasTimestamps = chatWindowUser.verifyTimestampCountAtLeast(2);
        if (hasTimestamps) {
            logPass("The message received while offline is timestamped");
        } else {
            logFail("The message received while offline is not timestamped");
        }

        // 15
        if (chatWindowUser.verifyPresenceDot(ChatWindow.Presence.ONLINE)) {
            logPass("The presence circles in the chat are green when back online");
        } else {
            logFail("The presence circles in the chat are not green when back online");
        }

        // 16
        chatWindowUser.sendMessage(message4);
        if (Waits.pollingWait(() -> chatWindowUser.verifyMessagesInBubble(messageGroup4))) {
            logPass("Sent a message from the user to friend");
        } else {
            logFail("Failed to send a message from the user to friend");
        }

        // 17
        chatWindowFriend.sendMessage(message5);
        if (Waits.pollingWait(() -> chatWindowFriend.verifyMessagesInBubble(messageGroup5))) {
            logPass("Sent a message from the friend to user");
        } else {
            logFail("Failed to send a message from the friend to user");
        }

        // 18
        final boolean fifthMessageOK = chatWindowUser.verifyMessagesInBubble(messageGroup5);
        final boolean fourthMessageOK = chatWindowFriend.verifyMessagesInBubble(messageGroup4);
        if (fifthMessageOK && fourthMessageOK) {
            logPass("Both fourth and fifth received messages remain in the chat window");
        } else {
            logFail("The fourth and fifth messages do not remain in the chat window");
        }
        softAssertAll();
    }
}
