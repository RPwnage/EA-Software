package com.ea.originx.automation.scripts.loginregistration;

import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.utils.Waits;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.login.PasswordRecoveryPage;
import com.ea.originx.automation.lib.pageobjects.login.PasswordRecoverySentPage;
import com.ea.originx.automation.lib.pageobjects.login.ResetPasswordPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.originx.automation.lib.helpers.EmailFetcher;
import com.ea.originx.automation.lib.resources.OriginClientData;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.webdrivers.BrowserType;
import com.ea.vx.originclient.helpers.ContextHelper;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Tests the forgot password feature
 *
 * @author mkalaivanan
 */
public class OAForgotPassword extends EAXVxTestTemplate {

    @TestRail(caseId = 9422)
    @Test(groups = {"loginregistration", "full_regression"})
    public void testForgotPassword(ITestContext context) throws Exception {

        final OriginClient client = OriginClientFactory.create(context);

        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        String newPassword = "Qatest3r";
        String username = userAccount.getUsername();

        logFlowPoint("Launch Origin and Create an account"); //1
        logFlowPoint("Logout of Origin"); //2
        logFlowPoint("Click on 'Forgot Password'"); //3
        logFlowPoint("Enter Email id and Captcha"); //4
        logFlowPoint("Verify email is received with a link to reset the password"); //5
        logFlowPoint("Open the link in a browser"); //6
        logFlowPoint("Enter the new password and confirm the new password"); //7
        logFlowPoint("Click on Submit button and Verify Successful Password Changed message is visible"); //8
        logFlowPoint("Verify user is able to login into account with the new password"); //9

        final WebDriver driver = startClientObject(context, client);
        String spaUrl = driver.getCurrentUrl();

        //1
        if (MacroLogin.startLogin(driver, userAccount)) {
            logPass("Successfully created user and logged in as " + username);
        } else {
            logFailExit("Failed to create user " + username);
        }

        //2
        boolean isClient = ContextHelper.isOriginClientTesing(context);
        LoginPage loginPage = new MiniProfile(driver).selectSignOut();
        if (!isClient) {
            loginPage.clickBrowserLoginBtn();
        }
        boolean isLoginPage = Waits.pollingWait(() -> loginPage.isPageThatMatchesOpen(OriginClientData.LOGIN_PAGE_URL_REGEX));
        if (isLoginPage) {
            logPass("Successfully logged out of Origin");
        } else {
            logFailExit("Failed to log out of Origin");
        }

        //3
        loginPage.clickForgotPassword();
        PasswordRecoveryPage passwordRecoveryPage = new PasswordRecoveryPage(driver);
        passwordRecoveryPage.waitForPasswordRecoveryPageToLoad();
        if (passwordRecoveryPage.verifyPasswordRecoveryPageReached()) {
            logPass("Clicked on 'Forgot Password' and Reset password page opened successfully");
        } else {
            logFailExit("Clicking on 'Forgot Password' failed");
        }

        //4
        passwordRecoveryPage.enterEmailAddress(userAccount.getEmail());
        passwordRecoveryPage.enterCaptcha(OriginClientData.CAPTCHA_AUTOMATION);
        passwordRecoveryPage.clickSendButton();
        PasswordRecoverySentPage passwordRecoverySentPage = new PasswordRecoverySentPage(driver);
        passwordRecoverySentPage.waitForPage();
        if (Waits.pollingWait(() -> passwordRecoverySentPage.verifyPasswordRecoverySentMessage())) {
            logPass("Email id and Captcha entered and Passord Recovery Sent Message appeared");
        } else {
            logFailExit("Email id and captcha could not be entered");
        }

        //5
        sleep(10000); // wait for email with the Reset Password Link to arrive
        EmailFetcher emailFetcher = new EmailFetcher(username);
        String resetLink = emailFetcher.getResetPasswordLink();
        boolean gotEmail = resetLink != null;
        if (gotEmail) {
            logPass("Verify Email is received with email link password");
        } else {
            logFailExit("Email with reset password link not received");
        }

        //6
        final WebDriver browserDriver;
        emailFetcher.closeConnection();
        if (isClient) {
            browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        } else {
            browserDriver = driver;
            browserDriver.close();
            String oldWindow = driver.getWindowHandles().toArray(new String[0])[0];
            browserDriver.switchTo().window(oldWindow);
        }
        browserDriver.get(resetLink);

        ResetPasswordPage resetPasswordPage = new ResetPasswordPage(browserDriver);
        if (resetPasswordPage.verifyResetPasswordPageReached()) {
            logPass("Reset Link opened in the browser successfully");
        } else {
            logFailExit("Could not open reset link in the browser");
        }

        //7
        resetPasswordPage.resetPassword(newPassword);
        if (resetPasswordPage.isSubmitButtonEnabled()) {
            logPass("Entered New Password and Confirmed the new password successfully");
        } else {
            logFailExit("New Password not entered successfully");
        }

        //8
        resetPasswordPage.clickSubmitButton();
        if (resetPasswordPage.verifyPasswordChangedMessageVisible()) {
            logPass("Clicked on Submit button and Successful change of password message is visible");
        } else {
            logFailExit("Password not changed successfully");
        }

        //9
        LoginPage loginPage1 = new LoginPage(driver);
        if (isClient) {
            passwordRecoverySentPage.waitForPage();
            passwordRecoverySentPage.clickBackButton();
            Waits.waitForPageThatMatches(driver, OriginClientData.LOGIN_PAGE_URL_REGEX, 60);
            loginPage1.inputUsername(username);
        } else {
            driver.get(spaUrl);
            loginPage1.clickBrowserLoginBtn();
            loginPage1.inputUsername(userAccount.getEmail());
        }

        loginPage1.inputPassword(newPassword);
        loginPage1.clickLoginBtn();
        loginPage.waitForDiscoverPage();
        if (MacroLogin.verifyMiniProfileUsername(driver, userAccount.getUsername())) {
            logPass("User successfully logged into account with the new password: " + newPassword);
        } else {
            logFailExit("User could not log in with the new password" + newPassword);
        }
        softAssertAll();

    }
}
