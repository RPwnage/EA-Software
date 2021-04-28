package com.ea.originx.automation.scripts.loginregistration;

import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
import com.ea.originx.automation.lib.pageobjects.account.SecurityPage;
import com.ea.originx.automation.lib.pageobjects.login.LoginOptions;
import com.ea.originx.automation.lib.pageobjects.login.LoginPage;
import com.ea.originx.automation.lib.pageobjects.login.SecurityQuestionAnswerPage;
import com.ea.originx.automation.lib.pageobjects.profile.MiniProfile;
import com.ea.originx.automation.scripts.EAXVxTestTemplate;
import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.vx.originclient.helpers.ContextHelper;
import com.ea.vx.originclient.utils.ProcessUtil;
import com.ea.vx.originclient.webdrivers.BrowserType;
import org.openqa.selenium.WebDriver;
import org.testng.ITestContext;
import org.testng.annotations.Test;

/**
 * Verifies that the user can log into Origin by pressing enter
 *
 * @author nvarthakavi
 */
public class OAAutoLoginChangePassword extends EAXVxTestTemplate {

    @TestRail(caseId = 9421)
    @Test(groups = {"loginregistration", "full_regression"})
    public void testAutoLoginChangePassword(ITestContext context) throws Exception {
        boolean isQt = ContextHelper.isOriginClientTesing(context);
        final OriginClient client = OriginClientFactory.create(context);

        final UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final String userName = userAccount.getUsername();
        final String oldPassword = userAccount.getPassword();
        final String newPassword = userAccount.getPassword() + "New";

        logFlowPoint("Launch Origin"); // 1
        logFlowPoint("Login to Origin with 'Remember Me' checked"); // 2
        logFlowPoint("Navigate to the EA Account and Billing Page"); //3
        logFlowPoint("From a browser,Navigate to the Security Page and Change the User password by completing the Security question"); //4
        if (!isQt) {
            logFlowPoint("Log out of Origin and Verify the page navigates back to Login Window"); //5
        }
        logFlowPoint("Attempt to login with the old password and verify that user cannot login to Origin client with the old password and Verify that there is a message is visible");//6
        logFlowPoint("Login with the New Password and Verify successfully"); //7

        // 1
        WebDriver driver = startClientObject(context, client);
        logPassFail(driver != null, false);

        // 2
        logPassFail(MacroLogin.startLogin(driver, userAccount, new LoginOptions().setRememberMe(true), "", "", "", false, SecurityQuestionAnswerPage.DEFAULT_PRIVACY_VISIBLITY_SETTING), true);

        // 3
        WebDriver browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        final AccountSettingsPage accountSettingsPage = new AccountSettingsPage(browserDriver);
        accountSettingsPage.navigateToIndexPage();
        accountSettingsPage.login(userAccount);
        logPassFail(accountSettingsPage.verifyAccountSettingsPageReached(), true);

        // 4
        accountSettingsPage.gotoSecuritySection();
        SecurityPage securityPage = new SecurityPage(browserDriver);
        securityPage.changePassword(userAccount, newPassword);
        logPassFail(securityPage.verifySecuritySectionReached(), true);

        // 5
        if (!isQt) {

            MiniProfile miniProfile = new MiniProfile(driver);
            miniProfile.clickProfileAvatar();
            LoginPage loginPage = miniProfile.selectSignOut();
            logPassFail(loginPage.isLoginBtnVisible(), true);
        }

        // 6
        ProcessUtil.killOriginProcess(client);
        client.stop();
        driver = startClientObject(context, client);
        LoginPage loginPage = new LoginPage(driver);
        loginPage.inputUsername(userName);
        loginPage.inputPassword(oldPassword);
        loginPage.clickLoginBtn();
        logPassFail(loginPage.isInvalidCredentailsBannerVisible(), true);

        // 7
        loginPage.inputPassword(newPassword);
        loginPage.clickLoginBtn();
        logPassFail(!loginPage.isInvalidCredentailsBannerVisible(), true);

        softAssertAll();
    }
}