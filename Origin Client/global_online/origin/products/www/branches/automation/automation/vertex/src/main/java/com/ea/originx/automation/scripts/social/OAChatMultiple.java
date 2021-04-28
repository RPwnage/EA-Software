package com.ea.originx.automation.scripts.social;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.Criteria;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.social.ChatWindow;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import java.util.ArrayList;
import org.openqa.selenium.StaleElementReferenceException;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.utils.Waits;

/**
 * Test the chat window behavior when opening chats with multiple users.
 *
 * @author lscholte
 * @author jmittertreiner
 * @author caleung
 */
public class OAChatMultiple extends EAXVxTestTemplate {

    private final ArrayList<UserAccount> friendAccounts = new ArrayList<>();
    private WebDriver driver;

    private String friend(int i) {
        return friendAccounts.get(i - 1).getUsername();
    }

    private boolean openChatWithFriend(int i) {
        return openChatWithFriend(i, 0);
    }

    private boolean openChatWithFriend(int i, int tries) {
        try {
            SocialHub socialHub = new SocialHub(driver);
            socialHub.getFriend(friend(i)).openChat();
            socialHub.waitForAngularHttpComplete();
        } catch (StaleElementReferenceException e) {
            if (tries < 5) { // only try up to 5 times to prevent infinite loop
                resultLogger.logInfo("Ignoring StaleElementReferenceException and trying again");
                // The official Selenium docs say "a good strategy is to look up the element again
                // so let's do just that
                return openChatWithFriend(i, ++tries);
            } else {
                resultLogger.logFatal("Reached maximum amount of tries");
                throw e;
            }
        }
        return Waits.pollingWait(() -> new ChatWindow(driver).verifyChatTabActive(friend(i)));
    }

    @TestRail(caseId = 13045)
    @Test(groups = {"social", "client_only", "full_regression"})
    public void testChatMultiple(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        // The number of tabs that Origin makes before going to overflow
        final int numTabs = 3;

        //Setup friends
        final AccountManager accountManager = AccountManager.getInstance();
        final Criteria criteria = new Criteria.CriteriaBuilder().build();

        UserAccount userAccount = accountManager.requestWithCriteria(criteria, 360);
        for (int i = 1; i <= 7; ++i) {
            friendAccounts.add(accountManager.requestWithCriteria(criteria, 360));
        }

        userAccount.cleanFriends();
        friendAccounts.forEach(account -> {
            account.cleanFriends();
            userAccount.addFriend(account);
        });

        logFlowPoint("Log into Origin."); // 1
        logFlowPoint("Open the 'Friends List'."); // 2
        logFlowPoint("Begin 3 separate chats with friends and verify that opening new chat " +
                "tabs open to the left of currently opened tabs."); // 3
        logFlowPoint("Verify the tab overflow menu appears after 4 chat tabs are opened."); // 4
        logFlowPoint("Select a chat tab and verify that users can click on chat sessions in the row of tabs."); // 5
        logFlowPoint("Verify that clicking on a tab opens the chat tab and makes it the current tab."); // 6
        logFlowPoint("In the 'Friends List', attempt to initiate a chat with a friend who already has a chat tab" +
                " open but is not the active tab, and verify the tab is focused and highlighted."); // 7
        logFlowPoint("Verify the user is able to close a chat tab by hovering over it and clicking the 'X' " +
                        "button"); // 8
        logFlowPoint("Verify there are no issues opening and closing multiple tabs within a single login instance."); // 9

        // 1
        driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        // 2
        MacroSocial.restoreAndExpandFriends(driver);
        SocialHub socialHub = new SocialHub(driver);
        if (socialHub.verifySocialHubVisible()) {
            logPass("Successfully opened the 'Friends List'.");
        } else {
            logFailExit("Could not open the 'Friends List'.");
        }

        // 3
        ChatWindow chatWindow = new ChatWindow(driver);
        socialHub.getFriend(friend(1)).openChat();
        socialHub.getFriend(friend(2)).openChat();
        socialHub.getFriend(friend(3)).openChat();
        if (chatWindow.verifyChatTabsExist() && chatWindow.verifyChatTabOrder(new String[]{friend(3), friend(2), friend(1)})) {
            logPass("Successfully opened chat tabs with friends 1, 2, and 3 and verified the tabs are in the correct order.");
        } else {
            logFailExit("Failed to open chat tabs with friends 1, 2, and 3 or failed to verify the tabs are in the correct order.");
        }

        // 4
        socialHub.getFriend(friend(4)).openChat();
        final boolean overflowTabExists = chatWindow.verifyOverflowChatTabExists();
        final boolean correctNumberOverflowTabs = chatWindow.verifyOverflowNumber(1);
        if (overflowTabExists && correctNumberOverflowTabs) {
            logPass("Verified that the 'Overflow' tab appears and shows 1 'Overflow' tab.");
        } else {
            logFail("Failed to verify that the 'Overflow' tab appeared or did not show 1 'Overflow' tab.");
        }

        // 5
        chatWindow.selectChatTab(friend(2), false);
        if (Waits.pollingWait(() -> chatWindow.verifyChatTabActive(friend(2)))) {
            logPass("Verified that the user was able to click on chat sessions in the row of tabs.");
        } else {
            logFail("Failed to verify that the user was able to click on chat sessions in the row of tabs.");
        }

        // 6
        chatWindow.selectChatTab(friend(3), false);
        if (Waits.pollingWait(() -> chatWindow.verifyChatTabActive(friend(3)))) {
            logPass("Verified that the user was able to click on a chat tab and make it the active tab.");
        } else {
            logFail("Failed to verify that the user was able to click on a chat tab and make it the active tab.");
        }

        // 7
        openChatWithFriend(1);
        if (chatWindow.verifyChatTabHighlighted()) {
            logPass("Verified that opening a chat with a friend whose chat tab is already open makes that chat " +
                    "the active chat.");
        } else {
            logFail("Failed to verify that opening a chat with a friend whose chat tab is already open makes the chat" +
                    " the active chat.");
        }

        // 8
        chatWindow.closeChatTab(friend(1), false);
        if (!chatWindow.verifyChatWithUserOpen(friend(1))) {
            logPass("Verified that the user was able to close a chat tab by hovering over it and clicking the 'X' " +
                    "button.");
        } else {
            logFail("Failed to verify that the user was able to close a chat tab by hovering over it and clicking " +
                    "the 'X' button.");
        }

        // 9
        socialHub.getFriend(friend(5)).openChat();
        socialHub.getFriend(friend(6)).openChat();
        chatWindow.closeChatTab(friend(5), false);
        chatWindow.closeChatTab(friend(6), false);
        boolean twoFriendsChatClosed = !chatWindow.verifyChatWithUserOpen(friend(5)) && !chatWindow.verifyChatWithUserOpen(friend(6));
        if (twoFriendsChatClosed) {
            logPass("Verified the user was able to open and close multiple chat tabs within a single login instance.");
        } else {
            logFail("Failed to verify the user was able to open and close multiple chat tabs within a single login instance.");
        }

        softAssertAll();
    }
}