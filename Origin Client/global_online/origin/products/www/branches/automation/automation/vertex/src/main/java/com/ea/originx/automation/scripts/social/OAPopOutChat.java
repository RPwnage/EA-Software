package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.helpers.UserAccountHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.social.ChatWindow;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * This script checks the functionality of Origin's chat window when it is popped out
 *
 * @author jdickens
 */
public class OAPopOutChat extends EAXVxTestTemplate {

    @TestRail(caseId = 12657)
    @Test(groups = {"social", "full_regression", "client_only"})
    public void testPopOutChat(ITestContext context) throws Exception {

        OriginClient clientA = OriginClientFactory.create(context);
        OriginClient clientB = OriginClientFactory.create(context);
        UserAccount userAccountA = AccountManager.getRandomAccount();
        final String userAccountAName = userAccountA.getUsername();
        UserAccount userAccountB = AccountManager.getRandomAccount();
        final String userAccountBName = userAccountB.getUsername();
        UserAccount userAccountC = AccountManager.getRandomAccount();
        final String userAccountCName = userAccountC.getUsername();

        userAccountA.cleanFriends();
        userAccountB.cleanFriends();
        userAccountC.cleanFriends();
        UserAccountHelper.addFriends(userAccountA, userAccountB, userAccountC);

        final String firstMessage = "Message Number: 1";
        final String secondMessage = "Message Number: 2";
        final String thirdMessage = "Message Number: 3";
        final String fourthMessage = "Message Number: 4";

        logFlowPoint("Launch Origin and login with an account that has multiple friends"); // 1
        logFlowPoint("Launch a second Origin client and login with an account that is friends with the first account"); // 2
        logFlowPoint("Click on a friend's name from the friends list in the social hub and click 'Send Message'"); // 3
        logFlowPoint("Click the 'Pop-out' arrow in the upper right corner of the message box"); // 4
        logFlowPoint("'Pop' the chat window back into the main browser"); // 5
        logFlowPoint("Start a conversation with a second friend and verify the message is " +
                "saved when popping in and out of the chat window"); // 6
        logFlowPoint("Verify that Sending/receiving messages still works in the popped-out chat window"); // 7
        logFlowPoint("Verify that Timestamps still appear in the popped-out chat window"); // 8
        logFlowPoint("Verify that the Typing animation still works in the popped-out chat window"); // 9
        logFlowPoint("Verify that the User Avatar is still present in the popped-out chat window"); // 10
        logFlowPoint("Verify that the Input field scroll bar still works in the popped-out chat window"); // 11
        logFlowPoint("Verify that the chat window can be minimized separately once it's been popped out"); // 12
        logFlowPoint("Verify that closing the popped-out chat window closes the chat window outright"); // 13

        // 1
        WebDriver driver1 = startClientObject(context, clientA);
        if (MacroLogin.startLogin(driver1, userAccountA)) {
            logPass("Launched Origin and logged in with an account " + userAccountAName + " that has multiple friends");
        } else {
            logFailExit("Failed to log into Origin with account " + userAccountAName + " that has multiple friends");
        }

        // 2
        WebDriver driver2 = startClientObject(context, clientB);
        if (MacroLogin.startLogin(driver2, userAccountB)) {
            logPass("Successfully logged into second Origin client with the user " + userAccountBName);
        } else {
            logFailExit("Failed to log into Origin client with the user " + userAccountBName);
        }

        // 3
        MacroSocial.openChatWithFriend(driver1, userAccountCName);
        ChatWindow chatWindowA = new ChatWindow(driver1);
        if (chatWindowA.verifyChatOpen()) {
            logPass("Clicked on a friend's name from the friends list and selected send a message");
        } else {
            logFail("Failed to click on a friend's name from the friends list or select send a message");
        }

        // 4
        chatWindowA.popoutChatWindow();
        // Note that verifyChatWindowPoppedOut() switches the active window to the Chat Window
        if (chatWindowA.verifyChatWindowPoppedOut(driver1) && chatWindowA.verifyChatOpen()) {
            logPass("Successfully popped out the chat window");
        } else {
            logFailExit("Failed to pop out the chat window");
        }

        // 5
        chatWindowA.popChatWindowBackIn();
        Waits.waitForWindowHandlesToStabilize(driver1,30);
        Waits.switchToPageThatMatches(driver1, OriginClientData.MAIN_SPA_PAGE_URL);
        if (chatWindowA.verifyChatOpen()) {
            logPass("Successfully popped the chat window back into the main window");
        } else {
            logFailExit("Failed to pop the chat window back into the main window");
        }

        // 6
        chatWindowA.closePoppedInChatWindow();
        MacroSocial.openChatWithFriend(driver1, userAccountBName);
        chatWindowA.sendMessage(firstMessage);
        chatWindowA.popoutChatWindow();
        chatWindowA.switchToPoppedOutChatWindow(driver1);
        boolean isMessageSavedPoppedOut = chatWindowA.getLastMessage().equals(firstMessage);
        chatWindowA.popChatWindowBackIn();
        Waits.waitForWindowHandlesToStabilize(driver1,30);
        Waits.switchToPageThatMatches(driver1, OriginClientData.MAIN_SPA_PAGE_URL);
        boolean isMessageSavedPoppedIn = chatWindowA.getLastMessage().equals(firstMessage);
        if (isMessageSavedPoppedOut && isMessageSavedPoppedIn) {
            logPass("Sent a message to a second friend and verified that the text entered into the input field is saved when popping in and out of the chat window.");
        } else {
            logFail("Failed to verify that the text entered into the input field is saved when popping in and out of the chat window");
        }

        // 7
        chatWindowA.popoutChatWindow();
        chatWindowA.switchToPoppedOutChatWindow(driver1);
        ChatWindow chatWindowB = new ChatWindow(driver2);
        chatWindowB.popoutChatWindow();
        chatWindowB.switchToPoppedOutChatWindow(driver2);
        chatWindowB.sendMessage(secondMessage);
        // Note that getLastMessage() returns the last message sent from the active user and not
        // the last overall message
        boolean isMessageReceivedInFirstChatWindow = chatWindowA.getLastMessage().equals(firstMessage);
        boolean isMessageReceivedInSecondChatWindow = chatWindowB.getLastMessage().equals(secondMessage);
        if (isMessageReceivedInSecondChatWindow && isMessageReceivedInFirstChatWindow) {
            logPass("Successfully sent messages between both users in the popped out chat windows");
        } else {
            logFailExit("Failed to send messages between both users in the popped out chat windows");
        }

        // 8
        if (chatWindowA.verifyTimestampCountAtLeast(1) && chatWindowB.verifyTimestampCountAtLeast(1)) {
            logPass("Verified that a timestamp appears in both chat windows");
        } else {
            logFailExit("Failed to verify that a timestamp appears in both chat windows");
        }

        // 9
        chatWindowB.typeMessageWithoutEnter(thirdMessage);
        boolean verifyTypingDotsAreVisibleInFirstChat = chatWindowA.verifyTypingDotsVisible();
        chatWindowA.typeMessageWithoutEnter(fourthMessage);
        boolean verifyTypingDotsAreVisibleInSecondChat = chatWindowB.verifyTypingDotsVisible();
        if (verifyTypingDotsAreVisibleInFirstChat && verifyTypingDotsAreVisibleInSecondChat) {
            logPass("Verified that typing dots are visible when sending messages back and forth between " + userAccountBName + " to " + userAccountAName);
        } else {
            logFail("Could not verify that typing dots are visible when sending messages back and forth between " + userAccountBName + " to " + userAccountAName);
        }
        chatWindowB.typeEnter();
        sleep(3000); // Allow time for chat messages to appear in the correct order
        chatWindowA.typeEnter();
        
        // 10
        boolean verifyAvatarsVisible = chatWindowA.verifyAvatarsAreDisplayed() && chatWindowB.verifyAvatarsAreDisplayed();
        if (verifyAvatarsVisible) {
            logPass("Verified that the user avatars are still present in the popped-out chat windows");
        } else {
            logFail("Failed to verify that the user avatars are still present in the popped-out chat windows");
        }

        // 11
        // Send 5 messages in order to hide the phishing warning
        chatWindowB.sendMultipleMessages(5, "Extra Message");

        // Check that scrolling to the top of the chat works
        boolean isPhishingWarningNotVisibleBeforeScrollUp = chatWindowB.verifyPhishingWarningNotVisible();
        chatWindowB.scrollToBeginningOfChat();
        boolean isPhishingWarningVisibleAfterScrollUp = !chatWindowB.verifyPhishingWarningNotVisible();

        // Check that scrolling to the bottom of the chat works
        chatWindowB.scrollToEndOfChat();
        boolean isPhishingWarningNotVisibleAfterScrollDown = chatWindowB.verifyPhishingWarningNotVisible();
        if (isPhishingWarningNotVisibleBeforeScrollUp && isPhishingWarningVisibleAfterScrollUp && isPhishingWarningNotVisibleAfterScrollDown) {
            logPass("Verified that the input field scroll bar still works in the popped-out chat window of " + userAccountBName);
        } else {
            logFail("Failed to verify that the input field scroll bar still works in the popped-out chat window of " + userAccountBName);
        } 

        // 12
        chatWindowA.switchToPoppedOutChatWindow(driver1);
        WebDriver qtDriver1 = clientA.getQtWebDriver();
        Waits.switchToPageThatMatches(driver1, OriginClientData.MAIN_SPA_PAGE_URL);
        chatWindowA.minimizePoppedOutChatWindow(qtDriver1);
        // Note that the javascript command in verifyPoppedOutChatWindowMinimized(driver1)
        // must be executed from the 'Main SPA' page otherwise it will not work
        if (chatWindowA.verifyPoppedOutChatWindowMinimized(driver1)) {
            logPass("Successfully minimized the popped out chat window for user " + userAccountAName);
        } else {
            logFail("Failed to minimize the popped out chat window for user " + userAccountAName);
        }

        // 13
        WebDriver qtDriver2 = clientB.getQtWebDriver();
        chatWindowB.closePoppedOutChatWindow(qtDriver2);
        Waits.waitForWindowHandlesToStabilize(driver2,30);
        if (!chatWindowB.verifyChatOpen()) {
            logPass("Successfully closed the popped out chat window for user " + userAccountBName);
        } else {
            logFail("Failed to close the popped out chat window for user " + userAccountBName);
        }

        softAssertAll();
    }
}