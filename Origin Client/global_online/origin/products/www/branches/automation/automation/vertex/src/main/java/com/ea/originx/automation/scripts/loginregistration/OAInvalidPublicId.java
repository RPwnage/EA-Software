package com.ea.originx.automation.scripts.loginregistration;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.pageobjects.login.BasicInfoRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.CountryDoBRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.GetStartedPage;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.login.SecurityQuestionAnswerPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test registration for PublicId with already taken id,blacklisted word,foreign
 * characters
 *
 * @author mkalaivanan
 */
public class OAInvalidPublicId extends EAXVxTestTemplate {

    @TestRail(caseId = 9432)
    @Test(groups = {"loginregistration", "full_regression"})
    public void testInvalidPublicId(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        String publicIdAlreadyInUse = AccountManager.getRandomAccount().getUsername();
        String blackListedPublicId = "fuckShit343";
        String foreignCharPublicId = "ऑऋसउ";

        logFlowPoint("Launch Origin and Begin Registration"); //1
        logFlowPoint("Register using already in use PublicId and Verify 'The Public ID you have entered is already in use' message is displayed"); //2
        logFlowPoint("Register with PublicId that has blacklisted word  and Verify 'ID contains a prohibited word or character' message is displayed"); //3
        logFlowPoint("Register with PublicId that has foreign characters and Verify 'ID contains a prohibited word or character' message is displayed"); //4
        logFlowPoint("Register with no PublicId and Verify the user is unable to click Next"); //5
        logFlowPoint("Register with a valid PublicId and Verify the user is able to continue registration"); //6

        //1
        final WebDriver driver = startClientObject(context, client);

        LoginPage loginPage = new LoginPage(driver);
        loginPage.clickRegistrationLink();

        CountryDoBRegisterPage countryDoBRegisterPage = new CountryDoBRegisterPage(driver);
        if (countryDoBRegisterPage.verifyCountryInputVisible()) {
            logPass("Origin Launched and Started Registration");
        } else {
            logFailExit("User registration did not start");
        }

        //2
        countryDoBRegisterPage.enterDoBCountryInformation(userAccount);
        BasicInfoRegisterPage createAccPage = new BasicInfoRegisterPage(driver);
        createAccPage.waitForBasicInfoPage();

        createAccPage.inputEmailId(userAccount.getEmail());
        createAccPage.inputPassword(userAccount.getPassword());
        createAccPage.inputPublicId(publicIdAlreadyInUse);
        createAccPage.inputFirstName(userAccount.getFirstName());
        if (Waits.pollingWait(() -> createAccPage.verifyPublicIdAlreadyInUseMessageVisible())) {
            logPass("Registered using Already in Use PublicId:" + publicIdAlreadyInUse + "and Verified 'The Public ID you have entered is already in use' message is displayed");
        } else {
            logFail("Already in use PublicId did not trigger 'The Public ID you have entered is already in use' ");
        }

        //3
        createAccPage.inputPublicId(blackListedPublicId);
        createAccPage.inputFirstName(userAccount.getFirstName());
        if (Waits.pollingWait(() -> createAccPage.verifyPublicIdInvalidMessageVisible())) {
            logPass("Registered using PublicId that has blacklisted word" + blackListedPublicId + "and Verified 'ID contains a prohibited word or character' message is displayed'");
        } else {
            logFail("PublicId with blacklisted word did not trigger 'ID contains a prohibited word or character' message is displayed'");
        }

        //4
        createAccPage.inputPublicId(foreignCharPublicId);
        createAccPage.inputFirstName(userAccount.getFirstName());
        if (Waits.pollingWait(() -> createAccPage.verifyPublicIdInvalidMessageVisible())) {
            logPass("Registered using PublicId that has foreign characters" + blackListedPublicId + " and Verified 'ID contains a prohibited word or character' message is displayed'");
        } else {
            logFail("PublicId with foreign characters did not trigger 'ID contains a prohibited word or character' message is displayed'");
        }

        //5
        createAccPage.inputPublicId("");
        createAccPage.inputFirstName(userAccount.getFirstName());
        createAccPage.inputLastName(userAccount.getLastName());
        createAccPage.inputCaptcha(OriginClientData.CAPTCHA_AUTOMATION);
        boolean isNextButtonDisabled = Waits.pollingWait(() -> createAccPage.isBasicInfoButtonEnabled());
        if (!isNextButtonDisabled) {
            logPass("Registered using No PublicId and Next Button was disabled");
        } else {
            logFailExit("Next button is enabled when public id is not present");
        }

        //6
        createAccPage.inputPublicId(userAccount.getUsername());
        createAccPage.inputFirstName(userAccount.getFirstName());
        createAccPage.inputLastName(userAccount.getLastName());
        createAccPage.inputCaptcha(OriginClientData.CAPTCHA_AUTOMATION);
        boolean isNextButtonEnabled = createAccPage.isBasicInfoButtonEnabled();
        createAccPage.clickOnBasicInfoNextButton();

        new SecurityQuestionAnswerPage(driver).enterSecurityQuestionAnswerWithRetry();
        GetStartedPage getStartedPage = new GetStartedPage(driver);
        getStartedPage.waitForPage();
        getStartedPage.clickGetStartedButton();

        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 30);
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.waitForPageToLoad();
        miniProfile.waitForJQueryAJAXComplete(30);
        miniProfile.waitForAngularHttpComplete();
        boolean miniProfileReached = Waits.pollingWait(() -> miniProfile.verifyUser(userAccount.getUsername()));
        if (isNextButtonEnabled && miniProfileReached) {
            logPass("Registered Successfully with a valid PublicId");
        } else {
            logFailExit("Registeration failed with a valid PublicId");
        }

        softAssertAll();
    }
}
