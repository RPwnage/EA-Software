package com.ea.originx.automation.scripts.loginregistration;

import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.login.BasicInfoRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.CountryDoBRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.login.SecurityQuestionAnswerPage;
import com.ea.vx.originclient.resources.OriginClientConstants;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.client.OriginClient;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.testng.annotations.Test;

/**
 * Tests that the newsletter opt in box's default is different based on the
 * user's country
 *
 * @author JMittertreiner
 * @author palui Merge all parameterized scripts into a single script
 */
public class OARegionalOptIn extends EAXVxTestTemplate {

    // The list of countries that should be opted in by default: See
    // https://eaoffice.sharepoint.com/sites/LegalDocs/PrivacyLRDs/_layouts/15/WopiFrame2.aspx?sourcedoc=%7b6B6E755C-C212-467B-B5C3-B498C501280D%7d&file=EADP%20Gen4%20Contact%20Preference%20Screen%20PRG.docx&action=default
    private final List<String> OPTED_IN_COUNTRIES = Arrays.asList("Austria", "Belgium",
            "Bulgaria", "Cyprus", "Czech Republic", "Denmark", "Estonia", "Finland", "France",
            "Greece", "Hungary", "Ireland", "Italy", "Latvia", "Lithuania", "Luxembourg", "Malta", "Mexico", "Netherlands",
            "Poland", "Portugal", "Puerto Rico", "Romania", "Slovakia (Slovak Republic)", "Slovenia", "Spain", "Sweden",
            "United Kingdom", "United States of America");
    private final List<String> failures = new ArrayList<>();

    @TestRail(caseId = 9433)
    @Test(groups = {"loginregistration", "full_regression"})
    public void testOARegionalOptIn(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        logFlowPoint("Launch Origin and select Create an Account"); // 1

        // 1
        final WebDriver driver = startClientObject(context, client);

        final LoginPage loginPage = new LoginPage(driver);
        loginPage.clickRegistrationLink();

        CountryDoBRegisterPage registerPage = new CountryDoBRegisterPage(driver);
        if (registerPage.verifyCountryInputVisible()) {
            logPass("Successfully logged in and started creating an account");
        } else {
            logFailExit("Failed to start creating an account");
        }
        registerPage.inputUserDOBRegistrationDetails(userAccount);

        // Get all the countries and add one third of them to the test list
        List<String> countries = registerPage.getCountries();
        countries.remove(" South Korea"); // Skip South Korea because of ipin registration
        countries.remove("Country");      // Remove the default option

        logFlowPoint("Check countries from " + countries.get(0).trim() + " to " + countries.get(countries.size() - 1).trim() + " to ensure that the opt in box is correct"); // 2

        // The first time through, we need to set all the information instead of
        // just the parts that get cleared
        final BasicInfoRegisterPage basicInfoPage = new BasicInfoRegisterPage(driver);
        final SecurityQuestionAnswerPage securityQuestionAnswerPage = new SecurityQuestionAnswerPage(driver);
        registerPage.acceptToS();
        registerPage.clickOnCountryDOBNextButton();
        basicInfoPage.waitForBasicInfoPage();
        basicInfoPage.inputUserAccountRegistrationDetailsWithRetry(userAccount);
        securityQuestionAnswerPage.waitForPage();
        // waitForPage doesn't seem to wait long enough. If you
        // don't wait long enough, Origin throws an error about experiencing
        // difficulties
        sleep(1000);
        securityQuestionAnswerPage.goBack();
        sleep(1000);
        basicInfoPage.goBack();

        for (String country : countries) {
            country = country.trim();
            sleep(1000);
            registerPage.selectCountry(country);
            sleep(1000); // Pause between selection and clicking next, as most 'Techincal Difficulties' tend to occur here
            registerPage.clickOnCountryDOBNextButton();

            basicInfoPage.waitForBasicInfoPage();
            basicInfoPage.inputPassword(OriginClientConstants.DEFAULT_ACCOUNT_PASSWORD);
            basicInfoPage.clickOnBasicInfoNextButton();

            securityQuestionAnswerPage.waitForPage();
            if (securityQuestionAnswerPage.verifyEmailMeIsSelected()
                    != OPTED_IN_COUNTRIES.contains(country)) {
                failures.add(country);
            }
            sleep(1000);
            securityQuestionAnswerPage.goBack();
            sleep(1000);
            basicInfoPage.goBack();
        }

        if (failures.isEmpty()) {
            logPass("All countries had the correct opt in default");
        } else {
            logFail(failures + " did not have the correct opt in default");
        }

        softAssertAll();
    }
}
