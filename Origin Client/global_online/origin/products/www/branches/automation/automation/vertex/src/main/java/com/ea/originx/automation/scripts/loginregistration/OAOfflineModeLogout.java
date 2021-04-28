package com.ea.originx.automation.scripts.loginregistration;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.TakeOverPage;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test to verify offline logout.
 *
 * @author palui
 */
public class OAOfflineModeLogout extends EAXVxTestTemplate {

    @TestRail(caseId = 39456)
    @Test(groups = {"loginregistration", "client_only", "full_regression"})
    public void testOfflineModeLogout(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getRandomAccount();

        logFlowPoint("Launch Origin and login as user: " + userAccount.getUsername()); // 1
        logFlowPoint("Click 'Go Offline' at the Origin menu and verify client is offline"); // 2
        logFlowPoint("Logout of Origin"); // 3
        logFlowPoint("Re-login to Origin as the same user: " + userAccount.getUsername()); // 4
        logFlowPoint("Verify client is now online"); // 5

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified login successful as user: " + userAccount.getUsername());
        } else {
            logFailExit("Failed: Cannot login as user: " + userAccount.getUsername());
        }

        // 2
        MainMenu mainMenu = new MainMenu(driver);
        mainMenu.selectGoOffline();
        sleep(1000); // wait for Origin menu to update
        boolean isMainMenuOffline = mainMenu.verifyItemEnabledInOrigin("Go Online");
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60); // Go to SPA page. Required after checking mainMenu
        DiscoverPage discoverPage = new NavigationSidebar(driver).gotoDiscoverPage();
        boolean isDiscoverPageOffline = discoverPage.verifyOfflineMessageExists();
        if (isMainMenuOffline && isDiscoverPageOffline) {
            logPass("Verified client is offline");
        } else {
            logFailExit("Failed: Client cannot 'Go Offline'");
        }

        // 3
        LoginPage loginPage = new MainMenu(driver).selectLogOut();
        boolean isLoginPage = Waits.pollingWait(() -> loginPage.getCurrentURL().matches(OriginClientData.LOGIN_PAGE_URL_REGEX));
        if (isLoginPage) {
            logPass("Verified logout of Origin successful");
        } else {
            logFailExit("Failed: Cannot logout of Origin");
        }

        // 4
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Verified re-login successful as user: " + userAccount.getUsername());
        } else {
            logFailExit("Failed: Cannot re-login as user: " + userAccount.getUsername());
        }

        // 5
        boolean isMainMenuOnline = mainMenu.verifyItemEnabledInOrigin("Go Offline");
        discoverPage = new NavigationSidebar(driver).gotoDiscoverPage();
        new TakeOverPage(driver).closeIfOpen();
        boolean isDiscoverPageOnline = discoverPage.verifyDiscoverPageReached();
        if (isMainMenuOnline && isDiscoverPageOnline) {
            logPass("Verified client is online after re-login");
        } else {
            logFailExit("Failed: Client is not online after re-login");
        }

        softAssertAll();
    }
}