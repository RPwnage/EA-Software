package com.ea.originx.automation.scripts.loginregistration;

import com.ea.originx.automation.lib.pageobjects.common.NavigationSidebar;
import com.ea.originx.automation.lib.pageobjects.common.TakeOverPage;
import com.ea.originx.automation.lib.pageobjects.discover.DiscoverPage;
import com.ea.originx.automation.lib.pageobjects.login.BasicInfoRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.CountryDoBRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.GetStartedPage;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.login.SecurityQuestionAnswerPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
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
 * Tests the registration flow of normal user
 *
 * @author mkalaivanan
 */
public class OARegistrationFlow extends EAXVxTestTemplate {

    @TestRail(caseId = 1151513)
    @Test(groups = {"loginregistration", "release_smoke"})
    public void testRegistrationFlow(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();

        logFlowPoint("Launch Origin and click on  the 'Create Account' button and verify 'Date of Birth' and 'Country' input fields exist"); // 1
        logFlowPoint("Verify the user cannot proceed without the 'Date of Birth' field completed"); // 2
        logFlowPoint("Enter the 'Date of Birth' and click 'Continue to Next page' "); // 3
        logFlowPoint("Verify 'Email', 'OriginId' and 'Password' input fields exist"); // 4
        logFlowPoint("Verify 'First Name', 'Last name' and Captcha input fields exist"); // 5
        logFlowPoint("Continue to register to next page without 'First Name' and 'Last Name' and verify 'Security Question and Answer' input exists"); // 6
        logFlowPoint("Verify profile viewing option exists and it defaults to 'Friends'"); // 7
        logFlowPoint("Verify checkbox exists for finding a user by email and is unchecked by default"); // 8
        logFlowPoint("Verify checkbox exists for signing up for a newsletter and is checked by default"); // 9
        logFlowPoint("Complete the registration page , click on 'Get Started' and verify that the user is automatically logged in on to the 'Discover' page"); // 10

        // 1
        final WebDriver driver = startClientObject(context, client);
        
        LoginPage loginPage = new LoginPage(driver);
        loginPage.clickRegistrationLink();

        CountryDoBRegisterPage countryDoBRegisterPage = new CountryDoBRegisterPage(driver);
        boolean isCountryInputVisible = countryDoBRegisterPage.verifyCountryInputVisible();
        boolean isDateOfBirthVisible = countryDoBRegisterPage.verifyDateOfBirthVisible();
        logPassFail(isCountryInputVisible && isDateOfBirthVisible, true);

        // 2
        boolean isContinueButtonIsDisabled = countryDoBRegisterPage.verifyContinueButtonIsDisabled();
        logPassFail(isContinueButtonIsDisabled, true);

        // 3
        countryDoBRegisterPage.enterDoBCountryInformation(userAccount);
        BasicInfoRegisterPage basicInfoRegisterPage = new BasicInfoRegisterPage(driver);
        basicInfoRegisterPage.waitForBasicInfoPage();
        boolean isBasicInfoRegisterPage = basicInfoRegisterPage.verifyBasicInfoPageReached();
        logPassFail(isBasicInfoRegisterPage, true);

        // 4
        boolean isEmailVisible = basicInfoRegisterPage.verifyEmailVisible();
        boolean isPublicIdVisible = basicInfoRegisterPage.verifyPublicIdVisible();
        boolean isPasswordVisible = basicInfoRegisterPage.verifyPasswordVisible();
        logPassFail(isEmailVisible && isPublicIdVisible && isPasswordVisible, true);

        // 5
        boolean isFirstNameVisible = basicInfoRegisterPage.verifyFirstNameVisible();
        boolean isLastNameVisible = basicInfoRegisterPage.verifyLastNameVisible();
        boolean isCaptchaVisible = basicInfoRegisterPage.verifyCaptchaVisible();
        logPassFail(isFirstNameVisible && isLastNameVisible && isCaptchaVisible, false);

        // 6
        basicInfoRegisterPage.inputUserAccountRegistrationDetailsWithRetry(userAccount);
        SecurityQuestionAnswerPage securityQuestionAnswerPage = new SecurityQuestionAnswerPage(driver);
        securityQuestionAnswerPage.waitForPage();
        boolean isSecurityQuestionVisible = securityQuestionAnswerPage.verifySecurityQuestionPresent();
        boolean isSecurityAnswerVisible = securityQuestionAnswerPage.verifySecurityAnswerVisible();
        logPassFail(isSecurityQuestionVisible && isSecurityAnswerVisible, false);

        // 7
        boolean isProfileVisiblityPresent = securityQuestionAnswerPage.verifyProfileVisibilityVisible();
        boolean defaultProfileEveryone = securityQuestionAnswerPage.verifyProfileVisiblitySetting("Friends");
        logPassFail(isProfileVisiblityPresent && defaultProfileEveryone, false);

        // 8
        boolean isFindByEmailCheckboxVisible = securityQuestionAnswerPage.verifyEmailVisiblityVisible();
        boolean isFindByEmailCheckboxSelected = !securityQuestionAnswerPage.isEmailVisiblityIsSelected();
        logPassFail(isFindByEmailCheckboxVisible && isFindByEmailCheckboxSelected, false);

        // 9
        boolean isContactMeCheckBoxVisible = securityQuestionAnswerPage.verifyContactMeVisible();
        boolean isContactMeCheckBoxChecked = securityQuestionAnswerPage.verifyEmailMeIsSelected();
        logPassFail(isContactMeCheckBoxVisible && isContactMeCheckBoxChecked, true);

        // 10
        securityQuestionAnswerPage.enterSecurityQuestionAnswerWithRetry();
        GetStartedPage getStartedPage = new GetStartedPage(driver);
        getStartedPage.waitForPage();
        getStartedPage.clickGetStartedButton();
        Waits.sleep(10000); // wait for 10 seconds for client to start
        Waits.waitForPageThatMatches(driver, OriginClientData.MAIN_SPA_PAGE_URL, 60);
        MiniProfile miniProfile = new MiniProfile(driver);
        miniProfile.waitForProfileToLoad();
        boolean isUsernameInMiniProfile = Waits.pollingWait(() -> miniProfile.verifyUser(userAccount.getUsername()));
        DiscoverPage discoverPage = new NavigationSidebar(driver).gotoDiscoverPage();
        new TakeOverPage(driver).closeIfOpen();
        discoverPage.waitForPage();
        boolean isMyHomeReached = discoverPage.verifyDiscoverPageReachedWithNewAccount();
        logPassFail(isUsernameInMiniProfile && isMyHomeReached, true);

        softAssertAll();
    }
}