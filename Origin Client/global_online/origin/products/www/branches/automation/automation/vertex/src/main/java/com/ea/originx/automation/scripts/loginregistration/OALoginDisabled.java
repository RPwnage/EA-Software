package com.ea.originx.automation.scripts.loginregistration;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Verifies user cannot login without both username and password entered
 *
 * @author rmcneil
 */
public class OALoginDisabled extends EAXVxTestTemplate {

    @TestRail(caseId = 1594440)
    @Test(groups = {"loginregistration", "full_regression"})
    public void testLoginDisabled(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount user = AccountManager.getRandomAccount();

        logFlowPoint("Verify user cannot login if only a username is entered"); // 1
        logFlowPoint("Verify user cannot login if only a password is entered"); // 2
        logFlowPoint("Verify user cannot login if neither a username nor password is entered"); // 3

        // 1
        WebDriver driver = startClientObject(context, client);
        LoginPage loginPage = initPage(driver, context, LoginPage.class);
        if (!OriginClient.getInstance(driver).isQtClient(driver)) {
            loginPage.clickBrowserLoginBtn();

        }
        loginPage.enterInformation(user.getUsername(), "");
        if (loginPage.verifyInvalidCredentialsBanner()) {
            logPass("Login failed when only a username was entered");
        } else {
            logFailExit("Login succeeded when only a username was entered");
        }

        // 2
        loginPage.clearLoginForm();
        loginPage.enterInformation("", user.getPassword());
        if (loginPage.verifyInvalidCredentialsBanner()) {
            logPass("Login failed when only a password was entered");
        } else {
            logFailExit("Login succeeded when only a password was entered");
        }

        // 3
        loginPage.clearLoginForm();
        loginPage.enterInformation("", "");
        if (loginPage.verifyInvalidCredentialsBanner()) {
            logPass("Login failed when all fields were cleared");
        } else {
            logFailExit("Login succeeded when all fields were cleared");
        }

        softAssertAll();
    }
}
