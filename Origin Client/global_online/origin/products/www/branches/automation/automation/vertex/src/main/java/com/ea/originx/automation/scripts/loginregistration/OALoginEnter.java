package com.ea.originx.automation.scripts.loginregistration;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.pageobjects.login.LoginOptions;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.TimeoutException;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Verifies that the user can log into Origin by pressing enter
 *
 * @author JMittertreiner
 */
public class OALoginEnter extends EAXVxTestTemplate {

    private boolean waitForLoginComplete(WebDriver driver) {
        try {
            Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 15);
            return true;
        } catch (TimeoutException te) {
            return false;
        }
    }

    @TestRail(caseId = 39656)
    @Test(groups = {"loginregistration", "full_regression"})
    public void testLoginEnter(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getRandomAccount();
        final String userEmail = userAccount.getEmail();
        final String password = userAccount.getPassword();

        logFlowPoint("Launch Origin"); // 1
        logFlowPoint("Verify the user can log into Origin by pressing enter"); // 2

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (driver != null) {
            logPass("Successfully launched Origin");
        } else {
            logFail("Failed to launch Origin");
        }

        // 2
        final LoginPage loginPage = new LoginPage(driver);
        loginPage.waitForPageToLoad();

        loginPage.quickLogin(userEmail, password, new LoginOptions().setUseEnter(true));

        if (waitForLoginComplete(driver)) {
            logPass("Successfully logged in with return");
        } else {
            logFail("Failed to log in with with return");
        }

        softAssertAll();
    }
}
