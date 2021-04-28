package com.ea.originx.automation.scripts.loginregistration;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.login.CountryDoBRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.account.AccountManager;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that signup cannot proceed unless the ToS (Terms of Service) box is
 * checked
 *
 * @author JMittertreiner
 */
public class OARegisterToS extends EAXVxTestTemplate {

    @TestRail(caseId = 1629666)
    @Test(groups = {"loginregistration", "full_regression"})
    public void testRegisterToS(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();

        logFlowPoint("Launch Origin and begin creating a new account"); // 1
        logFlowPoint("Continue creating the account but do not check the ToS box, verify that the next button is disabled"); // 2
        logFlowPoint("Check the ToS box and verify that the next button is enabled"); // 3

        // 1
        final WebDriver driver = startClientObject(context, client);
        LoginPage loginPage = new LoginPage(driver);
        loginPage.clickRegistrationLink();

        CountryDoBRegisterPage registerPage = new CountryDoBRegisterPage(driver);
        if (registerPage.verifyCountryInputVisible()) {
            logPass("Successfully started Origin client and started creating an account");
        } else {
            logFailExit("Could not launch Origin and start creating an account");
        }

        // 2
        registerPage.inputUserDOBRegistrationDetails(userAccount);
        if (registerPage.verifyContinueButtonIsDisabled()) {
            logPass("Successfully verified that the next button is disabled");
        } else {
            logFail("The next button is not disabled");
        }

        // 3
        registerPage.acceptToS();
        if (!registerPage.verifyContinueButtonIsDisabled()) {
            logPass("Successfully verified that the next button is enabled");
        } else {
            logFail("The next button is not enabled");
        }

        softAssertAll();
    }
}
