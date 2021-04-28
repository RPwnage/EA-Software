package com.ea.originx.automation.scripts.social;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.Criteria;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroSocial;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.social.ChatWindow;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test searching for current friends using the Social Hub search
 *
 * @author lscholte
 */
public class OAFriendsListSearch extends EAXVxTestTemplate {

    @TestRail(caseId = 13202)
    @Test(groups = {"social", "client_only", "full_regression"})
    public void testFriendsListSearch(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        //Setup friends
        final AccountManager accountManager = AccountManager.getInstance();
        final Criteria criteria = new Criteria.CriteriaBuilder().build();

        UserAccount userAccount = accountManager.requestWithCriteria(criteria, 360);
        UserAccount friend1Account = accountManager.requestWithCriteria(criteria, 360);
        UserAccount friend2Account = accountManager.requestWithCriteria(criteria, 360);

        userAccount.cleanFriends();
        friend1Account.cleanFriends();
        friend2Account.cleanFriends();

        userAccount.addFriend(friend1Account);
        userAccount.addFriend(friend2Account);

        //We want to search for the last 6 characters of Friend 1's username
        final int startIndex = Math.max(0, friend1Account.getUsername().length() - 6);
        final String friend1SearchName = friend1Account.getUsername().substring(startIndex);

        logFlowPoint("Launch and login to Origin with " + userAccount.getUsername()); //1
        logFlowPoint("Verify " + friend1Account.getUsername() + " and " + friend2Account.getUsername() + " are on the friends list"); //2
        logFlowPoint("Enter the search text '" + friend1SearchName + "' into the search bar"); //3
        logFlowPoint("Verify only " + friend1Account.getUsername() + " is visible in the friends list"); //4
        logFlowPoint("Clear the search bar text using the X button on the search bar"); //5
        logFlowPoint("Verify all friends are visible in the friend list"); //6
        logFlowPoint("Enter the text '" + friend1SearchName + "' into the search bar again"); //7
        logFlowPoint("Clear the search bar text manually"); //8
        logFlowPoint("Enter the text '" + friend1SearchName + "' into the search bar again"); //9
        logFlowPoint("Verify the real name of " + friend1Account.getUsername() + " appears when hovering over their friend bar"); //10
        logFlowPoint("Select 'Send Message' from " + friend1Account.getUsername() + "'s right-click menu"); //11

        // 1
        WebDriver driver = startClientObject(context, client);

        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user.");
        } else {
            logFailExit("Could not log into Origin with the user.");
        }

        //2
        MacroSocial.restoreAndExpandFriends(driver);
        SocialHub socialHub = new SocialHub(driver);
        boolean friend1Visible = socialHub.verifyFriendVisible(friend1Account.getUsername());
        boolean friend2Visible = socialHub.verifyFriendVisible(friend2Account.getUsername());
        if (friend1Visible && friend2Visible) {
            logPass("Found both friend 1 and friend 2 on the friends list");
        } else {
            logFailExit("Friend 1 or friend 2 could not be found on the friends list");
        }

        //3
        socialHub.searchFriendsByText(friend1SearchName);
        if (socialHub.verifySearchBarText(friend1SearchName)) {
            logPass("Successfully set the search bar text");
        } else {
            logFail("Could not enter text into the search bar");
        }

        //4
        friend1Visible = socialHub.verifyFriendVisible(friend1Account.getUsername());
        friend2Visible = socialHub.verifyFriendVisible(friend2Account.getUsername());
        if (friend1Visible && !friend2Visible) {
            logPass("Successfully searched for friend 1");
        } else {
            logFailExit("Could not search for friend 1");
        }

        //5
        if (!socialHub.verifySearchBarClearButtonExists()) {
            logFailExit("The X button on the search bar could not be found");
        }
        socialHub.clearSearchBarText(true);
        sleep(2000); //Allow time for social hub to stabilize
        if (socialHub.verifySearchBarText("")) {
            logPass("Successfully cleared the search bar using the X button");
        } else {
            logFail("Could not clear the search bar using the X button");
        }

        //6
        friend1Visible = socialHub.verifyFriendVisible(friend1Account.getUsername());
        friend2Visible = socialHub.verifyFriendVisible(friend2Account.getUsername());
        if (friend1Visible && friend2Visible) {
            logPass("All friends are now visible in the friends list");
        } else {
            logFail("Not all friends are visible in the friends list");
        }

        //7
        socialHub.searchFriendsByText(friend1SearchName);
        sleep(2000); //Allow time for social hub to stabilize
        friend1Visible = socialHub.verifyFriendVisible(friend1Account.getUsername());
        friend2Visible = socialHub.verifyFriendVisible(friend2Account.getUsername());
        if (friend1Visible && !friend2Visible) {
            logPass("Successfully searched for friend 1 again");
        } else {
            logFailExit("Could not search for friend 1");
        }

        //8
        socialHub.clearSearchBarText(true);
        sleep(2000); //Allow time for social hub to stabilize
        friend1Visible = socialHub.verifyFriendVisible(friend1Account.getUsername());
        friend2Visible = socialHub.verifyFriendVisible(friend2Account.getUsername());
        if (friend1Visible && friend2Visible) {
            logPass("Successfully cleared the search bar manually");
        } else {
            logFailExit("Could not clear the search bar manually");
        }

        //9
        socialHub.searchFriendsByText(friend1SearchName);
        sleep(2000); //Allow time for social hub to stabilize
        friend1Visible = socialHub.verifyFriendVisible(friend1Account.getUsername());
        friend2Visible = socialHub.verifyFriendVisible(friend2Account.getUsername());
        if (friend1Visible && !friend2Visible) {
            logPass("Successfully searched for friend 1 again");
        } else {
            logFailExit("Could not search for friend 1");
        }

        //10
        socialHub.getFriend(friend1Account.getUsername()).viewProfile();
        ProfileHeader profileHeader = new ProfileHeader(driver);

        profileHeader.waitForJQueryAJAXComplete(20);
        String expectedRealName = profileHeader.getUserRealName();
        if (socialHub.verifyRealName(friend1Account.getUsername(), expectedRealName)) {
            logPass("The real name of friend 1 appears and is correct");
        } else {
            logFail("The real name of friend 1 does not appear or is incorrect");
        }

        //11
        socialHub.getFriend(friend1Account.getUsername()).openChat(true);
        ChatWindow chatWindow = new ChatWindow(driver);
        if (chatWindow.verifyChatOpen()) {
            logPass("Successfully opened a chat using the right-click menu");
        } else {
            logFailExit("Could not open a chat using the right-click menu");
        }

        softAssertAll();
    }
}
