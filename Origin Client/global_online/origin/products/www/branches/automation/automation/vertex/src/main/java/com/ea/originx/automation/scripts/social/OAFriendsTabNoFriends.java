package com.ea.originx.automation.scripts.social;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.Criteria;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.dialog.SearchForFriendsDialog;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileFriendsTab;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the visibility and correctness of the 'No Friends' message when viewing
 * the 'Friends' tab of a user's profile
 *
 * @author lscholte
 */
public class OAFriendsTabNoFriends extends EAXVxTestTemplate {

    @TestRail(caseId = 13064)
    @Test(groups = {"social", "full_regression"})
    public void testFriendsTabNoFriends(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        //Setup friends
        final AccountManager accountManager = AccountManager.getInstance();
        final Criteria criteria = new Criteria.CriteriaBuilder().build();
        UserAccount userAccount = accountManager.requestWithCriteria(criteria, 360);
        userAccount.cleanFriends();

        logFlowPoint("Launch Origin and login with user " + userAccount.getUsername() + " who has no friends"); //1
        logFlowPoint("Navigate to the current user's profile"); //2
        logFlowPoint("Open the 'Friends' tab"); //3
        logFlowPoint("Verify the 'No Friends' message is correct"); //4
        logFlowPoint("Click 'Find Friends' and verify the 'Search for Friends' dialog has been opened"); //5

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        // 2
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.waitForPageToLoad();
        miniProfile.selectProfileDropdownMenuItem("View My Profile");
        ProfileHeader profileHeader = new ProfileHeader(driver);
        logPassFail(profileHeader.verifyOnProfilePage(), true);

        // 3
        profileHeader.openFriendsTab();
        logPassFail(profileHeader.verifyFriendsTabActive(), true);

        // 4
        ProfileFriendsTab friendsTab = new ProfileFriendsTab(driver);
        logPassFail(friendsTab.verifyNoFriendsMessage(), false);

        // 5
        friendsTab.clickFindFriendsLink();
        SearchForFriendsDialog searchFriendsDialog = new SearchForFriendsDialog(driver);
        logPassFail(searchFriendsDialog.waitIsVisible(), true);
        
        softAssertAll();
    }
}