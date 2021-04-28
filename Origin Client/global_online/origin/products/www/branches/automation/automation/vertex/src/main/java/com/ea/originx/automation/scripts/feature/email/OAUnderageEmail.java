package com.ea.originx.automation.scripts.feature.email;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.login.LoginOptions;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.originx.automation.lib.helpers.EmailFetcher;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test Underage verification email
 *
 * @author palui
 */
public class OAUnderageEmail extends EAXVxTestTemplate {

    @TestRail(caseId = 3068743)
    @Test(groups = {"email_smoke", "email", "client_only", "allfeaturesmoke"})
    public void testUnderageEmail(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount underageAccount = AccountManagerHelper.createNewThrowAwayAccount(12);// Underage account age 12
        final String underageName = underageAccount.getUsername();

        logFlowPoint("Launch Origin client and login as newly registered underage user: "); //1
        logFlowPoint("Verify parent receives email with the 'Verify Parent Email' link"); //2
        logFlowPoint("Logout and log back in. Verify login is prompted by the 'Underage Verification' login page"); //3
        logFlowPoint("Click 'Continue' at the 'Underage Verification' login page. Verify underage user can log back in"); //4
        logFlowPoint("Click 'Verify Parent Email' link. Verify 'Email Verification' page opens in a browser and indicates success"); //5
        logFlowPoint("Logout and log back in. Verify underage user can now login without being prompted"); //6

        //1
        final WebDriver originDriver = startClientObject(context, client);
        if (MacroLogin.startLogin(originDriver, underageAccount)) {
            logPass(String.format("Verified login to Origin successful as newly registered underage user %s", underageName));
        } else {
            logFailExit(String.format("Failed: Cannot launch Origin or login as newly registered underage user %s", underageName));
        }

        //2
        EmailFetcher emailFetcher = new EmailFetcher(underageName);
        String verifyEmailLink = emailFetcher.getVerifyParentEmailLink();
        if (verifyEmailLink != null) {
            logPass("Verified parent receives email with 'Verify Parent Email' link");
        } else {
            logFailExit("Failed: Parent does not receive email with 'Verify Parent Email' link");
        }
        emailFetcher.closeConnection();

        //3
        LoginPage loginPage = new MainMenu(originDriver).selectLogOut(true);
        MacroLogin.startLogin(originDriver, underageAccount,
                new LoginOptions().setNoWaitForDiscoverPage(true).setSkipVerifyingMiniProfileUsername(true));
        if (loginPage.verifyUnderageVerificationLoginPageVisible()) {
            logPass("Verified login is prompted by the 'Underage Verification' login page");
        } else {
            logFailExit("Failed: 'Underage verfication' login page is not visible");
        }

        //4
        loginPage.clickUnderageVerificationContinueButton();
        loginPage.waitForDiscoverPage();
        if (MacroLogin.verifyMiniProfileUsername(originDriver, underageName)) {
            logPass("Verified clicking 'Continue' button allows the underage user to login");
        } else {
            logFailExit("Failed: Clicking 'Continue' button does not allow the underage user to login");
        }

        //5
        final WebDriver browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        boolean success = MacroLogin.verifyParentEmail(browserDriver, underageName);
        if (success) {
            logPass("Verified 'Email Verification' page opens in a browser and indicates success");
        } else {
            logFailExit("Failed: 'Email Verification' page does not open in a browser or does not indicate success");
        }
        sleep(2000); // wait a little for email verification to take its course

        //6
        new MainMenu(originDriver).selectLogOut(true);
        if (MacroLogin.startLogin(originDriver, underageAccount)) {
            logPass("Verified underage user can now login without being prompted");
        } else {
            logFailExit("Failed: Underage user cannot login or is being prompted");
        }
        softAssertAll();
    }
}
