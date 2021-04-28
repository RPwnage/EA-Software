package com.ea.originx.automation.scripts.feature.loginregistration;

import com.ea.vx.annotations.TestRail;
import com.ea.vx.originclient.account.AccountManager;
import com.ea.vx.originclient.account.UserAccount;
import com.ea.vx.originclient.client.OriginClient;
import com.ea.vx.originclient.client.OriginClientFactory;
import com.ea.originx.automation.lib.macroaction.MacroAccount;
import com.ea.originx.automation.lib.macroaction.MacroLogin;
import com.ea.originx.automation.lib.pageobjects.account.AccountSettingsPage;
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
 * Test login with Two-Factor Authentication
 *
 * @author cbalea
 */
public class OATwoFactorAuthentication extends EAXVxTestTemplate {
    
    @TestRail(caseId = 3096150)
    @Test(groups = {"gamelibrary", "loginregistration_smoke", "loginregistration", "allfeaturesmoke", "client_only"})
    public void testTwoFactorAuthentication(ITestContext context) throws Exception {
        
        final OriginClient client = OriginClientFactory.create(context);
        
        UserAccount userAccount = AccountManager.getInstance().createNewThrowAwayAccount();
        final String username = userAccount.getUsername();
        
        logFlowPoint("Launch Origin and login as newly registered user " + username); // 1
        logFlowPoint("Go to 'EA Account and Billing' Page"); //2
        logFlowPoint("Turn on 'Two Factor Login Verification'"); //3
        logFlowPoint("Logout and log back in. Verify the 'Two Factor Login Page' is visible"); //4
        logFlowPoint("Click 'Send Verification Code' button and verify user receives email with the security code"); //5
        logFlowPoint("Verify user can login by entering the code at the 'Two Factor Login Page'"); //6

        //1
        WebDriver driver = startClientObject(context, client);
        logPassFail(MacroLogin.startLogin(driver, userAccount), true);

        //2
        final WebDriver browserDriver = client.getAnotherBrowserWebDriver(BrowserType.CHROME);
        final AccountSettingsPage accountSettingsPage = new AccountSettingsPage(browserDriver);
        accountSettingsPage.navigateToIndexPage();
        accountSettingsPage.login(userAccount);
        logPassFail(accountSettingsPage.verifyAccountSettingsPageReached(), true);

        //3
        accountSettingsPage.gotoSecuritySection();
        logPassFail(MacroAccount.turnOnTwoFactorAuthentication(browserDriver, username), true);

        //4
        LoginPage loginPage;
        loginPage = new MainMenu(driver).selectLogOut();

        MacroLogin.startLogin(driver, userAccount,
                new LoginOptions().setNoWaitForDiscoverPage(true).setSkipVerifyingMiniProfileUsername(true));
        logPassFail(loginPage.verifyTwoFactorLoginPageVisible(), true);

        //5
        loginPage.clickTwoFactorSendVerificationCodeButton();
        sleep(30000); // wait for the email to arrive
        EmailFetcher emailFetcher = new EmailFetcher(username);
        String securityCode = emailFetcher.getLatestSecurityCode();
        emailFetcher.closeConnection();
        logPassFail(securityCode != null, true);

        //6
        loginPage.enterTwoFactorLoginSecurityCode(securityCode);
        loginPage.clickTwoFactorSigninButton();
        loginPage.waitForDiscoverPage();
        logPassFail(MacroLogin.verifyMiniProfileUsername(driver, username), true);

        softAssertAll();
    }
}
