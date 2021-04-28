package com.ea.originx.automation.scripts.social;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.social.ChatWindow;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * A test which begins a chat with a friend, and verifies behaviour of sending
 * chat message and the chat window.
 *
 * @author glivingstone
 */
public class OABasicChat extends EAXVxTestTemplate {

    @TestRail(caseId = 3121919)
    @Test(groups = {"social", "long_smoke", "client_only"})
    public void testChatOneFriend(ITestContext context) throws Exception {

        final OriginClient clientA = OriginClientFactory.create(context);
        final OriginClient clientB = OriginClientFactory.create(context);

        // Setup Friends
        UserAccount userAccount = AccountManager.getRandomAccount();
        UserAccount friendAccount = AccountManager.getRandomAccount();
        UserAccount secondFriend = AccountManager.getRandomAccount();
        userAccount.cleanFriends();
        friendAccount.cleanFriends();
        secondFriend.cleanFriends();
        userAccount.addFriend(friendAccount);
        userAccount.addFriend(secondFriend);

        final String MESSAGE_ONE = "Chat One Friend One";
        final String MESSAGE_TWO = "Chat One Friend Two";
        final String[] FIRST_MESSAGE_GROUP = {MESSAGE_ONE};
        final String[] SECOND_MESSAGE_GROUP = {MESSAGE_TWO};

        logFlowPoint("Log into Origin with " + userAccount.getUsername()); // 1
        logFlowPoint("Open Chat Window with " + secondFriend.getUsername()); // 2
        logFlowPoint("Open Chat Window with " + friendAccount.getUsername()); // 3
        logFlowPoint("Switch to " + secondFriend.getUsername() + "'s Chat Window"); // 4
        logFlowPoint("Switch to " + friendAccount.getUsername() + "'s Chat Window"); // 5
        logFlowPoint("Log into Origin with " + friendAccount.getUsername()); // 6
        logFlowPoint("Send a Message to " + friendAccount.getUsername()); // 7
        logFlowPoint("Verify the Chat Window Automatically Opens"); // 8
        logFlowPoint("Verify the Messages Sent While Offline Appear"); // 9
        logFlowPoint("Send a Message to " + userAccount.getUsername()); // 10
        logFlowPoint("Verify the Message was Received"); // 11

        // 1
        WebDriver driver = startClientObject(context, clientA);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        // 2
        MacroSocial.restoreAndExpandFriends(driver);
        SocialHub socialHub = new SocialHub(driver);
        socialHub.getFriend(secondFriend.getUsername()).openChat();
        ChatWindow chatWindow = new ChatWindow(driver);
        if (chatWindow.verifyChatOpen()) {
            logPass("Successfully opened Chat Window with the friend.");
        } else {
            logFailExit("Could not open a chat window with the friend.");
        }

        // 3
        socialHub.getFriend(friendAccount.getUsername()).openChat();
        if (chatWindow.verifyChatTabActive(friendAccount.getUsername())) {
            logPass("Successfully opened Chat Window with the friend.");
        } else {
            logFailExit("Could not open a chat window with the friend.");
        }

        // 4
        chatWindow.selectChatTab(secondFriend.getUsername(), false);
        if (chatWindow.verifyChatTabActive(secondFriend.getUsername())) {
            logPass("Successfully opened Chat Window with the friend.");
        } else {
            logFailExit("Could not open a chat window with the friend.");
        }

        // 5
        chatWindow.selectChatTab(friendAccount.getUsername(), false);
        if (chatWindow.verifyChatTabActive(friendAccount.getUsername())) {
            logPass("Successfully opened Chat Window with the friend.");
        } else {
            logFailExit("Could not open a chat window with the friend.");
        }
        chatWindow.closeChatTab(secondFriend.getUsername(), false);

        // 6
        WebDriver driver2 = startClientObject(context, clientB);
        if (MacroLogin.startLogin(driver2, friendAccount)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        // 7
        chatWindow.sendMessage(MESSAGE_ONE);
        boolean messageSent = Waits.pollingWait(() -> chatWindow.verifyMessagesInBubble(FIRST_MESSAGE_GROUP));
        if (messageSent) {
            logPass("Successfully sent message to friend.");
        } else {
            logFailExit("Could not send message to friend.");
        }

        // 8
        ChatWindow chatWindow2 = new ChatWindow(driver2);
        if (chatWindow2.verifyChatOpen()) {
            logPass("Chat Window automatically opened upon receiving the message.");
        } else {
            logFailExit("Chat Window did not open automatically.");
        }

        // 9
        boolean messageReceived = Waits.pollingWait(() -> chatWindow2.verifyMessagesInBubble(FIRST_MESSAGE_GROUP));
        if (messageReceived) {
            logPass("Message sent earlier has been received.");
        } else {
            logFail("Did not receive message that was sent earlier");
        }

        // 10
        chatWindow2.sendMessage(MESSAGE_TWO);
        messageSent = Waits.pollingWait(() -> chatWindow2.verifyMessagesInBubble(SECOND_MESSAGE_GROUP));
        if (messageSent) {
            logPass("Successfully sent message from the friend to the user.");
        } else {
            logFailExit("Could not send message from the friend to the user.");
        }

        // 11
        messageReceived = Waits.pollingWait(() -> chatWindow.verifyMessagesInBubble(SECOND_MESSAGE_GROUP));
        if (messageReceived) {
            logPass("Message successfully received by the user.");
        } else {
            logFail("The user did not receive message.");
        }
        softAssertAll();
    }
}
