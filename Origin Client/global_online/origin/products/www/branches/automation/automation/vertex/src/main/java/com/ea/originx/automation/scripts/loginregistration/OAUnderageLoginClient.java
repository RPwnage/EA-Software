package com.ea.originx.automation.scripts.loginregistration;

import com.ea.originx.automation.lib.helpers.AccountManagerHelper;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.common.MainMenu;
import com.ea.originx.automation.lib.pageobjects.login.LoginOptions;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.originclient.webdrivers.BrowserType;
import com.ea.vx.annotations.TestRail;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the underAge user can login once before parental email is verified
 * and can login back once the parental email is verified
 *
 * @author nvarthakavi
 */
public class OAUnderageLoginClient extends EAXVxTestTemplate {

    @TestRail(caseId = 1016749)
    @Test(groups = {"loginregistration", "release_smoke", "client_only"})
    public void testOAUnderageLoginClient(ITestContext context) throws Exception {

        UserAccount underageAccount = AccountManagerHelper.createNewThrowAwayAccount(10);
        String underageUserID = underageAccount.getUsername();

        logFlowPoint("Login as newly register underage account"); //1
        logFlowPoint("Logout and login back as underage account and verify the user can navigate to Origin page on clicking continue"); //2
        logFlowPoint("Logout from the underage user and verify user will be prompted with the login page"); //3
        logFlowPoint("Login back as underage user and verify the user cannot login without verifying the parental email"); //4
        logFlowPoint("Verify the parental email address of the underage user by navigating to the verification link in the parent's email"); //5
        logFlowPoint("Login again as underage user to verify the account has been verified and is navigated automatically to Origin"); //6
        logFlowPoint("Logout from underage user to verify on logging out the user is navigated automatically to Sign In page"); //7

        //1
        final OriginClient client = OriginClientFactory.create(context);
        WebDriver driver = startClientObject(context, client);
        if (MacroLogin.startLogin(driver, underageAccount)) {
            logPass("Successfully created an underage user and logged in as: " + underageUserID);
        } else {
            logFailExit("Could not create or login as a underage user");
        }

        //2
        new MiniProfile(driver).selectSignOut();
        LoginPage loginPage = new LoginPage(driver);
        MacroLogin.startLogin(driver, underageAccount,
                new LoginOptions().setNoWaitForDiscoverPage(true).setSkipVerifyingMiniProfileUsername(true));
        loginPage.clickUnderageVerificationContinueButton();
        loginPage.waitForDiscoverPage();
        if (MacroLogin.verifyMiniProfileUsername(driver, underageUserID)) {
            logPass("Verified the user can navigate to Origin page");
        } else {
            logFailExit("Could not login as a underage user");
        }

        //3
        new MainMenu(driver).selectLogOut(true);
        if (loginPage.verifyLoginPageVisible()) {
            logPass("Verified the user is prompted login page after logging out");
        } else {
            logFailExit("Failed: the user was not prompted to login page after logging out");
        }

        //4
        MacroLogin.startLogin(driver, underageAccount,
                new LoginOptions().setNoWaitForDiscoverPage(true).setSkipVerifyingMiniProfileUsername(true));
        boolean underAgeVerificationPage = loginPage.verifyUnderageVerificationLoginPageVisible();
        loginPage.clickUnderageVerificationContinueButton();
        sleep(2000); // wait a little for page to load
        boolean underAgeEmailErrorMessage = loginPage.verifyUnderAgeEmailVerificationErrorMessageVisible();
        if (underAgeVerificationPage && underAgeEmailErrorMessage) {
            logPass("Verified the user cannot login without verifying the email");
        } else {
            logFailExit("Failed: the user was was able to login without verifying the email");
        }

        //5
        final WebDriver browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        boolean success = MacroLogin.verifyParentEmail(browserDriver, underageUserID);
        if (success) {
            logPass("Verified successfully the Parental Email of the UnderAge user");
        } else {
            logFailExit("Failed: Could not verify the Parental Email of the UnderAge user");
        }
        browserDriver.close(); // closing the browser

        //6
        loginPage.clickSignInLink();
        if (MacroLogin.startLogin(driver, underageAccount)) {
            logPass("Verified the account has been verified and is navigated automatically to Origin");
        } else {
            logFailExit("Could not verify the account has been verified or is not navigated automatically to Origin");
        }

        //7
        new MainMenu(driver).selectLogOut(true);
        if (loginPage.verifyLoginPageVisible()) {
            logPass("Verified on logging out the user is navigated automatically to Sign In page");
        } else {
            logFailExit("Failed: On logging out the user was not navigated automatically to Sign In page");
        }

        softAssertAll();
    }
}
