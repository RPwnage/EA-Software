package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
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
 * A test which begins a chat with a friend, and goes offline to verify that the behaviour
 * of sending a chat message is unavailable until online connection is re-established
 *
 * @author dchalasani
 */
public class OAOfflineModeChat extends EAXVxTestTemplate {

    @TestRail(caseId = 39460)
    @Test(groups = {"social", "full_regression", "client_only"})
    public void testAdministrativeOfflineChat(ITestContext context) throws Exception {
        final OriginClient clientA = OriginClientFactory.create(context);
        final OriginClient clientB = OriginClientFactory.create(context);

        // Setup Accounts
        UserAccount userA = AccountManager.getRandomAccount();
        UserAccount userB = AccountManager.getRandomAccount();
        userA.cleanFriends();
        userB.cleanFriends();
        userA.addFriend(userB);

        final String MESSAGE1 = "OAOfflineModeChat Online: AccountB to AccountA";
        final String MESSAGE2 = "OAOfflineModeChat Online: AccountA to AccountB";
        final String MESSAGE3 = "OAOfflineModeChat Back Online: AccountA to AccountB";
        final String MESSAGE4 = "OAOfflineModeChat Back Online: AccountB to AccountA";

        final String[] messageGroup1 = {MESSAGE1};
        final String[] messageGroup2 = {MESSAGE2};
        final String[] messageGroup3 = {MESSAGE3};
        final String[] messageGroup4 = {MESSAGE4};

        logFlowPoint("Log into Origin with userA"); // 1
        logFlowPoint("Log into Origin with userB"); // 2
        logFlowPoint("Open a chat window from userB to userA"); // 3
        logFlowPoint("Send a message from userB to userA"); // 4
        logFlowPoint("Send a message from userA to userB"); // 5
        logFlowPoint("Go offline with userA"); // 6
        logFlowPoint("Verify that if userA goes offline, userB is notified of their new status"); // 7
        logFlowPoint("Verify: There is a 'You are offline' system message in the chat window for userA"); // 8
        logFlowPoint("Verify: You cannot send a message from userA when you are offline"); // 9
        logFlowPoint("Go back online as userA and verify that the 'Friends List' refreshes once the " +
                "client enters 'Online Mode'."); // 10
        logFlowPoint("Verify that userB is notified of userA going online"); // 11
        logFlowPoint("Send a message from userA to userB"); // 12
        logFlowPoint("Send a message from userB to userA"); // 13


        // 1
        WebDriver webDriverA = startClientObject(context, clientA);
        logPassFail((MacroLogin.startLogin(webDriverA, userA)), true);

        // 2
        WebDriver webDriverB = startClientObject(context, clientB);
        logPassFail((MacroLogin.startLogin(webDriverB, userB)), true);

        // 3
        String accountBUsername = userB.getUsername();
        String accountAUsername = userA.getUsername();

        MacroSocial.restoreAndExpandFriends(webDriverA);
        SocialHub socialHubUserA = new SocialHub(webDriverA);
        socialHubUserA.getFriend(accountBUsername).openChat();
        ChatWindow chatWindowUserA = new ChatWindow(webDriverA);

        MacroSocial.restoreAndExpandFriends(webDriverB);
        SocialHub socialHubUserB = new SocialHub(webDriverB);

        // Call verify just for the side effect of waiting
        // for the chat social window to be ready
        socialHubUserB.verifyFriendVisible(accountBUsername);
        socialHubUserB.getFriend(accountAUsername).openChat();
        ChatWindow chatWindowUserB = new ChatWindow(webDriverB);

        boolean socialHubAVisible = socialHubUserA.verifySocialHubVisible();
        boolean socialHubBVisible = socialHubUserB.verifySocialHubVisible();
        boolean chatWindowAOpen = chatWindowUserA.verifyChatOpen();
        boolean chatWindowBOpen = chatWindowUserB.verifyChatOpen();

        logPassFail((socialHubAVisible && socialHubBVisible && chatWindowAOpen && chatWindowBOpen), true);

        // 4
        chatWindowUserA.sendMessage(MESSAGE1);
        logPassFail(Waits.pollingWait(() -> chatWindowUserB.verifyMessagesInBubble(messageGroup1)), false);

        // 5
        chatWindowUserB.sendMessage(MESSAGE2);
        logPassFail(Waits.pollingWait(() -> chatWindowUserA.verifyMessagesInBubble(messageGroup2)), false);

        // 6
        MainMenu mainMenuUserA = new MainMenu(webDriverA);
        mainMenuUserA.selectGoOffline();
        logPassFail(socialHubUserA.verifyUserOffline(), true);

        // 7
        boolean isUserOffline = socialHubUserB.verifyFriendOffline(accountAUsername);
        logPassFail(isUserOffline, true);

        // 8
        logPassFail(chatWindowUserA.verifyOfflineSystemMessage(), false);

        // 9
        logPassFail(chatWindowUserA.verifyOfflineChatDisabled(), false);

        // 10
        mainMenuUserA.selectGoOnline();
        boolean isUserOnline = socialHubUserB.verifyFriendOnline(accountAUsername);
        logPassFail(isUserOnline, true);

        // 11
        MacroSocial.restoreAndExpandFriends(webDriverA);
        boolean isFriendsVisible = socialHubUserA.getAllFriends().size() > 0;
        logPassFail(isFriendsVisible, false);

        // 12
        socialHubUserA.getFriend(accountBUsername).openChat();
        chatWindowUserA.sendMessage(MESSAGE3);
        logPassFail(Waits.pollingWait(() -> chatWindowUserA.verifyMessagesInBubble(messageGroup3)), false);

        // 13
        socialHubUserB.getFriend(accountAUsername).openChat();
        chatWindowUserB.sendMessage(MESSAGE4);
        logPassFail(Waits.pollingWait(() -> chatWindowUserB.verifyMessagesInBubble(messageGroup4)), false);

        softAssertAll();
    }
}
