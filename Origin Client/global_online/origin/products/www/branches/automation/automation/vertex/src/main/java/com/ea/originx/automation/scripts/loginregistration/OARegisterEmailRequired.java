package com.ea.originx.automation.scripts.loginregistration;

import com.ea.originx.automation.lib.pageobjects.login.BasicInfoRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.CountryDoBRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.templates.OATestBase;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test the email requirements during registration
 *
 * @author rocky
 */
public class OARegisterEmailRequired extends OATestBase {

    @TestRail(caseId = 9429)
    @Test(groups = {"full_regresssion", "loginregistration"})
    public void testRegisterEmailRequired(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);
        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        // for testing duplicate email address
        final UserAccount duplicateEmailUserAccount = AccountManager.getInstance().registerNewThrowAwayAccountThroughREST();
        final String duplicateEmailAddress = duplicateEmailUserAccount.getEmail();

        final String emailWithInvalidFormat = "something@@not.real";
        final String emailWithNumbers = "zakdiqe23929123131@gmail.com";

        logFlowPoint("Locate to Login page for origin and click create an account and enter a country, date of birth and check the TOS/EULA checkbox"); // 1
        logFlowPoint("Click next button and verify duplicate email address message by entering already registered email address"); // 2
        logFlowPoint("Verify email invalid message appears when entering email address with invalid format " + emailWithInvalidFormat); // 3
        logFlowPoint("Verify email invalid message does not appear when entering email address with numbers " + emailWithNumbers); // 4

        // 1
        final WebDriver driver = startClientObject(context, client);
        LoginPage loginPage = new LoginPage(driver);
        CountryDoBRegisterPage countryDoBRegisterPage = loginPage.clickRegistrationLink();
        countryDoBRegisterPage.enterDoB(userAccount.getBirthMonth(), userAccount.getBirthDay(), userAccount.getBirthYear());
        countryDoBRegisterPage.acceptToS();
        if (!countryDoBRegisterPage.verifyContinueButtonIsDisabled()) {
            logPass("Locate to Login page and successfully entered a country, Date of Birth and checked the TOS/EULA checkbox");
        } else {
            logFail("Failed to enter a country, Date of Birth, and/or checking the TOS/EULA checkbox");
        }

        // 2
        countryDoBRegisterPage.clickOnCountryDOBNextButton();
        BasicInfoRegisterPage basicInfoRegisterPage = new BasicInfoRegisterPage(driver);
        basicInfoRegisterPage.waitForBasicInfoPage();
        basicInfoRegisterPage.inputEmailId(duplicateEmailUserAccount.getEmail());
        // remove the focus from the email text field so that the error message can appear
        basicInfoRegisterPage.inputPassword(" ");
        if (basicInfoRegisterPage.verifyDuplicateEmailMessageVisible()) {
            logPass("Clicked next button and verified duplicate email address message appears when entering already registered email address " + duplicateEmailAddress);
        } else {
            logFail("Failed: The already registered email address message did not appear when entering already registered email address " + duplicateEmailAddress);
        }

        // 3
        basicInfoRegisterPage.inputEmailId(emailWithInvalidFormat);
        basicInfoRegisterPage.inputPassword(" ");
        if (basicInfoRegisterPage.verifyEmailInvalidMessageVisible()) {
            logPass("Verified The invalid email address message appears when entering email address with invalid format " + emailWithInvalidFormat);
        } else {
            logFail("Failed: The invalid email address message did not appear after entering email address with invalid format " + emailWithInvalidFormat);
        }

        // 4
        basicInfoRegisterPage.inputEmailId(emailWithNumbers);
        basicInfoRegisterPage.inputPassword(" ");
        if (!basicInfoRegisterPage.verifyEmailInvalidMessageVisible()) {
            logPass("Verified the invalid email address message does not appear when entering  email address with numbers " + emailWithNumbers);
        } else {
            logFail("Failed: The invalid email address message appeared after entering email address with numbers " + emailWithNumbers);
        }

        softAssertAll();
    }
}
