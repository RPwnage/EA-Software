package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.macroaction.MacroProfile;
import com.ea.originx.automation.lib.pageobjects.dialog.SearchForFriendsDialog;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileFriendsTab;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Checks that an account with no friends has a 'Fiend Friends' link that leads
 * to the find friends dialog
 *
 * @author jdickens
 */
public class OANoFriendsBrowserProfile extends EAXVxTestTemplate {

    @TestRail(caseId = 13135)
    @Test(groups = {"social", "browser_only","full_regression"})
    public void testNoFriendsBrowserProfile(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.PERFORMANCE_NO_FRIENDS);
        String userAccountName = userAccount.getUsername();

        logFlowPoint("Launch a browser and navigate to the Origin website and log in with a user with no friends"); // 1
        logFlowPoint("Navigate to the user's 'Friends' tab"); // 2
        logFlowPoint("Verify an empty 'Friends' list contains a message and a 'Find Friends' link"); // 3
        logFlowPoint("Verify that clicking on the 'Find Friends' link pops up the 'Find Friends' dialog"); // 4

        // 1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with the user " + userAccountName);
        } else {
            logFailExit("Could not log into Origin with the user " + userAccountName);
        }

        // 2
        if (MacroProfile.navigateToFriendsTab(driver)) {
            logPass("Successfully navigated to the 'Friends' tab");
        } else {
            logFailExit("Failed to navigate to the 'Friends' tab");
        }

        // 3
        ProfileFriendsTab friendsTab = new ProfileFriendsTab(driver);
        boolean isNoFriendsMessageVisible = friendsTab.verifyNoFriendsMessage();
        boolean isFindFriendsHyperlinkVisible = friendsTab.verifyNoFriends(); // checks for the 'Find Friends' link
        if (isNoFriendsMessageVisible && isFindFriendsHyperlinkVisible) {
            logPass("Successfully verified the correct message in the 'Friends' tab appears with a 'Find Friends' link");
        } else {
            logFailExit("Failed to verify that the correct message in the 'Friends' tab appears with a 'Find Friends' link");
        }

        // 4
        friendsTab.clickFindFriendsLink();
        if (new SearchForFriendsDialog(driver).waitIsVisible()) {
            logPass("Successfully opened the 'Search for Friends' dialog");
        } else {
            logFailExit("Failed to open the 'Search for Friends' dialog");
        }

        softAssertAll();
    }
}
