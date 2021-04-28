package com.ea.originx.automation.scripts.loginregistration;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.pageobjects.login.BasicInfoRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.CountryDoBRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.GetStartedPage;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.lib.pageobjects.social.SocialHub;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test the underage user registration flow
 *
 * @author mkalaivanan
 */
public class OAUnderageRegistrationFlow extends EAXVxTestTemplate {

    @TestRail(caseId = 1016703)
    @Test(groups = {"loginregistration", "release_smoke", "client_only"})
    public void testUnderageRegistrationFlow(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManagerHelper.createNewThrowAwayAccount(12);

        logFlowPoint("Launch Origin, Begin registration and Verify user is prompted to input date of birth and country"); //1
        logFlowPoint("Enter date of birth and Country and verify user cannot proceed registration without accepting EA's Terms of Service"); //2
        logFlowPoint("Verify that underage user can continue registration process after accepting EA's Terms of Service to enter user related information"); //3
        logFlowPoint("Verify user is prompted to input a parental email address, username and password"); //4
        logFlowPoint("Verify user is registered and logged in automatically after successful registration"); //5
        logFlowPoint("Verify the underage user does not have access to social feature"); //6

        final WebDriver driver = startClientObject(context, client);

        //1
        LoginPage loginPage = new LoginPage(driver);
        loginPage.clickRegistrationLink();
        CountryDoBRegisterPage countryDoBRegisterPage = new CountryDoBRegisterPage(driver);
        if (countryDoBRegisterPage.verifyCountryInputVisible() && countryDoBRegisterPage.verifyDateOfBirthVisible()) {
            logPass("Launched Origin, Clicked on 'Create Account' and verified Country and Date of Birth input fields are visible to the user");
        } else {
            logFailExit("Launch Origin, Begin registration failed");
        }

        //2
        countryDoBRegisterPage.inputUserDOBRegistrationDetails(userAccount);
        boolean isContinueButtonDisabled = countryDoBRegisterPage.verifyContinueButtonIsDisabled();
        if (isContinueButtonDisabled) {
            logPass("User cannot proceed registration without accepting EA's Terms of Service");
        } else {
            logFailExit("User is able to proceed registration without accepting EA's Term of Service");
        }

        //3
        countryDoBRegisterPage.acceptToS();
        boolean isContinueButtonEnabled = !countryDoBRegisterPage.verifyContinueButtonIsDisabled();
        if (isContinueButtonEnabled) {
            logPass("User can continue registration after accepting EA's Term of Service to enter user related information");
        } else {
            logFailExit("User is not able to continue registration after accepting EA's Term of Service");
        }

        //4
        countryDoBRegisterPage.clickOnCountryDOBNextButton();
        BasicInfoRegisterPage basicInfoPage = new BasicInfoRegisterPage(driver);

        boolean isParentEmailAddressVisible = basicInfoPage.verifyParentEmailAddressVisible();
        boolean isUsernameVisible = basicInfoPage.verifyPublicIdVisible();
        boolean isPasswordVisible = basicInfoPage.verifyPasswordVisible();
        if (isParentEmailAddressVisible && isUsernameVisible && isPasswordVisible) {
            logPass("User is prompted to input parental email address, username and password");
        } else {
            logFailExit("Parental email address, username and password are not visible to the user");
        }

        //5
        basicInfoPage.inputUserAccountRegistrationDetails(userAccount, true,false);
        basicInfoPage.clickOnBasicInfoNextButton();

        GetStartedPage getStartedPage = new GetStartedPage(driver);
        getStartedPage.waitForPage();
        getStartedPage.clickGetStartedButton();

        Waits.waitForWindowHandlesToStabilize(driver, 30);
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 30);
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.waitForProfileToLoad();
        boolean miniProfileReached = Waits.pollingWait(() -> miniProfile.verifyUser(userAccount.getUsername()));
        if (miniProfileReached) {
            logPass("User registered successfully and logged in automatically");
        } else {
            logFailExit("User registration failed");
        }

        //6
        boolean isSocialHubNotVisible = !new SocialHub(driver).verifySocialHubVisible();
        if (isSocialHubNotVisible) {
            logPass("Underage user has no access to social feature");
        } else {
            logFailExit("Underage user has access to social feature");
        }
        softAssertAll();
    }

}
