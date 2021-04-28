package com.ea.originx.automation.scripts.loginregistration;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.resources.AccountTags;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests logging in with an account which does not have access
 *
 * @author JMittertreiner
 */
public class OALoginNoAccessAccount extends EAXVxTestTemplate {

    @TestRail(caseId = 9470)
    @Test(groups = {"loginregistration", "full_regression"})
    public void testOALoginNoAccessAccount(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount deactivatedAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.DEACTIVATED);
        UserAccount deletedAccount = AccountManagerHelper.getTaggedUserAccount(AccountTags.DELETED);

        logFlowPoint("Launch Origin"); // 1
        logFlowPoint("Verify that you cannot log in with a deactivated account: " + deactivatedAccount.getUsername()); // 2
        logFlowPoint("Verify that you cannot log in with a deleted account: " + deletedAccount.getUsername()); // 3

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (driver != null) {
            logPass("Successfully started Origin.");
        } else {
            logFailExit("Could not launch Origin.");
        }

        // 2
        LoginPage loginPage = new LoginPage(driver);
        loginPage.enterInformation(deactivatedAccount.getEmail(), deactivatedAccount.getPassword());
        if (loginPage.isOnLoginPage() && loginPage.verifyInvalidCredentialsBanner()) {
            logPass("Could not login with a deactivated account.");
        } else {
            logFailExit("Logged in with a deactivated account.");
        }

        // 3
        loginPage.enterInformation(deletedAccount.getEmail(), deletedAccount.getPassword());
        if (loginPage.isOnLoginPage() && loginPage.verifyInvalidCredentialsBanner()) {
            logPass("Could not login with a deleted account.");
        } else {
            logFailExit("Logged in with a deleted account.");
        }

        softAssertAll();
    }
}
