package com.ea.originx.automation.scripts.loginregistration;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.login.LoginOptions;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Verifies that the user can log into Origin with different 'methods' - by
 * pressing 'Enter', by pressing the 'Sign in' button, and for accounts with
 * special character email
 *
 * @author palui
 */
public class OALoginMethods extends EAXVxTestTemplate {

    @TestRail(caseId = 9471)
    @Test(groups = {"loginregistration", "client_only", "full_regression"})
    public void testLoginMethods(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        // Random user
        final UserAccount userAccount = AccountManager.getRandomAccount();
        final String username = userAccount.getUsername();
        final String password = userAccount.getPassword();

        // User with email having special characters such as .~}|{`+^?=*$
        final String specialCharEmail = AccountManagerHelper.getTaggedUserAccount(AccountTags.SPECIAL_CHARS_EMAIL).getEmail();

        logFlowPoint(String.format("Launch Origin and login as %s by pressing 'Enter'", username)); //1
        logFlowPoint("Logout"); //2
        logFlowPoint(String.format("Login again as %s by pressing 'Sign in' button", username)); //3
        logFlowPoint("Logout again"); //4
        logFlowPoint(String.format("Login as %s (email with sepcial characters)", specialCharEmail)); //5

        //1
        final WebDriver driver = startClientObject(context, client);
        LoginPage loginPage = new LoginPage(driver);
        loginPage.quickLogin(username, password, new LoginOptions().setUseEnter(true));
        if (Waits.waitIsPageThatMatchesOpen(driver, OriginClientData.MAIN_SPA_PAGE_URL, 15)) {
            logPass(String.format("Verified login as %s by pressing 'Enter' successful", username));
        } else {
            logFailExit(String.format("Failed: Cannot login as %s by pressing 'Enter'", username));
        }

        //2
        new MainMenu(driver).selectLogOut();
        loginPage = new LoginPage(driver);
        if (Waits.waitIsPageThatMatchesOpen(driver, OriginClientData.LOGIN_PAGE_URL_REGEX, 15)) {
            logPass("Verified logout successful and the Login page re-appears");
        } else {
            logFailExit("Failed: Cannot logout from Origin or the Login page does not re-appear");
        }

        //3
        loginPage.quickLogin(username, password);
        if (Waits.waitIsPageThatMatchesOpen(driver, OriginClientData.MAIN_SPA_PAGE_URL, 15)) {
            logPass(String.format("Verified login as %s by pressing 'Sign in' button successful", username));
        } else {
            logFailExit(String.format("Failed: Cannot login as %s by pressing 'Sign in' button", username));
        }

        //4
        new MainMenu(driver).selectLogOut();
        loginPage = new LoginPage(driver);
        if (Waits.waitIsPageThatMatchesOpen(driver, OriginClientData.LOGIN_PAGE_URL_REGEX, 15)) {
            logPass("Verified logout successful and Login page re-appears");
        } else {
            logFailExit("Failed: Cannot logout from Origin or Login page does not re-appear");
        }

        //5
        loginPage.quickLogin(specialCharEmail, password);
        if (Waits.waitIsPageThatMatchesOpen(driver, OriginClientData.MAIN_SPA_PAGE_URL, 15)) {
            logPass(String.format("Verified login using email %s with special characters successful", specialCharEmail));
        } else {
            logFailExit(String.format("Failed: Cannot login using email %s with special characters", specialCharEmail));
        }

        softAssertAll();
    }
}
