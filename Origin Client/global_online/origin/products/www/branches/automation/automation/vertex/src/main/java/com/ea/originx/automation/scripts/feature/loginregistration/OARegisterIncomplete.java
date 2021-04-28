package com.ea.originx.automation.scripts.feature.loginregistration;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.originx.automation.lib.pageobjects.login.BasicInfoRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.CountryDoBRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.login.SecurityQuestionAnswerPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.utils.Waits;
import java.util.Calendar;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that a use is unable to register if they have incorrect or incomplete
 * information
 *
 * @author jmittertreiner
 */
public class OARegisterIncomplete extends EAXVxTestTemplate {

    @TestRail(caseId = 9497)
    @Test(groups = {"loginregistration", "loginregistration_smoke", "allfeaturesmoke", "full_regression"})
    public void testRegisterIncomplete(ITestContext context) throws Exception {

        boolean isQt = ContextHelper.isOriginClientTesing(context);
        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final UserAccount duplicateAccount = AccountManager.getRandomAccount();

        logFlowPoint("Launch Origin."); // 1
        logFlowPoint("Click create an account and check the Privacy, ToS and EULA checkbox and verify the next button is disabled."); // 2
        logFlowPoint("Uncheck the Privacy, ToS and EULA checkbox, enter a DOB and verify the next button is disabled."); // 3
        logFlowPoint("Check the Privacy, ToS and EULA checkbox and click next."); // 4
        logFlowPoint("Enter an invalid email address and verify that there is an error."); // 5
        logFlowPoint("Enter several different passwords (QAt3, t3t3st3rm4n, T3ST3RM4N, QAtester) that don't meet the requirements and verify all 4 have error messages."); // 6
        logFlowPoint("Enter an origin ID that is already in use and verify an error appears."); // 7
        logFlowPoint("Enter all information correctly, click next, and verify the create account button is disabled on the next page."); // 8
        logFlowPoint("Enter a DOB that indicates the user is under age, enter an invalid email address and verify there is an error."); //9
        logFlowPoint("Log in with an account that has a 'weak password' and verify there is an error.");

        // 1
        final WebDriver driver = startClientObject(context, client);
        if (!isQt) {
            new LoginPage(driver).clickBrowserLoginBtn();
        }
        LoginPage loginPage = new LoginPage(driver);
        if (Waits.pollingWait(loginPage::isOnLoginPage)) {
            logPass("Successfully launched Origin.");
        } else {
            logFailExit("Failed to launch Origin.");
        }

        // 2
        CountryDoBRegisterPage countryDoBRegisterPage = loginPage.clickRegistrationLink();
        countryDoBRegisterPage.acceptToS();
        if (countryDoBRegisterPage.verifyContinueButtonIsDisabled()) {
            logPass("The next button is still disabled.");
        } else {
            logFail("The next button is not disabled.");
        }

        // 3
        countryDoBRegisterPage.declineToS();
        countryDoBRegisterPage.enterDoB(userAccount.getBirthMonth(), userAccount.getBirthDay(), userAccount.getBirthYear());
        if (countryDoBRegisterPage.verifyContinueButtonIsDisabled()) {
            logPass("The next button is still disabled.");
        } else {
            logFail("The next button is not disabled.");
        }

        // 4
        countryDoBRegisterPage.acceptToS();
        countryDoBRegisterPage.clickOnCountryDOBNextButton();
        BasicInfoRegisterPage basicInfoRegisterPage = new BasicInfoRegisterPage(driver);
        basicInfoRegisterPage.waitForBasicInfoPage();
        if (basicInfoRegisterPage.verifyBasicInfoPageReached()) {
            logPass("Proceeded to the basic info page.");
        } else {
            logFailExit("Failed to proceed to the basic info page.");
        }

        // 5
        basicInfoRegisterPage.inputEmailId("FAKE_EMAIL");
        basicInfoRegisterPage.inputPassword("t"); // Trigger the email error
        if (basicInfoRegisterPage.verifyEmailInvalidMessageVisible()) {
            logPass("The invalid email address message appeared.");
        } else {
            logFail("The invalid email address message did not appear.");
        }

        // 6
        String[] passwords = {"QAt3", "t3t3st3rm4n", "T3ST3RM4N", "QAtester"};
        if (basicInfoRegisterPage.verifyAllPasswordsAreInvalid(passwords)) {
            logPass("All passwords triggered the invalid password message.");
        } else {
            logFailExit("One of the passwords did not trigger the invalid password message.");
        }
        
        // 7
        basicInfoRegisterPage.inputPublicId(duplicateAccount.getUsername());
        basicInfoRegisterPage.inputEmailId("t"); // Trigger the id error
        if (basicInfoRegisterPage.verifyPublicIdAlreadyInUseMessageVisible()) {
            logPass("The public id in use message appeared.");
        } else {
            logFail("The public id in use message did not appear.");
        }

        // 8
        basicInfoRegisterPage.inputUserAccountRegistrationDetailsWithRetry(userAccount);
        basicInfoRegisterPage.clickOnBasicInfoNextButton();
        SecurityQuestionAnswerPage securityQuestionAnswerPage = new SecurityQuestionAnswerPage(driver);
        securityQuestionAnswerPage.waitForPage();
        if (!securityQuestionAnswerPage.isCreateAccountEnabled()) {
            logPass("The create account button is disabled.");
        } else {
            logFail("The create account button is not disabled.");
        }
        
        // 9
        securityQuestionAnswerPage.goBack();
        basicInfoRegisterPage.goBack();
        int year = Calendar.getInstance().get(Calendar.YEAR) - 10;
        countryDoBRegisterPage.enterDoB("January", "1", Integer.toString(year)); //enter a DOB that indicates the user is under age
        countryDoBRegisterPage.clickOnCountryDOBNextButton();
        basicInfoRegisterPage.inputEmailId("FAKE_EMAIL");
        basicInfoRegisterPage.inputPassword("t");
        if (basicInfoRegisterPage.verifyEmailInvalidMessageVisible()) {
            logPass("The invalid email address message appeared.");
        } else {
            logFail("The invalid email address message did not appear.");
        }
        
        // 10
        loginPage.clickSignInLink();
        loginPage.inputUsername("passwordbone01@eadtest.ea.com");
        loginPage.inputPassword("aaaa");
        loginPage.clickLoginBtn();
        if (loginPage.verifyInvalidCredentialsBanner()) {
            logPass("Successfully verified the 'weak password' triggered an error message.");
        } else {
            logFailExit("Failed to verify the 'weak password' triggered an error message'.");
        }
        
        client.stop();
        softAssertAll();
    }
}
