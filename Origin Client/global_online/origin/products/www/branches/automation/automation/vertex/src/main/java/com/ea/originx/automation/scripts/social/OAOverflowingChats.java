package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.social.ChatWindow;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.Criteria;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Test the chat window behavior when there are overflowing chats.
 *
 * @author caleung
 */
public class OAOverflowingChats extends EAXVxTestTemplate {

    private final ArrayList<UserAccount> friendAccounts = new ArrayList<>();

    private String friend(int i) {
        return friendAccounts.get(i - 1).getUsername();
    }

    @TestRail(caseId = 12646)
    @Test(groups = {"social", "client_only", "full_regression"})
    public void testOverflowingChats(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        // Setup friends
        final AccountManager accountManager = AccountManager.getInstance();
        final Criteria criteria = new Criteria.CriteriaBuilder().build();

        UserAccount userAccount = accountManager.requestWithCriteria(criteria, 360);
        for (int i = 1; i <= 11; ++i) {
            friendAccounts.add(accountManager.requestWithCriteria(criteria, 360));
        }

        userAccount.cleanFriends();
        friendAccounts.forEach(account -> {
            account.cleanFriends();
            userAccount.addFriend(account);
        });

        logFlowPoint("Log into Origin."); // 1
        logFlowPoint("Open the 'Friends List'."); // 2
        logFlowPoint("Open more than 3 chat windows and verify the overflow menu appears."); // 3
        logFlowPoint("Verify there is a counter on the overflow menu that shows the number of tabs in the" +
                " overflow menu."); // 4
        logFlowPoint("Click on the overflow menu and verify that it opens."); // 5
        logFlowPoint("Verify the username and presence is displayed in the tab."); // 6
        logFlowPoint("Select a user from the overflow menu and verify that when the user clicks on an item in the" +
                " overflow menu, it disappears from the overflow menu."); // 7
        logFlowPoint("Open more chats and verify the user can scroll through the overflow menu."); // 8
        logFlowPoint("Verify the user can close tabs within the overflow menu by clicking the 'X' button."); // 9
        logFlowPoint("Verify when there is no more overflow, the menu disappears."); // 10

        // 1
        WebDriver driver = startClientObject(context, client);
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
        List<String> friendsToOpenChatWith = Arrays.asList(friend(1), friend(2), friend(3), friend(4));
        friendsToOpenChatWith.forEach((friend) -> socialHub.getFriend(friend).openChat());
        if (chatWindow.verifyChatTabsExist() && chatWindow.verifyOverflowChatTabExists()) {
            logPass("Successfully opened multiple chats and verified the overflow menu appears.");
        } else {
            logFail("Failed to verify that the overflow menu appeared.");
        }

        // 4
        if (chatWindow.verifyOverflowNumber(1)) {
            logPass("Verified that there is a number on the overflow menu that shows the number of tabs in the menu.");
        } else {
            logFail("Failed to verify that there is a number on the overflow menu that shows the number of tabs" +
                    " in the menu.");
        }

        // 5
        chatWindow.clickOverflowTab();
        if (chatWindow.verifyOverFlowOpen()) {
            logPass("Verified that clicking on the overflow tab opens the overflow dropdown menu.");
        } else {
            logFail("Failed to verify that clicking on the overflow tab opens the overflow dropdown menu.");
        }

        // 6
        if (chatWindow.verifyNameInOverflow(friend(1)) && chatWindow.verifyUserHasPresenceInOverflow(friend(1))) {
            logPass("Verified that username and presence is displayed in the overflow dropdown.");
        } else {
            logFail("Failed to verify that the username and presence is displayed in the overflow dropdown.");
        }

        // 7
        chatWindow.selectChatTab(friend(1), true);
        if (!chatWindow.verifyNameInOverflow(friend(1))) {
            logPass("Verified that clicking on a chat tab in the overflow menu removes that chat tab from the" +
                    " overflow menu.");
        } else {
            logFail("Failed to verify that clicking on a chat tab in the overflow menu removes that chat tab " +
                    "from the overflow menu.");
        }

        // 8
        friendsToOpenChatWith = Arrays.asList(friend(5), friend(6), friend(7), friend(8), friend(9),
                friend(10), friend(11));
        friendsToOpenChatWith.forEach((friend) -> socialHub.getFriend(friend).openChat());
        if (chatWindow.verifyOverFlowScrollable()) {
            logPass("Verified the overflow dropdown is scrollable.");
        } else {
            logFail("Failed to verify that the overflow dropdown is scrollable.");
        }

        // 9
        chatWindow.closeChatTab(friend(1), true);
        if (!chatWindow.verifyNameInOverflow(friend(1))) {
            logPass("Verified the user is able to close chat tabs in the overflow menu by clicking the 'X' button.");
        } else {
            logFail("Failed to verify that the user is able to close chat tabs in the overflow menu by clicking the" +
                    " 'X' button.");
        }

        // 10
        List<String> friendsToCloseChat = Arrays.asList(friend(2), friend(3), friend(4), friend(5), friend(6),
                friend(7), friend(8));
        friendsToCloseChat.forEach((friend) -> chatWindow.closeChatTab(friend, true));
        if (!chatWindow.verifyOverflowChatTabExists()) {
            logPass("Verified that when there is no more overflow, the overflow tab disappears.");
        } else {
            logFail("Failed to verify that when there is no more overflow, the overflow tab disappears.");
        }

        softAssertAll();
    }
}