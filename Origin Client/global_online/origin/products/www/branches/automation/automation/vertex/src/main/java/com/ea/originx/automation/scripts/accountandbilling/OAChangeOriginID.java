package com.ea.originx.automation.scripts.accountandbilling;

import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.account.AboutMePage;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.utils.Waits;

/**
 * Tests that a user can change their id and still log in to Origin
 *
 * @author jmitterteiner
 */
public class OAChangeOriginID extends EAXVxTestTemplate {

    @TestRail(caseId = 10099)
    @Test(groups = {"accountandbilling", "client_only", "full_regression"})
    public void testChangePassword(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();

        // We need a unique user id that hasn't been taking. Request a new account but don't actually create it.
        // Just use it to get a unique id
        sleep(1); // Sleep at least one millisecond so we get a new timestamp for the username
        UserAccount tempAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final String newId = tempAccount.getUsername();

        logFlowPoint("Login to Origin with " + userAccount.getUsername()); // 1
        logFlowPoint("Open the Account Settings Page on the browser and login"); // 2
        logFlowPoint("Change the user's Origin ID and press Save"); // 3
        logFlowPoint("Verify: The user ID has changed on the Account Settings Page"); // 4
        logFlowPoint("Logout of Origin"); // 5
        logFlowPoint("Verify: The user is unable to login with the old ID (" + userAccount.getUsername() + ")"); // 6
        logFlowPoint("Verify: The user is able to login with the new ID (" + tempAccount.getUsername() + ")"); // 7

        // 1
        final WebDriver clientDriver = startClientObject(context, client);
        if (MacroLogin.startLogin(clientDriver, userAccount)) {
            logPass("Successfully logged in to Origin as " + userAccount.getUsername());
        } else {
            logFailExit("Failed to log in to Origin as " + userAccount.getUsername());
        }

        final WebDriver browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        final AccountSettingsPage accountSettingsPage = new AccountSettingsPage(browserDriver);

        // 2
        accountSettingsPage.navigateToIndexPage();
        accountSettingsPage.login(userAccount);
        if (accountSettingsPage.verifyAccountSettingsPageReached()) {
            logPass("Opened the EA Account and Billing from Origin and login");
        } else {
            logFailExit("Failed to open the EA Account and Billing page from Origin");
        }

        // 3
        AboutMePage aboutMePage = new AboutMePage(browserDriver);
        aboutMePage.changeOriginId(newId);
        if (accountSettingsPage.verifyAccountSettingsPageReached()) {
            logPass("Changed the Origin ID of the user account to " + newId);
        } else {
            logFailExit("Failed to change the Origin ID of the user account");
        }

        // 4
        if (Waits.pollingWait(() -> aboutMePage.verifyOriginId(newId))) {
            logPass("Verified the user ID has changed to " + newId);
        } else {
            logFail("The user ID has not changed to " + newId);
        }

        // 5
        LoginPage loginPage = new MainMenu(clientDriver).selectLogOut();
        final boolean isLoginPage = Waits.pollingWait(() -> loginPage.getCurrentURL().matches(OriginClientData.LOGIN_PAGE_URL_REGEX));
        if (isLoginPage) {
            logPass("Logged out of Origin successfully");
        } else {
            logFailExit("Failed to logout of Origin");
        }

        // 6
        loginPage.enterInformation(userAccount.getUsername(), userAccount.getPassword());
        if (Waits.pollingWait(loginPage::isOnLoginPage, 2000, 500, 0)) {
            logPass("Could not log in with the old user id: " + userAccount.getUsername());
        } else {
            logFailExit("Could login with the old user id: " + userAccount.getUsername());
        }

        // 7
        loginPage.quickLogin(newId, userAccount.getPassword());
        if (new MiniProfile(clientDriver).verifyUser(newId)) {
            logPass("Successfully logged in to Origin with the new id: " + newId);
        } else {
            logFail("Failed to log in to Origin with the new id: " + newId);
        }

        softAssertAll();
    }
}
