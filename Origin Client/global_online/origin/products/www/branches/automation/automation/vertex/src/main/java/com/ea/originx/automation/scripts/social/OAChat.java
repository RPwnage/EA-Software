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
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * A test which verifies that messages are sent and received between two users
 *
 * @author caleung
 */

public class OAChat extends EAXVxTestTemplate {

    @TestRail(caseId = 1016752)
    @Test(groups = {"social", "client_only", "release_smoke"})
    public void testChat(ITestContext context) throws Exception {

        final OriginClient client1 = OriginClientFactory.create(context);
        final OriginClient client2 = OriginClientFactory.create(context);
        final UserAccount userAccountA = AccountManager.getInstance().createNewThrowAwayAccount();
        final UserAccount userAccountB = AccountManager.getInstance().createNewThrowAwayAccount();

        final String MESSAGE_ONE = "Chat One Friend One";
        final String MESSAGE_TWO = "Chat One Friend Two";
        final String[] FIRST_MESSAGE_GROUP = {MESSAGE_ONE};
        final String[] SECOND_MESSAGE_GROUP = {MESSAGE_TWO};

        logFlowPoint("Log into Origin with User A"); // 1
        logFlowPoint("Log into Origin with User B"); // 2
        logFlowPoint("Make User A and User B friends"); // 3
        logFlowPoint("From User A open chat window with User B"); // 4
        logFlowPoint("Send a message to User B"); // 5
        logFlowPoint("Verify the chat window automatically opens for User B"); // 6
        logFlowPoint("Verify the message has been received by User B"); // 7
        logFlowPoint("Send a message to User A"); // 8
        logFlowPoint("Verify the message has been received by User A"); // 9

        // 1
        WebDriver driver = startClientObject(context, client1);
        logPassFail(MacroLogin.startLogin(driver, userAccountA), true);

        // 2
        WebDriver driver2 = startClientObject(context, client2);
        logPassFail(MacroLogin.startLogin(driver2, userAccountB), true);

        // 3
        logPassFail(MacroSocial.addFriendThroughUI(driver, driver2, userAccountA, userAccountB), true);
        
        // 4
        MacroSocial.restoreAndExpandFriends(driver);
        SocialHub socialHub = new SocialHub(driver);
        socialHub.getFriend(userAccountB.getUsername()).openChat();
        ChatWindow chatWindow = new ChatWindow(driver);
        logPassFail(chatWindow.verifyChatOpen(), true);

        // 5
        chatWindow.sendMessage(MESSAGE_ONE);
        boolean messageSent = chatWindow.verifyMessagesInBubble(FIRST_MESSAGE_GROUP);
        logPassFail(messageSent, true);

        // 6
        ChatWindow chatWindow2 = new ChatWindow(driver2);
        logPassFail(chatWindow2.verifyChatOpen(), true);

        // 7
        boolean messageReceived = chatWindow2.verifyMessagesInBubble(FIRST_MESSAGE_GROUP);
        logPassFail(messageReceived, true);

        // 8
        chatWindow2.sendMessage(MESSAGE_TWO);
        messageSent = chatWindow2.verifyMessagesInBubble(SECOND_MESSAGE_GROUP);
        logPassFail(messageSent, true);

        // 9
        messageReceived = chatWindow.verifyMessagesInBubble(SECOND_MESSAGE_GROUP);
        logPassFail(messageReceived, true);

        softAssertAll();
    }
}