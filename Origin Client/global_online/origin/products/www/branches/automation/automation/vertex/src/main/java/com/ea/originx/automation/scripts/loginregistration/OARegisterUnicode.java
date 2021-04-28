package com.ea.originx.automation.scripts.loginregistration;

import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.login.BasicInfoRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.CountryDoBRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that non-ASCII characters are not allowed when registering a new user
 *
 * @author lscholte
 */
public class OARegisterUnicode extends EAXVxTestTemplate {

    @TestRail(caseId = 9430)
    @Test(groups = {"loginregistration", "full_regression"})
    public void testRegisterUnicode(ITestContext context) throws Exception {
        final OriginClient client = OriginClientFactory.create(context);
        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        //It's not clear which characters are allowed or disallowed in the
        //email, password, and public ID fields. In fact, the allowed characters
        //seem to be different for each field. The only thing that is known for
        //sure is that non-ASCII characters are not allowed
        String invalidEmail = "test\u2122email@gmail.com";
        String invalidPublicId = "testOrigin\u00A3ID";
        String invalidPassword = "QA\u00B1t3st3r";

        logFlowPoint("Launch Origin and Begin Registration"); // 1
        logFlowPoint("Verify that " + invalidEmail + " cannot be used as an email due to an invalid character"); // 2
        logFlowPoint("Verify that " + invalidPassword + " cannot be used as a password due to an invalid character"); // 3
        logFlowPoint("Verify that " + invalidPublicId + " cannot be used as a public ID due to an invalid character"); // 4

        // 1
        final WebDriver driver = startClientObject(context, client);
        LoginPage loginPage = new LoginPage(driver);
        loginPage.clickRegistrationLink();
        CountryDoBRegisterPage countryDoBRegisterPage = new CountryDoBRegisterPage(driver);
        if (countryDoBRegisterPage.verifyCountryInputVisible()) {
            logPass("Origin Launched and Started Registration");
        } else {
            logFailExit("User registration did not start");
        }

        // 2
        countryDoBRegisterPage.enterDoBCountryInformation(userAccount);
        BasicInfoRegisterPage basicInfoPage = new BasicInfoRegisterPage(driver);
        basicInfoPage.waitForBasicInfoPage();
        basicInfoPage.inputEmailId(invalidEmail);
        basicInfoPage.inputCaptcha("");
        if (basicInfoPage.verifyEmailInvalidMessageVisible()) {
            logPass("An error message appeared after entering in the email " + invalidEmail);
        } else {
            logFail("An error message did not appear after entering in the email " + invalidEmail);
        }

        // 3 
        basicInfoPage.inputPassword(invalidPassword);
        basicInfoPage.inputCaptcha("");
        if (basicInfoPage.verifyPasswordInvalidCharacterMessageVisible()) {
            logPass("An error message appeared after entering in the password " + invalidPassword);
        } else {
            logFail("An error message did not appear after entering in the password " + invalidPassword);
        }

        // 4 
        basicInfoPage.inputPublicId(invalidPublicId);
        basicInfoPage.inputCaptcha("");
        if (basicInfoPage.verifyPublicIdInvalidMessageVisible()) {
            logPass("An error message appeared after entering the a public ID " + invalidPublicId);
        } else {
            logFail("An error message did not appear after entering in the public ID " + invalidPublicId);
        }

        softAssertAll();
    }
}
