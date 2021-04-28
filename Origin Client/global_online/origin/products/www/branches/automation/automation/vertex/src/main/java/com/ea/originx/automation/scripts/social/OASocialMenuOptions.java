package com.ea.originx.automation.scripts.social;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.dialog.SearchForFriendsDialog;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.resources.OriginClientConstants;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * A test to verify that social menu options; checklist includes:
 *
 * - 'Friends' tab opens the dropdown with expected items
 * - 'Add a Friend…' menu item behaves correctly
 * - 'My Profile' on 'View' menu navigates to the user profile page
 *
 * @author micwang
 */
public class OASocialMenuOptions extends EAXVxTestTemplate {

    @TestRail(caseId = 9787)
    @Test(groups = {"client_only", "social", "full_regression"})
    public void testSocialMenuOptions(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManagerHelper.createNewThrowAwayAccount(OriginClientConstants.COUNTRY_USA);

        logFlowPoint("Login to Origin"); //1
        logFlowPoint("Click on the 'Friend' tab on the upper dropdown menu and verify there is no 'Show Friends List' option but there is an 'Add Friends' option."); //2
        logFlowPoint("Click on the 'Add Friends' option and verify that the user is prompted to search for friends."); //3
        logFlowPoint("Click on the 'View' menu, click 'My Profile', and verify that the user is brought to their profile."); //4

        //1
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin");
        } else {
            logFailExit("Could not log into Origin");
        }

        //2
        MainMenu mainMenu = new MainMenu(driver);
        mainMenu.openFriendsMenu();
        boolean isAddFriendsOptionExists = mainMenu.verifyItemExistsInFriends("Add a Friend…");
        boolean isShowFriendsListOptionExists = mainMenu.verifyItemExistsInFriends("Show Friends List");
        if (isAddFriendsOptionExists && !isShowFriendsListOptionExists) {
            logPass("Verified there is an 'Add Friends' option and no 'Show Friends List' option in the 'Friends' tab");
        } else {
            logFailExit("Failed: Could not verify there is an 'Add Friends' option and/or no 'Show Friends List' option in the 'Friends' tab");
        }

        //3
        mainMenu.selectAddFriend();
        SearchForFriendsDialog searchFriendsDialog = new SearchForFriendsDialog(driver);
        if (searchFriendsDialog.waitIsVisible()) {
            logPass("Successfully opened the 'Search for Friends' dialog");
        } else {
            logFailExit("Failed to open the 'Search for Friends' dialog");
        }
        searchFriendsDialog.closeAndWait();

        //4
        mainMenu.openViewMenu(false);
        mainMenu.selectMyProfile();
        ProfileHeader profileHeader = new ProfileHeader(driver);
        if (profileHeader.verifyOnProfilePage()) {
            logPass("Verified 'Profile' page opens");
        } else {
            logFailExit("Failed: Cannot open the 'Profile' page by clicking 'My Profile' on 'View' menu");
        }

        softAssertAll();
    }
}