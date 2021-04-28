package com.ea.originx.automation.scripts.social;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.social.ChatWindow;
import com.ea.originx.automation.lib.pageobjects.social.ChatWindowMinimized;
import com.ea.originx.automation.lib.pageobjects.social.Friend;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;
import com.ea.vx.annotations.TestRail;

/**
 * A test which begins a chat with a friend, and verifies behaviour of sending
 * chat message and the chat window.
 *
 * @author glivingstone
 */
public class OAChatOneFriend extends EAXVxTestTemplate {

    @TestRail(caseId = 12640) 
    @Test(groups = {"client_only", "full_regression", "social"})
    public void testChatOneFriend(ITestContext context) throws Exception {

        final OriginClient client1 = OriginClientFactory.create(context);
        final OriginClient client2 = OriginClientFactory.create(context);

        // Setup Friends
        UserAccount userAccount = AccountManager.getRandomAccount();
        UserAccount friendAccount = AccountManager.getRandomAccount();
        userAccount.cleanFriends();
        friendAccount.cleanFriends();
        userAccount.addFriend(friendAccount);
        final String MESSAGE = "Chat One Friend One";
        final String URL_MESSAGE = "www.youtube.com";

        logFlowPoint("Log into Origin with User A"); // 1
        logFlowPoint("Verify the 'Friends List' opens"); // 2
        logFlowPoint("Click on a friend and verify that User B is highlighted"); // 3
        logFlowPoint("Verify the highlight colour is grey"); // 4
        logFlowPoint("Double click on User B and verify it opens a chat window"); // 5
        logFlowPoint("Log into Origin with User B, have User A send multiple messages to User B and verify timestamps are shown with a single message, or with groups of messages"); // 6
        logFlowPoint("Verify that if a friend goes online, offline or away, there's a status message"); // 7
        logFlowPoint("Verify both User A and User B's avatar are displayed in the chat window"); // 8
        logFlowPoint("Verify that User A's avatar and messages in the chat conversation are right-justified"); // 9
        logFlowPoint("Verify that User B's avatar and messages are left-justified"); // 10
        logFlowPoint("Click on User A's avatar and verify it navigates to User A's profile page"); // 11
        logFlowPoint("Verify that User B's username appears in either the chat window title bar or in the chat tab"); // 12
        logFlowPoint("Verify the size of the box where users type messages changes based on the length of the message"); // 13
        logFlowPoint("Verify chat auto-scrolls the conversation unless the user manually scrolls the chat history up"); // 14
        logFlowPoint("Send a really long message and verify that no more 2048 characters can be entered into a chat"); // 15
        logFlowPoint("Send a URL as a chat message and verify URLs can be entered and are hyperlinked when sent"); // 16
        logFlowPoint("Verify that either user can click the hyperlink and it opens a web browser with the appropriate page"); // 17
        logFlowPoint("Send HTML code as a chat message and verify the HTML is stripped before the message is sent"); // 18
        logFlowPoint("Verify the chat window can be minimized and restored"); // 19
        logFlowPoint("Verify minimized chat window displays friend's username and presence"); // 20

        // 1
        WebDriver driver1 = startClientObject(context, client1);
        if (MacroLogin.startLogin(driver1, userAccount)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        // 2
        MacroSocial.restoreAndExpandFriends(driver1);
        SocialHub socialHub = new SocialHub(driver1);
        if (socialHub.verifySocialHubVisible()) {
            logPass("Successfully opened friends list");
        } else {
            logFailExit("Could not open friends list");
        }

        // 3
        Friend friendUser = socialHub.getFriend(friendAccount.getUsername());
        if (friendUser.verifyLeftClickFriend()) {
            logPass("Verified that a single click highlights the friend");
        } else {
            logFailExit("Failed to verify that a single click highlights the friend");
        }

        //4
        if (friendUser.verifyHighlightedFriend()) {
            logPass("Verified the highlight colour is grey");
        } else {
            logFailExit("Failed to verify the highlight colour is grey");
        }

        //5
        ChatWindow chatWindow = new ChatWindow(driver1);
        socialHub.getFriend(friendAccount.getUsername()).openChat();
        if (chatWindow.verifyChatOpen()) {
            logPass("Successfully opened chat window with " + friendAccount.getUsername());
        } else {
            logFailExit("Could not open a chat window with " + friendAccount.getUsername());
        }

        //6
        WebDriver driver2 = startClientObject(context, client2);
        MacroLogin.startLogin(driver2, friendAccount);
        SocialHub friendSocialHub = new SocialHub(driver2);
        chatWindow.sendMultipleMessages(3, MESSAGE);
        sleep(3000);
        if (chatWindow.verifyTimestampCountIs(1)) {
            logPass("Verified that a timestamp appears with the first message");
        } else {
            logFailExit("Failed to verify that a timestamp appears with the first message");
        }

        //7
        MacroSocial.restoreAndExpandFriends(driver2);
        friendSocialHub.setUserStatusAway();
        boolean isStatusAway = friendSocialHub.verifyPresenceDotAway();
        friendSocialHub.setUserStatusInvisible();
        boolean isStatusInvisible = friendSocialHub.verifyPresenceDotInvisible();
        if (isStatusAway && isStatusInvisible) {
            logPass("Verified that if a friend goes online, offline or away, there's a status message");
        } else {
            logFailExit("Failed to verify that if a friend goes online, offline or away, there's a status message");
        }

        //8
        friendSocialHub.setUserStatusOnline();
        ChatWindow chatWindow2 = new ChatWindow(driver2);
        chatWindow2.sendMessage(MESSAGE);
        sleep(3000); //needs to wait in order to view the avatars
        if (chatWindow.verifyAvatarsAreDisplayed()) {
            logPass("Avatars in the chat window match the users' profile avatars.");
        } else {
            logFail("Avatars in the chat window do not match the user's profile avatars.");
        }

        //9
        chatWindow.sendMessage(MESSAGE);
        if (chatWindow.verifyRightMessageAndAvatarPosition(userAccount.getUsername())) {
            logPass("Verified that messages sent by User A are located on the right side of the chat.");
        } else {
            logFail("Failed to verify that messages sent by User A are located on the right side of the chat.");
        }

        //10
        chatWindow2.sendMessage(URL_MESSAGE);
        if (chatWindow.verifyLeftMessageAndAvatarPosition(friendAccount.getUsername())) {
            logPass("Verified that messages sent by User B are located on the left side of the chat.");
        } else {
            logFail("Failed to verify that messages sent by User B are located on the left side of the chat.");
        }

        //11
        String userAvatar = new MiniProfile(driver1).getAvatarSource();
        chatWindow.clickUserAvatar(userAvatar);
        if (new ProfileHeader(driver1).verifyUsername(userAccount.getUsername())) {
            logPass("Verified user account's profile can be viewed by clicking the 'Chat Window' avatar");
        } else {
            logFailExit("Failed: Cannot view user account's profile by clicking the 'Chat Window' avatar");
        }

        //12
        if (chatWindow.verifyChatWindowTitle(friendAccount.getUsername())) {
            logPass("Successfully verified that User B's username appears in either the chat window title bar or in the tab chat");
        } else {
            logFailExit("Failed to verify that User B's username appears in either the chat window title bar or in the tab chat.");
        }

        //13
        if (chatWindow.verifyTextInputAreaResizes()) {
            logPass("Successfully verified that the box where users type messages changes the size based on the length of the messages");
        } else {
            logFailExit("Failed to verify that the box where users type messages changes the size based on the length of the messages.");
        }

        //14
        chatWindow.sendMessage("");
        chatWindow2.sendMultipleMessages(5, MESSAGE);
        boolean isConversationScrolledToEnd = chatWindow.verifyPhishingWarningNotVisible(); //should be true, as we need to have the scrollbar present
        chatWindow.scrollToBeginningOfChat();//scroll to the beginning of the conversation
        chatWindow2.sendMessage(MESSAGE);
        boolean verifyNoScrolling = chatWindow.verifyPhishingWarningNotVisible(); //should be false, as we need to see the phishing warning
        if (isConversationScrolledToEnd && !verifyNoScrolling) {
            logPass("Successfully verified chat auto-scrolls the conversation unless the user manually scrolls the chat history up");
        } else {
            logFailExit("Failed to verify chat auto-scrolls the conversation unless the user manually scrolls the chat history up");
        }

        //15
        if (chatWindow.verifyMaximumMessageSizeReached()) {
            logPass("Successfully verified no more than 2048 characters can be entered into a chat at any given time.");
        } else {
            logFailExit("Failed to verify no more than 2048 characters can be entered into a chat at any given time.");
        }

        //16
        chatWindow.sendMessage(URL_MESSAGE);
        if (chatWindow.verifyColourOfURL(URL_MESSAGE)) {
            logPass("Successfully verified URLs can be entered with no issues and are hyperlinked when sent");
        } else {
            logFailExit("Failed to verify URLs can be entered with no issues and are hyperlinked when sent");
        }

        //17
        if (chatWindow.verifyHyperlinkCanBeClicked(client1)) {
            logPass("Successfully verified hyperlinks can be clicked on by either user and opens a web browser with the appropriate page");
        } else {
            logFailExit("Failed to verify hyperlinks can be clicked on by either user and opens a web browser with the appropriate page");
        }

        //18
        String HTML_MESSAGE = "<strong>This text is bold</strong>";
        chatWindow.sendMessage(HTML_MESSAGE);
        if (!chatWindow.getLastMessage().contains("<") && !chatWindow.getLastMessage().contains(">")) {
            logPass("Successfully verified any HTML entered is stripped before the message is sent");
        } else {
            logFailExit("Failed to verify any HTML entered is stripped before the message is sent");
        }

        //19
        chatWindow.minimizeChatWindow();
        ChatWindowMinimized chatWindowMinimized = new ChatWindowMinimized(driver1);
        boolean isChatWindowMinimized = chatWindowMinimized.verifyMinimized();
        chatWindowMinimized.restoreChatWindow();
        boolean isChatWindowRestored = chatWindow.verifyChatOpen();
        if (isChatWindowMinimized && isChatWindowRestored) {
            logPass("Verified the chat window can be minimized and restored");
        } else {
            logFailExit("Failed: Cannot verify the chat window can be minimized and restored");
        }

        //20
        chatWindow.minimizeChatWindow();
        if (chatWindowMinimized.verifyNameAndPresenceDisplayed(friendAccount.getUsername())) {
            logPass("Successfully verified minimized chat window displays friend's username and presence");
        } else {
            logFail("Failed to verify minimized chat window displays friend's username and presence.");
        }
        softAssertAll();
    }
}
