package com.ea.originx.automation.scripts.loginregistration;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.originx.automation.lib.pageobjects.login.BasicInfoRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.CountryDoBRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.GetStartedPage;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests attempting to register an underage user using valid and invalid Origin
 * IDs
 *
 * @author lscholte
 */
public class OAUnderageRegisterOriginId extends EAXVxTestTemplate {

    @TestRail(caseId = 9593)
    @Test(groups = {"loginregistration", "client_only", "full_regression","release_smoke"})
    public void testUnderageRegisterOriginId(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManagerHelper.createNewThrowAwayAccount(7);
        String publicIdAlreadyInUse = AccountManager.getRandomAccount().getUsername();
        String blackListedPublicId = "fuckShit343";

        logFlowPoint("Launch Origin and begin registering the underage user " + userAccount.getUsername()); //1
        logFlowPoint("Verify the email field for 'Parent or Guradian's address' is displayed"); // 2
        logFlowPoint("Verify that 'Origin' checks for the username's availability"); // 3
        logFlowPoint("Verify that if the 'Origin ID' is in use, the user gets an error message"); // 4
        logFlowPoint("Verify that 'Origin' gives the user an error message when the 'Origin ID' contains blacklisted words"); // 5
        logFlowPoint("Verify that if all the inputs meet the criteria, no error message is displayed and user can continue registration"); // 6
        logFlowPoint("Verify that after finishing registration process a modal informs the user an email has been sent to parent/guardian"); // 7

        // 1
        final WebDriver driver = startClientObject(context, client);
        LoginPage loginPage = new LoginPage(driver);
        loginPage.clickRegistrationLink();
        CountryDoBRegisterPage countryDoBRegisterPage = new CountryDoBRegisterPage(driver);
        logPassFail(countryDoBRegisterPage.verifyCountryInputVisible(), true);

        // 2
        countryDoBRegisterPage.enterDoBCountryInformation(userAccount);
        BasicInfoRegisterPage createAccPage = new BasicInfoRegisterPage(driver);
        logPassFail(createAccPage.verifyParentEmailAddressVisible(), false);

        // 3
        createAccPage.inputPublicId(publicIdAlreadyInUse);
        createAccPage.inputCaptcha(OriginClientData.CAPTCHA_AUTOMATION);
        boolean isIDAvailable = Waits.pollingWait(() -> createAccPage.verifyPublicIdAlreadyInUseMessageVisible());
        createAccPage.inputPublicId(userAccount.getUsername());
        createAccPage.inputCaptcha(OriginClientData.CAPTCHA_AUTOMATION);
        boolean isIDTaken = !Waits.pollingWait(() -> createAccPage.verifyPublicIdAlreadyInUseMessageVisible());
        logPassFail(isIDAvailable && isIDTaken, false);

        // 4
        createAccPage.inputPublicId(publicIdAlreadyInUse);
        createAccPage.inputCaptcha(OriginClientData.CAPTCHA_AUTOMATION);
        logPassFail(Waits.pollingWait(() -> createAccPage.verifyPublicIdAlreadyInUseMessageVisible()), false);

        // 5
        createAccPage.inputPublicId(blackListedPublicId);
        createAccPage.inputCaptcha(OriginClientData.CAPTCHA_AUTOMATION);
        logPassFail(Waits.pollingWait(() -> createAccPage.verifyPublicIdInvalidMessageVisible()), false);

        // 6
        createAccPage.inputUserAccountRegistrationDetails(userAccount, true, false);
        logPassFail(createAccPage.isBasicInfoButtonEnabled(), true);

        // 7
        createAccPage.clickOnBasicInfoNextButton();
        GetStartedPage getStartedPage = new GetStartedPage(driver);
        getStartedPage.verifyOnGetStartedPage();
        logPassFail(getStartedPage.verifyParentEmailNotificationText(), isIDTaken);

        softAssertAll();
    }
}
