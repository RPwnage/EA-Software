package com.ea.originx.automation.scripts.loginregistration;

import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.login.CountryDoBRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test registration with a leap year birthday
 *
 * @author mkalaivanan
 */
public class OARegisterLeapYear extends EAXVxTestTemplate {

    @TestRail(caseId = 9437)
    @Test(groups = {"loginregistration", "full_regression"})
    public void testRegisterLeapYear(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        String day = "29";
        String month = "February";
        String year = "1996";

        logFlowPoint("Launch Origin and Begin Registration"); //1
        logFlowPoint("Set the month to 'February' ,Day to '29' and Year to '1996'"); //2
        logFlowPoint("Verify Continue Button is enabled"); //3
        logFlowPoint("Change the year to '1991' and Verify '29 is replaced by 'Day'"); //4
        logFlowPoint("Verify continue Button is disabled for registration"); //5

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
        countryDoBRegisterPage.selectCountry(userAccount.getCountry());
        countryDoBRegisterPage.enterDoB(month, day, year);
        countryDoBRegisterPage.acceptToS();

        boolean isDateOfBirthValuesSet = countryDoBRegisterPage.getDay().equals(day) && countryDoBRegisterPage.getMonth().equals(month) && countryDoBRegisterPage.getYear().equals(year);
        if (isDateOfBirthValuesSet) {
            logPass("Date of Birth set to Feb 29,1996 ");
        } else {
            logFailExit("Date of Birth not set to Feb 29,1996");
        }

        //3
        boolean isContinueButtonEnabled = !countryDoBRegisterPage.verifyContinueButtonIsDisabled();
        if (isContinueButtonEnabled) {
            logPass("Verified the 'Continue Button' is enabled");
        } else {
            logFailExit("'Continue' button is diabled");
        }

        //4
        countryDoBRegisterPage.selectYear("1991");
        if (countryDoBRegisterPage.getDay().equals("Day")) {
            logPass("Change the year to '1991' and day -'29' is replaced by 'Day'");
        } else {
            logFail("Changed the value of year to '1991' and day value did not change from '29' to 'Day'");
        }

        //5
        if (countryDoBRegisterPage.verifyContinueButtonIsDisabled()) {
            logPass("Verified the 'Continue Button' is disabled");
        } else {
            logFailExit("'Continue' button is enabled");
        }
        softAssertAll();
    }
}
