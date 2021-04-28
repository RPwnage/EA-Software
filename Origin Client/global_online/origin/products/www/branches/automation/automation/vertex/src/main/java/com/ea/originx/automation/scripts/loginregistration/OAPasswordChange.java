package com.ea.originx.automation.scripts.loginregistration;

import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.SecurityPage;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.webdrivers.BrowserType;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;

/**
 * Test for password change. Used by the following parameterized test cases:<br>
 * - OAChangePassword <br>
 * - OAOfflineModeChangePassword
 *
 * @author jmitterteiner, palui
 */
public abstract class OAPasswordChange extends EAXVxTestTemplate {

    private WebDriver browserDriver;

    /**
     * Test method supporting parameterized test cases for the Password Change
     *
     * @param context     context used
     * @param testOffline true offline mode tests, false otherwise
     * @param testName    name of the test script for initLogger to properly
     *                    attaches to the CRS test report
     * @throws Exception
     */
    public void testPasswordChange(ITestContext context, boolean testOffline, String testName) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final boolean isClient = ContextHelper.isOriginClientTesing(context);

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final String oldPassword = userAccount.getPassword();
        final String newPassword = userAccount.getPassword() + "New";
        final String username = userAccount.getUsername();

        logFlowPoint("Launch Origin client and login as user: " + username); //1
        if (testOffline) {
            logFlowPoint("Go offline at the Origin Client"); //2a
        }
        logFlowPoint("From a browser, go to Origin EA Account and Billing page"); //3
        logFlowPoint("Change the password of the user account to " + newPassword); //4
        if (testOffline) {
            logFlowPoint("Go online at the Origin client. Verify that Origin automatically logs the user out"); //5a
        } else if (!isClient) {
            logFlowPoint("Log out of Origin and navigate back to login screen"); //5b
        } else {
            logFlowPoint("Verify that Origin client automatically logs the user out"); //5c
        }
        logFlowPoint("Verify that user cannot login to Origin client with the old password"); //6
        logFlowPoint("Verify that user can re-login to Origin client with the new password"); //7

        //1
        WebDriver originDriver = startClientObject(context, client);
        if (MacroLogin.startLogin(originDriver, userAccount)) {
            logPass("Verified login to Origin client successful as user: " + username);
        } else {
            logFailExit("Failed: Cannot launch Origin client or login as user: " + username);
        }

        //2a
        MainMenu mainMenu = new MainMenu(originDriver);
        if (testOffline) {
            mainMenu.selectGoOffline();
            sleep(1000); // wait for Origin menu to update
            if (mainMenu.verifyItemEnabledInOrigin("Go Online")) {
                logPass("Verified client is offline");
            } else {
                logFailExit("Failed: Client cannot 'Go Offline'");
            }
        }

        //3
        browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        final AccountSettingsPage accountSettingsPage = new AccountSettingsPage(browserDriver);
        accountSettingsPage.navigateToIndexPage();
        accountSettingsPage.login(userAccount);
        if (accountSettingsPage.verifyAccountSettingsPageReached()) {
            logPass("Verified Origin EA Account and Billing page opens at a browser");
        } else {
            logFailExit("Failed: Cannot open Origin EA Account and Billing page at a browser");
        }

        //4
        accountSettingsPage.gotoSecuritySection();
        SecurityPage securityPage = new SecurityPage(browserDriver);
        securityPage.changePassword(userAccount, newPassword);
        if (securityPage.verifySecuritySectionReached()) {
            logPass("Verified user password sucessfully changed to: " + newPassword);
        } else {
            logFailExit("Failed: Cannot change password for user: " + username);
        }

        //5
        sleep(10000L);
        LoginPage loginPage = new LoginPage(originDriver);
        if (testOffline) {
            //5a
            mainMenu.selectGoOnline();
        } else if (!isClient) {
            //5b
            new MiniProfile(originDriver).selectSignOut();
            loginPage.clickBrowserLoginBtn();
        }

        //5c
        boolean isLoginPage = Waits.pollingWait(() -> loginPage.isPageThatMatchesOpen(OriginClientData.LOGIN_PAGE_URL_REGEX));
        if (isLoginPage) {
            logPass("Verified the login screen appears");
        } else {
            logFailExit("Failed: The login screen does not appear");
        }

        //6
        String loginName = isClient ? userAccount.getUsername() : userAccount.getEmail();
        loginPage.enterInformation(loginName, oldPassword);
        if (loginPage.verifyInvalidCredentialsBanner()) {
            logPass(String.format("Verified user %s cannot re-login to Origin client with old password: %s", username, oldPassword));
        } else {
            logFailExit(String.format("Failed: User %s successfully re-login to Origin client with old password: %s", username, oldPassword));
        }

        //7
        loginPage.enterInformation(loginName, newPassword);
        loginPage.waitForDiscoverPage();
        if (new MiniProfile(originDriver).verifyUser(userAccount.getUsername())) {
            logPass(String.format("Verified user %s successfully re-login to Origin client with new password: %s", username, newPassword));
        } else {
            logFailExit(String.format("Failed: User %s cannot re-login to Origin client with new password: %s", username, newPassword));
        }

        softAssertAll();
    }
}
