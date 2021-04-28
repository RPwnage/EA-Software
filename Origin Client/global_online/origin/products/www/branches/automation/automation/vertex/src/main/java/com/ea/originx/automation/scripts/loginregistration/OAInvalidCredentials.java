package com.ea.originx.automation.scripts.loginregistration;

import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.utils.Waits;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Test logging in with a variety of invalid credentials.
 *
 * @author glivingstone
 */
public class OAInvalidCredentials extends EAXVxTestTemplate {

    OriginClient client;
    private WebDriver driver;
    private LoginPage loginPage;

    private void relaunchOrigin(ITestContext context) throws Exception {
        client.stop();
        driver = startClientObject(context, client);
        loginPage = initPage(driver, context, LoginPage.class);
        if (loginPage.isOnLoginPage() && !loginPage.isInvalidCredentailsBannerVisible()) {
            logPass("Successfully closed and reopened the Origin client.");
        } else {
            logFailExit("Could not close and reopen the Origin client.");
        }
    }

    @TestRail(caseId = 9450)
    @Test(groups = {"full_regression", "loginregistration", "loginregistration_smoke"})
    public void testInvalidCredentials(ITestContext context) throws Exception {
        UserAccount userAccount = AccountManager.getRandomAccount();

        boolean isClient = ContextHelper.isOriginClientTesing(context);

        String loginId = isClient ? userAccount.getUsername() : userAccount.getEmail();
        String wrongEmail = "wrong" + userAccount.getEmail();
        String wrongID = userAccount.getUsername() + "wrong";
        String invalidCharID = loginId + "Ö¿ø¡";
        String wrongPassword = userAccount.getPassword() + "wrong";

        logFlowPoint("Launch Origin"); // 1
        logFlowPoint("Attempt to Login with an Invalid Email and Valid Password and Verify Login Fails"); // 2
        logFlowPoint("Verify there is a Banner Stating 'Your Email, ID, or Password is Incorrect. Please try again.'"); // 3
        logFlowPoint("Verify after the login attempt the password field is cleared"); // 4
        if (isClient) {
            logFlowPoint("Close then Launch Origin"); // 4.5
        }
        logFlowPoint("Attempt to Login with an Invalid Username and Valid Password and Verify Login Fails"); // 5
        logFlowPoint("Verify there is a Banner Stating 'Your Email, ID, or Password is Incorrect. Please try again.'"); // 6
        logFlowPoint("Verify after the login attempt the password field is cleared"); // 7
        if (isClient) {
            logFlowPoint("Close then Launch Origin"); // 7.5
        }
        logFlowPoint("Attempt to Login with a Username with Invalid Characters and Valid Password and Verify Login Fails"); // 8
        logFlowPoint("Verify there is a Banner Stating 'Your Email, ID, or Password is Incorrect. Please try again.'"); // 9
        logFlowPoint("Verify after the login attempt the password field is cleared"); // 10
        if (isClient) {
            logFlowPoint("Close then Launch Origin"); // 10.5
        }
        logFlowPoint("Attempt to Login with a Valid Username and Invalid Password and Verify Login Fails"); // 11
        logFlowPoint("Verify there is a Banner Stating 'Your Email, ID, or Password is Incorrect. Please try again.'"); // 12
        logFlowPoint("Verify after the login attempt the password field is cleared"); // 13
        if (isClient) {
            logFlowPoint("Close then Launch Origin"); // 13.5
        }
        logFlowPoint("Verify the User can Login with a Valid Username and Password"); // 14

        // 1
        client = OriginClientFactory.create(context);
        driver = startClientObject(context, client);
        if (driver != null) {
            logPass("Sucessfully started Origin client.");
        } else {
            logFailExit("Could not launch Origin.");
        }

        // 2
        loginPage = initPage(driver, context, LoginPage.class);
        loginPage.enterInformation(wrongEmail, userAccount.getPassword());
        if (Waits.pollingWaitEx(loginPage::isOnLoginPage, 2000, 500, 0)) {
            logPass("Could not login with an invalid email.");
        } else {
            logFailExit("Could login with an invalid email.");
        }

        // 3
        if (loginPage.verifyInvalidCredentialsBanner()) {
            logPass("Banner appears stating the credentials are invalid.");
        } else {
            logFailExit("Banner stating credentials are invalid does not appear.");
        }

        // 4
        if (loginPage.verifyPasswordCleared()) {
            logPass("The password box has been cleared");
        } else {
            logFail("The password box has not been cleared");
        }

        // 4.5
        if (isClient) {
            relaunchOrigin(context);
        }

        // 5
        loginPage.enterInformation(wrongID, userAccount.getPassword());
        if (Waits.pollingWaitEx(loginPage::isOnLoginPage, 2000, 500, 0)) {
            logPass("Could not login with an invalid ID.");
        } else {
            logFailExit("Could login with an invalid ID.");
        }

        // 6
        if (loginPage.verifyInvalidCredentialsBanner()) {
            logPass("Banner appears stating the credentials are invalid.");
        } else {
            logFailExit("Banner stating credentials are invalid does not appear.");
        }

        // 7
        if (loginPage.verifyPasswordError()) {
            logPass("Password field is empty after login attempt");
        } else {
            logFail("Password field is not empty after login attempt");
        }

        // 7.5
        if (isClient) {
            relaunchOrigin(context);
        }

        // 8
        loginPage.enterInformation(invalidCharID, userAccount.getPassword());
        if (Waits.pollingWait(loginPage::isOnLoginPage, 2000, 500, 0)) {
            logPass("Could not login with an ID with invalid characters.");
        } else {
            logFailExit("Could login with an ID with invalid characters.");
        }

        // 9
        if (loginPage.verifyInvalidCredentialsBanner()) {
            logPass("Banner appears stating the credentials are invalid.");
        } else {
            logFailExit("Banner stating credentials are invalid does not appear.");
        }

        // 10
        if (loginPage.verifyPasswordError()) {
            logPass("Password field is empty after login attempt");
        } else {
            logFail("Password field is not empty after login attempt");
        }

        // 10.5
        if (isClient) {
            relaunchOrigin(context);
        }

        // 11
        loginPage.enterInformation(loginId, wrongPassword);
        if (Waits.pollingWait(loginPage::isOnLoginPage, 2000, 500, 0)) {
            logPass("Could not login with an invalid password.");
        } else {
            logFailExit("Could login with an invalid password.");
        }

        // 12
        if (loginPage.verifyInvalidCredentialsBanner()) {
            logPass("Banner appears stating the credentials are invalid.");
        } else {
            logFailExit("Banner stating credentials are invalid does not appear.");
        }

        // 13
        if (loginPage.verifyPasswordError()) {
            logPass("Password field is empty after login attempt");
        } else {
            logFail("Password field is not empty after login attempt");
        }

        // 13.5
        if (isClient) {
            relaunchOrigin(context);
        }

        // 14
        loginPage.enterInformation(loginId, userAccount.getPassword());
        loginPage.waitForDiscoverPage();
        if (new MiniProfile(driver).verifyUser(userAccount.getUsername())) {
            logPass("Successfully logged into Origin as " + userAccount.getUsername());
        } else {
            logFailExit("Failed to log into Origin as " + userAccount.getUsername());
        }
        client.stop();
        softAssertAll();
    }
}
