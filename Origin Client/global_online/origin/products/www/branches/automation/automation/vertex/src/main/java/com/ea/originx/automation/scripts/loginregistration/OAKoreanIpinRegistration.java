package com.ea.originx.automation.scripts.loginregistration;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.login.CountryDoBRegisterPage;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests that identity verification is requested while South Korea is selected
 * as the registering country
 * (This ticket needs refactoring to match up with TestRail)
 *
 * @author JMittertreiner
 */
public class OAKoreanIpinRegistration extends EAXVxTestTemplate {

    @TestRail(caseId = 9495)
    @Test(groups = {"loginregistration", "full_regression"})
    public void testKoreanIpinRegistration(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        logFlowPoint("Launch Origin and select Create an Account"); // 1
        logFlowPoint("Select South Korea in the Country drop down"); // 2
        logFlowPoint("Verify that the 'Verify Identity' and 'Mobile Phone Information' options appear"); // 3

        // 1
        final WebDriver driver = startClientObject(context, client);

        LoginPage loginPage = new LoginPage(driver);
        loginPage.clickRegistrationLink();

        CountryDoBRegisterPage registerPage = new CountryDoBRegisterPage(driver);
        if (registerPage.verifyCountryInputVisible()) {
            logPass("Successfully started Origin client and started creating an account");
        } else {
            logFailExit("Could not launch Origin");
        }

        // 2
        registerPage.selectCountry("South Korea");
        if (registerPage.getCountry().equals("South Korea")) {
            logPass("Selected South Korea from the country drop down");
        } else {
            logFailExit("Failed to select South Korea from the country drop down");
        }

        // 3
        if (registerPage.verifySouthKoreanRegistration()) {
            logPass("The verify identity and mobile phone option appear");
        } else {
            logFail("The verify identity and mobile phone options do not appear");
        }

        softAssertAll();
    }
}
