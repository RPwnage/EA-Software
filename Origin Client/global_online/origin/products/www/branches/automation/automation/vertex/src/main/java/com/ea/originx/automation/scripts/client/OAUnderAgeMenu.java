package com.ea.originx.automation.scripts.client;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.login.LoginOptions;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.login.SecurityQuestionAnswerPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.lib.pageobjects.social.SocialHubMinimized;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the 'Origin Menu' for underage users.
 *
 * @author mkalaivanan, mdobre
 */
public class OAUnderAgeMenu extends EAXVxTestTemplate {

    @TestRail(caseId = 569303)
    @Test(groups = {"client_only", "full_regression", "long_smoke", "allfeaturesmoke"})
    public void testUnderAgeMenu(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManagerHelper.createNewThrowAwayAccount(12);

        logFlowPoint("Create an underage account and login to Origin."); // 1
        logFlowPoint("Verify 'Game Library' is in the Navigation Sidebar."); //2
        logFlowPoint("Verify 'Social Hub' does not exist."); //3
        logFlowPoint("Verify an underage account does not have an avatar."); //4
        logFlowPoint("Verify 'EA Account and Billing' and 'Order History' are not in the 'Origin Menu'."); // 5
        logFlowPoint("Verify 'Add From the Vault' is not in the 'Games Menu'."); // 6
        logFlowPoint("Verify 'Friends' does not exist in menu options."); // 7
        logFlowPoint("Verify 'Reload my Games' is in the 'Games Menu'."); // 8
        logFlowPoint("Verify 'Game Library', 'Download Queue', and 'Zoom' are in the 'View Menu'."); // 9
        logFlowPoint("Verify 'My Home', 'Store', 'Friends List', 'Active Chat', 'My Profile', 'Achievements', "
                + "'Wishlist', and 'Search' are missing from the 'View Menu'."); // 10
        logFlowPoint("Verify 'Origin Error Reporter', 'About', and 'Shortcuts' are in the 'Help Menu'."); // 11
        logFlowPoint("Verify the user can log out through the 'Origin Menu'."); // 12
        logFlowPoint("Log back into Origin with the underage account and verify the user can exit through the 'Origin Menu'."); // 13

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin with an underage account.");
        } else {
            logFailExit("Failed to log into Origin with an underage account.");
        }

        //2
        if (new NavigationSidebar(driver).verifyGameLibraryString()) {
            logPass("Successfully verified 'Game Library' is in the navigation sidebar.");
        } else {
            logFailExit("Failed to verify 'Game Library' is in the navigation sidebar.");
        }

        //3
        SocialHub socialHub = new SocialHub(driver);
        SocialHubMinimized socialHubMinimized = new SocialHubMinimized(driver);
        boolean socialHubNotVisible = !socialHub.verifySocialHubVisible() && !socialHubMinimized.verifyMinimized();
        if (socialHubNotVisible) {
            logPass("Successfully verified the Social Hub does not exist for an underage user.");
        } else {
            logFail("Failed to verify the Social Hub does not exist for an underage user.");
        }

        //4
        MiniProfile miniProfile = new MiniProfile(driver);
        if (!miniProfile.verifyAvatarVisible()) {
            logPass("Successfully verified an underage user does not have a visible avatar.");
        } else {
            logFail("Failed to verify an underage user does not have a visible avatar.");
        }

        // 5
        MainMenu mainMenu = new MainMenu(driver);
        boolean isAccountBillingInOriginMenu = mainMenu.verifyItemExistsInOrigin("EA Account and Billing");
        boolean isOrderHistoryInMenu = mainMenu.verifyItemExistsInOrigin("Order History");

        if (!isAccountBillingInOriginMenu && !isOrderHistoryInMenu) {
            logPass("Verified 'EA Account and Billing' and 'Order History' are not in the 'Origin Menu'.");
        } else {
            logFail("Failed to verify 'EA Account and Billing' and 'Order History' are not in the 'Origin Menu'.");
        }

        // 6
        boolean isAddFromVaultInGamesMenu = mainMenu.verifyItemExistsInGames("Add From the Vault");
        if (!isAddFromVaultInGamesMenu) {
            logPass("Verified 'Add From the Vault' is not in the 'Games Menu'.");
        } else {
            logFail("Failed to verify 'Add from the Vault' is not in the 'Games Menu'.");
        }

        // 7
        boolean isFriendsInMenu = !mainMenu.verifyMenuExists("Friends");
        if (isFriendsInMenu) {
            logPass("Verified 'Friends' is not in the 'Origin Menu'.");
        } else {
            logFail("Failed to verify 'Friends' is not in the 'Origin Menu'.");
        }

