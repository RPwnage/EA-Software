package com.ea.originx.automation.scripts.profile;

import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileHeader;
import com.ea.originx.automation.lib.pageobjects.profile.ProfileOfflinePage;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests 'Offline Mode' on the 'Profile' page.
 *
 * @author jmittertreiner
 */
public class OAProfileOfflineMode extends EAXVxTestTemplate {

    @TestRail(caseId = 39461)
    @Test(groups = {"client_only", "full_regression"})
    public void testManualOfflineMode(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getRandomAccount();

        logFlowPoint("Launch and log into Origin."); // 1
        logFlowPoint("Navigate to the user's profile page."); // 2
        logFlowPoint("Switch to 'Offline Mode' through the Origin menu and verify the user is brought to the 'Not Connected' page."); // 3
        logFlowPoint("Click 'Go Online' and verify the profile page reloads when the client returns to online state."); // 4

        // 1
        final WebDriver driver = startClientObject(context, client);
        initPage(driver, context, LoginPage.class);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully logged into Origin.");
        } else {
            logFailExit("Could not log into Origin.");
        }

        // 2
        new MiniProfile(driver).selectViewMyProfile();
        ProfileHeader profileHeader = new ProfileHeader(driver);
        if (profileHeader.verifyOnProfilePage()) {
            logPass("Successfully navigated to user's profile page.");
        } else {
            logFailExit("Failed to navigate to user's profile page.");
        }

        // 3
        new MainMenu(driver).selectGoOffline();
        ProfileOfflinePage profileOfflinePage = new ProfileOfflinePage(driver);
        if (profileOfflinePage.verifyOnOfflinePage()) {
            logPass("Verified the user is brought to the 'Not Connected' page after switching to 'Offline Mode'.");
        } else {
            logFailExit("Failed to verify the user is brought to the 'Not Connected' page after switching to 'Offline Mode'.");
        }

        // 4
        profileOfflinePage.clickGoOnline();
        if (profileHeader.verifyOnProfilePage()) {
            logPass("Verified the profile page reloads when the client returns to 'Online Mode'.");
        } else {
            logFailExit("Failed to verify the profile page reloads when the client returns to 'Online Mode'.");
        }

        softAssertAll();
    }
}