        // 8
        boolean isReloadGameLibraryInGamesMenu = mainMenu.verifyItemExistsInGames("Reload Game Library");
        if (isReloadGameLibraryInGamesMenu) {
            logPass("Verified 'Reload Game Library' is in the 'Games Menu'.");
        } else {
            logFail("Failed to verify 'Reload Game Library' is in the 'Games Menu'.");
        }

        // 9
        boolean isGameLibraryInViewMenu = mainMenu.verifyItemExistsInView("Game Library");
        boolean isDownloadQueueInViewMenu = mainMenu.verifyItemExistsInView("Download Queue");
        boolean isZoomInViewMenu = mainMenu.verifyItemExistsInView("Zoom");
        if (isGameLibraryInViewMenu && isDownloadQueueInViewMenu && isZoomInViewMenu) {
            logPass("Verified 'Game Library', 'Download Queue', and 'Zoom' are in the 'View Menu'.");
        } else {
            logFail("Failed to verify 'Game Library', 'Download Queue', and 'Zoom' are in the 'View Menu'.");
        }

        // 10
        boolean isMyHomeInViewMenu = mainMenu.verifyItemExistsInView("My Home");
        boolean isStoreInViewMenu = mainMenu.verifyItemExistsInView("Store");
        boolean isFriendsListInViewMenu = mainMenu.verifyItemExistsInView("Friends List");
        boolean isActiveChatInViewMenu = mainMenu.verifyItemExistsInView("Active Chat");
        boolean isMyProfileInViewMenu = mainMenu.verifyItemExistsInView("My Profile");
        boolean isAchievementsInViewMenu = mainMenu.verifyItemExistsInView("Achievements");
        boolean isWishlistInViewMenu = mainMenu.verifyItemExistsInView("Wishlist");
        boolean isSearchInViewMenu = mainMenu.verifyItemExistsInView("Search");

        if (!isMyHomeInViewMenu && !isStoreInViewMenu && !isFriendsListInViewMenu && !isActiveChatInViewMenu
                && !isMyProfileInViewMenu && !isAchievementsInViewMenu && !isWishlistInViewMenu && !isSearchInViewMenu) {
            logPass("Verified 'My Home', 'Store', 'Friends List', 'Active Chat', 'My Profile', 'Achievements', "
                    + "'Wishlist', and 'Search' are in the 'View Menu'.");
        } else {
            logFail("Failed to verify 'My Home', 'Store', 'Friends List', 'Active Chat', 'My Profile', 'Achievements', "
                    + "'Wishlist', and 'Search' are in the 'View Menu'.");
        }

        // 11
        boolean isAboutDialogInHelpMenu = mainMenu.verifyItemExistsInHelp("About");
        boolean isOriginErrorReporterInHelpMenu = mainMenu.verifyItemExistsInHelp("Origin Error Reporter");
        boolean verifyShortcutExistsInHelpMenu = mainMenu.verifyItemExistsInHelp("Shortcuts");
        if (isAboutDialogInHelpMenu && isOriginErrorReporterInHelpMenu && verifyShortcutExistsInHelpMenu) {
            logPass("Verified 'Origin Error Reporter', 'About', and 'Shortcuts' are in the 'Help Menu'.");
        } else {
            logFail("Failed to verify 'Origin Error Reporter', 'About', and 'Shortcuts' are in the 'Help Menu'.");
        }

        // 12
        LoginPage loginPageAfterLogout = mainMenu.selectLogOut(true);
        if (loginPageAfterLogout != null) {
            logPass("Verified user logged out successfully through the 'Origin Menu'.");
        } else {
            logFailExit("Failed to verify user was able to log out successfully through the 'Origin Menu'.");
        }

        // 13
        MacroLogin.startLogin(driver, userAccount, new LoginOptions().setHandleUnderAgeFirstLogin(true), "", "", "", false, SecurityQuestionAnswerPage.DEFAULT_PRIVACY_VISIBLITY_SETTING);
        Waits.sleep(4000);
        mainMenu.selectExit(true);
        boolean isExitThroughOriginMenuSuccessful = Waits.pollingWaitEx(() -> !ProcessUtil.isProcessRunning(client, "Origin.exe"));

        if (isExitThroughOriginMenuSuccessful) {
            logPass("Verified user successfully exited from the 'Origin Menu'.");
        } else {
            logFailExit("Failed to verify user was able to exit from the 'Origin Menu'.");
        }

        softAssertAll();
    }
}
